#include "../secrets_general.h"
#include "../config_general.h"
#include "config.h"
#include "secrets.h"
#include <buildinfo.h>

#include "WiFi.h"
#include "PubSubClient.h"
#include "Thermistor.h"
#include "Heater.h"

// not including the two headers here makes it failing to find them,
// might be solved with #110 on makeEspArduino
#include "Update.h"
#include "ESPmDNS.h"
#include "ArduinoOTA.h"

WiFiClient espclient;
PubSubClient client(espclient);

Thermistor Therm_Boiler(35, Thermistor::Type::B3988, TOPIC_WATER_TEMP, &client);
Thermistor Therm_Board(34, Thermistor::Type::B4300, TOPIC_WATER_TEMP, &client);
Heater heater(33, TOPIC_HEATER_STATUS, &client);

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void attempt_reconnect();

void setup()
{
  Serial.begin(115200);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
  WiFi.setHostname(MQTT_CLIENT_ID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  ArduinoOTA.setPassword(SECRET_OTA_PWD);

  ArduinoOTA
    .onStart([]() {
      if (client.connected())
      {
        client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_OTA, true);
      }
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  client.setServer(SECRET_MQTT_HOST, 1883);

  Therm_Board.setup();
  Therm_Boiler.setup();
  heater.setup();

  client.setCallback(mqtt_callback);
}

void loop()
{
  ArduinoOTA.handle();
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
  if(strTopic == TOPIC_HEATER_CONTROL)
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
    if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD, TOPIC_BOARD_STATUS, 0, true, PAYLOAD_BOARD_NA))
    {
      client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_AVAIL, true);
      client.publish(TOPIC_BOARD_BUILDVER, _BuildInfo.src_version, true);
      String buildtime = String(_BuildInfo.date) + "T" + String(_BuildInfo.time);
      client.publish(TOPIC_BOARD_BUILDTIME, buildtime.c_str(), true);
      client.subscribe(TOPIC_HEATER_CONTROL);
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
