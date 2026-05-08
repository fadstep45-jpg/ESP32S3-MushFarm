# MushFarm firmware (Arduino IDE)

Прошивка ESP32-S3 для грибной фермы. Локальная разработка ведётся в **Arduino IDE 2.x**, без ESP-IDF и без явных вызовов FreeRTOS API. Прежний ESP-IDF проект сохранён в [legacy_esp_idf/firmware/](../legacy_esp_idf/firmware/) как референс архитектуры (FSM, NVS resume, batch logger, climate loop).

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

## Что выводится при старте

- Версия прошивки `MF_FW_VERSION` (см. `mushfarm/mf_config.h`).
- Короткий git SHA, если CI его подставил, иначе `unknown`.
- Карта пинов и список включённых модулей.

## Архитектура

- **Без FreeRTOS API в нашем коде.** Под капотом ESP32 Arduino core всё равно держит FreeRTOS (это ограничение платформы), но из нашего кода никаких `xTaskCreate`, очередей, семафоров — только `setup()`/`loop()` и кооперативный планировщик на `millis()` (см. `mf_scheduler.h`).
- Все модули организованы как `mf_*.h` + `mf_*.cpp` рядом с скетчем, чтобы Arduino IDE автоматически их подхватывал.
- Конфигурация через `mf_config.h` (заменяет Kconfig).

## CI

GitHub Actions собирает скетч через `arduino-cli` (см. [.github/workflows/firmware-build.yml](../.github/workflows/firmware-build.yml)). Кэшируется ESP32 core и библиотеки, артефакт — `*.bin`.

## Что коммитим

- Исходники (`.ino`, `.h`, `.cpp`), README, версии core/libs.
- Не коммитим: `build/`, `*.bin`, временные артефакты IDE.
