#include <gbdk/platform.h>
#include <gb/gb.h>

#include "save.h"
#include "settings.h"

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
