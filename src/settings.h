#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#define SPEED_SLOW    0
#define SPEED_NORMAL  1
#define SPEED_FAST    2
#define SPEED_INSTANT 3
#define SPEED_COUNT   4

typedef struct {
    uint8_t speed;
    uint8_t sound;   /* 0 = off, 1 = on */
} Settings;

extern Settings settings;

void settings_defaults(void);

/* Label for the current value, for the menu to draw. */
const char *settings_speed_name(uint8_t speed);

#endif
