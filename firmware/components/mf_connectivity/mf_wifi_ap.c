#include "mf_wifi_ap.h"
#include "sdkconfig.h"
#include <string.h>

#if CONFIG_MF_WIFI_SOFTAP
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "mf_wifi";

static esp_netif_t *s_ap_netif;

esp_err_t mf_wifi_ap_start(void)
{
    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "MushFarm-Setup",
            .ssid_len = (uint8_t)strlen("MushFarm-Setup"),
            .channel = 1,
            .password = "mushfarm1",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP SSID=%s", wifi_config.ap.ssid);
    return ESP_OK;
}

esp_netif_t *mf_wifi_ap_netif(void)
{
    return s_ap_netif;
}

#else

esp_err_t mf_wifi_ap_start(void)
{
    return ESP_OK;
}

esp_netif_t *mf_wifi_ap_netif(void)
{
    return NULL;
}

#endif
