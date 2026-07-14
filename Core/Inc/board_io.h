#ifndef DINO_BOARD_IO_H
#define DINO_BOARD_IO_H
#include "dino_game.h"
void board_io_init(void);
uint8_t board_buttons_poll(uint32_t now_ms);
uint8_t board_buttons_held(void);
/* A raw 2x16 byte frame supports all eight LCD custom glyphs, including 0. */
void board_lcd_present(const uint8_t frame[2][DINO_COLUMNS]);
void board_status(bool fault);
#endif
