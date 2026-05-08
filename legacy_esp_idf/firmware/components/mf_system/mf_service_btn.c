#include "mf_service_btn.h"
#include "mf_board.h"
#include "mf_fsm.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "svc_btn";

static int64_t s_last_us;
static uint32_t s_low_ms_accum;

esp_err_t mf_service_btn_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << MF_SERVICE_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) {
        return err;
    }
    s_last_us = esp_timer_get_time();
    ESP_LOGI(TAG, "service button GPIO %d (long press 3s -> emergency stop)", (int)MF_SERVICE_BUTTON_GPIO);
    return ESP_OK;
}

void mf_service_btn_poll(void)
{
    int64_t now = esp_timer_get_time();
    int dt_ms = (int)((now - s_last_us) / 1000);
    if (dt_ms < 0 || dt_ms > 2000) {
        dt_ms = 100;
    }
    s_last_us = now;

    if (gpio_get_level(MF_SERVICE_BUTTON_GPIO) == 0) {
        s_low_ms_accum += (uint32_t)dt_ms;
        if (s_low_ms_accum >= 3000) {
            ESP_LOGW(TAG, "long press");
            mf_fsm_emergency_stop();
            s_low_ms_accum = 0;
        }
    } else {
        s_low_ms_accum = 0;
    }
}
