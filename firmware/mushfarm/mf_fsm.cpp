#include "mf_fsm.h"
#include "mf_log.h"
#include "mf_clock.h"
#include "mf_recipe.h"
#include "mf_nvs_session.h"
#include "mf_actuators.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"

#include <stddef.h>
#include <string.h>

// =====================================================================
// Internal state
// =====================================================================

static mf_runtime_state_t s_state = MF_STATE_BOOT;
static bool s_emergency_latch = false;
static bool s_resume_pending = false;
static uint32_t s_warn_flags = 0;
static int64_t s_state_entered_ms = 0;
// Sticky flag set whenever the configuration path completed at boot. Used
// by the BOOT row that requires gConfigReady but cannot read it directly.
static bool s_config_ready = false;

// =====================================================================
// Stringification
// =====================================================================

const char *mf_fsm_state_str(mf_runtime_state_t s) {
    switch (s) {
    case MF_STATE_BOOT: return "BOOT";
    case MF_STATE_SETUP_AP: return "SETUP_AP";
    case MF_STATE_IDLE_READY: return "IDLE_READY";
    case MF_STATE_ACTIVE_RUN: return "ACTIVE_RUN";
    case MF_STATE_PAUSED_SAFE: return "PAUSED_SAFE";
    case MF_STATE_DEGRADED_RUN: return "DEGRADED_RUN";
    case MF_STATE_EMERGENCY_STOP: return "EMERGENCY_STOP";
    default: return "UNKNOWN";
    }
}

const char *mf_fsm_event_str(mf_fsm_event_t e) {
    switch (e) {
    case MF_EV_NONE: return "NONE";
    case MF_EV_BOOT_COMPLETE: return "BOOT_COMPLETE";
    case MF_EV_CONFIG_MISSING: return "CONFIG_MISSING";
    case MF_EV_APPLY_CONFIG: return "APPLY_CONFIG";
    case MF_EV_SELECT_RECIPE: return "SELECT_RECIPE";
    case MF_EV_START_CYCLE: return "START_CYCLE";
    case MF_EV_STOP_CYCLE: return "STOP_CYCLE";
    case MF_EV_PAUSE_CYCLE: return "PAUSE_CYCLE";
    case MF_EV_RESUME_CYCLE: return "RESUME_CYCLE";
    case MF_EV_FAULT_NONFATAL: return "FAULT_NONFATAL";
    case MF_EV_FAULT_FATAL: return "FAULT_FATAL";
    case MF_EV_RECOVERY_VALIDATED: return "RECOVERY_VALIDATED";
    case MF_EV_EMERGENCY_STOP: return "EMERGENCY_STOP";
    case MF_EV_EMERGENCY_ACK: return "EMERGENCY_ACK";
    case MF_EV_SERVICE_BTN_LONG_PRESS: return "SERVICE_BTN_LONG_PRESS";
    default: return "?";
    }
}

mf_runtime_state_t mf_fsm_state() { return s_state; }
uint32_t mf_fsm_warn_flags() { return s_warn_flags; }
bool mf_fsm_emergency_latched() { return s_emergency_latch; }

// =====================================================================
// Guards
// =====================================================================

bool mf_fsm_g_sensors_min_set() {
    int64_t age = 0;
    return mf_scd41_ok(&age) && mf_mlx90614_ok(&age) && mf_water_ok(&age);
}

bool mf_fsm_g_hard_limits_safe() {
    return mf_scd41_co2_ppm() < 8000.0f;
}

bool mf_fsm_g_recovery_stable() {
    // Minimal policy: subsystem is considered recovered when sensors are
    // back online and the SD warning has been explicitly cleared by the
    // operator (i.e. WARN_SD_FAIL is the only blocker we currently track).
    if (!mf_fsm_g_sensors_min_set()) return false;
    if (s_warn_flags & MF_WARN_SD_FAIL) return false;
    return true;
}

bool mf_fsm_g_emergency_clear_allowed() {
    // Placeholder for the "physical interlock + authenticated ack" policy.
    // Today we only require the latch to actually be set.
    return s_emergency_latch;
}

static bool g_recipe_selected() {
    return mf_recipe_has_valid_selection();
}

static bool g_config_ready() {
    return s_config_ready;
}

// =====================================================================
// State entry / exit hooks (apply physical safety policy on transition)
// =====================================================================

static void apply_state_entry_outputs(mf_runtime_state_t to) {
    switch (to) {
    case MF_STATE_PAUSED_SAFE:
        // Inlet fan drops to prophylactic minimum, humidifier OFF; light
        // is intentionally left untouched (recipe stage continues to own
        // photoperiod policy when paused).
        mf_actuators_set_percent(MF_ACT_HUMIDIFIER, 0.0f);
        mf_actuators_set_percent(MF_ACT_FAN, 15.0f);
        break;
    case MF_STATE_EMERGENCY_STOP:
        mf_actuators_all_off();
        break;
    case MF_STATE_BOOT:
    case MF_STATE_SETUP_AP:
    case MF_STATE_IDLE_READY:
        // No active climate loop in these states — make sure outputs are
        // off so a stale duty from a previous state cannot keep running.
        mf_actuators_set_percent(MF_ACT_FAN, 0.0f);
        mf_actuators_set_percent(MF_ACT_HUMIDIFIER, 0.0f);
        break;
    case MF_STATE_ACTIVE_RUN:
    case MF_STATE_DEGRADED_RUN:
        // Climate loop will start writing on its next tick — nothing to
        // do here, do not stomp the previous values.
        break;
    default:
        break;
    }
}

static void enter_state(mf_runtime_state_t to, const char *reason) {
    if (to == s_state) return;
    mf_runtime_state_t from = s_state;
    s_state = to;
    s_state_entered_ms = (int64_t)mf_clock_millis();
    apply_state_entry_outputs(to);
    mf_log_info("fsm", "%s -> %s (%s)",
                mf_fsm_state_str(from), mf_fsm_state_str(to),
                reason ? reason : "transition");
}

// =====================================================================
// Transition actions (run after guard ok, before enter_state).
// Each returns true on success; false aborts the transition (treated as
// guard failure).
// =====================================================================

typedef bool (*action_fn_t)(const mf_fsm_event_ctx_t *ctx);

static bool act_noop(const mf_fsm_event_ctx_t *ctx) { (void)ctx; return true; }

static bool act_persist_warn_sd(const mf_fsm_event_ctx_t *ctx) {
    if (ctx && ctx->nonfatal_code == MF_NONFATAL_SD) {
        s_warn_flags |= MF_WARN_SD_FAIL;
    }
    return true;
}

static bool act_clear_session(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    mf_session_clear();
    return true;
}

static bool act_emergency_latch(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    mf_session_clear();
    s_emergency_latch = true;
    return true;
}

static bool act_emergency_clear(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    s_emergency_latch = false;
    mf_session_clear();
    return true;
}

static bool act_start_cycle(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    mf_recipe_build_runtime_snapshot();
    if (!mf_session_save("S0")) {
        mf_log_warn("fsm", "session checkpoint failed at start");
    }
    return true;
}

static bool act_apply_config(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    s_config_ready = true;
    mf_log_info("fsm", "config applied (network restart deferred to caller)");
    return true;
}

static bool act_recovery_clear_warn(const mf_fsm_event_ctx_t *ctx) {
    (void)ctx;
    s_warn_flags &= ~MF_WARN_SD_FAIL;
    return true;
}

// =====================================================================
// Transition table — ordered exactly per docs/architecture/state-machine.md
// =====================================================================

typedef bool (*guard_fn_t)();

struct transition_t {
    mf_runtime_state_t from;
    mf_fsm_event_t event;
    guard_fn_t guard;       // nullptr == always allow
    mf_runtime_state_t to;  // may equal `from` for self-transitions
    action_fn_t action;     // nullptr == no side effect
    const char *reason;     // human-readable label for the log
};

// Combined guards used by BOOT branches.
static bool g_boot_idle()      { return g_config_ready() && !s_resume_pending; }
static bool g_boot_resume_ok() { return g_config_ready() &&  s_resume_pending && mf_fsm_g_sensors_min_set(); }
static bool g_boot_resume_deg(){ return g_config_ready() &&  s_resume_pending && !mf_fsm_g_sensors_min_set(); }

static bool g_start_cycle() {
    return g_recipe_selected()
        && mf_fsm_g_sensors_min_set()
        && mf_fsm_g_hard_limits_safe();
}

static bool g_resume_cycle() { return mf_fsm_g_hard_limits_safe(); }

static bool g_emergency_ack() {
    return mf_fsm_g_hard_limits_safe() && mf_fsm_g_emergency_clear_allowed();
}

static const transition_t k_transitions[] = {
    // ---------- BOOT ----------
    { MF_STATE_BOOT, MF_EV_CONFIG_MISSING, nullptr,        MF_STATE_SETUP_AP,    act_noop,             "config missing" },
    { MF_STATE_BOOT, MF_EV_BOOT_COMPLETE,  g_boot_idle,    MF_STATE_IDLE_READY,  act_noop,             "boot ok, no resume" },
    { MF_STATE_BOOT, MF_EV_BOOT_COMPLETE,  g_boot_resume_ok,  MF_STATE_ACTIVE_RUN,   act_noop,         "boot ok, resume active" },
    { MF_STATE_BOOT, MF_EV_BOOT_COMPLETE,  g_boot_resume_deg, MF_STATE_DEGRADED_RUN, act_noop,         "boot ok, resume degraded" },
    { MF_STATE_BOOT, MF_EV_FAULT_NONFATAL, nullptr,        MF_STATE_BOOT,        act_persist_warn_sd,  "warn during boot" },

    // ---------- SETUP_AP ----------
    { MF_STATE_SETUP_AP, MF_EV_APPLY_CONFIG,   nullptr, MF_STATE_IDLE_READY,    act_apply_config,    "apply config" },
    { MF_STATE_SETUP_AP, MF_EV_START_CYCLE,    nullptr, MF_STATE_SETUP_AP,      nullptr,             "rejected in SETUP_AP" }, // ERR_STATE-equivalent
    { MF_STATE_SETUP_AP, MF_EV_FAULT_FATAL,    nullptr, MF_STATE_EMERGENCY_STOP,act_emergency_latch, "fatal fault" },
    { MF_STATE_SETUP_AP, MF_EV_EMERGENCY_STOP, nullptr, MF_STATE_EMERGENCY_STOP,act_emergency_latch, "operator stop" },
    { MF_STATE_SETUP_AP, MF_EV_FAULT_NONFATAL, nullptr, MF_STATE_SETUP_AP,      act_persist_warn_sd, "warn in SETUP_AP" },

    // ---------- IDLE_READY ----------
    { MF_STATE_IDLE_READY, MF_EV_SELECT_RECIPE,           nullptr,         MF_STATE_IDLE_READY,    act_noop,            "select recipe" },
    { MF_STATE_IDLE_READY, MF_EV_START_CYCLE,             g_start_cycle,   MF_STATE_ACTIVE_RUN,    act_start_cycle,     "start cycle" },
    { MF_STATE_IDLE_READY, MF_EV_SERVICE_BTN_LONG_PRESS,  nullptr,         MF_STATE_SETUP_AP,      act_noop,            "service btn -> AP" },
    { MF_STATE_IDLE_READY, MF_EV_FAULT_FATAL,             nullptr,         MF_STATE_EMERGENCY_STOP,act_emergency_latch, "fatal fault" },
    { MF_STATE_IDLE_READY, MF_EV_EMERGENCY_STOP,          nullptr,         MF_STATE_EMERGENCY_STOP,act_emergency_latch, "operator stop" },
    { MF_STATE_IDLE_READY, MF_EV_FAULT_NONFATAL,          nullptr,         MF_STATE_IDLE_READY,    act_persist_warn_sd, "warn in IDLE" },
    { MF_STATE_IDLE_READY, MF_EV_EMERGENCY_ACK,           nullptr,         MF_STATE_IDLE_READY,    nullptr,             "noop (already clear)" },
    { MF_STATE_IDLE_READY, MF_EV_STOP_CYCLE,              nullptr,         MF_STATE_IDLE_READY,    nullptr,             "noop (no cycle)" },

    // ---------- ACTIVE_RUN ----------
    { MF_STATE_ACTIVE_RUN, MF_EV_PAUSE_CYCLE,    nullptr, MF_STATE_PAUSED_SAFE,    act_noop,            "pause" },
    { MF_STATE_ACTIVE_RUN, MF_EV_FAULT_NONFATAL, nullptr, MF_STATE_DEGRADED_RUN,   act_persist_warn_sd, "non-fatal -> degraded" },
    { MF_STATE_ACTIVE_RUN, MF_EV_FAULT_FATAL,    nullptr, MF_STATE_EMERGENCY_STOP, act_emergency_latch, "fatal fault" },
    { MF_STATE_ACTIVE_RUN, MF_EV_EMERGENCY_STOP, nullptr, MF_STATE_EMERGENCY_STOP, act_emergency_latch, "operator stop" },
    { MF_STATE_ACTIVE_RUN, MF_EV_STOP_CYCLE,     nullptr, MF_STATE_IDLE_READY,     act_clear_session,   "stop cycle" },
    { MF_STATE_ACTIVE_RUN, MF_EV_START_CYCLE,    nullptr, MF_STATE_ACTIVE_RUN,     nullptr,             "noop (already running)" },
    { MF_STATE_ACTIVE_RUN, MF_EV_RESUME_CYCLE,   nullptr, MF_STATE_ACTIVE_RUN,     nullptr,             "noop (not paused)" },

    // ---------- PAUSED_SAFE ----------
    { MF_STATE_PAUSED_SAFE, MF_EV_RESUME_CYCLE,   g_resume_cycle, MF_STATE_ACTIVE_RUN,     act_noop,            "resume" },
    { MF_STATE_PAUSED_SAFE, MF_EV_PAUSE_CYCLE,    nullptr,        MF_STATE_PAUSED_SAFE,    nullptr,             "noop (already paused)" },
    { MF_STATE_PAUSED_SAFE, MF_EV_FAULT_NONFATAL, nullptr,        MF_STATE_DEGRADED_RUN,   act_persist_warn_sd, "non-fatal -> degraded" },
    { MF_STATE_PAUSED_SAFE, MF_EV_FAULT_FATAL,    nullptr,        MF_STATE_EMERGENCY_STOP, act_emergency_latch, "fatal fault" },
    { MF_STATE_PAUSED_SAFE, MF_EV_EMERGENCY_STOP, nullptr,        MF_STATE_EMERGENCY_STOP, act_emergency_latch, "operator stop" },
    { MF_STATE_PAUSED_SAFE, MF_EV_STOP_CYCLE,     nullptr,        MF_STATE_IDLE_READY,     act_clear_session,   "stop cycle" },

    // ---------- DEGRADED_RUN ----------
    { MF_STATE_DEGRADED_RUN, MF_EV_PAUSE_CYCLE,        nullptr,            MF_STATE_PAUSED_SAFE,    act_noop,                "pause" },
    { MF_STATE_DEGRADED_RUN, MF_EV_RECOVERY_VALIDATED, mf_fsm_g_recovery_stable, MF_STATE_ACTIVE_RUN, act_recovery_clear_warn, "recovery validated" },
    { MF_STATE_DEGRADED_RUN, MF_EV_FAULT_FATAL,        nullptr,            MF_STATE_EMERGENCY_STOP, act_emergency_latch,     "fatal fault" },
    { MF_STATE_DEGRADED_RUN, MF_EV_EMERGENCY_STOP,     nullptr,            MF_STATE_EMERGENCY_STOP, act_emergency_latch,     "operator stop" },
    { MF_STATE_DEGRADED_RUN, MF_EV_STOP_CYCLE,         nullptr,            MF_STATE_IDLE_READY,     act_clear_session,       "stop cycle" },
    { MF_STATE_DEGRADED_RUN, MF_EV_START_CYCLE,        g_start_cycle,      MF_STATE_ACTIVE_RUN,     act_start_cycle,         "start cycle (degraded)" },

    // ---------- EMERGENCY_STOP ----------
    { MF_STATE_EMERGENCY_STOP, MF_EV_SERVICE_BTN_LONG_PRESS, nullptr,           MF_STATE_SETUP_AP,    act_noop,            "service diagnostics" },
    { MF_STATE_EMERGENCY_STOP, MF_EV_EMERGENCY_ACK,          g_emergency_ack,   MF_STATE_IDLE_READY,  act_emergency_clear, "ack -> idle" },
};

static const size_t k_transitions_count = sizeof(k_transitions) / sizeof(k_transitions[0]);

// =====================================================================
// Dispatch
// =====================================================================

// Side-effects performed before the transition table is consulted. These
// are intentionally minimal — only data normalisation that is needed for
// guards to evaluate correctly (e.g. caching the recipe id).
static void preprocess_event(mf_fsm_event_t event, const mf_fsm_event_ctx_t *ctx) {
    if (event == MF_EV_SELECT_RECIPE && ctx && ctx->recipe_id) {
        mf_recipe_set_selected_id(ctx->recipe_id);
    }
}

mf_fsm_result_t mf_fsm_dispatch(mf_fsm_event_t event,
                                const mf_fsm_event_ctx_t *ctx) {
    mf_fsm_event_ctx_t empty = {};
    if (!ctx) ctx = &empty;

    // Emergency latch trumps everything except "release" events.
    if (s_emergency_latch
        && event != MF_EV_EMERGENCY_ACK
        && event != MF_EV_SERVICE_BTN_LONG_PRESS
        && event != MF_EV_EMERGENCY_STOP) {
        mf_log_warn("fsm", "rejected %s: emergency latch closed",
                    mf_fsm_event_str(event));
        return MF_FSM_ERR_LATCHED;
    }

    preprocess_event(event, ctx);

    bool any_row_for_state_event = false;
    for (size_t i = 0; i < k_transitions_count; ++i) {
        const transition_t &t = k_transitions[i];
        if (t.from != s_state || t.event != event) continue;
        any_row_for_state_event = true;

        if (t.guard && !t.guard()) {
            // Try the next row; multiple rows may share (from,event) but
            // differ by guard (BOOT/BOOT_COMPLETE branches).
            continue;
        }

        // Self-transition with no action == idempotent NOOP per the spec.
        if (t.to == s_state && t.action == nullptr) {
            mf_log_info("fsm", "%s in %s -> NOOP",
                        mf_fsm_event_str(event), mf_fsm_state_str(s_state));
            return MF_FSM_NOOP;
        }

        if (t.action && !t.action(ctx)) {
            mf_log_warn("fsm", "action for %s in %s failed",
                        mf_fsm_event_str(event), mf_fsm_state_str(s_state));
            return MF_FSM_ERR_GUARD;
        }

        if (t.to != s_state) {
            enter_state(t.to, t.reason);
        } else {
            // Self-transition with side-effect (e.g. WARN persist).
            mf_log_info("fsm", "%s in %s (%s)",
                        mf_fsm_event_str(event), mf_fsm_state_str(s_state),
                        t.reason ? t.reason : "self-transition");
        }
        return MF_FSM_OK;
    }

    if (any_row_for_state_event) {
        mf_log_warn("fsm", "guard rejected %s in %s",
                    mf_fsm_event_str(event), mf_fsm_state_str(s_state));
        return MF_FSM_ERR_GUARD;
    }
    mf_log_warn("fsm", "no transition for %s in %s",
                mf_fsm_event_str(event), mf_fsm_state_str(s_state));
    return MF_FSM_ERR_STATE;
}

// =====================================================================
// Resume / NVS
// =====================================================================

void mf_fsm_set_resume_pending(bool pending) {
    s_resume_pending = pending;
}

bool mf_fsm_resume_restore_from_nvs() {
    mf_session_snapshot_t snap = {};
    if (!mf_session_load(&snap)) {
        s_resume_pending = false;
        return false;
    }
    mf_recipe_set_selected_id(snap.recipe_id);
    s_resume_pending = true;

    int64_t now_unix = mf_clock_unix_seconds();
    int64_t elapsed = (now_unix > snap.stage_started_unix_s && snap.stage_started_unix_s > 0)
                          ? (now_unix - snap.stage_started_unix_s)
                          : 0;
    mf_recipe_restore_stage_timer(elapsed);
    mf_log_info("fsm", "resume pending recipe=%s stage=%s elapsed=%llds",
                snap.recipe_id, snap.stage_id, (long long)elapsed);
    return true;
}

// =====================================================================
// Backwards-compatible wrappers
// =====================================================================

void mf_fsm_boot_done_config_ok() {
    s_config_ready = true;
    mf_fsm_dispatch(MF_EV_BOOT_COMPLETE, nullptr);
}

void mf_fsm_boot_done_config_missing() {
    s_config_ready = false;
    mf_fsm_dispatch(MF_EV_CONFIG_MISSING, nullptr);
}

void mf_fsm_apply_config() {
    mf_fsm_dispatch(MF_EV_APPLY_CONFIG, nullptr);
}

void mf_fsm_fault_nonfatal(mf_fsm_nonfatal_t code) {
    mf_fsm_event_ctx_t c = {};
    c.nonfatal_code = code;
    mf_fsm_dispatch(MF_EV_FAULT_NONFATAL, &c);
}

void mf_fsm_fault_fatal(mf_fsm_fatal_t code) {
    mf_fsm_event_ctx_t c = {};
    c.fatal_code = code;
    mf_fsm_dispatch(MF_EV_FAULT_FATAL, &c);
}

void mf_fsm_recovery_validated() {
    mf_fsm_dispatch(MF_EV_RECOVERY_VALIDATED, nullptr);
}

void mf_fsm_emergency_stop() {
    mf_fsm_dispatch(MF_EV_EMERGENCY_STOP, nullptr);
}

void mf_fsm_emergency_ack() {
    mf_fsm_dispatch(MF_EV_EMERGENCY_ACK, nullptr);
}

void mf_fsm_service_button_long_press() {
    mf_fsm_dispatch(MF_EV_SERVICE_BTN_LONG_PRESS, nullptr);
}

void mf_fsm_select_recipe(const char *recipe_id) {
    if (!recipe_id) return;
    // SELECT_RECIPE has rows only for IDLE_READY in the spec; in other
    // states we still want to cache the id so callers (e.g. SETUP_AP UI)
    // can pre-select before applying config. Caching is done via
    // preprocess_event -> mf_recipe_set_selected_id, and the dispatch
    // call returns ERR_STATE without harm.
    mf_fsm_event_ctx_t c = {};
    c.recipe_id = recipe_id;
    mf_fsm_dispatch(MF_EV_SELECT_RECIPE, &c);
}

mf_fsm_result_t mf_fsm_start_cycle() {
    return mf_fsm_dispatch(MF_EV_START_CYCLE, nullptr);
}

mf_fsm_result_t mf_fsm_stop_cycle() {
    return mf_fsm_dispatch(MF_EV_STOP_CYCLE, nullptr);
}

mf_fsm_result_t mf_fsm_pause_cycle() {
    return mf_fsm_dispatch(MF_EV_PAUSE_CYCLE, nullptr);
}

mf_fsm_result_t mf_fsm_resume_cycle() {
    return mf_fsm_dispatch(MF_EV_RESUME_CYCLE, nullptr);
}

bool mf_fsm_stage_transition_checkpoint(const char *stage_id) {
    if (!stage_id || stage_id[0] == '\0') return false;
    if (s_state != MF_STATE_ACTIVE_RUN && s_state != MF_STATE_DEGRADED_RUN) return false;
    return mf_session_save(stage_id);
}

const char *mf_fsm_selected_recipe_id() {
    return mf_recipe_get_selected_id();
}
