/*
 * UART Library for AVR microcontrollers
 * Copyright (C) 2016, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */

#include <string.h>
#include "uart328.h"

void UART328::init(uint32_t baudrate, bool rx_interrupt) {

	uint32_t baudregister = F_CPU/16/baudrate - 1;

	// Set baud rate; lower byte and top nibble
	UBRR0H = ((baudregister) & 0xF00);
	UBRR0L = (uint8_t) ((baudregister) & 0xFF);

	UCSR0B |= _BV(TXEN0);	// Enable TX
	UCSR0B |= _BV(RXEN0);	// Enable RX

	if (rx_interrupt) {
		UCSR0B |= _BV(RXCIE0);	// Enable interrupt on RX complete
		//#define RX_INTDIS()		UCSR0B &= ~_BV(RXCIE0)	// Disable RX interrupt
		//#define TX_INTEN()		UCSR0B |= _BV(TXCIE0)	// Enable interrupt on TX complete
		//#define TX_INTDIS()		UCSR0B &= ~_BV(TXCIE0)	// Disable TX interrupt
	}

	// Set frame format = 8-N-1
	UCSR0C = (_DATA << UCSZ00);
}

void UART328::stop() {
	UCSR0B &= ~_BV(TXEN0);	// Disable TX
	UCSR0B &= ~_BV(RXEN0);	// Disable RX
}

// Use this function if the RX interrupt is not enabled. Returns 0 on empty buffer
uint8_t UART328::recv(void) {
	// Check to see if something was received
	while (!(UCSR0A & _BV(RXC0)) );
	return (uint8_t) UDR0;
}

bool UART328::isdata() {
	return (UCSR0A & _BV(RXC0));
}

bool UART328::recv(uint8_t *b) {
	// Check to see if something was received
	if (!isdata()) return false;
	*b = (uint8_t) UDR0;
	return true;
}

void UART328::recvline(char *buf, uint16_t lenmax) {
	uint8_t i = 0;
	while (i < lenmax) {
		buf[i] = recv();
		i++;
		buf[i] = 0;
		if (buf[i-1] == '\n') break;
	}
}

void UART328::recvbuf(uint8_t *buf, uint16_t len) {
	for (uint8_t i = 0; i< len; i++)
		buf[i] = recv();
}



//	Use this function if the TX interrupt is not enabled. Blocks the serial port while TX completes
void UART328::send(uint8_t data) {
	// Stay here until data buffer is empty
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = (unsigned char) data;

}

void UART328::send( char *str) {
	while (*str != '\0') send(*str++);
}

void UART328::send( uint8_t *str, uint8_t len) {
	for (uint8_t i = 0; i< len; i++)
		send(str[i]);
}

void UART328::send(char *buffer, uint16_t len, char *szFormat, ...) {
	va_list pArgs;
	va_start(pArgs, szFormat);
	vsnprintf(buffer, len, szFormat, pArgs);
	va_end(pArgs);
	send(buffer);
}

void UART328::send(char *buffer, uint16_t len, const char *szFormat, ...) {
	va_list pArgs;
	va_start(pArgs, szFormat);
	vsnprintf_P(buffer, len, szFormat, pArgs);
	va_end(pArgs);
	send(buffer);
}

