#include "mf_fsm.h"
#include "mf_recipe.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include <time.h>
#include <string.h>

static const char *TAG = "mf_fsm";

static mf_runtime_state_t s_state = MF_STATE_BOOT;
static bool s_emergency_latch;
static bool s_resume_pending;
static uint32_t s_warn_flags;

#define MF_FSM_NVS_NAMESPACE "mf_fsm"
#define MF_FSM_NVS_KEY_ACTIVE "sess_active"
#define MF_FSM_NVS_KEY_SNAPSHOT "sess_blob"
#define MF_FSM_SESSION_MAGIC 0x4D46534Du
#define MF_FSM_SESSION_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    char recipe_id[64];
    char stage_id[16];
    int64_t stage_started_unix_s;
    int64_t stage_started_monotonic_us;
} mf_fsm_session_snapshot_t;

static mf_fsm_session_snapshot_t s_resume_snapshot;

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

static esp_err_t session_nvs_open_rw(nvs_handle_t *out)
{
    return nvs_open(MF_FSM_NVS_NAMESPACE, NVS_READWRITE, out);
}

static esp_err_t session_clear_nvs(void)
{
    nvs_handle_t h;
    esp_err_t err = session_nvs_open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    (void)nvs_erase_key(h, MF_FSM_NVS_KEY_ACTIVE);
    (void)nvs_erase_key(h, MF_FSM_NVS_KEY_SNAPSHOT);
    err = nvs_commit(h);
    nvs_close(h);
    s_resume_pending = false;
    memset(&s_resume_snapshot, 0, sizeof(s_resume_snapshot));
    return err;
}

static esp_err_t session_save_checkpoint(const char *stage_id)
{
    nvs_handle_t h;
    esp_err_t err = session_nvs_open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }

    mf_fsm_session_snapshot_t snap = {
        .magic = MF_FSM_SESSION_MAGIC,
        .version = MF_FSM_SESSION_VERSION,
        .stage_started_unix_s = (int64_t)time(NULL),
        .stage_started_monotonic_us = esp_timer_get_time(),
    };
    strncpy(snap.recipe_id, mf_recipe_get_selected_id(), sizeof(snap.recipe_id) - 1);
    strncpy(snap.stage_id, stage_id ? stage_id : "S0", sizeof(snap.stage_id) - 1);

    err = nvs_set_blob(h, MF_FSM_NVS_KEY_SNAPSHOT, &snap, sizeof(snap));
    if (err == ESP_OK) {
        err = nvs_set_u8(h, MF_FSM_NVS_KEY_ACTIVE, 1u);
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    if (err == ESP_OK) {
        s_resume_snapshot = snap;
    }
    return err;
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

esp_err_t mf_fsm_resume_restore_from_nvs(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MF_FSM_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        s_resume_pending = false;
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    uint8_t active = 0;
    err = nvs_get_u8(h, MF_FSM_NVS_KEY_ACTIVE, &active);
    if (err == ESP_ERR_NVS_NOT_FOUND || active == 0) {
        nvs_close(h);
        s_resume_pending = false;
        return ESP_OK;
    }
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    mf_fsm_session_snapshot_t snap = {0};
    size_t sz = sizeof(snap);
    err = nvs_get_blob(h, MF_FSM_NVS_KEY_SNAPSHOT, &snap, &sz);
    nvs_close(h);
    if (err != ESP_OK) {
        return err;
    }
    if (sz != sizeof(snap) || snap.magic != MF_FSM_SESSION_MAGIC
        || snap.version != MF_FSM_SESSION_VERSION || snap.recipe_id[0] == '\0') {
        return ESP_ERR_INVALID_RESPONSE;
    }

    mf_recipe_set_selected_id(snap.recipe_id);
    s_resume_snapshot = snap;
    s_resume_pending = true;

    int64_t now_unix = (int64_t)time(NULL);
    int64_t elapsed_s = (now_unix > snap.stage_started_unix_s && snap.stage_started_unix_s > 0)
                            ? (now_unix - snap.stage_started_unix_s)
                            : 0;
    mf_recipe_restore_stage_timer(elapsed_s);
    ESP_LOGI(TAG,
             "resume pending from NVS: recipe=%s stage=%s elapsed=%llds",
             snap.recipe_id,
             snap.stage_id,
             (long long)elapsed_s);
    return ESP_OK;
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
    esp_err_t err = session_clear_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "session clear on emergency_stop failed: %s", esp_err_to_name(err));
    }
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
    esp_err_t err = session_clear_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "session clear on emergency_ack failed: %s", esp_err_to_name(err));
    }
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
    esp_err_t err = session_save_checkpoint("S0");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "session checkpoint start failed: %s", esp_err_to_name(err));
    }
    s_state = MF_STATE_ACTIVE_RUN;
    ESP_LOGI(TAG, "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_RES_OK;
}

mf_fsm_result_t mf_fsm_stop_cycle(void)
{
    if (s_state == MF_STATE_IDLE_READY) {
        return MF_FSM_RES_NOOP;
    }
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN && s_state != MF_STATE_PAUSED_SAFE) {
        return MF_FSM_RES_ERR_STATE;
    }
    esp_err_t err = session_clear_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "session clear on stop failed: %s", esp_err_to_name(err));
    }
    s_state = MF_STATE_IDLE_READY;
    ESP_LOGI(TAG, "stop cycle -> %s", mf_fsm_state_str(s_state));
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

esp_err_t mf_fsm_stage_transition_checkpoint(const char *stage_id)
{
    if (!stage_id || stage_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN) {
        return ESP_ERR_INVALID_STATE;
    }
    return session_save_checkpoint(stage_id);
}

const char *mf_fsm_selected_recipe_id(void)
{
    return mf_recipe_get_selected_id();
}
