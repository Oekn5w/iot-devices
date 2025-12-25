#include <buildinfo.h>


#if ESP32==1
#include "WiFi.h"
#else
#include "ESP8266WiFi.h"
#endif
#include "PubSubClient.h"

#include "WiFiUdp.h"
#include "NTPClient.h"

#if ESP32==1
// not including the two headers here makes it failing to find them,
// might be solved with #110 on makeEspArduino
#include "Update.h"
#include "ESPmDNS.h"
#else
#include "ESP8266mDNS.h"
#endif
#include "ArduinoOTA.h"

#define TOPIC_BOARD_STATUS (TOPIC_BOARD_BASE "/status")

#define CONNECTION_PERIOD_WIFI (3000)
#define CONNECTION_PERIOD_MQTT (2000)

// ----------
// needs to be defined in main ino!
void mqtt_callback(const String & topic, const String & payload);
void mqtt_setup();
// ----------

WiFiClient espclient;
PubSubClient client(espclient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, SECRET_NTP_SERVER);

namespace common{

  void mqtt_callback(char* topic, byte* payload, unsigned int length);

  unsigned long timevar_connectivity;
  byte n_attempts;

  bool uptime_published = false;
  unsigned long timevar_wifi = 0;
  unsigned long timevar_mqtt = 0;

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
    timevar_wifi = millis();
    Serial.println("Connected to WiFi!");

    ArduinoOTA.setPassword(SECRET_OTA_PWD);

    ArduinoOTA.onStart([]() {
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
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();

    Serial.println("OTA service set up");

    timeClient.begin();

    client.setServer(SECRET_MQTT_HOST, SECRET_MQTT_PORT);

    client.setCallback(mqtt_callback);

    n_attempts = 0;
    timevar_connectivity = 0;

    Serial.println("Setup done!");
  }

  void common_loop();

  void loop()
  {
    common_loop();

    ArduinoOTA.handle();
    client.loop();
  }

  void mqtt_callback(char* topic, byte* payload, unsigned int length)
  {
    String strTopic(topic);
    String strPayload = "";
    for (int i = 0; i < length; i++) {
      strPayload += (char)payload[i];
    }
    if(strTopic.startsWith(TOPIC_BOARD_BASE))
    {
      if (strTopic.endsWith("reboot") && (strcmp(strPayload.c_str(),MQTT_CLIENT_ID) == 0))
      {
        Serial.println("Rebooting ESP device because of MQTT command!");
        client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_MQTT_REBOOT, true);
        delay(50);
        ESP.restart();
      }
      #if ESP32==1
      else if (strTopic.endsWith("reconnect") && (strcmp(strPayload.c_str(),MQTT_CLIENT_ID) == 0))
      {
        Serial.println("Restarting Wifi!");
        client.publish(TOPIC_BOARD_STATUS, PAYLOAD_BOARD_MQTT_WIFI, true);
        delay(50);
        timeClient.end();
        WiFi.disconnect();
        return;
      }
      #endif
    }
    mqtt_callback(strTopic, strPayload);
  }

  void common_loop()
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if(!timevar_connectivity || (millis() - timevar_connectivity) >= 3000)
      {
        n_attempts++;
        if(n_attempts > 100)
        {
          Serial.println("WiFi unable to reconnect. Rebooting ESP device!");
          ESP.restart();
        }
        Serial.println("WiFi disconnected. Trying to reconnect ...");
        WiFi.disconnect();
        timeClient.end();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
        timevar_connectivity = millis();
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
    if(n_attempts && timevar_connectivity)
    {
      timevar_wifi = millis();
      Serial.println("WiFi reconnected!");
      n_attempts = 0;
      timevar_connectivity = 0;
      timeClient.begin();
    }
    if (!client.connected())
    {
      if(!timevar_connectivity || (millis() - timevar_connectivity) > CONNECTION_PERIOD_MQTT)
      {
        if (client.connect(MQTT_CLIENT_ID, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD, TOPIC_BOARD_STATUS, 0, true, PAYLOAD_BOARD_NA))
        {
          client.publish((TOPIC_BOARD_BASE "/status"), PAYLOAD_BOARD_AVAIL, true);
          client.publish((TOPIC_BOARD_BASE "/build/version"), _BuildInfo.src_version, true);
          String buildtime = String(_BuildInfo.date) + "T" + String(_BuildInfo.time);
          client.publish((TOPIC_BOARD_BASE "/build/time"), buildtime.c_str(), true);
          client.publish((TOPIC_BOARD_BASE "/info/clientID"), MQTT_CLIENT_ID, true);
          client.publish((TOPIC_BOARD_BASE "/info/IP"), WiFi.localIP().toString().c_str(), true);
          client.publish((TOPIC_BOARD_BASE "/info/MAC"), WiFi.macAddress().c_str(), true);

          client.subscribe(TOPIC_BOARD_BASE "/command/reboot");
          #if ESP32==1
          client.subscribe(TOPIC_BOARD_BASE "/command/reconnect");
          #endif
          mqtt_setup();
          Serial.println("MQTT connected!");
          timevar_mqtt = millis();
        }
        else
        {
          Serial.print("MQTT connection failed, rc=");
          Serial.print(client.state());
          Serial.println(", trying again in 1 second");
          timevar_connectivity = millis();
          return;
        }
      }
    }
    timevar_connectivity = 0;
    if (client.connected())
    {
      if (!uptime_published)
      {
        timeClient.update();
        unsigned long epochOnBoot = timeClient.getEpochTime() - (millis() / 1000);
        String timestampBoot = timeClient.getFormattedDate(epochOnBoot);
        client.publish((TOPIC_BOARD_BASE "/times/boot"), timestampBoot.c_str(), true);
        uptime_published = true;
      }
      if (timevar_wifi > 0)
      {
        timeClient.update();
        unsigned long epochOnWiFiConnect = timeClient.getEpochTime() - ((millis() - timevar_wifi) / 1000);
        String timestampWiFi = timeClient.getFormattedDate(epochOnWiFiConnect);
        client.publish((TOPIC_BOARD_BASE "/times/wifi"), timestampWiFi.c_str(), true);
        timevar_wifi = 0;
      }
      if (timevar_mqtt > 0)
      {
        timeClient.update();
        unsigned long epochOnMQTTConnect = timeClient.getEpochTime() - ((millis() - timevar_mqtt) / 1000);
        String timestampMQTT = timeClient.getFormattedDate(epochOnMQTTConnect);
        client.publish((TOPIC_BOARD_BASE "/times/mqtt"), timestampMQTT.c_str(), true);
        timevar_mqtt = 0;
      }
    }
  }
}
