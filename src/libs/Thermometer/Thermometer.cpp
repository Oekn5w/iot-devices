#include "Thermometer_config.h"
#include "Thermometer.h"

#define MSG_BUFFER_SIZE	(15)

Thermometer::Wire::Wire(byte GPIO_Bus, String base_topic, PubSubClient * mqttClient, unsigned int idBus = 0)
{
  this->wire = OneWire(GPIO_Bus);
  this->sensors = nonBlockingDT(&this->wire);

  this->baseTopicDev = base_topic + THERMO_SUBTOPIC_DEV;
  this->baseTopicBus = base_topic + THERMO_SUBTOPIC_BUS + "/" + String(idBus);
  this->mqttClient = mqttClient;

  this->readingPending = false;
  this->rescanPending = false;
  this->busInfoToPublish = false;
  this->sensorToPublish = false;

  this->timeConvStart = 0;
}

void Thermometer::Wire::setup()
{
  this->numSensors = this->sensors.begin(THERMO_RESOLUTION);
  if (this->devInfo)
  {
    free(this->devInfo);
    this->devInfo = nullptr;
  }
  this->devInfo = (sDevInfo*) malloc(this->numSensors * sizeof(sDevInfo));
  DeviceAddress addr;
  char temp[17];
  for(uint32_t i = 0; i < this->numSensors; ++i)
  {
    this->sensors.getAddress(addr, i);
    snprintf(temp, 17, "%02X%02X%02X%02X%02X%02X%02X%02X",
      addr[0],
      addr[1],
      addr[2],
      addr[3],
      addr[4],
      addr[5],
      addr[6],
      addr[7]
    );
    this->devInfo[i].address = String(temp);
    this->devInfo[i].publishedTemperature = -1.0f;
  }
  this->busInfoToPublish = true;
  this->sensorToPublish = false;
  this->wireState = IDLE;
  this->rescanPending = false;
  this->readingPending = true;
}

void Thermometer::Wire::loop()
{
  switch(this->wireState)
  {
    case WAITING:
      if (this->sensors.isConversionDone())
      {
        this->sensorToPublish = true;
        this->wireState = IDLE;
      }
      else if ((millis() - this->timeConvStart) > THERMO_SENSOR_TIMEOUT)
      {
        this->wireState = ABORTED;
      }
      break;
    case IDLE:
      if (this->rescanPending)
      {
        this->setup();
      }
      else if (this->readingPending)
      {
        this->wireState = WAITING;
        this->timeConvStart = millis();
        if (!this->sensors.startConvertion())
        {
          this->wireState = ABORTED;
        }
        this->readingPending = false;
      }
      break;
    case ABORTED:
      this->planRescan();
      this->wireState = IDLE;
      break;
  }
  this->publishBus();
  this->publishSensors();
}

void Thermometer::Wire::readTemperatures()
{
  this->readingPending = true;
}

void Thermometer::Wire::planRescan()
{
  this->rescanPending = true;
}

void Thermometer::Wire::publishBus()
{
  if(this->busInfoToPublish && this->mqttClient->connected())
  {
    String temp = "[";
    for(uint32_t i = 0; i < this->numSensors; ++i)
    {
      temp += this->devInfo[i].address;
      if (i < this->numSensors - 1)
      {
        temp += ",";
      }
    }
    temp += "]";

    this->busInfoToPublish = false;
  }
}

void Thermometer::Wire::publishSensors()
{
  if(this->sensorToPublish && this->mqttClient->connected())
  {
    String topic;
    float temperature;
    for(uint32_t i = 0; i < this->numSensors; ++i)
    {
      topic = this->baseTopicDev + this->devInfo[i].address;
      temperature = this->sensors.getLatestTempC(i);
      char msg[MSG_BUFFER_SIZE];
      if (abs(temperature - this->devInfo[i].publishedTemperature) > 0.1f)
      {
        if (temperature == DEVICE_DISCONNECTED_C)
        {
          strcpy(msg, "unavailable");
        }
        else
        {
          snprintf(msg, MSG_BUFFER_SIZE, "%.1f", temperature);
        }
        mqttClient->publish(topic.c_str(), msg, true);
        this->devInfo[i].publishedTemperature = temperature;
      }
    }
    this->sensorToPublish = false;
  }
}

Thermometer::MultiWire::MultiWire(byte* GPIO_Busses, unsigned int N_Busses, String base_topic, PubSubClient * mqttClient, unsigned long queryInterval = 0)
{
  this->Busses = (Wire*) malloc(N_Busses * sizeof(Wire));

  this->topicBase = base_topic;
  this->mqttClient = mqttClient;

  this->N_Busses = N_Busses;

  for (unsigned int i = 0; i < N_Busses; ++i)
  {
    this->Busses[i] = Wire(GPIO_Busses[i], base_topic, mqttClient, i);
  }

  this->queryInterval = queryInterval;
}

void Thermometer::MultiWire::readTemperatures()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i].readTemperatures();
  }
}

void Thermometer::MultiWire::setup()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i].setup();
  }
  if (this->queryInterval)
  {
    this->next_query = millis() + this->queryInterval;
  }
}

void Thermometer::MultiWire::loop()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i].loop();
  }
  if (this->next_query && millis() > this->next_query)
  {
    this->readTemperatures();
    if (this->queryInterval)
    {
      while (millis() > this->next_query)
      {
        this->next_query += this->queryInterval;
      }
    }
    else
    {
      this->next_query = 0;
    }
  }
}

void Thermometer::MultiWire::callback(String topic, const String & payload)
{
  if (topic.startsWith(this->topicBase))
  {
    topic = topic.substring(this->topicBase.length());
    if (topic == THERMO_SUBTOPIC_RESCAN)
    {
      for (unsigned int i = 0; i < this->N_Busses; ++i)
      {
        this->Busses[i].planRescan();
      }
      if (this->queryInterval)
      {
        this->next_query = millis() + this->queryInterval;
      }
    }
  }
}

void Thermometer::MultiWire::setupMQTT()
{
  String tempTopic = this->topicBase + THERMO_SUBTOPIC_RESCAN;
  mqttClient->subscribe(tempTopic.c_str());
}
