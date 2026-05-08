#include "mf_batch_logger.h"
#include "mf_fsm.h"
#include "mf_config.h"
#include <string.h>

#define MF_BATCH_CAP 64
#define MF_BATCH_LINE_MAX 192

static char s_lines[MF_BATCH_CAP][MF_BATCH_LINE_MAX];
static size_t s_head = 0;
static size_t s_count = 0;
static bool s_sd_available = false;
static bool s_sd_fault_reported = false;

void mf_batch_logger_init(bool sd_available) {
    s_head = 0;
    s_count = 0;
    s_sd_available = sd_available;
    s_sd_fault_reported = false;
}

void mf_batch_logger_set_sd_available(bool available) {
    s_sd_available = available;
    if (available) s_sd_fault_reported = false;
}

bool mf_batch_logger_sd_available() {
    return s_sd_available;
}

void mf_batch_logger_push(const char *line) {
    if (!line) return;
    strncpy(s_lines[s_head], line, MF_BATCH_LINE_MAX - 1);
    s_lines[s_head][MF_BATCH_LINE_MAX - 1] = '\0';
    s_head = (s_head + 1u) % MF_BATCH_CAP;
    if (s_count < MF_BATCH_CAP) s_count++;
}

static bool sd_append_stub(const char *line) {
    (void)line;
#if MF_SD_LOG_ENABLE
    // TODO: SD.open(...).println(line) when SD wiring is hooked up.
    return false;
#else
    return false;
#endif
}

static void mark_sd_nonfatal_once() {
    if (s_sd_fault_reported) return;
    s_sd_fault_reported = true;
    mf_fsm_fault_nonfatal(MF_NONFATAL_SD);
}

size_t mf_batch_logger_flush() {
    if (!s_count || !s_sd_available) return 0;
    size_t start = (s_head + MF_BATCH_CAP - s_count) % MF_BATCH_CAP;
    size_t flushed = 0;
    for (size_t i = 0; i < s_count; ++i) {
        size_t idx = (start + i) % MF_BATCH_CAP;
        if (!sd_append_stub(s_lines[idx])) {
            s_sd_available = false;
            mark_sd_nonfatal_once();
            break;
        }
        flushed++;
    }
    if (flushed > 0) s_count -= flushed;
    return flushed;
}
