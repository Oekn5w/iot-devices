// Do not remove the include below
#include "nonBlockingDT.h"
#include <Arduino.h>

uint8_t nonBlockingDT::begin(uint8_t defaultResolution = 9) {
	uint8_t index1, index2;
	DeviceAddress addr;

	Serial.println(F("nBlDT: begin"));
	DallasTemperature::begin();
	DallasTemperature::setWaitForConversion(false);
	numTempSensors = 0;
	parasiteMode = DallasTemperature::isParasitePowerMode();
	numTempSensors = DallasTemperature::getDS18Count();
	if (numTempSensors == 0) {
		return 0;
	}
	Serial.println(F("nBlDT: First thresh"));

	// Run through OneWire indexes again and get addresses for the DS18xxx-type devices
	index2 = 0;
	for (index1 = 0; index1 < DallasTemperature::getDeviceCount(); index1++) {
		if (!DallasTemperature::getAddress(addr, index1)) {
			return 0;
		}
		if (DallasTemperature::validFamily(addr)) {
			memcpy((infoPtr + index2)->oneWireAddress, addr,
					sizeof(DeviceAddress));
			DallasTemperature::setResolution((infoPtr + index2)->oneWireAddress,
					defaultResolution);
			(infoPtr + index2)->lastReadingRaw = DEVICE_DISCONNECTED_RAW;
			(infoPtr + index2)->oneWireIndex = index1;
			(infoPtr + index2)->readingPending = false;
			index2++;
			if (index2 == MAX_NBL_SENS) { break; }
		}
	}
	if (numTempSensors > MAX_NBL_SENS) { numTempSensors = MAX_NBL_SENS; }

	conversionInProcess = false;
	return numTempSensors;
}

boolean nonBlockingDT::startConvertion(uint8_t tempSensorIndex) {
	boolean success;
	if (!isConversionDone()) {
		return false;
	}
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	success = DallasTemperature::requestTemperaturesByAddress(
			(infoPtr + tempSensorIndex)->oneWireAddress);
	if (success) {
		conversionInProcess = true;
		conversionStartTime = millis();
		waitTime = DallasTemperature::millisToWaitForConversion(
				DallasTemperature::getResolution());
		(infoPtr + tempSensorIndex)->readingPending = true;
	}
	return success;
}

boolean nonBlockingDT::startConvertion() {
	uint8_t index;
	if (!isConversionDone()) {
		return false;
	}
	DallasTemperature::requestTemperatures();
	conversionInProcess = true;
	conversionStartTime = millis();
	waitTime = DallasTemperature::millisToWaitForConversion(
			DallasTemperature::getResolution());
	for (index = 0; index < numTempSensors; index++) {
		(infoPtr + index)->readingPending = true;
	}
	return true;
}

int16_t nonBlockingDT::getLatestTempRaw(uint8_t tempSensorIndex) {
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return (infoPtr + tempSensorIndex)->lastReadingRaw;
}

float nonBlockingDT::getLatestTempC(uint8_t tempSensorIndex) {
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return DallasTemperature::rawToCelsius(
			(infoPtr + tempSensorIndex)->lastReadingRaw);
}

float nonBlockingDT::getLatestTempF(uint8_t tempSensorIndex) {
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return DallasTemperature::rawToFahrenheit(
			(infoPtr + tempSensorIndex)->lastReadingRaw);
}

boolean nonBlockingDT::isConversionDone() {
	boolean done = false;
	if (conversionInProcess) {
		if (parasiteMode || useConversionTimer) {
			if (millis() - conversionStartTime >= waitTime) {
				done = true;
				conversionInProcess = false;
				updateTemps();
			}
		} else if (DallasTemperature::isConversionComplete()) {
			done = true;
			conversionInProcess = false;
			updateTemps();
		}
	} else {
		done = true;
	}
	return done;
}

void nonBlockingDT::updateTemps() {
	uint8_t index;
	int16_t raw;
	for (index = 0; index < numTempSensors; index++) {
		if ((infoPtr + index)->readingPending) {
			raw = DallasTemperature::getTemp((infoPtr + index)->oneWireAddress);
			(infoPtr + index)->lastReadingRaw = raw;
			(infoPtr + index)->readingPending = false;
		}
	}
}

void nonBlockingDT::getAddressFromTempSensorIndex(DeviceAddress addr,
		uint8_t tempSensorIndex) {
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	memcpy(addr, (infoPtr + tempSensorIndex)->oneWireAddress,
			sizeof(DeviceAddress));
}

uint8_t nonBlockingDT::getOneWireIndexFromTempSensorIndex(
		uint8_t tempSensorIndex) {
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return (infoPtr + tempSensorIndex)->oneWireIndex;
}

boolean nonBlockingDT::getUseConversionTimer() {
	return useConversionTimer;
}

void nonBlockingDT::setUseConversionTimer(boolean state) {
	useConversionTimer = state;
}
