#pragma once

#include <cstddef>
#include <cstdint>

/**
 * PSRAM Memory Management Utilities
 *
 * The M5Stack Paper S3 has 8MB of PSRAM (external SPIRAM). All large
 * allocations (>4KB) should use PSRAM to preserve internal RAM for
 * stack, ISRs, and DMA buffers.
 *
 * Usage:
 *   // Allocate raw PSRAM
 *   uint8_t* buffer = psram_malloc(100 * 1024);
 *   if (!buffer) { handle_error(); }
 *   // ... use buffer ...
 *   psram_free(buffer);
 *
 *   // RAII wrapper (preferred)
 *   PsramBuffer<uint8_t> buffer(100 * 1024);
 *   if (!buffer.get()) { handle_error(); }
 *   // ... use buffer.get() ...
 *   // automatically freed when buffer goes out of scope
 */

/**
 * Allocate memory from PSRAM (external SPIRAM)
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or nullptr on failure
 */
void* psram_malloc(size_t size);

/**
 * Allocate zero-initialized memory from PSRAM
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or nullptr on failure
 */
void* psram_calloc(size_t size);

/**
 * Free memory allocated with psram_malloc or psram_calloc
 * @param ptr Pointer to memory to free (safe to pass nullptr)
 */
void psram_free(void* ptr);

/**
 * Get total PSRAM size in bytes
 * @return Total PSRAM capacity
 */
size_t psram_get_total_size();

/**
 * Get free PSRAM size in bytes
 * @return Available PSRAM
 */
size_t psram_get_free_size();

/**
 * RAII wrapper for PSRAM allocations
 * Automatically frees memory when the object goes out of scope
 *
 * Example:
 *   {
 *     PsramBuffer<uint8_t> framebuffer(960 * 540);
 *     if (framebuffer.get()) {
 *       memset(framebuffer.get(), 0xFF, framebuffer.size());
 *       // ... render to framebuffer ...
 *     }
 *     // framebuffer automatically freed here
 *   }
 */
template<typename T>
class PsramBuffer {
public:
  /**
   * Allocate buffer in PSRAM
   * @param count Number of elements (not bytes)
   */
  explicit PsramBuffer(size_t count)
    : m_ptr(nullptr), m_size(count * sizeof(T))
  {
    if (m_size > 0) {
      m_ptr = static_cast<T*>(psram_malloc(m_size));
    }
  }

  // Disable copy (move would be ok but not needed yet)
  PsramBuffer(const PsramBuffer&) = delete;
  PsramBuffer& operator=(const PsramBuffer&) = delete;

  ~PsramBuffer() {
    if (m_ptr) {
      psram_free(m_ptr);
      m_ptr = nullptr;
    }
  }

  /**
   * Get raw pointer to buffer
   * @return Pointer to buffer, or nullptr if allocation failed
   */
  T* get() { return m_ptr; }
  const T* get() const { return m_ptr; }

  /**
   * Get buffer size in bytes
   * @return Size of buffer
   */
  size_t size() const { return m_size; }

  /**
   * Check if allocation succeeded
   * @return true if buffer is valid
   */
  explicit operator bool() const { return m_ptr != nullptr; }

private:
  T* m_ptr;
  size_t m_size;
};
