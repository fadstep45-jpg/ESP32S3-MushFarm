#include "mf_actuators.h"
#include "mf_board.h"
#include "mf_log.h"
#include <Arduino.h>

// LEDC PWM via Arduino-ESP32 core 3.x (new API: ledcAttach/ledcWrite directly on pin).
#define MF_PWM_FREQ_HZ   25000u
#define MF_PWM_RES_BITS  10u
#define MF_PWM_MAX_DUTY  ((1u << MF_PWM_RES_BITS) - 1u)

static const int s_pins[MF_ACT__COUNT] = {
    MF_PWM_FAN_PIN,
    MF_PWM_HUM_PIN,
    MF_PWM_LIGHT_PIN,
};
static float s_percent[MF_ACT__COUNT] = {0};

static uint32_t percent_to_duty(float percent) {
    if (percent <= 0.0f) return 0;
    if (percent >= 100.0f) return MF_PWM_MAX_DUTY;
    return (uint32_t)((percent / 100.0f) * (float)MF_PWM_MAX_DUTY + 0.5f);
}

void mf_actuators_init_safe_off() {
    for (int i = 0; i < (int)MF_ACT__COUNT; ++i) {
        // ledcAttach in Arduino-ESP32 3.x signature: ledcAttach(pin, freq, res_bits)
        ledcAttach(s_pins[i], MF_PWM_FREQ_HZ, MF_PWM_RES_BITS);
        ledcWrite(s_pins[i], 0);
        s_percent[i] = 0.0f;
    }
    mf_log_info("act", "init safe OFF (fan=%d hum=%d light=%d)",
                (int)s_pins[MF_ACT_FAN], (int)s_pins[MF_ACT_HUMIDIFIER], (int)s_pins[MF_ACT_LIGHT]);
}

void mf_actuators_set_percent(mf_actuator_t which, float percent) {
    if ((int)which < 0 || (int)which >= (int)MF_ACT__COUNT) return;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    s_percent[which] = percent;
    ledcWrite(s_pins[which], percent_to_duty(percent));
}

float mf_actuators_get_percent(mf_actuator_t which) {
    if ((int)which < 0 || (int)which >= (int)MF_ACT__COUNT) return 0.0f;
    return s_percent[which];
}

void mf_actuators_all_off() {
    for (int i = 0; i < (int)MF_ACT__COUNT; ++i) {
        mf_actuators_set_percent((mf_actuator_t)i, 0.0f);
    }
    mf_log_warn("act", "all OFF");
}
