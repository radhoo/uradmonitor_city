/*
 * UART Library for AVR microcontrollers
 * Copyright (C) 2016, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */

#pragma once

#include <stdio.h>
#include <avr/io.h>

//volatile static voidFuncPtru08 UartRxFunc;
//typedef void (*tUARTReceiverFunc)(unsigned char);

class UART328 {
	#define _DATA			0x03					// Number of data bits in frame = byte tranmission
	 char rxstr[100];

public:
	void init(uint32_t baudrate, bool rx_interrupt);
	bool isdata();
	uint8_t recv(void);
	bool recv(uint8_t *b);
	void recvline(char *buf, uint16_t lenmax);
	void recvbuf(uint8_t *buf, uint16_t len);
	void send(uint8_t data);
	void send(char *str);
	void send(uint8_t *str, uint8_t len);
	void send(char *buffer, uint16_t len, char *szFormat, ...); // normal array variant
	void send(char *buffer, uint16_t len, const char *szFormat, ...); // PSTR variant
	void stop();
};
