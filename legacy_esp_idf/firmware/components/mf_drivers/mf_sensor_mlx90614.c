#include "mf_sensor_mlx90614.h"
#include "mf_i2c_bus.h"
#include "mf_board.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "mlx90614";

#define MLX90614_ADDR 0x5A
#define CMD_READ_OBJ1 0x07

#if !CONFIG_MF_SENSORS_MOCK
static i2c_master_dev_handle_t s_dev;
#endif
static int64_t s_last_ok_us;
static float s_obj_c;
static bool s_ok;

#if !CONFIG_MF_SENSORS_MOCK
static esp_err_t read_raw(uint16_t *raw_out)
{
    if (!s_dev) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t cmd = CMD_READ_OBJ1;
    uint8_t buf[3];
    esp_err_t err = i2c_master_transmit_receive(s_dev, &cmd, 1, buf, 3, pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        return err;
    }
    uint16_t raw = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    *raw_out = raw;
    return ESP_OK;
}
#endif

esp_err_t mf_sensor_mlx90614_init(void)
{
#if CONFIG_MF_SENSORS_MOCK
    s_obj_c = 24.0f;
    s_ok = true;
    s_last_ok_us = esp_timer_get_time();
    ESP_LOGW(TAG, "MOCK mode: synthetic object temp");
    return ESP_OK;
#else
    i2c_master_bus_handle_t bus = mf_i2c_bus_handle();
    if (!bus) {
        return ESP_ERR_INVALID_STATE;
    }
    const i2c_device_config_t dev_cfg = {
        .device_address = MLX90614_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        return err;
    }
    uint16_t raw;
    err = read_raw(&raw);
    if (err != ESP_OK) {
        s_ok = false;
        return err;
    }
    s_obj_c = (float)raw * 0.02f - 273.15f;
    s_ok = true;
    s_last_ok_us = esp_timer_get_time();
    return ESP_OK;
#endif
}

void mf_sensor_mlx90614_poll(void)
{
#if CONFIG_MF_SENSORS_MOCK
    s_obj_c = 24.0f + 0.1f * ((float)(esp_timer_get_time() / 1000000) * 0.01f);
    s_ok = true;
    s_last_ok_us = esp_timer_get_time();
#else
    uint16_t raw;
    if (read_raw(&raw) == ESP_OK) {
        s_obj_c = (float)raw * 0.02f - 273.15f;
        s_ok = true;
        s_last_ok_us = esp_timer_get_time();
    } else {
        s_ok = false;
    }
#endif
}

bool mf_sensor_mlx90614_ok(int64_t *stale_age_ms_out)
{
    int64_t now = esp_timer_get_time();
    int64_t age_ms = s_ok ? (now - s_last_ok_us) / 1000 : INT64_MAX / 4;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age_ms;
    }
    return s_ok;
}

float mf_sensor_mlx90614_object_c(void) { return s_obj_c; }
