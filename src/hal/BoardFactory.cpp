#include "hal/BoardFactory.h"
#include "utils/Log.h"

// Include HAL implementations
#include "hal/EpdiyDisplay.h"
#include "hal/Gt911Touch.h"
#include "hal/AdcBattery.h"
#include "hal/SdStorage.h"

// TODO: Add when implemented
// #include "hal/Bmi270Imu.h"
// #include "hal/Bm8563Rtc.h"

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/spi_common.h>

static const char* TAG = "hal:factory";

Board* BoardFactory::create() {
  LOG_I(TAG, "Creating board instance");

#if defined(BOARD_TYPE_PAPER_S3)
  return create_paper_s3();
#elif defined(BOARD_TYPE_LILYGO_T5_47)
  return create_lilygo_t5_47();
#else
  LOG_E(TAG, "Unknown board type");
  return nullptr;
#endif
}

const char* BoardFactory::get_board_name(BoardType type) {
  switch (type) {
    case BoardType::PAPER_S3:
      return "M5Stack Paper S3";
    case BoardType::LILYGO_T5_47:
      return "LilyGo T5 4.7\"";
    case BoardType::CUSTOM:
      return "Custom Board";
    default:
      return "Unknown";
  }
}

Board* BoardFactory::create_paper_s3() {
#if defined(BOARD_TYPE_PAPER_S3)
  LOG_I(TAG, "Creating M5Stack Paper S3 board");

  Board* board = new Board();
  board->type = BoardType::PAPER_S3;
  board->name = "M5Stack Paper S3";
  board->version = "1.0";

  // Display: epdiy with ED047TC2 panel (960x540)
  LOG_I(TAG, "Initializing display driver");
  board->display = new EpdiyDisplay();

  // Touch: GT911 on I2C0
  // SDA=GPIO41, SCL=GPIO42, INT=GPIO48
  LOG_I(TAG, "Initializing touch driver");
  board->touch = new Gt911Touch(
    I2C_NUM_0,
    GPIO_NUM_41,  // SDA
    GPIO_NUM_42,  // SCL
    GPIO_NUM_48   // INT (not used in polling mode)
  );

  // Battery: ADC monitoring via GPIO3 (ADC1_CHANNEL_2) with 2:1 divider
  LOG_I(TAG, "Initializing battery driver");
#ifdef BATTERY_ADC_CHANNEL
  board->battery = new AdcBattery(
    BATTERY_ADC_CHANNEL,  // Defined in platformio.ini as ADC1_CHANNEL_2
    2.0f                   // 2:1 voltage divider
  );
#else
  LOG_W(TAG, "Battery ADC channel not defined, battery monitoring disabled");
  board->battery = nullptr;
#endif

  // IMU: BMI270 on I2C (not implemented yet)
  LOG_W(TAG, "IMU driver not implemented, skipping");
  board->imu = nullptr;

  // Storage: SD card on SPI2
  // MISO=GPIO40, MOSI=GPIO38, CLK=GPIO39, CS=GPIO47
  LOG_I(TAG, "Initializing storage driver");
#if defined(SD_CARD_PIN_NUM_MISO) && defined(SD_CARD_PIN_NUM_MOSI) && \
    defined(SD_CARD_PIN_NUM_CLK) && defined(SD_CARD_PIN_NUM_CS)
  board->storage = new SdStorage(
    SD_CARD_PIN_NUM_MISO,  // GPIO40
    SD_CARD_PIN_NUM_MOSI,  // GPIO38
    SD_CARD_PIN_NUM_CLK,   // GPIO39
    SD_CARD_PIN_NUM_CS,    // GPIO47
    SPI2_HOST
  );
#else
  LOG_E(TAG, "SD card pins not defined in build configuration");
  board->storage = nullptr;
#endif

  // RTC: BM8563 on I2C (not implemented yet)
  LOG_W(TAG, "RTC driver not implemented, skipping");
  board->rtc = nullptr;

  LOG_I(TAG, "Board created successfully: %s v%s", board->name.c_str(), board->version.c_str());
  return board;
#else
  LOG_E(TAG, "BOARD_TYPE_PAPER_S3 not defined");
  return nullptr;
#endif
}

Board* BoardFactory::create_lilygo_t5_47() {
#if defined(BOARD_TYPE_LILYGO_T5_47)
  LOG_I(TAG, "Creating LilyGo T5 4.7\" board");

  Board* board = new Board();
  board->type = BoardType::LILYGO_T5_47;
  board->name = "LilyGo T5 4.7\"";
  board->version = "1.0";

  // TODO: Implement LilyGo T5 board configuration
  // Different pins, possibly different touch controller, etc.
  LOG_E(TAG, "LilyGo T5 board not fully implemented yet");

  return board;
#else
  LOG_E(TAG, "BOARD_TYPE_LILYGO_T5_47 not defined");
  return nullptr;
#endif
}
