#include "ImpulseCounter.h"
#include "ImpulseCounter_config.h"

ImpulseCounter::ImpulseCounter(const ImpulseCounterSettings * pSettings, PubSubClient * client)
{
  this->pSet = pSettings;
  this->mqttClient = client;
}

void ImpulseCounter::setup()
{
  this->counterVal = 0.0;
  this->counterValPublished = false;
  if(this->pSet->inPullup)
  {
    pinMode(this->pSet->inPin, INPUT_PULLUP);
  }
  else
  {
    pinMode(this->pSet->inPin, INPUT);
  }
  delay(1);
  this->listening = false;
  this->initTimeout = 0;
  this->saveIn = digitalRead(this->pSet->inPin);
}

void ImpulseCounter::loop()
{
  if (this->initTimeout && millis() > this->initTimeout)
  {
    this->unsubMQTT();
    this->publishValue(true);
  }
  this->handleInput();
}

void ImpulseCounter::handleInput()
{
  int readIn = digitalRead(this->pSet->inPin);
  if(readIn ^ this->saveIn)
  {
    if(!(this->pSet->inRisingEdge ^ (bool)(readIn)))
    {
      this->counterVal += this->pSet->incValue;
      this->publishAll();
    }
  }
  this->saveIn = readIn;
}

void ImpulseCounter::callback(String topic, const String & payload)
{
  this->counterVal = payload.toDouble();
  this->counterValPublished = this->counterVal;
  this->unsubMQTT();
}

void ImpulseCounter::setupMQTT()
{
  if(this->counterVal != 0.0)
  {
    this->listening = false;
    this->initTimeout = 0;
    if(this->counterValPublished != this-> counterVal)
    {
      this->publishValue(false);
    }
  }
  else
  {
    String tempTopic = this->pSet->topicBase + IMPULSECOUNTER_VALUE_TOPIC;
    this->listening = true;
    this->initTimeout = millis() + IMPULSECOUNTER_MQTT_SUBSCRIPTION_RETAIN_TIMEOUT;
    mqttClient->subscribe(tempTopic.c_str());
  }
}

void ImpulseCounter::unsubMQTT()
{
  String tempTopic = this->pSet->topicBase + IMPULSECOUNTER_VALUE_TOPIC;
  mqttClient->unsubscribe(tempTopic.c_str());
  this->listening = false;
  this->initTimeout = 0;
}

void ImpulseCounter::publishAll()
{
  if(mqttClient->connected())
  {
    this->publishValue(false);
    this->publishPulse();
  }
}

void ImpulseCounter::publishValue(bool checkConnectivity)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    if (this->listening)
    {
      this->unsubMQTT();
    }
    String tempTopic = this->pSet->topicBase + IMPULSECOUNTER_VALUE_TOPIC;
    char payload[MSG_BUFFER_SIZE];
    snprintf(payload, MSG_BUFFER_SIZE, this->pSet->formatter.c_str(), this->counterVal);
    mqttClient->publish(tempTopic.c_str(), payload, true);
    this->counterValPublished = this->counterVal;
  }
}

void ImpulseCounter::publishPulse()
{
  if(this->pSet->pulseEnable)
  {
    String tempTopic = this->pSet->topicBase + IMPULSECOUNTER_PULSE_TOPIC;
    mqttClient->publish(tempTopic.c_str(), "pulse", false);
  }
}
