#include "mf_sensor_scd41.h"
#include "mf_i2c_bus.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "scd41";

#define SCD41_I2C_ADDR 0x62

static int64_t s_last_ok_us;
static float s_rh, s_temp_c;
static uint16_t s_co2_ppm;
static bool s_ok;

static void fill_mock(void)
{
    s_rh = 88.0f;
    s_temp_c = 22.5f;
    s_co2_ppm = 1200;
    s_ok = true;
    s_last_ok_us = esp_timer_get_time();
}

esp_err_t mf_sensor_scd41_init(void)
{
#if CONFIG_MF_SENSORS_MOCK
    ESP_LOGW(TAG, "MOCK mode: synthetic RH/CO2/T");
    fill_mock();
    return ESP_OK;
#else
    /* Minimal bring-up: periodic measurement not started here (Sprint 2 extension). */
    s_ok = false;
    ESP_LOGW(TAG, "Hardware path stub: start periodic read in next iteration");
    return ESP_OK;
#endif
}

void mf_sensor_scd41_poll(void)
{
#if CONFIG_MF_SENSORS_MOCK
    fill_mock();
#else
    (void)SCD41_I2C_ADDR;
#endif
}

bool mf_sensor_scd41_ok(int64_t *stale_age_ms_out)
{
    int64_t now = esp_timer_get_time();
    int64_t age_ms = s_ok ? (now - s_last_ok_us) / 1000 : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age_ms;
    }
    return s_ok;
}

float mf_sensor_scd41_rh_percent(void) { return s_rh; }
float mf_sensor_scd41_temp_c(void) { return s_temp_c; }
uint16_t mf_sensor_scd41_co2_ppm(void) { return s_co2_ppm; }
