# Runtime State Machine

## Purpose
This state machine defines deterministic lifecycle control for the farm and prevents ambiguous transitions between setup, active grow, and degraded operation.

## States

- `BOOT` - power-on initialization.
- `SETUP_AP` - AP portal mode for configuration and recovery.
- `IDLE_READY` - system is healthy and waiting for recipe selection/start.
- `ACTIVE_RUN` - recipe execution and closed-loop control.
- `PAUSED_SAFE` - cycle paused, actuators in safe-hold policy.
- `DEGRADED_RUN` - cycle running with partial functionality after non-fatal faults.
- `EMERGENCY_STOP` - explicit user/system stop, all critical outputs off.

## State Invariants

- `BOOT`: outputs OFF, no cycle actions.
- `SETUP_AP`: management interface ON; climate autonomous loop optional with **default safe outputs** per [service-mode-contract.md](service-mode-contract.md).
- `IDLE_READY`: recipe may be selected, no stage progression.
- `ACTIVE_RUN`: selected `current_recipe` snapshot exists in RAM.
- `PAUSED_SAFE`: stage timer stopped, no autonomous transitions.
- `DEGRADED_RUN`: at least one subsystem is disabled by fault policy.
- `EMERGENCY_STOP`: no autonomous restart.

## Events

- `evBootComplete`
- `evConfigMissing`
- `evConfigValid`
- `evSelectRecipe(recipe_id)`
- `evStartCycle`
- `evPauseCycle`
- `evResumeCycle`
- `evEmergencyStop`
- `evFaultNonFatal(code)`
- `evFaultFatal(code)`
- `evRecoveryValidated`
- `evServiceButtonLongPress`
- `evApplyConfig`
- `evEmergencyAcknowledged` - operator clears emergency latch after physical inspection (MQTT/AP/button policy).

## Guard Conditions

- `gConfigReady`: Wi-Fi/MQTT credentials accepted or offline policy allowed.
- `gRecipeSelected`: selected recipe exists and passes schema check.
- `gSensorsMinSet`: minimum required sensors for requested control loops are available.
- `gHardLimitsSafe`: no hard-limit violation at transition time.
- `gRecoveryStable`: faulted subsystem passes health checks during observation window.
- `gEmergencyClearAllowed`: policy satisfied to leave `EMERGENCY_STOP` (e.g. acknowledged command + optional physical interlock).
- `gResumePending`: NVS memory contains an active session flag and a valid `current_recipe` snapshot from before a power loss.
- `WARN_SD_FAIL`: sticky warning flag meaning SD init/mount/write failed and logging is running in RAM-only mode.

## Transition Matrix

| From | Event | Guard | To | Action |
| --- | --- | --- | --- | --- |
| `BOOT` | `evConfigMissing` | - | `SETUP_AP` | start AP portal, lock cycle commands |
| `BOOT` | `evBootComplete` | `gConfigReady && !gResumePending` | `IDLE_READY` | publish READY event |
| `BOOT` | `evBootComplete` | `gConfigReady && gResumePending && gSensorsMinSet` | `ACTIVE_RUN` | restore stage timers from NVS, resume PID |
| `BOOT` | `evBootComplete` | `gConfigReady && gResumePending && !gSensorsMinSet` | `DEGRADED_RUN` | restore stage timers, disable affected loops |
| `BOOT` | `evFaultNonFatal(SD_INIT/SD_MOUNT/SD_WRITE)` | - | `BOOT` | set `WARN_SD_FAIL`; continue normal boot sequence |
| `SETUP_AP` | `evApplyConfig` | `gConfigReady` | `IDLE_READY` | persist config, restart network stack |
| `SETUP_AP` | `evStartCycle` | - | `SETUP_AP` | reject with `ERR_STATE` |
| `SETUP_AP` | `evFaultFatal` | - | `EMERGENCY_STOP` | shutdown critical loads |
| `SETUP_AP` | `evEmergencyStop` | - | `EMERGENCY_STOP` | immediate stop + alert |
| `SETUP_AP` | `evFaultNonFatal` | - | `SETUP_AP` | set fault flags / WARN; may restrict hot-apply |
| `IDLE_READY` | `evSelectRecipe` | `gRecipeSelected` | `IDLE_READY` | cache selected recipe id |
| `IDLE_READY` | `evStartCycle` | `gRecipeSelected && gSensorsMinSet && gHardLimitsSafe` | `ACTIVE_RUN` | create `current_recipe` snapshot, write active flag to NVS |
| `IDLE_READY` | `evServiceButtonLongPress` | - | `SETUP_AP` | open AP without losing selection |
| `IDLE_READY` | `evFaultFatal` | - | `EMERGENCY_STOP` | shutdown critical loads |
| `IDLE_READY` | `evEmergencyStop` | - | `EMERGENCY_STOP` | immediate stop + alert |
| `IDLE_READY` | `evFaultNonFatal` | - | `IDLE_READY` | publish WARN; if fault removes `gSensorsMinSet`, reject `evStartCycle` |
| `ACTIVE_RUN` | `evPauseCycle` | - | `PAUSED_SAFE` | hold outputs (inlet_fan drops to prophylactic minimum, humidifier OFF), stop stage timers |
| `ACTIVE_RUN` | `evFaultNonFatal(SD_*)` | - | `DEGRADED_RUN` | keep PID/actuators running; switch logging to RAM buffer and mark `WARN_SD_FAIL` |
| `ACTIVE_RUN` | `evFaultFatal` | - | `EMERGENCY_STOP` | shutdown critical loads |
| `ACTIVE_RUN` | `evEmergencyStop` | - | `EMERGENCY_STOP` | immediate stop + alert, clear NVS active flag |
| `PAUSED_SAFE` | `evResumeCycle` | `gHardLimitsSafe` | `ACTIVE_RUN` | resume stage timers and normal PID control |
| `PAUSED_SAFE` | `evFaultNonFatal` | - | `DEGRADED_RUN` | move to degraded resume policy, await resume command |
| `PAUSED_SAFE` | `evFaultFatal` | - | `EMERGENCY_STOP` | shutdown critical loads |
| `PAUSED_SAFE` | `evEmergencyStop` | - | `EMERGENCY_STOP` | immediate stop + alert, clear NVS active flag |
| `DEGRADED_RUN` | `evPauseCycle` | - | `PAUSED_SAFE` | hold outputs, preserve fault flags in memory |
| `DEGRADED_RUN` | `evRecoveryValidated` | `gRecoveryStable` | `ACTIVE_RUN` | re-enable recovered loops |
| `DEGRADED_RUN` | `evFaultFatal` | - | `EMERGENCY_STOP` | shutdown critical loads |
| `DEGRADED_RUN` | `evEmergencyStop` | - | `EMERGENCY_STOP` | immediate stop + alert, clear NVS active flag |
| `EMERGENCY_STOP` | `evServiceButtonLongPress` | - | `SETUP_AP` | service diagnostics only |
| `EMERGENCY_STOP` | `evEmergencyAcknowledged` | `gHardLimitsSafe && gEmergencyClearAllowed` | `IDLE_READY` | clear stop latch; outputs OFF; clear NVS active flag |

## Recovery from `EMERGENCY_STOP`

1. **Latch**: entering `EMERGENCY_STOP` sets a persistent **stop latch** until explicitly cleared (survives non-fatal reboots if policy says so).
2. **Physical state**: all critical outputs OFF; climate autonomous control suspended.
3. **Clear path A (direct to idle)**: authenticated `evEmergencyAcknowledged` when `gHardLimitsSafe` and `gEmergencyClearAllowed` (e.g. operator confirms inspection, CO2/RH within safe bands).
4. **Clear path B (via service)**: `evServiceButtonLongPress` → `SETUP_AP` for diagnostics; after fixes, `evApplyConfig` / `evEmergencyAcknowledged` → `IDLE_READY` per policy.
5. **No auto-restart**: `evStartCycle` never fires from firmware alone after emergency; operator must issue start again.

## Idempotency Rules

- `evStartCycle` in `ACTIVE_RUN` returns `ACK_NOOP`.
- `evPauseCycle` in `PAUSED_SAFE` returns `ACK_NOOP`.
- `evResumeCycle` in `ACTIVE_RUN` returns `ACK_NOOP`.
- Repeated `evSelectRecipe` with same id updates timestamp only.
- `evEmergencyAcknowledged` in `IDLE_READY` returns `ACK_NOOP` (already clear).

## Safety Rules

- Transition to `ACTIVE_RUN` is forbidden without validated recipe snapshot.
- Any fatal power path anomaly forces `EMERGENCY_STOP`.
- `DEGRADED_RUN` is preferred over stop for non-fatal sensor/storage/network faults.
- SD-card failures are always treated as non-fatal: farm control must continue using RAM buffered logging.

## NVS Resume Checkpoint (`gResumePending`)

- Checkpoint writes happen only on macro-events (not PID ticks): `evStartCycle` and stage transition events.
- Stored snapshot in NVS includes active-session flag, selected recipe id, current stage id, and stage start timestamp.
- On `BOOT`, firmware reads NVS; if active flag exists and snapshot is valid, it sets `gResumePending=true` and restores into `ACTIVE_RUN` (or `DEGRADED_RUN` if guards fail).
- Session flag is cleared on `evEmergencyStop`, `evEmergencyAcknowledged`, and normal cycle stop/completion.
