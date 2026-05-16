#include "mf_loop_co2.h"
#include "mf_control_profile.h"
#include "mf_pid.h"
#include "mf_sensor_scd41.h"
#include "mf_config.h"
#include "mf_clock.h"

static mf_pid_t s_pid;
static bool s_inited = false;
static bool s_purge_active = false;
static uint32_t s_purge_until_ms = 0;

void mf_loop_co2_reset() {
    mf_pid_reset(&s_pid);
    s_inited = false;
    s_purge_active = false;
    s_purge_until_ms = 0;
}

mf_loop_demand_t mf_loop_co2_tick(float dt_sec) {
    mf_loop_demand_t out = {0.0f, false};
    const mf_control_profile_t *p = mf_control_profile_current();

    if (mf_scd41_fault_disconnected() || !mf_scd41_ok(nullptr)) {
        return out;
    }
    out.enabled = true;

    float co2 = mf_scd41_co2_ppm();
    float target = p->co2_target_ppm;
    if (p->clamp_pid_effective_setpoint_to_trust_range &&
        target > p->co2_sensor_trust_upper_ppm) {
        target = p->co2_sensor_trust_upper_ppm;
    }

    uint32_t now = mf_clock_millis();
    if (s_purge_active) {
        if (now < s_purge_until_ms) {
            out.demand_pct = p->inlet_fan_max_duty_percent;
            return out;
        }
        s_purge_active = false;
    }

    if (p->co2_operational_purge_enabled &&
        co2 > target + p->co2_hysteresis_ppm) {
        s_purge_active = true;
        s_purge_until_ms = now + p->purge_duration_sec * 1000u;
        out.demand_pct = p->inlet_fan_max_duty_percent;
        return out;
    }

    if (!s_inited) {
        mf_pid_init(&s_pid, MF_PID_CO2_KP, MF_PID_CO2_KI, MF_PID_CO2_KD,
                    p->inlet_fan_min_duty_percent, p->inlet_fan_max_duty_percent);
        s_inited = true;
    }
    s_pid.out_min = p->inlet_fan_min_duty_percent;
    s_pid.out_max = p->inlet_fan_max_duty_percent;

    float err = co2 - target;
    if (err < p->co2_hysteresis_ppm && err > -p->co2_hysteresis_ppm) {
        err = 0.0f;
    }
    if (err <= 0.0f) {
        out.demand_pct = p->inlet_fan_min_duty_percent;
        return out;
    }

    out.demand_pct = mf_pid_step(&s_pid, err, dt_sec);
    if (out.demand_pct < p->inlet_fan_min_duty_percent) {
        out.demand_pct = p->inlet_fan_min_duty_percent;
    }
    return out;
}
