#include "hal/SdStorage.h"
#include "utils/Log.h"
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>
#include <sys/stat.h>
#include <cstring>

static const char* TAG = "hal:storage";

// Use auto-allocated DMA channel for ESP32-S3 compatibility
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO

SdStorage::SdStorage(gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, spi_host_device_t spi_host)
  : m_miso(miso),
    m_mosi(mosi),
    m_clk(clk),
    m_cs(cs),
    m_spi_host(spi_host),
    m_card(nullptr),
    m_initialized(false),
    m_mounted(false)
{
}

SdStorage::~SdStorage() {
  if (m_mounted) {
    unmount();
  }

  if (m_initialized) {
    spi_bus_free(m_spi_host);
  }
}

bool SdStorage::init() {
  if (m_initialized) {
    LOG_W(TAG, "Storage already initialized");
    return true;
  }

  LOG_I(TAG, "Initializing SD card storage");

  // Configure SPI bus
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = m_mosi,
    .miso_io_num = m_miso,
    .sclk_io_num = m_clk,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0,
    .flags = 0,
    .intr_flags = 0
  };

  esp_err_t ret = spi_bus_initialize(m_spi_host, &bus_cfg, SPI_DMA_CHAN);
  if (ret != ESP_OK) {
    if (ret == ESP_ERR_INVALID_STATE) {
      LOG_W(TAG, "SPI bus already initialized");
      // This is okay - bus might be shared
    } else {
      LOG_E(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
      return false;
    }
  }

  m_initialized = true;
  LOG_I(TAG, "SD card storage initialized (SPI host %d)", m_spi_host);
  return true;
}

bool SdStorage::mount(const char* mount_point, bool format_if_failed) {
  if (!m_initialized) {
    LOG_E(TAG, "Cannot mount: storage not initialized");
    return false;
  }

  if (m_mounted) {
    LOG_W(TAG, "Storage already mounted at %s", m_mount_point.c_str());
    return true;
  }

  LOG_I(TAG, "Mounting SD card at %s", mount_point);

  m_mount_point = mount_point;

  // Configure FAT filesystem mount options
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = format_if_failed,
    .max_files = 10,
    .allocation_unit_size = 16 * 1024  // 16KB clusters
  };

  // Configure SD SPI slot
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20MHz for SD

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = m_cs;
  slot_config.host_id = m_spi_host;

  // Mount the filesystem
  esp_err_t ret = esp_vfs_fat_sdspi_mount(m_mount_point.c_str(), &host, &slot_config, &mount_config, &m_card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      LOG_E(TAG, "Failed to mount filesystem. Try format_if_failed=true");
    } else {
      LOG_E(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    }
    return false;
  }

  // Print card info
  sdmmc_card_print_info(stdout, m_card);

  // Create Books directory if it doesn't exist
  std::string books_dir = m_mount_point + "/Books";
  struct stat st;
  if (stat(books_dir.c_str(), &st) != 0) {
    if (mkdir(books_dir.c_str(), 0775) == 0) {
      LOG_I(TAG, "Created Books directory: %s", books_dir.c_str());
    }
  }

  // Create .paperread directory for cache
  std::string cache_dir = m_mount_point + "/.paperread";
  if (stat(cache_dir.c_str(), &st) != 0) {
    if (mkdir(cache_dir.c_str(), 0775) == 0) {
      LOG_I(TAG, "Created cache directory: %s", cache_dir.c_str());
    }
  }

  m_mounted = true;
  LOG_I(TAG, "SD card mounted successfully at %s", m_mount_point.c_str());
  return true;
}

bool SdStorage::unmount() {
  if (!m_mounted) {
    LOG_W(TAG, "Storage not mounted");
    return true;
  }

  LOG_I(TAG, "Unmounting SD card from %s", m_mount_point.c_str());

  esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_mount_point.c_str(), m_card);
  if (ret != ESP_OK) {
    LOG_E(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    return false;
  }

  m_card = nullptr;
  m_mounted = false;
  m_mount_point.clear();

  LOG_I(TAG, "SD card unmounted successfully");
  return true;
}

bool SdStorage::is_mounted() const {
  return m_mounted;
}

bool SdStorage::format() {
  if (m_mounted) {
    LOG_E(TAG, "Cannot format: storage is mounted. Unmount first.");
    return false;
  }

  LOG_W(TAG, "Formatting SD card (all data will be lost!)");

  // Mount with format_if_failed=true forces formatting
  bool result = mount("/fs", true);

  if (result) {
    // Immediately unmount after formatting
    unmount();
    LOG_I(TAG, "SD card formatted successfully");
  }

  return result;
}

bool SdStorage::get_info(StorageInfo* info) {
  if (!info) {
    return false;
  }

  if (!m_mounted || !m_card) {
    LOG_E(TAG, "Cannot get storage info: not mounted");
    return false;
  }

  // Get card capacity
  uint64_t card_total_bytes = ((uint64_t)m_card->csd.capacity) * m_card->csd.sector_size;

  // Get filesystem info (free/used space)
  uint64_t total_bytes = 0;
  uint64_t free_bytes = 0;

  esp_err_t ret = esp_vfs_fat_info(m_mount_point.c_str(), &total_bytes, &free_bytes);

  if (ret != ESP_OK) {
    LOG_E(TAG, "Failed to get filesystem info: %s", esp_err_to_name(ret));
    // Fall back to card capacity
    total_bytes = card_total_bytes;
    free_bytes = 0;
  }

  // Calculate used space
  uint64_t used_bytes = (total_bytes > free_bytes) ? (total_bytes - free_bytes) : 0;

  info->type = StorageType::SD_CARD;
  info->total_bytes = total_bytes;
  info->used_bytes = used_bytes;
  info->free_bytes = free_bytes;
  info->is_mounted = m_mounted;
  info->mount_point = m_mount_point;

  LOG_D(TAG, "Storage info: %llu MB total, %llu MB used, %llu MB free",
        total_bytes / (1024 * 1024),
        used_bytes / (1024 * 1024),
        free_bytes / (1024 * 1024));

  return true;
}

StorageType SdStorage::get_type() const {
  return StorageType::SD_CARD;
}

std::string SdStorage::get_mount_point() const {
  return m_mount_point;
}

bool SdStorage::is_available() const {
  // SD card is available if initialized and card is present
  return m_initialized && m_card != nullptr;
}
