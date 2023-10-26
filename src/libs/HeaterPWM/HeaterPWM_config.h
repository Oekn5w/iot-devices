
#define KEEPALIVE_INTERVAL 300000 // 5 min

#define PWM_FREQ (1000)
#define PWM_RES_BITS (10)
#define PWM_DC_MAX ((1<<(PWM_RES_BITS)) - 1)

#ifndef PWM_ALWAYS
#define PWM_ALWAYS (0)
#endif // PWM_ALWAYS

#ifndef PWM_CAPPED
#define PWM_CAPPED (0)
#endif // PWM_CAPPED

static_assert(!(PWM_ALWAYS && !PWM_CAPPED));

#if PWM_CAPPED > 0
#define PWM_DC_CAP_LOW (2)
#define PWM_DC_CAP_HIGH (PWM_DC_MAX - PWM_DC_CAP_LOW)
static_assert(PWM_DC_CAP_HIGH > PWM_DC_CAP_LOW);
#endif // PWM_CAPPED

// the resistor cannot puldown the 12V signal sufficiently to register the low part anymore
// apply full signal then, overriding the High CAP unless it is below that theshold
#define PWM_DC_FULL_THRESH ((int)(PWM_DC_MAX * 0.9952))

#define MSG_BUFFER_SIZE	(10)

#define PWMHEATER_SWITCH_TOPIC "/command/switch"
#define PWMHEATER_SWITCH_PAYLOAD_ON "ON"
#define PWMHEATER_SWITCH_PAYLOAD_OFF "OFF"

#define PWMHEATER_POWER_TOPIC "/command/power"
#define PWMHEATER_PWM_TOPIC "/command/pwm"
#define PWMHEATER_PWMINFO_TOPIC "/command/pwm_max"

#define PWMHEATER_STATE_SWTICH_TOPIC "/state/switch"
#define PWMHEATER_STATE_PWM_TOPIC "/state/pwm"
#define PWMHEATER_STATE_ACT_TOPIC "/state/actuated"
