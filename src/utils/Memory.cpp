#include "utils/Memory.h"

#ifndef UNIT_TEST
  #include <esp_heap_caps.h>
  #include <cstring>
#else
  #include <cstdlib>
  #include <cstring>
#endif

#include "utils/Log.h"

static const char* TAG = "utils:memory";

void* psram_malloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

#ifndef UNIT_TEST
  void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (!ptr) {
    LOG_E(TAG, "PSRAM allocation failed: %zu bytes", size);
  } else {
    LOG_D(TAG, "PSRAM allocated: %zu bytes at %p", size, ptr);
  }
  return ptr;
#else
  // Unit test fallback - use regular malloc
  return malloc(size);
#endif
}

void* psram_calloc(size_t size) {
  void* ptr = psram_malloc(size);
  if (ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

void psram_free(void* ptr) {
  if (!ptr) {
    return;
  }

#ifndef UNIT_TEST
  LOG_D(TAG, "PSRAM freed: %p", ptr);
  heap_caps_free(ptr);
#else
  free(ptr);
#endif
}

size_t psram_get_total_size() {
#ifndef UNIT_TEST
  return heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
#else
  return 8 * 1024 * 1024; // Mock 8MB for tests
#endif
}

size_t psram_get_free_size() {
#ifndef UNIT_TEST
  return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
  return 8 * 1024 * 1024; // Mock value for tests
#endif
}
