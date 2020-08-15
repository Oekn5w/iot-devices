#ifndef Heater_h
#define Heater_h

#define PAYLOAD_HEATER_ON "ON"
#define PAYLOAD_HEATER_OFF "OFF"

#include "PubSubClient.h"

class Heater
{
  public:
    Heater(byte pin, String topic_status, PubSubClient * mqttclient);

    void turn_on();
    void turn_off();

    void setup();
    void loop();
  private:
    PubSubClient * mqttclient;
    unsigned long timeout;
    String topic_status;
    byte pin;
    bool state;
    bool state_published;

    void publish();
};


#endif
