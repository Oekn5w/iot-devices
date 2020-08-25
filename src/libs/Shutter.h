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

    enum stState {
      STOPPED,
      OPENING,
      CLOSING
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
    void interrupt();

    void setup(void (* fcn_interrupt)());
    void loop();

    stState getState() const;
    float getClosedPercantage() const;
    bool getConfidence() const();

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

    // 0 -> open, 100 -> touching sill, 200 -> closed gaps
    float percentage_closed;

    bool is_confident;

    stState state;

};

#endif
