#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>

#include "board.h"
#include "render.h"
#include "rng.h"
#include "gfx.h"

#define EXP_2048 11   /* 2^11 */

static Board      board;
static MoveResult res;
static uint32_t   best;
static uint8_t    prev_pad;
static uint8_t    won_announced;

static uint8_t poll_pressed(void) {
    uint8_t now = joypad();
    uint8_t pressed = (uint8_t)(now & ~prev_pad);
    prev_pad = now;
    return pressed;
}

/* The title screen doubles as the entropy source: however many frames the
   player leaves it sitting there seeds the run. */
static void title_screen(void) {
    uint16_t ticks = 0;

    render_clear();
    render_cell_at(8, 4, EXP_2048);
    render_text_centered(10, "PRESS START");
    render_score(0, best);

    for (;;) {
        vsync();
        ticks++;
        if (poll_pressed() & (J_START | J_A)) break;
    }
    rng_seed(ticks);
}

static void new_game(void) {
    board_reset(&board);
    board_spawn(&board);
    board_spawn(&board);
    won_announced = 0;

    render_clear();
    render_board(&board);
    render_score(board.score, best);
}

static uint8_t direction_from(uint8_t pressed, Direction *dir) {
    if (pressed & J_LEFT)  { *dir = DIR_LEFT;  return 1; }
    if (pressed & J_RIGHT) { *dir = DIR_RIGHT; return 1; }
    if (pressed & J_UP)    { *dir = DIR_UP;    return 1; }
    if (pressed & J_DOWN)  { *dir = DIR_DOWN;  return 1; }
    return 0;
}

static void wait_for_start(void) {
    for (;;) {
        vsync();
        if (poll_pressed() & J_START) return;
    }
}

static void play(void) {
    uint8_t pressed;
    Direction dir;

    new_game();

    for (;;) {
        vsync();
        pressed = poll_pressed();

        if (pressed & J_SELECT) return;   /* back to title */

        if (direction_from(pressed, &dir)) {
            if (board_slide(&board, dir, &res)) {
                render_slide(&res);
                board_spawn(&board);
                render_board(&board);
                if (board.score > best) best = board.score;
                render_score(board.score, best);

                if (!won_announced && board_max_exp(&board) >= EXP_2048) {
                    won_announced = 1;
                    render_banner("2048", "START TO GO ON");
                    wait_for_start();
                    render_board(&board);
                }

                if (!board_can_move(&board)) {
                    render_banner("GAME OVER", "START FOR NEW");
                    wait_for_start();
                    return;
                }
            }
        }
    }
}

void main(void) {
    cpu_fast();          /* CGB double speed: headroom for full-board redraws */
    render_init();
    best = 0;
    prev_pad = 0;

    for (;;) {
        title_screen();
        play();
    }
}
