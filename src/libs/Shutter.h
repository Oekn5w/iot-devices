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
      STOPPED,
      OPENING,
      CLOSING
    };

    enum stState {
      OPEN,
      PARTIALLY_OPEN,
      PARTIALLY_CLOSED,
      CLOSED,
      FULLY_CLOSED
    };

    struct stTimings {
      // fully open to touching the sill
      unsigned int down;

      // touching the sill to motor shutoff (closing the gaps)
      unsigned int full_closing;

      // barely touching to fully open
      unsigned int up;

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

    stMovementState getMovementState() const;
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

    float queued_target_value;

    // 0 -> open, 100 -> touching sill, 200 -> closed gaps
    float percentage_closed;
    float percentage_closed_published;

    bool is_confident;

    void publishAll();
    void publishState(bool checkConnectivity = true) const;
    void publishValue(bool checkConnectivity = true);

};

#endif
