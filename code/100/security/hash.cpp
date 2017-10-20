/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */

#include "hash.h"

//CRC-8 - based on the CRC8 formulas by Dallas/Maxim
uint8_t crc8(const uint8_t *buffer, uint8_t length) {
	uint8_t crc = 0x00;
	for (int i=0;i<length;i++) {
		crc = crc ^ *(buffer++);
		for (int j=0;j<8;j++) {
			if (crc & 1)
					crc = (crc >> 1) ^ 0x8C;
			else
					crc = crc >> 1;
		}
	}
	return crc; // not-complemented!
}

// original CRC32
// note: if the seed is not "big" enough, first values in buffer will not have any influence
uint32_t crc32(uint8_t *buffer, int length, uint32_t seed) {
	uint32_t crc = seed;
	int i, j;
	for (i=0; i<length; i++) {
        crc = crc ^ *(buffer++);
		for (j=0; j<8; j++) {
			if (crc & 1)
		        crc = (crc>>1) ^ 0xEDB88320UL ;
			else
		        crc = crc >>1 ;
        }
    }
	return crc ^ 0xFFFFFFFFUL;
}

