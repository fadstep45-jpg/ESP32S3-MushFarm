# Climate Control Arbitration

## Objective
Run humidity and CO2 loops independently while enforcing hard safety limits and predictable conflict resolution.

## Control Loops

- `RH_LOOP`: controls humidifier duty.
- `CO2_LOOP`: controls exhaust duty (in recipes this may be named `inlet_fan` when a single physical fan handles ventilation; `hardware_mapping` binds logical name to PWM channel).
- `TEMP_LOOP` (optional): controls thermal mitigation based on available sensors.

Each loop computes desired actuator command every control tick from its own error signal.

## Arbitration Model

1. Compute loop outputs independently.
2. Apply hard-limit overrides.
3. Apply actuator-level combiner rules.
4. Emit final commands.

## Hard Limits (Safety First)

- `CO2_CRIT_PPM` / recipe `co2_emergency_absolute_ppm`: hard emergency purge threshold (recipes use e.g. 8000 ppm for fail-safe; stage flags cannot disable this).
- `RH_MIN_CRIT`: lower humidity emergency threshold.
- `RH_MAX_CRIT`: upper humidity emergency threshold.
- `TEMP_SUB_CRIT`: substrate overheat threshold.

Hard-limit events bypass normal PID weighting and can preempt less critical actions.

## Actuator Combiner Rules

- Exhaust fan command:
  - `max(co2_demand, temp_demand, safety_override_exhaust)`
- Humidifier command:
  - `max(rh_demand, safety_override_humidifier)` with dry-run constraints from water policy.
- If both exhaust and humidifier are high simultaneously, default is **stability-first** (parallel demands), subject to **cooperative caps** below.

## Cooperative arbitration (physics and plant health)

Pure `max()` combining can over-ventilate while saturating RH, encourage **condensation** on cold surfaces (sensor head, walls), and waste water. After independent loop demands are computed:

1. **RH slew cap**: limit humidifier **increase rate** per minute to a recipe or profile value (e.g. derived from `rh_hysteresis_percent` and chamber time constant); never violate hard `RH_MAX_CRIT` / `RH_MIN_CRIT`.
2. **Concurrent high-RH + high-exhaust window**: when `rh_demand` and `co2_demand` both exceed configured thresholds **and** exhaust is above `exhaust_concurrent_rh_cooperate_pct` (profile default e.g. 40%), apply **humidifier duty cap** `hum_cooperate_cap_pct` for bounded seconds unless `CO2_CRIT_PPM` is active (safety always wins).
3. **Condensate guard**: if **substrate/air delta** or enclosure dew-point estimate (when sensors allow) exceeds policy, temporarily **raise minimum exhaust floor** and **cap humidifier** until risk clears.
4. **Stage opt-in**: `stages[].arbitration.allow_parallel_inlet_and_humidifier: true` preserves aggressive parallel behaviour; when `false`, use **sequential bias** (e.g. alternate priority every N ticks) while CO2 is non-critical.

## Conflict Policies

### RH low and CO2 high

- Default: run both loops in parallel.
- If `CO2 >= CO2_CRIT_PPM`, force emergency purge profile:
  - exhaust to 100% for bounded window;
  - humidifier allowed only if water-safe policy permits.

### RH acceptable and CO2 high

- prioritize exhaust until CO2 returns below control band.

### RH low and CO2 acceptable

- prioritize humidification with minimum required ventilation.

## Anti-Oscillation Controls

- Per-loop hysteresis bands.
- Command slew-rate limit to avoid fast toggling.
- Minimum on/off dwell times for relay-protected loads.
- Integrator freeze when actuator saturated by hard-limit override.

## Fallback Behavior

- Missing RH data -> disable `RH_LOOP`, keep `CO2_LOOP`.
- Missing CO2 data -> disable `CO2_LOOP`, keep `RH_LOOP`.
- Missing both -> switch to safe timer policy and raise `ERROR`.

## Decision Trace (Required Logging)

**Full detail** (all fields below) is required **at least** when any of: hard-limit active, `arb_reason_code` changes, fault window open, or on a **slow cadence** (e.g. every **10** control ticks).

Each **control tick** must still persist a **compact record**: `ts_monotonic_ms`, final actuator outputs, `arb_reason_code`, worst stale-age among channels used.

Full-detail fields (when emitted):

- raw sensor values and stale age,
- loop demands (`rh_demand`, `co2_demand`, `temp_demand`),
- applied hard-limits and cooperative caps hit,
- final actuator outputs,
- arbitration reason code (example `ARB_CO2_CRIT_PURGE`).

See [slo-and-alerting.md](../ops/slo-and-alerting.md) for ring-buffer implications.
