# mushfarm_tests — host-runnable self-tests

Standalone Arduino sketch that runs the mushfarm source files against a set of
in-memory unit tests on the ESP32-S3 itself. No hardware sensors required:
everything runs against the mock pipeline (`MF_SENSORS_MOCK=1`) and the
`mf_mock_climate` scenarios already defined in `firmware/mushfarm/`.

## What is covered

| Suite | Tests |
| --- | --- |
| Climate arbiter golden vectors | `GV1` RH low / CO₂ OK, `GV2` CO₂ high, `GV3` CO₂ crit purge, `GV4` RH max crit, `GV5/GV6` cooperative cap or sequential bias on S0, `GV7` dry tank reserve, `GV8` SCD41 disconnect → safe timer, `GV9` condensate guard |
| Fault supervisor | Sensor disconnect injection → ACTIVE_RUN → DEGRADED_RUN; recovery injection → RECOVERY_VALIDATED → ACTIVE_RUN; water LOCKED publishes nonfatal(WATER); second sensor fault while already DEGRADED_RUN still sets its warn flag |
| FSM core | pause/resume, emergency latch + ACK, latch rejects pause, long-press → SETUP_AP from IDLE_READY |
| Water policy | NORMAL → RESERVE on first LOW; mid-reserve restore → NORMAL |
| S7 cmd_dispatch + dedup | `msg_id` LRU ring + TTL expiry; `cycle/start` NOOP in ACTIVE_RUN; `recipe/select` ERR_STATE outside IDLE_READY |

Each test prints `PASS` or `FAIL` and the sketch ends with `===== Result: N PASS, M FAIL =====`.

## How to build

1. Open `firmware/mushfarm_tests/mushfarm_tests.ino` in Arduino IDE.
2. Board: **ESP32S3 Dev Module** (same as the main sketch).
3. Verify / Upload.
4. Open Serial Monitor at **115200 baud**. The test report is printed in `setup()` and the sketch then idles.

## Layout

```
mushfarm_tests/
  mushfarm_tests.ino   ← test runner & assertions
  src_*.cpp            ← thin one-line shims that include the real
                          ../mushfarm/mf_*.cpp files
  test_README.md       ← this file
```

The `src_*.cpp` shims exist because Arduino IDE only compiles files that
live in the sketch folder. The shims pull each production source file
into its own translation unit so file-local `static` variables (e.g. each
file has its own `s_state` / `s_ok` / `s_inited`) do not collide.

When new modules are added to `firmware/mushfarm/`, add a matching
`src_<name>.cpp` shim here if the tests reference that module.

## Limitations

- **Inject vs production path:** `mf_fault_supervisor_inject_fault/recovery`
  call `mf_fsm_fault_nonfatal` directly. The full S2 pipeline
  (`MF_SENSOR_READ_RETRIES` consecutive bad reads → recovery streak of
  `MF_SENSOR_RECOVERY_SAMPLES` → supervisor edge) is exercised by
  `MF_MOCK_SCENARIO_DISCONNECT` in the main sketch or on hardware, not
  by the inject helpers in this suite.
- Uses real `millis()`. Time-based tests (full PULSE_ON → PULSE_OFF cycle,
  full reserve timer elapse) are only contract-tested, not full-duration
  exercised, to keep the sketch under a few seconds. Bench testing covers
  the wall-clock paths.
- `MF_SENSORS_MOCK=1` is enforced via `mf_config.h`. The test report banner
  shows `sensors=MOCK` for clarity.
- This sketch wipes the NVS `mf_fsm` namespace via `mf_session_clear()` to
  guarantee a clean start. Do not run it on a device with an in-flight
  production grow cycle.
