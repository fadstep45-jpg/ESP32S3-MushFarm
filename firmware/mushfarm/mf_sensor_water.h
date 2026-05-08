#pragma once

#include <stdint.h>

void mf_water_init();
void mf_water_poll();

bool mf_water_present();
bool mf_water_ok(int64_t *stale_age_ms_out);
