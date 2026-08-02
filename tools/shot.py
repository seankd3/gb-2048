#!/usr/bin/env python3
"""Capture screenshots from the ROM with scripted controller input.

binjgb-tester replays a joypad file and writes a PPM at frame N, so a run is
fully deterministic: same script in, same pixels out.

  python tools/shot.py --at 90
  python tools/shot.py --press 90:start --press 130:right --at 100 --at 160

Joypad file format (from binjgb src/joypad.h) is a flat array of:
    u64 ticks; u8 buttons; u8 padding[7];
with buttons packed as
    down<<7 up<<6 left<<5 right<<4 start<<3 select<<2 B<<1 A<<0
"""

import argparse
import os
import struct
import subprocess
import sys

from PIL import Image

TICKS_PER_FRAME = 70224   # binjgb reports 70224 ticks/frame

BUTTON_BITS = {
    'a': 0, 'b': 1, 'select': 2, 'start': 3,
    'right': 4, 'left': 5, 'up': 6, 'down': 7,
}

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_TESTER = os.path.expandvars(
    r'%USERPROFILE%\Tools\binjgb\binjgb-v0.1.11\bin\binjgb-tester.exe')


def build_joypad(presses, hold_frames):
    """presses: list of (frame, button_name). Returns joypad file bytes.

    A press at frame f is held over [f, f + hold_frames), and overlapping
    presses combine rather than cancelling each other. Only mask changes are
    written, since playback holds the last state until the next entry.
    """
    if not presses:
        return struct.pack('<QB7x', 0, 0)

    last = max(f for f, _ in presses) + hold_frames + 1
    blob = b''
    prev = None
    for frame in range(last + 1):
        mask = 0
        for pf, name in presses:
            if pf <= frame < pf + hold_frames:
                mask |= 1 << BUTTON_BITS[name]
        if mask != prev:
            blob += struct.pack('<QB7x', frame * TICKS_PER_FRAME, mask)
            prev = mask
    return blob


def capture(tester, rom, frame, joypad_path, out_png, scale):
    ppm = out_png + '.ppm'
    cmd = [tester, '-f', str(frame), '-o', ppm]
    if joypad_path:
        cmd += ['-j', joypad_path]
    cmd.append(rom)

    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0 or not os.path.exists(ppm):
        sys.stderr.write(proc.stdout + proc.stderr)
        raise SystemExit(f'binjgb-tester failed at frame {frame}')

    im = Image.open(ppm)
    if scale != 1:
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    im.save(out_png)
    os.remove(ppm)
    return out_png


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--rom', default=os.path.join(ROOT, 'build', 'gb2048.gbc'))
    ap.add_argument('--tester', default=DEFAULT_TESTER)
    ap.add_argument('--outdir', default=os.path.join(ROOT, 'build', 'shots'))
    ap.add_argument('--at', type=int, action='append', default=[],
                    help='capture a frame (repeatable)')
    ap.add_argument('--press', action='append', default=[],
                    help='FRAME:BUTTON, e.g. 90:start (repeatable)')
    ap.add_argument('--hold', type=int, default=4, help='frames to hold a press')
    ap.add_argument('--scale', type=int, default=3)
    args = ap.parse_args()

    presses = []
    for spec in args.press:
        frame, name = spec.split(':')
        name = name.strip().lower()
        if name not in BUTTON_BITS:
            raise SystemExit(f'unknown button: {name}')
        presses.append((int(frame), name))

    frames = sorted(set(args.at)) or [90]
    os.makedirs(args.outdir, exist_ok=True)

    joypad_path = None
    if presses:
        joypad_path = os.path.join(args.outdir, 'input.joypad')
        with open(joypad_path, 'wb') as f:
            f.write(build_joypad(presses, args.hold))

    for frame in frames:
        out = os.path.join(args.outdir, f'f{frame:05d}.png')
        capture(args.tester, args.rom, frame, joypad_path, out, args.scale)
        print(out)


if __name__ == '__main__':
    main()
