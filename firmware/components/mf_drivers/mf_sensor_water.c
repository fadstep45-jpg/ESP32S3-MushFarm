#include "mf_sensor_water.h"
#include "mf_board.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "water";

static int64_t s_last_us;
static int s_level; /* 1 = liquid detected (active-high assumption; invert if hardware differs) */

esp_err_t mf_sensor_water_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << MF_WATER_LEVEL_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config: %s", esp_err_to_name(err));
        return err;
    }
    s_last_us = esp_timer_get_time();
    s_level = gpio_get_level(MF_WATER_LEVEL_GPIO);
    ESP_LOGI(TAG, "initial level=%d", s_level);
    return ESP_OK;
}

void mf_sensor_water_poll(void)
{
    s_level = gpio_get_level(MF_WATER_LEVEL_GPIO);
    s_last_us = esp_timer_get_time();
}

bool mf_sensor_water_ok(int64_t *stale_age_ms_out)
{
    int64_t now = esp_timer_get_time();
    int64_t age_ms = (now - s_last_us) / 1000;
    if (stale_age_ms_out) {
        *stale_age_ms_out = age_ms;
    }
    return true;
}

bool mf_sensor_water_present(void)
{
    return s_level != 0;
}
