#include <unity.h>
#include "utils/Memory.h"
#include <cstring>

void setUp(void) {
  // Set up before each test
}

void tearDown(void) {
  // Clean up after each test
}

// Test basic allocation and deallocation
void test_psram_malloc_free() {
  size_t size = 1024;
  void* ptr = psram_malloc(size);

  TEST_ASSERT_NOT_NULL(ptr);

  // Write to the buffer to ensure it's accessible
  memset(ptr, 0xAA, size);

  // Verify the write
  uint8_t* bytes = static_cast<uint8_t*>(ptr);
  TEST_ASSERT_EQUAL_UINT8(0xAA, bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0xAA, bytes[size - 1]);

  psram_free(ptr);
}

// Test calloc zeros memory
void test_psram_calloc() {
  size_t size = 1024;
  void* ptr = psram_calloc(size);

  TEST_ASSERT_NOT_NULL(ptr);

  // Verify all bytes are zero
  uint8_t* bytes = static_cast<uint8_t*>(ptr);
  for (size_t i = 0; i < size; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, bytes[i]);
  }

  psram_free(ptr);
}

// Test PsramBuffer RAII wrapper
void test_psram_buffer_raii() {
  {
    PsramBuffer<uint8_t> buffer(1024);
    TEST_ASSERT_TRUE(buffer);
    TEST_ASSERT_NOT_NULL(buffer.get());
    TEST_ASSERT_EQUAL_size_t(1024, buffer.size());

    // Use the buffer
    memset(buffer.get(), 0xBB, buffer.size());
    TEST_ASSERT_EQUAL_UINT8(0xBB, buffer.get()[0]);

    // Buffer will be automatically freed when scope ends
  }
  // If we had memory tracking, we'd verify the buffer was freed here
}

// Test large allocation
void test_psram_large_allocation() {
  // Allocate 1MB
  size_t size = 1024 * 1024;
  PsramBuffer<uint8_t> buffer(size);

  TEST_ASSERT_TRUE(buffer);
  TEST_ASSERT_NOT_NULL(buffer.get());

  // Write to various points in the buffer
  buffer.get()[0] = 0x11;
  buffer.get()[size / 2] = 0x22;
  buffer.get()[size - 1] = 0x33;

  TEST_ASSERT_EQUAL_UINT8(0x11, buffer.get()[0]);
  TEST_ASSERT_EQUAL_UINT8(0x22, buffer.get()[size / 2]);
  TEST_ASSERT_EQUAL_UINT8(0x33, buffer.get()[size - 1]);
}

// Test zero-size allocation
void test_psram_zero_size() {
  void* ptr = psram_malloc(0);
  TEST_ASSERT_NULL(ptr);
}

// Test free of nullptr (should not crash)
void test_psram_free_nullptr() {
  psram_free(nullptr);
  // If we get here without crashing, test passes
  TEST_PASS();
}

int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_psram_malloc_free);
  RUN_TEST(test_psram_calloc);
  RUN_TEST(test_psram_buffer_raii);
  RUN_TEST(test_psram_large_allocation);
  RUN_TEST(test_psram_zero_size);
  RUN_TEST(test_psram_free_nullptr);

  return UNITY_END();
}
