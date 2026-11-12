#!/usr/bin/env python3
"""Measure WIDE FORCED's stage edges layer by layer, and write each stage's strip placeholders and
widescreen.json (docs/STAGE_EXTENSIONS.md).

A stage has up to 8 layers (0x092E8800, 0xA0 bytes each; layer 1 is the fighters' layer), each scrolling
at its own rate, so the art of each one ends at its own place. At a camera limit, the 48 columns
WIDE FORCED adds at that side show black wherever no layer has art. Each black pixel belongs to one
layer's strip: the farthest layer with art in that screen row near the edge (the plane the gap
continues), or, in a row where no layer has art near the edge, the farthest layer drawn at all. A gap
is a pixel where a magenta probe drawn behind everything shows through, so black that something
draws (a layer's own black art, the overlay's band under the gauges) is not one.

For each selectable stage and side it runs a Free Battle in WIDE FORCED with both fighters pinned at
that wall (--poke), once on the ground (camera y 0) and once high (camera y 16), and dumps the frame
with every layer, then each layer on its own (A3MX_STRIP_MEASURE, src/extensions/stagestrips.c). From
a memory snapshot it takes each layer's scroll at that limit, so a layer's strip goes at layer x
(its scroll - 48) on the left and (its scroll + 384) on the right.

Writes, per stage folder under assets/extensions/stages/<nn>_<name>/:
  widescreen_left.png / widescreen_right.png               the fighters' layer (layer 1)
  widescreen_layer<n>_left.png / widescreen_layer<n>_right.png  other layers
  widescreen.json                                            "layer<n>_left" / "layer<n>_right": layer x
Each placeholder is 48 x 288, transparent except the pixels that layer has to fill, which are its
layer's colour with the stage number, side and layer written on them in a darker shade. A strip that has to fill nothing
(or one with fewer than MIN_PIXELS) is not written (and an old one of that name is removed). A file that is not a placeholder (any colour
but transparent, a layer colour or the text colour) is somebody's painting: it is kept, and its place
in widescreen.json too.

Usage: python tools/measure_stage_layers.py [--exe build/a3mx] [--work work] [-j 2] [--stages 1,6,12]
                                            [--dry-run]
US program only (the addresses are ULUS-10062's). Needs Pillow.
"""
import argparse, concurrent.futures, json, os, shutil, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure_stages as ms
import asset_layout

ROOT = ms.ROOT
STRIP_W, CAM_RISE, SCREEN_W, SCREEN_H = 48, 16, 480, 272
STRIP_H = SCREEN_H + CAM_RISE
VIEW_W = 384                         # the 4:3 view the camera limits apply to
LIB_H = 224                          # the 2D library's rows, stretched to the screen's 272
BATTLE = 0x092E1228
LAYERS, LAYER_SIZE, NLAYERS, FIGHTERS = 0x092E8800, 0xA0, 8, 1
FIGHTERS_LAYER = LAYERS + FIGHTERS * LAYER_SIZE      # +34 camera x, +38 camera y, +68 / +70 limits
Y_OFF = 4                            # fighter y (s16) is at fighter x + 4: 40 on the ground
NEAR = 96                            # columns inward from the strip in which a layer counts as continuing
MIN_PIXELS = 64                      # a layer's share smaller than this is noise (a fighter's moving pixel), not a strip
EXT = os.path.join(ROOT, 'assets', 'extensions')
COLOURS = [(255, 128, 0), (255, 0, 255), (0, 160, 255), (0, 210, 0),
           (255, 220, 0), (160, 0, 255), (255, 60, 60), (0, 255, 200)]
# Strips asked for by hand, where the stage's own art isn't black but still has to be painted over or
# continued (the maintainer, 2026-11-06). cover 'green': the layer's bright green filler pixels in the
# edge columns; the strip is drawn in front of its layer ("layer<n>_<side>_front": 1) so it hides them.
# cover 'continue': every edge pixel in the rows where the layer has art near the edge (its art just
# stops at the camera limit, with a farther layer behind it, so the probe finds no gap).
EXTRAS = {
    0x03: [dict(layer=0, sides='LR', cover='green', front=True)],    # CHARLIE: green columns at both edges
    0x21: [dict(layer=1, sides='R', cover='continue')],              # FEILONG: the tree and the floor stop at the right
}
def text_colour(c): return tuple(v * 2 // 5 for v in c)   # the label: a dark shade of the layer's colour


def windows(e):
    """Frames from e: every layer (dumped at e), then layer n alone in e + 10 + 10n ... + 19 (dumped at
    + 15), then no layer at all (the fighters and the overlay only: 'none'), then every layer with the
    probe behind them ('probe': magenta wherever nothing covers the screen)."""
    w = ['%d-%d:%X' % (e + 10 + 10 * n, e + 19 + 10 * n, 1 << n) for n in range(NLAYERS)]
    w.append('%d-%d:0' % (e + 10 + 10 * NLAYERS, e + 19 + 10 * NLAYERS))
    w.append('%d-%d:1FF' % (e + 20 + 10 * NLAYERS, e + 29 + 10 * NLAYERS))   # every layer and the probe
    dumps = {None: e, 'none': e + 15 + 10 * NLAYERS, 'probe': e + 25 + 10 * NLAYERS}
    dumps.update({n: e + 15 + 10 * n for n in range(NLAYERS)})
    return ','.join(w), dumps


def run(exe, work, out, k, side, lifted):
    from PIL import Image
    d = os.path.join(out, '%02d%s%s' % (k, side, 'u' if lifted else 'g'))
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    open(os.path.join(d, 'a3mx.cfg'), 'w').write('[screen setting]\nscreen_size=WIDE FORCED\n')
    inp, f = ms.script(k); p, e = f + 1500, f + 1900
    last = e + 30 + 10 * NLAYERS
    x = '3E8' if side == 'R' else '118'
    pokes = ['%d-%d:%08X=%s/2' % (p, last, a, x) for a in (ms.P1_X, ms.P2_X)]
    y = '1F4' if lifted else '28'                # 500 (far above the camera's reach) or 40 (the ground)
    pokes += ['%d-%d:%08X=%s/2' % (p, last, a + Y_OFF, y) for a in (ms.P1_X, ms.P2_X)]
    spec, dumps = windows(e)
    cmd = [exe, '--work', work, '--assets', os.path.join(ROOT, 'assets', 'us'), '--save-dir', d,
           '--log', os.path.join(d, 'log'), '--headless', '--fast', '--renderer', 'lazy',
           '--frames', str(last + 1), '--input', inp, '--dump', d, '--dump-from', str(e), '--dump-every', '1',
           '--snap', '%d:%s' % (e, os.path.join(d, 'mem.bin'))]
    for q in pokes: cmd += ['--poke', q]
    env = dict(os.environ, A3MX_STRIP_MEASURE=spec)
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, env=env)
    mem = open(os.path.join(d, 'mem.bin'), 'rb').read()
    h = lambda a: struct.unpack_from('<h', mem, a - ms.USER_LO)[0]
    b = lambda a: mem[a - ms.USER_LO]
    layers = {}
    for n in range(NLAYERS):
        a = LAYERS + n * LAYER_SIZE
        layers[n] = {'kind': b(a + 3), 'x': h(a + 18), 'y': h(a + 98)}
    depth = {}
    for line in open(os.path.join(d, 'log'), errors='replace'):
        if 'stage strips: layer' in line:
            w = line.split('stage strips: layer', 1)[1].split()
            depth[int(w[0])] = int(w[2])
    os.remove(os.path.join(d, 'mem.bin'))
    shots = {}
    for key, frame in dumps.items():
        path = os.path.join(d, 'frame_%05d.png' % frame)
        if os.path.exists(path): shots[key] = Image.open(path).convert('RGB')
    return {'stage': h(BATTLE), 'camx': h(FIGHTERS_LAYER + 34), 'camy': h(FIGHTERS_LAYER + 38),
            'left': h(FIGHTERS_LAYER + 68), 'right': h(FIGHTERS_LAYER + 70),
            'layers': layers, 'depth': depth, 'shots': shots}


def edge_columns(side):
    return range(0, STRIP_W) if side == 'L' else range(SCREEN_W - STRIP_W, SCREEN_W)


def near_columns(side):
    return range(STRIP_W, STRIP_W + NEAR) if side == 'L' else range(SCREEN_W - STRIP_W - NEAR, SCREEN_W - STRIP_W)


def black(px): return px == (0, 0, 0)
def magenta(px): return px[0] >= 240 and px[1] <= 16 and px[2] >= 240


def attribute(res, side):
    """{layer: set of (strip x, strip y)} for one run: each black edge pixel given to a layer."""
    allshot, none, probe = res['shots'].get(None), res['shots'].get('none'), res['shots'].get('probe')
    if allshot is None or none is None or probe is None: return {}
    drawn = [n for n in sorted(res['depth'], key=lambda n: res['depth'][n]) if n in res['shots']]   # farthest (smallest depth) first
    if not drawn: return {}
    owned = {}
    for y in range(SCREEN_H):
        # where the probe shows through (nothing covers it: not a layer's own black art, nor the overlay's)
        gaps = [x for x in edge_columns(side) if magenta(probe.getpixel((x, y)))]
        if not gaps: continue
        owner = None
        for n in drawn:                          # the farthest layer with art near the edge in this row
            shot = res['shots'][n]
            if any(shot.getpixel((x, y)) != none.getpixel((x, y)) for x in near_columns(side)): owner = n; break
        if owner is None: owner = drawn[0]
        # strip row: the 2D library's row (the screen's, stretched by 272 / 224) + the layer's y scroll
        sy = int((y + 0.5) * LIB_H / SCREEN_H) + res['layers'][owner]['y']
        if not 0 <= sy < STRIP_H: continue
        for x in gaps:
            sx = x if side == 'L' else x - (SCREEN_W - STRIP_W)
            owned.setdefault(owner, set()).add((sx, sy))
    return owned


def green(px): return px[1] >= 100 and px[0] <= 60 and px[2] <= 60


def extra_pixels(res, side, n, cover):
    """{(strip x, strip y)} for one run and one EXTRAS entry."""
    shot, none = res['shots'].get(n), res['shots'].get('none')
    if shot is None or none is None: return set()
    out = set()
    for y in range(SCREEN_H):
        if cover == 'green': xs = [x for x in edge_columns(side) if shot.getpixel((x, y)) != none.getpixel((x, y)) and green(shot.getpixel((x, y)))]
        elif any(shot.getpixel((x, y)) != none.getpixel((x, y)) for x in near_columns(side)): xs = list(edge_columns(side))
        else: xs = []
        sy = int((y + 0.5) * LIB_H / SCREEN_H) + res['layers'][n]['y']
        if not 0 <= sy < STRIP_H: continue
        for x in xs: out.add((x if side == 'L' else x - (SCREEN_W - STRIP_W), sy))
    return out


def is_placeholder(path):
    from PIL import Image
    ok = {c + (255,) for c in COLOURS} | {text_colour(c) + (255,) for c in COLOURS} | {(0, 0, 0, 255)}
    im = Image.open(path).convert('RGBA')
    # also the first placeholders (0.33.0): magenta with cyan text, smoothed between the two
    old = lambda p: p[3] == 255 and p[2] >= 250 and abs(p[0] + p[1] - 255) <= 4
    return all(px[3] == 0 or px in ok or old(px) for px in im.getdata())


def make_placeholder(path, pixels, colour, label):
    from PIL import Image, ImageDraw, ImageFont
    fill = Image.new('RGBA', (STRIP_W, STRIP_H), colour + (255,))
    text = Image.new('L', (STRIP_W, STRIP_H), 0)
    d = ImageDraw.Draw(text)
    try: font = ImageFont.truetype('DejaVuSansMono-Bold.ttf', 14)
    except OSError: font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', 14)
    rows = [y for _, y in pixels]
    y = (min(rows) + max(rows)) // 2 - 8 * len(label) + 2
    for ch in label:
        l, t, r, bt = d.textbbox((0, 0), ch, font=font)
        d.text(((STRIP_W - (r - l)) // 2 - l, y - t), ch, font=font, fill=255)
        y += 16
    out = Image.new('RGBA', (STRIP_W, STRIP_H), (0, 0, 0, 0))
    for (x, yy) in pixels:
        out.putpixel((x, yy), text_colour(colour) + (255,) if text.getpixel((x, yy)) else colour + (255,))
    out.save(path, optimize=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stage_layers'))
    ap.add_argument('-j', type=int, default=2)
    ap.add_argument('--stages', help='menu numbers (1-38), comma-separated; default all')
    ap.add_argument('--dry-run', action='store_true', help='measure and report only; write no files')
    a = ap.parse_args()
    ks = [int(s) for s in a.stages.split(',')] if a.stages else list(range(1, 39))
    jobs = [(k, s, u) for k in ks for s in 'LR' for u in (False, True)]
    with concurrent.futures.ThreadPoolExecutor(a.j) as pool:
        res = dict(zip(jobs, pool.map(lambda j: run(a.exe, a.work, a.out, *j), jobs)))
    print('| # | Menu name | Stage | Layers (depth) | Left: pixels by layer | Right: pixels by layer |')
    print('|---|---|---|---|---|---|')
    for k in ks:
        stage = res[(k, 'L', False)]['stage']
        folder = os.path.join(EXT, *asset_layout.stage(stage).split('/'))
        geo_path = os.path.join(folder, 'widescreen.json')
        geo = json.load(open(geo_path)) if os.path.exists(geo_path) else {}
        cells = []
        for side in 'LR':
            g, u = res[(k, side, False)], res[(k, side, True)]
            lim = g['left'] if side == 'L' else g['right']
            if g['camx'] != lim or u['camx'] != lim or g['camy'] != 0 or u['camy'] != CAM_RISE:
                print('warning: %s %s: camera x %d / %d (limit %d), y %d / %d' % (
                    ms.MENU_NAMES[k - 1], side, g['camx'], u['camx'], lim, g['camy'], u['camy']), file=sys.stderr)
            owned = {}
            for r in (g, u):
                for n, px in attribute(r, side).items(): owned.setdefault(n, set()).update(px)
            owned = {n: px for n, px in owned.items() if len(px) >= MIN_PIXELS}
            word = 'left' if side == 'L' else 'right'
            front = set()
            for x in EXTRAS.get(stage, []):
                if side not in x['sides']: continue
                for r in (g, u): owned.setdefault(x['layer'], set()).update(extra_pixels(r, side, x['layer'], x['cover']))
                if x.get('front'): front.add(x['layer'])
            for n in range(NLAYERS):
                name = 'widescreen_%s.png' % word if n == FIGHTERS else 'widescreen_layer%d_%s.png' % (n, word)
                path = os.path.join(folder, name)
                key = 'layer%d_%s' % (n, word)
                if os.path.exists(path) and not is_placeholder(path):
                    print('kept (painted): %s' % os.path.relpath(path, ROOT), file=sys.stderr)
                    continue
                if n in front and n in owned: geo[key + '_front'] = 1
                else: geo.pop(key + '_front', None)
                if n not in owned:
                    if not a.dry_run and os.path.exists(path): os.remove(path)
                    geo.pop(key, None)
                    continue
                scroll = g['layers'][n]['x']
                geo[key] = scroll - STRIP_W if side == 'L' else scroll + VIEW_W
                if not a.dry_run:
                    os.makedirs(folder, exist_ok=True)
                    make_placeholder(path, owned[n], COLOURS[n], ['0x%02X' % stage, side, 'L%d' % n])
            cells.append(', '.join('%d: %d' % (n, len(owned[n])) for n in sorted(owned)) or 'nothing')
        if not a.dry_run:
            if geo: json.dump(dict(sorted(geo.items())), open(geo_path, 'w'), indent=1); open(geo_path, 'a').write('\n')
            elif os.path.exists(geo_path): os.remove(geo_path)
        r0 = res[(k, 'L', False)]
        lay = ', '.join('%d (%d)' % (n, r0['depth'][n]) for n in sorted(r0['depth'], key=lambda n: r0['depth'][n]))
        print('| %d | %s | 0x%02X | %s | %s | %s |' % (k, ms.MENU_NAMES[k - 1], stage, lay, *cells), flush=True)


if __name__ == '__main__':
    main()
