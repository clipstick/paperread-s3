#pragma once

#include "hal/IDisplay.h"

#if defined(BOARD_TYPE_PAPER_S3)
  #include <epdiy.h>
  #include <epd_highlevel.h>
#else
  #include <epd_driver.h>
  #include <epd_highlevel.h>
#endif

/**
 * Epdiy Display Implementation
 *
 * Wraps the epdiy library for controlling e-ink displays on ESP32.
 * Supports both ESP32 (legacy API) and ESP32-S3 (new API with board definitions).
 *
 * For M5Stack Paper S3:
 * - Panel: ED047TC2 (960x540, 4.7")
 * - Driver: 8-bit parallel LCD interface
 * - Custom board definition in EpdiyPaperS3Board.c
 * - Framebuffer allocated in PSRAM by epdiy
 */
class EpdiyDisplay : public IDisplay {
public:
  EpdiyDisplay();
  ~EpdiyDisplay() override;

  // IDisplay implementation
  bool init() override;
  void power_on() override;
  void power_off() override;
  uint16_t get_width() const override;
  uint16_t get_height() const override;
  uint8_t* get_framebuffer() override;
  void update(DisplayMode mode = DisplayMode::FULL) override;
  void clear() override;
  void set_rotation(DisplayRotation rotation) override;
  DisplayRotation get_rotation() const override;
  int get_temperature() const override;

private:
  EpdiyHighlevelState m_hl;  ///< Epdiy high-level state
  uint16_t m_width;          ///< Display width (native)
  uint16_t m_height;         ///< Display height (native)
  DisplayRotation m_rotation; ///< Current rotation
  uint8_t* m_framebuffer;    ///< Pointer to framebuffer (owned by epdiy)
  bool m_initialized;        ///< Initialization state
  bool m_powered;            ///< Power state
};
