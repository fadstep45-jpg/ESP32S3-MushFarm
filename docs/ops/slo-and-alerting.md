# SLO, Telemetry Pipeline and Alerting

## Reliability Objectives

- Climate continuity: no autonomous stop on non-fatal network/storage faults.
- Sensor freshness SLO: control decisions use data with stale age <= 10 sec.
- Command acknowledgment SLO: AP/MQTT command ack in <= 2 sec (normal mode).
- Fault notification SLO: CRITICAL alerts emitted in <= 5 sec.

## Telemetry Pipeline

## Data Types

- `sample`: periodic sensor snapshot.
- `action`: actuator command emitted by arbiter.
- `state`: state transition event.
- `fault`: detected fault + severity.
- `audit`: external command execution record.

## RAM Ring Buffer

- Structure: fixed-size circular buffer in RAM.
- Suggested capacity: 15 minutes at full sample rate minimum.
- **Control decision trace**: store **compact** arbiter records every tick; store **full** arbiter detail only on events or throttled interval (see [control-arbitration.md](../architecture/control-arbitration.md)) so average write rate stays within capacity.
- Record fields:
  - `ts_monotonic_ms`
  - `ts_wall` (if synced)
  - `type`
  - `severity`
  - `code`
  - `payload`

## Flush Policy to SD

- periodic flush every 15 minutes;
- immediate flush on buffer high watermark (for example 80%);
- forced flush on `CRITICAL` if SD available;
- write batch with checksum and sequence id.

## Backpressure and SD Failure

- if SD unavailable:
  - continue ring-buffer overwrite policy (latest data priority);
  - emit `WARN_STORAGE_DEGRADED`;
  - preserve recent `CRITICAL` entries in protected mini-buffer.

## Retention and Rotation

- folder strategy:
  - `/logs/YYYY/MM/DD/*.jsonl`
- rotate by:
  - max file size (for example 2 MB) or time slice (hourly)
- keep:
  - high-resolution telemetry: 7 days
  - fault/audit events: 30 days

## Severity and Event Codes

- `INFO_*`: normal lifecycle and recoveries.
- `WARN_*`: transient degradation.
- `ERROR_*`: sustained partial control loss.
- `CRITICAL_*`: immediate safety risk.

Examples:

- `WARN_WIFI_OFFLINE`
- `ERROR_SENSOR_SCD41_TIMEOUT`
- `CRITICAL_CO2_HARD_LIMIT`
- `INFO_FAULT_RECOVERED`

## Alert Routing

- MQTT online: publish `alert` immediately.
- MQTT offline: queue alerts in RAM and flush on reconnect.
- AP UI must show active unresolved alerts with timestamps and ack status.

## Operational Dashboards (Minimum)

- current state and selected recipe;
- sensor stale age;
- active fault codes;
- actuator duty timelines;
- ring-buffer usage and last SD flush result.
