#include "Thermistor.h"
#include "math.h"

const Thermistor::strTypeValues NTC_B3988(true, 1.125181376127538e-03f, 2.347420615672053e-04f, 0.0f, 8.536313443746071e-08f);
const Thermistor::strTypeValues NTC_B4300(true, 1.295546029021604e-03f, 2.158573800965529e-04f, 0.0f, 8.980104686571273e-08f);
const Thermistor::strTypeValues PTC_PT1000(false, -245.925378962424f, 0.235877422656155f, 1.00499560366463e-05f, 0.0f);
#define T0 (273.16f)
#define MSG_BUFFER_SIZE	(10)

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

#define QUERY_INTERVAL 60000

#if ESP32 == 1
Thermistor::Thermistor(byte channel, Type type, float R1, String topic, PubSubClient * mqttClient)
#else
Thermistor::Thermistor(Type type, float R1, String topic, PubSubClient * mqttClient)
#endif
{
#if ESP32 == 1
  this->channel = channel;
#else
  this->channel = A0;
#endif
  this->R1 = R1;
  this->topic = topic;
  this->mqttClient = mqttClient;
  this->publishedTemperature = -500.0f;
  switch (type) {
    case Type::B3988:
      this->typeValues = NTC_B3988;
      break;
    case Type::B4300:
      this->typeValues = NTC_B4300;
      break;
    case Type::PT1000:
      this->typeValues = PTC_PT1000;
      break;
    default:
      this->typeValues = strTypeValues();
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
  if (uptime > this->next_query && mqttClient->connected())
  {
    this->next_query += QUERY_INTERVAL;
    if (uptime > this->next_query - (QUERY_INTERVAL / 2))
    {
      this->next_query = uptime + QUERY_INTERVAL;
    }
    float temperature = this->getTemp();
    if (abs(temperature - this->publishedTemperature) > 0.1f)
    {
      char msg[MSG_BUFFER_SIZE];
      snprintf(msg, MSG_BUFFER_SIZE, "%.1f", temperature);
      mqttClient->publish(topic.c_str(), msg, true);
      this->publishedTemperature = temperature;
    }
  }
}

float Thermistor::getTemp()
{
  float Rth = this->getResistance();
  if(this->typeValues.useSH)
  {
    float temp = log(Rth);
    float temp2 = 
      this->typeValues.order0 + 
      this->typeValues.order1 * temp + 
      this->typeValues.order2 * temp * temp + 
      this->typeValues.order3 * temp * temp * temp;
    return (1.0f / temp2 - T0);
  }
  else
  {
    return
      this->typeValues.order0 + 
      this->typeValues.order1 * Rth + 
      this->typeValues.order2 * Rth * Rth + 
      this->typeValues.order3 * Rth * Rth * Rth;
  }
}

float Thermistor::getResistance()
{
  float ADCval = this->getADC();
  float Rth = (1.0 * this->R1 * ADCval)/(1.0 * ADC_RES - ADCval);
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
