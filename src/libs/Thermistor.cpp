#include "Thermistor.h"
#include "math.h"

#define R1 (2400.0f) // in Ohm

const float values_B3988[] = {1.125181376127538e-03f, 2.347420615672053e-04f, 8.536313443746071e-08f};
const float values_B4300[] = {1.295546029021604e-03f, 2.158573800965529e-04f, 8.980104686571273e-08f};
#define T0 (273.16f)
#define MSG_BUFFER_SIZE	(10)
char msg[MSG_BUFFER_SIZE];

#if ESP32 == 1
#define ADC_RES (4096.0f)
#define ADC_th (2499.778865502888f)
#define ADC_a (-1.899899879087783e-04f)
#define ADC_b (1.961955687412854f)
#define ADC_c (-1009.271504480745f)
#define ADC_a_lin (1.012089774549827f)
#define ADC_b_lin (177.9558625375548f)
#else
#define ADC_RES (1024.0f)
#endif

#define REPORT_INTERVAL 20000

#if ESP32 == 1
Thermistor::Thermistor(byte channel, Type type, String topic, PubSubClient * mqttClient)
#else
Thermistor::Thermistor(Type type, String topic, PubSubClient * mqttClient)
#endif
{
#if ESP32 == 1
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
#if ESP32 == 1
  analogReadResolution(12);
  analogSetPinAttenuation(this->channel, ADC_11db);
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
    float temperature = this->getTemp();
    snprintf(msg, MSG_BUFFER_SIZE, "%.1f", temperature);
    mqttClient->publish(topic.c_str(), msg);
  }
}

float Thermistor::getTemp()
{
  float Rth = this->getResistance();
  float temp = log(Rth);
  float temp2 = pvalues[0] + pvalues[1] * temp + pvalues[2] * temp * temp * temp;
  return (1.0f / temp2 - T0);
}

float Thermistor::getResistance()
{
  float ADCval = this->getADC();
  float Rth = (1.0 * R1 * ADCval)/(1.0 * ADC_RES - ADCval);
  return Rth;
}

float Thermistor::getADC()
{
  uint16_t val = analogRead(this->channel);
  val = 0;
  for (uint8_t i = 0; i<16; ++i)
  {
    delay(5);
    val += analogRead(this->channel);
  }
#if ESP32 == 1
  float temp = (float)(val >> 4);
  // nonlinearity of ESP32 ADC -> counter by transfer function (linear + tangential quadratic)
  if (temp < ADC_th)
  {
    return ADC_a_lin * temp + ADC_b_lin;
  }
  else
  {
    return ADC_a * temp * temp + ADC_b * temp + ADC_c;
  }
#else
  return (float)(val >> 4);
#endif
}
