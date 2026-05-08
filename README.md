# esp32s3_mushfarm

Проект грибной фермы на ESP32-S3.

- Прошивка (Arduino IDE 2.x, без явных вызовов FreeRTOS API): [firmware/](firmware/)
- Спецификации и архитектура: [docs/](docs/)
- Рецепты: [recipes/](recipes/)

## Прошивка

Подробности сборки и flash — в [firmware/README.md](firmware/README.md). Версии core/библиотек закреплены в `firmware/arduino-version.txt` и `firmware/arduino-libs.txt`. CI собирает скетч через `arduino-cli` (см. `.github/workflows/firmware-build.yml`).

## Документация

- Архитектура: `docs/architecture/`
- API: `docs/api/`
- Схема рецептов: `docs/recipes/recipe-schema-v1.md`
- Чеклист бета-тестирования: `docs/BETA_CHECKLIST.md`
- Единый roadmap проекта: `docs/roadmap.md`
