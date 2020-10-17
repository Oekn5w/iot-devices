#ifndef shutter_h
#define shutter_h

#include "PubSubClient.h"
#include "Shutter_config.h"

typedef unsigned long typeTime;
typedef long typeDeltaTime;

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
      typeTime startTime;
      typeTime endTime;
      float startPercentage;
      typeTime t0;
    };

    struct stTimings {
      // fully open to touching the sill
      typeDeltaTime closing;

      // touching the sill to motor shutoff (closing the gaps)
      typeDeltaTime full_closing;

      // barely touching to fully open
      typeDeltaTime opening;

      // downwards motor shutoff to barely touching the sill
      typeDeltaTime full_opening;
    };

    Shutter(stPins Pins, stTimings Timings, String topic_base, PubSubClient* client);

    void callback(String topic, String payload);
    void setSubscriptions();
    void interrupt();

    void setup(void (* fcn_interrupt)());
    void loop();

    stState getState() const;
    stMovementState getMovementState() const;
    float getClosedPercentage() const;
    bool getConfidence() const;

  private:
    stPins Pins;
    stTimings Timings;
    String topic_base;
    PubSubClient* mqttClient;

    // saving for interrupt handling
    int save_up;
    int save_down;

    stMovementState movement_state;

    stCalcBase calcBase;

    float queued_target_value;

    typeTime actuation_time;
    typeTime publish_time;
    typeTime confidence_subscription_timeout;
    typeTime button_time;

    // 0 -> open, 100 -> touching sill, 200 -> closed gaps
    float percentage_closed;
    float percentage_closed_published;

    stState state_published;
    stMovementState movement_state_published;

    bool is_confident;

    void actuation(float targetValue);
    void actuationRaw(stMovementState toMove, typeDeltaTime duration);
    void actuationLoop();
    typeTime updateOutput();

    void ButtonUpwardsPressed();
    void ButtonUpwardsReleased();
    void ButtonDownwardsPressed();
    void ButtonDownwardsReleased();

    void setTarget(float targetValue);
    void calibrate();

    void publishAll(bool forcePublish = false);
    void publishState(bool checkConnectivity = true, bool forcePublish = false);
    void publishValue(bool checkConnectivity = true, bool forcePublish = false);

    float getIntermediatePercentage(typeTime time);
    float getPercentage(typeDeltaTime trel, stMovementState movement, float fallback = 0.0f) const;
    typeDeltaTime getRelativeTime(float percentage, stMovementState movement) const;

    static bool equalWithEps(float value);
    static float clampPercentage(float value);
};

#endif
