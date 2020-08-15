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
#if CHIP==esp32
    Thermistor(byte channel, Type type, String topic, PubSubClient * mqttClient);
#else
    Thermistor(Type type, String topic, PubSubClient * mqttClient);
#endif

    void setup();
    void loop();

  private:
    const double* pvalues;
    byte channel;
    String topic;
    unsigned long next_query;
    PubSubClient* mqttClient;

    double getTemp();
    double getResistance();
    double getADC();

};

#endif
