#include "mf_control_profile.h"
#include "mf_log.h"
#include <string.h>

static mf_control_profile_t s_profile;

static void merge_safety(mf_control_profile_t *dst) {
    dst->rh_min_crit_percent = 60.0f;
    dst->rh_max_crit_percent = 99.0f;
    dst->co2_emergency_absolute_ppm = 8000.0f;
    dst->co2_sensor_trust_upper_ppm = 5000.0f;
    dst->clamp_pid_effective_setpoint_to_trust_range = true;
    dst->air_temp_min_crit_c = 5.0f;
    dst->air_temp_max_crit_c = 32.0f;
    dst->substrate_temp_min_crit_c = 4.0f;
    dst->substrate_temp_max_crit_c = 33.0f;
    dst->exhaust_concurrent_rh_cooperate_pct = 40.0f;
    dst->hum_cooperate_cap_pct = 25.0f;
    dst->cooperate_window_sec = 120u;
}

static void load_stage_s0(mf_control_profile_t *p) {
    p->rh_target_percent = 88.0f;
    p->co2_target_ppm = 2000.0f;
    p->air_temp_target_c = 22.0f;
    p->substrate_temp_target_c = 24.0f;
    p->light_hours_per_day = 0;
    p->rh_hysteresis_percent = 4.0f;
    p->co2_hysteresis_ppm = 500.0f;
    p->inlet_fan_min_duty_percent = 0.0f;
    p->inlet_fan_max_duty_percent = 20.0f;
    p->humidifier_max_duty_percent = 60.0f;
    p->grow_light_duty_percent = 0.0f;
    p->max_substrate_air_delta_c = 8.0f;
    p->allow_parallel_inlet_and_humidifier = false;
    p->co2_operational_purge_enabled = false;
    p->purge_duration_sec = 0;
}

static void load_stage_s1(mf_control_profile_t *p) {
    p->rh_target_percent = 92.0f;
    p->co2_target_ppm = 700.0f;
    p->air_temp_target_c = 12.0f;
    p->substrate_temp_target_c = 14.0f;
    p->light_hours_per_day = 12;
    p->rh_hysteresis_percent = 2.0f;
    p->co2_hysteresis_ppm = 100.0f;
    p->inlet_fan_min_duty_percent = 40.0f;
    p->inlet_fan_max_duty_percent = 100.0f;
    p->humidifier_max_duty_percent = 100.0f;
    p->grow_light_duty_percent = 100.0f;
    p->max_substrate_air_delta_c = 4.0f;
    p->allow_parallel_inlet_and_humidifier = true;
    p->co2_operational_purge_enabled = true;
    p->purge_duration_sec = 120u;
}

const mf_control_profile_t *mf_control_profile_current() {
    return &s_profile;
}

void mf_control_profile_load_defaults() {
    load_stage_s0(&s_profile);
    merge_safety(&s_profile);
}

void mf_control_profile_load_demo_stage(const char *stage_id) {
    if (stage_id && strcmp(stage_id, "S1") == 0) {
        load_stage_s1(&s_profile);
    } else {
        load_stage_s0(&s_profile);
    }
    merge_safety(&s_profile);
}

void mf_control_profile_log_current() {
    const mf_control_profile_t *p = &s_profile;
    mf_log_info("profile",
                "rh=%.1f co2=%.0f airT=%.1f subT=%.1f hum_max=%.0f fan=%.0f-%.0f "
                "co2_emerg=%.0f parallel=%d",
                p->rh_target_percent, p->co2_target_ppm, p->air_temp_target_c,
                p->substrate_temp_target_c, p->humidifier_max_duty_percent,
                p->inlet_fan_min_duty_percent, p->inlet_fan_max_duty_percent,
                p->co2_emergency_absolute_ppm, (int)p->allow_parallel_inlet_and_humidifier);
}
