#include "mf_sensor_water.h"
#include "mf_board.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>

static bool s_present = true;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;
static int s_sim_level = -1;  // -1 disabled, 0 force absent, 1 force present

#if !MF_SENSORS_MOCK
static int s_raw_last = -1;
static uint8_t s_stable_count = 0;
#endif

void mf_water_set_simulated_present(int level) {
    if (level < -1 || level > 1) return;
    s_sim_level = level;
}

void mf_water_init() {
#if MF_SENSORS_MOCK
    s_present = true;
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("water", "MOCK mode: water level always present");
#else
    pinMode(MF_WATER_LEVEL_PIN, INPUT_PULLUP);
    s_raw_last = -1;
    s_stable_count = 0;
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
#endif
}

void mf_water_poll() {
    uint32_t now = mf_clock_millis();
#if MF_SENSORS_MOCK
    s_present = true;
    s_ok = true;
    s_last_ok_ms = now;
#else
    int raw = digitalRead(MF_WATER_LEVEL_PIN);
    bool level_low = (raw == LOW);
    if (s_raw_last < 0) {
        s_raw_last = raw;
        s_stable_count = 1;
    } else if (raw == s_raw_last) {
        if (s_stable_count < 255) {
            ++s_stable_count;
        }
    } else {
        s_raw_last = raw;
        s_stable_count = 1;
    }
    if (s_stable_count >= MF_WATER_DEBOUNCE_SAMPLES) {
        s_present = level_low;
        s_ok = true;
        s_last_ok_ms = now;
    }
#endif
}

bool mf_water_present() {
    if (s_sim_level == 0) return false;
    if (s_sim_level == 1) return true;
    return s_present;
}

bool mf_water_ok(int64_t *stale_age_ms_out) {
    int64_t age = s_ok ? (int64_t)(mf_clock_millis() - s_last_ok_ms) : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok && age <= (int64_t)MF_SENSOR_STALE_MS;
}
