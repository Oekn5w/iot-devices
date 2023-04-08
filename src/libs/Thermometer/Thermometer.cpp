#include "Thermometer.h"

Thermometer::Wire::Wire(byte GPIO_Bus, String base_topic, PubSubClient * mqttClient)
{
  this->wire = OneWire(GPIO_Bus);
  this->sensors = nonBlockingDT(&this->wire);

  this->base_topic = base_topic;
  this->mqttClient = mqttClient;
}

void Thermometer::Wire::setup()
{
  this->sensors.begin();
}

void Thermometer::Wire::loop()
{
  
}

Thermometer::MultiWire::MultiWire(byte* GPIO_Busses, unsigned int N_Busses, String base_topic, PubSubClient * mqttClient)
{
  this->Busses = (Wire*) malloc(N_Busses * sizeof(Wire));

  this->base_topic = base_topic;
  this->mqttClient = mqttClient;

  this->N_Busses = N_Busses;

  for (unsigned int i = 0; i < N_Busses; ++i)
  {
    this->Busses[i] = Wire(GPIO_Busses[i], base_topic, mqttClient);
  }
}

void Thermometer::MultiWire::setup()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i].setup();
  }
}

void Thermometer::MultiWire::loop()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i].loop();
  }
}
