#include "hal/EpdiyDisplay.h"
#include "utils/Log.h"
#include "utils/Memory.h"
#include <cstring>

#if defined(BOARD_TYPE_PAPER_S3)
  #include <epdiy.h>
  #include <epd_highlevel.h>
  extern const EpdBoardDefinition paper_s3_board;
#else
  #include <epd_driver.h>
  #include <epd_highlevel.h>
#endif

static const char* TAG = "hal:display";

EpdiyDisplay::EpdiyDisplay()
  : m_width(0),
    m_height(0),
    m_rotation(DisplayRotation::ROTATE_0),
    m_framebuffer(nullptr),
    m_initialized(false),
    m_powered(false)
{
}

EpdiyDisplay::~EpdiyDisplay() {
  if (m_initialized) {
    power_off();
    epd_deinit();
    // Note: epdiy doesn't provide epd_hl_deinit, cleanup is done by epd_deinit
  }
}

bool EpdiyDisplay::init() {
  if (m_initialized) {
    LOG_W(TAG, "Display already initialized");
    return true;
  }

  LOG_I(TAG, "Initializing epdiy display");

#if defined(BOARD_TYPE_PAPER_S3)
  // For Paper S3 we use the new epdiy API with a custom board definition
  epd_set_board(&paper_s3_board);
  epd_init(epd_current_board(), &ED047TC2, EPD_OPTIONS_DEFAULT);

  // The C fallback for the ESP32-S3 LUT path is slower than the original
  // vector assembly, so a 20 MHz pixel clock can cause line buffer underruns.
  // Run the LCD at 5 MHz for stability.
  epd_set_lcd_pixel_clock_MHz(5);

  LOG_I(TAG, "Using Paper S3 board definition with ED047TC2 panel");
#else
  // Legacy epdiy API used by ESP32-based boards
  epd_init(EPD_OPTIONS_DEFAULT);
  LOG_I(TAG, "Using legacy epdiy API");
#endif

  // Initialize high-level interface
  m_hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);

  // Get framebuffer pointer (allocated in PSRAM by epdiy)
  m_framebuffer = epd_hl_get_framebuffer(&m_hl);
  if (!m_framebuffer) {
    LOG_E(TAG, "Failed to get framebuffer from epdiy");
    return false;
  }

  // Get display dimensions from the display struct
  const EpdDisplay_t* display = epd_get_display();
  if (!display) {
    LOG_E(TAG, "Failed to get display info from epdiy");
    return false;
  }

  m_width = display->width;
  m_height = display->height;

  LOG_I(TAG, "Display initialized: %dx%d, framebuffer @ %p",
        m_width, m_height, m_framebuffer);

  m_initialized = true;
  return true;
}

void EpdiyDisplay::power_on() {
  if (!m_initialized) {
    LOG_E(TAG, "Cannot power on: display not initialized");
    return;
  }

  if (m_powered) {
    return;
  }

  epd_poweron();
  m_powered = true;
  LOG_D(TAG, "Display powered on");
}

void EpdiyDisplay::power_off() {
  if (!m_initialized || !m_powered) {
    return;
  }

  epd_poweroff();
  m_powered = false;
  LOG_D(TAG, "Display powered off");
}

uint16_t EpdiyDisplay::get_width() const {
  // Account for rotation
  if (m_rotation == DisplayRotation::ROTATE_90 ||
      m_rotation == DisplayRotation::ROTATE_270) {
    return m_height;
  }
  return m_width;
}

uint16_t EpdiyDisplay::get_height() const {
  // Account for rotation
  if (m_rotation == DisplayRotation::ROTATE_90 ||
      m_rotation == DisplayRotation::ROTATE_270) {
    return m_width;
  }
  return m_height;
}

uint8_t* EpdiyDisplay::get_framebuffer() {
  if (!m_initialized) {
    LOG_E(TAG, "Cannot get framebuffer: display not initialized");
    return nullptr;
  }
  return m_framebuffer;
}

void EpdiyDisplay::update(DisplayMode mode) {
  if (!m_initialized) {
    LOG_E(TAG, "Cannot update: display not initialized");
    return;
  }

  // Map DisplayMode to epdiy mode
  enum EpdDrawMode epd_mode;
  const char* mode_name;

  switch (mode) {
    case DisplayMode::FULL:
      epd_mode = MODE_GC16;  // High quality, slowest
      mode_name = "FULL (GC16)";
      break;

    case DisplayMode::PARTIAL:
      epd_mode = MODE_GL16;  // Good quality, medium speed
      mode_name = "PARTIAL (GL16)";
      break;

    case DisplayMode::FAST:
      epd_mode = MODE_DU;    // Fast, 1-bit mode
      mode_name = "FAST (DU/A2)";
      break;

    case DisplayMode::DIRECT:
      epd_mode = MODE_DU;    // Direct update
      mode_name = "DIRECT (DU)";
      break;

    default:
      epd_mode = MODE_GC16;
      mode_name = "DEFAULT (GC16)";
      break;
  }

  LOG_D(TAG, "Updating display with mode: %s", mode_name);

  int temperature = get_temperature();
  epd_hl_update_screen(&m_hl, epd_mode, temperature);

  LOG_D(TAG, "Display update complete");
}

void EpdiyDisplay::clear() {
  if (!m_initialized) {
    LOG_E(TAG, "Cannot clear: display not initialized");
    return;
  }

  LOG_D(TAG, "Clearing display to white");

  // Set framebuffer to white (0xFF)
  memset(m_framebuffer, 0xFF, m_width * m_height);

  // Full clear with high quality
  int temperature = get_temperature();
  epd_fullclear(&m_hl, temperature);

  LOG_D(TAG, "Display cleared");
}

void EpdiyDisplay::set_rotation(DisplayRotation rotation) {
  m_rotation = rotation;
  LOG_I(TAG, "Display rotation set to %d", (int)rotation);

  // Note: Actual rotation transformation should be done by the renderer
  // when drawing to the framebuffer. This just stores the rotation state.
}

DisplayRotation EpdiyDisplay::get_rotation() const {
  return m_rotation;
}

int EpdiyDisplay::get_temperature() const {
  if (!m_initialized) {
    return 20;  // Default temperature
  }

#if defined(BOARD_TYPE_PAPER_S3)
  // Paper S3 board definition returns fixed 20°C
  // Could be enhanced to read from IMU temperature sensor
  const EpdBoardDefinition* board = epd_current_board();
  if (board && board->get_temperature) {
    float temp = board->get_temperature();
    LOG_D(TAG, "Display temperature: %.1f°C", temp);
    return (int)temp;
  }
#endif

  // Default to room temperature
  return 20;
}
