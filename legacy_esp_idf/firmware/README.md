# MushFarm firmware

Локальная разработка ведётся в **Arduino IDE** (без `arduino-cli`).

## Что установить

- Arduino IDE 2.x
- ESP32 core (Boards Manager): версия фиксируется в `arduino-version.txt`

## Базовые шаги (Arduino IDE)

1. Открыть скетч `firmware/mushfarm/mushfarm.ino`
2. Выбрать плату: `ESP32S3 Dev Module`
3. Проверить Board Settings (Flash mode/PSRAM/Partition)
4. Нажать **Verify**
5. Для прошивки: **Upload**
6. Для логов: **Serial Monitor**

## Версия сборки

При старте в UART должны выводиться:
- версия прошивки
- короткий git SHA (или `unknown`, если сборка не из git)

## Что коммитим

- исходники,
- конфиги и README,
- не коммитим временные build-артефакты.

## CI

CI может использовать `arduino-cli` или action для автоматической проверки, но это не требуется для локальной разработки в Arduino IDE.
