#include "mf_control_limits.h"
#include "mf_control_profile.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"

void mf_control_limits_evaluate(mf_control_limits_state_t *out) {
    if (!out) return;
    const mf_control_profile_t *p = mf_control_profile_current();
    float rh = mf_scd41_rh_percent();
    float co2 = mf_scd41_co2_ppm();
    float air_t = mf_scd41_temp_c();
    float sub_t = mf_mlx90614_object_c();
    float delta = sub_t - air_t;

    out->co2_crit = co2 >= p->co2_emergency_absolute_ppm;
    out->rh_min_crit = rh <= p->rh_min_crit_percent;
    out->rh_max_crit = rh >= p->rh_max_crit_percent;
    out->substrate_overheat = sub_t >= p->substrate_temp_max_crit_c;
    out->air_temp_crit =
        air_t <= p->air_temp_min_crit_c || air_t >= p->air_temp_max_crit_c;
    out->condensate_guard = delta > p->max_substrate_air_delta_c;

    out->safety_exhaust_pct = 0.0f;
    out->safety_hum_pct = 0.0f;
    out->condensate_exhaust_floor_pct = 0.0f;
    out->condensate_hum_cap_pct = 100.0f;

    if (out->co2_crit) {
        out->safety_exhaust_pct = 100.0f;
    }
    if (out->rh_max_crit) {
        out->safety_hum_pct = 0.0f;
    }
    if (out->substrate_overheat || out->air_temp_crit) {
        if (out->safety_exhaust_pct < p->inlet_fan_max_duty_percent) {
            out->safety_exhaust_pct = p->inlet_fan_max_duty_percent;
        }
    }
    if (out->condensate_guard) {
        out->condensate_exhaust_floor_pct = p->inlet_fan_min_duty_percent;
        if (out->condensate_exhaust_floor_pct < 30.0f) {
            out->condensate_exhaust_floor_pct = 30.0f;
        }
        out->condensate_hum_cap_pct = p->hum_cooperate_cap_pct;
    }
}

bool mf_control_limits_hard_safe() {
    mf_control_limits_state_t lim;
    mf_control_limits_evaluate(&lim);
    return !lim.co2_crit && !lim.rh_min_crit && !lim.substrate_overheat && !lim.air_temp_crit;
}
