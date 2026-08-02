#ifndef MENU_H
#define MENU_H

#include <stdint.h>

typedef enum {
    MENU_RESUME = 0,
    MENU_NEW_GAME
} MenuResult;

/* Takes over the screen until the player leaves. The caller redraws the board
   afterwards. Settings are written back to the cartridge on exit, so a change
   survives a power cycle even if the run does not. */
MenuResult menu_open(uint32_t best);

#endif
