#include "Shutter.h"
#include "EEPROM.h"
#include "math.h"

Shutter::Shutter(stPins Pins, stTimings Timings, String topic_base, 
  unsigned int eep_addr, PubSubClient* client, EEPROMClass* eep)
{
  this->Pins = Pins;
  this->Timings = Timings;
  this->topic_base = topic_base;
  this->eep_addr = eep_addr;
  this->mqttClient = client;
  this->eep = eep;
}

void Shutter::callback(String topic, String payload)
{

}

void Shutter::setSubscriptions()
{
  String tempTopic = this->topic_base + SHUTTER_TOPIC_POSITION_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
  tempTopic = this->topic_base + SHUTTER_TOPIC_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
}

void Shutter::mqttReconnect()
{
  this->percentage_closed_published = -1.0f;
  this->state_published = UNDEFINED;
  this->publishAll();
}

void Shutter::interrupt()
{

}

void Shutter::setup(void (* fcn_interrupt)())
{
  this->percentage_closed = eep->read(this->eep_addr);
  if (this->percentage_closed < 0.0f || this->percentage_closed > 200.0f)
  {
    this->percentage_closed = NAN;
  }
  this->is_confident = false;
  this->queued_target_value = -1.0f;
  this->movement_state = stMovementState::mvSTOPPED;
  this->percentage_closed_published = -1.0f;
  this->state_published = UNDEFINED;
  digitalWrite(Pins.actuator.down, 0);
  digitalWrite(Pins.actuator.up, 0);
  delay(1);
  pinMode(Pins.actuator.down, OUTPUT);
  pinMode(Pins.actuator.up, OUTPUT);
  pinMode(Pins.input.down, INPUT_PULLUP);
  pinMode(Pins.input.up, INPUT_PULLUP);
  delay(1);
  this->save_down = digitalRead(Pins.input.down);
  this->save_up = digitalRead(Pins.input.up);
  attachInterrupt(digitalPinToInterrupt(Pins.input.down), fcn_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pins.input.up), fcn_interrupt, CHANGE);
}

void Shutter::loop()
{

}

void Shutter::setTarget(float targetValue)
{

}

void Shutter::calibrate()
{

}

Shutter::stState Shutter::getState() const
{
  if (this->movement_state == mvSTOPPED)
  {
    if (!this->is_confident)
    {
      return stState::UNDEFINED;
    }
    else if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD)
    {
      return stState::OPEN;
    }
    else
    {
      return stState::CLOSED;
    }
  }
  else if (this->movement_state == mvOPENING)
  {
    return stState::OPENING;
  }
  else if (this->movement_state == mvCLOSING)
  {
    return stState::CLOSING;
  }
  return stState::UNDEFINED;
}

void Shutter::publishAll()
{
  if (this->mqttClient->connected())
  {
    this->publishState(false);
    this->publishValue(false);
  }
}

void Shutter::publishState(bool checkConnectivity)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    stState tempState = this->getState();
    if (tempState != this->state_published)
    {
      String payload = "";
      bool retain = true;
      switch (tempState)
      {
        case OPEN:
          payload = SHUTTER_PAYLOAD_STATE_OPEN;
          break;
        case CLOSED:
          payload = SHUTTER_PAYLOAD_STATE_CLOSED;
          break;
        case OPENING:
          payload = SHUTTER_PAYLOAD_STATE_OPENING;
          retain = false;
          break;
        case CLOSING:
          payload = SHUTTER_PAYLOAD_STATE_CLOSING;
          retain = false;
          break;
        default:
          payload = "undefined";
          break;
      }
      String topic = this->topic_base + SHUTTER_TOPIC_STATE_PUBLISH;
      this->mqttClient->publish(topic.c_str(), payload.c_str(), retain);
      this->state_published = tempState;
    }
  }
}

void Shutter::publishValue(bool checkConnectivity)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    if (this->percentage_closed_published < 0.0f || std::abs(this->percentage_closed - this->percentage_closed_published) > 0.1f)
    {
      char payload[10];

      String topic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH;
      snprintf(payload, 10, "%.0f", min(this->percentage_closed, 100.0f));
      this->mqttClient->publish(topic.c_str(), payload);

      snprintf(payload, 10, "%.0f", this->percentage_closed);
      topic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED;
      this->mqttClient->publish(topic.c_str(), payload);

      this->percentage_closed_published = this->percentage_closed;
    }
  }
}
