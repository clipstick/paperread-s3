#pragma once

#include <cstdint>
#include <string>

/**
 * Storage Interface - Hardware abstraction for file storage
 *
 * This interface abstracts file storage backends (SD card, SPIFFS,
 * LittleFS, etc.), providing a unified API for file operations.
 *
 * The storage is mounted at a specific mount point (e.g., "/fs")
 * and uses standard POSIX file operations underneath.
 *
 * Usage Example:
 *   IStorage* storage = board.storage;
 *
 *   if (!storage->init()) {
 *     LOG_E("app", "Storage init failed");
 *     return;
 *   }
 *
 *   if (!storage->mount("/fs")) {
 *     LOG_E("app", "Failed to mount storage");
 *     return;
 *   }
 *
 *   // Now use standard file I/O at /fs/...
 *   FILE* f = fopen("/fs/Books/mybook.epub", "rb");
 *
 *   // Unmount when done (before sleep/shutdown)
 *   storage->unmount();
 */

/**
 * Storage type
 */
enum class StorageType {
  UNKNOWN,
  SD_CARD,    ///< SD/microSD card
  SPIFFS,     ///< SPIFFS flash filesystem
  LITTLEFS,   ///< LittleFS flash filesystem
  FATFS       ///< FAT filesystem (SD or flash)
};

/**
 * Storage information
 */
struct StorageInfo {
  StorageType type;        ///< Storage type
  uint64_t total_bytes;    ///< Total capacity in bytes
  uint64_t used_bytes;     ///< Used space in bytes
  uint64_t free_bytes;     ///< Free space in bytes
  bool is_mounted;         ///< Mounted status
  std::string mount_point; ///< Mount point (e.g., "/fs")
};

/**
 * Abstract interface for file storage hardware
 */
class IStorage {
public:
  virtual ~IStorage() = default;

  /**
   * Initialize storage hardware
   * Does not mount the filesystem yet - call mount() separately
   * @return true on success, false on failure
   */
  virtual bool init() = 0;

  /**
   * Mount the filesystem at specified path
   * @param mount_point Path to mount at (e.g., "/fs")
   * @param format_if_failed If true, format and retry on mount failure
   * @return true on success, false on failure
   */
  virtual bool mount(const char* mount_point, bool format_if_failed = false) = 0;

  /**
   * Unmount the filesystem
   * Always call before sleep/shutdown to ensure data integrity
   * @return true on success, false on failure
   */
  virtual bool unmount() = 0;

  /**
   * Check if storage is currently mounted
   * @return true if mounted, false otherwise
   */
  virtual bool is_mounted() const = 0;

  /**
   * Format the storage (erases all data!)
   * Must be unmounted before formatting
   * @return true on success, false on failure
   */
  virtual bool format() = 0;

  /**
   * Get storage information
   * @param info Pointer to StorageInfo struct to fill
   * @return true on success, false on failure
   */
  virtual bool get_info(StorageInfo* info) = 0;

  /**
   * Get storage type
   * @return Storage type
   */
  virtual StorageType get_type() const = 0;

  /**
   * Get mount point
   * @return Mount point path (e.g., "/fs") or empty if not mounted
   */
  virtual std::string get_mount_point() const = 0;

  /**
   * Check if storage hardware is present and working
   * For SD cards, this checks if a card is inserted
   * @return true if available, false otherwise
   */
  virtual bool is_available() const = 0;
};
