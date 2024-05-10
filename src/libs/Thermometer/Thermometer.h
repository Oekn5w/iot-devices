#ifndef thermometer_h
#define thermometer_h

#include "PubSubClient.h"
#include "nonBlockingDT.h"
#include "Thermometer_config.h"

namespace Thermometer
{
  struct sDevInfo {
    String address;
    float publishedTemperature;
  };

  enum eWireState {
    IDLE, WAITING, ABORTED
  };

  class Wire {
    public:
      Wire() { }
      Wire(byte GPIO_Bus, String base_topic, PubSubClient * mqttClient, unsigned int idBus);
      void setup();
      void loop();
      void readTemperatures();
      void planRescan();
    private:
      OneWire wire;
      nonBlockingDT sensors;

      String baseTopicDev;
      String baseTopicBus;
      unsigned int idBus;
      PubSubClient* mqttClient;
      eWireState wireState;
      sDevInfo devInfo[MAX_NUM_SENS_p_B];

      bool readingPending;
      bool rescanPending;
      void publishBus();
      void publishSensors();

      bool busInfoToPublish;
      bool sensorToPublish;
      
      uint64_t timeConvStart;
      uint32_t numSensors;
  };

  class MultiWire {
    public:
      MultiWire(byte* GPIO_Busses, unsigned int N_Busses, String base_topic, PubSubClient * mqttClient, unsigned long queryInterval);
      void callback(String topic, const String & payload);
      void readTemperatures();
      void setupMQTT();
      void setup();
      void loop();
    private:
      Wire Busses[MAX_NUM_BUSSES];
      unsigned int N_Busses;
      String topicBase;
      unsigned long next_query;
      unsigned long queryInterval;
      PubSubClient* mqttClient;
  };

}

#endif
