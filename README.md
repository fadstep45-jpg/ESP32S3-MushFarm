# esp32s3_mushfarm

Проект грибной фермы на ESP32-S3.

Целевая платформа — **ESP32-S3 с разъёмом и модулем камеры** (FFC 24-pin) и слотом microSD. Подробности подключения — в [docs/Подключение компонентов.md](docs/Подключение%20компонентов.md).

- Прошивка (Arduino IDE 2.x): [firmware/](firmware/)
- Спецификации и архитектура: [docs/](docs/)
- Рецепты: [recipes/](recipes/)

## Прошивка

Подробности сборки и flash — в [firmware/README.md](firmware/README.md). Версии core/библиотек закреплены в `firmware/arduino-version.txt` и `firmware/arduino-libs.txt`. CI собирает скетч через `arduino-cli` (см. `.github/workflows/firmware-build.yml`).

## Документация

См. [docs/README.md](docs/README.md). Кратко:

- Архитектура: [docs/architecture/](docs/architecture/)
- Временная документация разработки (удалить в проде): [docs/dev/](docs/dev/)
- API: [docs/api/](docs/api/)
- Roadmap: [docs/roadmap.md](docs/roadmap.md)
