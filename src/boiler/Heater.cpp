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
}

void Heater::turn_on()
{
  this->timeout = millis() + KEEPALIVE_INTERVAL;
  if (!this->state)
  {
    this->state = true;
    digitalWrite(this->pin, PIN_ON);
    mqttclient->publish(topic_status.c_str(), PAYLOAD_HEATER_ON);
  }
}

void Heater::turn_off()
{
  this->timeout = -1;
  if (this->state)
  {
    this->state = false;
    digitalWrite(this->pin, PIN_OFF);
    mqttclient->publish(topic_status.c_str(), PAYLOAD_HEATER_OFF);
  }
}
