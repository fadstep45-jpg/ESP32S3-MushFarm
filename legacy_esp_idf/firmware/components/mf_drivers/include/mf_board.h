#pragma once

#include "driver/gpio.h"

/* From docs/Подключение компонентов.md — DevKitC defaults; override in mf_board.c if needed */
#define MF_I2C_SDA_GPIO        GPIO_NUM_1
#define MF_I2C_SCL_GPIO        GPIO_NUM_2
#define MF_WATER_LEVEL_GPIO    GPIO_NUM_47

#define MF_PWM_FAN_GPIO        GPIO_NUM_8
#define MF_PWM_HUM_GPIO        GPIO_NUM_9
#define MF_PWM_LIGHT_GPIO      GPIO_NUM_10

#define MF_SERVICE_BUTTON_GPIO GPIO_NUM_0

void mf_board_pins_log_defaults(void);
