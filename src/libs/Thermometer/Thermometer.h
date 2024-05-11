#ifndef thermometer_h
#define thermometer_h

#include "PubSubClient.h"
#include "nonBlockingDT.h"
#include "Thermometer_config.h"

namespace Thermometer
{
  struct sDevInfo {
    char address[17];
    float publishedTemperature;
  };

  enum eWireState {
    IDLE, WAITING, ABORTED
  };

  class singleBus {
    public:
      singleBus() {}
      singleBus(byte GPIO_Bus, const String & base_topic, PubSubClient * mqttClient, unsigned int idBus = 0);
      ~singleBus();
      void init(byte GPIO_Bus, const String & base_topic, PubSubClient * mqttClient, unsigned int idBus = 0);
      void* operator new (unsigned int);
      void operator delete(void*);
      void setup();
      void loop();
      void readTemperatures();
      void planRescan();
    private:
      OneWire refWire;
      DallasTemperature refDT;
      nonBlockingDT sensors;

      char baseTopicDev[THERMO_LENGTH_DEVICE];
      uint16_t baseTopicDevIdxAddr;
      char baseTopicBus[THERMO_LENGTH_BUS];
      unsigned int idBus;
      PubSubClient* mqttClient;
      eWireState wireState;
      sDevInfo* devInfo = nullptr;

      bool readingPending;
      bool rescanPending;
      void publishBus();
      void publishSensors();

      bool busInfoToPublish;
      bool sensorToPublish;
      
      uint64_t timeConvStart;
      uint32_t numSensors;
  };

  class multiBus {
    public:
      multiBus(byte* GPIO_Busses, unsigned int N_Busses, const String & base_topic, PubSubClient * mqttClient, unsigned long queryInterval);
      ~multiBus();
      void callback(String topic, const String & payload);
      void readTemperatures();
      void setupMQTT();
      void setup();
      void loop();
    private:
      singleBus** Busses = nullptr;
      unsigned int N_Busses;
      String topicBase;
      unsigned long next_query;
      unsigned long queryInterval;
      PubSubClient* mqttClient;
  };

}

#endif
