#pragma once

#include <cstdint>
#include <cstddef>

/**
 * Display Interface - Hardware abstraction for e-ink displays
 *
 * This interface abstracts the e-ink display hardware, allowing the
 * application to be independent of the specific display driver used
 * (epdiy, generic EPD, etc.).
 *
 * Usage Example:
 *   IDisplay* display = board.display;
 *
 *   // Initialize display
 *   if (!display->init()) {
 *     LOG_E("app", "Display init failed");
 *     return;
 *   }
 *
 *   // Get framebuffer and draw
 *   uint8_t* fb = display->get_framebuffer();
 *   memset(fb, 0xFF, display->get_width() * display->get_height());
 *
 *   // Update display
 *   display->update(DisplayMode::FULL);
 *
 *   // Power down when done
 *   display->power_off();
 */

/**
 * Display update modes
 * Different modes trade off quality vs speed
 */
enum class DisplayMode {
  /** Full refresh with high quality (GC16) - slowest, best quality */
  FULL,

  /** Partial refresh with good quality (GL16) - medium speed */
  PARTIAL,

  /** Fast refresh for text (A2 mode) - fastest, some ghosting */
  FAST,

  /** Direct update without waveform (for animations) */
  DIRECT
};

/**
 * Display rotation
 */
enum class DisplayRotation {
  ROTATE_0,     // Portrait
  ROTATE_90,    // Landscape (rotated right)
  ROTATE_180,   // Portrait upside-down
  ROTATE_270    // Landscape (rotated left)
};

/**
 * Abstract interface for e-ink display hardware
 */
class IDisplay {
public:
  virtual ~IDisplay() = default;

  /**
   * Initialize the display hardware
   * @return true on success, false on failure
   */
  virtual bool init() = 0;

  /**
   * Power on the display
   * Call before drawing operations
   */
  virtual void power_on() = 0;

  /**
   * Power off the display to save power
   * Call when done with drawing
   */
  virtual void power_off() = 0;

  /**
   * Get display width in pixels
   * @return Width (accounting for current rotation)
   */
  virtual uint16_t get_width() const = 0;

  /**
   * Get display height in pixels
   * @return Height (accounting for current rotation)
   */
  virtual uint16_t get_height() const = 0;

  /**
   * Get pointer to framebuffer
   * The framebuffer is a grayscale buffer (1 byte per pixel, 0-255)
   * where 0 = black, 255 = white
   *
   * @return Pointer to framebuffer (size = width * height bytes)
   *         Allocated in PSRAM on ESP32-S3
   */
  virtual uint8_t* get_framebuffer() = 0;

  /**
   * Update the display from the framebuffer
   * @param mode Update mode (full, partial, fast)
   */
  virtual void update(DisplayMode mode = DisplayMode::FULL) = 0;

  /**
   * Clear the display to white
   * Writes 0xFF to entire framebuffer and updates display
   */
  virtual void clear() = 0;

  /**
   * Set display rotation
   * @param rotation Desired rotation
   */
  virtual void set_rotation(DisplayRotation rotation) = 0;

  /**
   * Get current rotation
   * @return Current rotation
   */
  virtual DisplayRotation get_rotation() const = 0;

  /**
   * Get display temperature (affects e-ink waveforms)
   * @return Temperature in Celsius
   */
  virtual int get_temperature() const = 0;
};
