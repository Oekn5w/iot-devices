#ifndef HeaterPWM_h
#define HeaterPWM_h

#include "PubSubClient.h"

namespace Heater
{

struct HeaterPWMSettings {
  byte outPin;
  byte outChannel;
  bool outActiveHigh = true;
  bool fbEnable = true;
  byte fbPin;
  bool fbActiveHigh = true;
  bool senseEnable = false;
  byte sensePin;
  bool ledEnable = false;
  byte ledPin;
  bool ledActiveHigh = true;
  String topicBase;
};

class HeaterPWM
{
  public:
    HeaterPWM(const HeaterPWMSettings * pSettings, PubSubClient * client);

    void callback(String topic, const String & payload);
    void setupMQTT();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    const HeaterPWMSettings * pSet;
    unsigned long timeoutPWM;
    unsigned long timeoutSwitch;
    bool bSwitch;
    uint16_t dC_Standby;
    uint16_t dC;
    bool published;

    void turnOff();
    void processPower(float power);
    void processDutyCycle(uint16_t dutyCycle);
    void processSwitch(bool newState);
    void actuateNewDC(uint16_t dutyCycle);
    void publish();
};

}

#endif
