#include "secrets.h"
#include <buildinfo.h>

#if ESP32==1
#include "WiFi.h"
#else
#include "ESP8266WiFi.h"
#endif
#include "PubSubClient.h"

// not including the two headers here makes it failing to find them,
// might be solved with #110 on makeEspArduino
#if ESP32==1
#include "Update.h"
#include "ESPmDNS.h"
#else
#include "ESP8266mDNS.h"
#endif
#include "ArduinoOTA.h"

#define TOPIC_BOARD_STATUS "esp-initial-setup/board/status"
#define TOPIC_BOARD_BUILDVER "esp-initial-setup/board/build/version"
#define TOPIC_BOARD_BUILDTIME "esp-initial-setup/board/build/time"
#define TOPIC_IP_ADDR "esp-initial-setup/info/IP"
#define TOPIC_MAC_ADDR "esp-initial-setup/info/MAC"

#define MQTT_CLIENT_ID "ESP-Initial-Setup-Device"
#define MQTT_RESEND_INTERVAL (5000)

WiFiClient espclient;
PubSubClient client(espclient);

void check_connectivity();
void send_info();

unsigned long connectivity_timevar;
unsigned long mqtt_resend_timevar;
byte n_attempts;

String buildtime = String(_BuildInfo.date) + "T" + String(_BuildInfo.time);

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

  n_attempts = 0;
  connectivity_timevar = 0;
  mqtt_resend_timevar = 0;

  Serial.println("Setup done!");
}

void loop()
{
  check_connectivity();

  ArduinoOTA.handle();
  client.loop();

  if(mqtt_resend_timevar && millis() > mqtt_resend_timevar)
  {
    send_info();
  }

  delay(50);
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
      if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD))
      {
        send_info();
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

void send_info()
{
  if (client.connected())
  {
    client.publish(TOPIC_BOARD_STATUS, "temporary-online", false);
    client.publish(TOPIC_BOARD_BUILDVER, _BuildInfo.src_version, false);
    client.publish(TOPIC_BOARD_BUILDTIME, buildtime.c_str(), false);
    client.publish(TOPIC_IP_ADDR, WiFi.localIP().toString().c_str(), false);
    client.publish(TOPIC_MAC_ADDR, WiFi.macAddress().c_str(), false);
    mqtt_resend_timevar = millis() + MQTT_RESEND_INTERVAL;
  }
}
