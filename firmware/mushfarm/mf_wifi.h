#pragma once

#include <stdbool.h>
#include <stdint.h>

void mf_wifi_init();
void mf_wifi_poll();

void mf_wifi_sta_begin();
void mf_wifi_request_restart();

bool mf_wifi_sta_connected();
int32_t mf_wifi_sta_rssi();
const char *mf_wifi_sta_ip_str();
const char *mf_wifi_mode_str();
bool mf_wifi_softap_active();
