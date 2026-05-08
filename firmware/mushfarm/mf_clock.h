#pragma once

#include <stdint.h>

// Monotonic time helpers. Wraps Arduino millis()/micros() so the rest of
// the codebase stays platform-independent and future-proof for NTP support.

uint32_t mf_clock_millis();
uint64_t mf_clock_micros();

// Returns true once `interval_ms` has elapsed since `*last_ms`. On hit, the
// helper updates `*last_ms` so the caller does not need to reset it manually.
bool mf_clock_elapsed(uint32_t *last_ms, uint32_t interval_ms);

// Wall clock seconds since epoch. Returns 0 until NTP/RTC sets the time.
int64_t mf_clock_unix_seconds();
