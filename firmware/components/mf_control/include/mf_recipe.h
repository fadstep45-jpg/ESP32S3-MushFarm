#pragma once

#include <stdbool.h>

void mf_recipe_set_selected_id(const char *recipe_id);
const char *mf_recipe_get_selected_id(void);
bool mf_recipe_has_valid_selection(void);
void mf_recipe_build_runtime_snapshot(void);
float mf_recipe_rh_target_percent(void);
