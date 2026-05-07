# MQTT Control Contract

## Scope
MQTT is used for remote management and telemetry. All control messages are versioned and acknowledged.

## Envelope

```json
{
  "msg_id": "uuid-v4",
  "ts": 1710000000,
  "api_ver": "1.0",
  "device_id": "mushfarm-001",
  "payload": {}
}
```

## Topics

- `mf/{device_id}/cmd/+` - incoming commands.
- `mf/{device_id}/ack/{msg_id}` - command ack/nack (per-command topic; clients subscribe with wildcards).
- `mf/{device_id}/ack` - optional **single-stream** alternative: publish ack JSON here only; `msg_id` inside payload (preferred if broker ACL or subscription limits are tight).
- `mf/{device_id}/evt/+` - lifecycle/events.
- `mf/{device_id}/telemetry` - sampled data.
- `mf/{device_id}/alert` - warning/error/critical alerts.

**Broker policy**: do not set **retained** messages on `cmd` topics. ACL SHOULD deny RETAIN on `mf/+/cmd/#`.


## Recipe Lifecycle Commands

- `cmd/recipe/list`
  - request: `{ "page": 1, "page_size": 20 }`
  - response payload: `{ "items": [{"recipe_id":"r1","name":"fruiting_v1","rev":3}], "total": 1 }`
- `cmd/recipe/get`
  - request: `{ "recipe_id": "r1" }`
- `cmd/recipe/upload`
  - request: `{ "name": "fruiting_v2", "content": "<string YAML or JSON object>" }`
  - **Limits (normative defaults)**:
    - Maximum MQTT **application** payload for any command: **65536 bytes** (64 KiB); firmware MAY advertise a lower `max_cmd_payload` in `telemetry`/`status`.
    - `recipe/upload` decoded recipe document after parse: **49152 bytes** (48 KiB) uncompressed body limit unless device advertises higher.
    - Reject with `ERR_SCHEMA_INVALID` or dedicated `ERR_PAYLOAD_TOO_LARGE` when exceeded.
  - **Chunking**: if larger recipes are required later, define `cmd/recipe/upload_begin|chunk|commit` in a minor API revision; v1 single-message upload must stay within limits above.
- `cmd/recipe/delete`
  - request: `{ "recipe_id": "r1" }`
- `cmd/recipe/rename`
  - request: `{ "recipe_id": "r1", "new_name": "fruiting_v1_legacy" }`
- `cmd/recipe/select`
  - request: `{ "recipe_id": "r2" }`
  - side effect: updates `selected_recipe_id`.
- `cmd/cycle/start`
  - request: `{ "recipe_id": "r2" }` or empty when selected exists.
  - guard: selected recipe must exist and validate.
  - side effect: create immutable `current_recipe` runtime snapshot.
- `cmd/cycle/pause`
- `cmd/cycle/resume`
- `cmd/cycle/stop_emergency`
- `cmd/cycle/next_stage`
  - allowed from `ACTIVE_RUN`, `DEGRADED_RUN`; reject from `PAUSED_SAFE` with `ERR_STATE` unless policy explicitly allows advance-while-paused (default: reject).
- `cmd/cycle/prev_stage`
  - same guards as `next_stage`; validate target index against recipe bounds and [transition-rules-v1.md](../recipes/transition-rules-v1.md) for the active stage.
- `cmd/cycle/patch_stage`
  - request: `{ "stage_idx": 2, "patch": {"target_rh": 90} }`

## ACK and Error Model

### Success

```json
{
  "msg_id": "same-as-request",
  "status": "ok",
  "code": "ACK_OK",
  "state": "ACTIVE_RUN",
  "payload": {}
}
```

### Error

```json
{
  "msg_id": "same-as-request",
  "status": "error",
  "code": "ERR_RECIPE_NOT_SELECTED",
  "detail": "Select recipe before cycle/start",
  "state": "IDLE_READY"
}
```

## Standard Error Codes

- `ERR_SCHEMA_INVALID`
- `ERR_STATE`
- `ERR_RECIPE_NOT_FOUND`
- `ERR_RECIPE_NOT_SELECTED`
- `ERR_RECIPE_VALIDATION`
- `ERR_SENSOR_MINSET`
- `ERR_SAFETY_LIMIT`
- `ERR_STORAGE_UNAVAILABLE`
- `ERR_NOT_AUTHORIZED`
- `ERR_PAYLOAD_TOO_LARGE`

## Idempotency Requirements

- `cmd/recipe/select` with current `recipe_id` returns `ACK_NOOP`.
- `cmd/cycle/start` in `ACTIVE_RUN` returns `ACK_NOOP`.
- `cmd/cycle/pause` in `PAUSED_SAFE` returns `ACK_NOOP`.
- Duplicate `msg_id` must not execute side effects twice.
- **Dedup store**: keep bounded **LRU** of processed `msg_id` values (minimum **256** entries) with **TTL wall or monotonic window** (minimum **24 h** of device uptime); on overflow evict oldest. Re-send with unknown old id after TTL MAY execute again (client responsibility).

## Replay and clock skew

- Envelope `ts` is Unix seconds; reject commands with `ts` older than **300 s** below device’s trusted clock or newer than **60 s** ahead (configurable). Requires NTP or RTC for strict mode.
- Optional `nonce` field (string, once per write) when anti-replay policy enabled.

## Security Baseline

- Shared secret token in command payload or MQTT username/password.
- Reject control commands without token when policy requires auth.
- All write commands must be audit-logged in event stream.
- **Normative reference**: [threat-model-and-time.md](../architecture/threat-model-and-time.md) (TLS, retained-msg ban, rotation).
