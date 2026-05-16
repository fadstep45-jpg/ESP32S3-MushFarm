#include "mf_climate.h"
#include "mf_actuators.h"
#include "mf_fsm.h"
#include "mf_control_profile.h"
#include "mf_climate_arbiter.h"
#include "mf_climate_trace.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_log.h"
#include "mf_clock.h"
#include "mf_config.h"

static uint32_t s_last_climate_ms = 0;

static void apply_light_from_profile() {
    const mf_control_profile_t *p = mf_control_profile_current();
    float duty = 0.0f;
    if (p->light_hours_per_day > 0) {
        duty = p->grow_light_duty_percent;
    }
    mf_actuators_set_percent(MF_ACT_LIGHT, duty);
}

static int64_t max_stale_age_ms() {
    int64_t a = 0, b = 0, c = 0;
    mf_scd41_ok(&a);
    mf_mlx90614_ok(&b);
    (void)c;
    if (a > b) return a;
    return b;
}

static bool rh_co2_both_missing() {
    return mf_scd41_fault_disconnected() || !mf_scd41_ok(nullptr);
}

void mf_climate_tick() {
    mf_runtime_state_t st = mf_fsm_state();

    if (st == MF_STATE_EMERGENCY_STOP) {
        mf_actuators_all_off();
        return;
    }

    if (st == MF_STATE_PAUSED_SAFE) {
        mf_actuators_set_percent(MF_ACT_HUMIDIFIER, 0.0f);
        mf_actuators_set_percent(MF_ACT_FAN, 15.0f);
        apply_light_from_profile();
        return;
    }

    if (st != MF_STATE_ACTIVE_RUN && st != MF_STATE_DEGRADED_RUN) {
        mf_actuators_set_percent(MF_ACT_FAN, 0);
        mf_actuators_set_percent(MF_ACT_HUMIDIFIER, 0);
        apply_light_from_profile();
        return;
    }

    uint32_t now = mf_clock_millis();
    float dt = (s_last_climate_ms == 0u)
                   ? ((float)MF_TICK_CLIMATE_MS / 1000.0f)
                   : ((float)(now - s_last_climate_ms) / 1000.0f);
    s_last_climate_ms = now;
    if (dt <= 0.0f) {
        dt = (float)MF_TICK_CLIMATE_MS / 1000.0f;
    }

    bool both_missing = rh_co2_both_missing();

    mf_arb_result_t arb;
    mf_climate_arbiter_run(dt, both_missing, &arb);

    mf_actuators_set_percent(MF_ACT_FAN, arb.fan_pct);
    mf_actuators_set_percent(MF_ACT_HUMIDIFIER, arb.hum_pct);
    apply_light_from_profile();

    mf_climate_trace_on_tick(&arb, max_stale_age_ms());

    if (both_missing) {
        static uint32_t s_last_warn_ms = 0;
        if (now - s_last_warn_ms > 30000u) {
            s_last_warn_ms = now;
            mf_log_warn("climate", "RH+CO2 unavailable — safe timer fan min");
        }
    }
}
