#!/usr/bin/env python3
"""Generate GBC tile data for gb-2048.

Emits src/gfx.c and src/gfx.h:
  - a deduplicated 8x8 2bpp tile bank
  - a 4x4 tile-index map per tile value (empty, 2, 4, ... 65536)
  - a 5x7 UI font mapped from ASCII
  - the background palette ramp

Colour indices inside cell art:
  0 = board background (gutter between cells)
  1 = cell fill
  2 = digit ink
  3 = cell edge shadow
"""

import os

CELL_PX = 32          # cell is 4x4 tiles
CELL_TILES = 4
MAX_EXP = 16          # 2^16 = 65536

# ---------------------------------------------------------------- fonts

# 4x7 digits used inside the cells (pitch 5 -> six digits fit in 32px).
CELL_DIGITS = {
    '0': ".##.|#..#|#..#|#..#|#..#|#..#|.##.",
    '1': "..#.|.##.|..#.|..#.|..#.|..#.|.###",
    '2': ".##.|#..#|...#|..#.|.#..|#...|####",
    '3': "####|...#|..#.|..##|...#|#..#|.##.",
    '4': "...#|..##|.#.#|#..#|####|...#|...#",
    '5': "####|#...|###.|...#|...#|#..#|.##.",
    '6': ".##.|#..#|#...|###.|#..#|#..#|.##.",
    '7': "####|...#|...#|..#.|..#.|.#..|.#..",
    '8': ".##.|#..#|#..#|.##.|#..#|#..#|.##.",
    '9': ".##.|#..#|#..#|.###|...#|#..#|.##.",
}

# 5x7 font for the score bar and overlays.
UI_FONT = {
    ' ': ".....|.....|.....|.....|.....|.....|.....",
    'A': ".###.|#...#|#...#|#####|#...#|#...#|#...#",
    'B': "####.|#...#|####.|#...#|#...#|#...#|####.",
    'C': ".###.|#...#|#....|#....|#....|#...#|.###.",
    'D': "####.|#...#|#...#|#...#|#...#|#...#|####.",
    'E': "#####|#....|####.|#....|#....|#....|#####",
    'F': "#####|#....|####.|#....|#....|#....|#....",
    'G': ".###.|#...#|#....|#.###|#...#|#...#|.###.",
    'H': "#...#|#...#|#####|#...#|#...#|#...#|#...#",
    'I': ".###.|..#..|..#..|..#..|..#..|..#..|.###.",
    'J': "....#|....#|....#|....#|#...#|#...#|.###.",
    'K': "#...#|#..#.|#.#..|##...|#.#..|#..#.|#...#",
    'L': "#....|#....|#....|#....|#....|#....|#####",
    'M': "#...#|##.##|#.#.#|#...#|#...#|#...#|#...#",
    'N': "#...#|##..#|#.#.#|#..##|#...#|#...#|#...#",
    'O': ".###.|#...#|#...#|#...#|#...#|#...#|.###.",
    'P': "####.|#...#|#...#|####.|#....|#....|#....",
    'Q': ".###.|#...#|#...#|#...#|#.#.#|#..#.|.##.#",
    'R': "####.|#...#|#...#|####.|#.#..|#..#.|#...#",
    'S': ".####|#....|#....|.###.|....#|....#|####.",
    'T': "#####|..#..|..#..|..#..|..#..|..#..|..#..",
    'U': "#...#|#...#|#...#|#...#|#...#|#...#|.###.",
    'V': "#...#|#...#|#...#|#...#|#...#|.#.#.|..#..",
    'W': "#...#|#...#|#...#|#.#.#|#.#.#|##.##|#...#",
    'X': "#...#|#...#|.#.#.|..#..|.#.#.|#...#|#...#",
    'Y': "#...#|#...#|.#.#.|..#..|..#..|..#..|..#..",
    'Z': "#####|....#|...#.|..#..|.#...|#....|#####",
    '0': ".###.|#...#|#..##|#.#.#|##..#|#...#|.###.",
    '1': "..#..|.##..|..#..|..#..|..#..|..#..|.###.",
    '2': ".###.|#...#|....#|...#.|..#..|.#...|#####",
    '3': "#####|...#.|..##.|....#|....#|#...#|.###.",
    '4': "...#.|..##.|.#.#.|#..#.|#####|...#.|...#.",
    '5': "#####|#....|####.|....#|....#|#...#|.###.",
    '6': ".###.|#...#|#....|####.|#...#|#...#|.###.",
    '7': "#####|....#|...#.|..#..|.#...|.#...|.#...",
    '8': ".###.|#...#|#...#|.###.|#...#|#...#|.###.",
    '9': ".###.|#...#|#...#|.####|....#|#...#|.###.",
    '!': "..#..|..#..|..#..|..#..|..#..|.....|..#..",
    '-': ".....|.....|.....|#####|.....|.....|.....",
}

UI_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-"


def glyph_rows(spec):
    return spec.split('|')


def blit(canvas, gx, gy, spec, colour):
    """Draw a glyph onto a 2-D list canvas at (gx, gy)."""
    for dy, row in enumerate(glyph_rows(spec)):
        for dx, ch in enumerate(row):
            if ch == '#':
                y, x = gy + dy, gx + dx
                if 0 <= y < len(canvas) and 0 <= x < len(canvas[0]):
                    canvas[y][x] = colour


# ---------------------------------------------------------------- cell art

def make_cell(text):
    """Render one 32x32 cell as a colour-index grid. Empty text = blank cell."""
    c = [[0] * CELL_PX for _ in range(CELL_PX)]

    # Rounded rect inset 1px, so neighbouring cells leave a 2px gutter.
    for y in range(1, CELL_PX - 1):
        for x in range(1, CELL_PX - 1):
            # clip the four corners for roundness
            corner = (
                (x <= 2 and y <= 2) or (x >= CELL_PX - 3 and y <= 2) or
                (x <= 2 and y >= CELL_PX - 3) or
                (x >= CELL_PX - 3 and y >= CELL_PX - 3)
            )
            if corner and (abs((x if x <= 2 else CELL_PX - 1 - x) -
                               (y if y <= 2 else CELL_PX - 1 - y)) >= 2):
                continue
            c[y][x] = 1

    # Soft shadow along the bottom and right inner edge.
    for x in range(3, CELL_PX - 3):
        c[CELL_PX - 2][x] = 3
    for y in range(3, CELL_PX - 3):
        c[y][CELL_PX - 2] = 3

    if text:
        pitch = 5
        w = len(text) * pitch - 1
        x0 = (CELL_PX - w) // 2
        y0 = (CELL_PX - 7) // 2
        for i, ch in enumerate(text):
            blit(c, x0 + i * pitch, y0, CELL_DIGITS[ch], 2)
    return c


def make_ui_glyph(ch):
    """Render one UI character into an 8x8 colour-index grid."""
    g = [[0] * 8 for _ in range(8)]
    blit(g, 1, 0, UI_FONT[ch], 2)
    return g


# ---------------------------------------------------------------- tiles

def slice_tiles(grid):
    """Cut a grid whose dimensions are multiples of 8 into 8x8 tiles."""
    out = []
    for ty in range(len(grid) // 8):
        for tx in range(len(grid[0]) // 8):
            tile = tuple(
                tuple(grid[ty * 8 + y][tx * 8 + x] for x in range(8))
                for y in range(8)
            )
            out.append(tile)
    return out


def encode_2bpp(tile):
    """GB tile format: 2 bytes per row, low bitplane first."""
    data = []
    for row in tile:
        lo = hi = 0
        for x, px in enumerate(row):
            bit = 7 - x
            lo |= (px & 1) << bit
            hi |= ((px >> 1) & 1) << bit
        data += [lo, hi]
    return data


class TileBank:
    def __init__(self):
        self.tiles = []
        self.index = {}

    def add(self, tile):
        if tile not in self.index:
            self.index[tile] = len(self.tiles)
            self.tiles.append(tile)
        return self.index[tile]


# ---------------------------------------------------------------- palettes

def rgb555(hexstr):
    v = int(hexstr.lstrip('#'), 16)
    r, g, b = (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF
    return (r >> 3), (g >> 3), (b >> 3)


# [board bg, cell fill, digit ink, edge shadow]
PALETTES = [
    ['#241f1c', '#3a322d', '#8d7f74', '#2e2724'],   # 0: UI + empty cell
    ['#241f1c', '#d5caba', '#3b332c', '#b0a495'],   # 1: 2, 4
    ['#241f1c', '#d9a05c', '#3b2a1c', '#b17e42'],   # 2: 8, 16
    ['#241f1c', '#c4623a', '#2a1712', '#9c4a2a'],   # 3: 32, 64
    ['#241f1c', '#d4b048', '#3b2f14', '#a98a30'],   # 4: 128, 256
    ['#241f1c', '#bf8f22', '#332510', '#966e16'],   # 5: 512, 1024
    ['#241f1c', '#e0562e', '#2b1009', '#b03d1d'],   # 6: 2048, 4096
    ['#241f1c', '#8f6fd0', '#1d1330', '#6d51a6'],   # 7: 8192+
]

# tile value exponent -> palette index
def palette_for(exp):
    if exp == 0:
        return 0
    return min(1 + (exp - 1) // 2, 7)


# ---------------------------------------------------------------- emit

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = os.path.join(root, 'src')
    os.makedirs(src, exist_ok=True)

    bank = TileBank()

    # UI font first so its indices are contiguous and easy to map from ASCII.
    ui_base = 0
    for ch in UI_CHARS:
        bank.add(slice_tiles(make_ui_glyph(ch))[0])

    # One 4x4 tile map per value: index 0 = empty cell, 1..MAX_EXP = 2..65536.
    cell_maps = []
    for exp in range(0, MAX_EXP + 1):
        text = '' if exp == 0 else str(1 << exp)
        tiles = slice_tiles(make_cell(text))
        cell_maps.append([bank.add(t) for t in tiles])

    n = len(bank.tiles)
    assert n <= 256, f"tile bank overflow: {n} > 256"

    with open(os.path.join(src, 'gfx.h'), 'w', newline='\n') as f:
        f.write(f"""/* Generated by tools/gen_assets.py - do not edit. */
#ifndef GFX_H
#define GFX_H

#include <gbdk/platform.h>
#include <stdint.h>

#define GFX_NUM_TILES   {n}
#define GFX_MAX_EXP     {MAX_EXP}
#define GFX_CELL_TILES  {CELL_TILES}
#define GFX_UI_BASE     {ui_base}

extern const uint8_t gfx_tiles[{n} * 16];
extern const uint8_t gfx_cell_map[{MAX_EXP + 1}][{CELL_TILES * CELL_TILES}];
extern const uint8_t gfx_palette_for_exp[{MAX_EXP + 1}];
extern const palette_color_t gfx_bkg_palettes[{len(PALETTES)} * 4];

/* Map an ASCII character to its font tile index. */
uint8_t gfx_char_tile(char c);

#endif
""")

    with open(os.path.join(src, 'gfx.c'), 'w', newline='\n') as f:
        f.write("/* Generated by tools/gen_assets.py - do not edit. */\n")
        f.write('#include "gfx.h"\n\n')

        f.write(f"const uint8_t gfx_tiles[{n} * 16] = {{\n")
        for i, t in enumerate(bank.tiles):
            body = ','.join(f"0x{b:02X}" for b in encode_2bpp(t))
            f.write(f"    {body},\n")
        f.write("};\n\n")

        f.write(f"const uint8_t gfx_cell_map[{MAX_EXP + 1}][{CELL_TILES*CELL_TILES}] = {{\n")
        for exp, m in enumerate(cell_maps):
            f.write("    {" + ','.join(str(v) for v in m) + f"}}, /* {(1<<exp) if exp else 0} */\n")
        f.write("};\n\n")

        f.write(f"const uint8_t gfx_palette_for_exp[{MAX_EXP + 1}] = {{")
        f.write(','.join(str(palette_for(e)) for e in range(MAX_EXP + 1)))
        f.write("};\n\n")

        f.write(f"const palette_color_t gfx_bkg_palettes[{len(PALETTES)} * 4] = {{\n")
        for pi, pal in enumerate(PALETTES):
            cols = []
            for hexstr in pal:
                r, g, b = rgb555(hexstr)
                cols.append(f"RGB({r},{g},{b})")
            f.write(f"    {','.join(cols)}, /* pal {pi} */\n")
        f.write("};\n\n")

        # ASCII -> tile lookup, built as a sparse switch to keep ROM small.
        f.write("uint8_t gfx_char_tile(char c) {\n    switch (c) {\n")
        for i, ch in enumerate(UI_CHARS):
            lit = "' '" if ch == ' ' else f"'{ch}'"
            f.write(f"        case {lit}: return {ui_base + i};\n")
        f.write("        default: return %d;\n    }\n}\n" % ui_base)

    print(f"tiles: {n}/256   cell maps: {len(cell_maps)}   ui glyphs: {len(UI_CHARS)}")


if __name__ == '__main__':
    main()
