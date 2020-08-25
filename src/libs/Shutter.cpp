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
  this->state = stState::STOPPED;
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
