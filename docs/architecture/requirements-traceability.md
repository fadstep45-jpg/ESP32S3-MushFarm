# Requirements traceability

## Purpose

Maps normative architecture documents to **expected firmware modules** and **verification hooks**. Until source lives in this repository, module names are a stable contract for implementation and reviews.

| Document | Primary responsibility | Suggested firmware module(s) | Verification (tests / checks) |
| --- | --- | --- | --- |
| [state-machine.md](state-machine.md) | Lifecycle states, transition guards, idempotency | `mf_fsm` (table `k_transitions[]`), `mf_climate`, `mf_actuators`, `mf_service_btn` | Table-driven unit tests: every `(state, event, guard)` row; property: no `ACTIVE_RUN` without recipe snapshot; emergency latch + `SETUP_AP` long-press per matrix |
| [fault-model.md](fault-model.md) | Severity, debounce, degradation, recovery windows | `mf_sensor_scd41`, `mf_sensor_mlx90614`, `mf_sensor_water` (S2 pipeline); future `fault_supervisor` | Fault injection tests per subsystem row; recovery counter tests; mock rare NaN path in sensors |
| [control-arbitration.md](control-arbitration.md) | Loop outputs, hard limits, combiner, anti-windup | `climate_arbiter`, `pid_rh`, `pid_co2` | Golden-vector tests: sensor tuples → expected duties + `arb_reason_code` |
| [service-mode-contract.md](service-mode-contract.md) | Service classes, hot vs restart apply, manual test bounds | `service_mode`, `actuator_test_runner` | Tests: manual test mutex with auto loop; timeout rollback; deny when hard-limit active |
| [../api/mqtt-contract.md](../api/mqtt-contract.md) | MQTT envelope, topics, commands, idempotency | `mqtt_command`, `mqtt_ack` | Protocol tests: duplicate `msg_id`, oversized upload, ACL assumptions |
| [../api/ap-contract.md](../api/ap-contract.md) | HTTP parity with MQTT | `ap_http_api`, `request_id` correlator | API contract tests mirroring MQTT command matrix |
| [../recipes/recipe-schema-v1.md](../recipes/recipe-schema-v1.md) | Field semantics, CO2 layers, transitions | `recipe_parser`, `recipe_validator` | CI: validate `recipes/*.yaml` against [recipe.schema.v1.json](../recipes/recipe.schema.v1.json) |
| [../recipes/transition-rules-v1.md](../recipes/transition-rules-v1.md) | Registry of `transition_rule` ids | `mf_recipe` (embedded demo stages S0/S1 + NVS checkpoint) | Enum parity test: registry keys match firmware enum |
| [../ops/slo-and-alerting.md](../ops/slo-and-alerting.md) | Buffers, flush, alert routing | `telemetry_ring`, `sd_logger`, `alert_router` | Soak / capacity test against stated SLOs where feasible |
| Roadmap **S1** (heap + OTA scheme note) | Boot observability; partition policy for future OTA | `mf_resources`, [arduino-partition-ota.md](../ops/arduino-partition-ota.md) | Boot log includes heap + largest block; developer picks OTA-capable scheme from linked doc |
| Camera subsystem (S8.5) — cosmetic | Timelapse + low-FPS service-mode preview per [service-mode-contract.md](service-mode-contract.md) "Camera Behavior" and [state-machine.md](state-machine.md) "Camera Behavior per State". **Не control-input**: отказ обрабатывается как `WARN_CAMERA_FAIL` (sticky-флаг, без FSM-переходов и без degraded mode) — см. [fault-model.md](fault-model.md) "Non-control auxiliary features". | `mf_camera` (stub today with `MF_CAMERA_ENABLE=0`, real driver in S8.5); model OV2640 / OV3660 / OV5640 TBD | Smoke-test: `mf_camera_init()` возвращает OK и не валит main loop при переключении `MF_CAMERA_ENABLE`. Изоляция: симуляция отказа камеры (FFC unplugged / SCCB NACK / frame timeout) → `WARN_CAMERA_FAIL` установлен, FSM не двигается, PID/датчики работают штатно. |

## Cross-cutting concerns

| Concern | Documents | Module touchpoints |
| --- | --- | --- |
| `current_recipe` immutability | state-machine, recipe-schema | `recipe_runtime`, `runtime_fsm` |
| Hard CO2 safety | recipe-schema, control-arbitration, fault-model | `climate_arbiter`, `fault_supervisor` |
| Time / TZ for lights | recipe-schema, [threat-model-and-time.md](threat-model-and-time.md) (normative) | `clock_manager`, `light_scheduler` |

## Maintenance rule

When a document changes behavior:

1. Update this matrix if module boundaries or test type shift.
2. Add or adjust a test name in firmware CI (once present) and reference the doc section in the test comment.
