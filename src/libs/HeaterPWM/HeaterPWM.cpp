#include "HeaterPWM.h"

#define KEEPALIVE_INTERVAL 300000 // 5 min
#define PWM_INVERSE (0)
#define PWM_FREQ (1000)
#define PWM_RES_BITS (10)
#define PWM_DC_MAX ((1<<(PWM_RES_BITS)) - 1)

#define MSG_BUFFER_SIZE	(10)

HeaterPWM::HeaterPWM(byte pin, byte channel, String topicStatus, PubSubClient * client)
{
  this->pin = pin;
  this->channel = channel;
  this->mqttClient = client;
  this->topicStatus = topicStatus;
  this->state = 0;
  this->statePublished = false;
}

void HeaterPWM::setup()
{
  ledcSetup(this->channel, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(this->pin, this->channel);
#if PWM_INVERSE > 0
  ledcWrite(this->channel, PWM_DC_MAX);
#else
  ledcWrite(this->channel, 0);
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

void HeaterPWM::processTarget(uint16_t dutyCycle)
{
  if(dutyCycle > PWM_DC_MAX)
  {
    dutyCycle = 0;
  }
  if(dutyCycle)
  {
    this->timeout = millis() + KEEPALIVE_INTERVAL;
  }
  else
  {
    this->timeout = -1;
  }
  if (this->state != dutyCycle)
  {
    #if PWM_INVERSE > 0
    ledcWrite(this->channel, PWM_DC_MAX - dutyCycle);
    #else
    ledcWrite(this->channel, dutyCycle);
    #endif
    this->state = dutyCycle;
    this->publish();
  }
}

void HeaterPWM::turnOff()
{
  this->processTarget(0);
}
