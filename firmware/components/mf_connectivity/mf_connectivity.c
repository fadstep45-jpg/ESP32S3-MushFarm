#include "mf_connectivity.h"
#include "mf_wifi_ap.h"
#include "mf_http_api.h"
#include "mf_mqtt_app.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "mf_conn";

esp_err_t mf_connectivity_init(void)
{
    int mqtt_enabled = 0;
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(err));
        return err;
    }

#if CONFIG_MF_WIFI_SOFTAP
    err = mf_wifi_ap_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mf_wifi_ap_start: %s", esp_err_to_name(err));
        return err;
    }
#endif
#if CONFIG_MF_HTTP_API && CONFIG_MF_WIFI_SOFTAP
    err = mf_http_api_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mf_http_api_start: %s", esp_err_to_name(err));
        return err;
    }
#endif
#if CONFIG_MF_MQTT_ENABLE
    err = mf_mqtt_app_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mf_mqtt_app_start: %s", esp_err_to_name(err));
        return err;
    }
    mqtt_enabled = 1;
#endif

    ESP_LOGI(TAG, "connectivity up (AP=%d HTTP=%d MQTT=%d)",
             CONFIG_MF_WIFI_SOFTAP, CONFIG_MF_HTTP_API, mqtt_enabled);
    return ESP_OK;
}
