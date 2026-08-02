#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>

#include "board.h"
#include "render.h"
#include "rng.h"
#include "save.h"
#include "sound.h"
#include "frame.h"
#include "input.h"
#include "menu.h"
#include "settings.h"
#include "gfx.h"

#define EXP_2048 11   /* 2^11 */

static Board      board;
static Board      undo_board;   /* snapshot taken before each accepted move */
static MoveResult res;
static uint32_t   best;
static uint8_t    won_announced;
static uint8_t    can_undo;

/* The title screen doubles as the entropy source: however many frames the
   player leaves it sitting there seeds the run. */
static void title_screen(void) {
    uint16_t ticks = 0;
    uint8_t  shown = 1;

    render_clear();
    render_cell_at(8, 4, EXP_2048);
    render_text_centered(10, "PRESS START");

    /* Only the best score belongs here. The old screen showed SCORE 0, which
       is just a zero taking up room. */
    if (best) render_best_line(14, best);

    for (;;) {
        frame_next();
        ticks++;

        /* Slow blink, the classic attract-screen tell that it wants a press. */
        if ((ticks & 31) == 0) {
            shown = shown ? 0 : 1;
            render_text_centered(10, shown ? "PRESS START" : "           ");
        }

        if (input_pressed() & (J_START | J_A)) break;
    }
    rng_seed(ticks);
    sound_start();
}

static void new_game(void) {
    board_reset(&board);
    board_spawn(&board);
    board_spawn(&board);
    won_announced = 0;
    can_undo = 0;

    render_clear();
    render_board(&board);
    render_score(board.score, best);
    render_gain(0);
}

/* Best is deliberately not rolled back by undo: it records the highest score
   actually reached, and taking a move back should not erase that. */
static void undo_move(void) {
    if (!can_undo) return;
    board = undo_board;
    can_undo = 0;
    render_board(&board);
    render_score(board.score, best);
    render_gain(0);
}

/* Highest value produced by this move, or 0 if nothing merged. Drives the
   merge pitch, so a cascade is voiced by its biggest tile. */
static uint8_t top_merge_exp(const MoveResult *r) {
    uint8_t i, m = 0, e;
    for (i = 0; i < r->count; i++) {
        if (r->moves[i].merged) {
            e = (uint8_t)(r->moves[i].exp + 1);
            if (e > m) m = e;
        }
    }
    return m;
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
        frame_next();
        if (input_pressed() & J_START) return;
    }
}

static void play(void) {
    uint8_t pressed;
    Direction dir;

    new_game();

    for (;;) {
        frame_next();

        pressed = input_pressed();

        if (pressed & J_SELECT) {
            if (menu_open(best) == MENU_NEW_GAME) {
                new_game();
                continue;
            }
            render_clear();
            render_board(&board);
            render_score(board.score, best);
            continue;
        }
        if (pressed & J_B) undo_move();

        if (direction_from(pressed, &dir)) {
            undo_board = board;
            if (board_slide(&board, dir, &res)) {
                uint8_t merged = top_merge_exp(&res);
                uint8_t spawn;
                can_undo = 1;
                if (merged) sound_merge(merged); else sound_move();
                render_slide(&res);

                /* Draw the settled board with the new tile's slot still empty,
                   flash the merges, then let the new tile arrive. Spawning it
                   in the same frame as everything else made it look like it
                   had been there all along. */
                spawn = board_spawn(&board);
                render_board(&board);
                render_cell(spawn, 0);
                render_merge_pop(&board, &res);
                render_cell(spawn, board.cell[spawn]);
                render_gain(res.gained);

                if (board.score > best) {
                    best = board.score;
                    save_store(best);
                }
                render_score(board.score, best);

                if (!won_announced && board_max_exp(&board) >= EXP_2048) {
                    won_announced = 1;
                    sound_win();
                    render_banner("2048", "START TO GO ON");
                    wait_for_start();
                    render_board(&board);
                }

                if (!board_can_move(&board)) {
                    sound_gameover();
                    render_banner("GAME OVER", "START FOR NEW");
                    wait_for_start();
                    return;
                }
            } else {
                sound_reject();   /* pressed a direction that changes nothing */
            }
        }
    }
}

void main(void) {
    cpu_fast();          /* CGB double speed: headroom for full-board redraws */
    render_init();
    sound_init();
    save_load(&best);
    input_reset();

    for (;;) {
        title_screen();
        play();
    }
}

