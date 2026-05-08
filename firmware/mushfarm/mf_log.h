#pragma once

#include <Arduino.h>

// Lightweight Serial-based logger. Replaces ESP_LOG* from ESP-IDF.
// Lines are also pushed to the RAM batch logger if it is initialised.

void mf_log_init(unsigned long baud_rate);

void mf_log_info(const char *tag, const char *fmt, ...);
void mf_log_warn(const char *tag, const char *fmt, ...);
void mf_log_error(const char *tag, const char *fmt, ...);
