#include "mf_app.h"
#include "sdkconfig.h"
#include "mf_board.h"
#include "mf_i2c_bus.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "mf_actuator_pwm.h"
#include "mf_fsm.h"
#include "mf_connectivity.h"
#include "mf_system.h"
#include "mf_climate.h"
#include "mf_arbiter.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mf_app";

static void mf_app_task(void *arg)
{
    (void)arg;
    int n = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        mf_sensor_scd41_poll();
        mf_sensor_mlx90614_poll();
        mf_sensor_water_poll();
        mf_system_poll();
        n++;
        if ((n % 4) == 0) {
            mf_climate_tick();
        }
        if ((n % 20) == 0) {
            mf_arbiter_log_compact_trace();
        }
    }
}

void mf_app_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    mf_board_pins_log_defaults();
    ESP_ERROR_CHECK(mf_actuator_pwm_init_safe_off());

    err = mf_i2c_bus_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "I2C bus init: %s (continuing for mock sensors)", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(mf_sensor_scd41_init());
    ESP_ERROR_CHECK(mf_sensor_mlx90614_init());
    ESP_ERROR_CHECK(mf_sensor_water_init());

    ESP_ERROR_CHECK(mf_connectivity_init());
    ESP_ERROR_CHECK(mf_system_init());
    mf_fsm_boot_done_config_ok();

#if CONFIG_MF_AUTO_DEMO_CYCLE
    mf_fsm_select_recipe("embedded_demo");
    mf_fsm_result_t r = mf_fsm_start_cycle();
    if (r != MF_FSM_RES_OK) {
        ESP_LOGW(TAG, "auto start cycle: %d", (int)r);
    }
#endif

    xTaskCreatePinnedToCore(mf_app_task, "mf_app", 8192, NULL, 5, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "mf_app started");
}
