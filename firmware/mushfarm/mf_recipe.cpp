#include "mf_recipe.h"
#include "mf_log.h"
#include <string.h>

// Embedded demo recipe used until full YAML/JSON loading is wired in.
static const char *k_embedded_demo_id = "embedded_demo";
static const float k_embedded_demo_rh_target = 92.0f;

static char s_selected[128] = {0};
static float s_rh_target = 90.0f;
static int64_t s_stage_elapsed_s = 0;

void mf_recipe_set_selected_id(const char *recipe_id) {
    if (!recipe_id) return;
    strncpy(s_selected, recipe_id, sizeof(s_selected) - 1);
    s_selected[sizeof(s_selected) - 1] = '\0';
}

const char *mf_recipe_get_selected_id() {
    return s_selected;
}

bool mf_recipe_has_valid_selection() {
    return s_selected[0] != '\0';
}

void mf_recipe_build_runtime_snapshot() {
    // Minimal stub mirroring legacy_esp_idf/firmware/components/mf_control/mf_recipe.c.
    if (strcmp(s_selected, k_embedded_demo_id) == 0) {
        s_rh_target = k_embedded_demo_rh_target;
    }
    s_stage_elapsed_s = 0;
    mf_log_info("recipe", "runtime snapshot id=%s rh_target=%.1f%%", s_selected, s_rh_target);
}

void mf_recipe_restore_stage_timer(int64_t elapsed_s) {
    if (elapsed_s < 0) elapsed_s = 0;
    s_stage_elapsed_s = elapsed_s;
    mf_log_info("recipe", "restored stage timer elapsed=%llds", (long long)s_stage_elapsed_s);
}

float mf_recipe_rh_target_percent() { return s_rh_target; }

int64_t mf_recipe_stage_elapsed_seconds() { return s_stage_elapsed_s; }
