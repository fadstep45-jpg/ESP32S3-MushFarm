#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t mf_sensor_scd41_init(void);
void mf_sensor_scd41_poll(void);
bool mf_sensor_scd41_ok(int64_t *stale_age_ms_out);
float mf_sensor_scd41_rh_percent(void);
float mf_sensor_scd41_temp_c(void);
uint16_t mf_sensor_scd41_co2_ppm(void);
