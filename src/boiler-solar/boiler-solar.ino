#include "../secrets_general.h"
#include "../config_general.h"
#include "config.h"
#include "secrets.h"

#include "../common.h"

extern PubSubClient client;

#include "Heater.h"
#include "HeaterPWM.h"
#include "ImpulseCounter.h"
#include "Thermometer.h"

#define SETUP_DISABLE_LED(gpio) pinMode(gpio, OUTPUT); digitalWrite(gpio, 0);

Heater heaterRel(21, TOPIC_HEATER_REL_STATUS, &client);
HeaterPWM heaterPWM({{2, true}, 0, true, {12, true}}, TOPIC_BASE_HEATER_PWM, &client);

struct ImpulseCounterSettings counterGasSet;
ImpulseCounter counterGas(&counterGasSet, &client);

byte thermoBusPins[] {22, 18, 16};
static_assert(sizeof(TOPIC_BASE_THERMO_BUSSES) - 1 <= THERMO_MAX_LEN_BASE_TOPIC);
Thermometer::multiBus thermoBusses(
  thermoBusPins,
  sizeof(thermoBusPins)/sizeof(byte),
  TOPIC_BASE_THERMO_BUSSES,
  &client,
  20000
);

void setupSettingStructs()
{
  counterGasSet.inPin = 26;
  counterGasSet.incValue = 0.01;
  counterGasSet.inPullup = false;
  counterGasSet.formatter = "%.2f";
  counterGasSet.topicBase = TOPIC_BASE_COUNTER_GAS;
  counterGasSet.ledEnable = false;
  counterGasSet.ledPin = 14;
}

void setup()
{
  common::setup();

  setupSettingStructs();

  heaterRel.setup();
  heaterPWM.setup();

  counterGas.setup();

  thermoBusses.setup();

  SETUP_DISABLE_LED(13); // PWM
  SETUP_DISABLE_LED(27); // WW
  SETUP_DISABLE_LED(14); // Gas
  SETUP_DISABLE_LED(19); // Temp_1
  SETUP_DISABLE_LED(5);  // Temp_2
  SETUP_DISABLE_LED(17); // Temp_3
}

void loop()
{
  common::loop();

  heaterRel.loop();
  heaterPWM.loop();

  counterGas.loop();

  thermoBusses.loop();
}

void mqtt_setup()
{
  heaterPWM.setupMQTT();
  client.subscribe(TOPIC_HEATER_REL_CONTROL);
  counterGas.setupMQTT();
  thermoBusses.setupMQTT();
}

void mqtt_callback(const String & topic, const String & payload)
{
  if(topic.startsWith(TOPIC_BASE_HEATER_PWM))
  {
    heaterPWM.callback(topic, payload);
  }
  else if(topic == TOPIC_HEATER_REL_CONTROL)
  {
    if(payload == PAYLOAD_HEATER_ON)
    {
      heaterRel.turnOn();
    }
    else if(payload == PAYLOAD_HEATER_OFF)
    {
      heaterRel.turnOff();
    }
  }
  else if(topic.startsWith(TOPIC_BASE_COUNTER_GAS))
  {
    counterGas.callback(topic, payload);
  }
  else if(topic.startsWith(TOPIC_BASE_THERMO_BUSSES))
  {
    thermoBusses.callback(topic, payload);
  }
}
