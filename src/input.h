#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* Edge-triggered buttons: the bits that went down since the last call. Call
   once per frame. Menu and game share this so neither can steal the other's
   press or see a stale edge. */
uint8_t input_pressed(void);

/* Treat everything currently held as already handled. Call when changing
   mode, so the button that opened a screen does not immediately act inside
   it. */
void input_reset(void);

#endif
