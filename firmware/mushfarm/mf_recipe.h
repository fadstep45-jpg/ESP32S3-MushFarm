#pragma once

#include <stdint.h>
#include <stdbool.h>

void mf_recipe_set_selected_id(const char *recipe_id);
const char *mf_recipe_get_selected_id();
bool mf_recipe_has_valid_selection();

void mf_recipe_build_runtime_snapshot();
void mf_recipe_restore_stage_timer(int64_t elapsed_s);

/** Restore stage index + RH target after NVS resume (stage_id e.g. "S0"). */
void mf_recipe_apply_checkpoint(const char *stage_id, int64_t stage_elapsed_s);

float mf_recipe_rh_target_percent();
int64_t mf_recipe_stage_elapsed_seconds();
const char *mf_recipe_current_stage_id();

/** Advance stage timers while cycle is ACTIVE/DEGRADED and not paused. */
void mf_recipe_runtime_tick();

void mf_recipe_runtime_set_timer_frozen(bool frozen);
void mf_recipe_runtime_abort();
