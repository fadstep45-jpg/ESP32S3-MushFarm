# S6 acceptance — AP/HTTP local management

Normative API: [ap-contract.md](../../api/ap-contract.md). Firmware: `mf_net_config`, `mf_wifi`, `mf_http_api`, `mf_actuator_test`.

## Policy

- SoftAP `MushFarm_Setup` @ `192.168.4.1` **only** while FSM is `SETUP_AP` (no creds, long-press service, or post factory-reset).
- In `IDLE_READY` / run states: **STA** only; HTTP on `wifi.ip` from `/status`.
- Auth disabled in S6 (`MF_HTTP_AUTH_ENABLED=0`).

## Must-pass checklist

| # | Test | Pass criteria |
| --- | --- | --- |
| M1 | First boot (no NVS creds) | FSM `SETUP_AP`; SoftAP visible; `GET http://192.168.4.1/api/v1/status` → `"state":"SETUP_AP"` |
| M2 | Wi-Fi provisioning | `POST /api/v1/config/apply` with `{"wifi":{"ssid":"…","password":"…"}}` → reboot → STA connected; `status.wifi.sta_connected=true` |
| M3 | Sensors | `GET /api/v1/sensors/live` returns scd41/mlx90614/water fields |
| M4 | Recipe + cycle | `GET /recipes` lists `embedded_demo`; `POST …/select`; `POST /cycle/start` → `ACTIVE_RUN`; pause/resume; `POST /cycle/stop-emergency` → `EMERGENCY_STOP` |
| M5 | Factory reset | `POST /api/v1/config/factory-reset` body `{"confirm_token":"FACTORY_RESET"}` → reboot → `SETUP_AP` |
| M6 | SNTP | After STA up, `time_synced` becomes true within ~2 min |
| M7 | Actuator test | `POST /api/v1/service/test-actuator` `{"actuator":"fan","percent":50,"timeout_s":5}` → rolls back after timeout |
| M8 | CI | `arduino-cli compile` green on `firmware/mushfarm` |

## curl examples (SETUP_AP)

```bash
curl -s "http://192.168.4.1/api/v1/status?request_id=t01"
curl -s "http://192.168.4.1/api/v1/sensors/live"
curl -s -X POST "http://192.168.4.1/api/v1/config/apply" \
  -H "Content-Type: application/json" \
  -d '{"wifi":{"ssid":"MyHome","password":"secret"}}'
```

## curl examples (STA)

Replace `DEVICE_IP` with `wifi.ip` from status.

```bash
curl -s "http://DEVICE_IP/api/v1/status"
curl -s -X POST "http://DEVICE_IP/api/v1/recipes/embedded_demo/select"
curl -s -X POST "http://DEVICE_IP/api/v1/cycle/start" -H "Content-Type: application/json" -d '{}'
```

## Deferred (explicit `ERR_NOT_IMPLEMENTED`)

- `POST /api/v1/recipes` (upload), `PATCH`/`DELETE` recipes
- `POST /api/v1/cycle/next-stage`, `prev-stage`, `PATCH …/stage/{idx}`
- HTTPS / API token auth (S9)
