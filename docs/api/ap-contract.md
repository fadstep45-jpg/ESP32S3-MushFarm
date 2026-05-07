# AP HTTP Contract

## Scope
AP portal is the local management plane. API mirrors MQTT semantics for consistency.

## Versioning

- Base path: `/api/v1`
- All responses include:
  - `request_id`
  - `status` (`ok` or `error`)
  - `code`
  - `state`

## Recipe Library Endpoints

- `GET /api/v1/recipes`
  - list recipe metadata.
- `GET /api/v1/recipes/{recipe_id}`
  - full recipe payload.
- `POST /api/v1/recipes`
  - upload new recipe.
  - **Body limit**: same as MQTT — request body **≤ 65536 bytes** before gzip; decoded recipe document **≤ 49152 bytes** unless device advertises otherwise. Excess → `ERR_PAYLOAD_TOO_LARGE` / `ERR_SCHEMA_INVALID`.
- `PATCH /api/v1/recipes/{recipe_id}`
  - rename or metadata patch.
- `DELETE /api/v1/recipes/{recipe_id}`
  - delete recipe if not active.
- `POST /api/v1/recipes/{recipe_id}/select`
  - set `selected_recipe_id`.

## Cycle Control Endpoints

- `POST /api/v1/cycle/start`
  - body: `{ "recipe_id": "optional" }`
  - rejects start without valid selected/uploaded recipe.
- `POST /api/v1/cycle/pause`
- `POST /api/v1/cycle/resume`
- `POST /api/v1/cycle/stop-emergency`
- `POST /api/v1/cycle/next-stage`
  - same state guards as MQTT `cmd/cycle/next_stage`.
- `POST /api/v1/cycle/prev-stage`
  - same state guards as MQTT `cmd/cycle/prev_stage`.
- `PATCH /api/v1/cycle/stage/{stage_idx}`

## Service Endpoints

- `GET /api/v1/status`
  - state, faults, selected recipe, active stage.
- `GET /api/v1/sensors/live`
  - last valid readings + stale age.
- `POST /api/v1/service/test-actuator`
  - manual test with timeout safety lock.
- `POST /api/v1/config/apply`
  - accepts hot-apply and restart-required keys.
- `POST /api/v1/config/factory-reset`
  - requires explicit confirmation token.

## Response Examples

### Success

```json
{
  "request_id": "d8a2",
  "status": "ok",
  "code": "ACK_OK",
  "state": "IDLE_READY",
  "payload": {}
}
```

### Error

```json
{
  "request_id": "d8a2",
  "status": "error",
  "code": "ERR_STATE",
  "detail": "Cannot delete active recipe"
}
```

## Validation Rules

- Recipe schema must pass before upload is committed.
- `cycle/start` requires `selected_recipe_id` or valid `recipe_id` in request.
- Mutating endpoints must be authenticated when auth policy is enabled.
- Control-plane hardening: see [threat-model-and-time.md](../architecture/threat-model-and-time.md).

## UI Workflow Requirements

- UI flow is mandatory: `list -> select/upload -> validate -> start`.
- Every mutating action surfaces explicit ack/error in UI.
- UI must show stale sensor badge when data age exceeds policy threshold.
