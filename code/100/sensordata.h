#pragma once

#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "config.h"
#include "misc/aux.h"
#include "security/hash.h"
#include "misc/adc.h"

#define				SENSORS_COUNT			5

#define 				READ_ERROR				0
#define					READ_COMMAND			1
#define					READ_OK					2
#define					READ_IDLE				3

#define					WARMUP_INPROGRESS		0
#define					WARMUP_NOW				1
#define					WARMUP_DONE				2

#define					P_TUBEVOLFB_PIN			PC0

class SensorData {
	uint32_t 			deviceID;						// device ID will be initialized later
	double				data[SENSORS_COUNT],
						temperature;
	uint16_t			count;							// sensor data is averaged

	// E3 implementation
	uint8_t 			dataArray[60],					// data structure: create a pointer to hold all values in order, like concatenated:
						offset;							// most significant bytes go first (natural reading)

	//ADC10b				adc;
public:


	SensorData();

	void setDeviceID(uint32_t id);
	uint32_t getDeviceID();

	void initSensors() {
	//	adc.start();
	}

	double* getData() { return data; }

	double getInternalTemperature() {
		return temperature;
	}

	uint8_t readSensors();

	uint16_t packData(char *szClient, uint16_t len, uint16_t verHW, uint16_t verSW, uint32_t time) ;

	void resetData();

};
