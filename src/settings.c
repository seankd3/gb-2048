#include "settings.h"

Settings settings;

/* Fast by default: it lands at roughly the 130ms the browser original uses,
   which is the pace the game was designed around. Normal is smoother but
   reads as sluggish once you are playing at speed. */
void settings_defaults(void) {
    settings.speed = SPEED_FAST;
    settings.sound = 1;
}

const char *settings_speed_name(uint8_t speed) {
    switch (speed) {
        case SPEED_SLOW:    return "SLOW";
        case SPEED_FAST:    return "FAST";
        case SPEED_INSTANT: return "INSTANT";
        default:            return "NORMAL";
    }
}
