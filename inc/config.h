/**
 * @file config.h
 * @brief Configuration file for Vital Signs Monitor
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 */

#pragma once

#include <stdint.h>

// === I2C Configuration ===
// I2C0: Both sensors (MLX90614 and MAX30102) share the same bus
#define I2C0_PORT i2c0
#define I2C0_SDA_PIN 8      // GP8 - I2C0 SDA for both sensors
#define I2C0_SCL_PIN 9      // GP9 - I2C0 SCL for both sensors
#define I2C0_BAUDRATE 100000  // 100kHz 

// I2C1: SSD1306 OLED Display
#define I2C1_PORT i2c1
#define I2C1_SDA_PIN 2      // GP2 - I2C1 SDA for display
#define I2C1_SCL_PIN 3      // GP3 - I2C1 SCL for display
#define I2C1_BAUDRATE 400000  // 400kHz 

// Legacy definitions for backward compatibility (use I2C0)
#define I2C_PORT i2c0
#define I2C_SDA_PIN I2C0_SDA_PIN
#define I2C_SCL_PIN I2C0_SCL_PIN
#define I2C_BAUDRATE I2C0_BAUDRATE

// === Sensor I2C Addresses ===
#define MAX30102_ADDR 0x57  // Heart rate and SpO2 sensor (default)
#define MLX90614_ADDR 0x5A  // Temperature sensor (default)

// === GPIO Pin Definitions ===
#define BUTTON_PIN   5     // Button for triggering temperature reading
#define BUZZER_PIN   21    // Buzzer pin (connected through transistor)

// === RGB LED Matrix (WS2812) Configuration ===
// BitDogLab has a 5x5 WS2812 LED matrix
// The matrix is connected via a single GPIO pin (typically GPIO 16 or similar)
// For now, we'll use individual RGB control if available, or simulate with GPIO
#define LED_MATRIX_PIN 16  // WS2812 data pin (adjust if needed)
#define LED_MATRIX_SIZE 25  // 5x5 = 25 LEDs

// GPIO 11 = VERDE
// GPIO 12 = AZUL
// GPIO 13 = VERMELHO
#define LED_RED_PIN   13   // Red LED GPIO
#define LED_GREEN_PIN 11   // Green LED GPIO
#define LED_BLUE_PIN  12   // Blue LED GPIO

// === Timing Configuration ===
#define SAMPLE_INTERVAL_MS 200   // <= 500 ms (RNF01 requirement)
#define TIMER_INTERVAL_MS  200   // Timer interrupt period
#define DISPLAY_UPDATE_MS  500   // Display update interval

// === Measurement Configuration ===
#define MEASUREMENT_DURATION_MS 20000  // Total measurement duration (20 seconds)
#define MEASUREMENT_INTERVAL_MS 200    // Time between samples during measurement
#define MEASUREMENT_FIFO_READS_PER_SAMPLE 5  // Number of FIFO reads per sample for HR calculation

// === Signal Processing ===
#define MOVING_AVG_SIZE 10        // Moving average filter size
#define BEAT_THRESHOLD 1000       // Heart beat detection threshold
#define BEAT_DEBOUNCE_MS 300      // Minimum time between beats

// === Vital Signs Thresholds ===
#define SPO2_NORMAL_MIN 95.0f    // Normal SpO2 minimum (%)
#define SPO2_WARNING_MIN 90.0f   // Warning SpO2 minimum (%)
#define TEMP_NORMAL_MIN 35.0f    // Normal temperature minimum (°C)
#define TEMP_NORMAL_MAX 37.5f    // Normal temperature maximum (°C)
#define BPM_NORMAL_MIN 60        // Normal heart rate minimum (BPM)
#define BPM_NORMAL_MAX 130       // Normal heart rate maximum (BPM)

// === Buzzer Configuration ===
#define BUZZER_FREQ_HZ 2700       // Buzzer frequency (2.7kHz)
#define BUZZER_ALERT_MS 100       // Alert duration (ms)

// === MAX30102 Register Addresses ===
#define MAX30102_REG_INTR_STATUS_1    0x00
#define MAX30102_REG_INTR_STATUS_2    0x01
#define MAX30102_REG_INTR_ENABLE_1   0x02
#define MAX30102_REG_INTR_ENABLE_2   0x03
#define MAX30102_REG_FIFO_WR_PTR     0x04
#define MAX30102_REG_OVF_COUNTER     0x05
#define MAX30102_REG_FIFO_RD_PTR     0x06
#define MAX30102_REG_FIFO_DATA       0x07
#define MAX30102_REG_FIFO_CONFIG     0x08
#define MAX30102_REG_MODE_CONFIG     0x09
#define MAX30102_REG_SPO2_CONFIG     0x0A
#define MAX30102_REG_LED1_PA         0x0C  // Red LED pulse amplitude
#define MAX30102_REG_LED2_PA         0x0D  // IR LED pulse amplitude
#define MAX30102_REG_PILOT_PA        0x10
#define MAX30102_REG_MULTI_LED_CTRL1 0x11
#define MAX30102_REG_MULTI_LED_CTRL2 0x12
#define MAX30102_REG_TEMP_INTR       0x1F
#define MAX30102_REG_TEMP_FRAC       0x20
#define MAX30102_REG_TEMP_CONFIG     0x21
#define MAX30102_REG_REV_ID          0xFE
#define MAX30102_REG_PART_ID         0xFF

// === MLX90614 Register Addresses ===
#define MLX90614_RAWIR1      0x04
#define MLX90614_RAWIR2      0x05
#define MLX90614_TA          0x06
#define MLX90614_TOBJ1       0x07
#define MLX90614_TOBJ2       0x08
#define MLX90614_TOMAX       0x20
#define MLX90614_TOMIN       0x21
#define MLX90614_PWMCTRL     0x22
#define MLX90614_TARANGE     0x23
#define MLX90614_EMISSIVITY  0x24
#define MLX90614_CONFIG      0x25
#define MLX90614_SMBUS_ADDR  0x2E
#define MLX90614_ID1         0x3C
#define MLX90614_ID2         0x3D
#define MLX90614_ID3         0x3E
#define MLX90614_ID4         0x3F
