#include "mf_scheduler.h"
#include "mf_clock.h"
#include "mf_log.h"
#include <string.h>

struct mf_sched_task_t {
    const char *name;
    uint32_t interval_ms;
    uint32_t last_ms;
    mf_sched_fn_t fn;
};

static mf_sched_task_t s_tasks[MF_SCHED_MAX_TASKS];
static size_t s_task_count = 0;

bool mf_scheduler_add(const char *name, uint32_t interval_ms, mf_sched_fn_t fn) {
    if (!fn || s_task_count >= MF_SCHED_MAX_TASKS) {
        mf_log_warn("sched", "add rejected name=%s count=%u", name ? name : "?", (unsigned)s_task_count);
        return false;
    }
    s_tasks[s_task_count] = mf_sched_task_t{name, interval_ms, 0, fn};
    s_task_count++;
    mf_log_info("sched", "registered name=%s every=%lums", name ? name : "?", (unsigned long)interval_ms);
    return true;
}

void mf_scheduler_tick() {
    for (size_t i = 0; i < s_task_count; ++i) {
        if (mf_clock_elapsed(&s_tasks[i].last_ms, s_tasks[i].interval_ms)) {
            s_tasks[i].fn();
        }
    }
}
