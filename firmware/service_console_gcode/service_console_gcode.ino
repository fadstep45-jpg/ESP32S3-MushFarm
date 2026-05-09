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

SensirionI2CScd4x scd4x;
Adafruit_MLX90614 mlx;

static char s_line_buf[96];
static size_t s_line_len = 0;
static bool s_estop_latched = false;

static uint16_t s_co2_ppm = 0;
static float s_scd_temp_c = NAN;
static float s_scd_rh = NAN;
static float s_mlx_obj_c = NAN;
static bool s_scd_ok = false;
static bool s_mlx_ok = false;

static float s_fan_pct = 0.0f;
static float s_hum_pct = 0.0f;
static float s_light_pct = 0.0f;

static uint32_t percent_to_duty(float percent) {
    if (percent <= 0.0f) return 0;
    if (percent >= 100.0f) return PWM_MAX_DUTY;
    return (uint32_t)((percent / 100.0f) * (float)PWM_MAX_DUTY + 0.5f);
}

static float clamp_percent(float p) {
    if (p < 0.0f) return 0.0f;
    if (p > 100.0f) return 100.0f;
    return p;
}

static bool water_present() {
    return digitalRead(PIN_WATER) == LOW;
}

static void set_pwm_percent(uint8_t pin, float percent) {
    ledcWrite(pin, percent_to_duty(clamp_percent(percent)));
}

static void all_off() {
    s_fan_pct = 0.0f;
    s_hum_pct = 0.0f;
    s_light_pct = 0.0f;
    set_pwm_percent(PIN_FAN, 0.0f);
    set_pwm_percent(PIN_HUM, 0.0f);
    set_pwm_percent(PIN_LIGHT, 0.0f);
}

static void telemetry_poll() {
    // SCD41: only read when new data is ready.
    bool ready = false;
    uint16_t err = scd4x.getDataReadyStatus(ready);
    if (err == 0 && ready) {
        uint16_t co2 = 0;
        float t = NAN;
        float rh = NAN;
        err = scd4x.readMeasurement(co2, t, rh);
        if (err == 0 && co2 > 0) {
            s_co2_ppm = co2;
            s_scd_temp_c = t;
            s_scd_rh = rh;
            s_scd_ok = true;
        } else {
            s_scd_ok = false;
        }
    }

    // MLX90614: read each telemetry cycle.
    if (s_mlx_ok) {
        float t_obj = mlx.readObjectTempC();
        if (!isnan(t_obj)) {
            s_mlx_obj_c = t_obj;
        } else {
            s_mlx_ok = false;
        }
    }
}

static bool parse_s_param_percent(const char* cmd, float* out_percent) {
    const char* s = strchr(cmd, 'S');
    if (!s || !out_percent) return false;
    char* endp = nullptr;
    float v = strtof(s + 1, &endp);
    if (endp == s + 1) return false;
    *out_percent = clamp_percent(v);
    return true;
}

static void handle_m114() {
    telemetry_poll();
    Serial.println("ok M114");
    Serial.printf("telemetry: water=%s scd41_ok=%s mlx_ok=%s\n",
                  water_present() ? "WET" : "DRY",
                  s_scd_ok ? "1" : "0",
                  s_mlx_ok ? "1" : "0");
    Serial.printf("co2_ppm=%u scd_temp_c=%.2f scd_rh=%.2f mlx_obj_c=%.2f\n",
                  s_co2_ppm, s_scd_temp_c, s_scd_rh, s_mlx_obj_c);
    Serial.printf("actuators: fan=%.1f hum=%.1f light=%.1f\n",
                  s_fan_pct, s_hum_pct, s_light_pct);
}

static void handle_m106(const char* cmd) {
    if (s_estop_latched) {
        Serial.println("error ESTOP_LATCHED (reboot required)");
        return;
    }
    float p = 0.0f;
    if (!parse_s_param_percent(cmd, &p)) {
        Serial.println("error M106 expects S<0-100>");
        return;
    }
    s_fan_pct = p;
    set_pwm_percent(PIN_FAN, p);
    Serial.printf("ok M106 fan=%.1f\n", s_fan_pct);
}

static void handle_m140(const char* cmd) {
    if (s_estop_latched) {
        Serial.println("error ESTOP_LATCHED (reboot required)");
        return;
    }
    float p = 0.0f;
    if (!parse_s_param_percent(cmd, &p)) {
        Serial.println("error M140 expects S<0-100>");
        return;
    }
    if (p > 0.0f && !water_present()) {
        Serial.println("error DRY_RUN_GUARD");
        return;
    }
    s_hum_pct = p;
    set_pwm_percent(PIN_HUM, p);
    Serial.printf("ok M140 hum=%.1f\n", s_hum_pct);
}

static void handle_m150(const char* cmd) {
    if (s_estop_latched) {
        Serial.println("error ESTOP_LATCHED (reboot required)");
        return;
    }
    float p = 0.0f;
    if (!parse_s_param_percent(cmd, &p)) {
        Serial.println("error M150 expects S<0-100>");
        return;
    }
    s_light_pct = p;
    set_pwm_percent(PIN_LIGHT, p);
    Serial.printf("ok M150 light=%.1f\n", s_light_pct);
}

static void handle_m112() {
    all_off();
    s_estop_latched = true;
    Serial.println("ok M112 ESTOP engaged; outputs OFF");
}

static void handle_m999(const char* cmd) {
    if (!s_estop_latched) {
        Serial.println("error ESTOP is not active");
        return;
    }
    if (strcmp(cmd, "M999") == 0) {
        s_estop_latched = false;
        Serial.println("ok M999 ESTOP released");
        return;
    }
    Serial.println("error Usage: M999");
}

static void handle_command(char* line) {
    // Trim leading spaces.
    while (*line == ' ' || *line == '\t') {
        ++line;
    }
    if (*line == '\0') return;

    if (strcmp(line, "M114") == 0) {
        handle_m114();
    } else if (strncmp(line, "M106", 4) == 0) {
        handle_m106(line);
    } else if (strncmp(line, "M140", 4) == 0) {
        handle_m140(line);
    } else if (strncmp(line, "M150", 4) == 0) {
        handle_m150(line);
    } else if (strcmp(line, "M112") == 0) {
        handle_m112();
    } else if (strncmp(line, "M999", 4) == 0) {
        handle_m999(line);
    } else {
        Serial.println("error Unknown command");
    }
}

static void serial_poll() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            s_line_buf[s_line_len] = '\0';
            handle_command(s_line_buf);
            s_line_len = 0;
            continue;
        }
        if (s_line_len < sizeof(s_line_buf) - 1) {
            s_line_buf[s_line_len++] = c;
        } else {
            // Overflow protection: reset parser state.
            s_line_len = 0;
            Serial.println("error Line too long");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println("\n=== MushFarm Service Console (S3.5) ===");

    pinMode(PIN_WATER, INPUT_PULLUP);
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    scd4x.begin(Wire);
    (void)scd4x.wakeUp();
    (void)scd4x.stopPeriodicMeasurement();
    (void)scd4x.startPeriodicMeasurement();
    s_scd_ok = true;

    s_mlx_ok = mlx.begin();

    ledcAttach(PIN_FAN, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_HUM, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_LIGHT, PWM_FREQ_HZ, PWM_RES_BITS);
    all_off();

    Serial.println("ready");
    Serial.println("commands: M114 | M106 S<0-100> | M140 S<0-100> | M150 S<0-100> | M112 | M999");
}

void loop() {
    serial_poll();
    delay(2);
}
