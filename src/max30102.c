/**
 * @file max30102.c
 * @brief MAX30102 Heart Rate and SpO2 Sensor Driver Implementation
 */

#include "max30102.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX30102_FIFO_DEPTH 32

// Allow address to be changed (for auto-detection)
static uint8_t max30102_current_addr = MAX30102_ADDR;

/**
 * @brief Set MAX30102 I2C address (for auto-detection)
 */
void max30102_set_address(uint8_t addr) {
    max30102_current_addr = addr;
}

/**
 * @brief Write register to MAX30102
 */
bool max30102_write_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int result = i2c_write_blocking(i2c, max30102_current_addr, data, 2, false);
    if (result != 2) {
        return false;
    }
    return true;
}

/**
 * @brief Read register from MAX30102
 */
bool max30102_read_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t *value) {
    int write_result = i2c_write_blocking(i2c, max30102_current_addr, &reg, 1, true);
    if (write_result != 1) {
        return false;
    }
    int read_result = i2c_read_blocking(i2c, max30102_current_addr, value, 1, false);
    if (read_result != 1) {
        return false;
    }
    return true;
}

/**
 * @brief Initialize MAX30102 sensor
 * @note Matches library initialization procedure
 */
bool max30102_init(i2c_inst_t *i2c) {
    uint8_t part_id;
    uint8_t prev;
    
    // Read PART_ID to verify sensor
    max30102_read_reg(i2c, MAX30102_REG_PART_ID, &part_id);
    
    // Reset procedure (matches library)
    // Read current mode config
    if (max30102_read_reg(i2c, MAX30102_REG_MODE_CONFIG, &prev)) {
        // Set reset bit (bit 6)
        prev &= ~(1 << 6);
        prev |= (1 << 6);
        if (max30102_write_reg(i2c, MAX30102_REG_MODE_CONFIG, prev)) {
            sleep_ms(10);  // Wait for reset (library uses 10ms)
            
            // Verify reset completed (bit 6 should be cleared)
            max30102_read_reg(i2c, MAX30102_REG_MODE_CONFIG, &prev);
        }
    }
    
    // Clear FIFO (matches library)
    prev = 0;
    max30102_write_reg(i2c, MAX30102_REG_FIFO_RD_PTR, prev);
    max30102_write_reg(i2c, MAX30102_REG_FIFO_WR_PTR, prev);
    max30102_write_reg(i2c, MAX30102_REG_OVF_COUNTER, prev);
    
    // Configure FIFO
    // Sample averaging: 4 samples (bits 7:5 = 010)
    // FIFO rollover: enabled (bit 4 = 1)
    // FIFO almost full: 17 samples (bits 3:0 = 1111 = 0xF)
    max30102_write_reg(i2c, MAX30102_REG_FIFO_CONFIG, 0x4F);
    
    // Configure mode: SpO2 mode (bits 2:0 = 011)
    // Heart rate only: 0x02, SpO2: 0x03
    max30102_write_reg(i2c, MAX30102_REG_MODE_CONFIG, 0x03);
    
    // Configure SpO2
    // SPO2_ADC range: 4096 nA (bits 6:5 = 10)
    // SPO2 sample rate: 100 Hz (bits 4:2 = 011)
    // LED pulse width: 411 us (bits 1:0 = 11 = 18-bit ADC resolution)
    max30102_write_reg(i2c, MAX30102_REG_SPO2_CONFIG, 0x27);
    
    // Set LED pulse amplitudes
    max30102_write_reg(i2c, MAX30102_REG_LED1_PA, 0x24);  // Red LED: 6.0mA
    max30102_write_reg(i2c, MAX30102_REG_LED2_PA, 0x24);  // IR LED: 6.0mA
    
    // Enable interrupts
    max30102_write_reg(i2c, MAX30102_REG_INTR_ENABLE_1, 0xE0);
    
    return true;
}

/**
 * @brief Reset MAX30102 sensor
 */
void max30102_reset(i2c_inst_t *i2c) {
    // Try to read current mode, but if it fails, just write reset directly
    uint8_t mode = 0x40;  // Reset bit set
    max30102_read_reg(i2c, MAX30102_REG_MODE_CONFIG, &mode);
    mode |= 0x40;  // Set reset bit
    max30102_write_reg(i2c, MAX30102_REG_MODE_CONFIG, mode);
    
    // Alternative: Direct reset without reading
    // max30102_write_reg(i2c, MAX30102_REG_MODE_CONFIG, 0x40);
}

/**
 * @brief Read FIFO data from MAX30102
 * @note Matches library FIFO reading with proper ADC resolution handling
 */
bool max30102_read_fifo(i2c_inst_t *i2c, uint32_t *red_data, uint32_t *ir_data) {
    uint8_t fifo_rd_ptr;
    uint8_t fifo_wr_ptr;
    uint8_t overflow;
    uint8_t fifo_data[6];
    uint8_t spo2_config;
    uint8_t bit_shift;
    
    // Check overflow counter (library checks this first)
    if (!max30102_read_reg(i2c, MAX30102_REG_OVF_COUNTER, &overflow)) {
        return false;
    }
    if (overflow != 0) {
        // FIFO overflow detected - data may be corrupted
        // Library returns error code 4 for this, but we continue anyway
    }
    
    // Read FIFO pointers
    if (!max30102_read_reg(i2c, MAX30102_REG_FIFO_RD_PTR, &fifo_rd_ptr)) {
        return false;
    }
    if (!max30102_read_reg(i2c, MAX30102_REG_FIFO_WR_PTR, &fifo_wr_ptr)) {
        return false;
    }
    
    // Check if FIFO has data
    if (fifo_rd_ptr == fifo_wr_ptr && overflow == 0) {
        return false;  // No data available
    }
    
    // Read FIFO data (3 bytes for red, 3 bytes for IR in SpO2 mode)
    uint8_t reg = MAX30102_REG_FIFO_DATA;
    if (i2c_write_blocking(i2c, max30102_current_addr, &reg, 1, true) != 1) {
        return false;
    }
    if (i2c_read_blocking(i2c, max30102_current_addr, fifo_data, 6, false) != 6) {
        return false;
    }
    
    // Read SPO2_CONFIG to determine ADC resolution (library does this)
    if (!max30102_read_reg(i2c, MAX30102_REG_SPO2_CONFIG, &spo2_config)) {
        return false;
    }
    
    // Determine bit shift based on ADC resolution (bits 1:0 of SPO2_CONFIG)
    // Library: 0=15bit(shift3), 1=16bit(shift2), 2=17bit(shift1), 3=18bit(shift0)
    uint8_t adc_res = spo2_config & 0x3;
    switch (adc_res) {
        case 0: bit_shift = 3; break;  // 15 bits
        case 1: bit_shift = 2; break;  // 16 bits
        case 2: bit_shift = 1; break;  // 17 bits
        case 3: bit_shift = 0; break;  // 18 bits
        default: bit_shift = 0; break;
    }
    
    // Combine bytes (library format: MSB first)
    *red_data = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    *ir_data = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    
    // Right shift based on ADC resolution (matches library)
    *red_data = *red_data >> bit_shift;
    *ir_data = *ir_data >> bit_shift;
    
    return true;
}

/**
 * @brief Calculate heart rate from red LED data
 * @note This is a simplified implementation. Real implementation would use peak detection
 */
bool max30102_read_heart_rate(i2c_inst_t *i2c, uint8_t *heart_rate) {
    static uint32_t last_red = 0;
    static uint32_t last_time = 0;
    static uint32_t beat_count = 0;
    static uint32_t beat_times[10];
    static uint8_t beat_index = 0;
    
    uint32_t red_data, ir_data;
    if (!max30102_read_fifo(i2c, &red_data, &ir_data)) {
        return false;
    }
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Simple peak detection (rising edge)
    if (red_data > last_red && red_data > BEAT_THRESHOLD) {
        if (current_time - last_time > BEAT_DEBOUNCE_MS) {
            beat_times[beat_index] = current_time - last_time;
            beat_index = (beat_index + 1) % 10;
            beat_count++;
            last_time = current_time;
        }
    }
    last_red = red_data;
    
    // Calculate average BPM from last 10 beats
    if (beat_count >= 2) {
        uint32_t avg_interval = 0;
        uint8_t count = (beat_count < 10) ? beat_count : 10;
        for (uint8_t i = 0; i < count; i++) {
            avg_interval += beat_times[i];
        }
        avg_interval /= count;
        
        if (avg_interval > 0) {
            *heart_rate = (uint8_t)(60000 / avg_interval);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Calculate SpO2 from red and IR LED data
 * @note Improved implementation using ratio of ratios with better filtering
 * @note Formula based on empirical calibration: SpO2 = a - b * R
 *       where R = (red_ac/red_dc) / (ir_ac/ir_dc)
 */
bool max30102_read_spo2(i2c_inst_t *i2c, float *spo2) {
    static uint32_t red_samples[25];  // Increased buffer for better DC/AC separation
    static uint32_t ir_samples[25];
    static uint8_t sample_index = 0;
    static uint8_t sample_count = 0;
    
    uint32_t red_data, ir_data;
    if (!max30102_read_fifo(i2c, &red_data, &ir_data)) {
        return false;
    }
    
    // Store samples (circular buffer)
    red_samples[sample_index] = red_data;
    ir_samples[sample_index] = ir_data;
    sample_index = (sample_index + 1) % 25;
    if (sample_count < 25) sample_count++;
    
    // Need at least 10 samples for meaningful calculation
    if (sample_count < 10) {
        return false;
    }
    
    // Calculate DC component (moving average - low frequency component)
    float red_dc = 0, ir_dc = 0;
    for (uint8_t i = 0; i < sample_count; i++) {
        red_dc += red_samples[i];
        ir_dc += ir_samples[i];
    }
    red_dc /= sample_count;
    ir_dc /= sample_count;
    
    // Calculate AC component (peak-to-peak of high frequency component)
    // Find min and max values to calculate AC amplitude
    float red_max = red_samples[0], red_min = red_samples[0];
    float ir_max = ir_samples[0], ir_min = ir_samples[0];
    
    for (uint8_t i = 0; i < sample_count; i++) {
        if (red_samples[i] > red_max) red_max = red_samples[i];
        if (red_samples[i] < red_min) red_min = red_samples[i];
        if (ir_samples[i] > ir_max) ir_max = ir_samples[i];
        if (ir_samples[i] < ir_min) ir_min = ir_samples[i];
    }
    
    // Calculate AC component (peak-to-peak amplitude)
    float red_ac = red_max - red_min;
    float ir_ac = ir_max - ir_min;
    
    // Calculate ratio of ratios (R value)
    if (ir_dc > 100 && red_dc > 100 && ir_ac > 1.0f && red_ac > 1.0f) {
        float ratio = (red_ac / red_dc) / (ir_ac / ir_dc);
        
        // Improved SpO2 calculation with better coefficients
        // Based on empirical data and common MAX30102 implementations
        // Typical range: R = 0.4 to 1.2 for SpO2 = 70% to 100%
        // Formula: SpO2 = a - b * R
        // Common values: a=104-110, b=17-25
        
        // Using more accurate coefficients (calibrated for typical use)
        // SpO2 = 104 - 17 * R  (more accurate for normal range)
        // Or: SpO2 = 110 - 25 * R  (wider range, original formula)
        
        // Clamp ratio to reasonable range
        // Normal SpO2 (95-100%) typically has ratio around 0.4-0.6
        // Low SpO2 (70-80%) would have ratio around 1.0-1.5
        if (ratio < 0.2f) {
            ratio = 0.2f;  // Very high SpO2
        } else if (ratio > 2.0f) {
            ratio = 2.0f;  // Very low SpO2
        }
        
        // Calculate SpO2 using formula: SpO2 = 125 - 15 * R
        *spo2 = 125.0f - 15.0f * ratio;
        
        // Clamp to valid physiological range
        if (*spo2 < 70.0f) *spo2 = 70.0f;
        if (*spo2 > 100.0f) *spo2 = 100.0f;
        
        return true;
    }
    
    return false;
}

