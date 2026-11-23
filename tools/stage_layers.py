#!/usr/bin/env python3
"""Each stage's layers whole, from the disc: one PNG per layer, the layer's entire map, not the part a
camera shows. For the art pack (tools/art_pack.py) and anyone painting the Stage Extensions.

A stage's archive (stNN.pac) holds its tile maps in entry 0 (0x6000 bytes of map words, then its palettes
at +0x6000) and its textures in page 0xC (slots 0 up; the 4- and 8-bit ones take their colours from an
archive's entry 0, as tools/data/palette_pairs.json pairs them). The fight draws them as
core/stage/background.c's draw_background and core/stage/floor.c do; the numbering here is the game's:

  layer 0  map +0x0000: 16-pixel tiles, 64 x 32 (0x0881A8E0); 32-pixel tiles, 32 x 16 (0x0881AB28) on
           DHALSIM and VEGA (layer 0's +155, camera_frame.c); BALROG's kind 0x85, 144 x 16 (its three
           frames side by side); GUILE's 13 strips of 16 x 16 tiles side by side (0x0881C354; a table
           picks which show as the layer scrolls); BISON 1 / 2: a picture, the mountains (0x08819498:
           page 0xC slot 6, 256 x 80, two frames, at layer x 0x200 - 0x300, y 0x60 - 0xB0)
  layer 1  the fighters' layer, map +0x2000: 16-pixel tiles, 64 x 32 (0x0881BD28); BALROG's map at
           +0x2400, GUILE's at +0x3400 (0x0881A8E0); BISON 1 / 2: a picture, the sky (0x0881C0C8: three
           256 x 160 columns from slots 7 and 8, rippled by a wave table as it draws; drawn here flat)
  layer 2  map +0x4000: 32-pixel tiles, 32 x 16 (0x0881AB28); 16-pixel, 64 x 32, on KEN, CHARLIE and
           SAGAT (layer 2's +155); BALROG's map at +0x4400, GUILE's at +0x5400; BISON 1 / 2 animated
           (kinds 0xDA / 0xEE: 11 frames, each 24 tiles of a 0x110-word row, 8 rows)
  layer 3  the floor band (floor.c), on the stages whose own routine draws one: four 256-pixel quads of
           page 0xC slot 0 side by side from layer x 128, quad i from texture rows 64i + 64 - height to
           64i + 64 (height = 40 + the stage's lean top, 0x08A09268), drawn here flat, before the
           perspective lean. BISON 1 / 2 put two bands above it (slots 4 and 0, 32 and 16 rows: rows
           0xA0 - 0xD0 of the layer), stacked above the floor here.
  layer 4  CHARLIE only: the picture his floor routine draws first (slot 1, 192 x 48, at layer x 0x270,
           y 0x50)

A map word: bit 31 a tile to draw, bits 28 / 29 flip it across / up and down, bit 30 its blend, bits 8 -
15 its texture (page 0xC's slot), bits 0 - 7 its 16 x 16 cell in that texture (tiles.c, draw_tile).

Writes <out>/<nn>_<name>/layer<n>.png (transparent where nothing is drawn; an animated layer as
layer<n>_frame<ff>.png) and layers.json: each layer's source, size, drawer and where it sits in the stage.
The stage objects (sprites: Ryu's straw, Ken's crowd ...) are not layers and are not drawn.

Usage: python tools/stage_layers.py [--disc work/iso/PSP_GAME/USRDIR/usa/data] [--work work]
                                    [--out art-pack/us/layers] [--stages 0x00,0x20]
Standard library only.
"""
import argparse, json, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import pac  # noqa: E402

BASE = 0x08804000                 # work/analysis.image.bin's first address
FLOOR_ROUTINES = 0x08A0893C       # u32[42]: the stage's own routine (its floor)
LEAN_TOP = 0x08A09268             # u16[42]: the lean's first height below 0xD8
FLOOR, FLOOR_CHARLIE, FLOOR_BISON = 0x0881C5A8, 0x088196DC, 0x0881C82C
BALROG, GUILE, BISON1, BISON2, CHARLIE = 0x15, 0x20, 0x0A, 0x1E, 0x03
GUILE_STRIPS = 13                 # 0xD0 words a row / 16


def tile_layers(stage):
    """[(layer, map offset, tile, columns shown, rows, words a row, drawer, note)] of the tile layers"""
    out = []
    if stage == GUILE:
        out.append((0, 0x0000, 16, 16 * GUILE_STRIPS, 16, 0xD0, '0x0881C354', 'strips 16 tiles wide side by side; a table (0x08A08BDD) picks which show as the layer scrolls'))
    elif stage == BALROG:
        out.append((0, 0x0000, 16, 144, 16, 144, '0x0881A8E0', 'kind 0x85, animated: its frames side by side'))
    elif stage in (0x1C, 0x0F):
        out.append((0, 0x0000, 32, 32, 16, 32, '0x0881AB28', ''))
    elif stage not in (BISON1, BISON2):
        out.append((0, 0x0000, 16, 64, 32, 64, '0x0881A8E0', ''))
    if stage == BALROG:
        out.append((1, 0x2400, 16, 64, 32, 64, '0x0881A8E0', ''))
    elif stage == GUILE:
        out.append((1, 0x3400, 16, 64, 32, 64, '0x0881A8E0', ''))
    elif stage not in (BISON1, BISON2):
        out.append((1, 0x2000, 16, 64, 32, 64, '0x0881BD28', ''))
    if stage == BALROG:
        out.append((2, 0x4400, 32, 32, 16, 32, '0x0881AB28', ''))
    elif stage == GUILE:
        out.append((2, 0x5400, 32, 32, 16, 32, '0x0881AB28', ''))
    elif stage in (BISON1, BISON2):
        out.append((2, 0x4000, 32, 24, 8, 0x110, '0x0881AB28', 'animated: 11 frames, each 24 tiles, 24 words on from the last'))
    elif stage in (0x01, CHARLIE, 0x0B):
        out.append((2, 0x4000, 16, 64, 32, 64, '0x0881A8E0', ''))
    else:
        out.append((2, 0x4000, 32, 32, 16, 32, '0x0881AB28', ''))
    return out


class Image:
    """RGBA rows; transparent where nothing is drawn"""
    def __init__(self, w, h):
        self.w, self.h, self.rows = w, h, [bytearray(w * 4) for _ in range(h)]

    def put(self, x, y, src_rows):
        """opaque texels of src_rows (RGBA bytes each) over this, from (x, y)"""
        for j, src in enumerate(src_rows):
            if not 0 <= y + j < self.h: continue
            dst = self.rows[y + j]
            alpha = src[3::4]
            if not any(alpha): continue
            if 0 not in alpha and x >= 0 and x * 4 + len(src) <= len(dst):
                dst[x * 4:x * 4 + len(src)] = src
                continue
            for i in range(0, len(src), 4):
                if src[i + 3] and 0 <= x + i // 4 < self.w: dst[x * 4 + i:x * 4 + i + 4] = src[i:i + 4]

    def save(self, path):
        pac.png(path, self.w, self.h, [bytes(r) for r in self.rows], 6)


class Textures:
    """page 0xC's textures of an archive, decoded on demand: slot -> (rows of RGBA bytes, width)"""
    def __init__(self, disc, name, pairs):
        self.disc, self.name, self.pairs, self.cache, self.archives = disc, name, pairs, {}, {}
        self.entries = self.archive(name)

    def archive(self, name):
        if name not in self.archives: self.archives[name] = pac.read(os.path.join(self.disc, name + '.pac'))
        return self.archives[name]

    def __getitem__(self, slot):
        if slot not in self.cache: self.cache[slot] = self.decode(slot)
        return self.cache[slot]

    def decode(self, slot):
        for i, (t, e) in enumerate(self.entries):
            if (t >> 16) in (0x1000, 0x3000) and (t >> 8) & 0xFF == 0x0C and t & 0xFF == slot: break
        else: return None
        hd = pac.pvr_header(e)
        if not hd: return None
        off, pf, df, w, h = hd
        bits = {1: 16, 5: 4, 7: 8}.get(df)
        if bits is None: return None
        data, clut = e[off:], None
        if bits != 16:
            p = self.pairs.get('%s/%d' % (self.name, i))
            if not p: return None
            clut = struct.unpack_from('<%dH' % (1 << bits), self.archive(p['archive'])[p['entry']][1], p['offset'])
        lut = {}
        def col(v, f):
            k = (v, f)
            if k not in lut:
                r = pac.rgba16(v, f)
                lut[k] = bytes(r[:3]) + b'\xff' if r[3] >= 128 else b'\0\0\0\0'
            return lut[k]
        rows = []
        for y in range(h):
            if bits == 16:
                rows.append(b''.join(col(v, pf) for v in struct.unpack_from('<%dH' % w, data, y * w * 2)))
            else:
                if bits == 8: idx = data[y * w:(y + 1) * w]
                else: idx = [k for b in data[y * w // 2:(y + 1) * w // 2] for k in (b & 15, b >> 4)]
                rows.append(b''.join(b'\0\0\0\0' if k == 0 else col(clut[k], 0) for k in idx))
        return rows, w

    def rect(self, slot, u, v, w, h):
        """texels u..u+w, v..v+h of a slot as RGBA rows (None if the texture isn't there)"""
        t = self[slot]
        if t is None: return None
        rows, tw = t
        return [rows[(v + j) % len(rows)][u * 4:(u + w) * 4] for j in range(h)]


def render_map(data, off, tile, cols, rows, step, tex, first=0):
    """a tile map as an Image; also (tiles drawn, tiles whose texture is missing)"""
    img = Image(cols * tile, rows * tile)
    drawn = missing = 0
    for r in range(rows):
        for c in range(cols):
            a = off + (first + r * step + c) * 4
            if a + 4 > len(data): continue
            w = struct.unpack_from('<I', data, a)[0]
            if not w & 0x80000000: continue
            slot, cell = (w >> 8) & 0xFF, w & 0xFF
            t = tex[slot]
            if t is None: missing += 1; continue
            trows, tw = t
            u, v = (cell & 0xF) << 4, cell & 0xF0
            u, v = u % tw, v % len(trows)      # a texture smaller than 256 repeats, as the GE wraps it
            if u + tile > tw or v + tile > len(trows): missing += 1; continue
            src = [trows[v + y][u * 4:(u + tile) * 4] for y in range(tile)]
            if w & 0x10000000: src = [b''.join(s[i:i + 4] for i in range(len(s) - 4, -4, -4)) for s in src]
            if w & 0x20000000: src.reverse()
            img.put(c * tile, r * tile, src)
            drawn += 1
    return img, drawn, missing


def floor_bands(stage, routine, lean_top, tex):
    """the floor band (and BISON's two above it) flat: (Image, record)"""
    height = 0x100 - (0xD8 - lean_top)
    parts = []
    if routine == FLOOR_BISON:
        parts.append(('BISON band, layer rows 0xA0 - 0xC0', [tex.rect(4, 0, 32 * i, 256, 32) for i in range(4)]))
        parts.append(('BISON band, layer rows 0xC0 - 0xD0', [tex.rect(0, 0, 64 * i, 256, 16) for i in range(4)]))
    parts.append(('the floor', [tex.rect(0, 0, 64 * i + 64 - height, 256, height) for i in range(4)]))
    total = sum(len(q[0]) for _, q in parts if q[0])
    img, y = Image(1024, total), 0
    for _, quads in parts:
        for i, q in enumerate(quads):
            if q: img.put(256 * i, y, q)
        y += len(quads[0]) if quads[0] else 0
    note = ('flat (the game leans it in perspective as the camera moves); from layer x 128, the floor\'s '
            'top at layer y 256 - %d; texture rows 64i + 64 - %d .. 64i + 64 for quad i' % (height, height))
    if routine == FLOOR_BISON: note = 'the two bands above the floor (32 and 16 rows), then the floor; ' + note
    return img, {'drawer': '0x%08X' % routine, 'source': 'page 0xC slot 0' + (', slot 4' if routine == FLOOR_BISON else ''),
                 'x': 128, 'note': note}


def write_stage(disc, stage, folder, pairs, image=None):
    """the stage's layers into folder; returns a one-line summary"""
    name = 'st%02x' % stage
    tex = Textures(disc, name, pairs)
    data = tex.entries[0][1]
    os.makedirs(folder, exist_ok=True)
    info, line = [], []

    def save(layer, img, rec, suffix=''):
        fn = 'layer%d%s.png' % (layer, suffix)
        img.save(os.path.join(folder, fn))
        rec.setdefault('files', []).append(fn)
        rec['size'] = [img.w, img.h]

    if stage in (BISON1, BISON2):
        rec = {'layer': 0, 'drawer': '0x08819498', 'source': 'page 0xC slot 6', 'x': 0x200, 'y': 0x60,
               'note': 'a picture, the mountains: two frames (texture rows 0 / 0x50), swapped every 8 frames'}
        for f in range(2):
            img = Image(256, 80); img.put(0, 0, tex.rect(6, 0, 0x50 * f, 256, 80) or [])
            save(0, img, rec, '_frame%02d' % f)
        info.append(rec); line.append('layer0 picture')
        rec = {'layer': 1, 'drawer': '0x0881C0C8', 'source': 'page 0xC slots 7, 8',
               'note': 'a picture, the sky: three 256 x 160 columns side by side, the texture rows running on '
                       'from slot 7 into slot 8; drawn flat (the game ripples each row by a wave table, 0x08A08ADC)'}
        img, slot, v, texrow = Image(768, 160), 7, 0, 1
        for c in range(3):
            for i in range(160):          # as draw_sky counts its slices
                was = texrow; texrow += 1
                if was == 0x100: slot, v, texrow = 8, 0, 1
                img.put(256 * c, i, tex.rect(slot, 0, v, 256, 1) or [])
                v = texrow
        save(1, img, rec); info.append(rec); line.append('layer1 picture')

    for layer, off, tile, cols, rows, step, drawer, note in tile_layers(stage):
        rec = {'layer': layer, 'drawer': drawer, 'source': '%s entry 0 +0x%04X' % (name, off), 'tile': tile,
               'columns': cols, 'rows': rows, 'note': note}
        if stage in (BISON1, BISON2) and layer == 2:
            for f in range(11):
                img, n, miss = render_map(data, off, tile, cols, rows, step, tex, first=8 + f * 24)
                save(layer, img, rec, '_frame%02d' % f)
        else:
            img, n, miss = render_map(data, off, tile, cols, rows, step, tex)
            save(layer, img, rec)
            if miss: rec['missing_textures'] = miss
        info.append(rec)
        line.append('layer%d %dx%d' % (layer, rec['size'][0], rec['size'][1]) + (' (%d tiles missing)' % miss if miss else ''))

    if image is not None:
        w = lambda a: struct.unpack_from('<I', image, a - BASE)[0]
        h = lambda a: struct.unpack_from('<H', image, a - BASE)[0]
        routine = w(FLOOR_ROUTINES + 4 * stage)
        if routine in (FLOOR, FLOOR_CHARLIE, FLOOR_BISON):
            img, rec = floor_bands(stage, routine, h(LEAN_TOP + 2 * stage), tex)
            rec['layer'] = 3
            save(3, img, rec); info.append(rec); line.append('layer3 floor %dx%d' % (img.w, img.h))
        if routine == FLOOR_CHARLIE:
            rec = {'layer': 4, 'drawer': '0x088196DC', 'source': 'page 0xC slot 1', 'x': 0x270, 'y': 0x50,
                   'note': 'a picture CHARLIE\'s floor routine draws before the floor; it slides with the lean'}
            img = Image(192, 48); img.put(0, 0, tex.rect(1, 0, 0, 192, 48) or [])
            save(4, img, rec); info.append(rec); line.append('layer4 picture')

    info.sort(key=lambda r: r['layer'])
    json.dump({'stage': '0x%02X' % stage, 'archive': name + '.pac', 'layers': info},
              open(os.path.join(folder, 'layers.json'), 'w'), indent=1)
    return ', '.join(line)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--disc', default=os.path.join(ROOT, 'work', 'iso', 'PSP_GAME', 'USRDIR', 'usa', 'data'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'art-pack', 'us', 'layers'))
    ap.add_argument('--stages', help='stage numbers, e.g. 0x00,0x20 (default: every stage with its own archive)')
    a = ap.parse_args()
    import asset_layout
    pairs = json.load(open(os.path.join(ROOT, 'tools', 'data', 'palette_pairs.json')))['textures']
    img_path = os.path.join(a.work, 'analysis.image.bin')
    image = open(img_path, 'rb').read() if os.path.exists(img_path) else None
    if image is None: print('No %s: the floor bands are left out.' % img_path)
    stages = [int(s, 16) for s in a.stages.split(',')] if a.stages else range(0x2A)
    seen = set()
    for stage in stages:
        try: rel = asset_layout.stage(stage)
        except KeyError: continue
        if rel in seen or not os.path.exists(os.path.join(a.disc, 'st%02x.pac' % stage)): continue
        seen.add(rel)
        folder = os.path.join(a.out, rel.split('/')[-1])
        print('st%02x %-13s %s' % (stage, rel.split('/')[-1], write_stage(a.disc, stage, folder, pairs, image)), flush=True)


if __name__ == '__main__':
    main()
