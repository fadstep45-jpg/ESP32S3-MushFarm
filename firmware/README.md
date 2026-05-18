# MushFarm firmware (Arduino IDE)

Прошивка ESP32-S3 для грибной фермы. Локальная разработка ведётся в **Arduino IDE 2.x**, без ESP-IDF и без явных вызовов FreeRTOS API.

## Что установить

- Arduino IDE 2.x
- ESP32 core (Boards Manager) — версия в [arduino-version.txt](arduino-version.txt)
- Библиотеки из [arduino-libs.txt](arduino-libs.txt) (через Library Manager или `arduino-cli`)

## Шаги (Arduino IDE)

1. Открыть скетч [mushfarm/mushfarm.ino](mushfarm/mushfarm.ino).
2. Tools → Board → `ESP32S3 Dev Module`.
3. Board settings (минимум):
   - `Flash Mode: QIO`
   - `Flash Size: 8 MB (или по плате)`
   - `Partition Scheme: Default 4MB with spiffs`
   - `PSRAM: OPI PSRAM` (если плата с PSRAM)
   - `USB CDC On Boot: Enabled` (для логов через Serial Monitor)
4. **Verify** для сборки, **Upload** для прошивки, **Serial Monitor** на 115200 — для логов.

### Отдельный boot self-check (S2.5)

- Для быстрой стендовой диагностики можно открыть отдельный скетч [selfcheck_boot/selfcheck_boot.ino](selfcheck_boot/selfcheck_boot.ino).
- Он один раз на старте проверяет доступность датчиков (AUTO) и по очереди запускает актуаторы на 50% на 2 секунды (NEEDS_OPERATOR).
- Тест увлажнителя автоматически пропускается при `DRY` состоянии датчика воды (защита от сухого хода).

### Service console GCODE (S3.5)

- Для ручной диагностики можно открыть [service_console_gcode/service_console_gcode.ino](service_console_gcode/service_console_gcode.ino).
- Serial 115200, команды:
  - `M114` — телеметрия (SCD41/MLX90614/water + текущие мощности актуаторов).
  - `M106 S<0-100>` — вентилятор.
  - `M140 S<0-100>` — увлажнитель (с `DRY_RUN_GUARD` при пустом баке).
  - `M150 S<0-100>` — свет.
  - `M112` — аварийный стоп (latched).
  - `M999` — снять latched-режим аварийного стопа.
- `autorollback` по таймауту отключён: тест актуаторов может длиться дольше 30 секунд.

## Что выводится при старте

- Версия прошивки `MF_FW_VERSION` (см. `mushfarm/mf_config.h`).
- Короткий git SHA, если CI его подставил, иначе `unknown`.
- Карта пинов и список включённых модулей.
- **S1:** строка `heap free=… min_free=… largest_block=… flash_chip=…` (`mf_resources.cpp`).
- **S2.5:** строка `sensors=MOCK` или `sensors=LIVE` по флагу `MF_SENSORS_MOCK`.

## OTA и partition scheme (S1)

Нормативная заметка для Arduino IDE: [docs/ops/arduino-partition-ota.md](../docs/ops/arduino-partition-ota.md).

## S6 — AP/HTTP

Модули: `mf_net_config` (NVS Wi‑Fi), `mf_wifi` (SoftAP только в `SETUP_AP`, STA в штатном режиме), `mf_http_api` (`WebServer` на порту 80), `mf_actuator_test`.

- Без сохранённых creds: boot → `SETUP_AP` + AP `MushFarm_Setup` @ `192.168.4.1`.
- Provisioning: `POST /api/v1/config/apply` с `{"wifi":{"ssid","password"}}` → reboot.
- Чеклист и curl: [docs/architecture/s6-acceptance.md](../docs/architecture/s6-acceptance.md).
- Флаги: `MF_WIFI_SOFTAP`, `MF_HTTP_API`, `MF_CLOCK_SNTP_ENABLED` в `mf_config.h`.

## S7 — MQTT

Модули: `mf_mqtt_config` (NVS broker), `mf_mqtt` (PubSubClient), `mf_cmd_dispatch` (общий с HTTP), `mf_msg_dedup`.

- Provisioning: `POST /api/v1/config/apply` с `mqtt: { broker, port, username, password, device_id }` (вместе с `wifi` или после).
- Топики: `mf/{device_id}/cmd/#`, ack на `mf/{device_id}/ack`, telemetry ~30 с, alerts `mf/{device_id}/alert`.
- Чеклист: [docs/architecture/s7-acceptance.md](../docs/architecture/s7-acceptance.md).
- Флаги: `MF_MQTT_ENABLE`, `MF_MQTT_TELEMETRY_MS` в `mf_config.h`.

## S5 — климат и арбитраж

Модули: `mf_control_profile`, `mf_control_limits`, `mf_pid`, `mf_loop_rh` / `mf_loop_co2` / `mf_loop_temp`, `mf_climate_arbiter`, `mf_climate_trace`, обёртка `mf_climate`.

- Тик климата: `MF_TICK_CLIMATE_MS` (2 с) в `mf_config.h`.
- Setpoints/limitы — embedded-профиль по стадии demo (`mf_control_profile_load_demo_stage`), без YAML на устройстве.
- В RAM ring (`mf_batch_logger`) каждый climate-tick пишется compact-строка `arb …`; полный trace — при смене `arb_reason_code` или каждые 10 тиков.
- Mock golden vectors: `MF_MOCK_CLIMATE_SCENARIO` в `mf_config.h` (см. [docs/architecture/s5-acceptance.md](../docs/architecture/s5-acceptance.md)).

## Камера

Целевая плата физически содержит модуль камеры (FFC 24-pin), модель — OV2640 / OV3660 / OV5640, финализируется на этапе закупки. В firmware есть только stub-модуль `mf_camera.{h,cpp}`, реальный драйвер появится отдельным спринтом.

- Включается флагом `MF_CAMERA_ENABLE` в `mushfarm/mf_config.h` (по умолчанию `0`).
- При `MF_CAMERA_ENABLE 0` в boot-логе строка `camera=disabled (stub)`.
- При `MF_CAMERA_ENABLE 1` сейчас тоже stub (`camera=stub`), без реального захвата кадров — placeholder для будущего драйвера.
- Конкретные GPIO для DVP/SCCB пока **TBD**, см. `docs/Подключение компонентов.md` раздел "Камера (DVP)".

## Архитектура

- **Без FreeRTOS API в нашем коде.** Под капотом ESP32 Arduino core всё равно держит FreeRTOS (это ограничение платформы), но из нашего кода никаких `xTaskCreate`, очередей, семафоров — только `setup()`/`loop()` и кооперативный планировщик на `millis()` (см. `mf_scheduler.h`).
- Все модули организованы как `mf_*.h` + `mf_*.cpp` рядом с скетчем, чтобы Arduino IDE автоматически их подхватывал.
- Конфигурация через `mf_config.h` (заменяет Kconfig).

## CI

GitHub Actions собирает скетч через `arduino-cli` (см. [.github/workflows/firmware-build.yml](../.github/workflows/firmware-build.yml)). Кэшируется ESP32 core и библиотеки, артефакт — `*.bin`.

## Что коммитим

- Исходники (`.ino`, `.h`, `.cpp`), README, версии core/libs.
- Не коммитим: `build/`, `*.bin`, временные артефакты IDE.
