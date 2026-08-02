#include "board.h"
#include "rng.h"

/* Index of the k-th cell along a line, walking from the packing edge inward.
   Every direction reduces to "pack toward k=0", so the slide below is written
   once and reused four ways. */
static uint8_t line_index(Direction dir, uint8_t line, uint8_t k) {
    switch (dir) {
        case DIR_LEFT:  return (uint8_t)(line * BOARD_SIDE + k);
        case DIR_RIGHT: return (uint8_t)(line * BOARD_SIDE + (BOARD_SIDE - 1 - k));
        case DIR_UP:    return (uint8_t)(k * BOARD_SIDE + line);
        default:        return (uint8_t)((BOARD_SIDE - 1 - k) * BOARD_SIDE + line);
    }
}

void board_reset(Board *b) {
    uint8_t i;
    for (i = 0; i < BOARD_CELLS; i++) b->cell[i] = 0;
    b->score = 0;
}

uint8_t board_slide(Board *b, Direction dir, MoveResult *res) {
    uint8_t line, k, write, src, dst, v;
    uint8_t out[BOARD_SIDE];
    uint8_t fused[BOARD_SIDE];

    res->count = 0;
    res->moved = 0;
    res->gained = 0;

    for (line = 0; line < BOARD_SIDE; line++) {
        for (k = 0; k < BOARD_SIDE; k++) { out[k] = 0; fused[k] = 0; }
        write = 0;

        for (k = 0; k < BOARD_SIDE; k++) {
            src = line_index(dir, line, k);
            v = b->cell[src];
            if (v == 0) continue;

            if (write > 0 && out[write - 1] == v && !fused[write - 1]) {
                /* Merge into the tile already parked at write-1. */
                out[write - 1] = (uint8_t)(v + 1);
                fused[write - 1] = 1;
                res->gained += (uint32_t)1 << (v + 1);
                dst = line_index(dir, line, (uint8_t)(write - 1));
                res->moves[res->count].from   = src;
                res->moves[res->count].to     = dst;
                res->moves[res->count].exp    = v;
                res->moves[res->count].merged = 1;
                res->count++;
                res->moved = 1;
            } else {
                out[write] = v;
                fused[write] = 0;
                dst = line_index(dir, line, write);
                res->moves[res->count].from   = src;
                res->moves[res->count].to     = dst;
                res->moves[res->count].exp    = v;
                res->moves[res->count].merged = 0;
                res->count++;
                if (dst != src) res->moved = 1;
                write++;
            }
        }

        for (k = 0; k < BOARD_SIDE; k++) {
            b->cell[line_index(dir, line, k)] = out[k];
        }
    }

    b->score += res->gained;
    return res->moved;
}

uint8_t board_spawn(Board *b) {
    uint8_t i, n = 0, pick;
    uint8_t empty[BOARD_CELLS];

    for (i = 0; i < BOARD_CELLS; i++) {
        if (b->cell[i] == 0) empty[n++] = i;
    }
    if (n == 0) return 0xFF;

    pick = empty[(uint8_t)(rng_next() % n)];
    /* Classic 2048 odds: nine 2s for every 4. */
    b->cell[pick] = ((rng_next() % 10) == 0) ? 2 : 1;
    return pick;
}

uint8_t board_can_move(const Board *b) {
    uint8_t r, c, i;

    for (i = 0; i < BOARD_CELLS; i++) {
        if (b->cell[i] == 0) return 1;
    }
    for (r = 0; r < BOARD_SIDE; r++) {
        for (c = 0; c < BOARD_SIDE; c++) {
            i = (uint8_t)(r * BOARD_SIDE + c);
            if (c + 1 < BOARD_SIDE && b->cell[i] == b->cell[i + 1]) return 1;
            if (r + 1 < BOARD_SIDE && b->cell[i] == b->cell[i + BOARD_SIDE]) return 1;
        }
    }
    return 0;
}

uint8_t board_max_exp(const Board *b) {
    uint8_t i, m = 0;
    for (i = 0; i < BOARD_CELLS; i++) {
        if (b->cell[i] > m) m = b->cell[i];
    }
    return m;
}
