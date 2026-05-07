#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t mf_sensor_mlx90614_init(void);
void mf_sensor_mlx90614_poll(void);
bool mf_sensor_mlx90614_ok(int64_t *stale_age_ms_out);
float mf_sensor_mlx90614_object_c(void);
