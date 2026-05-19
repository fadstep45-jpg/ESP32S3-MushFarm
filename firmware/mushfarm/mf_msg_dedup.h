#pragma once

#include "mf_config.h"
#include <stdbool.h>
#include <stdint.h>

void mf_msg_dedup_init();
void mf_msg_dedup_reset();

/** True if msg_id was already processed and is still within TTL. */
bool mf_msg_dedup_seen(const char *msg_id);

/** Record msg_id after a command was applied (or duplicate ack path). */
void mf_msg_dedup_commit(const char *msg_id);

#if MF_TEST_HOOKS
void mf_msg_dedup_test_advance_ms(uint32_t ms);
uint32_t mf_msg_dedup_test_entry_count();
#endif
