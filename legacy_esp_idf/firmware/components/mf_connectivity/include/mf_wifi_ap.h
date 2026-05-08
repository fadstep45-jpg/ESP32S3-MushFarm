#pragma once

#include "esp_err.h"
#include "esp_netif.h"

esp_err_t mf_wifi_ap_start(void);
esp_netif_t *mf_wifi_ap_netif(void);
