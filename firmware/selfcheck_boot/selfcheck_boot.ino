#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_MLX90614.h>

// Pin map (ESP32-S3 DevKitC).
static constexpr uint8_t PIN_I2C_SDA = 1;
static constexpr uint8_t PIN_I2C_SCL = 2;
static constexpr uint8_t PIN_WATER   = 47;
static constexpr uint8_t PIN_FAN     = 8;
static constexpr uint8_t PIN_HUM     = 9;
static constexpr uint8_t PIN_LIGHT   = 10;

// PWM settings.
static constexpr uint32_t PWM_FREQ_HZ = 25000;
static constexpr uint8_t PWM_RES_BITS = 10;
static constexpr uint32_t PWM_MAX_DUTY = (1u << PWM_RES_BITS) - 1u;

// Self-check profile (S2.5): brief and safe.
static constexpr float ACT_TEST_PERCENT = 50.0f;
static constexpr uint32_t ACT_TEST_MS = 2000;

SensirionI2CScd4x scd4x;
Adafruit_MLX90614 mlx;

static uint32_t percent_to_duty(float percent) {
    if (percent <= 0.0f) return 0;
    if (percent >= 100.0f) return PWM_MAX_DUTY;
    return (uint32_t)((percent / 100.0f) * (float)PWM_MAX_DUTY + 0.5f);
}

static void set_pwm(uint8_t pin, float percent) {
    ledcWrite(pin, percent_to_duty(percent));
}

static void test_actuator(uint8_t pin, const char* name) {
    Serial.printf("-> %s %.0f%% на %lu ms... [NEEDS_OPERATOR]\n",
                  name, ACT_TEST_PERCENT, (unsigned long)ACT_TEST_MS);
    set_pwm(pin, ACT_TEST_PERCENT);
    delay(ACT_TEST_MS);
    set_pwm(pin, 0.0f);
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("\n=== MushFarm: Auto Self-Check (S2.5) ===");

    uint8_t auto_ok = 0;
    uint8_t auto_fail = 0;
    uint8_t needs_operator = 0;

    // 1) I2C init.
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // 2) SCD41 probe.
    scd4x.begin(Wire);
    uint16_t err = 0;
    char err_msg[256];

    err = scd4x.wakeUp();
    if (err) {
        errorToString(err, err_msg, sizeof(err_msg));
        Serial.printf("[WARN] SCD41 wakeUp: %s\n", err_msg);
    }

    err = scd4x.stopPeriodicMeasurement();
    if (err) {
        errorToString(err, err_msg, sizeof(err_msg));
        Serial.printf("[WARN] SCD41 stopPeriodicMeasurement: %s\n", err_msg);
    }

    err = scd4x.reinit();
    if (err) {
        errorToString(err, err_msg, sizeof(err_msg));
        Serial.printf("[WARN] SCD41 reinit: %s\n", err_msg);
    }

    uint16_t sn0 = 0, sn1 = 0, sn2 = 0;
    err = scd4x.getSerialNumber(sn0, sn1, sn2);
    if (err) {
        errorToString(err, err_msg, sizeof(err_msg));
        Serial.printf("[FAIL][AUTO] SCD41 не найден: %s\n", err_msg);
        auto_fail++;
    } else {
        Serial.printf("[ OK ][AUTO] SCD41 S/N: 0x%04x%04x%04x\n", sn0, sn1, sn2);
        auto_ok++;
    }

    // 3) MLX90614 probe.
    if (!mlx.begin()) {
        Serial.println("[FAIL][AUTO] MLX90614 не найден (проверьте SDA/SCL).");
        auto_fail++;
    } else {
        Serial.println("[ OK ][AUTO] MLX90614 найден.");
        auto_ok++;
    }

    // 4) Water sensor (digital level only; no pass/fail here).
    pinMode(PIN_WATER, INPUT_PULLUP);
    bool water_present = (digitalRead(PIN_WATER) == LOW);
    Serial.printf("[INFO][AUTO] Water level: %s\n", water_present ? "WET (LOW)" : "DRY (HIGH)");
    auto_ok++;

    // 5) PWM init safe-off.
    ledcAttach(PIN_FAN, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_HUM, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_LIGHT, PWM_FREQ_HZ, PWM_RES_BITS);
    set_pwm(PIN_FAN, 0.0f);
    set_pwm(PIN_HUM, 0.0f);
    set_pwm(PIN_LIGHT, 0.0f);

    // 6) Actuator checks are operator-confirmed.
    Serial.println("\n--- Тест актуаторов (NEEDS_OPERATOR) ---");
    test_actuator(PIN_FAN, "Вентилятор");
    needs_operator++;

    if (!water_present) {
        Serial.println("-> Увлажнитель: [SKIP] DRY-RUN protection.");
    } else {
        test_actuator(PIN_HUM, "Увлажнитель");
        needs_operator++;
    }

    test_actuator(PIN_LIGHT, "Свет");
    needs_operator++;

    Serial.println("\n=== Self-check summary ===");
    Serial.printf("AUTO_OK=%u, AUTO_FAIL=%u, NEEDS_OPERATOR=%u\n",
                  auto_ok, auto_fail, needs_operator);
    Serial.println(auto_fail == 0 ? "RESULT: PASS_WITH_OPERATOR_CHECKS"
                                  : "RESULT: FAIL");
}

void loop() {
    // One-shot boot self-check: nothing in loop.
}
