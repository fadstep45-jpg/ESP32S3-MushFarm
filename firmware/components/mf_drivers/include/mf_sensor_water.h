#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t mf_sensor_water_init(void);
void mf_sensor_water_poll(void);
bool mf_sensor_water_ok(int64_t *stale_age_ms_out);
bool mf_sensor_water_present(void);
