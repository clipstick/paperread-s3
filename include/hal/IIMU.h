#pragma once

#include <cstdint>

/**
 * IMU Interface - Hardware abstraction for inertial measurement units
 *
 * This interface abstracts IMU hardware (BMI270, MPU6050, etc.) for
 * detecting device orientation, motion, and gestures like lift-to-wake.
 *
 * Usage Example:
 *   IIMU* imu = board.imu;
 *
 *   if (!imu->init()) {
 *     LOG_W("app", "IMU not available");
 *   }
 *
 *   // Read orientation
 *   Orientation orient = imu->get_orientation();
 *   if (orient == Orientation::LANDSCAPE_LEFT) {
 *     display->set_rotation(DisplayRotation::ROTATE_90);
 *   }
 *
 *   // Detect lift gesture
 *   if (imu->is_lifted()) {
 *     wake_from_sleep();
 *   }
 */

/**
 * Device orientation based on accelerometer
 */
enum class Orientation {
  UNKNOWN,
  PORTRAIT,           ///< Upright portrait
  PORTRAIT_INVERTED,  ///< Upside-down portrait
  LANDSCAPE_LEFT,     ///< Rotated left (home button right)
  LANDSCAPE_RIGHT,    ///< Rotated right (home button left)
  FACE_UP,            ///< Lying flat, screen up
  FACE_DOWN           ///< Lying flat, screen down
};

/**
 * 3D acceleration vector
 */
struct Acceleration {
  float x;  ///< X-axis acceleration (m/s²)
  float y;  ///< Y-axis acceleration (m/s²)
  float z;  ///< Z-axis acceleration (m/s²)
};

/**
 * 3D gyroscope data
 */
struct Gyroscope {
  float x;  ///< X-axis rotation rate (deg/s)
  float y;  ///< Y-axis rotation rate (deg/s)
  float z;  ///< Z-axis rotation rate (deg/s)
};

/**
 * Abstract interface for IMU hardware
 */
class IIMU {
public:
  virtual ~IIMU() = default;

  /**
   * Initialize the IMU
   * @return true on success, false on failure or not available
   */
  virtual bool init() = 0;

  /**
   * Check if IMU is available and working
   * @return true if IMU is available, false otherwise
   */
  virtual bool is_available() const = 0;

  /**
   * Read acceleration data
   * @param accel Pointer to Acceleration struct to fill
   * @return true if read succeeded, false otherwise
   */
  virtual bool get_acceleration(Acceleration* accel) = 0;

  /**
   * Read gyroscope data
   * @param gyro Pointer to Gyroscope struct to fill
   * @return true if read succeeded, false otherwise
   */
  virtual bool get_gyroscope(Gyroscope* gyro) = 0;

  /**
   * Get device orientation based on accelerometer
   * @return Current orientation
   */
  virtual Orientation get_orientation() = 0;

  /**
   * Detect if device has been lifted (for wake-on-lift feature)
   * @return true if lift gesture detected, false otherwise
   */
  virtual bool is_lifted() = 0;

  /**
   * Detect if device has been tapped
   * @return true if tap gesture detected, false otherwise
   */
  virtual bool is_tapped() = 0;

  /**
   * Configure wake-on-motion interrupt
   * The IMU can trigger an interrupt when motion is detected,
   * useful for waking from deep sleep.
   * @param threshold Motion threshold (higher = less sensitive)
   * @param enable true to enable interrupt, false to disable
   */
  virtual void set_motion_interrupt(uint8_t threshold, bool enable) = 0;

  /**
   * Read IMU temperature (if available)
   * @return Temperature in Celsius, or 0.0 if not supported
   */
  virtual float get_temperature() = 0;
};
