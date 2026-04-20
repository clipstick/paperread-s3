#pragma once

#include "hal/Board.h"

/**
 * Board Factory - Creates Board instances for specific hardware
 *
 * The factory selects the appropriate hardware implementations based on
 * compile-time defines (e.g., BOARD_TYPE_PAPER_S3, BOARD_TYPE_LILYGO_T5).
 *
 * Compile-time board selection is done via platformio.ini build flags:
 *   -DBOARD_TYPE_PAPER_S3       -> M5Stack Paper S3
 *   -DBOARD_TYPE_LILYGO_T5_47   -> LilyGo T5 4.7"
 *
 * Usage Example:
 *   // In main.cpp
 *   Board* board = BoardFactory::create();
 *   if (!board) {
 *     LOG_E("main", "Failed to create board");
 *     return;
 *   }
 *
 *   if (!board->init()) {
 *     LOG_E("main", "Failed to initialize board");
 *     delete board;
 *     return;
 *   }
 *
 *   // Use board->display, board->touch, etc.
 *
 *   // Cleanup
 *   delete board;
 */

class BoardFactory {
public:
  /**
   * Create a Board instance for the current hardware
   * The board type is selected based on compile-time defines
   * @return Pointer to Board instance, or nullptr on failure
   *         Caller is responsible for deleting the Board
   */
  static Board* create();

  /**
   * Get board type name as string
   * @param type Board type
   * @return Human-readable board name
   */
  static const char* get_board_name(BoardType type);

private:
  // Factory functions for specific boards
  static Board* create_paper_s3();
  static Board* create_lilygo_t5_47();
};
