#pragma once

#include "mf_fsm.h"
#include <stdbool.h>

typedef struct {
    bool ok;
    const char *code;
    const char *detail;
} mf_cmd_result_t;

/** Map FSM dispatch result to API ack/error code. */
mf_cmd_result_t mf_cmd_from_fsm(mf_fsm_result_t r, const char *reject_detail);

mf_cmd_result_t mf_cmd_recipe_select(const char *recipe_id);
mf_cmd_result_t mf_cmd_cycle_start(const char *recipe_id_optional);
mf_cmd_result_t mf_cmd_cycle_pause();
mf_cmd_result_t mf_cmd_cycle_resume();
mf_cmd_result_t mf_cmd_cycle_stop_emergency();
mf_cmd_result_t mf_cmd_not_implemented(const char *detail);

bool mf_cmd_recipe_id_valid(const char *recipe_id);

#if defined(__cplusplus)
#include <ArduinoJson.h>
void mf_cmd_fill_recipe_list(JsonObject payload);
bool mf_cmd_fill_recipe_get(const char *recipe_id, JsonObject payload);
#endif
