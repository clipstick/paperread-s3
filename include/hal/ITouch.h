#pragma once

#include <cstdint>
#include <functional>

/**
 * Touch Interface - Hardware abstraction for capacitive touch screens
 *
 * This interface abstracts the touch controller hardware (GT911, FT6x36, etc.),
 * providing touch coordinates and gesture detection.
 *
 * Usage Example:
 *   ITouch* touch = board.touch;
 *
 *   if (!touch->init()) {
 *     LOG_E("app", "Touch init failed");
 *     return;
 *   }
 *
 *   // Poll for touch events
 *   while (running) {
 *     if (touch->is_touched()) {
 *       uint16_t x, y;
 *       if (touch->get_touch_point(&x, &y)) {
 *         LOG_I("app", "Touch at (%d, %d)", x, y);
 *         handle_touch(x, y);
 *       }
 *     }
 *     vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz polling
 *   }
 */

/**
 * Touch point data
 */
struct TouchPoint {
  uint16_t x;        ///< X coordinate (0 to display width-1)
  uint16_t y;        ///< Y coordinate (0 to display height-1)
  uint8_t pressure;  ///< Pressure (0-255, if supported)
  uint8_t id;        ///< Touch point ID for multi-touch (if supported)
};

/**
 * Touch gesture types
 */
enum class Gesture {
  NONE,
  TAP,
  LONG_PRESS,
  SWIPE_LEFT,
  SWIPE_RIGHT,
  SWIPE_UP,
  SWIPE_DOWN
};

/**
 * Abstract interface for touch controller hardware
 */
class ITouch {
public:
  virtual ~ITouch() = default;

  /**
   * Initialize the touch controller
   * @return true on success, false on failure
   */
  virtual bool init() = 0;

  /**
   * Check if screen is currently being touched
   * @return true if touched, false otherwise
   */
  virtual bool is_touched() = 0;

  /**
   * Get the current touch point coordinates
   * @param x Pointer to store X coordinate
   * @param y Pointer to store Y coordinate
   * @return true if touch data is valid, false otherwise
   */
  virtual bool get_touch_point(uint16_t* x, uint16_t* y) = 0;

  /**
   * Get detailed touch point data
   * @param point Pointer to TouchPoint struct to fill
   * @return true if touch data is valid, false otherwise
   */
  virtual bool get_touch_data(TouchPoint* point) = 0;

  /**
   * Get number of touch points (for multi-touch)
   * @return Number of active touch points
   */
  virtual uint8_t get_touch_count() = 0;

  /**
   * Calibrate touch coordinates to display
   * Call after display rotation changes
   * @param display_width Display width in pixels
   * @param display_height Display height in pixels
   */
  virtual void calibrate(uint16_t display_width, uint16_t display_height) = 0;

  /**
   * Enable or disable touch interrupt
   * Some controllers support interrupt-driven touch detection
   * @param enable true to enable interrupt, false to disable
   */
  virtual void set_interrupt_enabled(bool enable) = 0;
};
