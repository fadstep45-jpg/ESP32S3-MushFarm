#pragma once

#include <stdint.h>
#include <stdbool.h>

// Monotonic time helpers. Wraps Arduino millis()/micros() so the rest of
// the codebase stays platform-independent and future-proof for NTP support.

uint32_t mf_clock_millis();
uint64_t mf_clock_micros();

// Returns true once `interval_ms` has elapsed since `*last_ms`. On hit, the
// helper updates `*last_ms` so the caller does not need to reset it manually.
bool mf_clock_elapsed(uint32_t *last_ms, uint32_t interval_ms);

// Wall clock seconds since epoch. Returns 0 until NTP/RTC sets the time.
// Callers that need to differentiate "not synced" from "epoch zero" should
// gate on mf_clock_time_synced() first.
int64_t mf_clock_unix_seconds();

// Monotonic seconds since boot. Always valid (no NTP required). Useful for
// stage-elapsed accounting within a single power-on; resets on every reboot.
int64_t mf_clock_monotonic_seconds();

// True once the system clock has been set (via SNTP, RTC, or a manual call).
// Threshold: time(NULL) > 2000-01-01.
bool mf_clock_time_synced();

// Configure SNTP and the IANA TZ string (e.g. "UTC0" or
// "MSK-3" for Europe/Moscow without DST). Safe to call before Wi-Fi is up;
// SNTP will sit waiting until the network appears (S6). Until then,
// mf_clock_time_synced() stays false and consumers use the monotonic path.
void mf_clock_init_ntp(const char *tz_posix, const char *ntp_server_primary,
                       const char *ntp_server_secondary);
