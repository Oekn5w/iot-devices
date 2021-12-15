#include <Arduino.h>
#include "../secrets_general.h"
#include "../config_general.h"
#include "hw_config.h"
#include "config.h"
#include "secrets.h"
#include <buildinfo.h>

#include "WiFi.h"
#include "PubSubClient.h"
#include "Thermistor.h"
#include "Shutter.h"

// not including the two headers here makes it failing to find them,
// might be solved with #110 on makeEspArduino
#include "Update.h"
#include "ESPmDNS.h"
#include "ArduinoOTA.h"

WiFiClient espclient;
PubSubClient client(espclient);

Thermistor Therm_Board(BOARD_PIN_TEMP, Thermistor::Type::B4300, TOPIC_BOARD_TEMP, &client);

Thermistor Therm_Relais_A(PIN_A_TEMP, Thermistor::Type::B4300, TOPIC_RELAIS_A_TEMP, &client);
Thermistor Therm_Relais_B(PIN_B_TEMP, Thermistor::Type::B4300, TOPIC_RELAIS_B_TEMP, &client);
Thermistor Therm_Relais_C(PIN_C_TEMP, Thermistor::Type::B4300, TOPIC_RELAIS_C_TEMP, &client);

Shutter Shutter_A(
  {{SHUTTER_A_PIN_IN_D, SHUTTER_A_PIN_IN_U}, {SHUTTER_A_PIN_OUT_D, SHUTTER_A_PIN_OUT_U}},
  {SHUTTER_A_TIME_0_1, SHUTTER_A_TIME_1_2, SHUTTER_A_TIME_1_0, SHUTTER_A_TIME_2_1},
  BASE_TOPIC_SHUTTER_A, &client);
Shutter Shutter_B(
  {{SHUTTER_B_PIN_IN_D, SHUTTER_B_PIN_IN_U}, {SHUTTER_B_PIN_OUT_D, SHUTTER_B_PIN_OUT_U}},
  {SHUTTER_B_TIME_0_1, SHUTTER_B_TIME_1_2, SHUTTER_B_TIME_1_0, SHUTTER_B_TIME_2_1},
  BASE_TOPIC_SHUTTER_B, &client);
Shutter Shutter_C(
  {{SHUTTER_C_PIN_IN_D, SHUTTER_C_PIN_IN_U}, {SHUTTER_C_PIN_OUT_D, SHUTTER_C_PIN_OUT_U}},
  {SHUTTER_C_TIME_0_1, SHUTTER_C_TIME_1_2, SHUTTER_C_TIME_1_0, SHUTTER_C_TIME_2_1},
  BASE_TOPIC_SHUTTER_C, &client);

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void check_connectivity();
void IRAM_ATTR handleInterrupt();

unsigned long connectivity_timevar;
byte n_attempts;

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(MQTT_CLIENT_ID);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

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

  ArduinoOTA.onStart([]() {
    if (client.connected())
    {
      client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_OTA, true);
    }
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
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
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

  Serial.println("OTA service set up");

  client.setServer(SECRET_MQTT_HOST, SECRET_MQTT_PORT);

  Therm_Board.setup();

  Therm_Relais_A.setup();
  Therm_Relais_B.setup();
  Therm_Relais_C.setup();

  Shutter_A.setup();
  Shutter_B.setup();
  Shutter_C.setup();

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

  Therm_Relais_A.loop();
  Therm_Relais_B.loop();
  Therm_Relais_C.loop();

  Shutter_A.loop();
  Shutter_B.loop();
  Shutter_C.loop();

  delay(5);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String strTopic(topic);
  String strPayload = "";
  for (int i = 0; i < length; i++) {
    strPayload += (char)payload[i];
  }
  if(strTopic.startsWith(BASE_TOPIC_SHUTTER_A))
  {
    Shutter_A.callback(strTopic, strPayload);
  }
  else if(strTopic.startsWith(BASE_TOPIC_SHUTTER_B))
  {
    Shutter_B.callback(strTopic, strPayload);
  }
  else if(strTopic.startsWith(BASE_TOPIC_SHUTTER_C))
  {
    Shutter_C.callback(strTopic, strPayload);
  }
}

void check_connectivity()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if (!connectivity_timevar || millis() > connectivity_timevar)
    {
      n_attempts++;
      if (n_attempts > 10 && 
          Shutter_A.getMovementState() == Shutter::stMovementState::mvSTOPPED &&
          Shutter_B.getMovementState() == Shutter::stMovementState::mvSTOPPED &&
          Shutter_C.getMovementState() == Shutter::stMovementState::mvSTOPPED
        )
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
        Shutter_A.setupMQTT();
        Shutter_B.setupMQTT();
        Shutter_C.setupMQTT();
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
