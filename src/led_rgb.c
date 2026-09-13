/**
 * @file led_rgb.c
 * @brief RGB LED Control Implementation
 */

#include "led_rgb.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static bool led_initialized = false;

/**
 * @brief Initialize RGB LED
 */
void led_rgb_init(void) {
    if (led_initialized) {
        return;
    }
    
    // Configure GPIO pins as outputs and clear any previous state
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    
    // Clear previous state by setting pins to LOW
    gpio_put(LED_RED_PIN, 0);
    gpio_put(LED_GREEN_PIN, 0);
    gpio_put(LED_BLUE_PIN, 0);
    
    // Initialize PWM for each LED
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);
    
    // Get PWM slices
    uint slice_red = pwm_gpio_to_slice_num(LED_RED_PIN);
    uint slice_green = pwm_gpio_to_slice_num(LED_GREEN_PIN);
    uint slice_blue = pwm_gpio_to_slice_num(LED_BLUE_PIN);
    
    // Clear any previous PWM state
    pwm_set_enabled(slice_red, false);
    pwm_set_enabled(slice_green, false);
    pwm_set_enabled(slice_blue, false);
    
    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);  // Adjust for brightness
    pwm_config_set_wrap(&config, 255);
    
    pwm_init(slice_red, &config, true);
    pwm_init(slice_green, &config, true);
    pwm_init(slice_blue, &config, true);
    
    // Clear PWM levels to ensure LED is off
    pwm_set_gpio_level(LED_RED_PIN, 255);  // Inverted: 255 = off
    pwm_set_gpio_level(LED_GREEN_PIN, 255);
    pwm_set_gpio_level(LED_BLUE_PIN, 255);
    
    led_initialized = true;
    led_rgb_off();
}

/**
 * @brief Set RGB LED color
 */
void led_rgb_set(uint8_t r, uint8_t g, uint8_t b) {
    if (!led_initialized) {
        led_rgb_init();
    }
    
    // Set PWM levels (inverted for common cathode)
    pwm_set_gpio_level(LED_RED_PIN, 255 - r);
    pwm_set_gpio_level(LED_GREEN_PIN, 255 - g);
    pwm_set_gpio_level(LED_BLUE_PIN, 255 - b);
}

/**
 * @brief Set RGB LED color using color structure
 */
void led_rgb_set_color(rgb_color_t color) {
    led_rgb_set(color.r, color.g, color.b);
}

/**
 * @brief Turn off RGB LED
 */
void led_rgb_off(void) {
    led_rgb_set(0, 0, 0);
}

/**
 * @brief Set LED color based on vital signs status
 */
void led_rgb_set_status(uint8_t heart_rate, float spo2, float temperature) {
    // Check for critical conditions (red)
    // Any value outside normal range triggers red
    if (spo2 < SPO2_WARNING_MIN || spo2 > 100.0f ||
        temperature < TEMP_NORMAL_MIN || temperature > TEMP_NORMAL_MAX ||
        heart_rate < BPM_NORMAL_MIN || heart_rate > BPM_NORMAL_MAX) {
        led_rgb_set(255, 0, 0);  // Red
        return;
    }
    
    // Check for warning conditions (orange)
    if (spo2 < SPO2_NORMAL_MIN || 
        (heart_rate >= BPM_NORMAL_MIN && heart_rate <= BPM_NORMAL_MAX && 
         (temperature < TEMP_NORMAL_MIN + 0.5f || temperature > TEMP_NORMAL_MAX - 0.5f))) {
        led_rgb_set(255, 165, 0);  // Orange
        return;
    }
    
    // Normal conditions (green)
    if (spo2 >= SPO2_NORMAL_MIN && 
        temperature >= TEMP_NORMAL_MIN && temperature <= TEMP_NORMAL_MAX &&
        heart_rate >= BPM_NORMAL_MIN && heart_rate <= BPM_NORMAL_MAX) {
        led_rgb_set(0, 255, 0);  // Green
        return;
    }
    
    // Default: blue (measuring/invalid)
    led_rgb_set(255, 0, 0);  // Blue
}

/**
 * @brief Initialize RGB LED GPIO pins (simple on/off control)
 * @details Configures the red, green, and blue LED pins as outputs
 * and sets them to off (low) state. Uses GPIO control for simple on/off.
 */
void led_rgb_init_gpio(void) {
    // Initialize and configure LED pins as outputs
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    
    // Start with all LEDs off
    gpio_put(LED_RED_PIN, 0);
    gpio_put(LED_GREEN_PIN, 0);
    gpio_put(LED_BLUE_PIN, 0);
}

/**
 * @brief Set RGB LED color using GPIO (simple on/off control)
 * @details Sets the RGB LED to the specified color by controlling
 * individual GPIO pins. This is a simple on/off control, not PWM.
 * @param r Red component (0 = off, >0 = on)
 * @param g Green component (0 = off, >0 = on)
 * @param b Blue component (0 = off, >0 = on)
 */
void led_rgb_set_gpio(uint8_t r, uint8_t g, uint8_t b) {
    gpio_put(LED_RED_PIN, (r > 0) ? 1 : 0);
    gpio_put(LED_GREEN_PIN, (g > 0) ? 1 : 0);
    gpio_put(LED_BLUE_PIN, (b > 0) ? 1 : 0);
}

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
void led_rgb_set_status_gpio(uint8_t heart_rate, float spo2, float temperature) {
    // Check for critical conditions (red)
    if (spo2 < SPO2_WARNING_MIN || 
        temperature < TEMP_NORMAL_MIN || temperature > TEMP_NORMAL_MAX ||
        heart_rate < BPM_NORMAL_MIN || heart_rate > BPM_NORMAL_MAX) {
        // Critical: Red LED
        led_rgb_set_gpio(255, 0, 0);
        return;
    }
    
    // Check for warning conditions (yellow - red + green)
    if (spo2 < SPO2_NORMAL_MIN || 
        (temperature >= TEMP_NORMAL_MIN && temperature <= TEMP_NORMAL_MAX && 
         (temperature < 35.0f || temperature > 37.0f)) ||
        (heart_rate >= BPM_NORMAL_MIN && heart_rate <= BPM_NORMAL_MAX && 
         (heart_rate < 70 || heart_rate > 100))) {
        // Warning: Yellow (red + green)
        led_rgb_set_gpio(255, 255, 0);
        return;
    }
    
    // Normal: Green LED
    led_rgb_set_gpio(0, 255, 0);
}


