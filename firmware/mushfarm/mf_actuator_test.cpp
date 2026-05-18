#include "mf_actuator_test.h"
#include "mf_sensor_water.h"
#include "mf_actuators.h"
#include "mf_log.h"
#include <Arduino.h>

static bool s_active = false;
static uint32_t s_end_ms = 0;
static mf_actuator_t s_which = MF_ACT_FAN;

bool mf_actuator_test_blocks_climate() {
    return s_active;
}

bool mf_actuator_test_start(mf_actuator_t which, float percent, uint32_t timeout_s) {
    if (which < 0 || which >= MF_ACT__COUNT) {
        return false;
    }
    if (timeout_s == 0 || timeout_s > 600) {
        return false;
    }
    if (percent < 0.0f) {
        percent = 0.0f;
    }
    if (percent > 100.0f) {
        percent = 100.0f;
    }
    if (which == MF_ACT_HUMIDIFIER && !mf_water_present()) {
        mf_log_warn("act_test", "humidifier test denied (dry tank)");
        return false;
    }

    mf_actuators_all_off();
    mf_actuators_set_percent(which, percent);
    s_which = which;
    s_end_ms = millis() + timeout_s * 1000ul;
    s_active = true;
    mf_log_info("act_test", "started actuator=%d pct=%.0f timeout=%us",
                (int)which, (double)percent, (unsigned)timeout_s);
    return true;
}

void mf_actuator_test_poll() {
    if (!s_active) {
        return;
    }
    if ((int32_t)(millis() - s_end_ms) >= 0) {
        mf_actuators_all_off();
        s_active = false;
        mf_log_info("act_test", "timeout rollback");
    }
}
