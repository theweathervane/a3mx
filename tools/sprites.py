#!/usr/bin/env python3
"""A fighter's sprites from its pl*.pac archive (docs/SPRITE_FORMAT.md).

The archive's entries, as the loader places them (src/core/loader/async.c):
    0x00100000  the picture list   (0x08E75AD8[slot]): 12-byte pictures, their cells after them
    0x00200000  the tile set       (0x092D9D40[slot]): u32 offsets (from the set) to LZ-packed tiles
    0x00400000  the palettes       (0x092D9E00[slot]): 6 colours x 5 palettes x 16 ABGR1555 colours
and entry 6 (data, `06_data`) holds the animation frames (10 bytes each) at the start of its data.

The drawing code these follow is decompiled: src/core/object/sprite_cache.c (cells, tiles, the LZ
unpacker), sprite_grid.c (placing cells), sprite_drawers.c (pictures by frame).

Usage:
    sprites.py verify --pac work/iso/.../pl00_u.pac --snap RAM.bin --slot 0
        rebuilds every picture-cache page the game holds for that fighter slot in a RAM snapshot
        (--snap of 0x08800000 - 0x0A000000) and compares them byte for byte
    sprites.py picture --pac PAC --picture N [--colour C] --out FILE.png
        one picture as a PNG (anchor at its origin, see the metadata)
"""
import argparse, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pac

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMAGE_BASE = 0x08804000          # work/analysis.image.bin: the program image from here
LZ_STEP = 0x08A3E680             # u8[8]: the write's step by (address & 7)
LZ_BACK = 0x08A3E600             # u8[8][16]: how far back a copy starts, by (address & 7) and h
TILE_PLACES = 0x08A042EC         # u32[16]: where the i-th tile of a page goes in it
RAM_BASE = 0x08800000            # --snap files start here

KIND_PICTURES, KIND_TILES, KIND_PALETTES = 0x00100000, 0x00200000, 0x00400000
EMPTY = 0xFF
FLIP_X, FLIP_Y, BLEND = 0x40, 0x80, 0x20
TILE, PAGE = 128, 2048           # bytes: a 16 x 16 tile, a 64 x 64 page (4-bit pixels, 32-byte rows)
SPILL = 512                      # a tile's last token may write past its 128 bytes (past the page's end)


def program_tables(work):
    img = open(os.path.join(work, 'analysis.image.bin'), 'rb').read()
    at = lambda a, n: img[a - IMAGE_BASE:a - IMAGE_BASE + n]
    return (at(LZ_STEP, 8), at(LZ_BACK, 128), struct.unpack('<16I', at(TILE_PLACES, 64)))


class Fighter:
    def __init__(self, pac_path, work):
        self.entries = pac.read(pac_path)
        by = {t & 0xFFFF0000: e for t, e in self.entries if t & 0xFFFF0000 in (KIND_PICTURES, KIND_TILES, KIND_PALETTES)}
        self.pictures, self.tiles, self.palettes = by[KIND_PICTURES], by[KIND_TILES], by.get(KIND_PALETTES, b'')
        self.step, self.back, self.places = program_tables(work)

    # ---- tiles (0x088D2B64) ----
    def unpack_tile(self, n, out, at):
        """Tile n of the set unpacked into `out` (a bytearray laid out as a page: 32-byte rows) at
        offset `at`, exactly as the game writes it. Answers the stream's length."""
        src = struct.unpack_from('<I', self.tiles, n * 4)[0]
        s, dst, written, bits, flags = src, at, 0, 0, 0
        step, back = self.step, self.back
        while True:
            token = self.tiles[s]; s += 1
            if bits == 0:
                flags = token; token = self.tiles[s]; s += 1; bits = 8
            if not flags & 1:
                out[dst] = token; dst += step[dst & 7]; written += 1
            else:
                h, l = token >> 4, token & 15
                if h == 0:
                    v = self.tiles[s]; s += 1
                    for _ in range(l + 1):
                        out[dst] = v; dst += step[dst & 7]
                    written += l + 1
                else:
                    frm = dst - back[(dst & 7) * 16 + h]
                    for _ in range(l):
                        out[dst] = out[frm]; dst += step[dst & 7]; frm += step[frm & 7]
                    written += l
            flags >>= 1; bits -= 1
            if written >= TILE:
                return s - src

    # ---- pictures ----
    def picture(self, n):
        """(cells offset, columns, rows, origin x, origin y, goes on into the next)"""
        off, cols, rows, ox, oy, more = struct.unpack_from('<IBBhhh', self.pictures, n * 12)
        return off, cols, rows, ox, oy, more != 0

    def cells(self, n):
        """The picture's cells in order: None (empty) or (tile, flags)."""
        off, cols, rows = self.picture(n)[:3]
        p, out = off, []
        for _ in range(cols * rows):
            if self.pictures[p] == EMPTY:
                out.append(None); p += 1
            else:
                out.append((self.pictures[p] << 8 | self.pictures[p + 1], self.pictures[p + 2])); p += 3
        return out

    def cache_pages(self, n, big, mask=False):
        """The pages 0x0880EE34 / 0x0880EFB8 fill for picture n: a list of 2048-byte pages (with mask,
        which of their bytes the tiles write)."""
        pages, page, fill, to = [], bytearray(PAGE + SPILL), 0, 0
        per_page = 4 if big else 16
        for cell in self.cells(n):
            if cell is None: continue
            if fill >= per_page:
                pages.append(page[:PAGE]); page, fill, to = bytearray(PAGE + SPILL), 0, 0
            tiles = [cell[0] * 4 + k for k in range(4)] if big else [cell[0]]
            for t in tiles:
                place = self.places[(to >> 7) & 0xFF]
                if mask:
                    for y in range(16): page[place + y * 32:place + y * 32 + 8] = b'\1' * 8
                else:
                    self.unpack_tile(t, page, place)
                to += TILE
            fill += 1
        pages.append(page[:PAGE])
        return pages

    # ---- pixels ----
    def tile_pixels(self, t):
        """Tile t as 16 rows of 16 colour indices."""
        buf = bytearray(PAGE + SPILL)
        self.unpack_tile(t, buf, 0)
        return [[(buf[y * 32 + x // 2] >> (4 * (x & 1))) & 15 for x in range(16)] for y in range(16)]

    def cell_pixels(self, tile, big):
        if not big: return self.tile_pixels(tile)
        q = [self.tile_pixels(tile * 4 + k) for k in range(4)]   # top left, bottom left, top right, bottom right
        return [q[0][y] + q[2][y] for y in range(16)] + [q[1][y] + q[3][y] for y in range(16)]

    def picture_indices(self, n, big=True):
        """The picture as rows of (palette, index) pairs, None where nothing is drawn."""
        off, cols, rows = self.picture(n)[:3]
        size = 32 if big else 16
        img = [[None] * (cols * size) for _ in range(rows * size)]
        for i, cell in enumerate(self.cells(n)):
            if cell is None: continue
            tile, flags = cell
            px = self.cell_pixels(tile, big)
            if flags & FLIP_X: px = [r[::-1] for r in px]
            if flags & FLIP_Y: px = px[::-1]
            cx, cy = (i % cols) * size, (i // cols) * size
            for y in range(size):
                for x in range(size):
                    if px[y][x]: img[cy + y][cx + x] = (flags & 0x1F, px[y][x])
        return img

    def colours(self, colour):
        """The fighter's palettes for one colour choice: 5 lists of 16 RGBA tuples."""
        base = colour * 160
        return [[pac.rgba16(struct.unpack_from('<H', self.palettes, base + p * 32 + i * 2)[0], 0)
                 for i in range(16)] for p in range(5)]


def write_png(path, img, colours):
    rows = []
    for r in img:
        line = bytearray()
        for px in r:
            line += bytes((0, 0, 0, 0)) if px is None else bytes(colours[px[0] % len(colours)][px[1]][:3] + (255,))
        rows.append(bytes(line))
    pac.png(path, len(img[0]) if img else 0, len(img), rows, 6)


def cmd_verify(a):
    f = Fighter(a.pac, a.work)
    ram = open(a.snap, 'rb').read()
    W = lambda x: struct.unpack_from('<I', ram, x - RAM_BASE)[0]
    B = lambda x: ram[x - RAM_BASE]
    # the snapshot holds this archive for the slot?
    plist, tset = W(0x08E75AD8 + 4 * a.slot), W(0x092D9D40 + 4 * a.slot)
    same = ram[plist - RAM_BASE:plist - RAM_BASE + len(f.pictures)] == f.pictures and \
           ram[tset - RAM_BASE:tset - RAM_BASE + len(f.tiles)] == f.tiles
    print('slot %d: pictures at %08X, tiles at %08X: %s' % (a.slot, plist, tset, 'this archive' if same else 'NOT this archive'))
    if not same: return 1
    by_offset = {}
    for n in range(len(f.pictures) // 12):
        off = f.picture(n)[0]
        if off < len(f.pictures): by_offset.setdefault(off, n)
    checked = bad = 0
    for i in range(512):
        first = W(0x092E8000 + 4 * i)
        if first == 0 or B(first + 14) == 0 or B(first + 12) != a.slot: continue
        n = by_offset.get(W(first + 4))
        if n is None: continue
        chain = [first]
        while W(chain[-1]) and len(chain) < 64: chain.append(W(chain[-1]))
        big = a.cells == 32
        want = f.cache_pages(n, big)
        masks = f.cache_pages(n, big, mask=True)   # only the bytes the tiles write: the rest of a page is stale
        got = [ram[W(0x08B0C334 + 24 * W(p + 8)) - RAM_BASE:][:PAGE] for p in chain[:len(want)]]
        diff = [(k, i) for k, (g, w, m) in enumerate(zip(got, want, masks)) for i in range(PAGE) if m[i] and g[i] != w[i]]
        ok = not diff and len(got) == len(want)
        checked += 1; bad += not ok
        if not ok and bad <= 5:
            print('  picture %d (%d pages, %d in the chain, %s cells): %d bytes differ, first page %s byte %s'
                  % (n, len(want), len(chain), '32' if big else '16', len(diff), *(diff[0] if diff else ('-', '-'))))
    print('cached pictures of slot %d checked: %d, differing: %d' % (a.slot, checked, bad))
    return 1 if bad or not checked else 0


def cmd_picture(a):
    f = Fighter(a.pac, a.work)
    img = f.picture_indices(a.picture, not a.small)
    write_png(a.out, img, f.colours(a.colour))
    off, cols, rows, ox, oy, more = f.picture(a.picture)
    print('picture %d: %d x %d cells, origin (%d, %d)%s -> %s' % (a.picture, cols, rows, ox, oy, ', goes on' if more else '', a.out))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    v = sub.add_parser('verify'); v.add_argument('--pac', required=True); v.add_argument('--snap', required=True)
    v.add_argument('--slot', type=int, default=0); v.add_argument('--cells', type=int, choices=(16, 32), default=32)
    v.add_argument('--work', default=os.path.join(ROOT, 'work'))
    p = sub.add_parser('picture'); p.add_argument('--pac', required=True); p.add_argument('--picture', type=int, required=True)
    p.add_argument('--colour', type=int, default=0); p.add_argument('--small', action='store_true')
    p.add_argument('--out', required=True); p.add_argument('--work', default=os.path.join(ROOT, 'work'))
    a = ap.parse_args()
    return {'verify': cmd_verify, 'picture': cmd_picture}[a.cmd](a) or 0


if __name__ == '__main__':
    sys.exit(main())
