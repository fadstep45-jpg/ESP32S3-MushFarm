#include "mf_sensor_mlx90614.h"
#include "mf_mock_climate.h"
#include "mf_sensor_scd41.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <stdint.h>

static float s_obj_c = 24.0f;
static bool s_ok = false;
static uint32_t s_last_ok_ms = 0;
static bool s_fault_disconnected = false;
static uint8_t s_consecutive_bad = 0;
static uint8_t s_recovery_streak = 0;

static bool validate_obj(float c) {
    if (isnan(c) || isinf(c)) return false;
    if (c < -40.0f || c > 125.0f) return false;
    return true;
}

static void mark_good(uint32_t now_ms) {
    s_consecutive_bad = 0;
    if (s_fault_disconnected) {
        if (++s_recovery_streak >= MF_SENSOR_RECOVERY_SAMPLES) {
            s_fault_disconnected = false;
            s_recovery_streak = 0;
            mf_log_info("mlx90614", "fault cleared after recovery samples");
        }
    } else {
        s_recovery_streak = 0;
    }
    s_ok = true;
    s_last_ok_ms = now_ms;
}

static void mark_bad() {
    s_recovery_streak = 0;
    if (++s_consecutive_bad >= MF_SENSOR_READ_RETRIES) {
        s_fault_disconnected = true;
        s_ok = false;
    }
}

#if !MF_SENSORS_MOCK
static bool mlx90614_i2c_probe() {
    Wire.beginTransmission(0x5A);
    return Wire.endTransmission() == 0;
}
#endif

void mf_mlx90614_init() {
    s_consecutive_bad = 0;
    s_recovery_streak = 0;
    s_fault_disconnected = false;
#if MF_SENSORS_MOCK
    s_ok = true;
    s_last_ok_ms = mf_clock_millis();
    mf_log_warn("mlx90614", "MOCK mode: synthetic object temp");
#else
    bool any = false;
    for (uint32_t i = 0; i < MF_SENSOR_READ_RETRIES; ++i) {
        if (mlx90614_i2c_probe()) {
            any = true;
            break;
        }
        delayMicroseconds(200);
    }
    s_ok = any;
    s_last_ok_ms = any ? mf_clock_millis() : 0;
    if (!any) {
        s_fault_disconnected = true;
        mf_log_warn("mlx90614", "probe failed after retries");
    } else {
        mf_log_info("mlx90614", "I2C probe OK (driver stub)");
    }
#endif
}

void mf_mlx90614_poll() {
    uint32_t now = mf_clock_millis();
#if MF_SENSORS_MOCK
    uint32_t t = now / 1000u;
    float obj = 24.0f + ((float)((t * 2u) % 50)) * 0.05f;
    bool good = false;
    for (uint32_t attempt = 0; attempt < MF_SENSOR_READ_RETRIES; ++attempt) {
        float v = obj;
        if (attempt == 0 && (t % 53u) == 0u) {
            v = NAN;
        }
        if (validate_obj(v)) {
            s_obj_c = v;
            good = true;
            break;
        }
        if ((t % 53u) == 0u) {
            obj = 24.0f + ((float)((t * 2u) % 50)) * 0.05f;
        }
    }
    if (good) {
        float air = mf_scd41_temp_c();
        mf_mock_climate_apply_mlx(&s_obj_c, air);
        mark_good(now);
    } else {
        mark_bad();
    }
#else
    bool good = false;
    for (uint32_t attempt = 0; attempt < MF_SENSOR_READ_RETRIES; ++attempt) {
        if (!mlx90614_i2c_probe()) {
            continue;
        }
        if (validate_obj(s_obj_c)) {
            good = true;
            break;
        }
    }
    if (good) {
        mark_good(now);
    } else {
        mark_bad();
    }
#endif
    if (s_ok && (int64_t)(now - s_last_ok_ms) > (int64_t)MF_SENSOR_STALE_MS) {
        s_ok = false;
        mf_log_warn("mlx90614", "stale sample age > %lums", (unsigned long)MF_SENSOR_STALE_MS);
    }
}

float mf_mlx90614_object_c() { return s_obj_c; }

bool mf_mlx90614_ok(int64_t *stale_age_ms_out) {
    int64_t age = (s_ok && s_last_ok_ms > 0)
                      ? (int64_t)(mf_clock_millis() - s_last_ok_ms)
                      : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age;
    }
    return s_ok && !s_fault_disconnected && age <= (int64_t)MF_SENSOR_STALE_MS;
}

bool mf_mlx90614_fault_disconnected() {
    return s_fault_disconnected;
}
