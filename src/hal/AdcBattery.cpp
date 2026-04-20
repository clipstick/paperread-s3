#include "hal/AdcBattery.h"
#include "utils/Log.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <cmath>
#include <cstring>

static const char* TAG = "hal:battery";

// LiPo battery voltage curve constants
// Polynomial curve fitting for LiPo discharge (3.5V-4.2V range)
// Source: https://github.com/G6EJD/ESP32-e-Paper-Weather-Display/issues/146
#define LIPO_POLY_A  2836.9625f
#define LIPO_POLY_B  -43987.4889f
#define LIPO_POLY_C  255233.8134f
#define LIPO_POLY_D  -656689.7123f
#define LIPO_POLY_E  632041.7303f

// Voltage thresholds for percentage calculation
#define VBAT_FULL    4.20f  // 100%
#define VBAT_EMPTY   3.50f  // 0%
#define VBAT_LOW     3.60f  // Low battery warning threshold

AdcBattery::AdcBattery(adc1_channel_t adc_channel, float voltage_divider_ratio)
  : m_adc_channel(adc_channel),
    m_voltage_divider_ratio(voltage_divider_ratio),
    m_initialized(false)
{
  memset(&m_adc_chars, 0, sizeof(m_adc_chars));
}

AdcBattery::~AdcBattery() {
  // ADC doesn't need explicit cleanup
}

bool AdcBattery::init() {
  if (m_initialized) {
    LOG_W(TAG, "Battery monitoring already initialized");
    return true;
  }

  LOG_I(TAG, "Initializing ADC battery monitoring on channel %d", m_adc_channel);

  // Configure ADC width (12-bit resolution)
  adc1_config_width(ADC_WIDTH_BIT_12);

  // Configure ADC attenuation (full scale ~2450mV with 12dB attenuation)
  // ADC_ATTEN_DB_12 (was ADC_ATTEN_DB_11) for measuring up to ~2.45V
  adc1_config_channel_atten(m_adc_channel, ADC_ATTEN_DB_12);

  // Characterize ADC for calibration
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                           1100,  // Default Vref (mV)
                           &m_adc_chars);

  m_initialized = true;
  LOG_I(TAG, "ADC battery monitoring initialized (voltage divider: %.1f:1)", m_voltage_divider_ratio);
  return true;
}

float AdcBattery::get_voltage() {
  if (!m_initialized) {
    LOG_E(TAG, "Battery monitoring not initialized");
    return 0.0f;
  }

  // Read raw ADC value
  int adc_raw = adc1_get_raw(m_adc_channel);

  // Convert to calibrated voltage in mV
  uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &m_adc_chars);

  // Account for voltage divider (Paper S3 uses 2:1 divider)
  float battery_voltage = (voltage_mv * m_voltage_divider_ratio) / 1000.0f;

  LOG_D(TAG, "Battery voltage: %.2fV (ADC raw: %d, scaled: %lu mV)",
        battery_voltage, adc_raw, voltage_mv);

  return battery_voltage;
}

uint8_t AdcBattery::get_charge_percent() {
  float voltage = get_voltage();

  if (voltage >= VBAT_FULL) {
    return 100;
  }

  if (voltage <= VBAT_EMPTY) {
    return 0;
  }

  // Use polynomial curve fit for LiPo discharge curve
  // This provides more accurate percentage in the middle range
  float percent = LIPO_POLY_A * std::pow(voltage, 4) +
                  LIPO_POLY_B * std::pow(voltage, 3) +
                  LIPO_POLY_C * std::pow(voltage, 2) +
                  LIPO_POLY_D * voltage +
                  LIPO_POLY_E;

  // Clamp to 0-100 range
  if (percent < 0.0f) {
    percent = 0.0f;
  }
  if (percent > 100.0f) {
    percent = 100.0f;
  }

  uint8_t percent_int = (uint8_t)std::roundf(percent);
  LOG_D(TAG, "Battery charge: %d%% (%.2fV)", percent_int, voltage);

  return percent_int;
}

bool AdcBattery::is_charging() {
  // ADC-based battery monitoring cannot detect charging directly
  // Would need additional GPIO or fuel gauge chip
  // For now, return false
  return false;
}

ChargeState AdcBattery::get_charge_state() {
  // Without charge detection hardware, we can only report discharging
  float voltage = get_voltage();

  if (voltage >= VBAT_FULL) {
    // Could be full, but without charge detection we don't know for sure
    return ChargeState::UNKNOWN;
  }

  // Assume discharging if we're measuring voltage
  return ChargeState::DISCHARGING;
}

bool AdcBattery::is_available() const {
  return m_initialized;
}

uint16_t AdcBattery::get_time_remaining_minutes() {
  // ADC battery monitoring doesn't provide time-to-empty estimation
  // Would need fuel gauge chip with current sensing
  return 0;
}
