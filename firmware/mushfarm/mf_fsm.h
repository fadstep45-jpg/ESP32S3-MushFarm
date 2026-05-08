#pragma once

#include <stdint.h>
#include <stdbool.h>

enum mf_runtime_state_t {
    MF_STATE_BOOT = 0,
    MF_STATE_SETUP_AP,
    MF_STATE_IDLE_READY,
    MF_STATE_ACTIVE_RUN,
    MF_STATE_PAUSED_SAFE,
    MF_STATE_DEGRADED_RUN,
    MF_STATE_EMERGENCY_STOP,
};

enum mf_fsm_result_t {
    MF_FSM_OK = 0,
    MF_FSM_NOOP,
    MF_FSM_ERR_STATE,
    MF_FSM_ERR_GUARD,
};

enum mf_fsm_warn_t {
    MF_WARN_NONE = 0,
    MF_WARN_SD_FAIL = (1u << 0),
};

enum mf_fsm_nonfatal_t {
    MF_NONFATAL_NONE = 0,
    MF_NONFATAL_SD = 1,
};

const char *mf_fsm_state_str(mf_runtime_state_t s);
mf_runtime_state_t mf_fsm_state();
uint32_t mf_fsm_warn_flags();

bool mf_fsm_g_sensors_min_set();
bool mf_fsm_g_hard_limits_safe();

void mf_fsm_set_resume_pending(bool pending);
bool mf_fsm_resume_restore_from_nvs();
void mf_fsm_boot_done_config_ok();
void mf_fsm_boot_done_config_missing();

void mf_fsm_fault_nonfatal(mf_fsm_nonfatal_t code);
void mf_fsm_emergency_stop();
void mf_fsm_emergency_ack();

void mf_fsm_select_recipe(const char *recipe_id);
mf_fsm_result_t mf_fsm_start_cycle();
mf_fsm_result_t mf_fsm_stop_cycle();
mf_fsm_result_t mf_fsm_pause_cycle();
mf_fsm_result_t mf_fsm_resume_cycle();
bool mf_fsm_stage_transition_checkpoint(const char *stage_id);

const char *mf_fsm_selected_recipe_id();
