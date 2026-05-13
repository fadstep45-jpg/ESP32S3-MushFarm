# Arduino IDE: OTA-friendly partition scheme (S1)

Roadmap S1 requires an **OTA-compatible partition table** before relying on over-the-air updates.

## Recommended setting (ESP32 Arduino core 3.x)

In **Tools → Partition Scheme**, pick an option that reserves **two OTA application slots** (names vary by core version), for example:

- **Minimal SPIFFS (1.9MB APP with OTA / 190KB SPIFFS)** — typical starting point on 4MB flash boards.
- On **8MB** flash boards, prefer a scheme with **larger app + OTA** if available in the menu.

Avoid **single-app-only** schemes (no `ota_0` / `ota_1` style layout) if you plan to ship OTA.

## Why this is recorded here

- Arduino IDE stores the choice per **board menu**, not in this repository.
- CI (`.github/workflows/firmware-build.yml`) uses `PartitionScheme=default`; align local development with the same scheme when possible, or document intentional differences.

## References

- Espressif Arduino-ESP32 partition documentation:  
  https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/partition_table.html
