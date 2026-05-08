#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void mf_batch_logger_init(bool sd_available);
void mf_batch_logger_set_sd_available(bool available);
bool mf_batch_logger_sd_available();
void mf_batch_logger_push(const char *line);
size_t mf_batch_logger_flush();
