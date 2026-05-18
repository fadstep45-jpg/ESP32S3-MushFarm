#include "mf_http_api.h"
#include "mf_config.h"
#include "mf_http_codes.h"
#include "mf_api_codes.h"
#include "mf_cmd_dispatch.h"
#include "mf_fsm.h"
#include "mf_recipe.h"
#include "mf_wifi.h"
#include "mf_clock.h"
#include "mf_climate_trace.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "mf_net_config.h"
#include "mf_mqtt_config.h"
#include "mf_nvs_session.h"
#include "mf_actuator_test.h"
#include "mf_actuators.h"
#include "mf_log.h"
#if MF_MQTT_ENABLE
#include "mf_mqtt.h"
#endif

#if MF_HTTP_API

#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Esp.h>  // ESP32 core 3.x: Esp.h (case-sensitive on Linux CI)
#include <string.h>

static WebServer s_server(80);
static char s_req_id[8];

static void http_make_request_id() {
    snprintf(s_req_id, sizeof(s_req_id), "%04x", (unsigned)(millis() & 0xffffu));
}

static const char *http_request_id() {
    if (s_server.hasArg("request_id")) {
        return s_server.arg("request_id").c_str();
    }
    return s_req_id;
}

static void http_send_envelope(bool ok, const char *code, const char *detail,
                               JsonVariantConst payload) {
    StaticJsonDocument<1536> doc;
    doc["request_id"] = http_request_id();
    doc["status"] = ok ? "ok" : "error";
    doc["code"] = code;
    doc["state"] = mf_fsm_state_str(mf_fsm_state());
    if (detail && detail[0]) {
        doc["detail"] = detail;
    }
    if (!payload.isNull()) {
        doc["payload"] = payload;
    } else {
        doc.createNestedObject("payload");
    }
    String out;
    serializeJson(doc, out);
    s_server.send(ok ? 200 : 400, "application/json", out);
}

static void http_send_cmd_result(const mf_cmd_result_t *res) {
    if (res->ok) {
        http_send_envelope(true, res->code, res->detail, JsonVariantConst());
    } else {
        http_send_envelope(false, res->code, res->detail, JsonVariantConst());
    }
}

static void http_send_error(const char *code, const char *detail) {
    http_send_envelope(false, code, detail, JsonVariantConst());
}

static void handle_get_status() {
    http_make_request_id();
    StaticJsonDocument<768> doc;
    JsonObject payload = doc.to<JsonObject>();
    payload["warn_flags"] = mf_fsm_warn_flags();
    payload["selected_recipe_id"] = mf_fsm_selected_recipe_id();
    payload["stage_id"] = mf_recipe_current_stage_id();
    payload["stage_elapsed_s"] = (int32_t)mf_recipe_stage_elapsed_seconds();
    payload["emergency_latched"] = mf_fsm_emergency_latched();
    payload["time_synced"] = mf_clock_time_synced();
    payload["arb_reason"] = mf_climate_trace_last_reason_str();
    payload["fan_pct"] = mf_climate_trace_last_fan_pct();
    payload["hum_pct"] = mf_climate_trace_last_hum_pct();
    JsonObject wifi = payload.createNestedObject("wifi");
    wifi["mode"] = mf_wifi_mode_str();
    wifi["sta_connected"] = mf_wifi_sta_connected();
    wifi["rssi"] = mf_wifi_sta_rssi();
    wifi["ip"] = mf_wifi_sta_ip_str();
    wifi["softap_active"] = mf_wifi_softap_active();
#if MF_MQTT_ENABLE
    JsonObject mqtt = payload.createNestedObject("mqtt");
    mqtt["configured"] = mf_mqtt_config_is_configured();
    mqtt["connected"] = mf_mqtt_connected();
#endif
    http_send_envelope(true, MF_HTTP_ACK_OK, nullptr, payload);
}

static void handle_get_sensors_live() {
    http_make_request_id();
    StaticJsonDocument<512> doc;
    JsonObject payload = doc.to<JsonObject>();
    int64_t stale = 0;

    JsonObject scd = payload.createNestedObject("scd41");
    if (mf_scd41_ok(&stale)) {
        scd["co2_ppm"] = mf_scd41_co2_ppm();
        scd["rh_percent"] = mf_scd41_rh_percent();
        scd["temp_c"] = mf_scd41_temp_c();
        scd["stale_age_ms"] = (int32_t)stale;
        scd["fault"] = mf_scd41_fault_disconnected();
    } else {
        scd["valid"] = false;
        scd["fault"] = mf_scd41_fault_disconnected();
    }

    JsonObject mlx = payload.createNestedObject("mlx90614");
    if (mf_mlx90614_ok(&stale)) {
        mlx["object_c"] = mf_mlx90614_object_c();
        mlx["stale_age_ms"] = (int32_t)stale;
        mlx["fault"] = mf_mlx90614_fault_disconnected();
    } else {
        mlx["valid"] = false;
        mlx["fault"] = mf_mlx90614_fault_disconnected();
    }

    JsonObject water = payload.createNestedObject("water");
    water["present"] = mf_water_present();
    http_send_envelope(true, MF_HTTP_ACK_OK, nullptr, payload);
}

static void handle_get_recipes_list() {
    http_make_request_id();
    StaticJsonDocument<256> doc;
    JsonObject payload = doc.to<JsonObject>();
    mf_cmd_fill_recipe_list(payload);
    http_send_envelope(true, MF_HTTP_ACK_OK, nullptr, payload);
}

static void handle_get_recipe_embedded_demo() {
    http_make_request_id();
    StaticJsonDocument<384> doc;
    JsonObject payload = doc.to<JsonObject>();
    if (!mf_cmd_fill_recipe_get("embedded_demo", payload)) {
        http_send_error(MF_API_ERR_RECIPE_NOT_FOUND, "Unknown recipe");
        return;
    }
    http_send_envelope(true, MF_HTTP_ACK_OK, nullptr, payload);
}

static void handle_post_recipe_select_embedded_demo() {
    http_make_request_id();
    mf_cmd_result_t res = mf_cmd_recipe_select("embedded_demo");
    http_send_cmd_result(&res);
}

static void handle_post_cycle_start() {
    http_make_request_id();
    const char *rid = nullptr;
    char rid_buf[48];
    if (s_server.hasArg("plain")) {
        StaticJsonDocument<128> body;
        if (deserializeJson(body, s_server.arg("plain")) == DeserializationError::Ok) {
            const char *r = body["recipe_id"] | "";
            if (r[0]) {
                strncpy(rid_buf, r, sizeof(rid_buf) - 1);
                rid_buf[sizeof(rid_buf) - 1] = '\0';
                rid = rid_buf;
            }
        }
    }
    mf_cmd_result_t res = mf_cmd_cycle_start(rid);
    http_send_cmd_result(&res);
}

static mf_actuator_t parse_actuator(const char *name) {
    if (!name) {
        return MF_ACT__COUNT;
    }
    if (strcmp(name, "fan") == 0) {
        return MF_ACT_FAN;
    }
    if (strcmp(name, "hum") == 0 || strcmp(name, "humidifier") == 0) {
        return MF_ACT_HUMIDIFIER;
    }
    if (strcmp(name, "light") == 0) {
        return MF_ACT_LIGHT;
    }
    return MF_ACT__COUNT;
}

static void handle_post_test_actuator() {
    http_make_request_id();
    if (!s_server.hasArg("plain")) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "JSON body required");
        return;
    }
    StaticJsonDocument<256> body;
    if (deserializeJson(body, s_server.arg("plain")) != DeserializationError::Ok) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "Invalid JSON");
        return;
    }
    const char *act = body["actuator"] | "";
    float pct = body["percent"] | 0.0f;
    uint32_t timeout_s = body["timeout_s"] | 30u;
    mf_actuator_t which = parse_actuator(act);
    if (which >= MF_ACT__COUNT) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "Unknown actuator");
        return;
    }
    if (!mf_actuator_test_start(which, pct, timeout_s)) {
        http_send_error(MF_HTTP_ERR_SAFETY_LIMIT, "Actuator test denied");
        return;
    }
    http_send_envelope(true, MF_HTTP_ACK_OK, nullptr, JsonVariantConst());
}

static void handle_post_config_apply() {
    http_make_request_id();
    if (!s_server.hasArg("plain")) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "JSON body required");
        return;
    }
    StaticJsonDocument<512> body;
    if (deserializeJson(body, s_server.arg("plain")) != DeserializationError::Ok) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "Invalid JSON");
        return;
    }
    JsonObject wifi = body["wifi"];
    if (!wifi.isNull()) {
        const char *ssid = wifi["ssid"] | "";
        const char *pass = wifi["password"] | "";
        if (!ssid[0]) {
            http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "ssid required");
            return;
        }
        if (!mf_net_config_save(ssid, pass)) {
            http_send_error(MF_HTTP_ERR_STATE, "Failed to save Wi-Fi credentials");
            return;
        }
        mf_log_info("http", "wifi credentials saved");
    } else if (!mf_net_config_is_configured()) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "wifi object required on first provision");
        return;
    }

#if MF_MQTT_ENABLE
    JsonObject mqtt = body["mqtt"];
    if (!mqtt.isNull()) {
        const char *host = mqtt["broker"] | mqtt["host"] | "";
        uint16_t port = mqtt["port"] | 1883;
        const char *user = mqtt["username"] | mqtt["user"] | "";
        const char *pass = mqtt["password"] | "";
        const char *devid = mqtt["device_id"] | "";
        if (!host[0]) {
            http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "mqtt.broker required");
            return;
        }
        if (!mf_mqtt_config_save(host, port, user, pass, devid[0] ? devid : nullptr)) {
            http_send_error(MF_HTTP_ERR_STATE, "Failed to save MQTT config");
            return;
        }
        mf_log_info("http", "mqtt config saved");
    }
#endif

    mf_log_info("http", "config apply; restarting");
    StaticJsonDocument<64> payload;
    http_send_envelope(true, MF_HTTP_ACK_OK, "Rebooting to apply config", payload.as<JsonObject>());
    delay(200);
    ESP.restart();
}

static void handle_post_factory_reset() {
    http_make_request_id();
    if (!s_server.hasArg("plain")) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "JSON body required");
        return;
    }
    StaticJsonDocument<128> body;
    if (deserializeJson(body, s_server.arg("plain")) != DeserializationError::Ok) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "Invalid JSON");
        return;
    }
    const char *token = body["confirm_token"] | "";
    if (strcmp(token, "FACTORY_RESET") != 0) {
        http_send_error(MF_HTTP_ERR_SCHEMA_INVALID, "Invalid confirm_token");
        return;
    }
    mf_net_config_clear();
#if MF_MQTT_ENABLE
    mf_mqtt_config_clear();
#endif
    mf_session_clear();
    mf_log_info("http", "factory reset; rebooting");
    StaticJsonDocument<64> payload;
    http_send_envelope(true, MF_HTTP_ACK_OK, "Factory reset complete", payload.as<JsonObject>());
    delay(200);
    ESP.restart();
}

static void handle_not_implemented() {
    http_make_request_id();
    mf_cmd_result_t res = mf_cmd_not_implemented("Endpoint not implemented in this firmware phase");
    http_send_cmd_result(&res);
}

static void route_request() {
    String uri = s_server.uri();
    HTTPMethod method = s_server.method();

    if (uri == "/api/v1/status" && method == HTTP_GET) {
        handle_get_status();
        return;
    }
    if (uri == "/api/v1/sensors/live" && method == HTTP_GET) {
        handle_get_sensors_live();
        return;
    }
    if (uri == "/api/v1/recipes" && method == HTTP_GET) {
        handle_get_recipes_list();
        return;
    }
    if (uri == "/api/v1/recipes/embedded_demo" && method == HTTP_GET) {
        handle_get_recipe_embedded_demo();
        return;
    }
    if (uri == "/api/v1/recipes/embedded_demo/select" && method == HTTP_POST) {
        handle_post_recipe_select_embedded_demo();
        return;
    }
    if (uri == "/api/v1/cycle/start" && method == HTTP_POST) {
        handle_post_cycle_start();
        return;
    }
    if (uri == "/api/v1/cycle/pause" && method == HTTP_POST) {
        http_make_request_id();
        mf_cmd_result_t res = mf_cmd_cycle_pause();
        http_send_cmd_result(&res);
        return;
    }
    if (uri == "/api/v1/cycle/resume" && method == HTTP_POST) {
        http_make_request_id();
        mf_cmd_result_t res = mf_cmd_cycle_resume();
        http_send_cmd_result(&res);
        return;
    }
    if (uri == "/api/v1/cycle/stop-emergency" && method == HTTP_POST) {
        http_make_request_id();
        mf_cmd_result_t res = mf_cmd_cycle_stop_emergency();
        http_send_cmd_result(&res);
        return;
    }
    if (uri == "/api/v1/config/apply" && method == HTTP_POST) {
        handle_post_config_apply();
        return;
    }
    if (uri == "/api/v1/config/factory-reset" && method == HTTP_POST) {
        handle_post_factory_reset();
        return;
    }
    if (uri == "/api/v1/service/test-actuator" && method == HTTP_POST) {
        handle_post_test_actuator();
        return;
    }
    if (uri.startsWith("/api/v1/recipes") && method == HTTP_POST) {
        handle_not_implemented();
        return;
    }
    if (uri.startsWith("/api/v1/cycle/")) {
        handle_not_implemented();
        return;
    }

    s_server.send(404, "application/json", "{\"status\":\"error\",\"code\":\"ERR_NOT_FOUND\"}");
}

void mf_http_init() {
    s_server.onNotFound(route_request);
    s_server.begin();
    mf_log_info("http", "API /api/v1 on port 80");
}

void mf_http_poll() {
    for (int i = 0; i < 4; i++) {
        s_server.handleClient();
    }
}

#else

void mf_http_init() {}
void mf_http_poll() {}

#endif
