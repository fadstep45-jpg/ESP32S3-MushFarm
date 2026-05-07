#include "mf_board.h"
#include "esp_log.h"

static const char *TAG = "mf_board";

void mf_board_pins_log_defaults(void)
{
    ESP_LOGI(TAG, "I2C SDA=%d SCL=%d water=%d PWM fan/hum/light=%d/%d/%d svc_btn=%d",
             (int)MF_I2C_SDA_GPIO, (int)MF_I2C_SCL_GPIO, (int)MF_WATER_LEVEL_GPIO,
             (int)MF_PWM_FAN_GPIO, (int)MF_PWM_HUM_GPIO, (int)MF_PWM_LIGHT_GPIO,
             (int)MF_SERVICE_BUTTON_GPIO);
}
