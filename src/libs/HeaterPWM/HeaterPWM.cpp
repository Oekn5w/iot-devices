#include "HeaterPWM.h"
#include "HeaterPWM_config.h"

HeaterPWM::HeaterPWM(HeaterPWM::stHWInfo infoHW, String topicBase, PubSubClient * client)
{
  this->infoHW = infoHW;
  this->mqttClient = client;
  this->topicBase = topicBase;

  this->dC = 0;
  this->published = false;
}

void HeaterPWM::setup()
{
  ledcSetup(this->infoHW.channel, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(this->infoHW.PWM.Pin, this->infoHW.channel);
  if (this->infoHW.useEnable)
  { 
    pinMode(this->infoHW.Enable.Pin, OUTPUT);
    digitalWrite(this->infoHW.Enable.Pin, this->infoHW.Enable.activeHigh ? 0 : 1);
  }
  this->bSwitch = false;
  this->timeoutSwitch = 0;
  this->dC_Standby = 0;
  this->timeoutPWM = 0;
  this->turnOff();
}

void HeaterPWM::loop()
{
  if(this->bSwitch && millis() > this->timeoutSwitch)
  {
    this->processSwitch(false);
    this->timeoutSwitch = 0;
  }
  if(this->dC_Standby > 0 && millis() > this->timeoutPWM)
  {
    this->processDutyCycle(0);
    this->timeoutPWM = 0;
  }
  if(!this->published && mqttClient->connected())
  {
    this->publish();
  }
}

void HeaterPWM::callback(String topic, String payload)
{
  if (topic.startsWith(this->topicBase))
  {
    topic = topic.substring(this->topicBase.length());
    if (topic == HEATER_SWITCH_TOPIC)
    {
      if (payload == HEATER_SWITCH_PAYLOAD_ON)
      {
        this->processSwitch(true);
      }
      else if (payload == HEATER_SWITCH_PAYLOAD_OFF)
      {
        this->processSwitch(false);
      }
    }
    else if (topic == HEATER_POWER_TOPIC)
    {
      this->processPower((payload.toFloat()));
    }
    else if (topic == HEATER_PWM_TOPIC)
    {
      this->processDutyCycle((uint16_t)(payload.toInt()));
    }
  }
}

void HeaterPWM::setupMQTT()
{
  String tempTopic = this->topicBase + HEATER_SWITCH_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topicBase + HEATER_POWER_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topicBase + HEATER_PWM_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topicBase + HEATER_PWMINFO_TOPIC;
  char msg[MSG_BUFFER_SIZE];
  snprintf(msg, MSG_BUFFER_SIZE, "%d", PWM_DC_MAX);
  mqttClient->publish(tempTopic.c_str(), msg, true);
}

void HeaterPWM::publish()
{
  if(mqttClient->connected())
  {
    char msg[MSG_BUFFER_SIZE];
    String tempTopic = this->topicBase + HEATER_STATE_SWTICH_TOPIC;
    mqttClient->publish(tempTopic.c_str(),
        this->bSwitch ? HEATER_SWITCH_PAYLOAD_ON : HEATER_SWITCH_PAYLOAD_OFF,
        true);
    
    tempTopic = this->topicBase + HEATER_STATE_PWM_TOPIC;
    snprintf(msg, MSG_BUFFER_SIZE, "%d", this->dC_Standby);
    mqttClient->publish(tempTopic.c_str(), msg, true);
    
    tempTopic = this->topicBase + HEATER_STATE_ACT_TOPIC;
    snprintf(msg, MSG_BUFFER_SIZE, "%d", this->dC);
    mqttClient->publish(tempTopic.c_str(), msg, true);
    
    this->published = true;
  }
  else
  {
    this->published = false;
  }
}

void HeaterPWM::processPower(float power)
{
  uint16_t dutyCycle;
  // TODO: Lookup table
  this->processDutyCycle(dutyCycle);
}

void HeaterPWM::processDutyCycle(uint16_t dutyCycle)
{
  if (dutyCycle > PWM_DC_MAX)
  {
    dutyCycle = 0;
  }
  if (dutyCycle > 0)
  {
    this->timeoutPWM = millis() + KEEPALIVE_INTERVAL;
  }
  else
  {
    this->timeoutPWM = 0;
  }
  this->dC_Standby = dutyCycle;
  this->actuateNewDC(this->bSwitch * this->dC_Standby);
}

void HeaterPWM::processSwitch(bool newState)
{
  if (newState)
  {
    this->timeoutSwitch = millis() + KEEPALIVE_INTERVAL;
  }
  else
  {
    this->timeoutSwitch = 0;
  }
  this->bSwitch = newState;
  this->actuateNewDC(this->bSwitch * this->dC_Standby);
}

void HeaterPWM::actuateNewDC(uint16_t dutyCycle)
{
  if(dutyCycle > PWM_DC_MAX)
  {
    dutyCycle = 0;
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
    if (this->infoHW.PWM.activeHigh)
    {
      ledcWrite(this->infoHW.channel, capped);
    }
    else
    {
      ledcWrite(this->infoHW.channel, PWM_DC_MAX - capped);
    }
    if (this->infoHW.useEnable)
    {
      if (capped > 0)
      {
        digitalWrite(this->infoHW.Enable.Pin, this->infoHW.Enable.activeHigh ? 1 : 0);
      }
      else
      {
        digitalWrite(this->infoHW.Enable.Pin, this->infoHW.Enable.activeHigh ? 0 : 1);
      }
    }
  #else
    if (this->infoHW.PWM.activeHigh)
    {
      ledcWrite(this->infoHW.channel, dutyCycle);
    }
    else
    {
      ledcWrite(this->infoHW.channel, PWM_DC_MAX - dutyCycle);
    }
    if (this->infoHW.useEnable)
    {
      if (dutyCycle > 0)
      {
        digitalWrite(this->infoHW.Enable.Pin, this->infoHW.Enable.activeHigh ? 1 : 0);
      }
      else
      {
        digitalWrite(this->infoHW.Enable.Pin, this->infoHW.Enable.activeHigh ? 0 : 1);
      }
    }
  #endif
    this->dC = dutyCycle;
    this->publish();
  }
}

void HeaterPWM::turnOff()
{
  this->processSwitch(false);
}
