#include "mf_cmd_dispatch.h"
#include "mf_api_codes.h"
#include "mf_fsm.h"
#include "mf_recipe.h"
#include <ArduinoJson.h>
#include <string.h>

static const char *k_embedded_demo = "embedded_demo";

mf_cmd_result_t mf_cmd_from_fsm(mf_fsm_result_t r, const char *reject_detail) {
    mf_cmd_result_t out = {false, MF_API_ERR_STATE, reject_detail};
    switch (r) {
    case MF_FSM_OK:
        out.ok = true;
        out.code = MF_API_ACK_OK;
        out.detail = nullptr;
        break;
    case MF_FSM_NOOP:
        out.ok = true;
        out.code = MF_API_ACK_NOOP;
        out.detail = nullptr;
        break;
    case MF_FSM_ERR_LATCHED:
        out.code = MF_API_ERR_LATCHED;
        break;
    case MF_FSM_ERR_GUARD:
    case MF_FSM_ERR_STATE:
    default:
        out.code = MF_API_ERR_STATE;
        break;
    }
    return out;
}

bool mf_cmd_recipe_id_valid(const char *recipe_id) {
    return recipe_id && strcmp(recipe_id, k_embedded_demo) == 0;
}

void mf_cmd_fill_recipe_list(JsonObject payload) {
    JsonArray items = payload.createNestedArray("items");
    JsonObject r0 = items.createNestedObject();
    r0["recipe_id"] = k_embedded_demo;
    r0["name"] = "Embedded demo (S0/S1)";
    r0["rev"] = 1;
    payload["total"] = 1;
}

bool mf_cmd_fill_recipe_get(const char *recipe_id, JsonObject payload) {
    if (!mf_cmd_recipe_id_valid(recipe_id)) {
        return false;
    }
    payload["recipe_id"] = k_embedded_demo;
    JsonArray stages = payload.createNestedArray("stages");
    JsonObject s0 = stages.createNestedObject();
    s0["id"] = "S0";
    s0["duration_s"] = 120;
    s0["rh_target"] = 88.0;
    JsonObject s1 = stages.createNestedObject();
    s1["id"] = "S1";
    s1["duration_s"] = 300;
    s1["rh_target"] = 92.0;
    return true;
}

mf_cmd_result_t mf_cmd_recipe_select(const char *recipe_id) {
    if (!recipe_id || !recipe_id[0]) {
        return {false, MF_API_ERR_SCHEMA_INVALID, "recipe_id required"};
    }
    if (!mf_cmd_recipe_id_valid(recipe_id)) {
        return {false, MF_API_ERR_RECIPE_NOT_FOUND, "Unknown recipe"};
    }
    if (mf_fsm_state() != MF_STATE_IDLE_READY) {
        return {false, MF_API_ERR_STATE, "Select recipe only in IDLE_READY"};
    }
    const char *cur = mf_fsm_selected_recipe_id();
    if (cur && strcmp(cur, recipe_id) == 0) {
        return {true, MF_API_ACK_NOOP, nullptr};
    }
    mf_fsm_select_recipe(recipe_id);
    return {true, MF_API_ACK_OK, nullptr};
}

mf_cmd_result_t mf_cmd_cycle_start(const char *recipe_id_optional) {
    if (recipe_id_optional && recipe_id_optional[0]) {
        if (!mf_cmd_recipe_id_valid(recipe_id_optional)) {
            return {false, MF_API_ERR_RECIPE_NOT_FOUND, "Unknown recipe"};
        }
        mf_fsm_select_recipe(recipe_id_optional);
    }
    mf_fsm_result_t r = mf_fsm_start_cycle();
    mf_cmd_result_t out = mf_cmd_from_fsm(r, "Cannot start cycle");
    return out;
}

mf_cmd_result_t mf_cmd_cycle_pause() {
    return mf_cmd_from_fsm(mf_fsm_pause_cycle(), "FSM rejected pause");
}

mf_cmd_result_t mf_cmd_cycle_resume() {
    return mf_cmd_from_fsm(mf_fsm_resume_cycle(), "FSM rejected resume");
}

mf_cmd_result_t mf_cmd_cycle_stop_emergency() {
    mf_fsm_emergency_stop();
    return {true, MF_API_ACK_OK, nullptr};
}

mf_cmd_result_t mf_cmd_not_implemented(const char *detail) {
    return {false, MF_API_ERR_NOT_IMPLEMENTED, detail ? detail : "Not implemented"};
}
