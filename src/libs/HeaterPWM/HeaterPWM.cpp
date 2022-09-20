#include "HeaterPWM.h"
#include "HeaterPWM_config.h"

HeaterPWM::HeaterPWM(HeaterPWM::HW_Config hw_config, String topicBase, PubSubClient * client)
{
  this->hw_config = hw_config;
  this->mqttClient = client;
  this->topicBase = topicBase;
  this->dC = 0;
  this->dC_Published = false;
}

void HeaterPWM::setup()
{
  ledcSetup(this->hw_config.channel, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(this->hw_config.pinPWM, this->hw_config.channel);
#if PWM_CAPPED > 0
  #if PWM_INVERSE > 0
  ledcWrite(this->hw_config.channel, PWM_DC_MAX - PWM_DC_CAP_LOW);
  #else
  ledcWrite(this->hw_config.channel, PWM_DC_CAP_LOW);
  #endif
#else
  #if PWM_INVERSE > 0
  ledcWrite(this->hw_config.channel, PWM_DC_MAX);
  #else
  ledcWrite(this->hw_config.channel, 0);
  #endif
#endif
  this->timeout = 0;
}

void HeaterPWM::loop()
{
  if(this->state && millis() > this->timeout)
  {
    this->turnOff();
  }
  if(!this->statePublished && mqttClient->connected())
  {
    this->publish();
  }
}

void HeaterPWM::callback(String topic, String payload)
{

}

void HeaterPWM::setupMQTT()
{
}

void HeaterPWM::publish()
{
  if(mqttClient->connected())
  {
    char msg[MSG_BUFFER_SIZE];
    snprintf(msg, MSG_BUFFER_SIZE, "%d", this->state);
    mqttClient->publish(topicStatus.c_str(), msg, true);
    this->statePublished = true;
  }
  else
  {
    this->statePublished = false;
  }
}

void HeaterPWM::processPower(float power)
{

}

void HeaterPWM::processDutyCycle(uint16_t dutyCycle)
{
}

void HeaterPWM::processSwitch(bool newState)
{

}

void HeaterPWM::actuateNewDC(uint16_t dutyCycle)
{
  if(dutyCycle > PWM_DC_MAX)
  {
    dutyCycle = 0;
  }
  if(!dutyCycle)
  {
    this->timeoutPWM = -1;
    this->timeoutSwitch= -1;
  }
  if (this->dC != dutyCycle)
  {
  #if PWM_CAPPED > 0
    uint16_t capped = dutyCycle;
    if (capped > PWM_DC_CAP_HIGH) capped = PWM_DC_CAP_HIGH;
    #if PWM_ALWAYS > 0
    if (capped < PWM_DC_CAP_LOW) capped = PWM_DC_CAP_LOW;
    #else
    if (capped && capped < PWM_DC_CAP_LOW) capped = PWM_DC_CAP_LOW;
    #endif
    if (this->hw_config.PWMActiveLow)
    {
      ledcWrite(this->hw_config.channel, PWM_DC_MAX - capped);
    }
    else
    {
      ledcWrite(this->hw_config.channel, capped);
    }
  #else
    if (this->hw_config.PWMActiveLow)
    {
      ledcWrite(this->hw_config.channel, PWM_DC_MAX - dutyCycle);
    }
    else
    {
      ledcWrite(this->hw_config.channel, dutyCycle);
    }
  #endif
    this->state = dutyCycle;
    this->publish();
  }
}

void HeaterPWM::turnOff()
{
  this->actuateNewDC(0);
}
