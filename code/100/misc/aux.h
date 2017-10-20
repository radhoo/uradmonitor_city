/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */
 
#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define MIN(x,y) (x<y)?x:y

// Reset the microcontroller
void aux_softwarereset();

// Proper reset using the watchdog: untested
void aux_wdreset() ;

// Reads the ADC port specified by i
uint16_t aux_ADCRead(uint8_t i);

double aux_ADCTemp(void);

// return ADC port voltage computed against given VRef, with resistive divider
uint16_t aux_readDivVoltage(double vref, uint16_t divider_R1, uint16_t divider_R2, uint8_t i);

double aux_readVoltage(double vref, uint8_t i);

// not more than 10 chars!
char *aux_detectorName(uint8_t param);

/**
 * Encodes a small double (-127 .. 127) to an 16 bit integer, including sign and two decimals
 */
uint16_t double2int(double val);

/**
 * copyBytes
 * copy specified src_bytes from src to dst at specified offset
 * this function is not safe, as dst is not checked for size. Make sure it is big enough!
 */
uint16_t copyBytes(void *dst, uint16_t dst_offset, void *src, uint8_t src_bytes);



/**
 * jsonKeyFind
 * finds a key and copies its value to the value output pointer
 */
bool jsonKeyFind(char *response, char *key, char *value, uint8_t size, uint8_t offset = 3);

/**
 * getKeyFind
 * finds a key and copies its value to the output pointer
 * example: getKeyFind("/s?u=test&p=123_this%40is.ke+mab123 HTTP/1.1", "u=" .. will return test
 */
bool getKeyFind(char *response, char *key, char *value, uint8_t size);

/*
 * Searches for a substring in the rigth part of a string after a given key
 * The substring should be delimited by two other substrings, startwith and stopwith
 * the result is returned in pointer 'value'
 * True on success, false if the search op didn't find any results
 */
bool find(char *response, char *key, char *startwith, char *stopwith, char *value, uint8_t size);

/**
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
uint32_t hex2int(char *hex);

uint8_t hexdigit(uint8_t digit);


void hex2bin(char *buffer, uint16_t len);

// handle hex codes and plus character for encoded urls
void urldecode(char *urlenc);

/*double recAvg(double new_value, double current_avg, uint16_t current_count) ;
uint16_t recAvg(uint16_t new_value, uint16_t current_avg, uint16_t current_count) ;
uint32_t recAvg(uint32_t new_value, uint32_t current_avg, uint16_t current_count) ;*/
//#define recAvg(new_value, current_avg, current_count) ((new_value + current_avg * current_count) / (current_count + 1))
#define recAvg(new_value, current_avg, current_count) (double)(new_value / (double)(current_count +1) + current_avg * ((double)current_count / (double) (current_count + 1)))

