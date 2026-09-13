/**
 * @file buzzer.c
 * @brief Buzzer Control Implementation
 */

#include "buzzer.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

static bool buzzer_initialized = false;
static uint8_t beep_count = 0;
static bool was_alerting = false;

/**
 * @brief Initialize buzzer
 * @note Buzzer is connected through a transistor, so we use simple GPIO output
 */
void buzzer_init(void) {
    if (buzzer_initialized) {
        return;
    }
    
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    
    buzzer_initialized = true;
    buzzer_off();
}

/**
 * @brief Turn on buzzer at specified frequency
 * @param frequency Frequency in Hz (for passive buzzer, toggles GPIO at this frequency)
 */
void buzzer_on(uint32_t frequency) {
    if (!buzzer_initialized) {
        buzzer_init();
    }
    
    // For passive buzzer connected through transistor, we toggle GPIO at the desired frequency
    // Calculate period in microseconds: period_us = 1000000 / frequency
    uint32_t period_us = 1000000 / frequency;
    uint32_t half_period_us = period_us / 2;
    
    // Toggle GPIO to generate tone
    // This is a blocking call - for non-blocking, use timer interrupts
    gpio_put(BUZZER_PIN, 1);
    sleep_us(half_period_us);
    gpio_put(BUZZER_PIN, 0);
    sleep_us(half_period_us);
}

/**
 * @brief Turn off buzzer
 */
void buzzer_off(void) {
    if (!buzzer_initialized) {
        return;
    }
    gpio_put(BUZZER_PIN, 0);
}

/**
 * @brief Play alert beep
 * @param duration_ms Duration of beep in milliseconds
 */
void buzzer_beep(uint32_t duration_ms) {
    if (!buzzer_initialized) {
        buzzer_init();
    }
    
    // Calculate number of cycles for the desired duration
    uint32_t period_us = 1000000 / BUZZER_FREQ_HZ;
    uint32_t half_period_us = period_us / 2;
    uint32_t total_us = duration_ms * 1000;
    uint32_t cycles = total_us / period_us;
    
    // Generate tone by toggling GPIO
    for (uint32_t i = 0; i < cycles; i++) {
        gpio_put(BUZZER_PIN, 1);
        sleep_us(half_period_us);
        gpio_put(BUZZER_PIN, 0);
        sleep_us(half_period_us);
    }
    
    // Ensure buzzer is off
    gpio_put(BUZZER_PIN, 0);
}

/**
 * @brief Check if alert should be triggered
 */
bool buzzer_check_alert(uint8_t heart_rate, float spo2, float temperature) {
    // Critical conditions
    if (spo2 < SPO2_WARNING_MIN) {
        return true;
    }
    if (temperature < TEMP_NORMAL_MIN || temperature > TEMP_NORMAL_MAX) {
        return true;
    }
    if (heart_rate < BPM_NORMAL_MIN || heart_rate > BPM_NORMAL_MAX) {
        return true;
    }
    return false;
}

/**
 * @brief Reset buzzer alert state (call when starting new measurement)
 */
void buzzer_reset_alert_state(void) {
    beep_count = 0;
    was_alerting = false;
}

/**
 * @brief Update buzzer based on vital signs
 * @note When alert conditions are met, beeps 3 times quickly, then stops
 */
void buzzer_update(uint8_t heart_rate, float spo2, float temperature) {
    static uint32_t last_alert_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    bool is_alerting = buzzer_check_alert(heart_rate, spo2, temperature);
    
    if (is_alerting) {
        // If we just started alerting, reset counter
        if (!was_alerting) {
            beep_count = 0;
            last_alert_time = current_time;
        }
        
        // Beep 3 times quickly (300ms between beeps)
        if (beep_count < 3) {
            if (current_time - last_alert_time >= 300) {
                buzzer_beep(BUZZER_ALERT_MS);
                beep_count++;
                last_alert_time = current_time;
            }
        } else {
            // Already beeped 3 times, turn off
            buzzer_off();
        }
    } else {
        // No alert conditions, reset state
        beep_count = 0;
        buzzer_off();
    }
    
    was_alerting = is_alerting;
}


