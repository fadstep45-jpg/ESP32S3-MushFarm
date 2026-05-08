#include "mf_i2c_bus.h"
#include "mf_board.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "mf_i2c";

static i2c_master_bus_handle_t s_bus;

esp_err_t mf_i2c_bus_init(void)
{
    const i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = MF_I2C_SDA_GPIO,
        .scl_io_num = MF_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 4,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus: %s", esp_err_to_name(err));
    }
    return err;
}

i2c_master_bus_handle_t mf_i2c_bus_handle(void)
{
    return s_bus;
}
