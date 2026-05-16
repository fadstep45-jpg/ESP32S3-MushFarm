#pragma once

#include <stdbool.h>
#include <stdint.h>

/** Embedded runtime control profile (recipe-schema v1 fields, no YAML parser). */
typedef struct mf_control_profile_t {
    /* Setpoints */
    float rh_target_percent;
    float co2_target_ppm;
    float air_temp_target_c;
    float substrate_temp_target_c;
    uint8_t light_hours_per_day;

    /* Control bands / clamps */
    float rh_hysteresis_percent;
    float co2_hysteresis_ppm;
    float inlet_fan_min_duty_percent;
    float inlet_fan_max_duty_percent;
    float humidifier_max_duty_percent;
    float grow_light_duty_percent;
    float max_substrate_air_delta_c;

    /* Safety (safety_profile) */
    float rh_min_crit_percent;
    float rh_max_crit_percent;
    float co2_emergency_absolute_ppm;
    float co2_sensor_trust_upper_ppm;
    bool clamp_pid_effective_setpoint_to_trust_range;
    float air_temp_min_crit_c;
    float air_temp_max_crit_c;
    float substrate_temp_min_crit_c;
    float substrate_temp_max_crit_c;

    /* Arbitration */
    bool allow_parallel_inlet_and_humidifier;
    bool co2_operational_purge_enabled;
    uint32_t purge_duration_sec;
    float exhaust_concurrent_rh_cooperate_pct;
    float hum_cooperate_cap_pct;
    uint32_t cooperate_window_sec;
} mf_control_profile_t;

const mf_control_profile_t *mf_control_profile_current();
void mf_control_profile_load_demo_stage(const char *stage_id);
void mf_control_profile_load_defaults();
void mf_control_profile_log_current();
