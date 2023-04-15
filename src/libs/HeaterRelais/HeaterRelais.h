#ifndef HeaterRelais_h
#define HeaterRelais_h

#include "PubSubClient.h"

namespace Heater
{

struct HeaterRelaisSettings {
  byte switchPin;
  bool switchActiveHigh = true;
  String topicBase;
  bool ledEnable = false;
  byte ledPin;
  bool ledActiveHigh = true;
};

class HeaterRelais
{
  public:
    HeaterRelais(const HeaterRelaisSettings * pSettings, PubSubClient * client);

    void callback(String topic, String payload);
    void setupMQTT();

    void turnOn();
    void turnOff();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    unsigned long timeout;
    bool state;
    bool statePublished;

    const HeaterRelaisSettings * pSet;

    void publish();
};

}

#endif
