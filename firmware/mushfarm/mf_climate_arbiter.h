#pragma once

#include "mf_loop_common.h"
#include "mf_control_limits.h"

typedef enum mf_arb_reason_t {
    MF_ARB_NORMAL = 0,
    MF_ARB_CO2_CRIT_PURGE,
    MF_ARB_RH_MAX_CRIT,
    MF_ARB_COOP_HUM_CAP,
    MF_ARB_CONDENSATE_GUARD,
    MF_ARB_SEQ_BIAS_FAN,
    MF_ARB_SEQ_BIAS_HUM,
    MF_ARB_SAFE_TIMER,
    MF_ARB_RH_SLEW_CAP,
} mf_arb_reason_t;

typedef struct mf_arb_result_t {
    float fan_pct;
    float hum_pct;
    float rh_demand;
    float co2_demand;
    float temp_demand;
    mf_arb_reason_t reason;
    mf_control_limits_state_t limits;
} mf_arb_result_t;

const char *mf_arb_reason_str(mf_arb_reason_t reason);
void mf_climate_arbiter_reset();
void mf_climate_arbiter_run(float dt_sec, bool rh_co2_both_missing, mf_arb_result_t *out);
