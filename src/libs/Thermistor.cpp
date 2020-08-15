#include "Thermistor.h"
#include "math.h"

#define R1 (2400) // in Ohm

const double values_B3988[] = {1.125181376127538e-03, 2.347420615672053e-04, 8.536313443746071e-08};
const double values_B4300[] = {1.295546029021604e-03, 2.158573800965529e-04, 8.980104686571273e-08};
#define T0 (273.16)
#define MSG_BUFFER_SIZE	(10)
char msg[MSG_BUFFER_SIZE];

#if CHIP==esp32
#define ESP32
#endif

#ifdef ESP32
#define ADC_RES (4096)
#else
#define ADC_RES (1024)
#endif

#define REPORT_INTERVAL 5000

#ifdef ESP32
Thermistor::Thermistor(byte channel, Type type, String topic, PubSubClient * mqttClient)
#else
Thermistor::Thermistor(Type type, String topic, PubSubClient * mqttClient)
#endif
{
#ifdef ESP32
  this->channel = channel;
#else
  this->channel = A0;
#endif
  this->topic = topic;
  this->mqttClient = mqttClient;
  switch (type) {
    case Type::B3988:
      this->pvalues = values_B3988;
      break;
    case Type::B4300:
    default:
      this->pvalues = values_B4300;
      break;
  }
}

void Thermistor::setup()
{
  this->next_query = millis();
#ifdef ESP32
  analogReadResolution(12);
  analogSetPinAttenuation(this->channel, ADC_11db);
#else
#endif
}

void Thermistor::loop()
{
  unsigned long uptime = millis();
  if (uptime > this->next_query)
  {
    this->next_query += REPORT_INTERVAL;
    if (uptime > this->next_query)
    {
      this->next_query = uptime + REPORT_INTERVAL;
    }
    double temperature = this->getTemp();
    snprintf (msg, MSG_BUFFER_SIZE, "%.1f", temperature);
    mqttClient->publish(topic.c_str(), msg);
  }
}

double Thermistor::getTemp()
{
  double Rth = this->getResistance();
  double temp = log(Rth);
  double temp2 = pvalues[0] + pvalues[1] * temp + pvalues[2] * temp * temp * temp;
  return (1.0 / temp2 - T0);
}

double Thermistor::getResistance()
{
  uint16_t ADCval = this->getADC();

}

uint16_t Thermistor::getADC()
{
  uint16_t val = analogRead(this->channel);
  val = 0;
  for (byte i = 0; i<16; ++i)
  {
    delay(5);
    val += analogRead(this->channel);
  }
  return val >> 4;
}
