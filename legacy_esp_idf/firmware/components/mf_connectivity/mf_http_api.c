#include "sdkconfig.h"
#if CONFIG_MF_HTTP_API
#include "mf_http_api.h"
#include "mf_fsm.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mf_http";

static esp_err_t send_json(httpd_req_t *req, int status_code, cJSON *obj)
{
    char *s = cJSON_PrintUnformatted(obj);
    if (!s) {
        return ESP_ERR_NO_MEM;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, status_code == 200 ? "200 OK" : "500 Internal Server Error");
    esp_err_t err = httpd_resp_send(req, s, HTTPD_RESP_USE_STRLEN);
    cJSON_free(s);
    return err;
}

static esp_err_t h_get_status(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddStringToObject(root, "request_id", "dev");
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddStringToObject(root, "code", "ACK_OK");
    cJSON_AddStringToObject(root, "state", mf_fsm_state_str(mf_fsm_state()));
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "selected_recipe_id", mf_fsm_selected_recipe_id());
    cJSON_AddItemToObject(root, "payload", payload);
    esp_err_t e = send_json(req, 200, root);
    cJSON_Delete(root);
    return e;
}

esp_err_t mf_http_api_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return ESP_FAIL;
    }

    httpd_uri_t u = {
        .uri = "/api/v1/status",
        .method = HTTP_GET,
        .handler = h_get_status,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &u);
    ESP_LOGI(TAG, "HTTP GET /api/v1/status on :80");
    return ESP_OK;
}

#else

#include "mf_http_api.h"
#include "esp_err.h"

esp_err_t mf_http_api_start(void)
{
    return ESP_OK;
}

#endif
