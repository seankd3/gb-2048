#include <gbdk/platform.h>
#include <gb/gb.h>

#include "input.h"

static uint8_t prev;

uint8_t input_pressed(void) {
    uint8_t now = joypad();
    uint8_t pressed = (uint8_t)(now & ~prev);
    prev = now;
    return pressed;
}

void input_reset(void) {
    prev = joypad();
}
