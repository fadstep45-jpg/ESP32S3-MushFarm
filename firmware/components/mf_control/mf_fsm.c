#include "mf_fsm.h"
#include "mf_recipe.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mf_fsm";

static mf_runtime_state_t s_state = MF_STATE_BOOT;
static bool s_emergency_latch;
static bool s_resume_pending;
static uint32_t s_warn_flags;

const char *mf_fsm_state_str(mf_runtime_state_t s)
{
    switch (s) {
    case MF_STATE_BOOT: return "BOOT";
    case MF_STATE_SETUP_AP: return "SETUP_AP";
    case MF_STATE_IDLE_READY: return "IDLE_READY";
    case MF_STATE_ACTIVE_RUN: return "ACTIVE_RUN";
    case MF_STATE_PAUSED_SAFE: return "PAUSED_SAFE";
    case MF_STATE_DEGRADED_RUN: return "DEGRADED_RUN";
    case MF_STATE_EMERGENCY_STOP: return "EMERGENCY_STOP";
    default: return "UNKNOWN";
    }
}

mf_runtime_state_t mf_fsm_state(void)
{
    return s_state;
}

static bool guards_for_start(void)
{
    return mf_recipe_has_valid_selection()
           && mf_fsm_g_sensors_min_set()
           && mf_fsm_g_hard_limits_safe();
}

bool mf_fsm_g_sensors_min_set(void)
{
    int64_t st;
    return mf_sensor_scd41_ok(&st) && mf_sensor_mlx90614_ok(&st) && mf_sensor_water_ok(&st);
}

bool mf_fsm_g_hard_limits_safe(void)
{
    /* Sprint 5: tie to safety_profile thresholds; placeholder always true */
    return mf_sensor_scd41_co2_ppm() < 8000;
}

uint32_t mf_fsm_warn_flags(void)
{
    return s_warn_flags;
}

void mf_fsm_set_resume_pending(bool pending)
{
    s_resume_pending = pending;
}

void mf_fsm_boot_done_config_ok(void)
{
    if (s_state == MF_STATE_BOOT) {
        if (s_resume_pending && mf_fsm_g_sensors_min_set()) {
            s_state = MF_STATE_ACTIVE_RUN;
        } else if (s_resume_pending) {
            s_state = MF_STATE_DEGRADED_RUN;
        } else {
            s_state = MF_STATE_IDLE_READY;
        }
        ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    }
}

void mf_fsm_boot_done_config_missing(void)
{
    if (s_state == MF_STATE_BOOT) {
        s_state = MF_STATE_SETUP_AP;
        ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    }
}

void mf_fsm_fault_nonfatal(mf_fsm_nonfatal_code_t code)
{
    if (code == MF_FSM_NONFATAL_SD) {
        s_warn_flags |= MF_FSM_WARN_SD_FAIL;
    }

    if (s_state == MF_STATE_ACTIVE_RUN || s_state == MF_STATE_PAUSED_SAFE) {
        s_state = MF_STATE_DEGRADED_RUN;
    }

    ESP_LOGW(TAG,
             "non-fatal fault code=%d warn=0x%08lx state=%s",
             (int)code,
             (unsigned long)s_warn_flags,
             mf_fsm_state_str(s_state));
}

void mf_fsm_emergency_stop(void)
{
    s_emergency_latch = true;
    s_state = MF_STATE_EMERGENCY_STOP;
    ESP_LOGW(TAG, "-> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_emergency_ack(void)
{
    if (s_state != MF_STATE_EMERGENCY_STOP) {
        return;
    }
    if (!mf_fsm_g_hard_limits_safe()) {
        ESP_LOGW(TAG, "ack rejected: hard limits not safe");
        return;
    }
    s_emergency_latch = false;
    s_state = MF_STATE_IDLE_READY;
    ESP_LOGI(TAG, "emergency ack -> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_select_recipe(const char *recipe_id)
{
    if (!recipe_id) {
        return;
    }
    mf_recipe_set_selected_id(recipe_id);
    if (s_state == MF_STATE_IDLE_READY || s_state == MF_STATE_SETUP_AP) {
        ESP_LOGI(TAG, "selected recipe_id=%s", mf_recipe_get_selected_id());
    }
}

mf_fsm_result_t mf_fsm_start_cycle(void)
{
    if (s_state == MF_STATE_EMERGENCY_STOP || s_emergency_latch) {
        return MF_FSM_RES_ERR_STATE;
    }
    if (s_state == MF_STATE_ACTIVE_RUN) {
        return MF_FSM_RES_NOOP;
    }
    if (s_state != MF_STATE_IDLE_READY && s_state != MF_STATE_DEGRADED_RUN) {
        return MF_FSM_RES_ERR_STATE;
    }
    if (!guards_for_start()) {
        return MF_FSM_RES_ERR_GUARD;
    }
    mf_recipe_build_runtime_snapshot();
    s_state = MF_STATE_ACTIVE_RUN;
    ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_RES_OK;
}

mf_fsm_result_t mf_fsm_pause_cycle(void)
{
    if (s_state == MF_STATE_PAUSED_SAFE) {
        return MF_FSM_RES_NOOP;
    }
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN) {
        return MF_FSM_RES_ERR_STATE;
    }
    s_state = MF_STATE_PAUSED_SAFE;
    ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_RES_OK;
}

mf_fsm_result_t mf_fsm_resume_cycle(void)
{
    if (s_state == MF_STATE_ACTIVE_RUN) {
        return MF_FSM_RES_NOOP;
    }
    if (s_state != MF_STATE_PAUSED_SAFE) {
        return MF_FSM_RES_ERR_STATE;
    }
    if (!mf_fsm_g_hard_limits_safe()) {
        return MF_FSM_RES_ERR_GUARD;
    }
    s_state = MF_STATE_ACTIVE_RUN;
    ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_RES_OK;
}

const char *mf_fsm_selected_recipe_id(void)
{
    return mf_recipe_get_selected_id();
}
