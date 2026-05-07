# Beta checklist (Sprint 10)

Use before field trial. Items map to the sprint roadmap and `docs/architecture/requirements-traceability.md`.

## Hardware

- [ ] Power path: 5 V rail within spec, grounds star-connected per `docs/Подключение компонентов.md`.
- [ ] I2C pull-ups present; SDA/SCL match `mf_board.h` or updated Kconfig pins.
- [ ] SCD41, MLX90614, XKC-Y25 wired; water sensor polarity verified.
- [ ] Fan / humidifier / light MOSFET wiring matches PWM GPIOs or updated config.

## Firmware image

- [ ] `PROJECT_VER` and git SHA logged at boot.
- [ ] `MF_SENSORS_MOCK` disabled for real I2C validation.
- [ ] Climate loop rate and watchdog behaviour observed under load.

## Network and API

- [ ] SoftAP connects from phone/laptop; `GET http://192.168.4.1/api/v1/status` returns JSON with expected `state`.
- [ ] MQTT (if enabled): broker reachable; duplicate `msg_id` does not double-apply commands.

## Safety

- [ ] Long-press service button triggers `EMERGENCY_STOP`; outputs go to safe duty.
- [ ] `evEmergencyAcknowledged` path tested from HTTP/MQTT when implemented.

## Soak

- [ ] Minimum 24 h continuous run without reset; review ring-buffer / SD flush if enabled.

## Sign-off

- [ ] Known limitations documented for this beta tag.
