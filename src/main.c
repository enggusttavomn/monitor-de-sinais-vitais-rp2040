/**
 * @file main.c
 * @brief Vital Signs Monitor - Main Program
 * @author Beatriz Moreira Alves Ferreira
 * @author Gustavo De Melo Nascimento
 * 
 * @details
 * This program implements a vital signs monitor using:
 * - MAX30102: Heart rate and SpO2 sensor
 * - MLX90614: Body temperature sensor (non-contact)
 * - OLED Display: Real-time display of measurements
 * - RGB LED: Visual feedback based on vital signs
 * - Buzzer: Audio alerts for abnormal values
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

#include "config.h"
#include "max30102.h"
#include "mlx90614.h"
#include "buzzer.h"
#include "ssd1306_i2c.h"
#include "ssd1306.h"
#include "led_rgb.h"
#include "hardware/gpio.h"

// Macro for counting array elements
#ifndef count_of
#define count_of(a) (sizeof(a) / sizeof(*(a)))
#endif

/**
 * @brief Display buffer for SSD1306 OLED display
 * @details Buffer used to store pixel data before rendering to the display
 */
static uint8_t ssd1306_buffer[ssd1306_buffer_length];

/**
 * @brief Current heart rate measurement in BPM (Beats Per Minute)
 */
static uint8_t heart_rate = 0;

/**
 * @brief Current SpO2 (blood oxygen saturation) measurement in percentage
 */
static float spo2 = 0.0f;

/**
 * @brief Current body temperature measurement in degrees Celsius
 */
static float temperature = 0.0f;

/**
 * @brief Flag indicating if the heart rate measurement is valid
 * @details Set to true when a valid heart rate has been measured
 */
static bool heart_rate_valid = false;

/**
 * @brief Flag indicating if the SpO2 measurement is valid
 * @details Set to true when a valid SpO2 value has been measured
 */
static bool spo2_valid = false;

/**
 * @brief Flag indicating if the temperature measurement is valid
 * @details Set to true when a valid temperature has been measured
 */
static bool temp_valid = false;

/**
 * @brief Initialize I2C0 bus for sensors
 * @details Configures I2C0 hardware for communication with MLX90614
 * (temperature sensor) and MAX30102 (heart rate and SpO2 sensor).
 * Sets up SDA and SCL pins with pull-up resistors.
 */
void init_i2c0(void) {
    i2c_init(I2C0_PORT, I2C0_BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
}

/**
 * @brief Initialize I2C1 bus for display
 * @details Configures I2C1 hardware for communication with the SSD1306
 * OLED display. Sets up SDA and SCL pins with pull-up resistors.
 */
void init_i2c1(void) {
    i2c_init(I2C1_PORT, I2C1_BAUDRATE);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
}

/**
 * @brief Initialize display
 * @details Initializes the SSD1306 OLED display by:
 * 1. Initializing the display I2C bus (I2C1)
 * 2. Clearing the display buffer
 * 3. Calling the SSD1306 initialization routine
 */
void init_display(void) {
    // Initialize I2C1 for display
    i2c_init(I2C1_PORT, I2C1_BAUDRATE);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    sleep_ms(50);
    
    // Initialize display buffer
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    
    // Initialize display
    ssd1306_init();
    sleep_ms(200);  // Give display time to initialize
}

/**
 * @brief Update display with vital signs
 * @details Updates the OLED display with current vital signs measurements:
 * - Heart rate (FC) in BPM
 * - SpO2 in percentage
 * - Temperature (TEMP) in degrees Celsius
 * Shows "---" for invalid measurements. Clears the buffer and renders
 * the updated content to the display.
 */
void update_display(void) {
    char buffer[32];
    
    // Clear display buffer
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    
    // Title
    ssd1306_draw_string(ssd1306_buffer, 0, 0, "SINAIS VITAIS");
    
    // Heart rate
    if (heart_rate_valid) {
        snprintf(buffer, sizeof(buffer), "FC: %3d BPM", heart_rate);
    } else {
        snprintf(buffer, sizeof(buffer), "FC: --- BPM");
    }
    ssd1306_draw_string(ssd1306_buffer, 0, 16, buffer);
    
    // SpO2
    if (spo2_valid) {
        snprintf(buffer, sizeof(buffer), "SPO2: %5.1f%%", spo2);
    } else {
        snprintf(buffer, sizeof(buffer), "SPO2: ---%%");
    }
    ssd1306_draw_string(ssd1306_buffer, 0, 32, buffer);
    
    // Temperature
    if (temp_valid) {
        snprintf(buffer, sizeof(buffer), "TEMP: %5.1f C", temperature);
    } else {
        snprintf(buffer, sizeof(buffer), "TEMP: --- C");
    }
    ssd1306_draw_string(ssd1306_buffer, 0, 48, buffer);
    
    // Update display
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&area);
    
    // Send buffer to display
    render_on_display(ssd1306_buffer, &area);
    
    // Small delay to ensure I2C transmission completes
    sleep_us(100);
}

/**
 * @brief Calculate heart rate from raw red LED data samples
 * @details Uses peak detection algorithm to find heartbeats in the red LED
 * data from the MAX30102 sensor. Calculates the average interval between
 * peaks and converts it to BPM (Beats Per Minute).
 * @param red_samples Array of red LED intensity values from MAX30102
 * @param sample_count Number of samples in the array
 * @param timestamps Array of timestamps corresponding to each sample (in milliseconds)
 * @return Calculated heart rate in BPM (30-200 range), or 0 if calculation failed
 * unrealistic intervals (outside 30-200 BPM range).
 */
static uint8_t calculate_heart_rate_from_samples(uint32_t *red_samples, uint16_t sample_count, uint32_t *timestamps) {
    if (sample_count < 10) {
        return 0;  // Need at least 10 samples
    }
    
    // Find peaks in the red LED data
    uint32_t peaks[100];  // Increased for longer measurements
    uint16_t peak_count = 0;
    uint32_t threshold = 0;
    
    // Calculate threshold (average of max and min)
    uint32_t max_val = red_samples[0];
    uint32_t min_val = red_samples[0];
    for (uint16_t i = 1; i < sample_count; i++) {
        if (red_samples[i] > max_val) max_val = red_samples[i];
        if (red_samples[i] < min_val) min_val = red_samples[i];
    }
    threshold = (max_val + min_val) / 2;
    
    // Find peaks (local maxima above threshold)
    // Use a window to avoid detecting noise as peaks
    for (uint16_t i = 2; i < sample_count - 2; i++) {
        if (red_samples[i] > threshold &&
            red_samples[i] > red_samples[i-1] &&
            red_samples[i] > red_samples[i+1] &&
            red_samples[i] > red_samples[i-2] &&
            red_samples[i] > red_samples[i+2]) {
            if (peak_count < 100) {
                peaks[peak_count] = timestamps[i];
                peak_count++;
            }
        }
    }
    
    // Calculate heart rate from peak intervals
    if (peak_count >= 2) {
        uint32_t total_interval = 0;
        uint16_t interval_count = 0;
        
        for (uint16_t i = 1; i < peak_count; i++) {
            uint32_t interval = peaks[i] - peaks[i-1];
            // Filter out unrealistic intervals (too short or too long)
            if (interval >= 300 && interval <= 2000) {  // 30-200 BPM range
                total_interval += interval;
                interval_count++;
            }
        }
        
        if (interval_count > 0) {
            uint32_t avg_interval = total_interval / interval_count;
            uint8_t bpm = (uint8_t)(60000 / avg_interval);
            // Clamp to reasonable range
            if (bpm < 30) bpm = 30;
            if (bpm > 200) bpm = 200;
            return bpm;
        }
    }
    
    return 0;
}

/**
 * @brief Take multiple measurements and calculate averages
 * @details Collects sensor data for MEASUREMENT_DURATION_MS, calculates
 * heart rate, SpO2, and temperature averages, and updates global variables.
 */
void take_measurements(void) {
    // Calculate maximum number of samples we might collect
    // At 200ms intervals, 20 seconds = 100 samples, plus FIFO reads
    #define MAX_SAMPLES 150
    #define MAX_FIFO_SAMPLES 500  // Enough for 20 seconds of FIFO reads
    
    // Arrays to store samples
    uint32_t red_samples[MAX_FIFO_SAMPLES];
    uint32_t ir_samples[MAX_FIFO_SAMPLES];
    uint32_t sample_timestamps[MAX_FIFO_SAMPLES];
    float spo2_samples[MAX_SAMPLES];
    float temp_samples[MAX_SAMPLES];
    
    uint16_t red_count = 0;
    uint16_t spo2_count = 0;
    uint16_t temp_count = 0;
    
    // Clear previous values
    heart_rate_valid = false;
    spo2_valid = false;
    temp_valid = false;
    
    // Update display to show "Measuring..."qu
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd1306_buffer, 0, 0, "MEDINDO");
    ssd1306_draw_string(ssd1306_buffer, 0, 16, "SINAIS VITAIS...");
    ssd1306_draw_string(ssd1306_buffer, 0, 32, "Aguarde ...");
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&area);
    render_on_display(ssd1306_buffer, &area);
    
    // Start time
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t last_spo2_read = start_time;
    uint32_t last_temp_read = start_time;
    
    // Measure for the specified duration
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed = current_time - start_time;
        
        // Check if we've reached the measurement duration
        if (elapsed >= MEASUREMENT_DURATION_MS) {
            break;
        }
        
        // Continuously collect FIFO readings for heart rate calculation
        if (red_count < MAX_FIFO_SAMPLES) {
            uint32_t red_data, ir_data;
            if (max30102_read_fifo(I2C0_PORT, &red_data, &ir_data)) {
                red_samples[red_count] = red_data;
                ir_samples[red_count] = ir_data;
                sample_timestamps[red_count] = current_time;
                red_count++;
            }
            sleep_ms(50);  // Small delay between FIFO reads
        }
        
        // Read SpO2 periodically (every MEASUREMENT_INTERVAL_MS)
        if (current_time - last_spo2_read >= MEASUREMENT_INTERVAL_MS && spo2_count < MAX_SAMPLES) {
            float spo2_raw = 0.0f;
            if (max30102_read_spo2(I2C0_PORT, &spo2_raw)) {
                spo2_samples[spo2_count++] = spo2_raw;
            }
            last_spo2_read = current_time;
            sleep_ms(10);  // Small delay
        }
        
        // Read temperature periodically (every MEASUREMENT_INTERVAL_MS, but offset from SpO2)
        if (current_time - last_temp_read >= MEASUREMENT_INTERVAL_MS && temp_count < MAX_SAMPLES) {
            sleep_ms(20);  // Delay before reading MLX90614
            float temp_raw = 0.0f;
            if (mlx90614_read_temperature(I2C0_PORT, &temp_raw)) {
                temp_samples[temp_count++] = temp_raw;
            }
            last_temp_read = current_time;
        }
        
        // Small sleep to prevent CPU spinning
        sleep_ms(10);
    }
    
    // Calculate heart rate from collected red LED samples
    if (red_count >= 10) {
        heart_rate = calculate_heart_rate_from_samples(red_samples, red_count, sample_timestamps);
        if (heart_rate > 0) {
            heart_rate_valid = true;
        } else {
            heart_rate_valid = false;
        }
    } else {
        heart_rate_valid = false;
    }
    
    // Calculate SpO2 - try both methods and use the better one
    // Method 1: Average of individual SpO2 readings
    float spo2_from_readings = 0.0f;
    bool spo2_from_readings_valid = false;
    if (spo2_count > 0) {
        float spo2_sum = 0.0f;
        for (uint8_t i = 0; i < spo2_count; i++) {
            spo2_sum += spo2_samples[i];
        }
        spo2_from_readings = spo2_sum / spo2_count;
        spo2_from_readings_valid = true;
    }
    
    // Method 2: Calculate SpO2 directly from collected red/IR samples (more accurate)
    // This uses all the collected data, not just periodic readings
    if (red_count >= 50) {
        // Calculate DC component (average)
        float red_dc = 0, ir_dc = 0;
        for (uint16_t i = 0; i < red_count; i++) {
            red_dc += red_samples[i];
            ir_dc += ir_samples[i];
        }
        red_dc /= red_count;
        ir_dc /= red_count;
        
        // Calculate AC component (peak-to-peak)
        float red_max = red_samples[0], red_min = red_samples[0];
        float ir_max = ir_samples[0], ir_min = ir_samples[0];
        for (uint16_t i = 0; i < red_count; i++) {
            if (red_samples[i] > red_max) red_max = red_samples[i];
            if (red_samples[i] < red_min) red_min = red_samples[i];
            if (ir_samples[i] > ir_max) ir_max = ir_samples[i];
            if (ir_samples[i] < ir_min) ir_min = ir_samples[i];
        }
        float red_ac = red_max - red_min;
        float ir_ac = ir_max - ir_min;
        
        // Calculate ratio of ratios
        if (ir_dc > 100 && red_dc > 100 && ir_ac > 1.0f && red_ac > 1.0f) {
            float ratio = (red_ac / red_dc) / (ir_ac / ir_dc);
            
            // Clamp ratio
            if (ratio < 0.2f) ratio = 0.2f;
            if (ratio > 2.0f) ratio = 2.0f;
            
            // Use improved formula: SpO2 = 125 - 15 * R
            float spo2_from_samples = 125.0f - 15.0f * ratio;
            if (spo2_from_samples < 70.0f) spo2_from_samples = 70.0f;
            if (spo2_from_samples > 100.0f) spo2_from_samples = 100.0f;
            
            // Use the sample-based calculation (more accurate with more data)
            spo2 = spo2_from_samples;
            spo2_valid = true;
        } else {
            // Fall back to readings if sample calculation fails
            if (spo2_from_readings_valid) {
                spo2 = spo2_from_readings;
                spo2_valid = true;
            } else {
                spo2_valid = false;
            }
        }
    } else if (spo2_from_readings_valid) {
        // Use readings if we don't have enough samples
        spo2 = spo2_from_readings;
        spo2_valid = true;
    } else {
        spo2_valid = false;
    }
    
    // Calculate temperature average
    if (temp_count > 0) {
        float temp_sum = 0.0f;
        for (uint8_t i = 0; i < temp_count; i++) {
            temp_sum += temp_samples[i];
        }
        temperature = temp_sum / temp_count;
        temp_valid = true;
    } else {
        temp_valid = false;
    }
}

/**
 * @brief Initialize I2C bus for sensors and detect sensor presence
 * @details Performs I2C bus recovery, initializes I2C0 hardware, scans the bus,
 * and tests for the presence of MAX30102 and MLX90614 sensors. Verifies sensor
 * communication by reading device-specific registers.
 * Tests MAX30102 at address 0x57 and MLX90614 at address 0x5A.
 */
static void init_sensors_i2c(void) {
    // Initialize I2C0 hardware
    init_i2c0();
    
    sleep_ms(200);  // Let I2C bus stabilize
    
    // Test MAX30102 on I2C0
    uint8_t max30102_reg = 0xFF;  // PART_ID register
    uint8_t max30102_data;
    
    if (i2c_write_blocking(I2C0_PORT, MAX30102_ADDR, &max30102_reg, 1, true) == 1) {
        sleep_us(100);
        if (i2c_read_blocking(I2C0_PORT, MAX30102_ADDR, &max30102_data, 1, false) == 1) {
            if (max30102_data == 0x15 || max30102_data == 0x11) {
                // MAX30102 detected
            }
        }
    }
    
    // Test MLX90614 at default address on I2C0
    uint8_t mlx90614_reg = 0x25;  // Config register
    uint8_t mlx90614_response[2];
    
    // Add delay before testing MLX90614 to ensure I2C bus is stable
    sleep_ms(50);
    
    if (i2c_write_blocking(I2C0_PORT, MLX90614_ADDR, &mlx90614_reg, 1, true) == 1) {
        sleep_ms(5);  // MLX90614 needs more time at 100kHz
        if (i2c_read_blocking(I2C0_PORT, MLX90614_ADDR, mlx90614_response, 2, false) == 2) {
            uint16_t mlx_config = (uint16_t)mlx90614_response[0] | ((uint16_t)mlx90614_response[1] << 8);
            if (mlx_config != 0x0000 && mlx_config != 0xFFFF) {
                // Try reading temperature register
                mlx90614_reg = 0x07;  // Object temperature
                if (i2c_write_blocking(I2C0_PORT, MLX90614_ADDR, &mlx90614_reg, 1, true) == 1) {
                    sleep_ms(5);  // MLX90614 needs more time
                    if (i2c_read_blocking(I2C0_PORT, MLX90614_ADDR, mlx90614_response, 2, false) == 2) {
                        // Temperature read successfully
                    }
                }
            }
        }
    }
}

/**
 * @brief Initialize MAX30102 and MLX90614 sensors
 * @details Initializes both sensors on the I2C0 bus:
 * - MAX30102: Heart rate and SpO2 sensor
 * - MLX90614: Non-contact temperature sensor
 * Includes delays between initializations to ensure proper startup sequence.
 * initialization for hardware to stabilize.
 */
static void init_sensors(void) {
    // Initialize sensors
    max30102_init(I2C0_PORT);
    sleep_ms(200);
    
    mlx90614_init(I2C0_PORT);
    sleep_ms(200);
}

/**
 * @brief Initialize RGB LED, buzzer, and button
 * @details Initializes all user interface peripherals:
 * - RGB LED: Sets to blue during initialization
 * - Buzzer: Initializes and plays a short beep on startup
 * - Button: Configures as input with pull-up resistor (pressed = LOW)
 * button state is HIGH when not pressed and LOW when pressed.
 */
static void init_peripherals(void) {
    // Initialize RGB LED (using GPIO like P4)
    led_rgb_init_gpio();
    led_rgb_set_gpio(0, 0, 255);  // Blue during initialization
    sleep_ms(100);
    
    // Initialize buzzer
    buzzer_init();
    buzzer_beep(50);  // Short beep on startup
    sleep_ms(100);
    
    // Initialize button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // Pull-up for button (pressed = LOW)
}

/**
 * @brief Display welcome message on the OLED screen
 * @details Displays a welcome screen with instructions to press the button
 * to start measurements. Shows "MONITOR DE SAUDE VITAL" and "Pressione botao
 * para iniciar" messages.
 */
static void show_welcome_screen(void) {
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd1306_buffer, 0, 0, "MONITOR DE");
    ssd1306_draw_string(ssd1306_buffer, 0, 16, "SAUDE VITAL");
    ssd1306_draw_string(ssd1306_buffer, 0, 32, "Pressione botao");
    ssd1306_draw_string(ssd1306_buffer, 0, 48, "para iniciar");
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&area);
    render_on_display(ssd1306_buffer, &area);
}

/**
 * @brief Main function
 * @details Main program entry point. Initializes all hardware components
 * and enters the main event loop. The program flow:
 * 1. Initialize stdio for debugging
 * 2. Initialize I2C bus and detect sensors
 * 3. Initialize sensors (MAX30102, MLX90614)
 * 4. Initialize display system
 * 5. Initialize peripherals (LED, buzzer, button)
 * 6. Show welcome screen
 * 7. Enter main loop:
 *    - Monitor button for press events
 *    - When button pressed: show countdown, take measurements, update display
 *    - Periodically update display and LED/buzzer based on vital signs
 *    - Handle alerts for abnormal vital signs
 * @return Always returns 0 (program runs indefinitely in main loop)
 */
int main(void) {
    // Initialize stdio
    stdio_init_all();
    
    // Initialize I2C bus and detect sensors
    init_sensors_i2c();
    
    // Initialize sensors
    init_sensors();
    
    // Initialize display system
    init_display();
    
    // Initialize peripherals (LED, buzzer, button)
    init_peripherals();
    
    // Show welcome screen
    show_welcome_screen();
    
    uint32_t last_display_update = 0;
    uint32_t last_led_update = 0;
    bool last_button_state = true;  // Button is HIGH when not pressed (pull-up)
    bool button_pressed = false;
    
    // Main loop
    while (true) {
        // Check button state (with debouncing)
        bool current_button_state = gpio_get(BUTTON_PIN);
        
        // Detect button press (falling edge: HIGH -> LOW)
        if (last_button_state && !current_button_state) {
            // Button was just pressed
            button_pressed = true;
            sleep_ms(50);  // Debounce delay
        }
        last_button_state = current_button_state;
        
        // Take measurements when button is pressed
        if (button_pressed) {
            button_pressed = false;
            
            // Reset buzzer alert state for new measurement
            buzzer_reset_alert_state();
            
            // Beep when button is pressed
            buzzer_beep(50);
            
            // Show preparation message with countdown during 2-second delay
            struct render_area prep_area = {
                .start_column = 0,
                .end_column = ssd1306_width - 1,
                .start_page = 0,
                .end_page = ssd1306_n_pages - 1
            };
            calculate_render_area_buffer_length(&prep_area);
            
            char countdown_msg[16];
            for (int countdown = 2; countdown >= 0; countdown--) {
                memset(ssd1306_buffer, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd1306_buffer, 0, 0, "PREPARE-SE");
                ssd1306_draw_string(ssd1306_buffer, 0, 32, "Iniciando em");
                
                if (countdown > 0) {
                    snprintf(countdown_msg, sizeof(countdown_msg), "%d segundo%s...", countdown, countdown == 1 ? "" : "s");
                } else {
                    snprintf(countdown_msg, sizeof(countdown_msg), "AGORA!");
                }
                ssd1306_draw_string(ssd1306_buffer, 0, 48, countdown_msg);
                
                render_on_display(ssd1306_buffer, &prep_area);
                
                if (countdown > 0) {
                    sleep_ms(1000);  // Wait 1 second between countdown updates
                }
            }
            
            // Take multiple measurements and calculate averages
            take_measurements();
            
            // Beep when measurement is done
            buzzer_beep(50);
            
            // Update display with results
            update_display();
            
            // Update LED and buzzer based on results
            if (heart_rate_valid && spo2_valid && temp_valid) {
                led_rgb_set_status_gpio(heart_rate, spo2, temperature);
                buzzer_update(heart_rate, spo2, temperature);
            }
            
            last_display_update = to_ms_since_boot(get_absolute_time());
            last_led_update = to_ms_since_boot(get_absolute_time());
        }
        
        // Update display periodically (to refresh if needed)
        // Only update if we have valid measurements, otherwise keep welcome message
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_display_update >= DISPLAY_UPDATE_MS) {
            // Only show vital signs if measurements have been taken
            if (heart_rate_valid || spo2_valid || temp_valid) {
                update_display();
            }
            // If no measurements yet, keep the welcome message on screen
            last_display_update = current_time;
        }
        
        // Update LED and buzzer periodically (if we have valid data)
        if (current_time - last_led_update >= SAMPLE_INTERVAL_MS) {
            if (heart_rate_valid && spo2_valid && temp_valid) {
                led_rgb_set_status_gpio(heart_rate, spo2, temperature);
                buzzer_update(heart_rate, spo2, temperature);
            } else {
                // No measurements yet - blue LED
                led_rgb_set_gpio(0, 0, 255);
            }
            last_led_update = current_time;
        }
        
        sleep_ms(10);  // Small delay to prevent CPU spinning
    }
    
    return 0;
}

