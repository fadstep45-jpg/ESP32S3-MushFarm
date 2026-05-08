#pragma once

#include <stdint.h>

// Cooperative scheduler: a fixed-size table of periodic tasks driven from
// the main Arduino loop(). No FreeRTOS tasks, no preemption — every job is
// expected to be short and non-blocking.

typedef void (*mf_sched_fn_t)();

#define MF_SCHED_MAX_TASKS 12

bool mf_scheduler_add(const char *name, uint32_t interval_ms, mf_sched_fn_t fn);
void mf_scheduler_tick();
