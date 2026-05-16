#include "mf_climate_arbiter.h"
#include "mf_control_profile.h"
#include "mf_loop_rh.h"
#include "mf_loop_co2.h"
#include "mf_loop_temp.h"
#include "mf_sensor_water.h"
#include "mf_sensor_scd41.h"
#include "mf_water_policy.h"
#include "mf_config.h"

static uint32_t s_seq_tick = 0;
static bool s_seq_fan_priority = true;

static float clamp_pct(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 100.0f) return 100.0f;
    return v;
}

const char *mf_arb_reason_str(mf_arb_reason_t reason) {
    switch (reason) {
    case MF_ARB_CO2_CRIT_PURGE: return "ARB_CO2_CRIT_PURGE";
    case MF_ARB_RH_MAX_CRIT: return "ARB_RH_MAX_CRIT";
    case MF_ARB_COOP_HUM_CAP: return "ARB_COOP_HUM_CAP";
    case MF_ARB_CONDENSATE_GUARD: return "ARB_CONDENSATE_GUARD";
    case MF_ARB_SEQ_BIAS_FAN: return "ARB_SEQ_BIAS_FAN";
    case MF_ARB_SEQ_BIAS_HUM: return "ARB_SEQ_BIAS_HUM";
    case MF_ARB_SAFE_TIMER: return "ARB_SAFE_TIMER";
    case MF_ARB_RH_SLEW_CAP: return "ARB_RH_SLEW_CAP";
    case MF_ARB_NORMAL:
    default: return "ARB_NORMAL";
    }
}

void mf_climate_arbiter_reset() {
    s_seq_tick = 0;
    s_seq_fan_priority = true;
    mf_loop_rh_reset();
    mf_loop_co2_reset();
    mf_loop_temp_reset();
    mf_water_policy_reset();
}

void mf_climate_arbiter_run(float dt_sec, bool rh_co2_both_missing, mf_arb_result_t *out) {
    if (!out) return;
    const mf_control_profile_t *p = mf_control_profile_current();

    mf_loop_demand_t rh = mf_loop_rh_tick(dt_sec);
    mf_loop_demand_t co2 = mf_loop_co2_tick(dt_sec);
    mf_loop_demand_t temp = mf_loop_temp_tick(dt_sec);

    mf_control_limits_evaluate(&out->limits);
    out->rh_demand = rh.demand_pct;
    out->co2_demand = co2.demand_pct;
    out->temp_demand = temp.demand_pct;
    out->reason = MF_ARB_NORMAL;

    if (rh_co2_both_missing) {
        out->fan_pct = p->inlet_fan_min_duty_percent;
        out->hum_pct = 0.0f;
        out->reason = MF_ARB_SAFE_TIMER;
        return;
    }

    float fan = 0.0f;
    float hum = 0.0f;

    if (co2.enabled) {
        fan = co2.demand_pct;
    } else {
        fan = p->inlet_fan_min_duty_percent;
    }
    if (temp.enabled && temp.demand_pct > fan) {
        fan = temp.demand_pct;
    }
    if (rh.enabled) {
        hum = rh.demand_pct;
    }

    if (out->limits.co2_crit) {
        fan = 100.0f;
        out->reason = MF_ARB_CO2_CRIT_PURGE;
    }
    if (out->limits.safety_exhaust_pct > fan) {
        fan = out->limits.safety_exhaust_pct;
    }
    if (out->limits.rh_max_crit) {
        hum = 0.0f;
        out->reason = MF_ARB_RH_MAX_CRIT;
    }
    if (out->limits.safety_hum_pct < hum) {
        hum = out->limits.safety_hum_pct;
    }

    if (out->limits.condensate_guard) {
        if (out->limits.condensate_exhaust_floor_pct > fan) {
            fan = out->limits.condensate_exhaust_floor_pct;
        }
        if (hum > out->limits.condensate_hum_cap_pct) {
            hum = out->limits.condensate_hum_cap_pct;
        }
        out->reason = MF_ARB_CONDENSATE_GUARD;
    }

    if (!p->allow_parallel_inlet_and_humidifier && rh.enabled && co2.enabled &&
        rh.demand_pct > 5.0f && co2.demand_pct > p->exhaust_concurrent_rh_cooperate_pct) {
        s_seq_tick++;
        if ((s_seq_tick % MF_ARB_SEQ_ALTERNATE_TICKS) == 0u) {
            s_seq_fan_priority = !s_seq_fan_priority;
        }
        if (s_seq_fan_priority) {
            hum = hum * 0.25f;
            out->reason = MF_ARB_SEQ_BIAS_FAN;
        } else {
            fan = fan * 0.5f;
            if (fan < p->inlet_fan_min_duty_percent) {
                fan = p->inlet_fan_min_duty_percent;
            }
            out->reason = MF_ARB_SEQ_BIAS_HUM;
        }
    }

    if (p->allow_parallel_inlet_and_humidifier && rh.enabled && co2.enabled &&
        rh.demand_pct > 10.0f &&
        co2.demand_pct > p->exhaust_concurrent_rh_cooperate_pct &&
        !out->limits.co2_crit) {
        if (hum > p->hum_cooperate_cap_pct) {
            hum = p->hum_cooperate_cap_pct;
            out->reason = MF_ARB_COOP_HUM_CAP;
        }
    }

    // Water policy replaces the old `if (!present) hum = 0;` one-liner.
    // It implements reserve-timer → pulse-safe (15s ON / 180s OFF) → lock
    // escalation per fault-model.md §"Water Sensor".
    float rh_now = mf_scd41_ok(nullptr) ? mf_scd41_rh_percent() : 0.0f;
    hum = mf_water_policy_apply(hum, rh_now, dt_sec);

    out->fan_pct = clamp_pct(fan);
    out->hum_pct = clamp_pct(hum);
}
