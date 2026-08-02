#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>

#include "render.h"
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

/* Clear a four-row band across the middle of the board and write two lines
   into it, so overlays stay legible over a busy grid. */
void render_banner(const char *line1, const char *line2) {
    uint8_t y;
    uint8_t blank = gfx_char_tile(' ');
    memset(rowbuf, blank, SCREEN_W);
    memset(attrbuf, 0, SCREEN_W);
    for (y = 7; y <= 11; y++) {
        put_tiles(0, y, SCREEN_W, 1, rowbuf, attrbuf);
    }
    render_text_centered(8, line1);
    if (line2) render_text_centered(10, line2);
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
