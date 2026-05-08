#include "mf_log_ring.h"
#include "mf_batch_logger.h"
#include "esp_log.h"

static const char *TAG = "mf_log_ring";

void mf_log_ring_push(const char *line)
{
    mf_batch_logger_push(line);
}

void mf_log_ring_dump(void)
{
    size_t flushed = mf_batch_logger_flush();
    ESP_LOGI(TAG, "flush: %u lines, sd_available=%d", (unsigned)flushed, (int)mf_batch_logger_sd_available());
}
