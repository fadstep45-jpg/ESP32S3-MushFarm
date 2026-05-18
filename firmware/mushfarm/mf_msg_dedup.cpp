#include "mf_msg_dedup.h"
#include "mf_config.h"
#include "mf_clock.h"
#include <string.h>

#ifndef MF_MSG_DEDUP_CAPACITY
#define MF_MSG_DEDUP_CAPACITY 256
#endif

#ifndef MF_MSG_DEDUP_TTL_MS
#define MF_MSG_DEDUP_TTL_MS (24u * 3600u * 1000u)
#endif

#ifndef MF_MSG_ID_MAX
#define MF_MSG_ID_MAX 40
#endif

struct dedup_entry_t {
    char id[MF_MSG_ID_MAX];
    uint32_t seen_ms;
};

static dedup_entry_t s_ring[MF_MSG_DEDUP_CAPACITY];
static uint32_t s_head = 0;
static uint32_t s_count = 0;
#if MF_TEST_HOOKS
static uint32_t s_test_time_offset_ms = 0;
#endif

static uint32_t now_ms() {
#if MF_TEST_HOOKS
    return mf_clock_millis() + s_test_time_offset_ms;
#else
    return mf_clock_millis();
#endif
}

void mf_msg_dedup_init() {
    mf_msg_dedup_reset();
}

void mf_msg_dedup_reset() {
    s_head = 0;
    s_count = 0;
    for (uint32_t i = 0; i < MF_MSG_DEDUP_CAPACITY; ++i) {
        s_ring[i].id[0] = '\0';
        s_ring[i].seen_ms = 0;
    }
}

static int find_entry(const char *msg_id) {
    if (!msg_id || !msg_id[0]) {
        return -1;
    }
    uint32_t t = now_ms();
    for (uint32_t i = 0; i < s_count; ++i) {
        uint32_t idx = (s_head + MF_MSG_DEDUP_CAPACITY - 1 - i) % MF_MSG_DEDUP_CAPACITY;
        if (s_ring[idx].id[0] == '\0') {
            continue;
        }
        if ((t - s_ring[idx].seen_ms) > MF_MSG_DEDUP_TTL_MS) {
            s_ring[idx].id[0] = '\0';
            continue;
        }
        if (strcmp(s_ring[idx].id, msg_id) == 0) {
            return (int)idx;
        }
    }
    return -1;
}

bool mf_msg_dedup_seen(const char *msg_id) {
    return find_entry(msg_id) >= 0;
}

void mf_msg_dedup_commit(const char *msg_id) {
    if (!msg_id || !msg_id[0]) {
        return;
    }
    int existing = find_entry(msg_id);
    uint32_t t = now_ms();
    if (existing >= 0) {
        s_ring[(uint32_t)existing].seen_ms = t;
        return;
    }
    uint32_t slot;
    if (s_count < MF_MSG_DEDUP_CAPACITY) {
        slot = (s_head + s_count) % MF_MSG_DEDUP_CAPACITY;
        s_count++;
    } else {
        slot = s_head;
        s_head = (s_head + 1) % MF_MSG_DEDUP_CAPACITY;
    }
    strncpy(s_ring[slot].id, msg_id, MF_MSG_ID_MAX - 1);
    s_ring[slot].id[MF_MSG_ID_MAX - 1] = '\0';
    s_ring[slot].seen_ms = t;
}

#if MF_TEST_HOOKS
void mf_msg_dedup_test_advance_ms(uint32_t ms) {
    s_test_time_offset_ms += ms;
}

uint32_t mf_msg_dedup_test_entry_count() {
    return s_count;
}
#endif
