#include "../secrets_general.h"
#include "../config_general.h"

#ifndef SHUTTER_SINGLE_ID
#error "No shutter ID supplied by build chain!"
#endif

#include "secrets.h"
#include "config.h"
#ifndef SHUTTER_VALUE_MOTOR_OFF
#define SHUTTER_VALUE_MOTOR_OFF (SHUTTER_MOTOR_SHUTOFF_DEFAULT)
#endif

#include "../common.h"

extern PubSubClient client;

#include "Thermistor.h"
#include "Shutter.h"

Thermistor Therm_Relais(Thermistor::Type::B4300, 2400.0f, TOPIC_RELAIS_TEMP, &client);
Shutter Only_Shutter(
  {{SHUTTER_PIN_IN_D, SHUTTER_PIN_IN_U}, {SHUTTER_PIN_OUT_D, SHUTTER_PIN_OUT_U}},
  {SHUTTER_TIME_0_1, SHUTTER_TIME_1_2, SHUTTER_TIME_1_0, SHUTTER_TIME_2_1},
  BASE_TOPIC_SHUTTER, &client, SHUTTER_VALUE_MOTOR_OFF);

void setup()
{
  common::setup();

  Therm_Relais.setup();
  Only_Shutter.setup();
}

void loop()
{
  common::loop();

  Therm_Relais.loop();
  Only_Shutter.loop();

  delay(5);
}

void mqtt_setup()
{
  Only_Shutter.setupMQTT();
}

void mqtt_callback(const String & topic, const String & payload)
{
  if(topic.startsWith(BASE_TOPIC_SHUTTER))
  {
    Only_Shutter.callback(topic, payload);
  }
}
