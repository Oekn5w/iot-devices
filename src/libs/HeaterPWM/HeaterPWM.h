#ifndef HeaterPWM_h
#define HeaterPWM_h

#include "PubSubClient.h"

class HeaterPWM
{
  public:
    struct HW_Config
    {
      byte pinPWM;
      byte channel;
      bool PWMActiveLow;
      bool ENActive;
      byte pinEN;
      bool ENActiveLow;
    };

    HeaterPWM(HeaterPWM::HW_Config hw_config, String topicBase, PubSubClient * client);

    void callback(String topic, String payload);
    void setupMQTT();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    HeaterPWM::HW_Config hw_config;
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
