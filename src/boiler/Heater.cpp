#include "Heater.h"

#define KEEPALIVE_INTERVAL 300000 // 5 min
#define ACTIVE_HIGH

#ifdef ACTIVE_HIGH
#define PIN_ON 1
#define PIN_OFF 0
#else
#define PIN_ON 0
#define PIN_OFF 1
#endif

Heater::Heater(byte pin, String topicStatus, PubSubClient * client)
{
  this->pin = pin;
  this->mqttClient = client;
  this->topicStatus = topicStatus;
  this->state = false;
  this->statePublished = false;
}

void Heater::setup()
{
  pinMode(this->pin, OUTPUT);
  this->state = false;
  digitalWrite(this->pin, PIN_OFF);
  this->timeout = 0;
}

void Heater::loop()
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

void Heater::publish()
{
  if(mqttClient->connected())
  {
    mqttClient->publish(topicStatus.c_str(), this->state ? PAYLOAD_HEATER_ON : PAYLOAD_HEATER_OFF, true);
    this->statePublished = true;
  }
  else
  {
    this->statePublished = false;
  }
}

void Heater::turnOn()
{
  this->timeout = millis() + KEEPALIVE_INTERVAL;
  if (!this->state)
  {
    this->state = true;
    digitalWrite(this->pin, PIN_ON);
    this->publish();
  }
}

void Heater::turnOff()
{
  this->timeout = -1;
  if (this->state)
  {
    this->state = false;
    digitalWrite(this->pin, PIN_OFF);
    this->publish();
  }
}
