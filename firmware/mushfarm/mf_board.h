#pragma once

#include <stdint.h>

// Pin map for Waveshare ESP32-S3-ETH (ESP32-S3R8 + W5500 + DVP camera).
// Verified against the Waveshare wiki pinout. See docs/esp32s3-eth pins.md.
//
// Hard-wired peripherals on this board (DO NOT reuse):
//   W5500 Ethernet (SPI): MISO 12, MOSI 11, SCLK 13, CS 14, RST 9, INT 10
//   microSD (SPI):        CS 4, MOSI 6, MISO 5, SCK 7
//   Camera DVP:           see MF_CAM_* below
//   WS2812 RGB LED:       GPIO 21 (used for error signalling)
//   Octal PSRAM (R8):     GPIO 33..37 (internal, never use as GPIO)
//   USB CDC:              GPIO 19/20 (programming + logs; UART0 not needed)
//
// With the camera reserved, only 16/17/43/44 remain free, so the climate
// I2C bus is shared with the camera SCCB lines (47/48) — same I2C port,
// distinct device addresses (OV5640 0x3C, SCD41 0x62, MLX90614 0x5A).

// I2C climate sensors (SCD41, MLX90614) — shared with camera SCCB bus.
#define MF_I2C_SDA_PIN       48   // = camera SIOD
#define MF_I2C_SCL_PIN       47   // = camera SIOC

// Water level sensor (XKC-Y25-V) — digital input, INPUT_PULLUP.
// GPIO 44 = U0RXD; UART0 is sacrificed (board is flashed/logged over USB CDC).
#define MF_WATER_LEVEL_PIN   44

// MOSFET PWM actuators (LEDC).
#define MF_PWM_FAN_PIN       16
#define MF_PWM_HUM_PIN       17
// GPIO 43 = U0TXD; UART0 is sacrificed (board is flashed/logged over USB CDC).
#define MF_PWM_LIGHT_PIN     43

// Service button — shares the on-board BOOT button (strapping pin, has pull-up).
#define MF_SERVICE_BUTTON_PIN 0

// On-board WS2812 RGB LED — used for error/status signalling (no display).
#define MF_RGB_LED_PIN       21

// Camera DVP pins (reserved; MF_CAMERA_ENABLE gates the driver). Values from
// the Waveshare ESP32-S3-ETH wiki camera pinout.
#define MF_CAM_VSYNC_PIN     1
#define MF_CAM_HREF_PIN      2
#define MF_CAM_XCLK_PIN      3
#define MF_CAM_PCLK_PIN      39
#define MF_CAM_SIOD_PIN      48   // shared with MF_I2C_SDA_PIN
#define MF_CAM_SIOC_PIN      47   // shared with MF_I2C_SCL_PIN
#define MF_CAM_D7_PIN        18
#define MF_CAM_D6_PIN        15
#define MF_CAM_D5_PIN        38
#define MF_CAM_D4_PIN        40
#define MF_CAM_D3_PIN        42
#define MF_CAM_D2_PIN        46
#define MF_CAM_D1_PIN        45
#define MF_CAM_D0_PIN        41

void mf_board_log_pin_map();
