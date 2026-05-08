#include "mf_sensor_water.h"
#include "mf_board.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>

static bool s_present = true;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;

void mf_water_init() {
#if MF_SENSORS_MOCK
    s_present = true;
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("water", "MOCK mode: water level always present");
#else
    pinMode(MF_WATER_LEVEL_PIN, INPUT_PULLUP);
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
#endif
}

void mf_water_poll() {
#if MF_SENSORS_MOCK
    s_present = true;
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
#else
    int v = digitalRead(MF_WATER_LEVEL_PIN);
    s_present = (v == LOW);
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
#endif
}

bool mf_water_present() { return s_present; }

bool mf_water_ok(int64_t *stale_age_ms_out) {
    int64_t age = s_ok ? (int64_t)(mf_clock_millis() - s_last_ok_ms) : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok;
}
