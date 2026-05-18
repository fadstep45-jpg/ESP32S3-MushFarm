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

// Local management/HTTP API. SoftAP only while FSM is in SETUP_AP.
#define MF_WIFI_SOFTAP 1
#define MF_HTTP_API    1

// S6 Wi-Fi / HTTP (see docs/architecture/s6-acceptance.md).
#define MF_WIFI_AP_SSID            "MushFarm_Setup"
#define MF_WIFI_AP_IP              "192.168.4.1"
#define MF_WIFI_STA_TIMEOUT_MS     15000u
#define MF_WIFI_RECONNECT_MS       30000u
#define MF_TICK_NETWORK_MS         300u
#define MF_HTTP_AUTH_ENABLED       0

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

// S2 water-sensor false-positive policy (fault-model.md §Water Sensor).
// On first LOW the policy enters a reserve-warn window; if water stays
// LOW past MF_WATER_RESERVE_TIMER_S, the humidifier is duty-cycled
// (ON for MF_WATER_PULSE_ON_S, OFF for MF_WATER_PULSE_OFF_S) while RH
// trend is checked. If RH does not rise by MF_WATER_PULSE_RH_RISE_DELTA
// across MF_WATER_PULSE_FLAT_LIMIT consecutive pulse cycles, the
// humidifier is locked OFF and an evFaultNonFatal(WATER) is raised.
#define MF_WATER_RESERVE_TIMER_S      60u
#define MF_WATER_PULSE_ON_S           15u
#define MF_WATER_PULSE_OFF_S         180u
#define MF_WATER_PULSE_RH_RISE_DELTA   1.0f
#define MF_WATER_PULSE_FLAT_LIMIT      3u

// S8 NTP/TZ — recipe lighting and event timestamps need wall clock.
// Boot sets TZ via setenv only; SNTP (configTzTime) stays off until S6
// enables MF_CLOCK_SNTP_ENABLED and calls mf_clock_start_sntp() after
// Wi-Fi is up — avoids linking lwIP hooks without the Network library.
#define MF_CLOCK_SNTP_ENABLED    1
#define MF_CLOCK_TZ_POSIX        "MSK-3"
#define MF_CLOCK_NTP_PRIMARY     "pool.ntp.org"
#define MF_CLOCK_NTP_SECONDARY   "time.google.com"

// S4 recipe runtime tick (stage elapsed / transitions).
#define MF_TICK_RECIPE_MS           1000u

// Loop scheduling intervals (milliseconds).
#define MF_TICK_SENSORS_MS    500
#define MF_TICK_CLIMATE_MS    2000
#define MF_TICK_TRACE_MS      10000
#define MF_TICK_BATCH_FLUSH_MS 5000

// S5 PID gains (bench tuning in phase B).
#define MF_PID_RH_KP    2.0f
#define MF_PID_RH_KI    0.15f
#define MF_PID_RH_KD    0.0f
#define MF_PID_CO2_KP   0.03f
#define MF_PID_CO2_KI   0.005f
#define MF_PID_CO2_KD   0.0f
#define MF_PID_TEMP_KP  3.0f
#define MF_PID_TEMP_KI  0.1f
#define MF_PID_TEMP_KD  0.0f
#define MF_RH_SLEW_MAX_PER_MIN 15.0f
#define MF_ARB_SEQ_ALTERNATE_TICKS 4u
#define MF_CLIMATE_TRACE_FULL_EVERY_N 10u

// S5 mock golden-vector scenario (see docs/architecture/s5-acceptance.md).
#define MF_MOCK_CLIMATE_SCENARIO 0
