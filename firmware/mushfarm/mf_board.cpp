#include "mf_board.h"
#include "mf_log.h"

void mf_board_log_pin_map() {
    mf_log_info("board",
                "I2C SDA=%d SCL=%d water=%d PWM fan/hum/light=%d/%d/%d svc_btn=%d",
                (int)MF_I2C_SDA_PIN, (int)MF_I2C_SCL_PIN, (int)MF_WATER_LEVEL_PIN,
                (int)MF_PWM_FAN_PIN, (int)MF_PWM_HUM_PIN, (int)MF_PWM_LIGHT_PIN,
                (int)MF_SERVICE_BUTTON_PIN);
}
