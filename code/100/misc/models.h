/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */
 
#pragma once

#include <stdint.h>

// define device models
#define MODEL_A 0x1
#define MODEL_A2 0x4
#define MODEL_KIT1 0x5
#define MODEL_D 0x6
#define MODEL_A3 0x8
#define MODEL_EXP 0x13
#define MODEL_CITY 0x14

//1=model_A, 2=model_B, 3=model_C, 4=model_A2, 9=model_0

uint8_t getType(uint8_t param);
