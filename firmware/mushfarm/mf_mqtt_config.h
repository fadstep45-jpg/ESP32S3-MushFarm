#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MF_MQTT_HOST_MAX     64
#define MF_MQTT_USER_MAX     32
#define MF_MQTT_PASS_MAX     64
#define MF_MQTT_DEVICE_ID_MAX 32

void mf_mqtt_config_load();
bool mf_mqtt_config_is_configured();
const char *mf_mqtt_config_broker_host();
uint16_t mf_mqtt_config_broker_port();
const char *mf_mqtt_config_username();
const char *mf_mqtt_config_password();
const char *mf_mqtt_config_device_id();

bool mf_mqtt_config_save(const char *host, uint16_t port, const char *user,
                         const char *password, const char *device_id);
void mf_mqtt_config_clear();

/** Default mushfarm-XXXXXX from STA MAC; call after WiFi is available. */
void mf_mqtt_config_ensure_device_id();
