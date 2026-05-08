#include "mf_sensor_mlx90614.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>

static float s_obj_c = 24.0f;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;

void mf_mlx90614_init() {
#if MF_SENSORS_MOCK
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("mlx90614", "MOCK mode: synthetic object temp");
#else
    // TODO: real MLX90614 init via Adafruit_MLX90614 library.
    s_ok = false;
    mf_log_warn("mlx90614", "real driver not wired yet");
#endif
}

void mf_mlx90614_poll() {
#if MF_SENSORS_MOCK
    uint32_t t = mf_clock_millis() / 1000u;
    s_obj_c = 24.0f + ((float)((t * 2u) % 50)) * 0.05f;
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
#else
    // TODO: read real MLX90614 object temperature.
#endif
}

float mf_mlx90614_object_c() { return s_obj_c; }

bool mf_mlx90614_ok(int64_t *stale_age_ms_out) {
    int64_t age = s_ok ? (int64_t)(mf_clock_millis() - s_last_ok_ms) : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok;
}
