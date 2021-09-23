#ifndef Heater_h
#define Heater_h

#define PAYLOAD_HEATER_ON "ON"
#define PAYLOAD_HEATER_OFF "OFF"

#include "PubSubClient.h"

class Heater
{
  public:
    Heater(byte pin, String topicStatus, PubSubClient * client);

    void turnOn();
    void turnOff();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    unsigned long timeout;
    String topicStatus;
    byte pin;
    bool state;
    bool statePublished;

    void publish();
};


#endif
