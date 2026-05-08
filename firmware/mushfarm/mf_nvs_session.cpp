#include "mf_nvs_session.h"
#include "mf_recipe.h"
#include "mf_clock.h"
#include "mf_log.h"
#include <Preferences.h>
#include <string.h>

static const char *PREF_NAMESPACE = "mf_fsm";
static const char *KEY_ACTIVE = "sess_active";
static const char *KEY_BLOB   = "sess_blob";

bool mf_session_save(const char *stage_id) {
    Preferences p;
    if (!p.begin(PREF_NAMESPACE, false)) {
        mf_log_warn("session", "begin RW failed");
        return false;
    }
    mf_session_snapshot_t snap = {};
    snap.magic = MF_SESSION_MAGIC;
    snap.version = MF_SESSION_VERSION;
    strncpy(snap.recipe_id, mf_recipe_get_selected_id(), sizeof(snap.recipe_id) - 1);
    strncpy(snap.stage_id, stage_id ? stage_id : "S0", sizeof(snap.stage_id) - 1);
    snap.stage_started_unix_s = mf_clock_unix_seconds();

    size_t written = p.putBytes(KEY_BLOB, &snap, sizeof(snap));
    bool ok = (written == sizeof(snap));
    if (ok) {
        ok = (p.putUChar(KEY_ACTIVE, 1u) > 0);
    }
    p.end();
    if (!ok) {
        mf_log_warn("session", "save failed written=%u", (unsigned)written);
    } else {
        mf_log_info("session", "checkpoint recipe=%s stage=%s", snap.recipe_id, snap.stage_id);
    }
    return ok;
}

bool mf_session_clear() {
    Preferences p;
    if (!p.begin(PREF_NAMESPACE, false)) {
        return false;
    }
    p.remove(KEY_ACTIVE);
    p.remove(KEY_BLOB);
    p.end();
    mf_log_info("session", "cleared");
    return true;
}

bool mf_session_load(mf_session_snapshot_t *out) {
    if (!out) return false;
    Preferences p;
    if (!p.begin(PREF_NAMESPACE, true)) {
        // Namespace may not exist yet — that's a clean state, not an error.
        return false;
    }
    bool active = p.getUChar(KEY_ACTIVE, 0) == 1u;
    if (!active) {
        p.end();
        return false;
    }
    size_t read = p.getBytes(KEY_BLOB, out, sizeof(*out));
    p.end();
    if (read != sizeof(*out)) return false;
    if (out->magic != MF_SESSION_MAGIC || out->version != MF_SESSION_VERSION) return false;
    if (out->recipe_id[0] == '\0') return false;
    return true;
}
