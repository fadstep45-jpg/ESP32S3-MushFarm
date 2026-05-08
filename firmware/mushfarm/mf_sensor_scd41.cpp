#include "mf_sensor_scd41.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>

static float s_co2 = 600.0f;
static float s_rh = 60.0f;
static float s_t = 22.0f;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;

void mf_scd41_init() {
#if MF_SENSORS_MOCK
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("scd41", "MOCK mode: synthetic CO2/RH/T");
#else
    // TODO: real SCD41 init via SensirionI2cScd4x library, periodic measurement.
    s_ok = false;
    mf_log_warn("scd41", "real driver not wired yet");
#endif
}

void mf_scd41_poll() {
#if MF_SENSORS_MOCK
    uint32_t t = mf_clock_millis() / 1000u;
    s_co2 = 600.0f + (float)(t % 300);
    s_rh  = 55.0f + (float)((t * 7u) % 30);
    s_t   = 22.0f + ((float)((t * 3u) % 100)) * 0.05f;
    s_ok  = true;
    s_last_ok_ms = mf_clock_millis();
#else
    // TODO: read real SCD41 sample, update s_ok / s_last_ok_ms on success.
#endif
}

float mf_scd41_co2_ppm()    { return s_co2; }
float mf_scd41_rh_percent() { return s_rh; }
float mf_scd41_temp_c()     { return s_t; }

bool mf_scd41_ok(int64_t *stale_age_ms_out) {
    int64_t age = s_ok ? (int64_t)(mf_clock_millis() - s_last_ok_ms) : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok;
}
