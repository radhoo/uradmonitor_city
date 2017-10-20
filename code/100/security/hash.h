/*
 * General purpose code
 * Copyright (C) 2015, Radu Motisan , radu.motisan@gmail.com , All rights reserved.
 * http://www.pocketmagic.net/
 */
 
#pragma once

#include <stdint.h>

uint8_t crc8(const uint8_t *buffer, uint8_t length);
uint32_t crc32(uint8_t *buffer, int length, uint32_t seed = 0xFFFFFFFFUL);

