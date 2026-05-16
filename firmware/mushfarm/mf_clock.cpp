#include "mf_clock.h"
#include "mf_log.h"
#include <Arduino.h>
#include <time.h>

// Threshold: anything below ~2000-01-01 means the SNTP/RTC has not set
// the clock yet (boot time defaults to the epoch on ESP32). Picked well
// above the epoch but below any plausible real wall clock so the gate is
// unambiguous even before NTP.
#define MF_CLOCK_SYNCED_EPOCH_THRESHOLD ((time_t)946684800)  // 2000-01-01 UTC

uint32_t mf_clock_millis() {
    return millis();
}

uint64_t mf_clock_micros() {
    return micros();
}

bool mf_clock_elapsed(uint32_t *last_ms, uint32_t interval_ms) {
    if (!last_ms) {
        return false;
    }
    uint32_t now = millis();
    if ((now - *last_ms) >= interval_ms) {
        *last_ms = now;
        return true;
    }
    return false;
}

int64_t mf_clock_unix_seconds() {
    time_t now = time(nullptr);
    if (now < MF_CLOCK_SYNCED_EPOCH_THRESHOLD) {
        return 0;
    }
    return (int64_t)now;
}

int64_t mf_clock_monotonic_seconds() {
    return (int64_t)(millis() / 1000ul);
}

bool mf_clock_time_synced() {
    return time(nullptr) >= MF_CLOCK_SYNCED_EPOCH_THRESHOLD;
}

void mf_clock_init_ntp(const char *tz_posix, const char *ntp_primary,
                       const char *ntp_secondary) {
    (void)ntp_primary;
    (void)ntp_secondary;
    // TZ only until S6 brings Wi-Fi. configTzTime() pulls SNTP/lwIP and on
    // ESP32 core 3.0.x links ip6_input -> lwip_hook_ip6_input, which is
    // defined in the Network library — not linked in this sketch yet.
    if (!tz_posix) tz_posix = "UTC0";
    setenv("TZ", tz_posix, 1);
    tzset();
    mf_log_info("clock", "TZ set to %s; SNTP deferred until Wi-Fi (S6)", tz_posix);
}

#if MF_CLOCK_SNTP_ENABLED
#include <WiFi.h>

void mf_clock_start_sntp(const char *tz_posix, const char *ntp_primary,
                         const char *ntp_secondary) {
    if (!tz_posix) tz_posix = "UTC0";
    if (!ntp_primary) ntp_primary = "pool.ntp.org";
    if (!ntp_secondary) ntp_secondary = "time.google.com";
    configTzTime(tz_posix, ntp_primary, ntp_secondary);
    mf_log_info("clock", "SNTP started tz=%s primary=%s secondary=%s",
                tz_posix, ntp_primary, ntp_secondary);
}
#endif
