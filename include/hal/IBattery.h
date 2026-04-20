#pragma once

#include <cstdint>

/**
 * Battery Interface - Hardware abstraction for battery monitoring
 *
 * This interface abstracts battery voltage and charge state reading,
 * supporting various methods:
 * - ADC-based voltage divider (M5Stack Paper S3)
 * - I2C fuel gauge chips (BQ27441, MAX17048, etc.)
 * - Platform-specific battery APIs
 *
 * Usage Example:
 *   IBattery* battery = board.battery;
 *
 *   if (!battery->init()) {
 *     LOG_W("app", "Battery monitoring not available");
 *   }
 *
 *   // Read voltage
 *   float voltage = battery->get_voltage();
 *   LOG_I("app", "Battery: %.2fV", voltage);
 *
 *   // Get charge percentage
 *   uint8_t percent = battery->get_charge_percent();
 *   LOG_I("app", "Charge: %d%%", percent);
 *
 *   // Check charging status
 *   if (battery->is_charging()) {
 *     LOG_I("app", "Battery is charging");
 *   }
 */

/**
 * Battery charge state
 */
enum class ChargeState {
  UNKNOWN,      ///< Charge state unknown
  DISCHARGING,  ///< Running on battery
  CHARGING,     ///< Connected to power, charging
  FULL          ///< Fully charged
};

/**
 * Abstract interface for battery monitoring hardware
 */
class IBattery {
public:
  virtual ~IBattery() = default;

  /**
   * Initialize battery monitoring
   * @return true on success, false on failure or not available
   */
  virtual bool init() = 0;

  /**
   * Read battery voltage in volts
   * @return Battery voltage (e.g., 3.7 for a nominal LiPo)
   *         Returns 0.0 if reading failed
   */
  virtual float get_voltage() = 0;

  /**
   * Get battery charge percentage
   * @return Charge level (0-100%)
   *         Returns 0 if reading failed
   */
  virtual uint8_t get_charge_percent() = 0;

  /**
   * Check if battery is currently charging
   * @return true if charging, false otherwise
   */
  virtual bool is_charging() = 0;

  /**
   * Get detailed charge state
   * @return Current charge state
   */
  virtual ChargeState get_charge_state() = 0;

  /**
   * Check if battery monitoring is available
   * Some boards may not have battery monitoring hardware
   * @return true if battery can be monitored, false otherwise
   */
  virtual bool is_available() const = 0;

  /**
   * Get estimated time remaining (if supported by fuel gauge)
   * @return Time in minutes, or 0 if not supported/unknown
   */
  virtual uint16_t get_time_remaining_minutes() = 0;
};
