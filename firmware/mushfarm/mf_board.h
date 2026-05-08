#pragma once

#include <stdint.h>

// Pin map. Defaults from docs/Подключение компонентов.md (DevKitC).
// Override here if your wiring differs.

#define MF_I2C_SDA_PIN       1
#define MF_I2C_SCL_PIN       2
#define MF_WATER_LEVEL_PIN   47

#define MF_PWM_FAN_PIN       8
#define MF_PWM_HUM_PIN       9
#define MF_PWM_LIGHT_PIN     10

#define MF_SERVICE_BUTTON_PIN 0

void mf_board_log_pin_map();
