#include "../secrets_general.h"

#include "WiFi.h"
#include "PubSubClient.h"
#include "Thermistor.h"
#include "Heater.h"

#define TOPIC_TEMP_WATER "boiler/water/temperature"
#define TOPIC_TEMP_BOARD "boiler/board/temperature"
#define TOPIC_STATUS_BOARD "boiler/board/status"
#define TOPIC_CONTROL_HEATER "boiler/water/heater/set"
#define TOPIC_STATUS_HEATER "boiler/water/heater/status"
#define PAYLOAD_BOARD_AVAIL "running"
#define PAYLOAD_BOARD_NA "dead"

#define MQTT_CLIENT_ID "ESP32-Boiler"

WiFiClient espclient;
PubSubClient client(espclient);

Thermistor Therm_Boiler(35, Thermistor::Type::B3988, TOPIC_TEMP_WATER, &client);
Thermistor Therm_Board(34, Thermistor::Type::B4300, TOPIC_TEMP_WATER, &client);
Heater heater(33, TOPIC_STATUS_HEATER, &client);

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void attempt_reconnect();

void setup()
{
  Serial.begin(115200);

  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  client.setServer(SECRET_MQTT_HOST, 1883);

  Therm_Board.setup();
  Therm_Boiler.setup();
  heater.setup();

  client.setCallback(mqtt_callback);
}

void loop()
{
  if (!client.connected()) {
    attempt_reconnect();
  }
  client.loop();
  Therm_Board.loop();
  Therm_Boiler.loop();
  heater.loop();
  delay(50);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String strTopic(topic);
  String strPayload = "";
  for (int i = 0; i < length; i++) {
    strPayload += (char)payload[i];
  }
  if(strTopic == TOPIC_CONTROL_HEATER)
  {
    if(strPayload == PAYLOAD_HEATER_ON)
    {
      heater.turn_on();
    }
    else if(strPayload == PAYLOAD_HEATER_OFF)
    {
      heater.turn_off();
    }
  }
}

void attempt_reconnect()
{
  if (!client.connected())
  {
    if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD))
    {
      client.subscribe(TOPIC_CONTROL_HEATER);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying in next loop
      delay(3000);
    }
  }
}
