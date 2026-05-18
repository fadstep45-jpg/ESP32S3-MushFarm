# Документация MushFarm

## Навигация (продукт)

| Папка | Содержимое |
| --- | --- |
| [architecture/](architecture/) | Нормативная архитектура: FSM, fault-model, арбитраж |
| [api/](api/) | HTTP и MQTT контракты |
| [recipes/](recipes/) | Схема рецептов и правила переходов стадий |
| [ops/](ops/) | Эксплуатация: SLO, OTA partition |

## Корень `docs/`

| Файл | Назначение |
| --- | --- |
| [roadmap.md](roadmap.md) | Единый roadmap (актуальный план) |
| [BETA_CHECKLIST.md](BETA_CHECKLIST.md) | Чеклист перед полевым бета-тестом |
| [board-pinout-esp32s3.md](board-pinout-esp32s3.md), [Подключение компонентов.md](Подключение%20компонентов.md) | Железо и монтаж |

## Временное (`dev/` — удалить после выхода в прод)

Вся «разработческая» писанина собрана в [dev/](dev/): приёмка спринтов, статусы, старые заметки. См. [dev/README.md](dev/README.md).

Быстрые ссылки:

- Статус: [dev/status/status-2026-05-16.md](dev/status/status-2026-05-16.md)
- Отчёт S6/S7: [dev/status/status-s6-s7-report.md](dev/status/status-s6-s7-report.md)
- Приёмка S6: [dev/acceptance/s6-acceptance.md](dev/acceptance/s6-acceptance.md)
- Приёмка S7: [dev/acceptance/s7-acceptance.md](dev/acceptance/s7-acceptance.md)
