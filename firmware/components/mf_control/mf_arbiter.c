#include "mf_arbiter.h"
#include "mf_sensor_scd41.h"
#include "esp_log.h"

static const char *TAG = "mf_arbiter";

void mf_arbiter_log_compact_trace(void)
{
    int64_t stale_ms = -1;
    (void)mf_sensor_scd41_ok(&stale_ms);
    ESP_LOGI(TAG,
             "compact co2=%u rh=%.1f T=%.1f stale_ms=%lld reason=ARB_STUB",
             (unsigned)mf_sensor_scd41_co2_ppm(),
             mf_sensor_scd41_rh_percent(),
             mf_sensor_scd41_temp_c(),
             (long long)stale_ms);
}
