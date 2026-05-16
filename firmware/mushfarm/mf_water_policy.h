#pragma once

#include <stdbool.h>
#include <stdint.h>

// Water-tank false-positive policy (docs/architecture/fault-model.md
// "Water Sensor (Business Priority)").
//
//   NORMAL ──(water LOW)──▶ RESERVE
//          ◀──(water back)──
//   RESERVE ──(reserve timer elapsed)──▶ PULSE_ON
//   PULSE_ON ─(end of ON window)─▶ PULSE_OFF | LOCKED   (depending on RH trend)
//   PULSE_OFF ─(end of OFF window)─▶ PULSE_ON
//   PULSE_*   ──(water back)──▶ NORMAL
//   LOCKED is sticky: only mf_water_policy_reset() clears it.
//
// Inputs: water_present, requested hum %, current RH %, dt seconds.
// Output: the hum value that the climate arbiter should actually apply.
//
// The intent is to give the system a chance to keep humidity rising on a
// momentary or trickling water shortage instead of slamming the humidifier
// off on the first LOW reading. If pulsing fails to lift RH for several
// cycles in a row, lock the humidifier and let mf_fault_supervisor publish
// MF_NONFATAL_WATER → DEGRADED_RUN.
//
// Operator WARN vs FSM warn flags (fault-model.md "first LOW → WARN"):
//   - RESERVE / PULSE_* : policy logs `mf_log_warn` on state entry only;
//     MF_WARN_WATER_FAIL is NOT set yet — cycle stays in ACTIVE_RUN.
//   - LOCKED            : mf_fault_supervisor sets MF_WARN_WATER_FAIL and
//     dispatches evFaultNonFatal(WATER) → DEGRADED_RUN.

typedef enum {
    MF_WATER_POLICY_NORMAL = 0,
    MF_WATER_POLICY_RESERVE,
    MF_WATER_POLICY_PULSE_ON,
    MF_WATER_POLICY_PULSE_OFF,
    MF_WATER_POLICY_LOCKED,
} mf_water_policy_state_t;

void mf_water_policy_init();
void mf_water_policy_reset();

// Returns the hum % that should actually be commanded after applying the
// dry-tank policy. `rh_now` may be NaN/0 if RH is unavailable — in that
// case the trend check is skipped and the OFF window decision is made on
// time alone (conservative: assumes flat trend).
float mf_water_policy_apply(float requested_hum_pct, float rh_now, float dt_sec);

mf_water_policy_state_t mf_water_policy_state();
const char *mf_water_policy_state_str(mf_water_policy_state_t s);
bool mf_water_policy_locked();
