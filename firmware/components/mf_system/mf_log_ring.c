#include "mf_log_ring.h"
#include "esp_log.h"
#include <string.h>

#define MF_LOG_RING_CAP 32
#define MF_LOG_LINE_MAX 160

static char s_lines[MF_LOG_RING_CAP][MF_LOG_LINE_MAX];
static int s_head;
static int s_count;

void mf_log_ring_push(const char *line)
{
    if (!line) {
        return;
    }
    strncpy(s_lines[s_head], line, MF_LOG_LINE_MAX - 1);
    s_lines[s_head][MF_LOG_LINE_MAX - 1] = '\0';
    s_head = (s_head + 1) % MF_LOG_RING_CAP;
    if (s_count < MF_LOG_RING_CAP) {
        s_count++;
    }
}

void mf_log_ring_dump(void)
{
    ESP_LOGI("mf_log_ring", "capacity=%d used=%d (Sprint 8: flush to SD)", MF_LOG_RING_CAP, s_count);
}
