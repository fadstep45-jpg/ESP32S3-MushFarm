#pragma once

#include <stdint.h>

void mf_scd41_init();
void mf_scd41_poll();

float mf_scd41_co2_ppm();
float mf_scd41_rh_percent();
float mf_scd41_temp_c();

bool mf_scd41_ok(int64_t *stale_age_ms_out);
