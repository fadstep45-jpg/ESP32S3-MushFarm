#include "mf_actuator_pwm.h"
#include "mf_board.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "act_pwm";

#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY      (20000)

static esp_err_t setup_channel(ledc_channel_t ch, int gpio)
{
    const ledc_channel_config_t cch = {
        .gpio_num = gpio,
        .speed_mode = LEDC_MODE,
        .channel = ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = 0,
    };
    return ledc_channel_config(&cch);
}

esp_err_t mf_actuator_pwm_init_safe_off(void)
{
    const ledc_timer_config_t tmr = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&tmr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config: %s", esp_err_to_name(err));
        return err;
    }
    err = setup_channel(LEDC_CHANNEL_0, MF_PWM_FAN_GPIO);
    if (err != ESP_OK) {
        return err;
    }
    err = setup_channel(LEDC_CHANNEL_1, MF_PWM_HUM_GPIO);
    if (err != ESP_OK) {
        return err;
    }
    err = setup_channel(LEDC_CHANNEL_2, MF_PWM_LIGHT_GPIO);
    if (err != ESP_OK) {
        return err;
    }
    ESP_LOGI(TAG, "PWM safe OFF (duty=0) fan/hum/light");
    return ESP_OK;
}

esp_err_t mf_actuator_pwm_set_percent(mf_actuator_channel_t ch, float percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    const uint32_t max_duty = (1U << LEDC_DUTY_RES) - 1;
    uint32_t duty = (uint32_t)((percent / 100.0f) * (float)max_duty);
    ledc_channel_t lc = LEDC_CHANNEL_0;
    if (ch == MF_ACT_FAN) {
        lc = LEDC_CHANNEL_0;
    } else if (ch == MF_ACT_HUMIDIFIER) {
        lc = LEDC_CHANNEL_1;
    } else {
        lc = LEDC_CHANNEL_2;
    }
    esp_err_t err = ledc_set_duty(LEDC_MODE, lc, duty);
    if (err != ESP_OK) {
        return err;
    }
    return ledc_update_duty(LEDC_MODE, lc);
}
