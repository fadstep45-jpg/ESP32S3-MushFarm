#include "mf_recipe.h"
#include "mf_fsm.h"
#include "mf_log.h"
#include "mf_clock.h"
#include <string.h>

static const char *k_embedded_demo_id = "embedded_demo";

typedef struct {
    const char *id;
    uint32_t duration_s;
    float rh_target;
} mf_recipe_stage_def_t;

static const mf_recipe_stage_def_t k_demo_stages[] = {
    {"S0", 120u, 88.0f},
    {"S1", 300u, 92.0f},
};

static char s_selected[128] = {0};
static float s_rh_target = 90.0f;
static int64_t s_stage_elapsed_s = 0;
static uint8_t s_stage_idx = 0;
static bool s_timer_frozen = false;
static uint32_t s_last_tick_ms = 0;

static bool is_demo_recipe() {
    return strcmp(s_selected, k_embedded_demo_id) == 0;
}

static uint8_t stage_count() {
    return is_demo_recipe() ? (uint8_t)(sizeof(k_demo_stages) / sizeof(k_demo_stages[0])) : 1u;
}

static void apply_stage_index(uint8_t idx) {
    uint8_t n = stage_count();
    if (idx >= n) {
        idx = (uint8_t)(n - 1u);
    }
    s_stage_idx = idx;
    if (is_demo_recipe()) {
        s_rh_target = k_demo_stages[idx].rh_target;
    } else if (s_selected[0] != '\0') {
        s_rh_target = 90.0f;
    }
}

static int stage_index_from_id(const char *stage_id) {
    if (!stage_id || stage_id[0] == '\0') {
        return 0;
    }
    if (is_demo_recipe()) {
        for (uint8_t i = 0; i < (uint8_t)(sizeof(k_demo_stages) / sizeof(k_demo_stages[0])); ++i) {
            if (strcmp(stage_id, k_demo_stages[i].id) == 0) {
                return (int)i;
            }
        }
    }
    return 0;
}

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
    s_stage_elapsed_s = 0;
    s_timer_frozen = false;
    s_last_tick_ms = mf_clock_millis();
    apply_stage_index(0);
    mf_log_info("recipe", "runtime snapshot id=%s stage=%s rh_target=%.1f%%",
                s_selected, mf_recipe_current_stage_id(), s_rh_target);
}

void mf_recipe_restore_stage_timer(int64_t elapsed_s) {
    if (elapsed_s < 0) elapsed_s = 0;
    s_stage_elapsed_s = elapsed_s;
    mf_log_info("recipe", "restored stage timer elapsed=%llds", (long long)s_stage_elapsed_s);
}

void mf_recipe_apply_checkpoint(const char *stage_id, int64_t stage_elapsed_s) {
    if (stage_elapsed_s < 0) stage_elapsed_s = 0;
    s_stage_elapsed_s = stage_elapsed_s;
    apply_stage_index((uint8_t)stage_index_from_id(stage_id));
    s_last_tick_ms = mf_clock_millis();
    mf_log_info("recipe", "checkpoint applied stage=%s elapsed=%llds rh=%.1f%%",
                mf_recipe_current_stage_id(), (long long)s_stage_elapsed_s, s_rh_target);
}

float mf_recipe_rh_target_percent() {
    return s_rh_target;
}

int64_t mf_recipe_stage_elapsed_seconds() {
    return s_stage_elapsed_s;
}

const char *mf_recipe_current_stage_id() {
    if (is_demo_recipe()) {
        return k_demo_stages[s_stage_idx].id;
    }
    return "S0";
}

void mf_recipe_runtime_set_timer_frozen(bool frozen) {
    s_timer_frozen = frozen;
    if (frozen) {
        mf_log_info("recipe", "stage timer frozen (pause)");
    } else {
        s_last_tick_ms = mf_clock_millis();
        mf_log_info("recipe", "stage timer running (resume)");
    }
}

void mf_recipe_runtime_abort() {
    s_timer_frozen = false;
    s_last_tick_ms = 0;
    mf_log_info("recipe", "runtime aborted");
}

void mf_recipe_runtime_tick() {
    mf_runtime_state_t st = mf_fsm_state();
    if (st != MF_STATE_ACTIVE_RUN && st != MF_STATE_DEGRADED_RUN) {
        return;
    }
    if (s_timer_frozen) {
        return;
    }
    uint32_t now = mf_clock_millis();
    if (s_last_tick_ms == 0u) {
        s_last_tick_ms = now;
        return;
    }
    uint32_t dt = now - s_last_tick_ms;
    s_last_tick_ms = now;
    uint32_t add_s = dt / 1000u;
    if (add_s == 0u) {
        return;
    }
    s_stage_elapsed_s += (int64_t)add_s;

    if (!is_demo_recipe()) {
        return;
    }
    const uint8_t nst = (uint8_t)(sizeof(k_demo_stages) / sizeof(k_demo_stages[0]));
    for (;;) {
        uint32_t dur = k_demo_stages[s_stage_idx].duration_s;
        if ((uint64_t)s_stage_elapsed_s < (uint64_t)dur) {
            break;
        }
        if (s_stage_idx + 1u >= nst) {
            s_stage_elapsed_s = (int64_t)dur;
            break;
        }
        s_stage_elapsed_s -= (int64_t)dur;
        uint8_t next = (uint8_t)(s_stage_idx + 1u);
        apply_stage_index(next);
        if (!mf_fsm_stage_transition_checkpoint(k_demo_stages[next].id)) {
            mf_log_warn("recipe", "stage checkpoint failed for %s", k_demo_stages[next].id);
        }
        mf_log_info("recipe", "advanced to stage %s rh=%.1f%%", k_demo_stages[next].id, s_rh_target);
    }
}
