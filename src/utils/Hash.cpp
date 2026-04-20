#include "utils/Hash.h"

// xxHash is a header-only library, we need to define the implementation
#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#include "utils/xxhash.h"

#include <cstdio>
#include "utils/Log.h"

static const char* TAG = "utils:hash";

uint64_t hash_file_path(const std::string& path) {
  return hash_data(path.c_str(), path.length());
}

uint64_t hash_data(const void* data, size_t size) {
  // Use xxHash64 with seed 0
  uint64_t hash = XXH64(data, size, 0);
  LOG_D(TAG, "Hashed %zu bytes -> 0x%016llx", size, (unsigned long long)hash);
  return hash;
}

std::string hash_to_hex(uint64_t hash) {
  char hex[17];
  snprintf(hex, sizeof(hex), "%016llx", (unsigned long long)hash);
  return std::string(hex);
}

std::string get_cache_dir_for_epub(const std::string& epub_path) {
  uint64_t hash = hash_file_path(epub_path);
  std::string cache_dir = "/fs/.paperread/epub_" + hash_to_hex(hash);
  LOG_D(TAG, "Cache dir for '%s': %s", epub_path.c_str(), cache_dir.c_str());
  return cache_dir;
}
