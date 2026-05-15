# esp32s3_mushfarm

Проект грибной фермы на ESP32-S3.

Целевая платформа — **ESP32-S3 с разъёмом и модулем камеры** (FFC 24-pin) и слотом microSD. Подробности подключения — в [docs/Подключение компонентов.md](docs/Подключение%20компонентов.md).

- Прошивка (Arduino IDE 2.x): [firmware/](firmware/)
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
