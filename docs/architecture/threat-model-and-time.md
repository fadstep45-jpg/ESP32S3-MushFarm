# Threat model (control plane) and wall-clock policy

## Scope

Remote and local **control planes** (MQTT, AP HTTP) and **time sources** used for photoperiod and timestamps. Climate safety limits remain authoritative regardless of network trust.

## Assets

- Device configuration (Wi-Fi, MQTT, secrets).
- Recipe library and `selected_recipe_id` / runtime snapshot.
- Operator commands (start, pause, emergency, stage advance).
- Audit and telemetry streams.

## Adversaries (baseline)

- **Passive network observer** on untrusted LAN or path to broker.
- **Active MITM** on same segments if TLS is absent or misvalidated.
- **Broker or subscriber compromise** reading topics and retained messages.
- **Physical proximity attacker** on AP SSID (weak or leaked passphrase).

## Threat model (baseline deployment)

| Threat | Mitigation (normative) | Document / notes |
| --- | --- | --- |
| Eavesdropping on commands | Prefer **TLS** to broker (`mqtts://`); pin CA or use system trust per product policy | mqtt-contract |
| Credential theft from device | Store secrets in **NVS / flash encryption** where platform supports it; no secrets in logs | firmware (out of repo) |
| Replay of captured commands | **Monotonic `ts` + sliding window** per client id; reject stale `ts`; optional **nonce** in payload for writes | mqtt-contract |
| Duplicate delivery | **`msg_id` dedup** with bounded LRU + TTL | mqtt-contract |
| Unauthorized writer | **Shared secret** minimum; prefer **per-device password** or **mTLS** for production | mqtt-contract |
| Retained poisoned `cmd` | **Do not use MQTT retained** on command topics; broker ACL deny RETAIN on `mf/+/cmd/#` | broker policy |
| AP session hijack | Session token or digest auth when auth enabled; HTTPS if stack allows | ap-contract |

## Secret handling

- **Rotation**: changing MQTT password or API token must require **restart-class apply** (see [service-mode-contract.md](service-mode-contract.md)) unless hot-rotate is explicitly implemented.
- **Logging**: never emit full tokens; audit log records `token_id` hash or last 4 chars only.

## Time and timezone (`light_schedule`)

### Goals

- Local hour windows (`window_start_local_hour`, `window_end_local_hour`) must map to a defined civil-time interpretation.
- Monotonic time remains the source for control deadlines, debounce, and PID dt.

### Policy

1. **Wall clock**: sourced from RTC + **NTP** when available.
2. **Timezone offset**: stored configuration `tz_offset_minutes` (integer, `-720`..`840`) applied to UTC to obtain **local civil time** for light windows. **DST is not inferred** unless a future `tz_olson` field is added; growers set offset explicitly when DST changes.
3. **`requires_ntp: true`**: if NTP never succeeds, follow recipe `if_no_ntp` (`lights_off` | `use_monotonic_cycle_hours`) per [recipe-schema-v1.md](../recipes/recipe-schema-v1.md).
4. **Audit timestamps**: persist `ts_utc` when synced and `ts_monotonic_ms` always for ordering across reboots.

### Failure modes

- NTP flapping: use **hysteresis** (e.g. require 2 consecutive good syncs) before switching photoperiod mode.
- Large step correction: clamp light schedule jumps — optional one-shot **skip** or **extend** current window (firmware policy flag, document in release notes when added).
