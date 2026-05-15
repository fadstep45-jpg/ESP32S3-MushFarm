# Отчёт о прогрессе — Mushfarm (май 2026)

## TL;DR

С момента предыдущего среза доведены до code-complete остатки **S2, S3 и весь S4** (Фаза 1–2 roadmap), параллельно подравнен **S1** (boot-метрики ресурсов + OTA-партиция), формально учтён модуль камеры в проекте (BoM + архитектура + firmware-стаб), зафиксирован финальный пин-мап платы и проведена концептуальная корректировка роли камеры (cosmetic, не control-input). Сейчас код опережает наличие железа — ниже описано, что разумно делать дальше **до** прихода компонентов и что **ждёт** стенда.

> Уточнение по терминам: "code-complete" в этом отчёте означает "реализовано и собирается чисто в Arduino IDE". Полная приёмка по DoD спринта требует валидации на собранном стенде — этого этапа мы ещё не проходили ни по S2, ни по S3, ни по S4.

---

## Что сделано

### S1 — алайнмент после первичной сдачи

- Добавлены boot-метрики ресурсов: free heap, min free heap, largest free block, размер flash. Логируются при старте. Файл: `firmware/mushfarm/mf_resources.{h,cpp}`, вызов из `setup()`.
- Зафиксирована OTA-совместимая partition scheme: документ `docs/ops/arduino-partition-ota.md` с пошаговой инструкцией для Arduino IDE.

### S2 — датчики и fault-модель (code-complete)

- **SCD41 (CO₂/RH/T):** retry (`MF_SENSOR_READ_RETRIES`), детекция устаревших данных (`MF_SENSOR_STALE_MS`), NaN/range-валидация, recovery streak (`MF_SENSOR_RECOVERY_SAMPLES`), плейсхолдер I2C-probe для LIVE-режима.
- **MLX90614 (ИК-термометр):** аналогичный набор (retry/stale/NaN/range/recovery).
- **XKC-Y25 (вода):** debounce (`MF_WATER_DEBOUNCE_SAMPLES`) с recovery-логикой по тренду.
- Все параметры вынесены в `mf_config.h`.
- **S2.5 self-check:** в boot-логе печатается явный маркер `sensors=MOCK|LIVE`, чтобы случайно не уехать в полевые тесты на mock-данных.

### S3 — приводы и safety на старте (code-complete)

- `mf_actuators_all_off()` встроен в FSM-action `act_emergency_latch` и в `mf_fsm_emergency_ack()`.
- **`EMERGENCY_STOP`:** все приводы OFF; климат-контур ничего не override-ит (early return по состоянию).
- **`PAUSED_SAFE`:** увлажнитель OFF, вентилятор на профилактический минимум; климат-контур держит эти duties.
- **Защита от "сухого хода":** если `mf_water_present() == false`, увлажнитель форсится в 0 непосредственно в `mf_climate.cpp`.

### S4 — FSM + recipe runtime (code-complete, ключевой блок)

- **FSM рефакторинг:** перешли с ad-hoc `if/else` на табличную реализацию (`k_transitions[]`), строго по матрице из `docs/architecture/state-machine.md`. Каждая транзишн = строка таблицы `from / event / guard / to / action / reason`. Старое публичное API сохранено как тонкие wrapper'ы, все идут через `mf_fsm_dispatch()`.
- **Emergency latch:** реализован per spec; выход только через `mf_fsm_emergency_ack()` при `gEmergencyClearAllowed && gHardLimitsSafe`.
- **Recipe runtime:** многостадийный демо-рецепт (`k_demo_stages[]`), переходы стадий через `mf_recipe_runtime_tick()`, freeze таймера в `PAUSED_SAFE` (`mf_recipe_runtime_set_timer_frozen()`), abort на emergency-stop (`mf_recipe_runtime_abort()`).
- **NVS checkpoint:** запись на каждом stage-transition (`mf_fsm_stage_transition_checkpoint()`), восстановление на boot (`mf_recipe_apply_checkpoint()`). Это и есть тот самый resume-после-power-loss из DoD S4.
- **Scheduler:** добавлен tick `task_recipe` с периодом `MF_TICK_RECIPE_MS`. В `task_trace` к каждой записи добавлен текущий `stage_id`.
- **Service button:** long-press теперь идёт через `mf_fsm_service_button_long_press()` → транзишн в `SETUP_AP` (раньше дёргали emergency-stop напрямую, что нарушало FSM-контракт).

### Камера в проекте (cross-cutting)

Исторически в EBOM и архитектуре камера была неучтена, хотя плата её несёт. Закрыли этот долг:

- **EBOM:** строка "Модуль камеры (TBD)" + явное упоминание FFC 24-pin в строке микроконтроллера (`docs/Компоненты.md`).
- **Wiring:** новая секция "Камера (DVP)" в `docs/Подключение компонентов.md`.
- **Архитектура:** учтена в `docs/architecture/requirements-traceability.md`.
- **Firmware stub:** `firmware/mushfarm/mf_camera.{h,cpp}` с флагом `MF_CAMERA_ENABLE 0` в `mf_config.h`. При выключенном флаге пишет в лог `camera=disabled (stub)`. FSM/control loops не задействованы.
- **READMEs и beta-checklist:** обновлены.

### Концептуальная корректировка: камера = cosmetic feature

По итогам обсуждения зафиксировали, что камера в нашей системе — это **observability/UX** (end-of-cycle timelapse + low-FPS preview в сервисном режиме на ~10 fps), а **не control-input** (нет AI-анализа плесени, нет роста-трекинга, ни одного PID не зависит от кадра). На основании этого:

- Камера удалена из Fault Matrix и Degraded Modes в `fault-model.md`. Создана отдельная секция "Non-control auxiliary features", где явно зафиксировано: эти подсистемы вне fault-матрицы, потому что не могут повлиять на control-плоскость.
- Из `state-machine.md` удалена транзишн `ACTIVE_RUN → DEGRADED_RUN` по сбою камеры. Сбой теперь устанавливает только липкий флаг `WARN_CAMERA_FAIL` и **не вызывает state-переходов** в FSM.
- Спринт **S8.5 Camera** в roadmap переформулирован под cosmetic-постановку, с явным "Out of scope: AI-анализ, детекция плесени, использование как control-input — отдельный проект".
- В `service-mode-contract.md` камера помечена как "Role: observability / UX only".

### Пин-карта платы (финализирована)

- Создан отдельный документ `docs/board-pinout-esp32s3.md`: легенда силкскрина платы (`-` камера, `~` microSD, `*` OPI PSRAM), таблица всех физически выведенных GPIO с привязкой к проекту, секции про PSRAM (33..37 выведены, но занят OPI PSRAM модуля), microSD (слот распаян, firmware не использует).
- Сохранено референс-фото платы: `docs/img/board-pinout-reference.png`.
- Зафиксирован финальный пин-мап проекта в `firmware/mushfarm/mf_board.h` и в `docs/Подключение компонентов.md` (новые секции "Актуаторы (MOSFET-модули)" и "Управление"):

| GPIO | Назначение |
| ---: | --- |
| 1, 2 | I2C SDA, SCL (SCD41 + MLX90614) |
| 3    | MOSFET вентилятора (PWM) — **strapping pin**, требует pull-down на затворе MOSFET-модуля |
| 21   | XKC-Y25 (датчик воды) |
| 42   | MOSFET увлажнителя (PWM) |
| 47   | MOSFET света (PWM) |
| 0    | Сервисная кнопка (совмещена с BOOT-кнопкой) |

---

## Текущий статус по roadmap

| Спринт | Статус | Комментарий |
| --- | --- | --- |
| S1 Платформа / CI | code-complete | ресурсы + OTA-партиция зафиксированы |
| S2 Датчики / fault | code-complete | retry/debounce/NaN отработаны; финальная приёмка ждёт стенда |
| S2.5 Self-check | code-complete | mock/LIVE баннер в boot-логе |
| S3 Приводы / safety | code-complete | safe OFF в boot/emergency, защита от dry-run |
| S3.5 Service console | не начат | базируется на Serial CLI — можно делать без железа |
| S3.7 Power budget + PCB freeze | заблокирован | замеры тока требуют железо |
| S4 FSM + recipe runtime | code-complete | таблично-управляемая FSM по матрице + NVS checkpoint |
| S5 Климат / арбитраж | можно начинать code-side | hard-limits и структура контура — без замеров |
| S6 AP / HTTP | можно начинать code-side | контракт зафиксирован в `docs/api/ap-contract.md` |
| S7 MQTT | можно начинать code-side | контракт зафиксирован в `docs/api/mqtt-contract.md` |
| S8 Логи / SD / время | частично code-side | RAM ring buffer + degrade policy без SD-железа |
| S8.5 Камера (cosmetic) | заблокирован | требует включённую камеру |
| S9 Service hardening, S10 Бета | не начаты | финальная стабилизация после стенда |

---

## Что дальше

### Можно делать без железа (рекомендованный порядок)

1. **S5 climate arbitration (структура контура).** Прописать hard-limits, гистерезис, дед-бенды и арбитраж между fan/hum/light по `docs/architecture/control-arbitration.md`. PID-коэффициенты — заглушки до калибровки; safety-clamping реализуем целиком.
2. **S6 AP/HTTP скелет.** HTTP-сервер в `SETUP_AP` по `docs/api/ap-contract.md`: `/status`, `/sensors`, `/recipe`, `/wifi`. JSON-схемы валидируем оффлайн против контракта. Готовим сценарий first-boot → AP → ввод Wi-Fi → apply → reboot.
3. **S8 RAM-side (ring buffer + degrade policy).** Структура логирования в RAM с rotation, JSON-формат записей, политика degrade при отсутствии SD. SD-flush заглушить mock'ом до железа.
4. **S3.5 service console.** Базовый CLI на Serial с safety-guard, timeout, rollback. Работает без сети.

### Заблокировано до получения железа

1. **Финальная приёмка S2, S3, S4.** Fault-injection (отключение I2C-датчика на ходу, обрыв шланга, кратковременное падение питания) с реальной периферией — это и есть DoD спринтов "by the book".
2. **S3.7 Power budget.** Замеры тока на 3.3 В и 5 В, температуры на MOSFET-модулях. Гейт для финализации PCB-разводки.
3. **S5 PID tuning.** Калибровка коэффициентов на стенде.
4. **S8 SD flush.** Реальный SDMMC через `~`-помеченные пины платы.
5. **S8.5 Камера.** Включение драйвера, timelapse, preview в сервисном режиме.
6. **S9, S10.** Soak / stress / chaos-сценарии — только на собранном стенде.

### Рекомендация

Разумно потратить ожидание компонентов на **S5 + S6 + S8 (RAM-side) + S3.5**. Это не закрывает DoD этих спринтов (потому что DoD требует hardware-валидации), но снимает 60–70% работы каждого. В день получения стенда мы сразу переходим к замерам и интеграционным тестам, а не к написанию кода с нуля. Этот подход согласуется со стресс-тестом плана в `docs/roadmap.md` ("AP/MQTT/NTP/logging — эксплуатационное ядро, а не поздний декор", "Тестирование делается по спринтам, а не только в финале").

---

## Документы / артефакты, на которые стоит сослаться при ревью

- `docs/architecture/state-machine.md` — обновлена таблица переходов, добавлены ветки `BOOT + evFaultNonFatal(SD_*)` / `BOOT + evFaultNonFatal(CAMERA_*)`, добавлена секция "Camera Behavior per State".
- `docs/architecture/fault-model.md` — добавлена секция "Non-control auxiliary features" (камера); `DEG_CAMERA` удалён.
- `docs/architecture/service-mode-contract.md` — обновлена секция "Camera Behavior".
- `docs/architecture/requirements-traceability.md` — добавлена строка камеры (S8.5), привязана к новым секциям FSM/fault-model.
- `docs/roadmap.md` — добавлен субспринт **S8.5 Camera (cosmetic)** в Фазу 3 с подробным DoD и явным Out-of-Scope.
- `docs/board-pinout-esp32s3.md` — финальная пин-карта платы + легенда силкскрина.
- `docs/Подключение компонентов.md` — новые секции "Актуаторы (MOSFET-модули)" и "Управление".
- `docs/Компоненты.md` — обновлённый EBOM.
- `firmware/mushfarm/mf_board.h` — финальный пин-мап в коде (`SDA=1 SCL=2 water=21 fan=3 hum=42 light=47 svc_btn=0`).
- `firmware/mushfarm/mf_camera.{h,cpp}` — стаб камеры (`MF_CAMERA_ENABLE 0`).
