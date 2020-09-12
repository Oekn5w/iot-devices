// Time [ms] up to which the shutter travels to the next major stop (full open - on sill - full closed)
#define SHUTTER_TIMEOUT_BUTTON (700)

#define SHUTTER_TOPIC_STATE_COMMAND "/state/set"
#define SHUTTER_TOPIC_STATE_PUBLISH "/state/current"

#define SHUTTER_PAYLOAD_STATE_OPEN "open"
#define SHUTTER_PAYLOAD_STATE_PART_OPEN "partially-open"
#define SHUTTER_PAYLOAD_STATE_PART_CLOSED "partially-closed"
#define SHUTTER_PAYLOAD_STATE_CLOSED "closed"
#define SHUTTER_PAYLOAD_STATE_FULLCLOSED "fully-closed"

// 0 -> open, 100 -> touching sill, 200 -> closed gaps
#define SHUTTER_PAYLOAD_STATE_THRESHOLD_OPEN (10)
#define SHUTTER_PAYLOAD_STATE_THRESHOLD_PO_PC (50)
#define SHUTTER_PAYLOAD_STATE_THRESHOLD_CLOSED (90)
#define SHUTTER_PAYLOAD_STATE_THRESHOLD_C_FC (190)

#define SHUTTER_TOPIC_POSITION_COMMAND "/position/set"
#define SHUTTER_TOPIC_POSITION_PUBLISH "/position/current"
