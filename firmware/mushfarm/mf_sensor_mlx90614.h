#pragma once

#include <stdint.h>

void mf_mlx90614_init();
void mf_mlx90614_poll();

float mf_mlx90614_object_c();
bool mf_mlx90614_ok(int64_t *stale_age_ms_out);
