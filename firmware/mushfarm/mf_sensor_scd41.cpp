#include "mf_sensor_scd41.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <stdint.h>

#ifndef NAN
#define NAN (0.0f / 0.0f)
#endif

static float s_co2 = 600.0f;
static float s_rh = 60.0f;
static float s_t = 22.0f;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;
static bool s_fault_disconnected = false;
static uint8_t s_consecutive_bad = 0;
static uint8_t s_recovery_streak = 0;

static bool float_ok(float x) {
    return !isnan(x) && !isinf(x);
}

static bool validate_sample(float co2, float rh, float t) {
    if (!float_ok(co2) || !float_ok(rh) || !float_ok(t)) return false;
    if (co2 < 0.0f || co2 > 10000.0f) return false;
    if (rh < 0.0f || rh > 100.0f) return false;
    if (t < -40.0f || t > 85.0f) return false;
    return true;
}

static void mark_good_sample(uint32_t now_ms) {
    s_consecutive_bad = 0;
    if (s_fault_disconnected) {
        if (++s_recovery_streak >= MF_SENSOR_RECOVERY_SAMPLES) {
            s_fault_disconnected = false;
            s_recovery_streak = 0;
            mf_log_info("scd41", "fault cleared after recovery samples");
        }
    } else {
        s_recovery_streak = 0;
    }
    s_ok = true;
    s_last_ok_ms = now_ms;
}

static void mark_bad_cycle() {
    s_recovery_streak = 0;
    if (++s_consecutive_bad >= MF_SENSOR_READ_RETRIES) {
        s_fault_disconnected = true;
        s_ok = false;
    }
}

#if !MF_SENSORS_MOCK
static bool scd41_i2c_probe() {
    Wire.beginTransmission(0x62);
    return Wire.endTransmission() == 0;
}
#endif

void mf_scd41_init() {
    s_consecutive_bad = 0;
    s_recovery_streak = 0;
    s_fault_disconnected = false;
#if MF_SENSORS_MOCK
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("scd41", "MOCK mode: synthetic CO2/RH/T");
#else
    bool any = false;
    for (uint32_t i = 0; i < MF_SENSOR_READ_RETRIES; ++i) {
        if (scd41_i2c_probe()) {
            any = true;
            break;
        }
        delayMicroseconds(200);
    }
    s_ok = any;
    s_last_ok_ms = any ? mf_clock_millis() : 0;
    if (!any) {
        s_fault_disconnected = true;
        mf_log_warn("scd41", "probe failed after retries");
    } else {
        mf_log_info("scd41", "I2C probe OK (driver stub)");
    }
#endif
}

void mf_scd41_poll() {
    uint32_t now = mf_clock_millis();
#if MF_SENSORS_MOCK
    uint32_t t = now / 1000u;
    float co2 = 600.0f + (float)(t % 300);
    float rh  = 55.0f + (float)((t * 7u) % 30);
    float tc  = 22.0f + ((float)((t * 3u) % 100)) * 0.05f;
    bool one_shot_bad = ((t % 37u) == 0u); // rare invalid sample to exercise NaN path
    if (one_shot_bad) {
        co2 = NAN;
    }
    bool good = false;
    for (uint32_t attempt = 0; attempt < MF_SENSOR_READ_RETRIES; ++attempt) {
        float c = co2, r = rh, te = tc;
        if (attempt > 0 && one_shot_bad) {
            c = 600.0f + (float)(t % 300);
        }
        if (validate_sample(c, r, te)) {
            s_co2 = c;
            s_rh = r;
            s_t = te;
            good = true;
            break;
        }
    }
    if (good) {
        mark_good_sample(now);
    } else {
        mark_bad_cycle();
    }
#else
    bool good = false;
    for (uint32_t attempt = 0; attempt < MF_SENSOR_READ_RETRIES; ++attempt) {
        if (!scd41_i2c_probe()) {
            continue;
        }
        // Real Sensirion read not wired yet — treat probe success as a heartbeat sample.
        if (validate_sample(s_co2, s_rh, s_t)) {
            good = true;
            break;
        }
    }
    if (good) {
        mark_good_sample(now);
    } else {
        mark_bad_cycle();
    }
#endif
    if (s_ok && (int64_t)(now - s_last_ok_ms) > (int64_t)MF_SENSOR_STALE_MS) {
        s_ok = false;
        mf_log_warn("scd41", "stale sample age > %lums", (unsigned long)MF_SENSOR_STALE_MS);
    }
}

float mf_scd41_co2_ppm()    { return s_co2; }
float mf_scd41_rh_percent() { return s_rh; }
float mf_scd41_temp_c()     { return s_t; }

bool mf_scd41_ok(int64_t *stale_age_ms_out) {
    int64_t age = (s_ok && s_last_ok_ms > 0)
                      ? (int64_t)(mf_clock_millis() - s_last_ok_ms)
                      : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok && !s_fault_disconnected && age <= (int64_t)MF_SENSOR_STALE_MS;
}

bool mf_scd41_fault_disconnected() {
    return s_fault_disconnected;
}
