#pragma once

#include "hal/IBattery.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>

/**
 * ADC-Based Battery Monitoring
 *
 * Implements IBattery interface using ESP32 ADC to read battery voltage
 * through a voltage divider circuit.
 *
 * For M5Stack Paper S3:
 * - VBAT connected to GPIO3 via 2:1 voltage divider
 * - GPIO3 = ADC1_CHANNEL_2
 * - Measures LiPo battery voltage (3.0V - 4.2V nominal range)
 * - Uses polynomial curve fit for accurate percentage estimation
 *
 * Limitations:
 * - Cannot detect charging state (needs additional hardware)
 * - No time-to-empty estimation (needs fuel gauge chip)
 * - Voltage-based estimation only (not as accurate as coulomb counting)
 */
class AdcBattery : public IBattery {
public:
  /**
   * Constructor
   * @param adc_channel ADC1 channel connected to battery (e.g., ADC1_CHANNEL_2)
   * @param voltage_divider_ratio Voltage divider ratio (e.g., 2.0 for 2:1 divider)
   */
  AdcBattery(adc1_channel_t adc_channel, float voltage_divider_ratio = 2.0f);
  ~AdcBattery() override;

  // IBattery implementation
  bool init() override;
  float get_voltage() override;
  uint8_t get_charge_percent() override;
  bool is_charging() override;
  ChargeState get_charge_state() override;
  bool is_available() const override;
  uint16_t get_time_remaining_minutes() override;

private:
  adc1_channel_t m_adc_channel;
  float m_voltage_divider_ratio;
  esp_adc_cal_characteristics_t m_adc_chars;
  bool m_initialized;
};
