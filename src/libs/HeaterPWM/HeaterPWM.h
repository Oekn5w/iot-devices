#ifndef HeaterPWM_h
#define HeaterPWM_h

#include "PubSubClient.h"

class HeaterPWM
{
  public:
    struct stPinInfo
    {
      byte Pin;
      bool activeHigh;
    };

    struct stHWInfo
    {
      HeaterPWM::stPinInfo PWM;
      byte channel;
      bool useEnable;
      HeaterPWM::stPinInfo Enable;
    };

    HeaterPWM(HeaterPWM::stHWInfo infoHW, String topicBase, PubSubClient * client);

    void callback(String topic, String payload);
    void setupMQTT();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    HeaterPWM::stHWInfo infoHW;
    unsigned long timeoutPWM;
    unsigned long timeoutSwitch;
    String topicBase;
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


#endif
