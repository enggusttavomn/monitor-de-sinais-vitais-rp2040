/**
 * @file max30102.h
 * @brief MAX30102 Heart Rate and SpO2 Sensor Driver
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/**
 * @brief Set MAX30102 I2C address (for auto-detection)
 * @param addr I2C address
 */
void max30102_set_address(uint8_t addr);

/**
 * @brief Initialize MAX30102 sensor
 * @param i2c I2C instance
 * @return true if initialization successful, false otherwise
 */
bool max30102_init(i2c_inst_t *i2c);

/**
 * @brief Read heart rate from MAX30102
 * @param i2c I2C instance
 * @param heart_rate Pointer to store heart rate value (BPM)
 * @return true if read successful, false otherwise
 */
bool max30102_read_heart_rate(i2c_inst_t *i2c, uint8_t *heart_rate);

/**
 * @brief Read SpO2 from MAX30102
 * @param i2c I2C instance
 * @param spo2 Pointer to store SpO2 value (%)
 * @return true if read successful, false otherwise
 */
bool max30102_read_spo2(i2c_inst_t *i2c, float *spo2);

/**
 * @brief Read raw data from MAX30102 FIFO
 * @param i2c I2C instance
 * @param red_data Pointer to store red LED data
 * @param ir_data Pointer to store IR LED data
 * @return true if read successful, false otherwise
 */
bool max30102_read_fifo(i2c_inst_t *i2c, uint32_t *red_data, uint32_t *ir_data);

/**
 * @brief Reset MAX30102 sensor
 * @param i2c I2C instance
 */
void max30102_reset(i2c_inst_t *i2c);

/**
 * @brief Write register to MAX30102
 * @param i2c I2C instance
 * @param reg Register address
 * @param value Value to write
 * @return true if write successful, false otherwise
 */
bool max30102_write_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t value);

/**
 * @brief Read register from MAX30102
 * @param i2c I2C instance
 * @param reg Register address
 * @param value Pointer to store read value
 * @return true if read successful, false otherwise
 */
bool max30102_read_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t *value);

