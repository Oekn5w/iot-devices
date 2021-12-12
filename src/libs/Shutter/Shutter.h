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

      String toString() const
      {
          return (
            "[" + String(this->closing) + 
            "," + String(this->full_closing) +
            "," + String(this->opening) +
            "," + String(this->full_opening) +
            "]");
      };
    };

    Shutter(stPins Pins, stTimings Timings, String topicBase,
        PubSubClient* client, float motorValueMax = SHUTTER_MOTOR_SHUTOFF_DEFAULT);

    void callback(String topic, String payload);
    void setupMQTT();
    void handleInput();

    void setup();
    void loop();

    stState getState() const;
    stMovementState getMovementState() const;
    float getClosedPercentage() const;
    bool getConfidence() const;

  private:
    stPins Pins;
    stTimings Timings;
    String topicBase;
    float motorValueMax;
    PubSubClient* mqttClient;

    // saving for interrupt handling
    int save_up;
    int save_down;

    stMovementState stateMovement;

    stCalcBase calcBase;

    float targetValueQueued;

    typeTime timeActuation;
    typeTime timePublish;
    typeTime timeoutConfidenceSubscription;
    typeTime timeoutCalibrationSubscription;
    typeTime timeButton;

    // 0 -> open, 100 -> touching sill, 200 -> closed gaps
    float percentageClosed;
    float percentageClosedPublished;

    stState statePublished;
    stMovementState stateMovementPublished;

    bool isConfident;

    bool calibrationMode;
    typeTime calibrationTimeout;
    int calibrationState;
    stTimings calibrationTimings;
    typeTime calibrationCache;
    void calibrationLoop();
    void calibrationAbort();
    void calibrationInit();
    void calibrationPublish(bool publishFinal = false);

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
    float clampPercentage(float value) const;
};

#endif
