#pragma once

#include "esp_err.h"

esp_err_t mf_sd_log_mount(void);
esp_err_t mf_sd_log_append_line(const char *line);
