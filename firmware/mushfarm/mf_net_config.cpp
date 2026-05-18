#include "mf_net_config.h"
#include "mf_log.h"
#include <Preferences.h>
#include <string.h>

static const char *PREF_NS = "mf_net";
static const char *KEY_SSID = "ssid";
static const char *KEY_PASS = "pass";
static const char *KEY_CFG = "configured";

static char s_ssid[MF_NET_SSID_MAX + 1];
static char s_pass[MF_NET_PASS_MAX + 1];
static bool s_configured = false;

void mf_net_config_load() {
    Preferences p;
    s_ssid[0] = '\0';
    s_pass[0] = '\0';
    s_configured = false;
    if (!p.begin(PREF_NS, true)) {
        mf_log_warn("net_cfg", "NVS open failed");
        return;
    }
    s_configured = p.getBool(KEY_CFG, false);
    if (s_configured) {
        String ssid = p.getString(KEY_SSID, "");
        String pass = p.getString(KEY_PASS, "");
        strncpy(s_ssid, ssid.c_str(), MF_NET_SSID_MAX);
        s_ssid[MF_NET_SSID_MAX] = '\0';
        strncpy(s_pass, pass.c_str(), MF_NET_PASS_MAX);
        s_pass[MF_NET_PASS_MAX] = '\0';
        if (s_ssid[0] == '\0') {
            s_configured = false;
        }
    }
    p.end();
    mf_log_info("net_cfg", "loaded configured=%d ssid=%s",
                (int)s_configured, s_configured ? s_ssid : "(none)");
}

bool mf_net_config_is_configured() {
    return s_configured;
}

const char *mf_net_config_ssid() {
    return s_ssid;
}

const char *mf_net_config_password() {
    return s_pass;
}

bool mf_net_config_save(const char *ssid, const char *password) {
    if (!ssid || ssid[0] == '\0') {
        return false;
    }
    size_t sl = strlen(ssid);
    if (sl == 0 || sl > MF_NET_SSID_MAX) {
        return false;
    }
    if (password && strlen(password) > MF_NET_PASS_MAX) {
        return false;
    }

    Preferences p;
    if (!p.begin(PREF_NS, false)) {
        return false;
    }
    size_t n1 = p.putString(KEY_SSID, ssid);
    size_t n2 = p.putString(KEY_PASS, password ? password : "");
    bool ok = (n1 > 0) && p.putBool(KEY_CFG, true);
    (void)n2;
    p.end();
    if (!ok) {
        mf_log_warn("net_cfg", "save failed");
        return false;
    }

    strncpy(s_ssid, ssid, MF_NET_SSID_MAX);
    s_ssid[MF_NET_SSID_MAX] = '\0';
    strncpy(s_pass, password ? password : "", MF_NET_PASS_MAX);
    s_pass[MF_NET_PASS_MAX] = '\0';
    s_configured = true;
    mf_log_info("net_cfg", "saved ssid=%s", s_ssid);
    return true;
}

void mf_net_config_clear() {
    Preferences p;
    if (p.begin(PREF_NS, false)) {
        p.clear();
        p.end();
    }
    s_ssid[0] = '\0';
    s_pass[0] = '\0';
    s_configured = false;
    mf_log_info("net_cfg", "cleared");
}
