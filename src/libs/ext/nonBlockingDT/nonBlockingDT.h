// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _nonBlockingDT_H_
#define _nonBlockingDT_H_
#include "Arduino.h"
#include <DallasTemperature.h>

struct tempSensorInfo {
	DeviceAddress oneWireAddress;
	int16_t lastReadingRaw;
	uint8_t oneWireIndex;
	boolean readingPending;
};

class nonBlockingDT {
public:
	nonBlockingDT() {}
	nonBlockingDT(DallasTemperature *w) :
			ptrDT(w) {
	}
	~nonBlockingDT() {
		if (infoPtr) {
			free(infoPtr);
		}
	}
	void getAddressFromTempSensorIndex(DeviceAddress addr, uint8_t tempSensorIndex);
	void setUseConversionTimer(boolean state);
	boolean startConvertion();
	boolean startConvertion(uint8_t tempSensorIndex);
	boolean isConversionDone();
	boolean getUseConversionTimer();
	uint8_t begin(uint8_t defaultResolution);
	uint8_t getOneWireIndexFromTempSensorIndex(uint8_t tempSensorIndex);
	int16_t getLatestTempRaw(uint8_t tempSensorIndex);
	float getLatestTempC(uint8_t tempSensorIndex);
	float getLatestTempF(uint8_t tempSensorIndex);

private:
	DallasTemperature *ptrDT = nullptr;
	boolean parasiteMode;
	boolean conversionInProcess;
	boolean useConversionTimer = true;
	uint8_t numTempSensors;
	uint32_t waitTime;
	uint32_t conversionStartTime;
	tempSensorInfo *infoPtr = nullptr;
	void updateTemps();
};


#endif /* _nonBlockingDT_H_ */
