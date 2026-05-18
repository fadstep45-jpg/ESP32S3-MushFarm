#include "mf_mqtt.h"
#include "mf_config.h"
#include "mf_mqtt_config.h"
#include "mf_cmd_dispatch.h"
#include "mf_msg_dedup.h"
#include "mf_api_codes.h"
#include "mf_fsm.h"
#include "mf_wifi.h"
#include "mf_net_config.h"
#include "mf_clock.h"
#include "mf_climate_trace.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "mf_recipe.h"
#include "mf_resources.h"
#include "mf_log.h"

#if MF_MQTT_ENABLE

#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>

static WiFiClient s_wifi_client;
static PubSubClient s_mqtt(s_wifi_client);
static char s_topic_cmd[80];
static char s_topic_ack[80];
static char s_topic_telemetry[80];
static char s_topic_alert[80];
static char s_rx_buf[MF_MQTT_BUFFER_SIZE];
static bool s_was_connected = false;
static uint32_t s_next_reconnect_ms = 0;
static uint32_t s_reconnect_delay_ms = MF_MQTT_RECONNECT_BASE_MS;
static uint32_t s_last_telemetry_ms = 0;
static bool s_subscribed = false;

struct alert_slot_t {
    char severity[12];
    char code[32];
    char detail[64];
    bool used;
};

static alert_slot_t s_alert_queue[MF_MQTT_ALERT_QUEUE_SIZE];

static void build_topics() {
    mf_mqtt_config_ensure_device_id();
    const char *id = mf_mqtt_config_device_id();
    snprintf(s_topic_cmd, sizeof(s_topic_cmd), "mf/%s/cmd/#", id);
    snprintf(s_topic_ack, sizeof(s_topic_ack), "mf/%s/ack", id);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "mf/%s/telemetry", id);
    snprintf(s_topic_alert, sizeof(s_topic_alert), "mf/%s/alert", id);
}

static bool may_connect() {
    if (!mf_mqtt_config_is_configured()) {
        return false;
    }
    if (!mf_net_config_is_configured()) {
        return false;
    }
    if (!mf_wifi_sta_connected()) {
        return false;
    }
    mf_runtime_state_t st = mf_fsm_state();
    if (st == MF_STATE_SETUP_AP || st == MF_STATE_BOOT) {
        return false;
    }
    return true;
}

static void publish_ack(const char *msg_id, const mf_cmd_result_t *res) {
    StaticJsonDocument<384> doc;
    doc["msg_id"] = msg_id ? msg_id : "";
    doc["status"] = res->ok ? "ok" : "error";
    doc["code"] = res->code;
    doc["state"] = mf_fsm_state_str(mf_fsm_state());
    if (res->detail && res->detail[0]) {
        doc["detail"] = res->detail;
    }
    doc.createNestedObject("payload");
    char out[MF_MQTT_BUFFER_SIZE];
    size_t n = serializeJson(doc, out, sizeof(out));
    if (n > 0) {
        s_mqtt.publish(s_topic_ack, out, false);
    }
}

static void publish_ack_payload(const char *msg_id, const mf_cmd_result_t *res,
                                JsonObjectConst payload) {
    StaticJsonDocument<512> doc;
    doc["msg_id"] = msg_id ? msg_id : "";
    doc["status"] = res->ok ? "ok" : "error";
    doc["code"] = res->code;
    doc["state"] = mf_fsm_state_str(mf_fsm_state());
    if (res->detail && res->detail[0]) {
        doc["detail"] = res->detail;
    }
    if (!payload.isNull()) {
        doc["payload"] = payload;
    } else {
        doc.createNestedObject("payload");
    }
    char out[MF_MQTT_BUFFER_SIZE];
    size_t n = serializeJson(doc, out, sizeof(out));
    if (n > 0) {
        s_mqtt.publish(s_topic_ack, out, false);
    }
}

static bool is_deferred_cmd(const char *path) {
    if (!path) {
        return false;
    }
    if (strstr(path, "upload") || strstr(path, "delete") || strstr(path, "rename")) {
        return true;
    }
    if (strcmp(path, "cycle/next_stage") == 0 || strcmp(path, "cycle/prev_stage") == 0 ||
        strcmp(path, "cycle/patch_stage") == 0) {
        return true;
    }
    if (strncmp(path, "service/", 8) == 0) {
        return true;
    }
    return false;
}

static void handle_command(const char *cmd_path, const char *msg_id, JsonObject payload) {
    mf_cmd_result_t res;
    StaticJsonDocument<384> payload_doc;
    JsonObject out_payload = payload_doc.to<JsonObject>();

    if (is_deferred_cmd(cmd_path)) {
        res = mf_cmd_not_implemented("Not implemented in S7");
        publish_ack(msg_id, &res);
        return;
    }

    if (strcmp(cmd_path, "recipe/list") == 0) {
        mf_cmd_fill_recipe_list(out_payload);
        res = {true, MF_API_ACK_OK, nullptr};
        publish_ack_payload(msg_id, &res, out_payload);
        return;
    }

    if (strcmp(cmd_path, "recipe/get") == 0) {
        const char *rid = payload["recipe_id"] | "";
        if (!mf_cmd_fill_recipe_get(rid, out_payload)) {
            res = {false, MF_API_ERR_RECIPE_NOT_FOUND, "Unknown recipe"};
            publish_ack(msg_id, &res);
            return;
        }
        res = {true, MF_API_ACK_OK, nullptr};
        publish_ack_payload(msg_id, &res, out_payload);
        return;
    }

    if (strcmp(cmd_path, "recipe/select") == 0) {
        const char *rid = payload["recipe_id"] | "";
        res = mf_cmd_recipe_select(rid);
        publish_ack(msg_id, &res);
        if (res.ok || strcmp(res.code, MF_API_ACK_NOOP) == 0) {
            mf_log_info("mqtt_cmd", "recipe/select %s -> %s", rid, res.code);
        }
        return;
    }

    if (strcmp(cmd_path, "cycle/start") == 0) {
        const char *rid = payload["recipe_id"] | "";
        res = mf_cmd_cycle_start(rid[0] ? rid : nullptr);
        publish_ack(msg_id, &res);
        mf_log_info("mqtt_cmd", "cycle/start -> %s", res.code);
        return;
    }

    if (strcmp(cmd_path, "cycle/pause") == 0) {
        res = mf_cmd_cycle_pause();
        publish_ack(msg_id, &res);
        return;
    }

    if (strcmp(cmd_path, "cycle/resume") == 0) {
        res = mf_cmd_cycle_resume();
        publish_ack(msg_id, &res);
        return;
    }

    if (strcmp(cmd_path, "cycle/stop_emergency") == 0) {
        res = mf_cmd_cycle_stop_emergency();
        publish_ack(msg_id, &res);
        mf_log_info("mqtt_cmd", "cycle/stop_emergency");
        return;
    }

    res = {false, MF_API_ERR_NOT_FOUND, "Unknown command"};
    publish_ack(msg_id, &res);
}

static void mqtt_callback(char *topic, byte *payload, unsigned int length) {
    if (length >= sizeof(s_rx_buf)) {
        mf_log_warn("mqtt", "message too large (%u)", length);
        mf_cmd_result_t res = {false, MF_API_ERR_PAYLOAD_TOO_LARGE,
                               "MQTT payload exceeds device buffer"};
        publish_ack("", &res);
        return;
    }
    memcpy(s_rx_buf, payload, length);
    s_rx_buf[length] = '\0';

    mf_mqtt_config_ensure_device_id();
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "mf/%s/cmd/", mf_mqtt_config_device_id());
    if (strncmp(topic, prefix, strlen(prefix)) != 0) {
        return;
    }
    const char *cmd_path = topic + strlen(prefix);

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, s_rx_buf, length) != DeserializationError::Ok) {
        mf_cmd_result_t res = {false, MF_API_ERR_SCHEMA_INVALID, "Invalid JSON"};
        publish_ack("", &res);
        return;
    }

    const char *msg_id = doc["msg_id"] | "";
    JsonObject inner = doc["payload"].as<JsonObject>();
    if (inner.isNull()) {
        inner = doc.as<JsonObject>();
    }

    if (msg_id[0] && mf_msg_dedup_seen(msg_id)) {
        mf_cmd_result_t dup = {true, MF_API_ACK_NOOP, "duplicate msg_id"};
        publish_ack(msg_id, &dup);
        return;
    }

    handle_command(cmd_path, msg_id, inner);

    if (msg_id[0]) {
        mf_msg_dedup_commit(msg_id);
    }
}

static bool try_connect() {
    build_topics();
    const char *host = mf_mqtt_config_broker_host();
    uint16_t port = mf_mqtt_config_broker_port();
    s_mqtt.setServer(host, port);
    s_mqtt.setBufferSize(MF_MQTT_BUFFER_SIZE);
    s_mqtt.setCallback(mqtt_callback);

    const char *user = mf_mqtt_config_username();
    const char *pass = mf_mqtt_config_password();
    bool ok = false;
    if (user[0]) {
        ok = s_mqtt.connect(mf_mqtt_config_device_id(), user, pass);
    } else {
        ok = s_mqtt.connect(mf_mqtt_config_device_id());
    }
    if (!ok) {
        mf_log_warn("mqtt", "connect failed rc=%d", s_mqtt.state());
        return false;
    }
    if (!s_mqtt.subscribe(s_topic_cmd)) {
        mf_log_warn("mqtt", "subscribe failed");
        s_mqtt.disconnect();
        return false;
    }
    s_subscribed = true;
    mf_log_info("mqtt", "connected %s:%u dev=%s", host, (unsigned)port,
                mf_mqtt_config_device_id());
    mf_log_resource_metrics("mqtt");
    s_reconnect_delay_ms = MF_MQTT_RECONNECT_BASE_MS;
    return true;
}

static void flush_alert_queue() {
    for (uint32_t i = 0; i < MF_MQTT_ALERT_QUEUE_SIZE; ++i) {
        if (!s_alert_queue[i].used) {
            continue;
        }
        StaticJsonDocument<256> doc;
        doc["severity"] = s_alert_queue[i].severity;
        doc["code"] = s_alert_queue[i].code;
        doc["detail"] = s_alert_queue[i].detail;
        doc["ts"] = mf_clock_unix_seconds();
        char out[256];
        if (serializeJson(doc, out, sizeof(out)) > 0) {
            s_mqtt.publish(s_topic_alert, out, false);
        }
        s_alert_queue[i].used = false;
    }
}

static void publish_telemetry() {
    uint32_t now = mf_clock_millis();
    if ((now - s_last_telemetry_ms) < MF_MQTT_TELEMETRY_MS) {
        return;
    }
    s_last_telemetry_ms = now;

    StaticJsonDocument<512> doc;
    doc["ts"] = mf_clock_unix_seconds();
    doc["state"] = mf_fsm_state_str(mf_fsm_state());
    doc["warn_flags"] = mf_fsm_warn_flags();
    doc["recipe_id"] = mf_fsm_selected_recipe_id();
    doc["stage_id"] = mf_recipe_current_stage_id();
    doc["fan_pct"] = mf_climate_trace_last_fan_pct();
    doc["hum_pct"] = mf_climate_trace_last_hum_pct();
    doc["time_synced"] = mf_clock_time_synced();
    doc["rh_percent"] = mf_scd41_rh_percent();
    doc["co2_ppm"] = mf_scd41_co2_ppm();
    doc["air_temp_c"] = mf_scd41_temp_c();
    doc["object_temp_c"] = mf_mlx90614_object_c();
    doc["water_present"] = mf_water_present();
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["sta_connected"] = mf_wifi_sta_connected();
    wifi["ip"] = mf_wifi_sta_ip_str();
    wifi["rssi"] = mf_wifi_sta_rssi();

    char out[MF_MQTT_BUFFER_SIZE];
    if (serializeJson(doc, out, sizeof(out)) > 0) {
        s_mqtt.publish(s_topic_telemetry, out, false);
    }
}

void mf_mqtt_init() {
    mf_msg_dedup_init();
    for (uint32_t i = 0; i < MF_MQTT_ALERT_QUEUE_SIZE; ++i) {
        s_alert_queue[i].used = false;
    }
    s_mqtt.setBufferSize(MF_MQTT_BUFFER_SIZE);
    mf_mqtt_config_load();
    mf_log_info("mqtt", "init enable=1");
}

void mf_mqtt_poll() {
    bool want = may_connect();
    bool connected = s_mqtt.connected();

    if (!want) {
        if (connected) {
            s_mqtt.disconnect();
        }
        s_subscribed = false;
        if (s_was_connected) {
            mf_log_info("mqtt", "disconnected (policy)");
        }
        s_was_connected = false;
        return;
    }

    uint32_t now = mf_clock_millis();
    if (!connected) {
        if (now < s_next_reconnect_ms) {
            return;
        }
        if (!try_connect()) {
            s_next_reconnect_ms = now + s_reconnect_delay_ms;
            if (s_reconnect_delay_ms < MF_MQTT_RECONNECT_MAX_MS) {
                s_reconnect_delay_ms *= 2;
                if (s_reconnect_delay_ms > MF_MQTT_RECONNECT_MAX_MS) {
                    s_reconnect_delay_ms = MF_MQTT_RECONNECT_MAX_MS;
                }
            }
            return;
        }
    }

    if (!s_was_connected && s_mqtt.connected()) {
        flush_alert_queue();
    }
    s_was_connected = s_mqtt.connected();

    s_mqtt.loop();
    publish_telemetry();
}

bool mf_mqtt_connected() {
    return s_mqtt.connected();
}

void mf_mqtt_alert_publish(const char *severity, const char *code, const char *detail) {
    if (!severity) {
        severity = "WARN";
    }
    if (!code) {
        code = "ALERT";
    }
    if (!detail) {
        detail = "";
    }

    if (s_mqtt.connected()) {
        StaticJsonDocument<256> doc;
        doc["severity"] = severity;
        doc["code"] = code;
        doc["detail"] = detail;
        doc["ts"] = mf_clock_unix_seconds();
        build_topics();
        char out[256];
        if (serializeJson(doc, out, sizeof(out)) > 0) {
            s_mqtt.publish(s_topic_alert, out, false);
        }
        return;
    }

    for (uint32_t i = 0; i < MF_MQTT_ALERT_QUEUE_SIZE; ++i) {
        if (s_alert_queue[i].used) {
            continue;
        }
        strncpy(s_alert_queue[i].severity, severity, sizeof(s_alert_queue[i].severity) - 1);
        strncpy(s_alert_queue[i].code, code, sizeof(s_alert_queue[i].code) - 1);
        strncpy(s_alert_queue[i].detail, detail, sizeof(s_alert_queue[i].detail) - 1);
        s_alert_queue[i].severity[11] = '\0';
        s_alert_queue[i].code[31] = '\0';
        s_alert_queue[i].detail[63] = '\0';
        s_alert_queue[i].used = true;
        return;
    }
    mf_log_warn("mqtt", "alert queue full, dropped %s", code);
}

#else

void mf_mqtt_init() {}
void mf_mqtt_poll() {}
bool mf_mqtt_connected() { return false; }
void mf_mqtt_alert_publish(const char *, const char *, const char *) {}

#endif
