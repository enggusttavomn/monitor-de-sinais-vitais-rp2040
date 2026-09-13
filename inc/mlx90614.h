/**
 * @file mlx90614.h
 * @brief MLX90614 Infrared Temperature Sensor Driver
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/**
 * @brief Set MLX90614 I2C address (for auto-detection)
 * @param addr I2C address
 */
void mlx90614_set_address(uint8_t addr);

/**
 * @brief Initialize MLX90614 sensor
 * @param i2c I2C instance
 * @return true if initialization successful, false otherwise
 */
bool mlx90614_init(i2c_inst_t *i2c);

/**
 * @brief Read object temperature from MLX90614
 * @param i2c I2C instance
 * @param temperature Pointer to store temperature value (°C)
 * @return true if read successful, false otherwise
 */
bool mlx90614_read_temperature(i2c_inst_t *i2c, float *temperature);

/**
 * @brief Read ambient temperature from MLX90614
 * @param i2c I2C instance
 * @param temperature Pointer to store ambient temperature value (°C)
 * @return true if read successful, false otherwise
 */
bool mlx90614_read_ambient(i2c_inst_t *i2c, float *temperature);

/**
 * @brief Read 16-bit data from MLX90614
 * @param i2c I2C instance
 * @param reg Register address
 * @param data Pointer to store read data
 * @return true if read successful, false otherwise
 */
bool mlx90614_read_16bit(i2c_inst_t *i2c, uint8_t reg, uint16_t *data);

/**
 * @brief Get emissivity from MLX90614 EEPROM
 * @param i2c I2C instance
 * @return Emissivity value (0.1 to 1.0), or 0.0 on error
 */
float mlx90614_get_emissivity(i2c_inst_t *i2c);

/**
 * @brief Set emissivity in MLX90614 EEPROM
 * @param i2c I2C instance
 * @param emissivity Physical emissivity value (0.1 to 1.0)
 * @return true if successful, false otherwise
 * @note EEPROM write requires erase/write cycle (~10ms total)
 */
bool mlx90614_set_emissivity(i2c_inst_t *i2c, float emissivity);
