#include "mf_mqtt_config.h"
#include "mf_config.h"
#include "mf_log.h"

#if MF_MQTT_ENABLE

#include <Preferences.h>
#include <WiFi.h>
#include <string.h>
#include <stdio.h>

static const char *PREF_NS = "mf_mqtt";
static const char *KEY_HOST = "host";
static const char *KEY_PORT = "port";
static const char *KEY_USER = "user";
static const char *KEY_PASS = "pass";
static const char *KEY_DEVID = "devid";
static const char *KEY_CFG = "configured";

static char s_host[MF_MQTT_HOST_MAX + 1];
static char s_user[MF_MQTT_USER_MAX + 1];
static char s_pass[MF_MQTT_PASS_MAX + 1];
static char s_devid[MF_MQTT_DEVICE_ID_MAX + 1];
static uint16_t s_port = 1883;
static bool s_configured = false;

void mf_mqtt_config_load() {
    Preferences p;
    s_host[0] = '\0';
    s_user[0] = '\0';
    s_pass[0] = '\0';
    s_devid[0] = '\0';
    s_port = 1883;
    s_configured = false;
    if (!p.begin(PREF_NS, true)) {
        mf_log_warn("mqtt_cfg", "NVS open failed");
        return;
    }
    s_configured = p.getBool(KEY_CFG, false);
    if (s_configured) {
        String host = p.getString(KEY_HOST, "");
        strncpy(s_host, host.c_str(), MF_MQTT_HOST_MAX);
        s_host[MF_MQTT_HOST_MAX] = '\0';
        s_port = (uint16_t)p.getUShort(KEY_PORT, 1883);
        String user = p.getString(KEY_USER, "");
        String pass = p.getString(KEY_PASS, "");
        String dev = p.getString(KEY_DEVID, "");
        strncpy(s_user, user.c_str(), MF_MQTT_USER_MAX);
        strncpy(s_pass, pass.c_str(), MF_MQTT_PASS_MAX);
        strncpy(s_devid, dev.c_str(), MF_MQTT_DEVICE_ID_MAX);
        s_user[MF_MQTT_USER_MAX] = '\0';
        s_pass[MF_MQTT_PASS_MAX] = '\0';
        s_devid[MF_MQTT_DEVICE_ID_MAX] = '\0';
        if (s_host[0] == '\0') {
            s_configured = false;
        }
    }
    p.end();
    mf_log_info("mqtt_cfg", "loaded configured=%d host=%s port=%u dev=%s",
                (int)s_configured, s_configured ? s_host : "(none)",
                (unsigned)s_port, s_devid[0] ? s_devid : "(auto)");
}

bool mf_mqtt_config_is_configured() {
    return s_configured;
}

const char *mf_mqtt_config_broker_host() {
    return s_host;
}

uint16_t mf_mqtt_config_broker_port() {
    return s_port;
}

const char *mf_mqtt_config_username() {
    return s_user;
}

const char *mf_mqtt_config_password() {
    return s_pass;
}

const char *mf_mqtt_config_device_id() {
    return s_devid;
}

void mf_mqtt_config_ensure_device_id() {
    if (s_devid[0] != '\0') {
        return;
    }
    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    snprintf(s_devid, sizeof(s_devid), "mushfarm-%02x%02x%02x", mac[3], mac[4], mac[5]);
    s_devid[MF_MQTT_DEVICE_ID_MAX] = '\0';
}

bool mf_mqtt_config_save(const char *host, uint16_t port, const char *user,
                         const char *password, const char *device_id) {
    if (!host || host[0] == '\0') {
        return false;
    }
    if (strlen(host) > MF_MQTT_HOST_MAX) {
        return false;
    }
    if (port == 0) {
        port = 1883;
    }
    if (user && strlen(user) > MF_MQTT_USER_MAX) {
        return false;
    }
    if (password && strlen(password) > MF_MQTT_PASS_MAX) {
        return false;
    }
    if (device_id && strlen(device_id) > MF_MQTT_DEVICE_ID_MAX) {
        return false;
    }

    Preferences p;
    if (!p.begin(PREF_NS, false)) {
        return false;
    }
    bool ok = p.putString(KEY_HOST, host) > 0 && p.putUShort(KEY_PORT, port) &&
              p.putBool(KEY_CFG, true);
    p.putString(KEY_USER, user ? user : "");
    p.putString(KEY_PASS, password ? password : "");
    if (device_id && device_id[0]) {
        p.putString(KEY_DEVID, device_id);
    }
    p.end();
    if (!ok) {
        return false;
    }

    strncpy(s_host, host, MF_MQTT_HOST_MAX);
    s_host[MF_MQTT_HOST_MAX] = '\0';
    s_port = port;
    strncpy(s_user, user ? user : "", MF_MQTT_USER_MAX);
    strncpy(s_pass, password ? password : "", MF_MQTT_PASS_MAX);
    s_user[MF_MQTT_USER_MAX] = '\0';
    s_pass[MF_MQTT_PASS_MAX] = '\0';
    s_devid[0] = '\0';
    if (device_id && device_id[0]) {
        strncpy(s_devid, device_id, MF_MQTT_DEVICE_ID_MAX);
        s_devid[MF_MQTT_DEVICE_ID_MAX] = '\0';
    }
    s_configured = true;
    mf_log_info("mqtt_cfg", "saved host=%s port=%u", s_host, (unsigned)s_port);
    return true;
}

void mf_mqtt_config_clear() {
    Preferences p;
    if (p.begin(PREF_NS, false)) {
        p.clear();
        p.end();
    }
    s_host[0] = '\0';
    s_user[0] = '\0';
    s_pass[0] = '\0';
    s_devid[0] = '\0';
    s_port = 1883;
    s_configured = false;
    mf_log_info("mqtt_cfg", "cleared");
}

#else

void mf_mqtt_config_load() {}
bool mf_mqtt_config_is_configured() { return false; }
const char *mf_mqtt_config_broker_host() { return ""; }
uint16_t mf_mqtt_config_broker_port() { return 1883; }
const char *mf_mqtt_config_username() { return ""; }
const char *mf_mqtt_config_password() { return ""; }
const char *mf_mqtt_config_device_id() { return ""; }
bool mf_mqtt_config_save(const char *, uint16_t, const char *, const char *, const char *) {
    return false;
}
void mf_mqtt_config_clear() {}
void mf_mqtt_config_ensure_device_id() {}

#endif
