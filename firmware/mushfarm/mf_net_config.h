#pragma once

#include <stdbool.h>

#define MF_NET_SSID_MAX 32
#define MF_NET_PASS_MAX 64

void mf_net_config_load();
bool mf_net_config_is_configured();
const char *mf_net_config_ssid();
const char *mf_net_config_password();

bool mf_net_config_save(const char *ssid, const char *password);
void mf_net_config_clear();
