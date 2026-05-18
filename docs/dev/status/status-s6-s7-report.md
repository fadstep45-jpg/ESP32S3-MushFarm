# Отчёт по спринтам S6 и S7

Дата: 16.05.2026  
Коммиты на `main`: S6 — `4c48ae9`, S7 — `b8d8c65` (+ доработки в `b8d8c65`)

Чеклисты приёмки: [s6-acceptance.md](../acceptance/s6-acceptance.md), [s7-acceptance.md](../acceptance/s7-acceptance.md)

---

## S6 — локальное управление (Wi‑Fi + веб-API)

### Простая версия

**Зачем это нужно**  
Чтобы настроить ферму с телефона или ноутбука, не подключаясь к облаку: увидеть состояние, датчики, запустить или остановить цикл.

**Что сделали**

1. **Первый запуск — своя Wi‑Fi-сеть для настройки.**  
   Если домашний Wi‑Fi ещё не сохранён, плата поднимает сеть `MushFarm_Setup`. Подключаешься к ней, открываешь в браузере `http://192.168.4.1` — там отвечает «пульт» (API).

2. **Сохранение домашнего Wi‑Fi.**  
   Одной командой через API передаёшь имя сети и пароль → плата перезагружается → подключается к дому. Дальше «пульт» доступен по обычному IP в домашней сети (его видно в статусе).

3. **Что можно делать через API (без красивого сайта — голый JSON):**
   - посмотреть состояние (готов / идёт цикл / авария / настройка);
   - показания датчиков (сейчас часто «учебные» числа, если нет реальных датчиков);
   - выбрать демо-рецепт, старт / пауза / продолжение / аварийный стоп;
   - сброс к заводским настройкам (с подтверждением);
   - короткий тест вентилятора или увлажнителя (с автоматическим откатом через несколько секунд).

4. **Часы по интернету.**  
   После подключения к домашнему Wi‑Fi плата сама спрашивает время у NTP-серверов — это нужно для таймеров стадий и логов.

5. **Безопасность настройки.**  
   Сеть `MushFarm_Setup` включается только в режиме «первичная настройка» или после сброса / долгого нажатия сервисной кнопки. Пока идёт обычный цикл выращивания — лишней «открытой» точки доступа нет.

**Что сознательно не делали в S6**

- Загрузка своих рецептов с телефона, смена стадий вручную через API — отложено.
- Пароль на API и HTTPS — позже (спринт S9).
- Отдельный красивый веб-интерфейс — только API; UI можно сделать отдельно.

**Статус S6**

| | |
|---|---|
| Код в репозитории | Готово |
| Сборка на GitHub (CI) | Должна проходить после push |
| Проверка руками на плате (чеклист M1–M7) | **Ещё не зафиксирована** — нужны ESP32 + телефон/ноут |

---

### Техническая версия

**Цель спринта:** локальное управление по [ap-contract.md](api/ap-contract.md).

**Модули:** `mf_net_config`, `mf_wifi`, `mf_http_api`, `mf_actuator_test`.

**Политика**

- SoftAP `MushFarm_Setup` @ `192.168.4.1` только в FSM `SETUP_AP` (нет creds, factory-reset, long-press → service).
- В штатном режиме: STA, HTTP на `wifi.ip` из `GET /api/v1/status`.
- `MF_HTTP_AUTH_ENABLED=0`, `MF_CLOCK_SNTP_ENABLED=1`, SNTP после STA.

**Реализованные эндпоинты**

| Метод | Путь | Назначение |
|-------|------|------------|
| GET | `/api/v1/status` | state, warn_flags, recipe, stage, wifi, climate trace |
| GET | `/api/v1/sensors/live` | scd41 / mlx90614 / water |
| GET | `/api/v1/recipes` | список (`embedded_demo`) |
| GET | `/api/v1/recipes/embedded_demo` | тело демо-рецепта |
| POST | `/api/v1/recipes/embedded_demo/select` | выбор рецепта |
| POST | `/api/v1/cycle/start\|pause\|resume\|stop-emergency` | управление циклом |
| POST | `/api/v1/config/apply` | Wi‑Fi creds → reboot |
| POST | `/api/v1/config/factory-reset` | `confirm_token: FACTORY_RESET` |
| POST | `/api/v1/service/test-actuator` | тест привода + timeout rollback |

**Deferred:** upload/PATCH/DELETE recipes, next/prev-stage, HTTPS (→ S9).

**Приёмка:** M1–M8 в [s6-acceptance.md](../acceptance/s6-acceptance.md). M8 = CI compile.

---

## S7 — удалённое управление (MQTT)

### Простая версия

**Зачем это нужно**  
Чтобы управлять фермой из дома через интернет или домашний сервер (брокер сообщений), а не только когда стоишь рядом с платой по Wi‑Fi.

**Что сделали**

1. **Общий «мозг» команд для Wi‑Fi API и MQTT.**  
   Старт цикла, пауза, выбор рецепта — одна и та же логика. Не будет расхождения «по Wi‑Fi работает, по MQTT — нет».

2. **Подключение к брокеру MQTT.**  
   Адрес брокера сохраняется вместе с Wi‑Fi (через тот же `config/apply`). После перезагрузки плата сама подключается к брокеру, когда есть домашний Wi‑Fi.

3. **Команды и ответы.**  
   Команды приходят в топики вида `mf/имя-устройства/cmd/...`. На каждую команду плата отвечает в `mf/имя-устройства/ack` — успех или ошибка, плюс текущее состояние.

4. **Защита от двойного нажатия.**  
   У каждой команды есть уникальный номер (`msg_id`). Если сеть продублировала сообщение — вторая копия не запустит цикл второй раз.

5. **Телеметрия раз в ~30 секунд.**  
   В топик `telemetry` уходит снимок: состояние, влажность/CO₂ (или mock), мощность вентилятора и увлажнителя, Wi‑Fi, синхронизация времени.

6. **Оповещения при проблемах.**  
   Если отвалился датчик или закончилась вода (по политике «бак пуст») — сообщение в топик `alert`. Если брокер был недоступен — сообщения копятся в памяти и уходят после восстановления связи.

**Что сознательно не делали в S7**

- Загрузка рецептов, переключение стадий, тест моторов через MQTT — отложено.
- Шифрование (mqtts) и пароли на команды — S9.
- Управление только по MQTT без предварительной настройки Wi‑Fi через S6 — по-прежнему нужен хотя бы один раз `config/apply`.

**Статус S7**

| | |
|---|---|
| Код в репозитории | Готово (`b8d8c65`) |
| Автотесты без сети (dedup, команды) | Suite 5 в `mushfarm_tests` |
| Сборка CI | Должна проходить с `MF_MQTT_ENABLE=1` |
| Проверка с Mosquitto (T1–T7) | **Ещё не зафиксирована** |

---

### Техническая версия

**Цель спринта:** удалённое управление по [mqtt-contract.md](api/mqtt-contract.md) — ack, dedup `msg_id`, offline alert queue.

**Модули**

| Модуль | Роль |
|--------|------|
| `mf_cmd_dispatch` | recipe list/get/select, cycle start/pause/resume/stop_emergency |
| `mf_msg_dedup` | LRU 256, TTL 24h uptime |
| `mf_mqtt_config` | NVS: broker, port, user, pass, device_id |
| `mf_mqtt` | PubSubClient, subscribe `cmd/#`, ack, telemetry 30s, alert queue |
| `mf_api_codes` | общие ACK/ERR с HTTP |

**Интеграция**

- `mf_http_api` рефакторен на `mf_cmd_dispatch`; `config/apply` принимает блок `mqtt`.
- `mf_fault_supervisor` → `mf_mqtt_alert_publish` на warn SCD41/MLX/water.
- `mf_mqtt_poll` в `task_network` после `mf_wifi_poll`.
- Connect guards: STA up, broker configured, FSM ∉ {BOOT, SETUP_AP}.
- Reconnect backoff 30s → 120s; `retained=false` на publish.
- Duplicate `msg_id` → `ACK_NOOP` без повторного dispatch.
- Oversized payload → `ERR_PAYLOAD_TOO_LARGE` ack.
- Heap log после connect: `mf_log_resource_metrics("mqtt")`.

**MQTT commands (Phase A)**

- `recipe/list`, `recipe/get`, `recipe/select`
- `cycle/start`, `pause`, `resume`, `stop_emergency`
- Deferred → `ERR_NOT_IMPLEMENTED`

**Приёмка:** T1–T9 в [s7-acceptance.md](../acceptance/s7-acceptance.md).

**Unit tests:** `test_msg_dedup_*`, `test_cmd_dispatch_*` (suite 5); fault tests собираются с `MF_MQTT_ENABLE=0` в shim.

---

## Сводка: что закрыто, что нет

| Спринт | Phase A (код) | Ручная приёмка на железе |
|--------|---------------|---------------------------|
| **S6** | Да (`4c48ae9`) | Нет (M1–M7 curl) |
| **S7** | Да (`b8d8c65`) | Нет (T1–T7 mosquitto) |

Оба спринта закрыты **по разработке**, не по полевой проверке. Для «спринт сдан» в эксплуатационном смысле нужен один вечер с платой, роутером и (для S7) Mosquitto.

---

## Что делаем дальше

### Простая версия

1. **Пауза в разработке больших фич до приезда платы** — по плану так и задумано.

2. **Когда будет ESP32 на столе (можно без грибной камеры):**
   - пройти чеклист S6: настроить Wi‑Fi с телефона, проверить статус и старт/стоп цикла;
   - поднять Mosquitto на ПК или Raspberry Pi, пройти чеклист S7;
   - для чистых тестов выключить автостарт цикла: `MF_AUTO_DEMO_CYCLE 0` в `mf_config.h`.

3. **Когда приедут датчики и приводы:**
   - прошить `selfcheck_boot` — проверка «железо живое»;
   - в основной прошивке: `MF_SENSORS_MOCK 0`, дописать реальные чтения SCD41/MLX;
   - **S5 Phase B** — настройка PID и прогон 4–8 часов на стенде (главная проверка «ферма держит климат»).

4. **Потом по roadmap (не срочно до стенда):**
   - **S8** — логи на SD-карту, неблокирующая запись;
   - **S8.5** — камера, таймлапс (не влияет на урожай);
   - **S9** — пароли, HTTPS/mqtts;
   - **S10** — полевое бета-тестирование.

### Техническая версия

**Ближайшие действия (порядок)**

| # | Действие | Зависимости |
|---|----------|-------------|
| 1 | Дождаться зелёного CI на `b8d8c65` | — |
| 2 | Bench S6: M1–M7 ([s6-acceptance.md](../acceptance/s6-acceptance.md)) | ESP32-S3, Wi‑Fi AP/STA |
| 3 | Bench S7: T1–T7 ([s7-acceptance.md](../acceptance/s7-acceptance.md)) | + MQTT broker (Mosquitto) |
| 4 | Зафиксировать результаты в `status-2026-05-16.md` или дополнении к этому отчёту | после 2–3 |
| 5 | **Hardware track:** `selfcheck_boot` → `MF_SENSORS_MOCK 0` → I2C drivers in `mf_sensor_*` | плата + сенсоры |
| 6 | **S5 Phase B:** PID tuning + soak 4–8 h | шаг 5 + камера/приводы |
| 7 | **S8** logging/SD (NTP уже есть) | SD card optional |
| 8 | **S7/S9** deferred: recipe upload MQTT, TLS, strict `ts` | после стабильного стенда |

**Не начинаем до стенда (или низкий приоритет)**

- S8.5 camera driver
- S9 security hardening
- YAML recipe parser on device
- Harvest-first humidifier timer fallback при отвале SCD41 (зазор fault-model vs код)

**Рекомендуемые флаги для первого bench**

```c
// mf_config.h
#define MF_AUTO_DEMO_CYCLE 0   // ручной старт цикла через API/MQTT
#define MF_SENSORS_MOCK 1      // пока нет реальных датчиков на шине
```

---

## Ссылки

- Roadmap: [roadmap.md](../../roadmap.md)
- Общий статус: [status-2026-05-16.md](status-2026-05-16.md)
- Прошивка / модули: [firmware/README.md](../../../firmware/README.md)
