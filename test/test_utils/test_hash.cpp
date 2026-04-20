#include <unity.h>
#include "utils/Hash.h"
#include <cstring>

void setUp(void) {
  // Set up before each test
}

void tearDown(void) {
  // Clean up after each test
}

// Test hash stability - same input produces same output
void test_hash_stability() {
  std::string path = "/fs/Books/mybook.epub";

  uint64_t hash1 = hash_file_path(path);
  uint64_t hash2 = hash_file_path(path);

  TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

// Test different inputs produce different hashes
void test_hash_uniqueness() {
  std::string path1 = "/fs/Books/book1.epub";
  std::string path2 = "/fs/Books/book2.epub";

  uint64_t hash1 = hash_file_path(path1);
  uint64_t hash2 = hash_file_path(path2);

  TEST_ASSERT_NOT_EQUAL_UINT64(hash1, hash2);
}

// Test hash_data function
void test_hash_data() {
  const char* data1 = "Hello, World!";
  const char* data2 = "Hello, World!";
  const char* data3 = "Hello, World?";

  uint64_t hash1 = hash_data(data1, strlen(data1));
  uint64_t hash2 = hash_data(data2, strlen(data2));
  uint64_t hash3 = hash_data(data3, strlen(data3));

  // Same data produces same hash
  TEST_ASSERT_EQUAL_UINT64(hash1, hash2);

  // Different data produces different hash
  TEST_ASSERT_NOT_EQUAL_UINT64(hash1, hash3);
}

// Test hash_to_hex conversion
void test_hash_to_hex() {
  uint64_t hash = 0x0123456789abcdefULL;
  std::string hex = hash_to_hex(hash);

  TEST_ASSERT_EQUAL_STRING("0123456789abcdef", hex.c_str());
  TEST_ASSERT_EQUAL_size_t(16, hex.length());
}

// Test zero hash
void test_hash_zero() {
  uint64_t hash = 0;
  std::string hex = hash_to_hex(hash);

  TEST_ASSERT_EQUAL_STRING("0000000000000000", hex.c_str());
}

// Test max hash
void test_hash_max() {
  uint64_t hash = 0xFFFFFFFFFFFFFFFFULL;
  std::string hex = hash_to_hex(hash);

  TEST_ASSERT_EQUAL_STRING("ffffffffffffffff", hex.c_str());
}

// Test get_cache_dir_for_epub
void test_get_cache_dir() {
  std::string epub_path = "/fs/Books/test.epub";
  std::string cache_dir = get_cache_dir_for_epub(epub_path);

  // Should start with the cache prefix
  TEST_ASSERT_TRUE(cache_dir.find("/fs/.paperread/epub_") == 0);

  // Should be followed by 16 hex characters
  TEST_ASSERT_EQUAL_size_t(16 + 20, cache_dir.length()); // "/fs/.paperread/epub_" (20) + hash (16)

  // Same path should produce same cache dir
  std::string cache_dir2 = get_cache_dir_for_epub(epub_path);
  TEST_ASSERT_EQUAL_STRING(cache_dir.c_str(), cache_dir2.c_str());
}

// Test known hash value (regression test)
// This ensures xxHash64 implementation doesn't change
void test_known_hash_value() {
  const char* data = "The quick brown fox jumps over the lazy dog";
  uint64_t hash = hash_data(data, strlen(data));

  // xxHash64 of this string with seed 0 is known
  // This is the actual xxHash64 value for this string
  uint64_t expected = 0x0b242d361fda71bcULL;

  TEST_ASSERT_EQUAL_UINT64(expected, hash);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_hash_stability);
  RUN_TEST(test_hash_uniqueness);
  RUN_TEST(test_hash_data);
  RUN_TEST(test_hash_to_hex);
  RUN_TEST(test_hash_zero);
  RUN_TEST(test_hash_max);
  RUN_TEST(test_get_cache_dir);
  RUN_TEST(test_known_hash_value);

  return UNITY_END();
}
