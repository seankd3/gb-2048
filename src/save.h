#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

/* Best score in battery-backed cartridge RAM. Returns 0 when the save is
   absent or corrupt, so a fresh cartridge starts clean rather than showing
   whatever happened to be in RAM. */
uint32_t save_load_best(void);
void     save_store_best(uint32_t best);

#endif
