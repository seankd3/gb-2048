#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

/* Best score and settings in battery-backed cartridge RAM. A missing or
   corrupt save yields best = 0 and default settings, so a fresh cartridge
   starts clean rather than showing whatever happened to be in RAM. */
void save_load(uint32_t *best);
void save_store(uint32_t best);

#endif
