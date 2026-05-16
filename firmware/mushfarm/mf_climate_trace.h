#pragma once

#include <stdint.h>

#include "mf_climate_arbiter.h"

void mf_climate_trace_reset();
void mf_climate_trace_on_tick(const mf_arb_result_t *arb, int64_t max_stale_age_ms);

const char *mf_climate_trace_last_reason_str();
float mf_climate_trace_last_fan_pct();
float mf_climate_trace_last_hum_pct();
