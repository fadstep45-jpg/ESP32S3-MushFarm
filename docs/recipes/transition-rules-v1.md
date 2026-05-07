# Transition rules registry (v1)

Machine-facing `transition_rule` strings used in recipes. Firmware maps each id to concrete inputs (timers, GPIO, MQTT confirm, vision — future).

| `transition_rule` | Meaning | Typical inputs |
| --- | --- | --- |
| `auto_or_manual_after_casing` | After casing applied or operator confirms; else timer fallback | UI button, optional GPIO; `auto_transition_after_days` as fallback |
| `auto_after_pin_formation` | Pins visible / CV / operator confirm; else timer | Confirm topic or timer |
| `manual_harvest_stop` | No auto advance; only operator ends stage or batch | `transition_auto_enabled: false` recommended |
| `auto_or_manual_after_white_block` | Mycelium run / white block confirm; else timer | Confirm + timer fallback |
| `manual_after_browning` | Shiitake browning complete — operator only | `transition_auto_enabled: false`, timer = reminder only |
| `auto_after_cold_shock` | Cold shock completed (temp/time profile or confirm); else timer | Sensor threshold + timer |

**Policy**

- If `transition_auto_enabled: false`, `auto_transition_after_days` must use `auto_transition_after_days_role: operator_reminder_only` (no stage advance).
- If `transition_auto_enabled` omitted, default is **true** and `stage_transition_defaults.mode` applies (`event_first_timer_fallback` unless overridden).

Adding a new rule requires updating this table and the firmware enum. Recipe sources live under `recipes/*.yaml` and must validate against [recipe.schema.v1.json](recipe.schema.v1.json).
