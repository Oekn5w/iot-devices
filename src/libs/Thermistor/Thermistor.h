#ifndef Thermistor_h
#define Thermistor_h

#include "PubSubClient.h"

class Thermistor
{
  public:
    struct strTypeValues {
      strTypeValues() :
        useSH(false), order0(0.0f), order1(0.0f), order2(0.0f), order3(0.0f) {};
      strTypeValues(bool use, float d0, float d1, float d2, float d3) :
        useSH(use), order0(d0), order1(d1), order2(d2), order3(d3) {};
      bool useSH; // use Steinhart-Hart formula
      float order0;
      float order1;
      float order2;
      float order3;
    };

    enum Type {
      B3988,
      B4300
    };
#if ESP32 == 1
    Thermistor(byte channel, Type type, float R1, String topic, PubSubClient * mqttClient);
#else
    Thermistor(Type type, float R1, String topic, PubSubClient * mqttClient);
#endif

    void setup();
    void loop();

  private:
    strTypeValues typeValues;
    byte channel;
    float R1;
    String topic;
    unsigned long next_query;
    PubSubClient* mqttClient;

    float publishedTemperature;

    float getTemp();
    float getResistance();
    float getADC();

};

#endif
