#include "../secrets_general.h"
#include "../config_general.h"
#include "hw_config.h"
#include "config.h"
#include "secrets.h"

#ifndef SHUTTER_A_VALUE_MOTOR_OFF
#define SHUTTER_A_VALUE_MOTOR_OFF (SHUTTER_MOTOR_SHUTOFF_DEFAULT)
#endif
#ifndef SHUTTER_B_VALUE_MOTOR_OFF
#define SHUTTER_B_VALUE_MOTOR_OFF (SHUTTER_MOTOR_SHUTOFF_DEFAULT)
#endif
#ifndef SHUTTER_C_VALUE_MOTOR_OFF
#define SHUTTER_C_VALUE_MOTOR_OFF (SHUTTER_MOTOR_SHUTOFF_DEFAULT)
#endif

#include "../common.h"

extern PubSubClient client;

#include "Thermistor.h"
#include "Shutter.h"

Thermistor Therm_Board(BOARD_PIN_TEMP, Thermistor::Type::B4300, 2400.0f, TOPIC_BOARD_TEMP, &client);

Thermistor Therm_Relais_A(PIN_A_TEMP, Thermistor::Type::B4300, 2400.0f, TOPIC_RELAIS_A_TEMP, &client);
Thermistor Therm_Relais_B(PIN_B_TEMP, Thermistor::Type::B4300, 2400.0f, TOPIC_RELAIS_B_TEMP, &client);
Thermistor Therm_Relais_C(PIN_C_TEMP, Thermistor::Type::B4300, 2400.0f, TOPIC_RELAIS_C_TEMP, &client);

Shutter Shutter_A(
  {{SHUTTER_A_PIN_IN_D, SHUTTER_A_PIN_IN_U}, {SHUTTER_A_PIN_OUT_D, SHUTTER_A_PIN_OUT_U}},
  {SHUTTER_A_TIME_0_1, SHUTTER_A_TIME_1_2, SHUTTER_A_TIME_1_0, SHUTTER_A_TIME_2_1},
  BASE_TOPIC_SHUTTER_A, &client, SHUTTER_A_VALUE_MOTOR_OFF);
Shutter Shutter_B(
  {{SHUTTER_B_PIN_IN_D, SHUTTER_B_PIN_IN_U}, {SHUTTER_B_PIN_OUT_D, SHUTTER_B_PIN_OUT_U}},
  {SHUTTER_B_TIME_0_1, SHUTTER_B_TIME_1_2, SHUTTER_B_TIME_1_0, SHUTTER_B_TIME_2_1},
  BASE_TOPIC_SHUTTER_B, &client, SHUTTER_B_VALUE_MOTOR_OFF);
Shutter Shutter_C(
  {{SHUTTER_C_PIN_IN_D, SHUTTER_C_PIN_IN_U}, {SHUTTER_C_PIN_OUT_D, SHUTTER_C_PIN_OUT_U}},
  {SHUTTER_C_TIME_0_1, SHUTTER_C_TIME_1_2, SHUTTER_C_TIME_1_0, SHUTTER_C_TIME_2_1},
  BASE_TOPIC_SHUTTER_C, &client, SHUTTER_C_VALUE_MOTOR_OFF);

void setup()
{
  common::setup();

  Therm_Board.setup();

  Therm_Relais_A.setup();
  Therm_Relais_B.setup();
  Therm_Relais_C.setup();

  Shutter_A.setup();
  Shutter_B.setup();
  Shutter_C.setup();
}

void loop()
{
  common::loop();

  Therm_Board.loop();

  Therm_Relais_A.loop();
  Therm_Relais_B.loop();
  Therm_Relais_C.loop();

  Shutter_A.loop();
  Shutter_B.loop();
  Shutter_C.loop();

  delay(5);
}

void mqtt_setup()
{
  Shutter_A.setupMQTT();
  Shutter_B.setupMQTT();
  Shutter_C.setupMQTT();
}

void mqtt_callback(const String & topic, const String & payload)
{
  if(topic.startsWith(BASE_TOPIC_SHUTTER_A))
  {
    Shutter_A.callback(topic, payload);
  }
  else if(topic.startsWith(BASE_TOPIC_SHUTTER_B))
  {
    Shutter_B.callback(topic, payload);
  }
  else if(topic.startsWith(BASE_TOPIC_SHUTTER_C))
  {
    Shutter_C.callback(topic, payload);
  }
}
