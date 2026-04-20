#pragma once

#include <cstdint>
#include <ctime>

/**
 * RTC Interface - Hardware abstraction for real-time clock
 *
 * This interface abstracts RTC hardware (BM8563, DS3231, PCF8563, etc.)
 * for keeping time and scheduling alarms across power cycles and deep sleep.
 *
 * Usage Example:
 *   IRtc* rtc = board.rtc;
 *
 *   if (!rtc->init()) {
 *     LOG_W("app", "RTC not available");
 *   }
 *
 *   // Set time from NTP
 *   time_t ntp_time = get_ntp_time();
 *   rtc->set_time(ntp_time);
 *
 *   // Read time
 *   time_t now = rtc->get_time();
 *   struct tm* timeinfo = localtime(&now);
 *   LOG_I("app", "Current time: %02d:%02d:%02d",
 *         timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
 *
 *   // Set alarm for 8:00 AM tomorrow
 *   struct tm alarm_time = *timeinfo;
 *   alarm_time.tm_hour = 8;
 *   alarm_time.tm_min = 0;
 *   alarm_time.tm_sec = 0;
 *   alarm_time.tm_mday += 1;
 *   rtc->set_alarm(mktime(&alarm_time), true);
 */

/**
 * RTC alarm configuration
 */
struct RtcAlarm {
  time_t time;         ///< Alarm time (UNIX timestamp)
  bool enabled;        ///< Alarm enabled
  bool repeat_daily;   ///< Repeat every day at same time
  bool repeat_weekly;  ///< Repeat every week on same day/time
};

/**
 * Abstract interface for RTC hardware
 */
class IRtc {
public:
  virtual ~IRtc() = default;

  /**
   * Initialize the RTC
   * @return true on success, false on failure or not available
   */
  virtual bool init() = 0;

  /**
   * Check if RTC is available and keeping time
   * @return true if RTC is available, false otherwise
   */
  virtual bool is_available() const = 0;

  /**
   * Set the RTC time
   * @param time UNIX timestamp (seconds since 1970-01-01 00:00:00 UTC)
   * @return true on success, false on failure
   */
  virtual bool set_time(time_t time) = 0;

  /**
   * Get the current RTC time
   * @return UNIX timestamp, or 0 if RTC not available
   */
  virtual time_t get_time() = 0;

  /**
   * Set an alarm
   * The alarm will trigger an interrupt that can wake from deep sleep
   * @param time UNIX timestamp for alarm
   * @param enabled true to enable alarm, false to disable
   * @return true on success, false on failure
   */
  virtual bool set_alarm(time_t time, bool enabled = true) = 0;

  /**
   * Get current alarm configuration
   * @param alarm Pointer to RtcAlarm struct to fill
   * @return true on success, false on failure
   */
  virtual bool get_alarm(RtcAlarm* alarm) = 0;

  /**
   * Clear/disable the alarm
   * @return true on success, false on failure
   */
  virtual bool clear_alarm() = 0;

  /**
   * Check if alarm has triggered
   * @return true if alarm triggered, false otherwise
   */
  virtual bool is_alarm_triggered() = 0;

  /**
   * Clear alarm triggered flag
   * Call after handling alarm wake event
   */
  virtual void clear_alarm_flag() = 0;

  /**
   * Enable alarm interrupt
   * When enabled, alarm will trigger interrupt (can wake from sleep)
   * @param enable true to enable interrupt, false to disable
   */
  virtual void set_alarm_interrupt_enabled(bool enable) = 0;

  /**
   * Get RTC temperature (if available)
   * Some RTCs have built-in temperature sensors
   * @return Temperature in Celsius, or 0.0 if not supported
   */
  virtual float get_temperature() = 0;
};
