#include "mf_fsm.h"
#include "mf_log.h"
#include "mf_clock.h"
#include "mf_recipe.h"
#include "mf_nvs_session.h"
#include "mf_actuators.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"

static mf_runtime_state_t s_state = MF_STATE_BOOT;
static bool s_emergency_latch = false;
static bool s_resume_pending = false;
static uint32_t s_warn_flags = 0;

const char *mf_fsm_state_str(mf_runtime_state_t s) {
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

mf_runtime_state_t mf_fsm_state() { return s_state; }
uint32_t mf_fsm_warn_flags() { return s_warn_flags; }

bool mf_fsm_g_sensors_min_set() {
    int64_t age = 0;
    return mf_scd41_ok(&age) && mf_mlx90614_ok(&age) && mf_water_ok(&age);
}

bool mf_fsm_g_hard_limits_safe() {
    return mf_scd41_co2_ppm() < 8000.0f;
}

static bool guards_for_start() {
    return mf_recipe_has_valid_selection() && mf_fsm_g_sensors_min_set() && mf_fsm_g_hard_limits_safe();
}

void mf_fsm_set_resume_pending(bool pending) {
    s_resume_pending = pending;
}

bool mf_fsm_resume_restore_from_nvs() {
    mf_session_snapshot_t snap = {};
    if (!mf_session_load(&snap)) {
        s_resume_pending = false;
        return false;
    }
    mf_recipe_set_selected_id(snap.recipe_id);
    s_resume_pending = true;

    int64_t now_unix = mf_clock_unix_seconds();
    int64_t elapsed = (now_unix > snap.stage_started_unix_s && snap.stage_started_unix_s > 0)
                          ? (now_unix - snap.stage_started_unix_s)
                          : 0;
    mf_recipe_apply_checkpoint(snap.stage_id, elapsed);
    mf_log_info("fsm", "resume pending recipe=%s stage=%s elapsed=%llds",
                snap.recipe_id, snap.stage_id, (long long)elapsed);
    return true;
}

void mf_fsm_boot_done_config_ok() {
    if (s_state != MF_STATE_BOOT) return;
    if (s_resume_pending && mf_fsm_g_sensors_min_set()) {
        s_state = MF_STATE_ACTIVE_RUN;
    } else if (s_resume_pending) {
        s_state = MF_STATE_DEGRADED_RUN;
    } else {
        s_state = MF_STATE_IDLE_READY;
    }
    mf_log_info("fsm", "-> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_boot_done_config_missing() {
    if (s_state != MF_STATE_BOOT) return;
    s_state = MF_STATE_SETUP_AP;
    mf_log_info("fsm", "-> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_fault_nonfatal(mf_fsm_nonfatal_t code) {
    if (code == MF_NONFATAL_SD) {
        s_warn_flags |= MF_WARN_SD_FAIL;
    }
    if (s_state == MF_STATE_ACTIVE_RUN || s_state == MF_STATE_PAUSED_SAFE) {
        s_state = MF_STATE_DEGRADED_RUN;
    }
    mf_log_warn("fsm", "non-fatal code=%d warn=0x%08lx state=%s",
                (int)code, (unsigned long)s_warn_flags, mf_fsm_state_str(s_state));
}

void mf_fsm_emergency_stop() {
    mf_session_clear();
    mf_recipe_runtime_abort();
    mf_actuators_all_off();
    s_emergency_latch = true;
    s_state = MF_STATE_EMERGENCY_STOP;
    mf_log_warn("fsm", "-> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_emergency_ack() {
    if (s_state != MF_STATE_EMERGENCY_STOP) return;
    if (!mf_fsm_g_hard_limits_safe()) {
        mf_log_warn("fsm", "ack rejected: hard limits not safe");
        return;
    }
    s_emergency_latch = false;
    mf_session_clear();
    mf_actuators_all_off();
    s_state = MF_STATE_IDLE_READY;
    mf_log_info("fsm", "emergency ack -> %s", mf_fsm_state_str(s_state));
}

void mf_fsm_select_recipe(const char *recipe_id) {
    if (!recipe_id) return;
    mf_recipe_set_selected_id(recipe_id);
    if (s_state == MF_STATE_IDLE_READY || s_state == MF_STATE_SETUP_AP) {
        mf_log_info("fsm", "selected recipe_id=%s", mf_recipe_get_selected_id());
    }
}

mf_fsm_result_t mf_fsm_start_cycle() {
    if (s_state == MF_STATE_EMERGENCY_STOP || s_emergency_latch) return MF_FSM_ERR_STATE;
    if (s_state == MF_STATE_ACTIVE_RUN) return MF_FSM_NOOP;
    if (s_state != MF_STATE_IDLE_READY && s_state != MF_STATE_DEGRADED_RUN) return MF_FSM_ERR_STATE;
    if (!guards_for_start()) return MF_FSM_ERR_GUARD;

    mf_recipe_build_runtime_snapshot();
    if (!mf_session_save("S0")) {
        mf_log_warn("fsm", "session checkpoint failed at start");
    }
    s_state = MF_STATE_ACTIVE_RUN;
    mf_log_info("fsm", "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_OK;
}

mf_fsm_result_t mf_fsm_stop_cycle() {
    if (s_state == MF_STATE_IDLE_READY) return MF_FSM_NOOP;
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN && s_state != MF_STATE_PAUSED_SAFE) {
        return MF_FSM_ERR_STATE;
    }
    mf_recipe_runtime_abort();
    mf_session_clear();
    s_state = MF_STATE_IDLE_READY;
    mf_log_info("fsm", "stop -> %s", mf_fsm_state_str(s_state));
    return MF_FSM_OK;
}

mf_fsm_result_t mf_fsm_pause_cycle() {
    if (s_state == MF_STATE_PAUSED_SAFE) return MF_FSM_NOOP;
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN) return MF_FSM_ERR_STATE;
    mf_recipe_runtime_set_timer_frozen(true);
    s_state = MF_STATE_PAUSED_SAFE;
    mf_log_info("fsm", "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_OK;
}

mf_fsm_result_t mf_fsm_resume_cycle() {
    if (s_state == MF_STATE_ACTIVE_RUN) return MF_FSM_NOOP;
    if (s_state != MF_STATE_PAUSED_SAFE) return MF_FSM_ERR_STATE;
    if (!mf_fsm_g_hard_limits_safe()) return MF_FSM_ERR_GUARD;
    mf_recipe_runtime_set_timer_frozen(false);
    s_state = MF_STATE_ACTIVE_RUN;
    mf_log_info("fsm", "-> %s", mf_fsm_state_str(s_state));
    return MF_FSM_OK;
}

bool mf_fsm_stage_transition_checkpoint(const char *stage_id) {
    if (!stage_id || stage_id[0] == '\0') return false;
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN) return false;
    return mf_session_save(stage_id);
}

const char *mf_fsm_selected_recipe_id() {
    return mf_recipe_get_selected_id();
}
