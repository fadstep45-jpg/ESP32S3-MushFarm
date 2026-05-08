#include "mf_sd_log.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if CONFIG_MF_SD_LOG_ENABLE
static const char *TAG = "mf_sd";
#endif

esp_err_t mf_sd_log_mount(void)
{
#if CONFIG_MF_SD_LOG_ENABLE
    ESP_LOGW(TAG, "SD FAT mount: configure pins/host in mf_sd_log.c for your wiring (Sprint 8)");
    return ESP_ERR_NOT_SUPPORTED;
#else
    return ESP_OK;
#endif
}

esp_err_t mf_sd_log_append_line(const char *line)
{
    (void)line;
#if CONFIG_MF_SD_LOG_ENABLE
    return ESP_ERR_NOT_SUPPORTED;
#else
    return ESP_ERR_INVALID_STATE;
#endif
}
