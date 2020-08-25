#include "../secrets_general.h"
#include "../config_general.h"
#include <buildinfo.h>

#ifndef SHUTTER_SINGLE_ID
#define SHUTTER_SINGLE_ID 0
#endif

#include "secrets.h"
#include "config.h"

#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include "Thermistor.h"
#include "Shutter.h"

// not including this header here makes it failing to find them,
// might be solved with #110 on makeEspArduino
#include "ESP8266mDNS.h"
#include "ArduinoOTA.h"

WiFiClient espclient;
PubSubClient client(espclient);

Thermistor Therm_Relais(Thermistor::Type::B4300, TOPIC_TEMP_RELAIS, &client);
Shutter Only_Shutter(SHUTTER_PINS, SHUTTER_TIMINGS, BASE_TOPIC_SHUTTER, 0, &client, &EEPROM);

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void attempt_reconnect();
void IRAM_ATTR handleInterrupt();

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(MQTT_CLIENT_ID);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  ArduinoOTA.setPassword(SECRET_OTA_PWD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();

  client.setServer(SECRET_MQTT_HOST, 1883);

  Therm_Relais.setup();
  Only_Shutter.setup(handleInterrupt);

  client.setCallback(mqtt_callback);
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected()) {
    attempt_reconnect();
  }
  client.loop();
  Therm_Relais.loop();
  Only_Shutter.loop();
  delay(50);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String strTopic(topic);
  String strPayload = "";
  for (int i = 0; i < length; i++) {
    strPayload += (char)payload[i];
  }
  if(strTopic.startsWith(BASE_TOPIC_SHUTTER))
  {
    Only_Shutter.callback(strTopic, strPayload);
  }
}

void attempt_reconnect()
{
  if (!client.connected())
  {
    if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD, TOPIC_STATUS_BOARD, 0, true, PAYLOAD_BOARD_NA))
    {
      client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_AVAIL, true);
      client.publish(TOPIC_BOARD_BUILDVER, _BuildInfo.src_version, true);
      String buildtime = String(_BuildInfo.date) + "T" + String(_BuildInfo.time);
      client.publish(TOPIC_BOARD_BUILDTIME, buildtime.c_str(), true);
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

void IRAM_ATTR handleInterrupt()
{
  Only_Shutter.interrupt();
}
