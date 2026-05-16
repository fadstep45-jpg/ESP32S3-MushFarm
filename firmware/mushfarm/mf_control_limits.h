#pragma once

#include <stdbool.h>

typedef struct mf_control_limits_state_t {
    bool co2_crit;
    bool rh_min_crit;
    bool rh_max_crit;
    bool substrate_overheat;
    bool air_temp_crit;
    bool condensate_guard;
    float safety_exhaust_pct;
    float safety_hum_pct;
    float condensate_exhaust_floor_pct;
    float condensate_hum_cap_pct;
} mf_control_limits_state_t;

void mf_control_limits_evaluate(mf_control_limits_state_t *out);
bool mf_control_limits_hard_safe();
