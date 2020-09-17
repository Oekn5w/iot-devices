#ifndef shutter_h
#define shutter_h

#include "PubSubClient.h"
#include "Shutter_config.h"
#include "EEPROM.h"

class Shutter
{
  public:
  struct stPinGroup {
      byte down;
      byte up;
    };
    
    struct stPins {
      stPinGroup input;
      stPinGroup actuator;
    };

    enum stMovementState {
      mvOPENING,
      mvCLOSING,
      mvSTOPPED
    };

    enum stState {
      OPENING,
      CLOSING,
      UNDEFINED,
      CLOSED,
      OPEN
    };

    struct stCalcBase {
      stMovementState state;
      unsigned int startTime;
      unsigned int endTime;
      float startPercentage;
      float targetPercentage;
      unsigned int t0;
    };

    struct stTimings {
      // fully open to touching the sill
      unsigned int closing;

      // touching the sill to motor shutoff (closing the gaps)
      unsigned int full_closing;

      // barely touching to fully open
      unsigned int opening;

      // downwards motor shutoff to barely touching the sill
      unsigned int full_opening;
    };

    Shutter(stPins Pins, stTimings Timings, String topic_base,
      unsigned int eep_addr, PubSubClient* client, EEPROMClass* eep);

    void callback(String topic, String payload);
    void mqttReconnect();
    void setSubscriptions();
    void interrupt();

    void setup(void (* fcn_interrupt)());
    void loop();

    stState getState() const;
    float getClosedPercentage() const;
    bool getConfidence() const;

  private:
    stPins Pins;
    stTimings Timings;
    String topic_base;
    unsigned int eep_addr;
    PubSubClient* mqttClient;
    EEPROMClass* eep;

    // saving for interrupt handling
    bool save_up;
    bool save_down;

    stMovementState movement_state;

    stCalcBase calcBase;

    float queued_target_value;
    float actuation_time;

    // 0 -> open, 100 -> touching sill, 200 -> closed gaps
    float percentage_closed;
    float percentage_closed_published;

    stState state_published;

    bool is_confident;

    void actuation(float targetValue);
    void actuation(stMovementState toMove, unsigned int duration);
    void actuation();
    void updateOutput();

    void setTarget(float targetValue);
    void calibrate();

    void publishAll();
    void publishState(bool checkConnectivity = true);
    void publishValue(bool checkConnectivity = true);

    float getIntermediatePercentage(unsigned int time);
    float getPercentage(unsigned int trel, stMovementState movement, float fallback = 0.0f) const;
    unsigned int getRelativeTime(float percentage, stMovementState movement) const;

    static bool equalWithEps(float value);
    static float clampPercentage(float value);
};

#endif
