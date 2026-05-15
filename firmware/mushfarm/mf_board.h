#pragma once

#include <stdint.h>

// Pin map. Defaults from docs/Подключение компонентов.md (DevKitC).
// Override here if your wiring differs.

#define MF_I2C_SDA_PIN       1
#define MF_I2C_SCL_PIN       2
#define MF_WATER_LEVEL_PIN   21

// GPIO 3 — strapping pin (LOG_LEVEL / JTAG print select).
// Safe to drive after boot, but ensure the MOSFET-module has a gate
// pull-down to GND so the fan does not glitch during reset.
#define MF_PWM_FAN_PIN       3
#define MF_PWM_HUM_PIN       42
#define MF_PWM_LIGHT_PIN     47

#define MF_SERVICE_BUTTON_PIN 0

void mf_board_log_pin_map();
