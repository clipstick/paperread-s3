#pragma once

/**
 * Unified logging macros for PaperRead S3
 *
 * Usage:
 *   LOG_D("epub:parser", "Parsing chapter %d", chapter_num);
 *   LOG_I("hal:display", "Display initialized: %dx%d", width, height);
 *   LOG_W("net:wifi", "Connection retry %d of %d", retry, max_retries);
 *   LOG_E("storage:sd", "Failed to mount SD card: %s", error_msg);
 *
 * Tag naming convention:
 *   - "<component>:<subcomponent>" format
 *   - Examples: "epub:parser", "hal:display", "ui:library", "net:ota"
 *
 * Never use raw printf() or ESP_LOGx() directly in application code.
 */

#ifndef UNIT_TEST
  #include <esp_log.h>

  #define LOG_D(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
  #define LOG_I(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
  #define LOG_W(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
  #define LOG_E(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#else
  // Unit test environment - output to stdout
  #include <cstdio>

  #define LOG_D(tag, fmt, ...) printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define LOG_I(tag, fmt, ...) printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define LOG_W(tag, fmt, ...) printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define LOG_E(tag, fmt, ...) printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#endif
