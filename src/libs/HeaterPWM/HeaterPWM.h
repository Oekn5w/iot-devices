#ifndef HeaterPWM_h
#define HeaterPWM_h

#define PAYLOAD_HEATER_ON "ON"
#define PAYLOAD_HEATER_OFF "OFF"

#include "PubSubClient.h"

class HeaterPWM
{
  public:
    HeaterPWM(byte pin, byte channel, String topicStatus, PubSubClient * client);

    void processTarget(uint16_t dutyCycle);
    void turnOff();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    unsigned long timeout;
    String topicStatus;
    byte pin;
    byte channel;
    uint16_t state;
    bool statePublished;

    void publish();
};


#endif
