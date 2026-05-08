#pragma once

#include <stdint.h>

enum mf_actuator_t {
    MF_ACT_FAN = 0,
    MF_ACT_HUMIDIFIER,
    MF_ACT_LIGHT,
    MF_ACT__COUNT
};

void mf_actuators_init_safe_off();
void mf_actuators_set_percent(mf_actuator_t which, float percent);
float mf_actuators_get_percent(mf_actuator_t which);
void mf_actuators_all_off();
