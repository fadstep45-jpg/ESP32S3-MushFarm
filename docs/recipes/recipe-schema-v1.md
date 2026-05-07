# Recipe schema v1

This document defines stable field names and semantics for YAML recipes under `recipes/`. Firmware and validators should target `recipe_meta.schema_version: 1`.

## Canonical recipe files

- **Format**: UTF-8 **YAML** files named `recipes/*.yaml` (machine-facing source of truth).
- **Validation**: CI and device upload pipeline must validate parsed YAML against [recipe.schema.v1.json](recipe.schema.v1.json) (JSON Schema draft 2020-12). YAML parses to JSON-compatible types for validation.
- **Legacy**: `.md` recipe filenames are deprecated; do not add new ones.

## Top-level sections

| Section | Purpose |
| --- | --- |
| `recipe_meta` | Identity, versioning, human notes |
| `execution_policy` | Start/resume/logging behaviour |
| `hardware_profile` | Required sensors/actuators and minimum start set |
| `hardware_mapping` | Logical actuator names to physical roles (single-fan setups) |
| `global_targets` | Feature flags for control loops |
| `safety_profile` | Limits, CO2 policy, water overrides, RH hysteresis on limits |
| `stage_transition_defaults` | How timer vs event interact for all stages |
| `light_schedule_defaults` | Photoperiod when `light_hours_per_day` > 0 |
| `stages` | Ordered list of stage definitions |
| `acceptance_criteria` | Human test checklist |

Related registry: [transition-rules-v1.md](transition-rules-v1.md).

## CO2 policy (safety vs operational)

Three layers:

1. **Operational setpoint** — `stages[].setpoints.co2_target_ppm` drives PID toward grower intent.
2. **Stage operational purge** — `stages[].arbitration.co2_operational_purge_enabled` enables **PID- or stage-scheduled** high-CO2 ventilation (timed `purge_duration_sec`). Disabling it **does not** disable global safety below.
3. **Global safety** — `safety_profile.co2_safety.co2_emergency_absolute_ppm`: if measured CO2 **exceeds** this threshold, firmware **must** run **forced ventilation** on `prophylactic_ventilation_actuator` (or primary ventilation channel). **No stage flag may disable this.** Cycle stop remains governed by `stop_cycle_on_co2_global` and other policies.

`ignore_operational_co2_cap`: when true, stage ignores **operational** high-CO2 caps (PID/soft limits); it **never** disables `co2_emergency_absolute_ppm` response.

`co2_sensor_trust_upper_ppm`: readings **above** this are outside the sensor’s trusted range (SCD41 datasheet upper range is typically **5000 ppm**). Firmware may clamp PID effective setpoint and raise WARN.

`clamp_pid_effective_setpoint_to_trust_range`: when true, effective PID setpoint is limited into the trusted band.

`clamp_trust_upper_inclusive`: when true, a setpoint **equal to** `co2_sensor_trust_upper_ppm` is allowed (boundary is inclusive). When false, use `min(target, trust_upper - epsilon)` to avoid saturation ambiguity.

## Temperature limits

Use explicit air vs substrate bounds:

- `air_temp_min_crit_c` / `air_temp_max_crit_c` — SCD41 air temperature.
- `substrate_temp_min_crit_c` / `substrate_temp_max_crit_c` — MLX90614 object temperature.

Deprecated: generic `temp_min_crit_c` / `temp_max_crit_c` (ambiguous).

## Minimum sensors for start

If `global_targets.temp_control_enabled: true` and stages use `substrate_temp_target_c` or `max_substrate_air_delta_c`, `minimum_sensor_set_for_start` **must** include `mlx90614`.

## Stage transitions

`stage_transition_defaults.mode`:

- `event_first_timer_fallback` — fire `transition_rule` event or operator confirm first; `auto_transition_after_days` is a **fallback** if event never satisfied.
- `timer_first_event_shortcuts` — timer can be shortened/cancelled by event (document per recipe if needed).

Each stage keeps `transition_rule` (string id) and `auto_transition_after_days` (number). Allowed values are listed in [transition-rules-v1.md](transition-rules-v1.md).

Optional per stage:

- `transition_auto_enabled: false` — never auto-advance on timer; `auto_transition_after_days` may still be used as `operator_reminder_only` (see `auto_transition_after_days_role`).
- `auto_transition_after_days_role`: `timer_fallback` | `operator_reminder_only`.

## Arbitration (stage)

- `ignore_operational_co2_cap` — when true, stage does not treat high CO2 as a reason to stop the cycle or cap PID by operational band; **global** `co2_emergency_absolute_ppm` still forces ventilation.
- `co2_operational_purge_enabled` — enables **stage operational** CO2 purge (not safety). Use `purge_duration_sec` for timed operational purge windows.
- Deprecated alias (do not use in new recipes): `co2_emergency_purge_enabled` — treat as `co2_operational_purge_enabled`.

## Execution policy extensions

- `degraded_storage_policy` — e.g. `continue_active_from_ram_reduce_persistence` aligns with architecture fault-model for SD loss during `ACTIVE_RUN`.

## Light schedule

When `light_hours_per_day` > 0, `light_schedule_defaults` applies unless a stage sets overrides under `light_schedule`.

Fields:

- `photoperiod_hours` — must match or be overridden by stage `light_hours_per_day` when stage does not override.
- `window_start_local_hour` / `window_end_local_hour` — inclusive/exclusive convention: firmware uses `[start, end)` within local day; wrap across midnight if `end < start`.
- `requires_ntp` — if true and time not synced: policy `if_no_ntp` (`lights_off` | `use_monotonic_cycle_hours`).
- Civil time and timezone offset policy: [threat-model-and-time.md](../architecture/threat-model-and-time.md).

## Actuator mapping

`hardware_mapping.logical_to_physical` documents one physical fan vs two. Example: single fan = `inlet_fan` maps to `ventilation_fan_pwm_ch1`.

## Prophylactic ventilation

Under `safety_profile.harvest_first_overrides`:

| Field | Purpose |
| --- | --- |
| `prophylactic_ventilation_actuator` | Logical actuator (typically `inlet_fan`) |
| `prophylactic_ventilation_interval_hours` | Cadence |
| `prophylactic_ventilation_duration_min` | Run length per event |
| `prophylactic_ventilation_duty_percent` | PWM duty during prophylactic run |
| `prophylactic_ventilation_priority` | Arbitration order; use `below_co2_safety_above_pid` — safety (`co2_emergency_absolute_ppm`) wins; then prophylactic; then PID demand |
| `prophylactic_ventilation_blocked_when` | List of guards: e.g. `emergency_stop`, `humidifier_dry_run_lock`, `water_reserve_timer_active` (firmware-defined enum) |

Prophylactic runs **must not** override `EMERGENCY_STOP` or water dry-run lock. If `co2_emergency_absolute_ppm` fires during prophylactic, **merge** commands with `max(duty_safety, duty_prophylactic, duty_pid)` on the same actuator.
