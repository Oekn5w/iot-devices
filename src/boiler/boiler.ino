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
Thermistor Therm_Board(34, Thermistor::Type::B4300, TOPIC_BOARD_TEMP, &client);
Heater heater(33, TOPIC_HEATER_STATUS, &client);

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void check_connectivity();

unsigned long connectivity_timevar;
byte n_attempts;

void setup()
{
  Serial.begin(115200);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
  WiFi.setHostname(MQTT_CLIENT_ID);

  n_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && n_attempts++ < 20)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if(n_attempts >= 20)
  {
    Serial.print("WiFi unable to connect. Status is: ");
    Serial.println(WiFi.status());
    Serial.println("Rebooting ESP device!");
    ESP.restart();
  }
  Serial.println("Connected to WiFi!");

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

  Serial.println("OTA service set up");

  client.setServer(SECRET_MQTT_HOST, SECRET_MQTT_PORT);

  Therm_Board.setup();
  Therm_Boiler.setup();
  heater.setup();

  client.setCallback(mqtt_callback);

  n_attempts = 0;
  connectivity_timevar = 0;

  Serial.println("Setup done!");
}

void loop()
{
  check_connectivity();

  ArduinoOTA.handle();
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
      heater.turnOn();
    }
    else if(strPayload == PAYLOAD_HEATER_OFF)
    {
      heater.turnOff();
    }
  }
}

void check_connectivity()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if(!connectivity_timevar || millis() > connectivity_timevar)
    {
      n_attempts++;
      if(n_attempts > 10)
      {
        Serial.println("WiFi unable to reconnect. Rebooting ESP device!");
        ESP.restart();
      }
      Serial.println("WiFi disconnected. Trying to reconnect ...");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
      connectivity_timevar = millis() + 3000;
      if (WiFi.status() != WL_CONNECTED) // don't return when connected to check on MQTT
      {
        return;
      }
    }
    else
    {
      return;
    }
  }
  if(n_attempts && connectivity_timevar)
  {
    Serial.println("WiFi reconnected!");
  }
  if (!client.connected())
  {
    if(!connectivity_timevar || connectivity_timevar > millis())
    {
      if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD, TOPIC_BOARD_STATUS, 0, true, PAYLOAD_BOARD_NA))
      {
        client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_AVAIL, true);
        client.publish(TOPIC_BOARD_BUILDVER, _BuildInfo.src_version, true);
        String buildtime = String(_BuildInfo.date) + "T" + String(_BuildInfo.time);
        client.publish(TOPIC_BOARD_BUILDTIME, buildtime.c_str(), true);
        client.subscribe(TOPIC_HEATER_CONTROL);
        Serial.println("MQTT connected!");
      }
      else
      {
        Serial.print("MQTT connection failed, rc=");
        Serial.print(client.state());
        Serial.println(", trying again in 1 second");
        connectivity_timevar = millis() + 1000;
        return;
      }
    }
  }
  n_attempts = 0;
  connectivity_timevar = 0;
}
