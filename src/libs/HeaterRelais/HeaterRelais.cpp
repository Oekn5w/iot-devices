#include "HeaterRelais.h"
#include "HeaterRelais_config.h"

using namespace Heater;

#define PIN_ON (1)
#define PIN_OFF (0)
#define SET_OUTPUT(PIN, AH) (digitalWrite((PIN), (AH) ? (1) : (0)))
#define CLR_OUTPUT(PIN, AH) (digitalWrite((PIN), (AH) ? (0) : (1)))

HeaterRelais::HeaterRelais(const HeaterRelaisSettings * pSettings, PubSubClient * client)
{
  this->pSet = pSettings;
  this->mqttClient = client;
  this->state = false;
  this->statePublished = false;
}

void HeaterRelais::setup()
{
  pinMode(this->pSet->switchPin, OUTPUT);
  CLR_OUTPUT(this->pSet->switchPin, this->pSet->switchActiveHigh);
  if(this->pSet->ledEnable)
  {
    pinMode(this->pSet->ledPin, OUTPUT);
    CLR_OUTPUT(this->pSet->ledPin, this->pSet->ledActiveHigh);
  }
  this->state = false;
  this->timeout = 0;
}

void HeaterRelais::loop()
{
  if(this->state && this->timeout && millis() > this->timeout)
  {
    this->turnOff();
  }
  if(!this->statePublished && mqttClient->connected())
  {
    this->publish();
  }
}

void HeaterRelais::callback(String topic, String payload)
{
  if (topic.startsWith(this->pSet->topicBase))
  {
    topic = topic.substring(this->pSet->topicBase.length());
    if (topic == TOPIC_HEATER_REL_CMD)
    {
      if (payload == PAYLOAD_HEATER_ON)
      {
        this->turnOn();
      }
      else if (payload == PAYLOAD_HEATER_OFF)
      {
        this->turnOff();
      }
    }
  }
}

void HeaterRelais::setupMQTT()
{
  String tempTopic = this->topicBase + SUBTOPIC_HEATER_REL_CMD;
  mqttClient->subscribe(tempTopic.c_str());
}

void HeaterRelais::publish()
{
  if(mqttClient->connected())
  {
    String tempTopic = this->pSet->topicBase + SUBTOPIC_HEATER_REL_STATE;
    mqttClient->publish(topicStatus.c_str(), this->state ? PAYLOAD_HEATER_ON : PAYLOAD_HEATER_OFF, true);
    this->statePublished = true;
  }
  else
  {
    this->statePublished = false;
  }
}

void HeaterRelais::turnOn()
{
  this->timeout = millis() + KEEPALIVE_INTERVAL;
  if (!this->state)
  {
    this->state = true;
    SET_OUTPUT(this->pSet->switchPin, this->pSet->switchActiveHigh);
    if(this->pSet->ledEnable)
    {
      SET_OUTPUT(this->pSet->ledPin, this->pSet->ledActiveHigh);
    }
    this->publish();
  }
}

void HeaterRelais::turnOff()
{
  this->timeout = 0;
  if (this->state)
  {
    this->state = false;
    CLR_OUTPUT(this->pSet->switchPin, this->pSet->switchActiveHigh);
    if(this->pSet->ledEnable)
    {
      CLR_OUTPUT(this->pSet->ledPin, this->pSet->ledActiveHigh);
    }
    this->publish();
  }
}
