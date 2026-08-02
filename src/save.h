#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

#include "board.h"

/* Best score and settings in battery-backed cartridge RAM. A missing or
   corrupt save yields best = 0 and default settings, so a fresh cartridge
   starts clean rather than showing whatever happened to be in RAM. */
void save_load(uint32_t *best);
void save_store(uint32_t best);

/* The game in progress, kept in its own block with its own magic and
   checksum. Deliberately separate from the block above: appending to that one
   would move its checksum and invalidate every existing save, throwing away
   the best score to add a feature. */
uint8_t save_load_game(Board *b, uint8_t *won);   /* 1 if a game was restored */
void    save_store_game(const Board *b, uint8_t won);
void    save_clear_game(void);

#endif
