// Time [ms] up to which the shutter travels to the next major stop (full open - on sill - full closed)
#define SHUTTER_TIMEOUT_BUTTON (700)

#define SHUTTER_TOPIC_STATE_PUBLISH "/state/current"

#define SHUTTER_PAYLOAD_STATE_OPEN "open"
#define SHUTTER_PAYLOAD_STATE_OPENING "opening"
#define SHUTTER_PAYLOAD_STATE_CLOSING "closing"
#define SHUTTER_PAYLOAD_STATE_CLOSED "closed"
// 0 -> open, 100 -> touching sill, 200 -> closed gaps
#define SHUTTER_PAYLOAD_STATE_THRESHOLD (90)

#define SHUTTER_TOPIC_POSITION_COMMAND "/position/target"
#define SHUTTER_TOPIC_POSITION_PUBLISH "/position/current"
#define SHUTTER_TOPIC_POSITION_PUBLISH_DETAILED "/position/detailed"

#define SHUTTER_TOPIC_COMMAND "/command"
#define SHUTTER_PAYLOAD_COMMAND_OPEN "open"
#define SHUTTER_PAYLOAD_COMMAND_CLOSE "close"
#define SHUTTER_PAYLOAD_COMMAND_STOP "stop"
