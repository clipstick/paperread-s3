#pragma once

#include "hal/IStorage.h"
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>

/**
 * SD Card Storage Implementation
 *
 * Implements IStorage interface for SD/microSD cards using ESP-IDF's
 * VFS FAT filesystem layer over SPI (SDSPI) driver.
 *
 * Features:
 * - FAT filesystem support (FAT16/FAT32)
 * - SPI interface for maximum compatibility
 * - Auto-format option on mount failure
 * - Standard POSIX file operations
 * - Books and cache directory auto-creation
 *
 * Paper S3 Configuration:
 * - SPI Host: SPI2_HOST (HSPI)
 * - MISO: GPIO40
 * - MOSI: GPIO38
 * - CLK: GPIO39
 * - CS: GPIO47
 * - Default mount point: "/fs"
 *
 * Directory Structure:
 * - /fs/Books/ - EPUB files
 * - /fs/.paperread/ - Cache and settings
 */
class SdStorage : public IStorage {
public:
  /**
   * Constructor
   * @param miso MISO GPIO pin
   * @param mosi MOSI GPIO pin
   * @param clk Clock GPIO pin
   * @param cs Chip select GPIO pin
   * @param spi_host SPI host device (SPI2_HOST or SPI3_HOST)
   */
  SdStorage(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs,
            spi_host_device_t spi_host = SPI2_HOST);
  ~SdStorage() override;

  // IStorage implementation
  bool init() override;
  bool mount(const char* mount_point, bool format_if_failed = false) override;
  bool unmount() override;
  bool is_mounted() const override;
  bool format() override;
  bool get_info(StorageInfo* info) override;
  StorageType get_type() const override;
  std::string get_mount_point() const override;
  bool is_available() const override;

private:
  // Pin configuration
  gpio_num_t m_miso;
  gpio_num_t m_mosi;
  gpio_num_t m_clk;
  gpio_num_t m_cs;
  spi_host_device_t m_spi_host;

  // SD card state
  sdmmc_card_t* m_card;
  std::string m_mount_point;
  bool m_initialized;
  bool m_mounted;
};
