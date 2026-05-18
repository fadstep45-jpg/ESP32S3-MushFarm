#include "mf_fault_supervisor.h"
#include "mf_config.h"
#include "mf_fsm.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_water_policy.h"
#include "mf_log.h"
#if MF_MQTT_ENABLE
#include "mf_mqtt.h"
#endif

// Last observed disconnected state per subsystem; supervisor only fires
// FSM events on edges to avoid spamming the dispatcher and the trace.
static bool s_scd_was_bad = false;
static bool s_mlx_was_bad = false;
static bool s_water_was_locked = false;

static bool any_sensor_warn_set() {
    uint32_t w = mf_fsm_warn_flags();
    return (w & (MF_WARN_SCD41_FAIL | MF_WARN_MLX_FAIL | MF_WARN_WATER_FAIL)) != 0u;
}

#if MF_MQTT_ENABLE
static void publish_sensor_alert(const char *code, const char *detail) {
    mf_mqtt_alert_publish("WARN", code, detail);
}
#else
static void publish_sensor_alert(const char *, const char *) {}
#endif

static void maybe_publish_recovery() {
    // Only meaningful in DEGRADED_RUN. The FSM event itself has its own
    // guard (g_recovery_stable) and will reject if sensors aren't truly OK.
    if (any_sensor_warn_set()) return;
    if (mf_fsm_state() != MF_STATE_DEGRADED_RUN) return;
    mf_log_info("fs", "all sensor warns cleared -> publish RECOVERY_VALIDATED");
    mf_fsm_recovery_validated();
}

void mf_fault_supervisor_init() {
    s_scd_was_bad = false;
    s_mlx_was_bad = false;
    s_water_was_locked = false;
}

void mf_fault_supervisor_tick() {
    // --- SCD41 ---
    bool scd_bad_now = mf_scd41_fault_disconnected();
    if (scd_bad_now && !s_scd_was_bad) {
        mf_log_warn("fs", "edge: SCD41 disconnected -> evFaultNonFatal(SCD41)");
        publish_sensor_alert("WARN_SCD41_FAIL", "SCD41 RH/CO2 sensor disconnected");
        mf_fsm_fault_nonfatal(MF_NONFATAL_SCD41);
    } else if (!scd_bad_now && s_scd_was_bad) {
        // Driver has cleared its internal latch after the recovery streak
        // (MF_SENSOR_RECOVERY_SAMPLES consecutive valid samples).
        mf_log_info("fs", "edge: SCD41 recovered -> clear WARN_SCD41_FAIL");
        mf_fsm_clear_warn(MF_WARN_SCD41_FAIL);
        maybe_publish_recovery();
    }
    s_scd_was_bad = scd_bad_now;

    // --- MLX90614 ---
    bool mlx_bad_now = mf_mlx90614_fault_disconnected();
    if (mlx_bad_now && !s_mlx_was_bad) {
        mf_log_warn("fs", "edge: MLX90614 disconnected -> evFaultNonFatal(MLX90614)");
        publish_sensor_alert("WARN_MLX_FAIL", "MLX90614 substrate sensor disconnected");
        mf_fsm_fault_nonfatal(MF_NONFATAL_MLX90614);
    } else if (!mlx_bad_now && s_mlx_was_bad) {
        mf_log_info("fs", "edge: MLX90614 recovered -> clear WARN_MLX_FAIL");
        mf_fsm_clear_warn(MF_WARN_MLX_FAIL);
        maybe_publish_recovery();
    }
    s_mlx_was_bad = mlx_bad_now;

    // --- Water (escalates via mf_water_policy LOCKED state, not raw absence;
    // single LOW reading is a WARN, not a fault, per fault-model.md). ---
    bool water_locked_now = mf_water_policy_locked();
    if (water_locked_now && !s_water_was_locked) {
        mf_log_warn("fs", "edge: water policy LOCKED -> evFaultNonFatal(WATER)");
        publish_sensor_alert("WARN_WATER_FAIL", "Water reserve exhausted; humidifier locked");
        mf_fsm_fault_nonfatal(MF_NONFATAL_WATER);
    } else if (!water_locked_now && s_water_was_locked) {
        mf_log_info("fs", "edge: water policy left LOCKED -> clear WARN_WATER_FAIL");
        mf_fsm_clear_warn(MF_WARN_WATER_FAIL);
        maybe_publish_recovery();
    }
    s_water_was_locked = water_locked_now;
}

void mf_fault_supervisor_inject_fault(mf_fault_subsys_t which) {
    switch (which) {
    case MF_FS_SUBSYS_SCD41:
        s_scd_was_bad = true;
        publish_sensor_alert("WARN_SCD41_FAIL", "SCD41 RH/CO2 sensor disconnected");
        mf_fsm_fault_nonfatal(MF_NONFATAL_SCD41);
        break;
    case MF_FS_SUBSYS_MLX90614:
        s_mlx_was_bad = true;
        publish_sensor_alert("WARN_MLX_FAIL", "MLX90614 substrate sensor disconnected");
        mf_fsm_fault_nonfatal(MF_NONFATAL_MLX90614);
        break;
    case MF_FS_SUBSYS_WATER:
        s_water_was_locked = true;
        publish_sensor_alert("WARN_WATER_FAIL", "Water reserve exhausted; humidifier locked");
        mf_fsm_fault_nonfatal(MF_NONFATAL_WATER);
        break;
    }
}

void mf_fault_supervisor_inject_recovery(mf_fault_subsys_t which) {
    switch (which) {
    case MF_FS_SUBSYS_SCD41:
        s_scd_was_bad = false;
        mf_fsm_clear_warn(MF_WARN_SCD41_FAIL);
        break;
    case MF_FS_SUBSYS_MLX90614:
        s_mlx_was_bad = false;
        mf_fsm_clear_warn(MF_WARN_MLX_FAIL);
        break;
    case MF_FS_SUBSYS_WATER:
        s_water_was_locked = false;
        mf_fsm_clear_warn(MF_WARN_WATER_FAIL);
        break;
    }
    maybe_publish_recovery();
}
