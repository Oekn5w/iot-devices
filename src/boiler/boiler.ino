#include "../secrets_general.h"

#include "WiFi.h"
#include "PubSubClient.h"
#include "Thermistor.h"

#define TOPIC_TEMP_WATER "boiler/water/temperature"
#define TOPIC_TEMP_BOARD "boiler/board/temperature"
#define TOPIC_STATUS_BOARD "boiler/board/status"
#define TOPIC_CONTROL_HEATER "boiler/water/heater/set"
#define TOPIC_STATUS_HEATER "boiler/water/heater/status"
#define PAYLOAD_BOARD_AVAIL "running"
#define PAYLOAD_BOARD_NA "dead"

WiFiClient espclient;
PubSubClient client(espclient);

Thermistor Therm_Boiler(35, Thermistor::Type::B3988, TOPIC_TEMP_WATER, &client);
Thermistor Therm_Board(34, Thermistor::Type::B4300, TOPIC_TEMP_WATER, &client);

void setup()
{
}

void loop()
{

}
