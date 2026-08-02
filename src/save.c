#include <gbdk/platform.h>
#include <gb/gb.h>

#include "save.h"

#define SAVE_MAGIC 0x3248u   /* '2H' */

typedef struct {
    uint16_t magic;
    uint32_t best;
    uint8_t  check;
} SaveBlock;

static SaveBlock __at(0xA000) sram;

/* Magic alone is not enough: a half-written or randomly-matching cell would
   pass. The checksum makes a bogus best score vanishingly unlikely. */
static uint8_t checksum(uint32_t best) {
    return (uint8_t)((uint8_t)(best) ^ (uint8_t)(best >> 8) ^
                     (uint8_t)(best >> 16) ^ (uint8_t)(best >> 24) ^ 0xA5u);
}

uint32_t save_load_best(void) {
    uint32_t best = 0;

    ENABLE_RAM;
    if (sram.magic == SAVE_MAGIC && sram.check == checksum(sram.best)) {
        best = sram.best;
    }
    DISABLE_RAM;
    return best;
}

void save_store_best(uint32_t best) {
    ENABLE_RAM;
    sram.magic = SAVE_MAGIC;
    sram.best  = best;
    sram.check = checksum(best);
    DISABLE_RAM;
}
