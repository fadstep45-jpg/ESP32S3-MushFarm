#include "mf_water_policy.h"
#include "mf_sensor_water.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <math.h>

static mf_water_policy_state_t s_state = MF_WATER_POLICY_NORMAL;
static uint32_t s_reserve_start_ms = 0;
static uint32_t s_phase_start_ms = 0;
static float s_rh_baseline = 0.0f;
static uint8_t s_flat_windows = 0;

static bool rh_valid(float rh) { return !isnan(rh) && !isinf(rh) && rh >= 0.0f && rh <= 100.0f; }

const char *mf_water_policy_state_str(mf_water_policy_state_t s) {
    switch (s) {
    case MF_WATER_POLICY_NORMAL:    return "NORMAL";
    case MF_WATER_POLICY_RESERVE:   return "RESERVE";
    case MF_WATER_POLICY_PULSE_ON:  return "PULSE_ON";
    case MF_WATER_POLICY_PULSE_OFF: return "PULSE_OFF";
    case MF_WATER_POLICY_LOCKED:    return "LOCKED";
    default: return "?";
    }
}

mf_water_policy_state_t mf_water_policy_state() { return s_state; }
bool mf_water_policy_locked() { return s_state == MF_WATER_POLICY_LOCKED; }

void mf_water_policy_init() {
    s_state = MF_WATER_POLICY_NORMAL;
    s_reserve_start_ms = 0;
    s_phase_start_ms = 0;
    s_rh_baseline = 0.0f;
    s_flat_windows = 0;
}

void mf_water_policy_reset() {
    // Same as init, but log because we are clearing a non-trivial state
    // (e.g. operator stop or stage transition after a LOCKED scare).
    if (s_state != MF_WATER_POLICY_NORMAL) {
        mf_log_info("water", "policy reset from %s -> NORMAL", mf_water_policy_state_str(s_state));
    }
    mf_water_policy_init();
}

static void enter(mf_water_policy_state_t to, const char *reason) {
    if (to == s_state) return;
    mf_log_warn("water", "policy %s -> %s (%s)",
                mf_water_policy_state_str(s_state),
                mf_water_policy_state_str(to),
                reason ? reason : "");
    s_state = to;
}

float mf_water_policy_apply(float requested_hum_pct, float rh_now, float dt_sec) {
    (void)dt_sec;  // we use absolute millis for window edges, not dt accumulation

    bool present = mf_water_present();
    uint32_t now = mf_clock_millis();

    switch (s_state) {
    case MF_WATER_POLICY_NORMAL:
        if (!present) {
            s_reserve_start_ms = now;
            enter(MF_WATER_POLICY_RESERVE, "water LOW: reserve window starts");
        }
        return requested_hum_pct;

    case MF_WATER_POLICY_RESERVE:
        if (present) {
            enter(MF_WATER_POLICY_NORMAL, "water restored during reserve");
            return requested_hum_pct;
        }
        if ((now - s_reserve_start_ms) >= (uint32_t)MF_WATER_RESERVE_TIMER_S * 1000u) {
            s_phase_start_ms = now;
            s_rh_baseline = rh_valid(rh_now) ? rh_now : 0.0f;
            s_flat_windows = 0;
            enter(MF_WATER_POLICY_PULSE_ON, "reserve elapsed: start pulse-safe ON");
        }
        return requested_hum_pct;

    case MF_WATER_POLICY_PULSE_ON: {
        if (present) {
            enter(MF_WATER_POLICY_NORMAL, "water restored during pulse-on");
            return requested_hum_pct;
        }
        uint32_t in_phase = now - s_phase_start_ms;
        if (in_phase >= (uint32_t)MF_WATER_PULSE_ON_S * 1000u) {
            // End of ON window — evaluate RH trend.
            if (rh_valid(rh_now) && s_rh_baseline > 0.0f) {
                float delta = rh_now - s_rh_baseline;
                if (delta < MF_WATER_PULSE_RH_RISE_DELTA) {
                    s_flat_windows++;
                } else {
                    s_flat_windows = 0;
                }
                mf_log_info("water", "pulse window end: rh %.1f->%.1f delta=%.2f flat=%u",
                            (double)s_rh_baseline, (double)rh_now, (double)delta,
                            (unsigned)s_flat_windows);
            } else {
                // No RH telemetry — be conservative and treat as flat.
                s_flat_windows++;
                mf_log_warn("water", "pulse window end: no RH; treat as flat (flat=%u)",
                            (unsigned)s_flat_windows);
            }
            if (s_flat_windows >= MF_WATER_PULSE_FLAT_LIMIT) {
                enter(MF_WATER_POLICY_LOCKED,
                      "RH flat across pulse windows: lock humidifier (CRITICAL)");
                return 0.0f;
            }
            s_phase_start_ms = now;
            enter(MF_WATER_POLICY_PULSE_OFF, "end of pulse-on window");
            return 0.0f;
        }
        return requested_hum_pct;
    }

    case MF_WATER_POLICY_PULSE_OFF: {
        if (present) {
            enter(MF_WATER_POLICY_NORMAL, "water restored during pulse-off");
            return requested_hum_pct;
        }
        uint32_t in_phase = now - s_phase_start_ms;
        if (in_phase >= (uint32_t)MF_WATER_PULSE_OFF_S * 1000u) {
            s_phase_start_ms = now;
            s_rh_baseline = rh_valid(rh_now) ? rh_now : 0.0f;
            enter(MF_WATER_POLICY_PULSE_ON, "end of pulse-off window");
            return requested_hum_pct;
        }
        return 0.0f;
    }

    case MF_WATER_POLICY_LOCKED:
        // Sticky. Only mf_water_policy_reset() (cycle stop / stage advance)
        // clears it. fault-model.md: "lock humidifier and raise CRITICAL".
        return 0.0f;
    }
    return requested_hum_pct;
}
