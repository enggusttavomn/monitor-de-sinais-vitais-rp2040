/**
 * @file buzzer.h
 * @brief Buzzer Control for Audio Alerts
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize buzzer
 */
void buzzer_init(void);

/**
 * @brief Turn on buzzer at specified frequency
 * @param frequency Frequency in Hz
 */
void buzzer_on(uint32_t frequency);

/**
 * @brief Turn off buzzer
 */
void buzzer_off(void);

/**
 * @brief Play alert beep
 * @param duration_ms Duration in milliseconds
 */
void buzzer_beep(uint32_t duration_ms);

/**
 * @brief Check if alert should be triggered based on vital signs
 * @param heart_rate Heart rate value (BPM)
 * @param spo2 SpO2 value (%)
 * @param temperature Temperature value (°C)
 * @return true if alert should be triggered, false otherwise
 */
bool buzzer_check_alert(uint8_t heart_rate, float spo2, float temperature);

/**
 * @brief Reset buzzer alert state (call when starting new measurement)
 */
void buzzer_reset_alert_state(void);

/**
 * @brief Update buzzer based on vital signs
 * @param heart_rate Heart rate value (BPM)
 * @param spo2 SpO2 value (%)
 * @param temperature Temperature value (°C)
 */
void buzzer_update(uint8_t heart_rate, float spo2, float temperature);
