#include "mf_climate_trace.h"
#include "mf_batch_logger.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_config.h"
#include "mf_clock.h"
#include <stdio.h>

static mf_arb_reason_t s_last_reason = MF_ARB_NORMAL;
static uint32_t s_tick_count = 0;
static float s_last_fan = 0.0f;
static float s_last_hum = 0.0f;

void mf_climate_trace_reset() {
    s_last_reason = MF_ARB_NORMAL;
    s_tick_count = 0;
    s_last_fan = 0.0f;
    s_last_hum = 0.0f;
}

const char *mf_climate_trace_last_reason_str() {
    return mf_arb_reason_str(s_last_reason);
}

float mf_climate_trace_last_fan_pct() { return s_last_fan; }
float mf_climate_trace_last_hum_pct() { return s_last_hum; }

void mf_climate_trace_on_tick(const mf_arb_result_t *arb, int64_t max_stale_age_ms) {
    if (!arb) return;
    s_tick_count++;
    s_last_fan = arb->fan_pct;
    s_last_hum = arb->hum_pct;

    bool full = (arb->reason != s_last_reason) || arb->limits.co2_crit ||
                arb->limits.rh_max_crit || arb->limits.condensate_guard ||
                (s_tick_count % MF_CLIMATE_TRACE_FULL_EVERY_N) == 0u;

    char line[256];
    if (full) {
        snprintf(line, sizeof(line),
                 "arb_full ts=%lu reason=%s fan=%.0f hum=%.0f rh_d=%.0f co2_d=%.0f "
                 "temp_d=%.0f rh=%.1f co2=%.0f stale_ms=%lld crit=%d",
                 (unsigned long)mf_clock_millis(), mf_arb_reason_str(arb->reason),
                 arb->fan_pct, arb->hum_pct, arb->rh_demand, arb->co2_demand,
                 arb->temp_demand, mf_scd41_rh_percent(), mf_scd41_co2_ppm(),
                 (long long)max_stale_age_ms, (int)arb->limits.co2_crit);
    } else {
        snprintf(line, sizeof(line), "arb ts=%lu reason=%s fan=%.0f hum=%.0f stale_ms=%lld",
                 (unsigned long)mf_clock_millis(), mf_arb_reason_str(arb->reason),
                 arb->fan_pct, arb->hum_pct, (long long)max_stale_age_ms);
    }
    mf_batch_logger_push(line);
    s_last_reason = arb->reason;
}
