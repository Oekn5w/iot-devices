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
  tempTopic = this->topic_base + SHUTTER_TOPIC_STATE_COMMAND;
  mqttClient->subscribe(tempTopic.c_str());
}

void Shutter::mqttReconnect()
{
  this->percentage_closed_published = -1.0f;
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
  this->movement_state = stMovementState::STOPPED;
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

Shutter::stState Shutter::getState() const
{
  if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD_OPEN)
  {
    return stState::OPEN;
  }
  else if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD_PO_PC)
  {
    return stState::PARTIALLY_OPEN;
  }
  else if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD_CLOSED)
  {
    return stState::PARTIALLY_CLOSED;
  }
  else if (this->percentage_closed < SHUTTER_PAYLOAD_STATE_THRESHOLD_C_FC)
  {
    return stState::CLOSED;
  }
  else
  {
    return stState::FULLY_CLOSED;
  }
}

void Shutter::publishAll()
{
  if (this->mqttClient->connected())
  {
    this->publishState(false);
    this->publishValue(false);
  }
}

void Shutter::publishState(bool checkConnectivity) const
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    stState tempState = this->getState();
    String payload = "";
    switch (tempState)
    {
      case OPEN:
        payload = SHUTTER_PAYLOAD_STATE_OPEN;
        break;
      case PARTIALLY_OPEN:
        payload = SHUTTER_PAYLOAD_STATE_PART_OPEN;
        break;
      case PARTIALLY_CLOSED:
        payload = SHUTTER_PAYLOAD_STATE_PART_CLOSED;
        break;
      case CLOSED:
        payload = SHUTTER_PAYLOAD_STATE_CLOSED;
        break;
      case FULLY_CLOSED:
        payload = SHUTTER_PAYLOAD_STATE_FULLCLOSED;
        break;
      default:
        break;
    }
    String topic = this->topic_base + SHUTTER_TOPIC_STATE_PUBLISH;
    this->mqttClient->publish(topic.c_str(), payload.c_str());
  }
}

void Shutter::publishValue(bool checkConnectivity)
{
  if (!checkConnectivity || this->mqttClient->connected())
  {
    if (this->percentage_closed_published < 0.0f || std::abs(this->percentage_closed - this->percentage_closed_published) > 0.1f)
    {
      String topic = this->topic_base + SHUTTER_TOPIC_POSITION_PUBLISH;
      char payload[10];
      snprintf(payload, 10, "%.0f", this->percentage_closed);
      this->mqttClient->publish(topic.c_str(), payload);
      this->percentage_closed_published = this->percentage_closed;
    }
  }
}
