#!/usr/bin/env python3
"""Hyper Street Fighter Alpha's S-ISM super gauge (Street Fighter Alpha Anthology, PS2), for the
anthology extension's S-ISMs (src/extensions/anthology_select.c, the S-ISM gauge).

Hyper draws an S-ISM's gauge as the arcade bosses' "Shadaloo" gauge: the Shadaloo emblem on a swoosh,
one long bar, and CHARGE OK and SUPER over it when it is full, in the S-ISM's colours (red for S-ISM A,
blue for B, green for C; D, Champion Edition, has none). This tool takes those pictures from the user's
own Anthology disc and writes them as one PNG atlas: nothing from the disc is in the repo.

    python tools/anthology_hud.py [--iso PATH] [--out assets/us/anthology/hud]

What it reads on the Anthology disc (SLUS-21317; X_DATA.BIN entries by index, docs/HYPER_ALPHA.md):
  546  Hyper's HUD tiles (halfword LZ): 0x5B00 tiles of 16 x 16 at 4 bits, 128 bytes each, left pixel in
       the low nibble, pen 15 transparent (the game's object bank 1).
  544  zero9_usa.bin (halfword LZ; loads at 0x1F0000): the HUD's palettes, 16 colours of 0RGB (4 bits
       each), 32 bytes a row; the S-ISM gauge's (object palette 27) while it fills red 0x690E00, blue
       0x690EE0, green 0x690F40; full (CHARGE OK and SUPER; the row Hyper shows when it is full) red
       0x690E80, blue 0x6914E0, green 0x6915A0.
How Hyper lays the gauge out (player 1's; read from save states of the running game: its object list):
  0x39D0, 5 wide   the swoosh and the Shadaloo emblem, at the gauge's left (x 0, y 0)
  0x3671           the bar's ends: at x 16 (flipped across) and x 176
  0x3660, x 9      the bar, x 32 - 175 (144 pixels: the gauge's 0 - 144); pen 13 its fill
  0x365E, 2 wide   CHARGE OK, at x 0, when full
  0x3659, 5 wide   SUPER, at x 65, y -1, when full

Output: sism_gauge.png (256 x 256, 8-bit RGBA) and sism_gauge.json. For each of red, green, blue a band
of 72 rows: rows 0 - 23 the gauge empty (swoosh, emblem, the bar's ends, the bar with its fill in the
outline's colour), 24 - 47 the bar's fill alone, 48 - 71 CHARGE OK and SUPER. In each, the gauge's
(0, 0) is at column 0, row 4 of its 24.
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import anthology as an  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HUD_TILES, ZERO9, ZERO9_LOAD = 546, 544, 0x1F0000
# while it fills, then the five rows Hyper cycles through when it is full (its bar and SUPER pulsing)
PALETTES = {'red': (0x690E00, 0x690E00), 'green': (0x690F40, 0x691540), 'blue': (0x690EE0, 0x6914A0)}
STEPS = 5
TRANSPARENT, FILL, OUTLINE = 15, 13, 4
W, H, BAND, LAYER, TOP, COL2 = 512, 512, 168, 24, 4, 256
EMBLEM, END, BAR, CHARGE, SUPER = 0x39D0, 0x3671, 0x3660, 0x365E, 0x3659
# (layer, tile, width, x, y, flipped)
PIECES = [
    (0, EMBLEM, 5, 0, 0, False), (0, END, 1, 16, 0, True), (0, END, 1, 176, 0, False),
    *[(0, BAR, 1, 32 + 16 * i, 0, False) for i in range(9)],
    *[(1, BAR, 1, 32 + 16 * i, 0, False) for i in range(9)],
    (2, CHARGE, 2, 0, 0, False), (2, SUPER, 5, 65, -1, False),
    *[(3, BAR, 1, 32 + 16 * i, 0, False) for i in range(9)],
]


def png_rgba(path, w, h, rows):
    import zlib
    raw = b''.join(b'\0' + bytes(r) for r in rows)
    chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--iso', help='your Street Fighter Alpha Anthology (US) disc image (default: the .iso in imports/ps2/)')
    ap.add_argument('--out', default=os.path.join(ROOT, 'assets', 'us', 'anthology', 'hud'))
    a = ap.parse_args()
    iso = an.Iso(a.iso or an.find_iso(None))
    if 'X_DATA.BIN' not in iso.files:
        sys.exit("not Street Fighter Alpha Anthology (US, SLUS-21317)")
    base, entries = an.afs_entries(iso, 'X_DATA.BIN')
    entry = lambda i: iso.read(base + entries[i][0], entries[i][1])
    zero9 = an.unlz16(entry(ZERO9))
    if an.mwo3_header(zero9)['name'] != 'zero9_usa.bin':
        sys.exit(f"X_DATA[{ZERO9}] is not Hyper Street Fighter Alpha (zero9_usa.bin): a different disc?")
    tiles = an.unlz16(entry(HUD_TILES))
    if len(tiles) != 0x5B00 * 128:
        sys.exit(f"X_DATA[{HUD_TILES}] is not Hyper's HUD tiles: a different disc?")

    def pen(t, x, y):
        b = tiles[t * 128 + y * 8 + x // 2]
        return b & 15 if x % 2 == 0 else b >> 4

    rows = [bytearray(W * 4) for _ in range(H)]

    def put(t, x0, y0, flip, rgb, dark):
        for y in range(16):
            for x in range(16):
                p = pen(t, 15 - x if flip else x, y)
                if p == TRANSPARENT:
                    continue
                if dark and p == FILL:
                    p = OUTLINE
                rows[y0 + y][4 * (x0 + x): 4 * (x0 + x) + 4] = bytes((*rgb[p], 255))

    def palette(at):
        return [((c >> 8 & 15) * 17, (c >> 4 & 15) * 17, (c & 15) * 17)
                for c in struct.unpack_from('<16H', zero9, at - ZERO9_LOAD)]

    # each colour's band: column 0 layers 0 the gauge empty, 1 its fill, 2 - 6 CHARGE OK and SUPER in
    # the full cycle's rows 0 - 4; column 2 (x 256) layers 0 - 4 the fill in those rows
    for k, (name, (filling, full)) in enumerate(PALETTES.items()):
        y_band = k * BAND + TOP
        for layer, t0, wd, x0, y0, flip in PIECES:
            for c in range(wd):
                t, x = t0 + c, x0 + 16 * c
                if layer in (0, 1):
                    put(t, x, y_band + layer * LAYER + y0, flip, palette(filling), layer == 0 and t == BAR)
                elif layer == 2:
                    for step in range(STEPS):
                        put(t, x, y_band + (2 + step) * LAYER + y0, flip, palette(full + 0x20 * step), False)
                else:
                    for step in range(STEPS):
                        put(t, COL2 + x, y_band + step * LAYER + y0, flip, palette(full + 0x20 * step), False)
    os.makedirs(a.out, exist_ok=True)
    png_rgba(os.path.join(a.out, 'sism_gauge.png'), W, H, rows)
    with open(os.path.join(a.out, 'sism_gauge.json'), 'w') as f:
        json.dump({'source': 'Street Fighter Alpha Anthology (SLUS-21317): X_DATA 546 tiles, zero9 palettes',
                   'bands': list(PALETTES), 'band_rows': BAND, 'layer_rows': LAYER, 'origin_row': TOP,
                   'layers': ['empty', 'fill', 'full 0-4'], 'column2': 'fill 0-4', 'bar_x': 32, 'bar_w': 144}, f, indent=1)
    print(f"wrote {a.out}/sism_gauge.png ({W} x {H})")


if __name__ == '__main__':
    main()
