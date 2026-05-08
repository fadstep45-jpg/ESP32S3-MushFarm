#include "mf_batch_logger.h"
#include "mf_fsm.h"
#include "mf_sd_log.h"
#include "esp_log.h"
#include <string.h>

#define MF_BATCH_LOGGER_CAP 64
#define MF_BATCH_LOGGER_LINE_MAX 160

static const char *TAG = "mf_batch_logger";

static char s_lines[MF_BATCH_LOGGER_CAP][MF_BATCH_LOGGER_LINE_MAX];
static size_t s_head;
static size_t s_count;
static bool s_sd_available;
static bool s_sd_fault_reported;

void mf_batch_logger_init(bool sd_available)
{
    s_head = 0;
    s_count = 0;
    s_sd_available = sd_available;
    s_sd_fault_reported = false;
}

void mf_batch_logger_set_sd_available(bool available)
{
    s_sd_available = available;
    if (available) {
        s_sd_fault_reported = false;
    }
}

bool mf_batch_logger_sd_available(void)
{
    return s_sd_available;
}

void mf_batch_logger_push(const char *line)
{
    if (!line) {
        return;
    }

    strncpy(s_lines[s_head], line, MF_BATCH_LOGGER_LINE_MAX - 1);
    s_lines[s_head][MF_BATCH_LOGGER_LINE_MAX - 1] = '\0';
    s_head = (s_head + 1u) % MF_BATCH_LOGGER_CAP;
    if (s_count < MF_BATCH_LOGGER_CAP) {
        s_count++;
    }
}

static void mark_sd_nonfatal_once(void)
{
    if (s_sd_fault_reported) {
        return;
    }
    s_sd_fault_reported = true;
    mf_fsm_fault_nonfatal(MF_FSM_NONFATAL_SD);
}

size_t mf_batch_logger_flush(void)
{
    if (!s_count || !s_sd_available) {
        return 0;
    }

    size_t start = (s_head + MF_BATCH_LOGGER_CAP - s_count) % MF_BATCH_LOGGER_CAP;
    size_t flushed = 0;
    for (size_t i = 0; i < s_count; ++i) {
        size_t idx = (start + i) % MF_BATCH_LOGGER_CAP;
        esp_err_t err = mf_sd_log_append_line(s_lines[idx]);
        if (err != ESP_OK) {
            s_sd_available = false;
            mark_sd_nonfatal_once();
            ESP_LOGW(TAG, "SD append failed: %d; switching to RAM-only logging", (int)err);
            break;
        }
        flushed++;
    }

    if (flushed > 0) {
        s_count -= flushed;
    }
    return flushed;
}
