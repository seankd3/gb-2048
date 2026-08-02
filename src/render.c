#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

#include "render.h"
#include "frame.h"
#include "settings.h"
#include "gfx.h"

#define SCREEN_W 20
#define SCREEN_H 18

static uint8_t rowbuf[SCREEN_W];
static uint8_t attrbuf[SCREEN_W];

static void put_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      const uint8_t *tiles, const uint8_t *attrs) {
    VBK_REG = 1;
    set_bkg_tiles(x, y, w, h, attrs);
    VBK_REG = 0;
    set_bkg_tiles(x, y, w, h, tiles);
}

void render_init(void) {
    set_bkg_data(0, GFX_NUM_TILES, gfx_tiles);
    set_bkg_palette(0, 8, gfx_bkg_palettes);
    render_clear();
    SHOW_BKG;
    DISPLAY_ON;
}

void render_clear(void) {
    uint8_t y;
    uint8_t blank = gfx_char_tile(' ');
    memset(rowbuf, blank, SCREEN_W);
    memset(attrbuf, 0, SCREEN_W);
    for (y = 0; y < SCREEN_H; y++) {
        put_tiles(0, y, SCREEN_W, 1, rowbuf, attrbuf);
    }
}

void render_cell_at(uint8_t tx, uint8_t ty, uint8_t exp) {
    uint8_t attrs[GFX_CELL_TILES * GFX_CELL_TILES];
    memset(attrs, gfx_palette_for_exp[exp], sizeof(attrs));
    put_tiles(tx, ty, GFX_CELL_TILES, GFX_CELL_TILES, gfx_cell_map[exp], attrs);
}

void render_cell(uint8_t index, uint8_t exp) {
    render_cell_at((uint8_t)(BOARD_ORIGIN_X + (index & 3) * GFX_CELL_TILES),
                   (uint8_t)(BOARD_ORIGIN_Y + (index >> 2) * GFX_CELL_TILES),
                   exp);
}

void render_board(const Board *b) {
    uint8_t i;
    for (i = 0; i < BOARD_CELLS; i++) {
        render_cell(i, b->cell[i]);
    }
}

/* ---- slide animation -------------------------------------------------
   The board is composed into a WRAM shadow buffer and pushed to VRAM in two
   blits per step. Cells sit 4 tiles apart, so an interpolated position is
   always a whole number of tiles and no sub-tile scrolling is needed. */

#define BOARD_TILES 16

/* Speed controls both how many positions a tile is drawn at and how long each
   is held. Dwell alone is not enough: a step costs about a frame just to push
   the buffer to VRAM, so holding for one frame is no faster than two. Fast
   therefore halves the number of steps instead. Steps must divide the 4-tile
   cell pitch evenly so every interpolated position stays on a whole tile.
   Measured press-to-settled: slow ~366ms, normal ~266ms, fast ~133ms. */
static const uint8_t steps_for_speed[SPEED_COUNT] = { 4, 4, 2, 0 };
static const uint8_t hold_for_speed[SPEED_COUNT]  = { 3, 1, 1, 0 };

static uint8_t map_buf[BOARD_TILES * BOARD_TILES];
static uint8_t att_buf[BOARD_TILES * BOARD_TILES];

static void buf_put_cell(int tx, int ty, uint8_t exp);

/* Resets to the empty 4x4 grid rather than flat background, so the slots stay
   visible underneath the tiles sliding over them. */
static void buf_clear(void) {
    uint8_t i;
    memset(map_buf, gfx_char_tile(' '), sizeof(map_buf));
    memset(att_buf, 0, sizeof(att_buf));
    for (i = 0; i < BOARD_CELLS; i++) {
        buf_put_cell((int)(i & 3) * GFX_CELL_TILES,
                     (int)(i >> 2) * GFX_CELL_TILES, 0);
    }
}

static void buf_put_cell(int tx, int ty, uint8_t exp) {
    const uint8_t *m = gfx_cell_map[exp];
    uint8_t pal = gfx_palette_for_exp[exp];
    int r, c, x, y;

    for (r = 0; r < GFX_CELL_TILES; r++) {
        y = ty + r;
        if (y < 0 || y >= BOARD_TILES) continue;
        for (c = 0; c < GFX_CELL_TILES; c++) {
            x = tx + c;
            if (x < 0 || x >= BOARD_TILES) continue;
            map_buf[y * BOARD_TILES + x] = m[r * GFX_CELL_TILES + c];
            att_buf[y * BOARD_TILES + x] = pal;
        }
    }
}

/* Written a row at a time to keep each VRAM burst small. */
static void buf_flush(void) {
    uint8_t r;
    for (r = 0; r < BOARD_TILES; r++) {
        VBK_REG = 1;
        set_bkg_tiles(BOARD_ORIGIN_X, (uint8_t)(BOARD_ORIGIN_Y + r),
                      BOARD_TILES, 1, att_buf + r * BOARD_TILES);
        VBK_REG = 0;
        set_bkg_tiles(BOARD_ORIGIN_X, (uint8_t)(BOARD_ORIGIN_Y + r),
                      BOARD_TILES, 1, map_buf + r * BOARD_TILES);
    }
}

void render_slide(const MoveResult *res) {
    uint8_t i, h;
    int step, fx, fy, tx, ty;
    uint8_t sp = (settings.speed < SPEED_COUNT) ? settings.speed : SPEED_NORMAL;
    uint8_t hold  = hold_for_speed[sp];
    int     steps = steps_for_speed[sp];

    if (steps == 0) return;   /* instant: caller draws the settled board */

    for (step = 1; step <= steps; step++) {
        buf_clear();
        for (i = 0; i < res->count; i++) {
            fx = (int)(res->moves[i].from & 3) * GFX_CELL_TILES;
            fy = (int)(res->moves[i].from >> 2) * GFX_CELL_TILES;
            tx = (int)(res->moves[i].to & 3) * GFX_CELL_TILES;
            ty = (int)(res->moves[i].to >> 2) * GFX_CELL_TILES;
            /* Cells are 4 tiles apart, so these divisions are exact and the
               interpolated position is always a whole tile. */
            buf_put_cell(fx + (tx - fx) * step / steps,
                         fy + (ty - fy) * step / steps,
                         res->moves[i].exp);
        }
        /* Keep buf_flush out of any conditional in this loop. SDCC's optimizer
           drops the call when it sits behind an `if` here (it warns with
           "conditional flow changed by optimizer"), which silently disables
           the whole animation. */
        frame_next();
        buf_flush();
        for (h = 1; h < hold; h++) frame_next();
    }
}

void render_text(uint8_t x, uint8_t y, const char *s) {
    uint8_t n = 0;
    while (s[n] && (x + n) < SCREEN_W) {
        rowbuf[n] = gfx_char_tile(s[n]);
        attrbuf[n] = 0;
        n++;
    }
    if (n) put_tiles(x, y, n, 1, rowbuf, attrbuf);
}

void render_text_centered(uint8_t y, const char *s) {
    uint8_t len = (uint8_t)strlen(s);
    uint8_t x = (len >= SCREEN_W) ? 0 : (uint8_t)((SCREEN_W - len) / 2);
    render_text(x, y, s);
}

/* Clears the middle two rows of cells outright and writes two lines into the
   gap. The band is snapped to whole cells (screen rows 6-13) so the overlay
   never slices a tile in half. */
void render_banner(const char *line1, const char *line2) {
    uint8_t y;
    uint8_t blank = gfx_char_tile(' ');
    memset(rowbuf, blank, SCREEN_W);
    memset(attrbuf, 0, SCREEN_W);
    for (y = 6; y <= 13; y++) {
        put_tiles(0, y, SCREEN_W, 1, rowbuf, attrbuf);
    }
    render_text_centered(8, line1);
    if (line2) render_text_centered(11, line2);
}

static void u32_to_str(uint32_t v, char *out) {
    char tmp[11];
    uint8_t n = 0, i;
    if (v == 0) { out[0] = '0'; out[1] = 0; return; }
    while (v) {
        tmp[n++] = (char)('0' + (uint8_t)(v % 10));
        v /= 10;
    }
    for (i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    out[n] = 0;
}

/* Values are right-padded with spaces so shrinking numbers cannot leave
   stale digits behind. */
static void draw_value(uint8_t x, uint8_t y, uint32_t v, uint8_t width) {
    char buf[12];
    uint8_t n, i;
    u32_to_str(v, buf);
    n = (uint8_t)strlen(buf);
    for (i = n; i < width && i < sizeof(buf) - 1; i++) buf[i] = ' ';
    if (width < sizeof(buf)) buf[width] = 0;
    render_text(x, y, buf);
}

void render_score(uint32_t score, uint32_t best) {
    render_text(0, 0, "SCORE");
    render_text(12, 0, "BEST");
    draw_value(0, 1, score, 7);
    draw_value(12, 1, best, 7);
}

