#!/usr/bin/env python3
"""Which endings' scenes are cut from a stage, and where in that stage: the table WIDE NATIVE's scaled
endings use to draw the stage's own art on past a scene's sides (src/extensions/scaled_endings_stages.c).

An ending's archive (endNN_u.pac) holds its own maps (entry 0, loaded at 0x09345200) and textures
(page 0xC, slots 0x17 up). Its layer 1 (0x08819300, 16-pixel tiles, 64 columns) often holds a stage
scene cut to the 24 columns of the 4:3 picture: Ryu's stage behind Ryu and Sagat, for one. The stage's
archive (stNN.pac) has the whole of it, in its own maps (entry 0: layer 0 at +0, layer 1 at +0x2000,
layer 2 at +0x4000; 16- or 32-pixel tiles) and textures (page 0xC, slots 0 up).

For every band of rows of an ending's layer 0 / 1 (rows with art, between empty ones) this finds the
stage and the place in it whose tiles show the same pixels (16 x 16 blocks compared as colours, so a
texture kept in another 16-bit format still matches), which of the stage's layers make it up and in what
order (the ending's tiles are often two of the stage's layers drawn into one), and keeps a band only if
those layers, drawn in that order, give back at least MIN_SHARE of its blocks.

Usage:
    python tools/ending_stages.py [--disc work/iso/PSP_GAME/USRDIR/usa/data] [--write]
          prints the bands found; --write also writes src/extensions/scaled_endings_stages.h
Standard library only.
"""
import argparse, collections, glob, itertools, json, os, struct, sys, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import pac  # noqa: E402

MIN_SHARE = 0.6        # a band is kept when its stage's layers give back this share of its blocks
MIN_VOTES = 8
ENDING_LAYERS = {0: (0x0000, 0x2000), 1: (0x2000, 0x2000)}            # map offset, bytes: 16-pixel, 64 columns
STAGE_MAPS = [(0, 0x0000, 16), (0, 0x0000, 32), (1, 0x2000, 16), (2, 0x4000, 16), (2, 0x4000, 32)]   # layer, offset, tile
MAP_BYTES = 0x2000
DRAW_ORDER = [(0, 32), (0, 16), (2, 32), (2, 16), (1, 16)]   # the stage's layers, back to front (layer 1, the fighters', in front)


def pages(entries):
    """slot -> (pixels, width, bits, pixel format, entry index) for page 0xC's textures"""
    out = {}
    for i, (t, e) in enumerate(entries):
        if (t >> 16) in (0x1000, 0x3000) and (t >> 8) & 0xFF == 0x0C:
            hd = pac.pvr_header(e)
            if not hd: continue
            off, pf, df, w, h = hd
            bits = {1: 16, 5: 4, 7: 8}.get(df)
            if bits: out[t & 0xFF] = (e[off:], w, bits, pf, i)
    return out


_decoded = {}


def decoded(pg, p, cluts):
    """page p as RGBA rows (opaque pixels, transparent ones as zeros), decoded once"""
    key = (id(pg), p)                       # pg is kept alive below, so its id is never reused
    if key in _decoded: return _decoded[key][0]
    data, w, bits, pf, _ = pg[p]
    clut = cluts.get(p)
    if bits == 4 and clut is None:
        _decoded[key] = (None, pg)
        return None
    lut = {}
    def col(c, f):
        k = (c, f)
        if k not in lut:
            r = pac.rgba16(c, f)
            lut[k] = bytes(r[:3]) + b'\xff' if r[3] >= 128 else b'\0\0\0\0'
        return lut[k]
    rows = []
    for y in range(w):
        if bits == 16:
            vals = struct.unpack_from('<%dH' % w, data, y * w * 2)
            rows.append(b''.join(col(v, pf) for v in vals))
        else:
            line = data[y * w // 2:(y + 1) * w // 2]
            idx = [i for b in line for i in (b & 15, b >> 4)]
            rows.append(b''.join(b'\0\0\0\0' if i == 0 else col(clut[i], 0) for i in idx))
    _decoded[key] = ((rows, w), pg)
    return _decoded[key][0]


def texel_block(pg, word, tile, sx, sy, cluts):
    """The 16 x 16 block (sx, sy) of a tile, as RGBA bytes (transparent as zeros); None if unknown"""
    p, cell = (word >> 8) & 0xFF, word & 0xFF
    if p not in pg: return None
    d = decoded(pg, p, cluts)
    if d is None: return None
    rows, w = d
    u, v = (cell & 0xF) << 4, cell & 0xF0
    if u + tile > w or v + tile > w: return None
    flip_x, flip_y = word & 0x10000000, word & 0x20000000
    x0 = (tile - 16 - sx * 16) if flip_x else sx * 16
    y0 = (tile - 16 - sy * 16) if flip_y else sy * 16
    out = []
    for y in range(16):
        row = rows[v + y0 + y][(u + x0) * 4:(u + x0 + 16) * 4]
        if flip_x: row = b''.join(row[i:i + 4] for i in range(60, -4, -4))
        out.append(row)
    if flip_y: out.reverse()
    return b''.join(out)


def map_blocks(data, off, tile):
    """{(col, row) in 16-pixel units: (word, sx, sy)} for a map of 64 / 32 words a row"""
    cols = 64 if tile == 16 else 32
    n = min(MAP_BYTES, len(data) - off) // 4
    out = {}
    for k in range(n):
        w = struct.unpack_from('<I', data, off + k * 4)[0]
        if not w & 0x80000000: continue
        c, r = k % cols, k // cols
        for sy in range(tile // 16):
            for sx in range(tile // 16):
                out[(c * (tile // 16) + sx, r * (tile // 16) + sy)] = (w, sx, sy)
    return out


def stage_cluts(name, entries, pairs):
    """slot -> 16 colours for the stage's 4-bit textures, from tools/data/palette_pairs.json"""
    out = {}
    pg = pages(entries)
    for slot, (_, _, bits, _, idx) in pg.items():
        if bits != 4: continue
        p = pairs.get('%s/%d' % (name, idx))
        if not p or p['archive'] != name: continue
        e = entries[p['entry']][1]
        out[slot] = struct.unpack_from('<16H', e, p['offset'])
    return out


def compose(blocks):
    """the stage's layers drawn bottom first: RGBA bytes"""
    out = None
    for b in blocks:
        if b is None: continue
        if out is None: out = b; continue
        out = b''.join(b[i:i + 4] if b[i + 3] else out[i:i + 4] for i in range(0, len(b), 4))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--disc', default=os.path.join(ROOT, 'work', 'iso', 'PSP_GAME', 'USRDIR', 'usa', 'data'))
    ap.add_argument('--write', action='store_true')
    a = ap.parse_args()
    pairs = json.load(open(os.path.join(ROOT, 'tools', 'data', 'palette_pairs.json')))['textures']

    stages, index = {}, collections.defaultdict(list)
    for path in sorted(glob.glob(os.path.join(a.disc, 'st*.pac'))):
        name = os.path.basename(path)[:-4]
        es = pac.read(path)
        pg, cl, data = pages(es), stage_cluts(name, es, pairs), es[0][1]
        maps = {}
        for layer, off, tile in STAGE_MAPS:
            if off >= len(data): continue
            m = maps[(layer, tile)] = map_blocks(data, off, tile)
            for pos, (w, sx, sy) in m.items():
                b = texel_block(pg, w, tile, sx, sy, cl)
                if b and len(set(b)) > 4: index[b].append((name, layer, tile, pos))
        stages[name] = (pg, cl, maps)

    bands_out = []
    for path in sorted(glob.glob(os.path.join(a.disc, 'end*_u.pac'))):
        ending = int(os.path.basename(path)[3:5], 16)
        es = pac.read(path)
        pg, data = pages(es), es[0][1]
        for elayer, (off, size) in ENDING_LAYERS.items():
            m = map_blocks(data, off, 16)
            rows = sorted({r for _, r in m})
            bands = []
            for r in rows:
                if bands and r == bands[-1][-1] + 1: bands[-1].append(r)
                else: bands.append([r])
            for band in bands:
                own = {pos: v for pos, v in m.items() if pos[1] in band}
                pix = {pos: texel_block(pg, w, 16, sx, sy, {}) for pos, (w, sx, sy) in own.items()}
                votes = collections.Counter()
                for pos, bl in pix.items():
                    if not bl or len(set(bl[i:i + 4] for i in range(0, len(bl), 4))) < 4: continue
                    hits = index.get(bl, [])
                    for st, layer, tile, sp in hits:
                        votes[(st, (layer, tile), sp[0] - pos[0], sp[1] - pos[1])] += 1.0 / len(hits)
                by_stage = collections.Counter()
                for (st, L, dx, dy), n in votes.items(): by_stage[st] = max(by_stage[st], n)
                best = (0, None, None)
                for st, n in by_stage.most_common(3):
                    if n < MIN_VOTES / 2: continue
                    spg, scl, smaps = stages[st]
                    cands = collections.defaultdict(list)       # a layer's likeliest offsets
                    for (s2, L, dx, dy), v in votes.most_common():
                        if s2 == st and v >= 3 and len(cands[L]) < 3: cands[L].append((dx, dy))
                    def stage_block(src, c, r):
                        L, dx, dy = src
                        e = smaps[L].get((c + dx, r + dy))
                        return texel_block(spg, e[0], L[1], e[1], e[2], scl) if e else None
                    use, got = [], 0
                    for L in DRAW_ORDER:          # greedy, back to front: a layer (at its best offset) is kept if it gives back more blocks
                        if L not in smaps: continue
                        pick = None
                        for dx, dy in cands.get(L, []):
                            src = (L, dx, dy)
                            n2 = sum(1 for (c, r), bl in pix.items() if bl and compose([stage_block(M, c, r) for M in use + [src]]) == bl)
                            if n2 > got: got, pick = n2, src
                        if pick: use.append(pick)
                    if got > best[0]: best = (got, tuple(use), st)
                if not best[2]: continue
                st = best[2]
                share = best[0] / max(1, len(pix))
                cols = [c for c, _ in own]
                words = b''.join(struct.pack('<I', own[p][0]) for p in sorted(own) if own[p][1:] == (0, 0))
                line = '%s layer %d rows %d-%d cols %d-%d: %s, layers (layer, tile size, dx, dy) %s, %d / %d blocks (%.0f %%)' % (
                    os.path.basename(path), elayer, band[0], band[-1], min(cols), max(cols), st,
                    [(L[0], L[1], dx, dy) for L, dx, dy in best[1]], best[0], len(pix), share * 100)
                print(line + ('' if share >= MIN_SHARE else '  (left out)'))
                if share >= MIN_SHARE:
                    bands_out.append((ending, elayer, band[0], band[-1], min(cols), max(cols), zlib.crc32(words), st, best[1]))

    if not a.write: return 0
    used = sorted({b[7] for b in bands_out})
    out = ['/* Written by tools/ending_stages.py: do not edit. The endings\' scenes cut from a stage (scaled_endings_stages.c). */',
           '#pragma once', '#include <stdint.h>', '',
           'typedef struct { uint8_t layer, tile; int8_t dx, dy; } StageSource;   /* a stage layer (map at +0x2000 * n), its tile size, and its 16-pixel column / row for the ending\'s (0, 0) */',
           'typedef struct {',
           '    uint8_t ending, layer, row0, row1, col0, col1;   /* the ending (endNN_u.pac), its layer, the band\'s rows and art columns */',
           '    uint32_t crc;                                     /* crc32 of the band\'s map words, its first block of each tile, in order */',
           '    const char *stage;                                /* the stage\'s archive */',
           '    uint8_t nsources; StageSource sources[4];         /* drawn in this order, the first at the back */',
           '    uint8_t nclut; struct { uint8_t slot, entry; uint16_t offset; } cluts[4];   /* the 4-bit textures\' colours */',
           '} EndingStageBand;', '', 'static const EndingStageBand ENDING_STAGE_BANDS[] = {']
    pairs_by = {}
    for (ending, elayer, r0, r1, c0, c1, crc, st, src) in bands_out:
        spg, scl, _ = stages[st]
        cl = []
        for slot, (_, _, bits, _, idx) in sorted(spg.items()):
            if bits == 4:
                p = pairs.get('%s/%d' % (st, idx))
                if p and p['archive'] == st: cl.append('{ 0x%02X, %d, 0x%04X }' % (slot, p['entry'], p['offset']))
        srcs = ', '.join('{ %d, %d, %d, %d }' % (L[0], L[1], dx, dy) for L, dx, dy in src)
        out.append('    { 0x%02X, %d, %d, %d, %d, %d, 0x%08Xu, "%s.pac", %d, { %s }, %d, { %s } },' % (
            ending, elayer, r0, r1, c0, c1, crc, st, len(src), srcs, len(cl), ', '.join(cl) or '{ 0, 0, 0 }'))   # (an empty {} is not C before C23)
    out += ['};', '']
    dst = os.path.join(ROOT, 'src', 'extensions', 'scaled_endings_stages.h')
    open(dst, 'w', newline='\n').write('\n'.join(out))
    print('wrote', dst, len(bands_out), 'bands')
    return 0


if __name__ == '__main__':
    sys.exit(main())
