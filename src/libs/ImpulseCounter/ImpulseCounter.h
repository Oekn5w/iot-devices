#ifndef ImpulseCounter_h
#define ImpulseCounter_h

#include "PubSubClient.h"

struct ImpulseCounterSettings {
  byte inPin;
  bool inRisingEdge = false;
  bool inPullup = true;
  double incValue = 1.0;
  String formatter = "%.0f";
  bool pulseEnable = false;
  bool ledEnable = false;
  byte ledPin;
  bool ledActiveHigh = true;
  String topicBase;
};

class ImpulseCounter
{
  public:
    ImpulseCounter(const ImpulseCounterSettings * pSettings, PubSubClient * client);

    void callback(String topic, const String & payload);
    void setupMQTT();

    void setup();
    void loop();
  private:
    PubSubClient * mqttClient;
    unsigned long initTimeout;
    
    double counterVal;
    double counterValPublished;
    bool listening;

    int saveIn;

    const ImpulseCounterSettings * pSet;

    void handleInput();

    void unsubMQTT();

    void publishAll();
    void publishValue(bool checkConnectivity = true);
    void publishPulse();
};

#endif
