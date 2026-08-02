#include "rng.h"

/* 16-bit xorshift. Cheap, and long-period enough that a player will never
   see a repeat within a run. Seeded from how long the title screen sat idle. */
static uint16_t state = 1;

void rng_seed(uint16_t s) {
    state = s ? s : 1;
}

uint16_t rng_next(void) {
    state ^= (uint16_t)(state << 7);
    state ^= (uint16_t)(state >> 9);
    state ^= (uint16_t)(state << 8);
    return state;
}
