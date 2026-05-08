#include "mf_clock.h"
#include <Arduino.h>
#include <time.h>

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
    if (now < 1700000000) {
        return 0;
    }
    return (int64_t)now;
}
