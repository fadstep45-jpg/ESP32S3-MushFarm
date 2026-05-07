# Fault Model and Selective Degradation

## Policy
The system must remain fail-operational for non-fatal faults. Only affected functions are reduced or disabled.

## Severity Levels

- `INFO`: normal event, no action required.
- `WARN`: degraded quality, cycle continues.
- `ERROR`: partial control loss, manual attention required.
- `CRITICAL`: safety risk, immediate protective action.

## Fault Handling Pipeline

1. Detect anomaly.
2. Retry/debounce window.
3. Classify severity and affected loops.
4. Apply selective degradation.
5. Publish alert and persist event.
6. Enter observation window for recovery.

## Fault Matrix

| Subsystem | Fault Signal | Retry Policy | Degraded Action | Recovery Condition |
| --- | --- | --- | --- | --- |
| SCD41 RH/CO2 | NACK, timeout, NaN | 5 read retries in 5 sec | disable RH+CO2 auto loops, keep timer fallback for humidifier if policy enabled | 10 consecutive valid samples |
| MLX90614 | NACK, outlier delta | 5 retries + median filter | disable substrate-temperature loop only | 10 valid samples and delta within bound |
| XKC-Y25 water | stuck LOW/HIGH mismatch | debounce 10 sec + trend check RH response | keep humidifier in pulse-safe mode, do not immediate hard-lock on first anomaly | sensor state consistent for 3 checks or manual ack |
| SD card | mount/write error | remount x3 with backoff | continue run from RAM snapshot, suspend non-essential persistence | successful mount and write checksum |
| Wi-Fi | disconnected | reconnect every 30 sec | local control continues, queue telemetry | link up stable for 60 sec |
| MQTT | broker unavailable | reconnect exponential backoff | continue local/AP control, buffer alerts/telemetry | broker connected and publish ack |
| RTC/NTP | time drift/no sync | NTP retry every 10 min | use monotonic millis timestamps for runtime events | NTP sync success |
| Power 5V sag | brownout reset trend | immediate | enter `EMERGENCY_STOP` if repeated resets exceed threshold | power stable and manual restart |

## False-Positive / False-Negative Strategy

### Water Sensor (Business Priority)

- False positive `LOW` (water exists): prefer preserving humidity continuity.
- True positive `LOW` (empty tank): protect humidifier from dry-run.
- Decision policy:
  - first `LOW`: `WARN`, start reserve timer.
  - after reserve timer: pulse-safe mode (15s ON / 180s OFF).
  - if RH trend still rises, keep pulse-safe mode.
  - if RH trend flat after 3 windows, lock humidifier and raise `CRITICAL`.

### Climate Sensors

- Single bad sample never triggers emergency.
- Use rolling median and stale-age checks.
- If only one channel fails, keep unrelated channels in auto mode.

## Degraded Modes

- `DEG_SENSOR_RH_CO2`: RH/CO2 auto control off, manual/timer fallback.
- `DEG_SENSOR_TEMP`: substrate temp loop off; other loops active.
- `DEG_STORAGE`: logging reduced; ring buffer retained in RAM.
- `DEG_NETWORK`: no cloud control; AP/local control active.

## Recovery Rules

- Recovery is automatic only after stable observation window passes.
- Every auto-recovery emits `INFO` event with previous fault code.
- If same fault repeats more than `N` times/hour, require manual acknowledgment.

## Operator Notifications

- `WARN`: push once + periodic reminder every 30 min.
- `ERROR`: immediate push + repeated every 10 min.
- `CRITICAL`: immediate push, local buzzer/indicator (if available), cycle forced to safe policy.
