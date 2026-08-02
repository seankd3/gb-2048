#ifndef FRAME_H
#define FRAME_H

/* Waits for the next frame and runs everything that must tick once per frame.
   Use this instead of vsync() everywhere, or sounds stall during animations. */
void frame_next(void);

#endif
