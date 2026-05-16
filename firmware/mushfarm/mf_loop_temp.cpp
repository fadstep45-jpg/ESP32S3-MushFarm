#include "mf_loop_temp.h"
#include "mf_control_profile.h"
#include "mf_pid.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_config.h"

static mf_pid_t s_pid;
static bool s_inited = false;

void mf_loop_temp_reset() {
    mf_pid_reset(&s_pid);
    s_inited = false;
}

mf_loop_demand_t mf_loop_temp_tick(float dt_sec) {
    mf_loop_demand_t out = {0.0f, false};
    const mf_control_profile_t *p = mf_control_profile_current();

    if (mf_mlx90614_fault_disconnected() || !mf_mlx90614_ok(nullptr)) {
        return out;
    }
    out.enabled = true;

    if (!s_inited) {
        mf_pid_init(&s_pid, MF_PID_TEMP_KP, MF_PID_TEMP_KI, MF_PID_TEMP_KD, 0.0f,
                    p->inlet_fan_max_duty_percent);
        s_inited = true;
    }
    s_pid.out_max = p->inlet_fan_max_duty_percent;

    float sub = mf_mlx90614_object_c();
    float err = sub - p->substrate_temp_target_c;
    if (err <= 0.0f) {
        out.demand_pct = 0.0f;
        return out;
    }
    out.demand_pct = mf_pid_step(&s_pid, err, dt_sec);
    return out;
}
