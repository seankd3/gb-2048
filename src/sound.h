#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void sound_init(void);
void sound_update(void);          /* once per frame, via frame_next() */

/* A slide that changed the board. Deliberately the quietest sound here: it
   fires on nearly every press, so it has to survive hours of play. */
void sound_move(void);

/* A merge landed. Pitch rises with the resulting exponent, so building a big
   tile sounds bigger. */
void sound_merge(uint8_t exp);

void sound_reject(void);          /* direction pressed, nothing moved */
void sound_start(void);
void sound_win(void);
void sound_gameover(void);

#endif
