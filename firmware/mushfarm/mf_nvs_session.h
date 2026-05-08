#pragma once

#include <stdint.h>
#include <stdbool.h>

// Session checkpoint persisted in NVS via Preferences.h. Used for the
// gResumePending guard described in docs/architecture/state-machine.md.

struct mf_session_snapshot_t {
    uint32_t magic;
    uint32_t version;
    char recipe_id[64];
    char stage_id[16];
    int64_t stage_started_unix_s;
};

#define MF_SESSION_MAGIC   0x4D46534Du
#define MF_SESSION_VERSION 1u

bool mf_session_save(const char *stage_id);
bool mf_session_clear();
bool mf_session_load(mf_session_snapshot_t *out);
