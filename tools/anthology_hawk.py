#!/usr/bin/env python3
"""T. Hawk's walk back from Hyper Street Fighter Alpha (Street Fighter Alpha Anthology, PS2), for the
game's T.HAWK WALKBACK option (EX. SETTING page 5/5; src/extensions/anthology.c).

The PSP never had T. Hawk's walk-back art: its walk back (06_data animation set 1, number 3: pictures
51 down to 40) plays twelve copies of the walk forward's pictures (28 - 39) in reverse. Hyper Street
Fighter Alpha has the twelve pictures of its own. This tool takes them from the user's own Anthology
disc and writes them as the PSP's pictures 40 - 51 would be: nothing from either disc is in the repo.

    python tools/anthology_hawk.py [--iso PATH] [--out assets/us/anthology/hawk_walkback] [--psp-pac PL23]

What it reads on the Anthology disc (SLUS-21317; X_DATA.BIN entries by index, docs/HYPER_ALPHA.md):
  544  zero9_usa.bin, Hyper's program and data (MWo3, halfword LZ; loads at 0x1F0000). The animations
       by character version at 0x6A7A94 (T. Hawk's Alpha 3 version is 0x23): 12 pointers to slices of
       u16 offsets; animation 3 of the first slice is the walk back. An animation is 20-byte frames
       (u16 duration, u16, u32 sprite record, 12 bytes) up to one with 0x8000 / 0x4000 in its duration.
       A sprite record: u16 2, u16, u16 n, padding to 4, u32 its position list, then n + 1 parts of
       4 bytes: u16 tile, u8 flags (0x20 flipped across, 0x40 flipped down), u8 size (low nibble
       columns - 1, high nibble rows - 1, in 16-pixel tiles; tile + column + 16 x row, as CPS2 blocks).
  547  the position lists (halfword LZ; loaded at 0xC84000): s16 x, s16 y for each part, from the
       sprite's origin.
  386  T. Hawk's tiles: +0x14 u32 count, +0x30 u32 offsets (bit 31: packed), 16 x 16 tiles of 4-bit
       pixels (left pixel in the low nibble, 8 bytes a row; pen 15 transparent). Packed tiles: a flags
       byte for each eight tokens, the highest bit first; 0 a literal byte; 1 a byte hl: h = 0 repeats
       the last byte l + 2 times, otherwise copies l + 2 bytes from h + 1 back.
The pen p is the PSP's colour p + 1 in palette 0 (checked: Hyper's walk forward rebuilt this way is the
PSP's pictures 28 - 39 pixel for pixel; --psp-pac checks it again).

Output: pictures/0040.png - 0051.png (160 x 160, 8-bit indexed, index = colour, 0 transparent; the
sprite's origin at 80, 141, as the PSP's pictures 40 - 51) and walkback.json.
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import anthology as an  # noqa: E402
import pac  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ZERO9, ZERO9_LOAD = 544, 0x1F0000
POSITIONS, POSITIONS_LOAD = 547, 0xC84000
HAWK_TILES = 386
ANIMATIONS = 0x6A7A94            # u32[128]: each character version's animation block
HAWK = 0x23                      # T. Hawk, Alpha 3 version
WALK_BACK, WALK_FORWARD = 3, 2   # animation numbers in the block's first slice (as the PSP's set 1)
FIRST_PICTURE = 40               # the PSP's walk back: pictures 51 (first frame) down to 40
SIZE, ORIGIN_X, ORIGIN_Y = 160, 80, 141   # the PSP's pictures 40 - 51: 5 x 5 cells of 32, origin 80, 141
TRANSPARENT_PEN = 15


class Overlay:
    def __init__(self, data, load):
        self.d, self.load = data, load

    def u16(self, a): return struct.unpack_from('<H', self.d, a - self.load)[0]
    def u32(self, a): return struct.unpack_from('<I', self.d, a - self.load)[0]


def frames(z, start):
    out, a = [], start
    while True:
        out.append(a)
        if z.u16(a) & 0xC000:
            return out
        a += 20


def animation(z, character, number):
    block = z.u32(ANIMATIONS + 4 * character)
    first_slice = z.u32(block)
    off = z.u16(first_slice + 2 * number)
    if not off:
        sys.exit(f"character {character:#x} has no animation {number}")
    return [(z.u16(f) & 0xFF, z.u32(f + 4)) for f in frames(z, first_slice + off)]


def unpack_tile(bank, n):
    count = struct.unpack_from('<I', bank, 0x14)[0]
    if n >= count:
        return None
    o = struct.unpack_from('<I', bank, 0x30 + 4 * n)[0]
    if o == 0:
        return None
    if not o & 0x80000000:
        return bank[o:o + 128]
    s, out, bits, flags = o & 0x7FFFFFFF, bytearray(), 0, 0
    while len(out) < 128:
        if bits == 0:
            flags, s, bits = bank[s], s + 1, 8
        t = bank[s]; s += 1
        if not flags & 0x80:
            out.append(t)
        else:
            h, l = t >> 4, t & 15
            for _ in range(l + 2):
                out.append(out[-1] if h == 0 else out[-h - 1])
        flags, bits = (flags << 1) & 0xFF, bits - 1
    return bytes(out[:128])


def render(z, positions, bank, record):
    """A sprite record as SIZE x SIZE rows of PSP colours (0 transparent), the origin at ORIGIN_X / Y.
    Answers (rows, pixels that fell outside)."""
    if z.u16(record) != 2:
        sys.exit(f"{record:#x} is not a sprite record")
    n = z.u16(record + 4)
    p = record + 6 + (2 if (record + 6) % 4 else 0)
    plist = z.u32(p) - POSITIONS_LOAD
    rows = [[0] * SIZE for _ in range(SIZE)]
    outside = 0
    for i in range(n + 1):
        tile, flags, size = struct.unpack_from('<HBB', z.d, p + 4 + 4 * i - z.load)
        x, y = struct.unpack_from('<hh', positions, plist + 4 * i)
        w, h = (size & 15) + 1, (size >> 4) + 1
        fx, fy = flags & 0x20, flags & 0x40
        for r in range(h):
            for c in range(w):
                t = unpack_tile(bank, (tile + c + 16 * r) & 0xFFFF)
                if t is None:
                    continue
                cc, rr = (w - 1 - c if fx else c), (h - 1 - r if fy else r)
                for ty in range(16):
                    for tx in range(16):
                        pen = (t[ty * 8 + tx // 2] >> (4 * (tx & 1))) & 15
                        if pen == TRANSPARENT_PEN:
                            continue
                        px = ORIGIN_X + x + cc * 16 + (15 - tx if fx else tx)
                        py = ORIGIN_Y + y + rr * 16 + (15 - ty if fy else ty)
                        if 0 <= px < SIZE and 0 <= py < SIZE:
                            rows[py][px] = pen + 1
                        else:
                            outside += 1
    return rows, outside


def psp_palette(pac_path):
    """T. Hawk's first colour, palette 0, for viewing the PNGs (the game ignores the PNG's palette)."""
    entries = pac.read(pac_path)
    pal = next((e for t, e in entries if t & 0xFFFF0000 == 0x00400000), None)
    if pal is None:
        return None
    return [pac.rgba16(struct.unpack_from('<H', pal, 2 * i)[0], 0) for i in range(16)]


def write_png(path, rows, palette):
    plte, trns = bytearray(), bytearray()
    for i in range(256):
        rgba = palette[i] if palette and i < 16 else (i * 16 & 0xFF, i * 16 & 0xFF, i * 16 & 0xFF, 255)
        plte += bytes(rgba[:3]); trns.append(0 if i == 0 else 255)
    pac.png(path, SIZE, SIZE, [bytes(r) for r in rows], 3, bytes(plte), bytes(trns))


def psp_picture_rows(fighter, n):
    """The PSP's picture n as SIZE x SIZE colour indices (for the check), through tools/sprites.py."""
    return [list(r) for r in fighter.picture_rows(n)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--iso', help='your Street Fighter Alpha Anthology (US) disc image (default: the .iso in imports/ps2/)')
    ap.add_argument('--out', default=os.path.join(ROOT, 'assets', 'us', 'anthology', 'hawk_walkback'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'), help='your PSP disc, extracted (tools/extract_assets.py)')
    ap.add_argument('--psp-pac', help="the PSP's T. Hawk archive (default: pl23_u.pac in --work): its palette for the PNGs, "
                                      "and the walk-forward check")
    a = ap.parse_args()
    a.psp_pac = a.psp_pac or os.path.join(a.work, 'iso', 'PSP_GAME', 'USRDIR', 'usa', 'data', 'pl23_u.pac')

    iso_path = a.iso or an.find_iso(None)
    iso = an.Iso(iso_path)
    if 'X_DATA.BIN' not in iso.files or 'SLUS_213.17' not in iso.files:
        sys.exit(f"{iso_path}: not Street Fighter Alpha Anthology (US, SLUS-21317)")
    base, entries = an.afs_entries(iso, 'X_DATA.BIN')
    entry = lambda i: iso.read(base + entries[i][0], entries[i][1])
    zero9 = an.unlz16(entry(ZERO9))
    if an.mwo3_header(zero9)['name'] != 'zero9_usa.bin':
        sys.exit(f"X_DATA[{ZERO9}] is not Hyper Street Fighter Alpha (zero9_usa.bin): a different disc?")
    z = Overlay(zero9, ZERO9_LOAD)
    positions = an.unlz16(entry(POSITIONS))
    bank = entry(HAWK_TILES)
    if bank[:0x14] != bytes(0x14) or struct.unpack_from('<I', bank, 0x18)[0] != 0x30:
        sys.exit(f"X_DATA[{HAWK_TILES}] is not a tile bank: a different disc?")

    back = animation(z, HAWK, WALK_BACK)
    if len(back) != 12:
        sys.exit(f"T. Hawk's walk back has {len(back)} frames, not 12: a different disc?")
    have_pac = os.path.isfile(a.psp_pac)
    palette = psp_palette(a.psp_pac) if have_pac else None

    if have_pac and os.path.isfile(os.path.join(a.work, 'analysis.image.bin')):
        import sprites
        fighter = sprites.Fighter(a.psp_pac, a.work)
        forward = animation(z, HAWK, WALK_FORWARD)
        same = differ = 0
        for k, (_, rec) in enumerate(forward):
            rows, _ = render(z, positions, bank, rec)
            psp = psp_picture_rows(fighter, 28 + k)
            d = sum(a_ != b_ for r1, r2 in zip(rows, psp) for a_, b_ in zip(r1, r2))
            same += d == 0; differ += d
        # (the US disc: 10 of 12 identical; two differ by 1 and 31 pixels, small retouches in Hyper)
        print(f"check: Hyper's walk forward against the PSP's pictures 28 - 39: {same} / {len(forward)} identical, "
              f"{differ} pixels differ in all")
        if differ > SIZE * SIZE // 100:
            sys.exit("the walk forward doesn't match the PSP's: nothing written")

    os.makedirs(os.path.join(a.out, 'pictures'), exist_ok=True)
    pictures = []
    for j, (duration, rec) in enumerate(back):          # frame j is the PSP's picture 51 - j
        n = FIRST_PICTURE + len(back) - 1 - j
        rows, outside = render(z, positions, bank, rec)
        if outside:
            sys.exit(f"record {rec:#x}: {outside} pixels outside the {SIZE} x {SIZE} picture")
        name = f"pictures/{n:04d}.png"
        write_png(os.path.join(a.out, name), rows, palette)
        pictures.append({'n': n, 'file': name, 'hyper_record': f"{rec:#x}", 'duration': duration})
    pictures.sort(key=lambda p: p['n'])
    meta = {
        'what': "T. Hawk's walk back from Hyper Street Fighter Alpha, as the PSP's pictures 40 - 51 (pl23)",
        'source': {'disc': 'Street Fighter Alpha Anthology (US) SLUS-21317', 'iso': os.path.basename(iso_path),
                   'program': f'X_DATA[{ZERO9}] zero9_usa.bin', 'tiles': f'X_DATA[{HAWK_TILES}]',
                   'positions': f'X_DATA[{POSITIONS}]'},
        'format': 'index = colour (palette 0), 0 transparent',
        'size': [SIZE, SIZE], 'origin': [ORIGIN_X, ORIGIN_Y], 'cell_size': 32,
        'pictures': pictures,
    }
    with open(os.path.join(a.out, 'walkback.json'), 'w') as f:
        json.dump(meta, f, indent=1)
    print(f"wrote {len(pictures)} pictures of T. Hawk's walk back to {os.path.relpath(a.out, ROOT)}")


if __name__ == '__main__':
    main()
