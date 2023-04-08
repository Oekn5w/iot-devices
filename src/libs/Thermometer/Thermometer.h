#ifndef thermometer_h
#define thermometer_h

#include "PubSubClient.h"
#include "Thermometer_config.h"
#include "nonBlockingDT.h"

#define MAX_LENGTH_THERM_ (20)

namespace Thermometer
{
  struct SingleEntry {
    
  }

  struct Single {
    String ID;
    String topic;
    float publishedTemperature;
  }

  class Wire {
    public:
      Wire(byte GPIO_Bus, String base_topic, PubSubClient * mqttClient);
      void setup();
      void loop();
      void 
    private:
      OneWire wire;
      nonBlockingDT sensors;

      String base_topic;
      unsigned long next_query;
      PubSubClient* mqttClient;
  }

  class MultiWire {
    public:
      MultiWire(byte* GPIO_Busses, unsigned int N_Busses, String base_topic, PubSubClient * mqttClient);
      void setup();
      void loop();
    private:
      Wire* Busses;
      unsigned int N_Busses;
      String base_topic;
      unsigned long next_query;
      PubSubClient* mqttClient;
  }

}

#endif
