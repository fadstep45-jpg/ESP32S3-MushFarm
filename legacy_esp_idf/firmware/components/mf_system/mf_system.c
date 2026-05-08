#include "mf_system.h"
#include "mf_batch_logger.h"
#include "mf_fsm.h"
#include "mf_log_ring.h"
#include "mf_time_sync.h"
#include "mf_sd_log.h"
#include "mf_service_btn.h"
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "mf_system";

esp_err_t mf_system_init(void)
{
    bool sd_ok = true;
#if CONFIG_MF_SD_LOG_ENABLE
    esp_err_t sd_err = mf_sd_log_mount();
    sd_ok = (sd_err == ESP_OK);
    if (!sd_ok) {
        mf_fsm_fault_nonfatal(MF_FSM_NONFATAL_SD);
    }
#endif
    mf_batch_logger_init(sd_ok);
    mf_log_ring_push("boot");
    esp_err_t resume_err = mf_fsm_resume_restore_from_nvs();
    if (resume_err != ESP_OK) {
        ESP_LOGW(TAG, "resume restore from NVS failed: %s", esp_err_to_name(resume_err));
    }
    mf_time_sync_init();
    ESP_RETURN_ON_ERROR(mf_service_btn_init(), TAG, "service btn");
    ESP_LOGI(TAG, "mf_system_init done");
    return ESP_OK;
}

void mf_system_poll(void)
{
    mf_time_sync_poll();
    mf_service_btn_poll();
    (void)mf_batch_logger_flush();
}
