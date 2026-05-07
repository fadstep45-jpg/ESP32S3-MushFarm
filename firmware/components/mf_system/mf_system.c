#include "mf_system.h"
#include "mf_log_ring.h"
#include "mf_time_sync.h"
#include "mf_sd_log.h"
#include "mf_service_btn.h"
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "mf_system";

esp_err_t mf_system_init(void)
{
    mf_log_ring_push("boot");
#if CONFIG_MF_SD_LOG_ENABLE
    (void)mf_sd_log_mount();
#endif
    mf_time_sync_init();
    ESP_RETURN_ON_ERROR(mf_service_btn_init(), TAG, "service btn");
    ESP_LOGI(TAG, "mf_system_init done");
    return ESP_OK;
}

void mf_system_poll(void)
{
    mf_time_sync_poll();
    mf_service_btn_poll();
}
