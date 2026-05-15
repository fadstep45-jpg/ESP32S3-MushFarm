#pragma once

// Compile-time configuration. Replaces the old Kconfig.projbuild.
// Toggle features by editing these flags before Verify in Arduino IDE.

#define MF_FW_VERSION "0.2.0-arduino"

#ifndef MF_GIT_SHORT_SHA
#define MF_GIT_SHORT_SHA "unknown"
#endif

// Synthetic sensor values when no I2C hardware is connected.
// Disable on real hardware to use the I2C drivers.
#define MF_SENSORS_MOCK 1

// Local management/HTTP API. SoftAP is created on boot when enabled.
#define MF_WIFI_SOFTAP 1
#define MF_HTTP_API    1

// Remote MQTT client.
#define MF_MQTT_ENABLE 0

// SD card FAT logging (FAT mount + append). RAM batch logger always runs.
#define MF_SD_LOG_ENABLE 0

// Camera (DVP). The target board ships with a 24-pin FFC camera module
// (OV2640 / OV3660 / OV5640 — final model TBD). Pins/driver are wired
// in a future sprint; for now mf_camera is a stub.
#define MF_CAMERA_ENABLE 0

// Service mode: auto-select demo recipe and start cycle on boot.
#define MF_AUTO_DEMO_CYCLE 1

// S2 sensor pipeline (fault-model.md): retries per poll, stale window, recovery streak.
#define MF_SENSOR_READ_RETRIES       5u
#define MF_SENSOR_STALE_MS          5000u
#define MF_SENSOR_RECOVERY_SAMPLES  10u
#define MF_WATER_DEBOUNCE_SAMPLES   3u

// S4 recipe runtime tick (stage elapsed / transitions).
#define MF_TICK_RECIPE_MS           1000u

// Loop scheduling intervals (milliseconds).
#define MF_TICK_SENSORS_MS    500
#define MF_TICK_CLIMATE_MS    2000
#define MF_TICK_TRACE_MS      10000
#define MF_TICK_BATCH_FLUSH_MS 5000
