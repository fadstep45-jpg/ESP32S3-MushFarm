#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MF_STATE_BOOT = 0,
    MF_STATE_SETUP_AP,
    MF_STATE_IDLE_READY,
    MF_STATE_ACTIVE_RUN,
    MF_STATE_PAUSED_SAFE,
    MF_STATE_DEGRADED_RUN,
    MF_STATE_EMERGENCY_STOP,
} mf_runtime_state_t;

typedef enum {
    MF_FSM_RES_OK = 0,
    MF_FSM_RES_NOOP,
    MF_FSM_RES_ERR_STATE,
    MF_FSM_RES_ERR_GUARD,
} mf_fsm_result_t;

const char *mf_fsm_state_str(mf_runtime_state_t s);

void mf_fsm_boot_done_config_ok(void);
void mf_fsm_boot_done_config_missing(void);
void mf_fsm_emergency_stop(void);
void mf_fsm_emergency_ack(void);
void mf_fsm_select_recipe(const char *recipe_id);
mf_fsm_result_t mf_fsm_start_cycle(void);
mf_fsm_result_t mf_fsm_pause_cycle(void);
mf_fsm_result_t mf_fsm_resume_cycle(void);

mf_runtime_state_t mf_fsm_state(void);
const char *mf_fsm_selected_recipe_id(void);
bool mf_fsm_g_sensors_min_set(void);
bool mf_fsm_g_hard_limits_safe(void);
