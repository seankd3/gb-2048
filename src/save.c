#include <gbdk/platform.h>
#include <gb/gb.h>

#include "save.h"
#include "settings.h"
#include "gfx.h"

/* Bumped when the block layout changed to carry settings. An older save fails
   the check and falls back to defaults, which is the correct outcome. */
#define SAVE_MAGIC 0x3249u

typedef struct {
    uint16_t magic;
    uint32_t best;
    uint8_t  speed;
    uint8_t  sound;
    uint8_t  check;
} SaveBlock;

static SaveBlock __at(0xA000) sram;

/* Second block, placed clear of the first so the two can evolve separately. */
#define GAME_MAGIC 0x4750u   /* 'GP' */

typedef struct {
    uint16_t magic;
    uint8_t  cells[BOARD_CELLS];
    uint32_t score;
    uint8_t  won;
    uint8_t  check;
} GameBlock;

static GameBlock __at(0xA040) sram_game;

static uint8_t game_checksum(const uint8_t *cells, uint32_t score, uint8_t won) {
    uint8_t i, c = 0x5Au;
    for (i = 0; i < BOARD_CELLS; i++) c ^= cells[i];
    c ^= (uint8_t)(score) ^ (uint8_t)(score >> 8) ^
         (uint8_t)(score >> 16) ^ (uint8_t)(score >> 24);
    c ^= won;
    return c;
}

/* Magic alone is not enough: a half-written or randomly-matching cell would
   pass. The checksum makes a bogus save vanishingly unlikely. */
static uint8_t checksum(uint32_t best, uint8_t speed, uint8_t sound) {
    return (uint8_t)((uint8_t)(best) ^ (uint8_t)(best >> 8) ^
                     (uint8_t)(best >> 16) ^ (uint8_t)(best >> 24) ^
                     speed ^ sound ^ 0xA5u);
}

void save_load(uint32_t *best) {
    uint32_t stored_best;
    uint8_t  speed, sound;

    settings_defaults();
    *best = 0;

    ENABLE_RAM;
    if (sram.magic == SAVE_MAGIC) {
        stored_best = sram.best;
        speed = sram.speed;
        sound = sram.sound;
        if (sram.check == checksum(stored_best, speed, sound) &&
            speed < SPEED_COUNT && sound <= 1) {
            *best = stored_best;
            settings.speed = speed;
            settings.sound = sound;
        }
    }
    DISABLE_RAM;
}

void save_store(uint32_t best) {
    ENABLE_RAM;
    sram.magic = SAVE_MAGIC;
    sram.best  = best;
    sram.speed = settings.speed;
    sram.sound = settings.sound;
    sram.check = checksum(best, settings.speed, settings.sound);
    DISABLE_RAM;
}

uint8_t save_load_game(Board *b, uint8_t *won) {
    uint8_t i, ok = 0;
    uint8_t cells[BOARD_CELLS];
    uint32_t score;
    uint8_t w;

    ENABLE_RAM;
    if (sram_game.magic == GAME_MAGIC) {
        for (i = 0; i < BOARD_CELLS; i++) cells[i] = sram_game.cells[i];
        score = sram_game.score;
        w     = sram_game.won;
        if (sram_game.check == game_checksum(cells, score, w)) ok = 1;
    }
    DISABLE_RAM;

    if (!ok) return 0;

    /* A stored exponent past the table would index the tile art out of range,
       so a corrupt-but-checksummed block is still refused. */
    for (i = 0; i < BOARD_CELLS; i++) {
        if (cells[i] > GFX_MAX_EXP) return 0;
    }

    for (i = 0; i < BOARD_CELLS; i++) b->cell[i] = cells[i];
    b->score = score;
    *won = w ? 1 : 0;
    return 1;
}

void save_store_game(const Board *b, uint8_t won) {
    uint8_t i;
    ENABLE_RAM;
    sram_game.magic = GAME_MAGIC;
    for (i = 0; i < BOARD_CELLS; i++) sram_game.cells[i] = b->cell[i];
    sram_game.score = b->score;
    sram_game.won   = won;
    sram_game.check = game_checksum(b->cell, b->score, won);
    DISABLE_RAM;
}

void save_clear_game(void) {
    ENABLE_RAM;
    sram_game.magic = 0;
    DISABLE_RAM;
}
