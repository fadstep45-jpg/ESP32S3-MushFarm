#include "mf_service_btn.h"
#include "mf_board.h"
#include "mf_log.h"
#include "mf_fsm.h"
#include "mf_clock.h"
#include <Arduino.h>

#define MF_SVC_BTN_LONG_PRESS_MS 3000u

static bool s_was_pressed = false;
static uint32_t s_press_started_ms = 0;

void mf_service_btn_init() {
    pinMode(MF_SERVICE_BUTTON_PIN, INPUT_PULLUP);
}

void mf_service_btn_poll() {
    bool pressed = (digitalRead(MF_SERVICE_BUTTON_PIN) == LOW);
    uint32_t now = mf_clock_millis();
    if (pressed && !s_was_pressed) {
        s_press_started_ms = now;
    } else if (pressed && s_was_pressed) {
        if ((now - s_press_started_ms) >= MF_SVC_BTN_LONG_PRESS_MS) {
            mf_log_warn("svc_btn", "long press -> EMERGENCY_STOP");
            mf_fsm_emergency_stop();
            s_press_started_ms = now; // debounce so we don't spam
        }
    }
    s_was_pressed = pressed;
}
