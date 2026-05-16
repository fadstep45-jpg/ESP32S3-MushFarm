#pragma once

#include <stdint.h>
#include <stdbool.h>

// Table-driven runtime FSM per docs/architecture/state-machine.md (S4).
// Wrappers delegate to mf_fsm_dispatch for a single code path.

enum mf_runtime_state_t {
    MF_STATE_BOOT = 0,
    MF_STATE_SETUP_AP,
    MF_STATE_IDLE_READY,
    MF_STATE_ACTIVE_RUN,
    MF_STATE_PAUSED_SAFE,
    MF_STATE_DEGRADED_RUN,
    MF_STATE_EMERGENCY_STOP,
    MF_STATE__COUNT,
};

enum mf_fsm_event_t {
    MF_EV_NONE = 0,
    MF_EV_BOOT_COMPLETE,
    MF_EV_CONFIG_MISSING,
    MF_EV_APPLY_CONFIG,
    MF_EV_SELECT_RECIPE,
    MF_EV_START_CYCLE,
    MF_EV_STOP_CYCLE,
    MF_EV_PAUSE_CYCLE,
    MF_EV_RESUME_CYCLE,
    MF_EV_FAULT_NONFATAL,
    MF_EV_FAULT_FATAL,
    MF_EV_RECOVERY_VALIDATED,
    MF_EV_EMERGENCY_STOP,
    MF_EV_EMERGENCY_ACK,
    MF_EV_SERVICE_BTN_LONG_PRESS,
    MF_EV__COUNT,
};

enum mf_fsm_result_t {
    MF_FSM_OK = 0,
    MF_FSM_NOOP,
    MF_FSM_ERR_STATE,
    MF_FSM_ERR_GUARD,
    MF_FSM_ERR_LATCHED,
};

enum mf_fsm_warn_t {
    MF_WARN_NONE         = 0,
    MF_WARN_SD_FAIL      = (1u << 0),
    MF_WARN_SCD41_FAIL   = (1u << 1),
    MF_WARN_MLX_FAIL     = (1u << 2),
    MF_WARN_WATER_FAIL   = (1u << 3),
    MF_WARN_CAMERA_FAIL  = (1u << 4),
};

// Granular non-fatal subsystem codes carried by evFaultNonFatal.
// fault-model.md / state-machine.md: each maps to a sticky warn-flag,
// drives ACTIVE_RUN -> DEGRADED_RUN, and is cleared by evRecoveryValidated.
enum mf_fsm_nonfatal_t {
    MF_NONFATAL_NONE     = 0,
    MF_NONFATAL_SD       = 1,
    MF_NONFATAL_SCD41    = 2,
    MF_NONFATAL_MLX90614 = 3,
    MF_NONFATAL_WATER    = 4,
};

enum mf_fsm_fatal_t {
    MF_FATAL_NONE = 0,
    MF_FATAL_GENERIC = 1,
    MF_FATAL_HARD_LIMIT = 2,
    MF_FATAL_POWER = 3,
};

struct mf_fsm_event_ctx_t {
    const char *recipe_id;
    const char *stage_id;
    mf_fsm_nonfatal_t nonfatal_code;
    mf_fsm_fatal_t fatal_code;
};

const char *mf_fsm_state_str(mf_runtime_state_t s);
const char *mf_fsm_event_str(mf_fsm_event_t e);
mf_runtime_state_t mf_fsm_state();
uint32_t mf_fsm_warn_flags();
bool mf_fsm_emergency_latched();

// Sticky warn-flag mutators used by drivers / supervisor when fault state
// changes outside the regular FSM event flow (e.g. driver recovers but no
// recovery event is yet warranted).
void mf_fsm_set_warn(uint32_t mask);
void mf_fsm_clear_warn(uint32_t mask);

bool mf_fsm_g_sensors_min_set();
bool mf_fsm_g_hard_limits_safe();
bool mf_fsm_g_recovery_stable();
bool mf_fsm_g_emergency_clear_allowed();

void mf_fsm_set_resume_pending(bool pending);
bool mf_fsm_resume_restore_from_nvs();

mf_fsm_result_t mf_fsm_dispatch(mf_fsm_event_t event, const mf_fsm_event_ctx_t *ctx);

void mf_fsm_boot_done_config_ok();
void mf_fsm_boot_done_config_missing();
void mf_fsm_apply_config();

void mf_fsm_fault_nonfatal(mf_fsm_nonfatal_t code);
void mf_fsm_fault_fatal(mf_fsm_fatal_t code);
void mf_fsm_recovery_validated();
void mf_fsm_emergency_stop();
void mf_fsm_emergency_ack();
void mf_fsm_service_button_long_press();

void mf_fsm_select_recipe(const char *recipe_id);
mf_fsm_result_t mf_fsm_start_cycle();
mf_fsm_result_t mf_fsm_stop_cycle();
mf_fsm_result_t mf_fsm_pause_cycle();
mf_fsm_result_t mf_fsm_resume_cycle();
bool mf_fsm_stage_transition_checkpoint(const char *stage_id);

const char *mf_fsm_selected_recipe_id();
