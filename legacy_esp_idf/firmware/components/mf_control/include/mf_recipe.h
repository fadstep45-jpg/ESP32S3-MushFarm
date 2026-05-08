#pragma once

#include <stdbool.h>
#include <stdint.h>

void mf_recipe_set_selected_id(const char *recipe_id);
const char *mf_recipe_get_selected_id(void);
bool mf_recipe_has_valid_selection(void);
void mf_recipe_build_runtime_snapshot(void);
void mf_recipe_restore_stage_timer(int64_t elapsed_s);
float mf_recipe_rh_target_percent(void);
