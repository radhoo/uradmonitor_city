#include "sensordata.h"
#include <string.h>

SensorData::SensorData() {
	deviceID = 0;
	count = 0;

	memset(dataArray, 0, sizeof(dataArray));
	offset = 0;
}

void SensorData::setDeviceID(uint32_t id) {
	deviceID = id;
}

uint32_t SensorData::getDeviceID() {
	return deviceID;
}


uint8_t SensorData::readSensors() {
	uint8_t	sensors[SENSORS_COUNT] = { PC0, PC2, PC6, PC3, PC1 };

	for (uint8_t i = 0; i < SENSORS_COUNT; i++) {
		data[i] = recAvg(aux_readVoltage(VREF, sensors[i]), data[i], count);
	}

	// get multiple samples for internal temperature
	for (uint8_t i = 0; i < 10 ; i++) {
		temperature = recAvg(aux_readVoltage(VREF, 8) * 25.0 / 0.314, temperature, count);
	}

	count ++;

	// reading done
	return READ_OK;
}

uint16_t SensorData::packData(char *szClient, uint16_t len, uint16_t verHW, uint16_t verSW, uint32_t time) {

	if (len < 84) return 0;

	uint32_t tmp32 = 0;
	uint16_t offset = 0, tmp16 = 0;

	return offset;
}


void SensorData::resetData() {
	count = 0;
}

