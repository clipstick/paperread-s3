#include "hal/Gt911Touch.h"
#include "utils/Log.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "hal:touch";

// GT911 register addresses
#define GT911_REG_STATUS   0x814E
#define GT911_REG_POINT1   0x8150
#define GT911_REG_CONFIG   0x8140

// GT911 I2C addresses (device can be at either address)
#define GT911_ADDR1        0x14
#define GT911_ADDR2        0x5D

Gt911Touch::Gt911Touch(i2c_port_t i2c_port, gpio_num_t sda_pin, gpio_num_t scl_pin, gpio_num_t int_pin)
  : m_i2c_port(i2c_port),
    m_sda_pin(sda_pin),
    m_scl_pin(scl_pin),
    m_int_pin(int_pin),
    m_i2c_addr(0),
    m_display_width(960),
    m_display_height(540),
    m_initialized(false)
{
}

Gt911Touch::~Gt911Touch() {
  if (m_initialized && m_i2c_port != I2C_NUM_MAX) {
    i2c_driver_delete(m_i2c_port);
  }
}

bool Gt911Touch::init() {
  if (m_initialized) {
    LOG_W(TAG, "Touch already initialized");
    return true;
  }

  LOG_I(TAG, "Initializing GT911 touch controller");

  // Configure I2C
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = m_sda_pin;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = m_scl_pin;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000;  // 400kHz
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  conf.clk_flags = 0;
#endif

  esp_err_t err = i2c_param_config(m_i2c_port, &conf);
  if (err != ESP_OK) {
    LOG_E(TAG, "I2C param config failed: %d", err);
    return false;
  }

  err = i2c_driver_install(m_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    LOG_E(TAG, "I2C driver install failed: %d", err);
    return false;
  }

  // Detect GT911 at either possible address
  uint8_t test_byte = 0;
  if (read_reg(GT911_ADDR1, GT911_REG_CONFIG, &test_byte, 1) == ESP_OK) {
    m_i2c_addr = GT911_ADDR1;
    LOG_I(TAG, "GT911 detected at address 0x%02X", m_i2c_addr);
  } else if (read_reg(GT911_ADDR2, GT911_REG_CONFIG, &test_byte, 1) == ESP_OK) {
    m_i2c_addr = GT911_ADDR2;
    LOG_I(TAG, "GT911 detected at address 0x%02X", m_i2c_addr);
  } else {
    LOG_E(TAG, "GT911 not found on I2C bus");
    i2c_driver_delete(m_i2c_port);
    return false;
  }

  m_initialized = true;
  LOG_I(TAG, "GT911 touch initialized successfully");
  return true;
}

bool Gt911Touch::is_touched() {
  if (!m_initialized) {
    return false;
  }

  uint8_t status = 0;
  if (read_reg(m_i2c_addr, GT911_REG_STATUS, &status, 1) != ESP_OK) {
    return false;
  }

  // Check if buffer status bit is set and touch points > 0
  return (status & 0x80) && (status & 0x0F);
}

bool Gt911Touch::get_touch_point(uint16_t* x, uint16_t* y) {
  if (!x || !y) {
    return false;
  }

  TouchPoint point;
  if (!get_touch_data(&point)) {
    return false;
  }

  *x = point.x;
  *y = point.y;
  return true;
}

bool Gt911Touch::get_touch_data(TouchPoint* point) {
  if (!m_initialized || !point) {
    return false;
  }

  // Read status register
  uint8_t status = 0;
  if (read_reg(m_i2c_addr, GT911_REG_STATUS, &status, 1) != ESP_OK) {
    return false;
  }

  // Check if data is ready
  if (!(status & 0x80)) {
    return false;
  }

  uint8_t touch_count = status & 0x0F;
  if (touch_count == 0) {
    // Clear status flag
    uint8_t zero = 0;
    write_reg(m_i2c_addr, GT911_REG_STATUS, &zero, 1);
    return false;
  }

  // Read first touch point data (8 bytes total, we need first 4 for X,Y)
  uint8_t data[4] = {0};
  if (read_reg(m_i2c_addr, GT911_REG_POINT1, data, sizeof(data)) != ESP_OK) {
    return false;
  }

  // Parse touch data (little-endian)
  uint16_t raw_x = (uint16_t)(data[1] << 8 | data[0]);
  uint16_t raw_y = (uint16_t)(data[3] << 8 | data[2]);

  // GT911 on Paper S3 reports in 540x960 space
  // Map to display coordinates (accounting for any calibration)
  point->x = raw_x;
  point->y = raw_y;
  point->pressure = 0;  // GT911 doesn't report pressure
  point->id = 0;        // Always return first touch point

  // Clear status flag
  uint8_t zero = 0;
  write_reg(m_i2c_addr, GT911_REG_STATUS, &zero, 1);

  LOG_D(TAG, "Touch detected: (%d, %d)", point->x, point->y);
  return true;
}

uint8_t Gt911Touch::get_touch_count() {
  if (!m_initialized) {
    return 0;
  }

  uint8_t status = 0;
  if (read_reg(m_i2c_addr, GT911_REG_STATUS, &status, 1) != ESP_OK) {
    return 0;
  }

  if (!(status & 0x80)) {
    return 0;
  }

  return status & 0x0F;
}

void Gt911Touch::calibrate(uint16_t display_width, uint16_t display_height) {
  m_display_width = display_width;
  m_display_height = display_height;
  LOG_I(TAG, "Touch calibrated for display: %dx%d", display_width, display_height);
}

void Gt911Touch::set_interrupt_enabled(bool enable) {
  // GT911 interrupt configuration could be implemented here
  // For now, we use polling mode so this is a no-op
  (void)enable;
  LOG_D(TAG, "Touch interrupt %s (not implemented, using polling)", enable ? "enabled" : "disabled");
}

esp_err_t Gt911Touch::write_reg(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  if (!cmd) {
    return ESP_FAIL;
  }

  uint8_t reg_hi = reg >> 8;
  uint8_t reg_lo = reg & 0xFF;

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_hi, true);
  i2c_master_write_byte(cmd, reg_lo, true);
  if (data && len) {
    i2c_master_write(cmd, (uint8_t*)data, len, true);
  }
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(m_i2c_port, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return ret;
}

esp_err_t Gt911Touch::read_reg(uint8_t addr, uint16_t reg, uint8_t* data, size_t len) {
  if (!data || !len) {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  if (!cmd) {
    return ESP_FAIL;
  }

  uint8_t reg_hi = reg >> 8;
  uint8_t reg_lo = reg & 0xFF;

  // Write register address
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_hi, true);
  i2c_master_write_byte(cmd, reg_lo, true);

  // Read data
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(m_i2c_port, cmd, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd);
  return ret;
}
