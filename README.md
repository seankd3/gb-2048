# gb-2048

2048 for the Game Boy Color, built to run on real hardware — a ModRetro
Chromatic with an EverDrive, or any GBC.

The ROM is 32 KB, CGB-only, MBC5 with battery-backed RAM. The best score is
kept in the cartridge battery and survives a power cycle.

## Controls

| Button | Action |
|---|---|
| D-pad | Slide the board |
| B | Undo one move |
| Start | New game, or continue past 2048 |
| Select | Back to the title screen |

## Build

Requires GBDK-2020 (default path `%USERPROFILE%\Tools\gbdk`) and Python 3
with Pillow.

```powershell
.\build.ps1
```

The ROM is written to `build\gb2048.gbc`. Copy that file to the EverDrive SD
card and load it from the cart menu.

`build.ps1` regenerates the tile data first, so edits to
`tools/gen_assets.py` take effect on the next build.

## Screenshots

`tools/shot.py` runs the ROM under `binjgb-tester` with a scripted joypad
file and writes PNGs. Every capture is deterministic: the same script always
produces the same pixels.

```powershell
python tools\shot.py --press 90:start --press 130:left --at 135 --at 150
```

`--press FRAME:BUTTON` queues an input, `--at FRAME` captures that frame.

## Sound

Two pulse channels, driven by a small sequencer in `src/sound.c` that is
ticked once per frame from `frame_next()`. Use `frame_next()` rather than
`vsync()` anywhere you wait, or audio stalls during the slide animation.

Channel 1 carries anything melodic — merges and the win and lose stings.
Channel 2 carries interface blips. Splitting them means a merge and a move
can sound together without either cutting the other off.

Merge pitch rises with the value produced, so a 4 is a low C and 2048 is a
high C, and a cascade is voiced by its biggest tile. Every note comes from a
pentatonic set, chosen because merges overlap and a pentatonic scale has no
interval that can clash.

The move blip is deliberately the quietest thing here: volume 3 on the 12.5%
duty pulse, two frames long. It fires on almost every press, so it has to
survive hours of play rather than sound impressive once.

## Layout

The screen is 20x18 tiles. Rows 0-1 hold the score bar. The board fills rows
2-17 as a 16x16 block with a two-tile margin either side. Each cell is 4x4
tiles, so a cell pitch is 32 px and any interpolated slide position lands on
a whole tile. No sub-tile scrolling is needed.

## Art

`tools/gen_assets.py` draws each tile value into a 32x32 bitmap, cuts it into
8x8 tiles, removes duplicates, and writes `src/gfx.c`. The bank currently
uses 134 of the 256 available tiles.

Colour comes from the CGB palette, not the art. All values share the same
geometry and differ only by which of the eight background palettes they use,
which is why the whole ramp costs so few tiles. Values are paired across
seven palettes: 2 and 4 are bone, 8 and 16 amber, and so on up to violet.

## Things that will bite you

- **SDCC drops a call placed behind an `if` inside a frame-wait loop.** It
  warns about changing conditional flow. The animation timing stayed correct
  while nothing drew, which looked like a maths bug. Keep `buf_flush` out of
  any conditional in that loop.
- **SDCC writes per-file progress to stderr on success.** Under
  PowerShell's `$ErrorActionPreference = 'Stop'` that aborts a good build, so
  the compiler runs through `cmd` and is judged on its exit code alone.
- **The ROM is CGB-only (`0xC0`).** It will not boot on an original DMG. To
  support one, change the header flag and make every screen readable in four
  shades of grey.
- **Check where your instrument samples before believing it.** `NR52`'s low
  bits report which channels are sounding, which is a good way to verify
  audio without hearing it — but sampled from the main loop it reported that
  merges never played. Every merge sound is eight frames long and the slide
  animation is exactly eight frames, so the sound began and ended inside
  `render_slide` and the main loop never saw it. Sampling from `frame_next()`
  showed both channels firing. The audio had been correct all along.
