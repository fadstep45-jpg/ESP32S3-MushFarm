#include "esp_log.h"
#include "esp_app_desc.h"
#include "mf_app.h"
#include "fw_build_info.h"

static const char *TAG = "app_main";

void app_main(void)
{
    const esp_app_desc_t *d = esp_app_get_description();
    ESP_LOGI(TAG, "MushFarm %s git=%s", d->version, MF_GIT_SHORT_SHA);
    mf_app_start();
}
