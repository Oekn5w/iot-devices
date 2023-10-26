#include "HeaterPWM.h"
#include "HeaterPWM_config.h"
#include "HeaterPWM_LUT.h"

using namespace Heater;

HeaterPWM::HeaterPWM(const HeaterPWMSettings * pSettings, PubSubClient * client)
{
  this->pSet = pSettings;
  this->mqttClient = client;

  this->dC = 0;
  this->published = false;
}

void HeaterPWM::setup()
{
  ledcSetup(this->pSet->outChannel, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(this->pSet->outPin, this->pSet->outChannel);
  if (this->pSet->fbEnable)
  { 
    pinMode(this->pSet->fbPin, OUTPUT);
    digitalWrite(this->pSet->fbPin, this->pSet->fbActiveHigh ? 0 : 1);
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

void HeaterPWM::callback(String topic, const String & payload)
{
  if (topic.startsWith(this->pSet->topicBase))
  {
    topic = topic.substring(this->pSet->topicBase.length());
    if (topic == PWMHEATER_SWITCH_TOPIC)
    {
      if (payload == PWMHEATER_SWITCH_PAYLOAD_ON)
      {
        this->processSwitch(true);
      }
      else if (payload == PWMHEATER_SWITCH_PAYLOAD_OFF)
      {
        this->processSwitch(false);
      }
    }
    else if (topic == PWMHEATER_POWER_TOPIC)
    {
      this->processPower((payload.toFloat()));
    }
    else if (topic == PWMHEATER_PWM_TOPIC)
    {
      this->processDutyCycle((uint16_t)(payload.toInt()));
    }
  }
}

void HeaterPWM::setupMQTT()
{
  String tempTopic = this->pSet->topicBase + PWMHEATER_SWITCH_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->pSet->topicBase + PWMHEATER_POWER_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->pSet->topicBase + PWMHEATER_PWM_TOPIC;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->pSet->topicBase + PWMHEATER_PWMINFO_TOPIC;
  char msg[MSG_BUFFER_SIZE];
  snprintf(msg, MSG_BUFFER_SIZE, "%d", PWM_DC_MAX);
  mqttClient->publish(tempTopic.c_str(), msg, true);
}

void HeaterPWM::publish()
{
  if(mqttClient->connected())
  {
    char msg[MSG_BUFFER_SIZE];
    String tempTopic = this->pSet->topicBase + PWMHEATER_STATE_SWTICH_TOPIC;
    mqttClient->publish(tempTopic.c_str(),
        this->bSwitch ? PWMHEATER_SWITCH_PAYLOAD_ON : PWMHEATER_SWITCH_PAYLOAD_OFF,
        true);
    
    tempTopic = this->pSet->topicBase + PWMHEATER_STATE_PWM_TOPIC;
    snprintf(msg, MSG_BUFFER_SIZE, "%d", this->dC_Standby);
    mqttClient->publish(tempTopic.c_str(), msg, true);
    
    tempTopic = this->pSet->topicBase + PWMHEATER_STATE_ACT_TOPIC;
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
  if (power <= LUT_power[0])
  {
    dutyCycle = LUT_DC[0];
  }
  else if (power >= LUT_power[LUT_number - 1])
  {
    dutyCycle = LUT_DC[LUT_number - 1];
  }
  else
  {
    uint16_t i = 0;
    while (power >= LUT_power[i + 1]) i++;
    float t = (power - LUT_power[i]) / (LUT_power[i + 1] - LUT_power[i]);
    dutyCycle = LUT_DC[i] * (1.0f - t) + LUT_DC[i + 1] * t;
  }
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
    uint16_t dCToApply = dutyCycle;
  #if PWM_CAPPED > 0
    if (dCToApply > PWM_DC_CAP_HIGH) dCToApply = PWM_DC_CAP_HIGH;
    if (dCToApply >= PWM_DC_FULL_THRESH) dCToApply = PWM_DC_MAX;
    #if PWM_ALWAYS > 0
    if (dCToApply < PWM_DC_CAP_LOW) dCToApply = PWM_DC_CAP_LOW;
    #else // PWM_ALWAYS
    if (dCToApply && dCToApply < PWM_DC_CAP_LOW) dCToApply = PWM_DC_CAP_LOW;
    #endif // PWM_ALWAYS
    if (this->pSet->outActiveHigh)
    {
      ledcWrite(this->pSet->outChannel, dCToApply);
    }
    else
    {
      ledcWrite(this->pSet->outChannel, PWM_DC_MAX - dCToApply);
    }
    if (this->pSet->fbEnable)
    {
      if (dCToApply > 0)
      {
        digitalWrite(this->pSet->fbPin, this->pSet->fbActiveHigh ? 1 : 0);
      }
      else
      {
        digitalWrite(this->pSet->fbPin, this->pSet->fbActiveHigh ? 0 : 1);
      }
    }
  #else // PWM_CAPPED
    if (dCToApply >= PWM_DC_FULL_THRESH) dCToApply = PWM_DC_MAX;
    if (this->pSet->outActiveHigh)
    {
      ledcWrite(this->pSet->outChannel, dCToApply);
    }
    else
    {
      ledcWrite(this->pSet->outChannel, PWM_DC_MAX - dCToApply);
    }
    if (this->pSet->fbEnable)
    {
      if (dCToApply > 0)
      {
        digitalWrite(this->pSet->fbPin, this->pSet->fbActiveHigh ? 1 : 0);
      }
      else
      {
        digitalWrite(this->pSet->fbPin, this->pSet->fbActiveHigh ? 0 : 1);
      }
    }
  #endif // PWM_CAPPED
    this->dC = dutyCycle;
  }
  this->publish();
}

void HeaterPWM::turnOff()
{
  this->processSwitch(false);
}
