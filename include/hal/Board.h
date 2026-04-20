#pragma once

#include "hal/IDisplay.h"
#include "hal/ITouch.h"
#include "hal/IBattery.h"
#include "hal/IIMU.h"
#include "hal/IStorage.h"
#include "hal/IRtc.h"
#include <string>

/**
 * Board Configuration and Hardware Access
 *
 * This struct provides centralized access to all hardware interfaces
 * for a specific board configuration. Different boards (M5Stack Paper S3,
 * LilyGo T5, etc.) have different hardware, so the Board struct is
 * populated by the BoardFactory based on compile-time configuration.
 *
 * Usage Example:
 *   // Get board instance for current hardware
 *   Board* board = BoardFactory::create();
 *
 *   // Initialize all hardware
 *   if (!board->init()) {
 *     LOG_E("app", "Board init failed");
 *     return;
 *   }
 *
 *   // Access hardware through interfaces
 *   board->display->init();
 *   board->storage->mount("/fs");
 *   board->touch->init();
 *
 *   // Clean up
 *   delete board;
 */

/**
 * Board identification
 */
enum class BoardType {
  UNKNOWN,
  PAPER_S3,      ///< M5Stack Paper S3
  LILYGO_T5_47,  ///< LilyGo T5 4.7"
  CUSTOM         ///< Custom board configuration
};

/**
 * Board hardware configuration and interface access
 */
struct Board {
  // Board identification
  BoardType type;
  std::string name;
  std::string version;

  // Hardware interfaces (pointers are owned by Board)
  IDisplay* display;
  ITouch* touch;
  IBattery* battery;
  IIMU* imu;
  IStorage* storage;
  IRtc* rtc;

  /**
   * Constructor - initializes all pointers to nullptr
   */
  Board()
    : type(BoardType::UNKNOWN),
      name("Unknown"),
      version("0.0"),
      display(nullptr),
      touch(nullptr),
      battery(nullptr),
      imu(nullptr),
      storage(nullptr),
      rtc(nullptr)
  {
  }

  /**
   * Destructor - cleans up all hardware interfaces
   */
  ~Board() {
    cleanup();
  }

  /**
   * Initialize all available hardware
   * @return true if critical hardware (display, storage) initialized successfully
   */
  bool init() {
    bool success = true;

    // Critical: Display
    if (display && !display->init()) {
      success = false;
    }

    // Critical: Storage
    if (storage && !storage->init()) {
      success = false;
    }

    // Optional: Touch
    if (touch) {
      touch->init();
    }

    // Optional: Battery
    if (battery) {
      battery->init();
    }

    // Optional: IMU
    if (imu) {
      imu->init();
    }

    // Optional: RTC
    if (rtc) {
      rtc->init();
    }

    return success;
  }

  /**
   * Clean up all hardware interfaces
   */
  void cleanup() {
    delete display;
    delete touch;
    delete battery;
    delete imu;
    delete storage;
    delete rtc;

    display = nullptr;
    touch = nullptr;
    battery = nullptr;
    imu = nullptr;
    storage = nullptr;
    rtc = nullptr;
  }

  // Disable copy (Board owns hardware pointers)
  Board(const Board&) = delete;
  Board& operator=(const Board&) = delete;
};
