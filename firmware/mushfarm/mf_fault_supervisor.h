#pragma once

#include <stdint.h>
#include <stdbool.h>

// Sensor fault supervisor.
//
// Bridges the per-driver fault state (mf_scd41 / mf_mlx90614 / water
// policy) and the FSM event flow described in
// docs/architecture/state-machine.md and docs/architecture/fault-model.md.
//
// Drivers maintain their own retry/recovery counters and expose
// mf_*_fault_disconnected() / mf_*_ok(). The supervisor does the
// edge-detection: when a driver flips false->true on disconnect it fires
// mf_fsm_fault_nonfatal(MF_NONFATAL_*) (which moves ACTIVE_RUN ->
// DEGRADED_RUN). When the driver self-clears its fault after the
// recovery streak, the supervisor clears the corresponding warn flag and,
// once all sensor warn flags are clear AND we are in DEGRADED_RUN, fires
// mf_fsm_recovery_validated().
//
// While already in DEGRADED_RUN, additional sensor faults still dispatch
// evFaultNonFatal (FSM self-loop + act_persist_warn) so every subsystem
// gets its own MF_WARN_* flag.
//
// Call mf_fault_supervisor_tick() after every sensor poll (see task_sensors
// in mushfarm.ino).

void mf_fault_supervisor_init();
void mf_fault_supervisor_tick();

// Test hook: force-publish a fault for the given subsystem (used by the
// mushfarm_tests sketch and by service-mode fault injection).
typedef enum {
    MF_FS_SUBSYS_SCD41 = 0,
    MF_FS_SUBSYS_MLX90614,
    MF_FS_SUBSYS_WATER,
} mf_fault_subsys_t;

void mf_fault_supervisor_inject_fault(mf_fault_subsys_t which);
void mf_fault_supervisor_inject_recovery(mf_fault_subsys_t which);
