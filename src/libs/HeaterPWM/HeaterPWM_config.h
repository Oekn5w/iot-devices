
#define KEEPALIVE_INTERVAL 300000 // 5 min
// #define PWM_INVERSE (0)
#define PWM_ALWAYS (0)
#define PWM_FREQ (1000)
#define PWM_RES_BITS (10)
#define PWM_DC_MAX ((1<<(PWM_RES_BITS)) - 1)
#define PWM_CAPPED (1)
#if PWM_CAPPED > 0
#define PWM_DC_CAP_LOW (2)
#define PWM_DC_CAP_HIGH (PWM_DC_MAX - PWM_DC_CAP_LOW)
static_assert(PWM_DC_CAP_HIGH > PWM_DC_CAP_LOW);
#endif
static_assert(!(PWM_ALWAYS && !PWM_CAPPED));

#define MSG_BUFFER_SIZE	(10)

#define HEATER_SWITCH_TOPIC "/command/switch"
#define HEATER_SWITCH_PAYLOAD_ON "ON"
#define HEATER_SWITCH_PAYLOAD_OFF "OFF"

#define HEATER_POWER_TOPIC "/command/power"
#define HEATER_PWM_TOPIC "/command/pwm"
#define HEATER_PWMINFO_TOPIC "/command/pwm_max"

#define HEATER_STATE_SWTICH_TOPIC "/state/switch"
#define HEATER_STATE_PWM_TOPIC "/state/pwm"
#define HEATER_STATE_ACT_TOPIC "/state/actuated"
