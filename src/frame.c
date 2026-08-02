#include <gbdk/platform.h>
#include <gb/gb.h>

#include "frame.h"
#include "sound.h"

void frame_next(void) {
    vsync();
    sound_update();
}
