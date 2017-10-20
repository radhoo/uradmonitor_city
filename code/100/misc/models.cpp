/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */
 
#include "models.h"

uint8_t getType(uint8_t param) {
	switch (param) {
		case 0x11:
		case 0x12: return MODEL_A;
		case 0x13: return MODEL_EXP;
		case 0x14: return MODEL_CITY;
		case 0x41: return MODEL_A2;
		case 0x51: return MODEL_KIT1;
		case 0x82: return MODEL_A3;
		default: return param;
	}
}
