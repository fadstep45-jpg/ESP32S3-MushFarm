#include "mf_wifi.h"
#include "mf_config.h"
#include "mf_net_config.h"
#include "mf_fsm.h"
#include "mf_log.h"
#include "mf_clock.h"

#if MF_WIFI_SOFTAP

#include <WiFi.h>
#include <IPAddress.h>

static mf_runtime_state_t s_last_fsm = MF_STATE_BOOT;
static bool s_ap_up = false;
static bool s_sta_connecting = false;
static bool s_restart_pending = false;
static bool s_sntp_started = false;
static uint32_t s_sta_connect_started_ms = 0;
static uint32_t s_next_reconnect_ms = 0;
static char s_sta_ip[16] = "0.0.0.0";

static void start_softap() {
    if (s_ap_up) {
        return;
    }
    IPAddress ap_ip;
    ap_ip.fromString(MF_WIFI_AP_IP);
    IPAddress gateway = ap_ip;
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(MF_WIFI_AP_SSID, nullptr, 1, 0, 4)) {
        mf_log_warn("wifi", "softAP start failed");
        return;
    }
    WiFi.softAPConfig(ap_ip, gateway, subnet);
    s_ap_up = true;
    mf_log_info("wifi", "softAP %s @ %s", MF_WIFI_AP_SSID, MF_WIFI_AP_IP);
}

static void stop_softap() {
    if (!s_ap_up) {
        return;
    }
    WiFi.softAPdisconnect(true);
    s_ap_up = false;
    mf_log_info("wifi", "softAP stopped");
}

static void update_sta_ip_str() {
    if (WiFi.status() == WL_CONNECTED) {
        strncpy(s_sta_ip, WiFi.localIP().toString().c_str(), sizeof(s_sta_ip) - 1);
        s_sta_ip[sizeof(s_sta_ip) - 1] = '\0';
    } else {
        strncpy(s_sta_ip, "0.0.0.0", sizeof(s_sta_ip));
    }
}

static void maybe_start_sntp() {
#if MF_CLOCK_SNTP_ENABLED
    if (s_sntp_started || WiFi.status() != WL_CONNECTED) {
        return;
    }
    s_sntp_started = true;
    mf_clock_start_sntp(MF_CLOCK_TZ_POSIX, MF_CLOCK_NTP_PRIMARY, MF_CLOCK_NTP_SECONDARY);
#endif
}

static void sta_connect_attempt() {
    if (!mf_net_config_is_configured()) {
        return;
    }
    WiFi.mode(s_ap_up ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(mf_net_config_ssid(), mf_net_config_password());
    s_sta_connecting = true;
    s_sta_connect_started_ms = millis();
    mf_log_info("wifi", "STA connecting to %s", mf_net_config_ssid());
}

void mf_wifi_init() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    s_last_fsm = mf_fsm_state();
    s_sta_ip[0] = '\0';
}

void mf_wifi_sta_begin() {
    if (!mf_net_config_is_configured()) {
        return;
    }
    sta_connect_attempt();
}

void mf_wifi_request_restart() {
    s_restart_pending = true;
}

void mf_wifi_poll() {
    mf_runtime_state_t st = mf_fsm_state();

    if (st == MF_STATE_SETUP_AP) {
        start_softap();
    } else if (s_ap_up) {
        stop_softap();
        if (WiFi.getMode() == WIFI_AP_STA) {
            WiFi.mode(WIFI_STA);
        }
    }

    if (s_restart_pending) {
        s_restart_pending = false;
        WiFi.disconnect(true);
        s_sta_connecting = false;
        s_sntp_started = false;
        delay(100);
        sta_connect_attempt();
    }

    if (st != MF_STATE_SETUP_AP && mf_net_config_is_configured()) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            s_sta_connecting = false;
            update_sta_ip_str();
            maybe_start_sntp();
        } else if (s_sta_connecting) {
            if ((millis() - s_sta_connect_started_ms) > MF_WIFI_STA_TIMEOUT_MS) {
                mf_log_warn("wifi", "STA connect timeout");
                s_sta_connecting = false;
                s_next_reconnect_ms = millis() + MF_WIFI_RECONNECT_MS;
            }
        } else if (millis() >= s_next_reconnect_ms) {
            sta_connect_attempt();
            s_next_reconnect_ms = millis() + MF_WIFI_RECONNECT_MS;
        }
    }

    s_last_fsm = st;
}

bool mf_wifi_sta_connected() {
    return WiFi.status() == WL_CONNECTED;
}

int32_t mf_wifi_sta_rssi() {
    if (!mf_wifi_sta_connected()) {
        return 0;
    }
    return WiFi.RSSI();
}

const char *mf_wifi_sta_ip_str() {
    update_sta_ip_str();
    return s_sta_ip;
}

const char *mf_wifi_mode_str() {
    if (s_ap_up && mf_wifi_sta_connected()) {
        return "AP_STA";
    }
    if (s_ap_up) {
        return "AP";
    }
    if (mf_wifi_sta_connected()) {
        return "STA";
    }
    return "OFF";
}

bool mf_wifi_softap_active() {
    return s_ap_up;
}

#else

void mf_wifi_init() {}
void mf_wifi_poll() {}
void mf_wifi_sta_begin() {}
void mf_wifi_request_restart() {}
bool mf_wifi_sta_connected() { return false; }
int32_t mf_wifi_sta_rssi() { return 0; }
const char *mf_wifi_sta_ip_str() { return "0.0.0.0"; }
const char *mf_wifi_mode_str() { return "OFF"; }
bool mf_wifi_softap_active() { return false; }

#endif
