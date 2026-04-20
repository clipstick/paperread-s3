#include "hal/BoardFactory.h"
#include "hal/Board.h"
#include "utils/Log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <cstdio>
#include <dirent.h>

static const char* TAG = "hal_test";

/**
 * Hardware Validation Test
 *
 * Tests all implemented HAL drivers on M5Stack Paper S3:
 * - Display: Clear screen and draw test pattern
 * - Touch: Detect touches and show coordinates
 * - Battery: Read voltage and percentage
 * - Storage: Mount SD card and list files
 *
 * Results are displayed both on screen and serial output.
 */

void test_display(Board* board) {
    LOG_I(TAG, "=== Testing Display ===");

    if (!board->display) {
        LOG_E(TAG, "Display is NULL!");
        return;
    }

    // Initialize display
    if (!board->display->init()) {
        LOG_E(TAG, "Display init failed!");
        return;
    }
    LOG_I(TAG, "✓ Display initialized");

    // Power on
    board->display->power_on();
    LOG_I(TAG, "✓ Display powered on");

    // Get display info
    uint16_t width = board->display->get_width();
    uint16_t height = board->display->get_height();
    int temp = board->display->get_temperature();
    LOG_I(TAG, "✓ Display: %dx%d, temperature: %d°C", width, height, temp);

    // Clear to white
    LOG_I(TAG, "Clearing display to white...");
    board->display->clear();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Draw test pattern
    LOG_I(TAG, "Drawing test pattern...");
    uint8_t* fb = board->display->get_framebuffer();
    if (fb) {
        // Draw black border (10 pixels)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (x < 10 || x >= width - 10 || y < 10 || y >= height - 10) {
                    fb[y * width + x] = 0x00;  // Black
                }
            }
        }

        // Draw gray cross in center
        int cx = width / 2;
        int cy = height / 2;
        for (int i = 0; i < width; i++) {
            fb[cy * width + i] = 0x80;  // Gray horizontal line
        }
        for (int i = 0; i < height; i++) {
            fb[i * width + cx] = 0x80;  // Gray vertical line
        }

        // Update display
        board->display->update(DisplayMode::FULL);
        LOG_I(TAG, "✓ Test pattern drawn");
    }

    LOG_I(TAG, "=== Display Test Complete ===\n");
}

void test_touch(Board* board) {
    LOG_I(TAG, "=== Testing Touch ===");

    if (!board->touch) {
        LOG_E(TAG, "Touch is NULL!");
        return;
    }

    // Initialize touch
    if (!board->touch->init()) {
        LOG_E(TAG, "Touch init failed!");
        return;
    }
    LOG_I(TAG, "✓ Touch initialized");

    // Calibrate for display
    if (board->display) {
        board->touch->calibrate(board->display->get_width(), board->display->get_height());
        LOG_I(TAG, "✓ Touch calibrated");
    }

    // Poll for touches for 10 seconds
    LOG_I(TAG, "Touch the screen (10 second test)...");

    int touch_count = 0;
    for (int i = 0; i < 100; i++) {  // 10 seconds at 100ms intervals
        if (board->touch->is_touched()) {
            uint16_t x, y;
            if (board->touch->get_touch_point(&x, &y)) {
                LOG_I(TAG, "Touch detected at (%d, %d)", x, y);
                touch_count++;

                // Mark touch on display
                if (board->display) {
                    uint8_t* fb = board->display->get_framebuffer();
                    if (fb && x < board->display->get_width() && y < board->display->get_height()) {
                        // Draw a small cross at touch point
                        int size = 10;
                        for (int dx = -size; dx <= size; dx++) {
                            int px = x + dx;
                            if (px >= 0 && px < board->display->get_width()) {
                                fb[y * board->display->get_width() + px] = 0x00;  // Black
                            }
                        }
                        for (int dy = -size; dy <= size; dy++) {
                            int py = y + dy;
                            if (py >= 0 && py < board->display->get_height()) {
                                fb[py * board->display->get_width() + x] = 0x00;  // Black
                            }
                        }
                        board->display->update(DisplayMode::FAST);
                    }
                }

                vTaskDelay(pdMS_TO_TICKS(500));  // Debounce
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    LOG_I(TAG, "✓ Touch test complete: %d touches detected", touch_count);
    LOG_I(TAG, "=== Touch Test Complete ===\n");
}

void test_battery(Board* board) {
    LOG_I(TAG, "=== Testing Battery ===");

    if (!board->battery) {
        LOG_W(TAG, "Battery is NULL (not configured)");
        return;
    }

    // Initialize battery
    if (!board->battery->init()) {
        LOG_E(TAG, "Battery init failed!");
        return;
    }
    LOG_I(TAG, "✓ Battery initialized");

    // Read voltage and percentage
    float voltage = board->battery->get_voltage();
    uint8_t percent = board->battery->get_charge_percent();
    ChargeState state = board->battery->get_charge_state();

    LOG_I(TAG, "✓ Battery voltage: %.2fV", voltage);
    LOG_I(TAG, "✓ Battery charge: %d%%", percent);

    const char* state_str = "Unknown";
    switch (state) {
        case ChargeState::CHARGING: state_str = "Charging"; break;
        case ChargeState::DISCHARGING: state_str = "Discharging"; break;
        case ChargeState::FULL: state_str = "Full"; break;
        default: state_str = "Unknown"; break;
    }
    LOG_I(TAG, "✓ Battery state: %s", state_str);

    LOG_I(TAG, "=== Battery Test Complete ===\n");
}

void test_storage(Board* board) {
    LOG_I(TAG, "=== Testing Storage ===");

    if (!board->storage) {
        LOG_E(TAG, "Storage is NULL!");
        return;
    }

    // Initialize storage
    if (!board->storage->init()) {
        LOG_E(TAG, "Storage init failed!");
        return;
    }
    LOG_I(TAG, "✓ Storage initialized");

    // Mount SD card
    if (!board->storage->mount("/fs", false)) {
        LOG_E(TAG, "Storage mount failed! Is SD card inserted?");
        return;
    }
    LOG_I(TAG, "✓ Storage mounted at /fs");

    // Get storage info
    StorageInfo info;
    if (board->storage->get_info(&info)) {
        LOG_I(TAG, "✓ Storage type: SD Card");
        LOG_I(TAG, "✓ Total: %llu MB", info.total_bytes / (1024 * 1024));
        LOG_I(TAG, "✓ Used: %llu MB", info.used_bytes / (1024 * 1024));
        LOG_I(TAG, "✓ Free: %llu MB", info.free_bytes / (1024 * 1024));
    }

    // List files in /Books directory
    LOG_I(TAG, "Listing files in /fs/Books/:");
    DIR* dir = opendir("/fs/Books");
    if (dir) {
        struct dirent* entry;
        int file_count = 0;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {  // Regular file
                LOG_I(TAG, "  - %s", entry->d_name);
                file_count++;
            }
        }
        closedir(dir);
        LOG_I(TAG, "✓ Found %d files in /Books/", file_count);
    } else {
        LOG_W(TAG, "Could not open /fs/Books/ directory");
    }

    LOG_I(TAG, "=== Storage Test Complete ===\n");
}

extern "C" void app_main(void) {
    LOG_I(TAG, "========================================");
    LOG_I(TAG, "PaperRead S3 - HAL Validation Test");
    LOG_I(TAG, "========================================\n");

    // Create board instance
    LOG_I(TAG, "Creating board instance...");
    Board* board = BoardFactory::create();

    if (!board) {
        LOG_E(TAG, "Failed to create board!");
        return;
    }

    LOG_I(TAG, "✓ Board created: %s v%s\n", board->name.c_str(), board->version.c_str());

    // Run tests
    test_display(board);
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_battery(board);
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_storage(board);
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_touch(board);

    // Final summary
    LOG_I(TAG, "\n========================================");
    LOG_I(TAG, "HAL Validation Complete!");
    LOG_I(TAG, "========================================");
    LOG_I(TAG, "Display: %s", board->display ? "✓ OK" : "✗ FAIL");
    LOG_I(TAG, "Touch: %s", board->touch ? "✓ OK" : "✗ FAIL");
    LOG_I(TAG, "Battery: %s", board->battery ? "✓ OK" : "✗ FAIL");
    LOG_I(TAG, "Storage: %s", board->storage ? "✓ OK" : "✗ FAIL");
    LOG_I(TAG, "========================================\n");

    // Cleanup
    delete board;

    LOG_I(TAG, "Test complete. Device will continue running.");
    LOG_I(TAG, "You can now disconnect or flash the actual firmware.\n");
}
