// Time [ms] up to which the shutter travels to the next major stop (full open - on sill - full closed)
#define SHUTTER_TIMEOUT_BUTTON (700)
#define SHUTTER_CALIB_OVERSHOOT (2000)

#define SHUTTER_PUBLISH_INTERVAL (1000)

#define RELAIS_LOW (0)
#define RELAIS_HIGH (1)
#define BUTTON_ACTIVE_LOW 1

#define SHUTTER_TOPIC_STATE_PUBLISH "/state/current"
#define SHUTTER_TOPIC_MVSTATE_PUBLISH "/state/movement"

#define SHUTTER_PAYLOAD_STATE_OPEN "open"
#define SHUTTER_PAYLOAD_STATE_OPENING "opening"
#define SHUTTER_PAYLOAD_STATE_CLOSING "closing"
#define SHUTTER_PAYLOAD_STATE_CLOSED "closed"
#define SHUTTER_PAYLOAD_STATE_STOPPED "stopped"
// 0 -> open, 100 -> touching sill, 200 -> closed gaps
#define SHUTTER_PAYLOAD_STATE_THRESHOLD (90)

#define SHUTTER_TOPIC_POSITION_COMMAND "/position/target"
#define SHUTTER_TOPIC_POSITION_PUBLISH "/position/current"
#define SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED "/position/detailed"

#define SHUTTER_TOPIC_COMMAND "/command"
#define SHUTTER_PAYLOAD_COMMAND_OPEN "open"
#define SHUTTER_PAYLOAD_COMMAND_CLOSE "close"
#define SHUTTER_PAYLOAD_COMMAND_STOP "stop"

#define SHUTTER_TOPIC_EMULATE_UP "/emulate/upwards"
#define SHUTTER_TOPIC_EMULATE_DOWN "/emulate/downwards"

#define SHUTTER_PAYLOAD_BUT_DOWN "pressed"
#define SHUTTER_PAYLOAD_BUT_UP "released"
