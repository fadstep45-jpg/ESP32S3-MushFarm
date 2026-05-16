#include "mf_loop_rh.h"
#include "mf_control_profile.h"
#include "mf_pid.h"
#include "mf_sensor_scd41.h"
#include "mf_config.h"

static mf_pid_t s_pid;
static float s_last_out = 0.0f;
static bool s_inited = false;

void mf_loop_rh_reset() {
    mf_pid_reset(&s_pid);
    s_last_out = 0.0f;
    s_inited = false;
}

mf_loop_demand_t mf_loop_rh_tick(float dt_sec) {
    mf_loop_demand_t out = {0.0f, false};
    const mf_control_profile_t *p = mf_control_profile_current();

    if (mf_scd41_fault_disconnected() || !mf_scd41_ok(nullptr)) {
        return out;
    }
    out.enabled = true;

    if (!s_inited) {
        mf_pid_init(&s_pid, MF_PID_RH_KP, MF_PID_RH_KI, MF_PID_RH_KD, 0.0f,
                    p->humidifier_max_duty_percent);
        s_inited = true;
    }
    s_pid.out_max = p->humidifier_max_duty_percent;

    float rh = mf_scd41_rh_percent();
    float err = p->rh_target_percent - rh;
    if (err > -p->rh_hysteresis_percent && err < p->rh_hysteresis_percent) {
        err = 0.0f;
    }
    if (err <= 0.0f) {
        out.demand_pct = 0.0f;
        s_last_out = 0.0f;
        mf_pid_reset(&s_pid);
        return out;
    }

    float raw = mf_pid_step(&s_pid, err, dt_sec);
    float slew_per_sec = MF_RH_SLEW_MAX_PER_MIN / 60.0f;
    if (raw > s_last_out) {
        raw = mf_pid_slew(s_last_out, raw, slew_per_sec, dt_sec);
    }
    s_last_out = raw;
    out.demand_pct = raw;
    return out;
}
