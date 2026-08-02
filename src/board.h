#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define BOARD_SIDE   4
#define BOARD_CELLS  16

/* Cells hold an exponent, not a value: 0 = empty, 1 = "2", 2 = "4", ... */
typedef struct {
    uint8_t  cell[BOARD_CELLS];
    uint32_t score;
} Board;

typedef enum {
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} Direction;

/* One tile's journey during a slide. Kept so the renderer can animate it. */
typedef struct {
    uint8_t from;    /* source cell index */
    uint8_t to;      /* destination cell index */
    uint8_t exp;     /* exponent while travelling (before any merge) */
    uint8_t merged;  /* 1 if this tile lands on top of an equal one */
} TileMove;

typedef struct {
    TileMove moves[BOARD_CELLS];
    uint8_t  count;
    uint8_t  moved;    /* did anything actually change? */
    uint32_t gained;   /* score added by this move */
} MoveResult;

void    board_reset(Board *b);
uint8_t board_slide(Board *b, Direction dir, MoveResult *res);
uint8_t board_spawn(Board *b);          /* returns the cell index used */
uint8_t board_can_move(const Board *b);
uint8_t board_max_exp(const Board *b);

#endif
