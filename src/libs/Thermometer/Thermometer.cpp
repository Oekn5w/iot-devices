#include "Thermometer_config.h"
#include "Thermometer.h"

#define MSG_BUFFER_SIZE	(15)

Thermometer::singleBus::singleBus(byte GPIO_Bus, const String & base_topic, PubSubClient * mqttClient, unsigned int idBus /*= 0*/)
{
  this->init(GPIO_Bus, base_topic, mqttClient, idBus);
}

Thermometer::singleBus::~singleBus()
{
  if (this->devInfo)
  {
    free(this->devInfo);
    this->devInfo = nullptr;
  }
}

void Thermometer::singleBus::init(byte GPIO_Bus, const String & base_topic, PubSubClient * mqttClient, unsigned int idBus /*= 0*/)
{
  this->refWire.begin(GPIO_Bus);
  this->refDT = DallasTemperature(&this->refWire);
  this->sensors = nonBlockingDT(&this->refDT);
  uint16_t lenBaseTopic = base_topic.length();
  memset(baseTopicDev,0,THERMO_LENGTH_DEVICE);
  this->baseTopicDevIdxAddr = lenBaseTopic + (sizeof(THERMO_SUBTOPIC_DEV) - 1) + 1;
  strcpy(baseTopicDev,base_topic.c_str());
  strcat(baseTopicDev,THERMO_SUBTOPIC_DEV THERMO_DEV_ADD);

  memset(baseTopicBus,0,THERMO_LENGTH_BUS);
  strcpy(baseTopicBus,base_topic.c_str());
  strcat(baseTopicBus,THERMO_SUBTOPIC_BUS THERMO_BUS_ADD);
  sprintf(baseTopicBus + lenBaseTopic + (sizeof(THERMO_SUBTOPIC_BUS) - 1) + 1, "%d", idBus);
  this->mqttClient = mqttClient;

  this->readingPending = false;
  this->rescanPending = false;
  this->busInfoToPublish = false;
  this->sensorToPublish = false;

  this->timeConvStart = 0;
}

void* Thermometer::singleBus::operator new(unsigned int size)
{
  void * p;
  p = malloc(size);
  memset((Thermometer::singleBus*)p,0,size);
  *((Thermometer::singleBus*)p) = Thermometer::singleBus();
  return (Thermometer::singleBus*) p;
}

void Thermometer::singleBus::operator delete(void* p)
{
  singleBus* pNss = (singleBus*) p;
  pNss->~singleBus();
  free(p);
}

void Thermometer::singleBus::setup()
{
  this->numSensors = this->sensors.begin(THERMO_RESOLUTION);
  if (this->devInfo)
  {
    free(this->devInfo);
    this->devInfo = nullptr;
  }
  this->devInfo = (sDevInfo*) malloc(this->numSensors * sizeof(sDevInfo));
  DeviceAddress addr;
  for(uint32_t i = 0; i < this->numSensors; ++i)
  {
    this->sensors.getAddressFromTempSensorIndex(addr, i);
    snprintf(this->devInfo[i].address, 17, "%02X%02X%02X%02X%02X%02X%02X%02X",
      addr[0],
      addr[1],
      addr[2],
      addr[3],
      addr[4],
      addr[5],
      addr[6],
      addr[7]
    );
    this->devInfo[i].publishedTemperature = -1.0f;
  }
  this->busInfoToPublish = true;
  this->sensorToPublish = false;
  this->wireState = IDLE;
  this->rescanPending = false;
  this->readingPending = true;
}

void Thermometer::singleBus::loop()
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

void Thermometer::singleBus::readTemperatures()
{
  this->readingPending = true;
}

void Thermometer::singleBus::planRescan()
{
  this->rescanPending = true;
}

void Thermometer::singleBus::publishBus()
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
    mqttClient->publish(this->baseTopicBus, temp.c_str(), true);
    this->busInfoToPublish = false;
  }
}

void Thermometer::singleBus::publishSensors()
{
  if(this->sensorToPublish && this->mqttClient->connected())
  {
    float temperature;
    for(uint32_t i = 0; i < this->numSensors; ++i)
    {
      memcpy(baseTopicDev + baseTopicDevIdxAddr, devInfo[i].address, 16);
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
        mqttClient->publish(this->baseTopicDev, msg, true);
        this->devInfo[i].publishedTemperature = temperature;
      }
    }
    this->sensorToPublish = false;
  }
}

Thermometer::multiBus::multiBus(byte* GPIO_Busses, unsigned int N_Busses, const String & base_topic, PubSubClient * mqttClient, unsigned long queryInterval)
{
  this->Busses = (singleBus**) malloc(N_Busses * sizeof(singleBus*));
  memset(this->Busses, 0, N_Busses * sizeof(singleBus*));

  this->topicBase = base_topic;
  this->mqttClient = mqttClient;

  this->N_Busses = N_Busses;

  for (unsigned int i = 0; i < N_Busses; ++i)
  {
    this->Busses[i] = new singleBus;
    this->Busses[i]->init(GPIO_Busses[i], base_topic, mqttClient, i);
  }

  this->queryInterval = queryInterval;
}

Thermometer::multiBus::~multiBus()
{
  if (this->Busses)
  {
    for (unsigned int i = 0; i < this->N_Busses; ++i)
    {
      this->Busses[i]->~singleBus();
      free(this->Busses[i]);
    }
    free(this->Busses);
  }
}

void Thermometer::multiBus::readTemperatures()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i]->readTemperatures();
  }
}

void Thermometer::multiBus::setup()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i]->setup();
  }
  if (this->queryInterval)
  {
    this->next_query = millis() + this->queryInterval;
  }
}

void Thermometer::multiBus::loop()
{
  for (unsigned int i = 0; i < this->N_Busses; ++i)
  {
    this->Busses[i]->loop();
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

void Thermometer::multiBus::callback(String topic, const String & payload)
{
  if (topic.startsWith(this->topicBase))
  {
    topic = topic.substring(this->topicBase.length());
    if (topic == THERMO_SUBTOPIC_RESCAN)
    {
      for (unsigned int i = 0; i < this->N_Busses; ++i)
      {
        this->Busses[i]->planRescan();
      }
      if (this->queryInterval)
      {
        this->next_query = millis() + this->queryInterval;
      }
    }
  }
}

void Thermometer::multiBus::setupMQTT()
{
  String tempTopic = this->topicBase + THERMO_SUBTOPIC_RESCAN;
  mqttClient->subscribe(tempTopic.c_str());
}
