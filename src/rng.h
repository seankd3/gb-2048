#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void     rng_seed(uint16_t s);
uint16_t rng_next(void);

#endif
