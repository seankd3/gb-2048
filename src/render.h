#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include "board.h"

/* Screen is 20x18 tiles. Rows 0-1 hold the score bar; the 16x16 board fills
   rows 2-17 with a two-tile margin either side. */
#define BOARD_ORIGIN_X  2
#define BOARD_ORIGIN_Y  2

void render_init(void);
void render_clear(void);
void render_board(const Board *b);
void render_cell(uint8_t index, uint8_t exp);
void render_cell_at(uint8_t tx, uint8_t ty, uint8_t exp);
void render_score(uint32_t score, uint32_t best);
void render_gain(uint32_t gained);
void render_best_line(uint8_t y, uint32_t best);
void render_merge_pop(const Board *b, const MoveResult *res);

/* Animates every tile from its source to its destination. Call with the
   MoveResult from board_slide, before drawing the settled board. */
void render_slide(const MoveResult *res);
void render_text(uint8_t x, uint8_t y, const char *s);
void render_text_centered(uint8_t y, const char *s);
void render_banner(const char *line1, const char *line2);

#endif
