# S5 acceptance — climate control and arbitration

Normative spec: [control-arbitration.md](control-arbitration.md). Firmware modules: `mf_control_profile`, `mf_control_limits`, `mf_pid`, `mf_loop_rh`, `mf_loop_co2`, `mf_loop_temp`, `mf_climate_arbiter`, `mf_climate_trace`.

## Checklist (phase A — code + mock)

| # | Requirement (control-arbitration) | Module | Verify |
| --- | --- | --- | --- |
| 1 | Independent RH / CO2 / TEMP loop demands | `mf_loop_*` | Golden vectors below |
| 2 | Hard limits override PID | `mf_control_limits` | GV3, GV4 |
| 3 | Combiner `max()` fan/hum | `mf_climate_arbiter` | GV1, GV2 |
| 4 | Cooperative RH+CO2 cap | `mf_climate_arbiter` | GV5 |
| 5 | Sequential bias when `allow_parallel_inlet_and_humidifier: false` | `mf_climate_arbiter` | GV6 |
| 6 | Water dry-run → hum off | `mf_climate_arbiter` | GV7 |
| 7 | Missing RH → RH loop off; missing CO2 → CO2 off | `mf_loop_*` | GV8 |
| 8 | Both RH+CO2 missing → safe timer fan min | `mf_climate_tick` | GV9 |
| 9 | Compact trace every tick; full on reason/limit change | `mf_climate_trace` | Serial / batch log |
| 10 | FSM: no climate in BOOT/IDLE; PAUSED/ESTOP policies | `mf_climate_tick` | Manual FSM test |
| 11 | `gHardLimitsSafe` uses profile limits | `mf_control_limits` | GV3 |
| 12 | Embedded profile from recipe stage | `mf_control_profile` | Boot log on stage change |

## Golden vectors (mock)

Set `MF_MOCK_CLIMATE_SCENARIO` in `mf_config.h` or cycle scenarios at runtime via `mf_mock_climate_set_scenario()`.

| ID | Scenario | Mock inputs (approx) | Expected `arb_reason_code` | Fan % | Hum % |
| --- | --- | --- | --- | --- | --- |
| GV1 | RH low, CO2 OK | rh=70, target=92, co2=800 | `ARB_NORMAL` or RH demand | ≥ min fan | > 0 |
| GV2 | RH OK, CO2 high | rh=92, co2=2500, target=700 | `ARB_NORMAL` / CO2 | > min | low |
| GV3 | CO2 crit | co2=8500 | `ARB_CO2_CRIT_PURGE` | 100 | 0 or water-safe |
| GV4 | RH max crit | rh=99.5, max_crit=99 | `ARB_RH_MAX_CRIT` | — | capped 0 |
| GV5 | Coop cap | rh low + co2 high + parallel false | `ARB_COOP_HUM_CAP` | high | ≤ hum_cooperate_cap |
| GV6 | Sequential bias | stage S0 `allow_parallel=false` | alternates `ARB_SEQ_BIAS_FAN` / `ARB_SEQ_BIAS_HUM` | varies | varies |
| GV7 | Dry tank | water=0 | hum=0 | — | 0 |
| GV8 | SCD41 fault | `MF_MOCK_SCENARIO_DISCONNECT` | RH/CO2 loops disabled | temp/min policy | 0 |
| GV9 | Condensate | delta > max_substrate_air_delta | `ARB_CONDENSATE_GUARD` | raised floor | hum cap |

## Phase B (bench — requires hardware)

**Status:** not executed in repo — run when ESP32 + sensors + MOSFET + grow chamber are assembled.

| ID | Task | Pass criteria |
| --- | --- | --- |
| B1 | Pin smoke (fan GPIO3, hum 42, light 47) | Each MOSFET responds; no fan glitch at reset if gate pull-down present |
| B2 | S2 LIVE I2C | `sensors=LIVE`, SCD41+MLX readings stable, stale/retry logs sane |
| B3 | RH PID tuning | RH holds within ±`rh_hysteresis` for 30+ min without oscillation |
| B4 | CO2 PID tuning | CO2 returns below target band after disturbance; trust clamp at 5000 ppm |
| B5 | TEMP + condensate | Substrate overheat triggers exhaust; condensate scenario clears without RH runaway |
| B6 | Cooperative / pinning | At RH 98% + high CO2, no condensation on SCD41 (visual check) |
| B7 | Soak DoD | 4–8 h run: no `co2_crit` without cause; trace shows expected `arb_reason_code` mix |

After B3–B5, update `MF_PID_*` constants in `firmware/mushfarm/mf_config.h` and note values in this file or a bench log.
