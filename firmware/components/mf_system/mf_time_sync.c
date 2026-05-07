#include "mf_time_sync.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_sntp.h"

static const char *TAG = "mf_sntp";

static void on_sync(struct timeval *tv)
{
    (void)tv;
    ESP_LOGI(TAG, "time sync callback");
}

void mf_time_sync_init(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_set_time_sync_notification_cb(on_sync);
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP init (Sprint 8; works when STA uplink exists)");
}

void mf_time_sync_poll(void)
{
    /* Reserved: TZ offset from NVS per threat-model-and-time.md */
}
