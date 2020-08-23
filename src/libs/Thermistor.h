#ifndef Thermistor_h
#define Thermistor_h

#include "PubSubClient.h"

class Thermistor
{
  public:
    enum Type {
      B3988,
      B4300
    };
#if ESP32 == 1
    Thermistor(byte channel, Type type, String topic, PubSubClient * mqttClient);
#else
    Thermistor(Type type, String topic, PubSubClient * mqttClient);
#endif

    void setup();
    void loop();

  private:
    const float* pvalues;
    byte channel;
    String topic;
    unsigned long next_query;
    PubSubClient* mqttClient;

    float getTemp();
    float getResistance();
    float getADC();

};

#endif
