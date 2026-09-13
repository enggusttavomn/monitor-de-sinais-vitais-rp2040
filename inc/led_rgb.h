/**
 * @file led_rgb.h
 * @brief RGB LED Control for BitDogLab
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief RGB color structure
 */
typedef struct {
    uint8_t r;  // Red component (0-255)
    uint8_t g;  // Green component (0-255)
    uint8_t b;  // Blue component (0-255)
} rgb_color_t;

/**
 * @brief Initialize RGB LED
 */
void led_rgb_init(void);

/**
 * @brief Set RGB LED color
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 */
void led_rgb_set(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set RGB LED color using color structure
 * @param color RGB color structure
 */
void led_rgb_set_color(rgb_color_t color);

/**
 * @brief Turn off RGB LED
 */
void led_rgb_off(void);

/**
 * @brief Set LED color based on vital signs status
 * @param heart_rate Heart rate value (BPM)
 * @param spo2 SpO2 value (%)
 * @param temperature Temperature value (°C)
 */
void led_rgb_set_status(uint8_t heart_rate, float spo2, float temperature);

/**
 * @brief Initialize RGB LED GPIO pins (simple on/off control)
 * @details Configures the red, green, and blue LED pins as outputs
 * and sets them to off (low) state. Uses GPIO control for simple on/off.
 */
void led_rgb_init_gpio(void);

/**
 * @brief Set RGB LED color using GPIO (simple on/off control)
 * @details Sets the RGB LED to the specified color by controlling
 * individual GPIO pins. This is a simple on/off control, not PWM.
 * @param r Red component (0 = off, >0 = on)
 * @param g Green component (0 = off, >0 = on)
 * @param b Blue component (0 = off, >0 = on)
 */
void led_rgb_set_gpio(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set LED color based on vital signs status (GPIO version)
 * @details Determines the appropriate LED color based on vital signs:
 * - Red: Critical conditions (SpO2 < warning threshold, temperature
 *   or heart rate outside normal range)
 * - Yellow: Warning conditions (SpO2 below normal, temperature or
 *   heart rate slightly outside optimal range)
 * - Green: All vital signs within normal ranges
 * @param heart_rate Heart rate value in BPM (Beats Per Minute)
 * @param spo2 SpO2 value in percentage (0-100%)
 * @param temperature Temperature value in degrees Celsius
 */
void led_rgb_set_status_gpio(uint8_t heart_rate, float spo2, float temperature);

// Predefined colors
#define RGB_RED     {255, 0, 0}
#define RGB_GREEN   {0, 255, 0}
#define RGB_BLUE    {0, 0, 255}
#define RGB_YELLOW  {255, 255, 0}
#define RGB_ORANGE  {255, 165, 0}
#define RGB_WHITE   {255, 255, 255}
#define RGB_OFF     {0, 0, 0}


