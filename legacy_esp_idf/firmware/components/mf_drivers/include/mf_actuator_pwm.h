#pragma once

#include "esp_err.h"

typedef enum {
    MF_ACT_FAN = 0,
    MF_ACT_HUMIDIFIER = 1,
    MF_ACT_LIGHT = 2,
} mf_actuator_channel_t;

esp_err_t mf_actuator_pwm_init_safe_off(void);
esp_err_t mf_actuator_pwm_set_percent(mf_actuator_channel_t ch, float percent);
