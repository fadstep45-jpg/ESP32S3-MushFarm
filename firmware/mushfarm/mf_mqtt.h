#pragma once

#include <stdbool.h>

void mf_mqtt_init();
void mf_mqtt_poll();
bool mf_mqtt_connected();

void mf_mqtt_alert_publish(const char *severity, const char *code, const char *detail);
