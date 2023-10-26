#ifndef HeaterRelais_h
#define HeaterRelais_h

#include "PubSubClient.h"

namespace Heater
{

struct HeaterRelaisSettings {
  byte switchPin;
  bool switchActiveHigh = true;
  bool ledEnable = false;
  byte ledPin;
  bool ledActiveHigh = true;
  String topicBase;
};

class HeaterRelais
{
  public:
    HeaterRelais(const HeaterRelaisSettings * pSettings, PubSubClient * client);

    void callback(String topic, const String & payload);
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
