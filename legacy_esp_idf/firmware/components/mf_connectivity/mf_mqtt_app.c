#include "sdkconfig.h"
#include "mf_mqtt_app.h"
#include "esp_log.h"

#if CONFIG_MF_MQTT_ENABLE
#include "mqtt_client.h"
#include "esp_event.h"
#include <string.h>

static const char *TAG = "mf_mqtt";

static void log_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
}

esp_err_t mf_mqtt_app_start(void)
{
    const esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_MF_MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t c = esp_mqtt_client_init(&cfg);
    if (!c) {
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(c, ESP_EVENT_ANY_ID, log_event, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(c));
    ESP_LOGI(TAG, "MQTT client started -> %s", CONFIG_MF_MQTT_BROKER_URI);
    return ESP_OK;
}

#else

esp_err_t mf_mqtt_app_start(void)
{
    return ESP_OK;
}

#endif
