#include "esp_log.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include "mf_app.h"
#include "fw_build_info.h"

static const char *TAG = "app_main";

void app_main(void)
{
    const esp_app_desc_t *d = esp_app_get_description();
    ESP_LOGI(TAG, "MushFarm %s git=%s", d->version, MF_GIT_SHORT_SHA);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    mf_app_start();
}
