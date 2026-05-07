#include "mf_recipe.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mf_recipe";

/* Minimal embedded JSON aligned with docs/recipes schema (subset for bring-up). */
static const char k_embedded_demo_json[] =
    "{\"recipe_meta\":{\"schema_version\":1,\"recipe_id\":\"embedded_demo\","
    "\"recipe_name\":\"Bring-up demo\",\"species\":\"demo\",\"version\":1},"
    "\"stages\":[{\"stage_id\":\"S0\",\"setpoints\":{\"rh_target_percent\":92.0}}]}";

static char s_selected[128];
static float s_rh_target = 90.0f;

void mf_recipe_set_selected_id(const char *recipe_id)
{
    if (!recipe_id) {
        return;
    }
    strncpy(s_selected, recipe_id, sizeof(s_selected) - 1);
    s_selected[sizeof(s_selected) - 1] = '\0';
}

const char *mf_recipe_get_selected_id(void)
{
    return s_selected;
}

bool mf_recipe_has_valid_selection(void)
{
    return s_selected[0] != '\0';
}

void mf_recipe_build_runtime_snapshot(void)
{
    cJSON *root = cJSON_Parse(k_embedded_demo_json);
    if (!root) {
        ESP_LOGE(TAG, "embedded JSON parse failed");
        return;
    }
    cJSON *stages = cJSON_GetObjectItem(root, "stages");
    cJSON *st0 = stages && cJSON_IsArray(stages) ? cJSON_GetArrayItem(stages, 0) : NULL;
    cJSON *sp = st0 ? cJSON_GetObjectItem(st0, "setpoints") : NULL;
    cJSON *rh = sp ? cJSON_GetObjectItem(sp, "rh_target_percent") : NULL;
    if (rh && cJSON_IsNumber(rh)) {
        s_rh_target = (float)rh->valuedouble;
    }
    cJSON_Delete(root);
    ESP_LOGI(TAG, "runtime snapshot rh_target=%.1f %% (embedded demo)", s_rh_target);
}

float mf_recipe_rh_target_percent(void)
{
    return s_rh_target;
}
