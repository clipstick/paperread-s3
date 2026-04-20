#pragma once

#include "hal/ITouch.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_err.h>

/**
 * GT911 Capacitive Touch Controller Driver
 *
 * Implements ITouch interface for the Goodix GT911 touch controller
 * used on M5Stack Paper S3 and other e-ink displays.
 *
 * Features:
 * - I2C communication (400kHz)
 * - Multi-touch support (up to 5 points)
 * - Automatic I2C address detection (0x14 or 0x5D)
 * - Coordinate calibration for display mapping
 *
 * Paper S3 Configuration:
 * - I2C Port: I2C_NUM_0
 * - SDA: GPIO41
 * - SCL: GPIO42
 * - INT: GPIO48 (optional, not used in polling mode)
 * - Touch space: 540x960 (maps to display coordinate system)
 */
class Gt911Touch : public ITouch {
public:
  /**
   * Constructor
   * @param i2c_port I2C port number (e.g., I2C_NUM_0)
   * @param sda_pin SDA GPIO pin
   * @param scl_pin SCL GPIO pin
   * @param int_pin INT GPIO pin (optional, -1 if not used)
   */
  Gt911Touch(i2c_port_t i2c_port, gpio_num_t sda_pin, gpio_num_t scl_pin, gpio_num_t int_pin = GPIO_NUM_NC);
  ~Gt911Touch() override;

  // ITouch implementation
  bool init() override;
  bool is_touched() override;
  bool get_touch_point(uint16_t* x, uint16_t* y) override;
  bool get_touch_data(TouchPoint* point) override;
  uint8_t get_touch_count() override;
  void calibrate(uint16_t display_width, uint16_t display_height) override;
  void set_interrupt_enabled(bool enable) override;

private:
  // I2C communication
  esp_err_t write_reg(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len);
  esp_err_t read_reg(uint8_t addr, uint16_t reg, uint8_t* data, size_t len);

  // Configuration
  i2c_port_t m_i2c_port;
  gpio_num_t m_sda_pin;
  gpio_num_t m_scl_pin;
  gpio_num_t m_int_pin;
  uint8_t m_i2c_addr;  // Detected I2C address (0x14 or 0x5D)

  // Calibration
  uint16_t m_display_width;
  uint16_t m_display_height;

  // State
  bool m_initialized;
};
