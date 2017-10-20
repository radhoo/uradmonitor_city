/*
 * File:       	uRADMonitor.cpp
 * Version:    	v101 / 2017
 * Date:       	2017
 * License:		All rights reserved. Property of Magnasci SRL Romania
 * 
 * uRADMonitor model CITY board code
 * 
 * Copyright (C) 2013-2015 Radu Motisan, radu.motisan@gmail.com
 * Copyright (C) 2015-2017 Magnasci SRL, radu.motisan@magnasci.com

 * http://www.pocketmagic.net
 * http://www.uradmonitor.com
 * http://www.magnasci.com
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// local headers
#include "config.h"

#include "misc/aux.h"
#include "misc/models.h"
#include "time/timecounter.h"
#include "time/watchdog.h"
#include "gpio/DigitalPin.h"
#include "security/hash.h"
#include "uart/uart328.h"
#include "sensordata.h"



#define 			READ_ERROR				0
#define				READ_COMMAND			1
#define				READ_OK					2
#define				READ_IDLE				3


/************************************************************************************************************************************************/
/* Constants                                                            																		*/
/************************************************************************************************************************************************/

// constants for this device
#define				MAX_UINT32				0xFFFFFFFF //2^32 = 4294967296 , cpm count range 0..4294967295
#define				BUFFER_SIZE				500		// to be able to handle entire answer
#define				EEPROM_ADDR_DEVID		0x0		// address when the DEVID is being stored 1024Bytes available on atmega328p	: 4B
#define				EEPROM_ADDR_SENDINT		0x4		// address to store the send interval
#define				EEPROM_ADDR_APSSID		0x8		// 20 bytes
#define				EEPROM_ADDR_APKEY		0x1C	// 20 bytes
#define				EEPROM_AP_LEN			20

/************************************************************************************************************************************************/
/* Global Objects                                                       																		*/
/************************************************************************************************************************************************/
uint32_t			deviceNumber			= FORCE_DEV_NUMBER;	 	// If null, Dynamic ID  is enabled . Add a non null value to set number manually
uint16_t 			sendInterval			= FORCE_SEND_INTERVAL; 	// no default value, just set here if you want default!
char				szAPSSID[EEPROM_AP_LEN+1]	= FORCE_AP_SSID,
					szAPPassword[EEPROM_AP_LEN+1]	= FORCE_AP_PASSWORD,
					szLIP[20+1]				= {0},
					szStatus[20+1]			= {0};


Watchdog			wd;								// used to protect device from code deadlocks by issueing an autoreset when needed
UART328				uart;
uint32_t 			pcktsOK					= 0,
					pcktsTotal				= 0;
uint16_t			verSW					= VER_SW,
					verHW					= VER_HW;
uint8_t 			cmdRead 				= READ_ERROR,	// ask to read sensors
					cmdSend 				= 0,	// set 1 to send data, set in timer every minute
					cmdWarmupDone			= WARMUP_INPROGRESS;

TimeCounter 		time;							// handles the time interrupts
char				buffer[BUFFER_SIZE + 1]	= {0},
					szClient[BUFFER_SIZE+1]	= { 0 };// for the tcp client, use a buffer that keeps its value until callback is called

DigitalPin			pwrSensors(&PORTD, PD3),
					pwrModem(&PORTD, PD4);

SensorData			data;

/************************************************************************************************************************************************/
/* Network interface                                                    																		*/
/************************************************************************************************************************************************/
#define				URADMONITOR_SERVER 		"data.uradmonitor.com" 	// The name of the virtual host which you want to  contact to - hostname
#define 			URADMONITOR_SCRIPT 		"/api/v1/upload/city/"	// server resource to call

uint16_t			lastHTTPCode			= 0;	// http last code

enum {
	PAGE_ROOT,
	PAGE_JSON,
	PAGE_CONFIG,
	PAGE_GET,
	PAGE_NOTFOUND
};

// callback function called from the timecounter object when a full minute has elapsed
void callback_timeMinute() {
}


// called on elapsed second
void callback_timeSecond() {
	// read sensors even in warmup, to prepare them
	// on error, retry reading sensors next second
	if (cmdRead == READ_ERROR) cmdRead = READ_COMMAND;
	// issue READ command every given interval
	if (time.getTotalSec() % READ_INTERVAL == 0) {
		// read rest of sensors
		cmdRead = READ_COMMAND;
	}

	// issue SEND command every given interval
	if (time.getTotalSec() >= WARMUP) {
			if (cmdWarmupDone == WARMUP_INPROGRESS) cmdWarmupDone = WARMUP_NOW;
			if (time.getTotalSec() % sendInterval == 0 && time.getTotalSec() > 0) {
				// re-set HTTP code
				lastHTTPCode = 0;
				// ask to send data online
				cmdSend = 1;
			}
	}
}

// watchdog overflow interrupt, set to 1sec
ISR (WDT_vect) { wd.timerEvent(); }

// timer0 overflow interrupt   event to be executed every  2.048ms here when on 8MHz, and every 1.024ms for 16MHz */
ISR (TIMER0_OVF_vect) { time.TimerEvent(); }

// Callback function for client HTTP connection result
// eg. server reply: {"success":"ok"}
void callback_browser(uint16_t webstatuscode, char *serverAnswer) {
	lastHTTPCode = webstatuscode;
	// if the server replied OK, we reset the watchdog to prevent auto-reset. This is currently the only point in code where we do this.
	if (webstatuscode == 200) {
		char value[10] = {0}; // this has to be inited, or when parsing we could have extra junk screwing results
		pcktsOK ++;
		wd.wdt_my_reset();
	}
}

// This function is called upon a HARDWARE RESET, before main() :
void early_run(void) __attribute__((naked)) __attribute__((section(".init3")));

// disable WDT on reboot, to avoid any deadlocks
void early_run(void) {
	Watchdog::wdt_first_disable(); // call as static: object might not be ready yet
}


// waitserial, a function to keep on reading on a serial pipe until timeout or a predefined string is found
bool waitSerial(char *buffer, uint16_t len, UART328 *uart, TimeCounter *time, uint32_t timeout, char *waitresponse) {
	uint32_t start = time->getTotalSec();
	// listen for response
	memset(buffer, 0, len);
	uint16_t count = 0;
	// reset result to avoid infinite loop
	while (time->getTotalSec() - start < timeout && count < len) {
		// we keep on reading until timeout and compare buffer to predefined values
		if (uart->isdata()) {
			uint8_t ch = 0; uart->recv(&ch);
			// add non zeros to our buffer
			if (ch != 0) {
				buffer[count++] = ch;
				if (waitresponse) {
					if (strstr(buffer, waitresponse)) {
						return 1;
					}
				} else {
					if (strstr_P(buffer, PSTR("OK\r\n"))) return 1;
					if (strstr_P(buffer, PSTR("no change\r\n"))) return 1;
					if (strstr_P(buffer, PSTR("FAIL\r\n"))) return 0;
					if (strstr_P(buffer, PSTR("ERROR\r\n"))) return 0;
					if (strstr_P(buffer, PSTR("DNS Fail\r\n"))) return 0;
					if (strstr_P(buffer, PSTR("wrong syntax\r\n"))) return 0;
				}
			}

		}
	}
	return 0; // on timeout we fail
}

/**
 * sendSerial
 * this function sends formatted data to ESP, and waits on timeout response. Actual string returned is stored in the buffer
 * waitresponse is a specific string that you can use to force the function wait for
 */
bool sendSerial(char *buffer, uint16_t len, UART328 *uart, TimeCounter *time, uint32_t timeout, char *waitresponse, const char *szFormat, ...) {
	// prepare formatted string
	va_list pArgs;
	va_start(pArgs, szFormat);
	vsnprintf_P(buffer, len, szFormat, pArgs);
	va_end(pArgs);
	// send string
	uart->send(buffer);

	return waitSerial(buffer, len, uart, time, timeout, waitresponse);
}

void connectAP() {
	if (strlen(szAPSSID) && !sendSerial(buffer, BUFFER_SIZE, &uart, &time, 30, 0, PSTR("AT+CWJAP=\"%s\",\"%s\"\r\n"), szAPSSID, szAPPassword)) {
		//morse.encode('o'); // --- beeep beeep beeep
		//aux_softwarereset();
	}
	// start server mode
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CIPSERVER=1,80\r\n"));

	// check IP, should be two: server and client
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CIFSR\r\n"));
	jsonKeyFind(buffer, "CIFSR:STAIP", szLIP, sizeof(szLIP), 2);

	strcpy_P(szStatus, (szLIP[0] == '0'?PSTR("disconnected"):PSTR("connected")));
}

void serveHTML(char *in, uint8_t page, char *out) {
	
}


/************************************************************************************************************************************************/
/* Main entry point                                                    																			*/
/************************************************************************************************************************************************/
int main(void) {
//while(1) ;
	// 1.setup watchdog
	wd.wdt_init(DEFAULT_WATCHDOG_INTERVAL);

	// 2.config


	// 3.CREATE Timer T0 to count seconds
	time.init(callback_timeSecond, callback_timeMinute);

	uart.init(9600, 0);
	sei();

	// 6.Init Device ID
	// if we have a manual device ID, we use that, if not, we go for Dynamic ID (server received)
	eeprom_busy_wait();
	if (deviceNumber == 0) {
		deviceNumber = eeprom_read_dword((uint32_t *) EEPROM_ADDR_DEVID);
	}
	if (sendInterval == 0) {
		sendInterval = eeprom_read_word((uint16_t *) EEPROM_ADDR_SENDINT);
		if (sendInterval == 0xFFFF || sendInterval < 5 || sendInterval > 3600)
			sendInterval = DEFAULT_SEND_INTERVAL; // default to 1 min send interval
	}

	eeprom_read_block((void *)&szAPSSID, (uint32_t *) EEPROM_ADDR_APSSID, EEPROM_AP_LEN);
	eeprom_read_block((void *)&szAPPassword, (uint32_t *) EEPROM_ADDR_APKEY, EEPROM_AP_LEN);
	if (szAPSSID[0] == 0xFF) {
		strncpy(szAPSSID, FORCE_AP_SSID, EEPROM_AP_LEN);
		strncpy(szAPPassword, FORCE_AP_PASSWORD, EEPROM_AP_LEN);
	}


	// set WDT reset interval against sendInterval
	wd.wdt_setRebootSeconds(WATCHDOG_INTERVAL(sendInterval));

	// configure device ID
	data.setDeviceID(((uint32_t) DEV_CLASS << 24) | ((uint32_t) deviceNumber & 0x00FFFFFF));

	// configure devices
	pwrModem = 1;
	pwrSensors = 1;

	// configure WLAN:
	// go for reset
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, "ready\r\n", PSTR("AT+RST\r\n"));
	// get firmware info
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+GMR\r\n"));
	// enable watchdog
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CSYSWDTENABLE\r\n"));
	// set it as AP + STATION
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CWMODE=3\r\n"));
	// configure AP SSID, PASSWORD, CHANNEL and ENCRYPTION
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CWSAP=\"uRADMonitor-%02X\",\"%08lX\",9,4\r\n"), (uint8_t)(data.getDeviceID() & 0xFF), 0);
	// set multiple connections server / client
	sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CIPMUX=1\r\n"));
	// connect to WLAN if we have credentials
	connectAP();

	// 7.Main program loop
	while (1) {

		if (cmdRead == READ_COMMAND) {
			// the very first time the warmup is complete, reset data before reading the correct values
			if (cmdWarmupDone == WARMUP_NOW) {
				cmdWarmupDone = WARMUP_DONE;
				data.resetData();
			}
			// ready to read data
			cmdRead = data.readSensors();

		}

		// CLIENT
		// Send CLIENT Data, only if sensors are read
		// set 1 to send data, set in timer every minute
		if (cmdSend && cmdRead == READ_OK) {
			// prepare data as E3
			data.packData(szClient, 120, verHW, verSW, time.getTotalSec());
			// open socket
			if (sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CIPSTART=4,\"TCP\",\"%s\",80\r\n"), URADMONITOR_SERVER)) {
				sprintf_P(buffer, PSTR("POST %s%s HTTP/1.1\r\nHost: %s\r\n\r\n"),
					URADMONITOR_SCRIPT, szClient, URADMONITOR_SERVER);
				uint16_t len = strlen(buffer);
				// send data
				if (sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, ">", PSTR("AT+CIPSEND=4,%d\r\n"), len)) {
					// after sending data, we wait for server reply before anything else
					if (sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, "CLOSED\r\n", PSTR("POST %s%s HTTP/1.1\r\nHost: %s\r\n\r\n"),
							URADMONITOR_SCRIPT, szClient, URADMONITOR_SERVER)) {
						if (strstr_P(buffer, PSTR("+IPD,4"))) {
							char value[10] = {0};
							find(buffer, "HTTP/", " ", " ", value, 10);

							callback_browser(atoi(value), buffer);
						}
						// close connection
						sendSerial(buffer, BUFFER_SIZE, &uart, &time, 10, 0, PSTR("AT+CIPCLOSE=4\r\n"));
					}
				}
			}
			pcktsTotal ++;
			data.resetData();
			cmdSend = 0; // stop sending data until next minute
		}

	}//main while loop
	return (0);
}

