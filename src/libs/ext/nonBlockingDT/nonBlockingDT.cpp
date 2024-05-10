// Do not remove the include below
#include "nonBlockingDT.h"

uint8_t nonBlockingDT::begin(uint8_t defaultResolution = 9) {
	uint8_t index1, index2;
	DeviceAddress addr;
	if(!ptrDT){return 0;}

	ptrDT->begin();
	ptrDT->setWaitForConversion(false);
	numTempSensors = 0;
	if (infoPtr) {
		free(infoPtr);
		infoPtr = nullptr;
	}
	parasiteMode = ptrDT->isParasitePowerMode();
	numTempSensors = ptrDT->getDS18Count();
	if (numTempSensors == 0) {
		return 0;
	}

	// Get memory space for the number of temp sensors located
	infoPtr = (tempSensorInfo *) malloc(
			numTempSensors * sizeof(tempSensorInfo));
	if (infoPtr == NULL) {
		return 0;
	}

	// Run through OneWire indexes again and get addresses for the DS18xxx-type devices
	index2 = 0;
	for (index1 = 0; index1 < ptrDT->getDeviceCount(); index1++) {
		if (!ptrDT->getAddress(addr, index1)) {
			free(infoPtr);
			return 0;
		}
		if (ptrDT->validFamily(addr)) {
			memcpy((infoPtr + index2)->oneWireAddress, addr,
					sizeof(DeviceAddress));
			ptrDT->setResolution((infoPtr + index2)->oneWireAddress,
					defaultResolution);
			(infoPtr + index2)->lastReadingRaw = DEVICE_DISCONNECTED_RAW;
			(infoPtr + index2)->oneWireIndex = index1;
			(infoPtr + index2)->readingPending = false;
			index2++;
		}
	}

	conversionInProcess = false;
	return numTempSensors;
}

boolean nonBlockingDT::startConvertion(uint8_t tempSensorIndex) {
	if(!ptrDT){return false;}
	boolean success;
	if (!isConversionDone()) {
		return false;
	}
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	success = ptrDT->requestTemperaturesByAddress(
			(infoPtr + tempSensorIndex)->oneWireAddress);
	if (success) {
		conversionInProcess = true;
		conversionStartTime = millis();
		waitTime = ptrDT->millisToWaitForConversion(
				ptrDT->getResolution());
		(infoPtr + tempSensorIndex)->readingPending = true;
	}
	return success;
}

boolean nonBlockingDT::startConvertion() {
	if(!ptrDT){return false;}
	uint8_t index;
	if (!isConversionDone()) {
		return false;
	}
	ptrDT->requestTemperatures();
	conversionInProcess = true;
	conversionStartTime = millis();
	waitTime = ptrDT->millisToWaitForConversion(
			ptrDT->getResolution());
	for (index = 0; index < numTempSensors; index++) {
		(infoPtr + index)->readingPending = true;
	}
	return true;
}

int16_t nonBlockingDT::getLatestTempRaw(uint8_t tempSensorIndex) {
	if(!ptrDT){return DEVICE_DISCONNECTED_RAW;}
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return (infoPtr + tempSensorIndex)->lastReadingRaw;
}

float nonBlockingDT::getLatestTempC(uint8_t tempSensorIndex) {
	if(!ptrDT){return DEVICE_DISCONNECTED_C;}
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return ptrDT->rawToCelsius(
			(infoPtr + tempSensorIndex)->lastReadingRaw);
}

float nonBlockingDT::getLatestTempF(uint8_t tempSensorIndex) {
	if(!ptrDT){return DEVICE_DISCONNECTED_F;}
	(void) isConversionDone();
	tempSensorIndex = constrain(tempSensorIndex, 0, numTempSensors - 1);
	return ptrDT->rawToFahrenheit(
			(infoPtr + tempSensorIndex)->lastReadingRaw);
}

boolean nonBlockingDT::isConversionDone() {
	if(!ptrDT){return false;}
	boolean done = false;
	if (conversionInProcess) {
		if (parasiteMode || useConversionTimer) {
			if (millis() - conversionStartTime >= waitTime) {
				done = true;
				conversionInProcess = false;
				updateTemps();
			}
		} else if (ptrDT->isConversionComplete()) {
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
	if(!ptrDT){return;}
	uint8_t index;
	int16_t raw;
	for (index = 0; index < numTempSensors; index++) {
		if ((infoPtr + index)->readingPending) {
			raw = ptrDT->getTemp((infoPtr + index)->oneWireAddress);
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
