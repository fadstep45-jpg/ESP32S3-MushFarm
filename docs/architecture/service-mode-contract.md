# Service Mode Contract

## Goals

- Enable safe diagnostics and configuration changes during field operation.
- Prevent service actions from breaking life-critical climate guarantees.

## Entry Conditions

Service mode can be entered by:

- long-press hardware button;
- explicit AP/MQTT command;
- auto-entry fallback on provisioning failures.

## Service Mode Classes

- `OBSERVE_ONLY`: monitoring without control overrides.
- `MAINTENANCE_ACTIVE`: manual tests enabled with safety limits.
- `RECOVERY_CONFIG`: credential and connectivity recovery.

## Live Climate Policy During Service Mode

- In `ACTIVE_RUN`, climate loop continues unless user explicitly pauses/stops.
- In `SETUP_AP`, climate autonomous loop is **optional**; default policy is **safe idle outputs** (fans/humidifier/lights OFF or recipe-defined safe hold) unless operator explicitly enables “AP + climate continue” in maintenance policy. Hard safety limits remain evaluated if any output is enabled.
- Manual actuator tests are time-bounded and mutually exclusive with corresponding auto loop.
- Hard safety limits remain enforced at all times.

## Configuration Apply Matrix

### Hot Apply (no restart)

- setpoint/hysteresis changes;
- recipe stage patch;
- alert thresholds (non-critical);
- telemetry interval and log verbosity.

### Requires Restart

- Wi-Fi credentials;
- MQTT broker/security settings;
- low-level bus config;
- camera streaming mode changes when driver reinit required.

## Manual Test Contract

- actuator test request must include:
  - target actuator,
  - duty/power,
  - timeout (seconds),
  - operator confirmation token.
- automatic rollback to previous state after timeout.
- deny test when hard-limit guard is active.

## Calibration Contract

- calibrations are versioned and auditable.
- each calibration requires:
  - baseline capture,
  - validation sample window,
  - commit with rollback point.
- failed validation keeps previous calibration.

## Reset Policies

### Soft Reset

- restart runtime services;
- keep recipes and credentials.

### Factory Reset

- wipe credentials and runtime selection;
- optionally preserve recipe library by policy flag;
- requires two-step confirmation and physical button presence if possible.

## Camera Behavior

- baseline: on-demand snapshot for AP pages.
- optional: low-FPS stream in service mode only.
- camera failure must not block control loops.

## Safety Constraints

- `EMERGENCY_STOP` command always preempts service actions.
- any service command that conflicts with hard-limits returns `ERR_SAFETY_LIMIT`.
- all service actions generate audit records with actor, timestamp, and result.
