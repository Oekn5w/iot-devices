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

Heater::Heater(byte pin, String topic_status, PubSubClient * mqttclient)
{
  this->pin = pin;
  this->mqttclient = mqttclient;
  this->topic_status = topic_status;
  this->state = false;
  this->state_published = false;
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
    this->turn_off();
  }
  if(!this->state_published && mqttclient->connected())
  {
    this->publish();
  }
}

void Heater::publish()
{
  if(mqttclient->connected())
  {
    mqttclient->publish(topic_status.c_str(), this->state ? PAYLOAD_HEATER_ON : PAYLOAD_HEATER_OFF, true);
    this->state_published = true;
  }
  else
  {
    this->state_published = false;
  }
}

void Heater::turn_on()
{
  this->timeout = millis() + KEEPALIVE_INTERVAL;
  if (!this->state)
  {
    this->state = true;
    digitalWrite(this->pin, PIN_ON);
    this->publish();
  }
}

void Heater::turn_off()
{
  this->timeout = -1;
  if (this->state)
  {
    this->state = false;
    digitalWrite(this->pin, PIN_OFF);
    this->publish();
  }
}
