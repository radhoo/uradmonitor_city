/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */

#include <util/delay.h>
#include <string.h>
#include "aux.h"

bool adcRunning = false;

// Reset the microcontroller
void aux_softwarereset() {
	asm ("JMP 0");
	// http://forum.arduino.cc/index.php?topic=78020.0
}

// Proper reset using the watchdog: untested
void aux_wdreset() {
  wdt_enable(WDTO_15MS);
  while(1);
}

// Reads the ADC port specified by i
uint16_t aux_ADCRead(uint8_t i) {
	// enable ADC, select ADC clock = F_CPU / 128 (i.e. 125 kHz)
	if (!adcRunning) {
		ADCSRA = (1<<ADEN | 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0 ); // F_CPU / 128 = 62.5kHz , max resolution
		_delay_ms(1); //very important! allows ADEN to turn on ADC
		adcRunning = true;
	}
	// no REFS0 and no REFS1: AREF, Internal Vref turned off
	// REFS0: AVCC with external capacitor at AREF pin
	// REFS0 + REFS1: Internal 2.56V Voltage Reference with external capacitor at AREF pin
	//ADMUX = (1<<REFS0) | (1<<REFS1) | i;
	ADMUX = (1<<REFS0)  | i;

	// ADSC: start one conversion
	ADCSRA |= 1 << ADSC;
	// wait for conversion
	while (ADCSRA & (1 << ADSC));
	// return 16bit result
	return ADCL | (ADCH << 8);
}

double aux_ADCTemp(void) {
	// enable ADC, select ADC clock = F_CPU / 128 (i.e. 125 kHz)
	if (!adcRunning) {
		ADCSRA = (1<<ADEN | 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0 ); // F_CPU / 128 = 62.5kHz , max resolution
		_delay_ms(1); //very important! allows ADEN to turn on ADC
		adcRunning = true;
	}
	//read ch n of internal a/d  10 bit unsigned
	//ADMUX= 0xC0 + 8; //select 1.1 vref + channel 8
	ADMUX = (1<<REFS0)  | 8; // AVCC vref + chan 8
	// ADSC: start one conversion
	ADCSRA |= 1 << ADSC;
	// wait for conversion
	while (ADCSRA & (1 << ADSC));

	//double volts = ADC * 1.1 / 1024.0;
	double volts = (ADCL | (ADCH << 8)) * 3.3 / 1024.0;

	// convert volts to degrees celsius
	return volts * 25.0/0.314;

	//return   (ADCW - 324.31 ) / 1.22;
}

// return ADC port voltage computed against given VRef, with resistive divider
uint16_t aux_readDivVoltage(double vref, uint16_t divider_R1, uint16_t divider_R2, uint8_t i) {
	return (uint16_t) (aux_ADCRead(i) / 1024.0 * (divider_R1 + divider_R2) / divider_R2 * vref);
}

double aux_readVoltage(double vref, uint8_t i) {
	return aux_ADCRead(i) * vref / 1024.0;
}

/**
 * Encodes a small double (-127 .. 127) to an 16 bit integer, including sign and two decimals
 */
uint16_t double2int(double val) {
	int8_t sintp = (int8_t)val;				//signed integer part
	int8_t sdecp = (val - sintp ) * 100;	//signed decimal part
	uint8_t udecp = sdecp>0?sdecp:-1*sdecp; //removed decimal sign
	uint8_t uintp = sintp + 127;			//convert to unsigned
	uint16_t res = (udecp << 8) | (uint16_t)uintp;		//pack it together
	return res;
}

/**
 * copyBytes
 * copy specified src_bytes from src to dst at specified offset
 * this function is not safe, as dst is not checked for size. Make sure it is big enough!
 */
uint16_t copyBytes(void *dst, uint16_t dst_offset, void *src, uint8_t src_bytes) {
	if (!dst || !src_bytes) return dst_offset;

	for (int i=0; i< src_bytes;i++)
		((uint8_t *)dst)[dst_offset + i] = !src? 0 : ((uint8_t *)src)[src_bytes - i - 1]; // either 0 if empty source or most significant bytes go first
	return dst_offset + src_bytes;
}



/**
 * jsonKeyFind
 * finds a key and copies its value to the value output pointer
 * {"success":"ok"}
 * +CIFSR:APIP,"192.168.4.1"
 *	+CIFSR:STAIP,"192.168.2.106"
 */
bool jsonKeyFind(char *response, char *key, char *value, uint8_t size, uint8_t offset) {
	memset(value, 0, size);

	char *s1 = strstr(response, key);
	uint8_t len = strlen(key);
	if (s1 && len) {
		char *s2 = strstr(s1 + len + offset, "\"");
		if (s2) {
			strncpy(value, s1 + len + offset, MIN(s2 - s1 - len - offset, size - 1) );
			return true;
		}
	}
	return false;
}


/**
 * getKeyFind
 * finds a key and copies its value to the output pointer
 * example: getKeyFind("/s?u=test&p=123_this%40is.ke+mab123 HTTP/1.1", "u=" .. will return test
 */
bool getKeyFind(char *response, char *key, char *value, uint8_t size) {
	memset(value, 0, size);

	char *s1 = strstr(response, key);
	uint8_t len = strlen(key);
	if (s1 && len) {
		char *s2 = strstr(s1 + len, "&");
		char *s3 = strstr(s1 + len, " ");
		if (s2)
			strncpy(value, s1 + len, MIN(s2 - s1 - len, size - 1) );
		else
			strncpy(value, s1 + len, MIN(s3 - s1 - len, size - 1) );
		return true;

	}
	return false;
}
/*
 * Searches for a substring in the rigth part of a string after a given key
 * The substring should be delimited by two other substrings, startwith and stopwith
 * the result is returned in pointer 'value'
 * True on success, false if the search op didn't find any results
 */
bool find(char *response, char *key, char *startwith, char *stopwith, char *value, uint8_t size) {
	memset(value, 0, size);
	char *s1 = strstr(response, key);
	uint8_t len1 = strlen(key);
	if (s1 && len1) {
		char *s2 = strstr(s1 + len1, startwith);
		uint8_t len2 = strlen(startwith);
		if (s2) {
			char *s3 = strstr(s2 + len2, stopwith);
			if (s3) {
				strncpy(value, s2 + len2, MIN(s3 - s2 - len2, size - 1) );
				return true;
			}
		}
	}
	return false;
}


uint8_t hexdigit(uint8_t digit) {
	if (digit >= '0' && digit <= '9') return digit  - '0';
    else if (digit >= 'a' && digit <='f') return digit - ('a' - 10);
    else if (digit >= 'A' && digit <='F') return digit - ('A' - 10);
    else return 0;
}

/**
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
uint32_t hex2int(char *hex) {
    uint32_t val = 0;
     while (*hex) {
    	// shift 4 to make space for new digit, and add the 4 bits of the new digit
    	val = (val << 4) | (hexdigit(*hex++) & 0xF);
    }
    return val;
}

void hex2bin(char *buffer, uint16_t len) {
	for (uint16_t i=0;i<len / 2; i++)
		buffer[i] = (hexdigit(buffer[i*2]) << 4) + hexdigit(buffer[i*2 + 1]);
	buffer[len / 2] = 0;
}

// handle hex codes and plus character for encoded urls
void urldecode(char *urlenc) {
	char *dec = urlenc;
	while (*urlenc) {
	    if (*urlenc == '+')
	    	*dec = ' ';
	    else if (*urlenc == '%')
			*dec = (hexdigit(*++urlenc) << 4) | hexdigit(*++urlenc);
		else
			*dec = *urlenc;
		// next character
	    dec++;
	    urlenc++;
	}
	*dec = 0;
}

/*double recAvg(double new_value, double current_avg, uint16_t current_count) {
	return (new_value + current_avg * current_count) / (current_count + 1) ;
}
uint16_t recAvg(uint16_t new_value, uint16_t current_avg, uint16_t current_count) {
	return (new_value + current_avg * current_count) / (current_count + 1) ;
}
uint32_t recAvg(uint32_t new_value, uint32_t current_avg, uint16_t current_count) {
	return (new_value + current_avg * current_count) / (current_count + 1) ;
}*/
