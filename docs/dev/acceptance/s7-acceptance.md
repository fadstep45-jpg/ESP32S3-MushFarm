# S7 acceptance — MQTT remote management

Normative API: [mqtt-contract.md](../../api/mqtt-contract.md). Firmware: `mf_mqtt`, `mf_mqtt_config`, `mf_cmd_dispatch`, `mf_msg_dedup`.

## Policy

- MQTT connects only when STA is up, broker NVS is configured, and FSM is not `SETUP_AP` / `BOOT`.
- Commands share `mf_cmd_dispatch` with HTTP (S6 parity subset).
- Dedup: LRU 256 `msg_id`, TTL 24h device uptime (`mf_msg_dedup`).
- Acks published on `mf/{device_id}/ack` (single stream).
- Auth disabled in S7 (`MF_MQTT_AUTH_ENABLED=0`).
- Plain TCP port 1883 (TLS deferred to S9).

## Must-pass checklist

| # | Test | Pass criteria |
| --- | --- | --- |
| T1 | Provisioning | HTTP `POST /api/v1/config/apply` with `wifi` + `mqtt` → reboot → log `mqtt connected`; `GET /status` → `mqtt.connected=true` |
| T2 | Subscribe | Broker shows client subscribed to `mf/{device_id}/cmd/#` |
| T3 | Dedup | Two `cycle/start` with same `msg_id` → FSM transitions once; second ack without new side effect |
| T4 | Cycle | `recipe/select` + `cycle/start` → ack `state=ACTIVE_RUN`; `pause`/`resume`; `cycle/stop_emergency` → `EMERGENCY_STOP` |
| T5 | Telemetry | `mf/{device_id}/telemetry` every ~30s with `state`, sensors, `fan_pct`/`hum_pct` |
| T6 | Offline alert | Stop broker → trigger warn (e.g. sensor inject) → restart broker → alert delivered on `mf/{device_id}/alert` |
| T7 | Deferred | Publish to `mf/{id}/cmd/recipe/upload` → ack `ERR_NOT_IMPLEMENTED` |
| T8 | Unit tests | `mushfarm_tests` suite 5 (dedup capacity + TTL + cmd_dispatch) all PASS |
| T9 | CI | `arduino-cli compile` green on `firmware/mushfarm` with `MF_MQTT_ENABLE=1` |

## mosquitto examples

Replace `BROKER`, `DEVICE_ID`, and paths as needed.

```bash
# Subscribe to acks and telemetry
mosquitto_sub -h BROKER -t "mf/DEVICE_ID/ack" -v
mosquitto_sub -h BROKER -t "mf/DEVICE_ID/telemetry" -v

# Select recipe and start cycle
mosquitto_pub -h BROKER -t "mf/DEVICE_ID/cmd/recipe/select" \
  -m '{"msg_id":"r1","api_ver":"1.0","payload":{"recipe_id":"embedded_demo"}}'
mosquitto_pub -h BROKER -t "mf/DEVICE_ID/cmd/cycle/start" \
  -m '{"msg_id":"c1","api_ver":"1.0","payload":{}}'

# Duplicate msg_id (second should not restart cycle)
mosquitto_pub -h BROKER -t "mf/DEVICE_ID/cmd/cycle/start" \
  -m '{"msg_id":"c1","api_ver":"1.0","payload":{}}'
```

## HTTP provisioning (mqtt block)

```bash
curl -s -X POST "http://DEVICE_IP/api/v1/config/apply" \
  -H "Content-Type: application/json" \
  -d '{"wifi":{"ssid":"MyHome","password":"secret"},"mqtt":{"broker":"192.168.1.10","port":1883,"username":"","password":"","device_id":"mushfarm-lab01"}}'
```

## Deferred (explicit `ERR_NOT_IMPLEMENTED`)

- `recipe/upload`, `delete`, `rename`
- `cycle/next_stage`, `prev_stage`, `patch_stage`
- `service/test-actuator` over MQTT
- `mqtts://`, token auth, strict `ts` anti-replay (S9)
