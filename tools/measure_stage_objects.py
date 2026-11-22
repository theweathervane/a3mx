#!/usr/bin/env python3
"""Measure the stage objects drawn in front of the fighters and write placeholder pieces for them
(docs/STAGE_EXTENSIONS.md, "Foreground objects").

A foreground object (sprite layer 0, depth index 7: STAGES.md) is a sprite, so the layer strips can't
extend it: its art stops at the 4:3 edge. A piece is new art pinned to the object (src/extensions/
stagestrips.c, stagestrips_object): this tool finds each object's outline on screen and writes a piece
at each side of it, 64 units out and 8 under the object's own art (the same overlap as the strips).

For each stage that has them it runs a Free Battle in WIDE NATIVE with the fighters at the left wall,
in the middle and at the right wall, once as is and once per object (kind.type) with that object hidden
(A3MX_OBJ_MEASURE), and compares the two frames: the pixels that differ are the object's. The extension's
log gives the object's position, the screen's scale and offset, so the outline is turned into the 2D
library's units relative to the object's position, which is how the pieces are placed.

Writes, per stage folder under assets/extensions/stages/<nn>_<name>/:
  widescreen_object_<kind>_<type>_left.png / _right.png   yellow, labelled, transparent nowhere
  widescreen.json   "object_<kind>_<type>_<side>": x, "..._y": y (the piece's top left)
A piece that isn't a placeholder any more (somebody painted it) is kept, and its place too.

Usage: python tools/measure_stage_objects.py [--exe build/a3mx] [--stages 1,8] [--dry-run]
US program only. Needs Pillow.
"""
import argparse, json, os, re, shutil, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure_stages as ms
import asset_layout
import stage_objects as so

ROOT = ms.ROOT
EXT = os.path.join(ROOT, 'assets', 'extensions')
OUT_W, OVERLAP, PAD = 64, 8, 2
COLOUR, TEXT = (255, 220, 0), (102, 88, 0)
POSITIONS = {'L': '118', 'M': '280', 'R': '3E8'}     # fighters' x: the left wall, the middle, the right wall


def foreground(stage):
    """[(kind, type)] of the stage's foreground objects, from its records."""
    image = open(os.path.join(ROOT, 'work', 'analysis.image.bin'), 'rb').read()
    out = []
    for r in so.records(image, stage):
        if r['layer'] == 0 and r['value_24'] == 7 and (r['kind'], r['type']) not in out: out.append((r['kind'], r['type']))
    return out


def run(exe, out, k, pos, hide):
    from PIL import Image
    d = os.path.join(out, '%02d%s_%s' % (k, pos, hide.replace('.', '_').replace('-', 'm')))
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    open(os.path.join(d, 'a3mx.cfg'), 'w').write('[screen setting]\nscreen_size=WIDE NATIVE\n')
    inp, f = ms.script(k); p, e = f + 1500, f + 1900
    pokes = []
    for a in (ms.P1_X, ms.P2_X): pokes += ['--poke', '%d-%d:%08X=%s/2' % (p, e, a, POSITIONS[pos])]
    cmd = [exe, '--work', os.path.join(ROOT, 'work'), '--assets', os.path.join(ROOT, 'assets', 'us'), '--save-dir', d,
           '--log', os.path.join(d, 'log'), '--headless', '--fast', '--renderer', 'lazy', '--frames', str(e + 1),
           '--input', inp, '--dump', d, '--dump-from', str(e), '--dump-every', str(e),
           '--features', 'default,-boot_to_menu,stage_extensions'] + pokes
    env = dict(os.environ, A3MX_OBJ_MEASURE='%d-%d:%s' % (e - 5, e + 5, hide))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, env=env)
    objs = []
    pat = re.compile(r'stage objects: frame %d object (\d+)\.(\d+) at (-?\d+) (-?\d+) scale (\S+) (\S+) offset (\S+) (\S+)' % e)
    for line in open(os.path.join(d, 'log'), errors='replace'):
        m = pat.search(line)
        if m: objs.append((int(m[1]), int(m[2]), int(m[3]), int(m[4]), float(m[5]), float(m[6]), float(m[7]), float(m[8])))
    img = Image.open(os.path.join(d, 'frame_%05d.png' % e)).convert('RGB')
    return img, objs


def blobs(a, b):
    """Groups of columns where the two frames differ (gaps of 8 split them): [(x0, x1, y0, y1)]."""
    w, h = a.size
    pa, pb = a.load(), b.load()
    cols = {}
    for x in range(w):
        ys = [y for y in range(h) if pa[x, y] != pb[x, y]]
        if ys: cols[x] = (min(ys), max(ys))
    out = []
    for x in sorted(cols):
        if out and x - out[-1][1] <= 8:
            o = out[-1]; out[-1] = (o[0], x, min(o[2], cols[x][0]), max(o[3], cols[x][1]))
        else: out.append((x, x, cols[x][0], cols[x][1]))
    return [o for o in out if (o[1] - o[0]) * (o[3] - o[2]) >= 16]


def outline(exe, out, k, kind, typ):
    """(left, top, right, bottom) of the object relative to its position, in the 2D library's units, from
    the best view of it (a whole blob, not cut by the screen's edge, the widest); None if never seen."""
    best = None
    for pos in POSITIONS:
        full, objs = run(exe, out, k, pos, '-2.-2')
        hidden, _ = run(exe, out, k, pos, '%d.%d' % (kind, typ))
        mine = [o for o in objs if o[0] == kind and o[1] == typ]
        if not mine: continue
        sx, sy, ox, oy = mine[0][4:8]
        for (x0, x1, y0, y1) in blobs(full, hidden):
            whole = x0 > 0 and x1 < full.size[0] - 1
            cx = (x0 + x1) / 2
            near = min(mine, key=lambda o: abs((o[2] * sx + ox) - cx))       # the instance this blob is
            rel = ((x0 - ox) / sx - near[2], (y0 - oy) / sy - near[3], (x1 + 1 - ox) / sx - near[2], (y1 + 1 - oy) / sy - near[3])
            score = (whole, x1 - x0)
            if best is None or score > best[0]: best = (score, tuple(int(round(v)) for v in rel))
    return best[1] if best else None


def is_placeholder(path):
    from PIL import Image
    ok = {COLOUR + (255,), TEXT + (255,)}
    return all(px in ok for px in Image.open(path).convert('RGBA').getdata())


def make_piece(path, w, h, label):
    from PIL import Image, ImageDraw, ImageFont
    im = Image.new('RGBA', (w, h), COLOUR + (255,))
    d = ImageDraw.Draw(im)
    try: font = ImageFont.truetype('DejaVuSansMono-Bold.ttf', 10)
    except OSError: font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', 10)
    d.text((2, max(0, h // 2 - 6)), label, font=font, fill=TEXT + (255,))
    im = im.convert('RGBA')
    # keep to the two placeholder colours (no smoothed text edges)
    im.putdata([TEXT + (255,) if px[:3] != COLOUR else COLOUR + (255,) for px in im.getdata()])
    im.save(path, optimize=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stage_objects'))
    ap.add_argument('--stages', help='menu numbers (1-38), comma-separated; default all')
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()
    ks = [int(s) for s in a.stages.split(',')] if a.stages else list(range(1, 39))
    print('| # | Menu name | Stage | Object (kind.type): outline relative to it (left, top, right, bottom) |')
    print('|---|---|---|---|')
    for k in ks:
        stage = ms_stage(k)
        fg = foreground(stage)
        if not fg: continue
        folder = os.path.join(EXT, *asset_layout.stage(stage).split('/'))
        geo_path = os.path.join(folder, 'widescreen.json')
        geo = json.load(open(geo_path)) if os.path.exists(geo_path) else {}
        cells = []
        for kind, typ in fg:
            box = outline(a.exe, a.out, k, kind, typ)
            cells.append('%d.%d: %s' % (kind, typ, box if box else 'not seen'))
            if not box or a.dry_run: continue
            l, t, r, b = box
            t -= PAD; b += PAD
            h = min(b - t, 216)
            for side, x in (('left', l - OUT_W), ('right', r - OVERLAP)):
                name = 'widescreen_object_%d_%d_%s.png' % (kind, typ, side)
                path = os.path.join(folder, name)
                key = 'object_%d_%d_%s' % (kind, typ, side)
                if os.path.exists(path) and not is_placeholder(path):
                    print('kept (painted): %s' % os.path.relpath(path, ROOT), file=sys.stderr); continue
                os.makedirs(folder, exist_ok=True)
                make_piece(path, OUT_W + OVERLAP, h, '%d.%d %s' % (kind, typ, side[0].upper()))
                geo[key] = x; geo[key + '_y'] = t
        if not a.dry_run:
            json.dump(dict(sorted(geo.items())), open(geo_path, 'w'), indent=1); open(geo_path, 'a').write('\n')
        print('| %d | %s | 0x%02X | %s |' % (k, ms.MENU_NAMES[k - 1], stage, '; '.join(cells)), flush=True)


STAGE_BY_MENU = None
def ms_stage(k):
    """The stage number of menu entry k (the select screen's order)."""
    global STAGE_BY_MENU
    if STAGE_BY_MENU is None:
        STAGE_BY_MENU = [0x00, 0x17, 0x01, 0x02, 0x24, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0B, 0x0C, 0x0D,
                         0x0E, 0x0F, 0x10, 0x11, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x29, 0x0A, 0x1E,
                         0x20, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27, 0x28]
    return STAGE_BY_MENU[k - 1]


if __name__ == '__main__':
    main()
