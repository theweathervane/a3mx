#!/usr/bin/env python3
"""Measure the stage extensions: for every selectable stage, which part of each 48 x 288 edge strip
WIDE FORCED actually shows (the black the stage's art leaves at the camera limits), and optionally
(re)make the placeholder PNGs in assets/extensions/stages/<nn>_<name>/widescreen_<left|right>.png. Writes the table in docs/STAGE_EXTENSIONS.md.

The strips themselves (one per layer) and their placeholders: tools/measure_stage_layers.py. This tool
measures only the black at each edge as a whole.

A stage extension is a strip drawn behind the stage's art, joined to the fighters' layer at the camera
limits: the left one covers layer x 208 - 255, the right one 1024 - 1071, and 288 rows (the 272 of the
screen plus the 16 the camera rises when a fighter is in the air). Only WIDE FORCED shows it.

For each stage and side it runs a Free Battle in WIDE FORCED with both fighters pinned at that wall
(--poke), twice: held on the ground (camera y 0) and held high
(camera y at its top, 16). In each screenshot it counts the pure black columns at that edge, row by row,
and maps them onto the strip. Where the fight's overlay (life bars, portraits, gauges)
reaches the edge columns it hides what is behind it, so those rows can read narrower than they are.

Usage: python tools/measure_stage_extensions.py [--exe build/a3mx] [--work work] [-j 2]
                                                [--placeholders] [--masks DIR]
US program only (the addresses are ULUS-10062's).
"""
import argparse, concurrent.futures, os, shutil, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure_stages as ms
import asset_layout

ROOT = ms.ROOT
STRIP_W, CAM_RISE, SCREEN_W, SCREEN_H = 48, 16, 480, 272
STRIP_H = SCREEN_H + CAM_RISE
BATTLE = 0x092E1228
FIGHTERS_LAYER = 0x092E88A0          # +34 camera x, +38 camera y
Y_OFF = 4                            # fighter y (s16) is at fighter x + 4: 40 on the ground
PLACEHOLDER_DIR = os.path.join(ROOT, 'assets', 'extensions')     # + stages/<nn>_<name>/ (tools/asset_layout.py)


def run(exe, work, out, k, side, lifted):
    d = os.path.join(out, '%02d%s%s' % (k, side, 'u' if lifted else 'g'))
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    open(os.path.join(d, 'a3mx.cfg'), 'w').write('[screen setting]\nscreen_size=WIDE FORCED\n')
    inp, f = ms.script(k); p, e = f + 1500, f + 1900
    x = '3E8' if side == 'R' else '118'
    pokes = ['%d-%d:%08X=%s/2' % (p, e, a, x) for a in (ms.P1_X, ms.P2_X)]
    y = '1F4' if lifted else '28'                # 500 (far above the camera's reach) or 40 (the ground)
    pokes += ['%d-%d:%08X=%s/2' % (p, e, a + Y_OFF, y) for a in (ms.P1_X, ms.P2_X)]
    cmd = [exe, '--work', work, '--save-dir', d, '--log', os.path.join(d, 'log'), '--headless', '--fast',
           '--renderer', 'lazy', '--frames', str(e + 1), '--input', inp,
           '--dump', d, '--dump-from', str(e), '--dump-every', str(e), '--snap', '%d:%s' % (e, os.path.join(d, 'mem.bin'))]
    for q in pokes: cmd += ['--poke', q]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True)
    mem = open(os.path.join(d, 'mem.bin'), 'rb').read()
    h = lambda a: struct.unpack_from('<h', mem, a - ms.USER_LO)[0]
    png = [os.path.join(d, n) for n in os.listdir(d) if n.endswith('.png')][0]
    w, hgt, rows = ms.read_png_rgb(png)
    assert (w, hgt) == (SCREEN_W, SCREEN_H), (w, hgt)
    black = []                                   # per screen row: black columns from the edge (0 - 48)
    for r in rows:
        n = 0
        for i in range(STRIP_W):
            xx = i if side == 'L' else SCREEN_W - 1 - i
            if r[4 * xx] or r[4 * xx + 1] or r[4 * xx + 2]: break
            n += 1
        black.append(n)
    return {'stage': h(BATTLE), 'camx': h(FIGHTERS_LAYER + 34), 'camy': h(FIGHTERS_LAYER + 38),
            'left': h(FIGHTERS_LAYER + 68), 'right': h(FIGHTERS_LAYER + 70), 'black': black}


def strip_mask(side, ground, lifted):
    """Per strip row (0 = top, at the camera's highest): how many columns from the outer edge show black."""
    m = [0] * STRIP_H
    for res in (ground, lifted):
        top = CAM_RISE - res['camy']             # strip row shown on screen row 0
        for y, n in enumerate(res['black']): m[top + y] = max(m[top + y], n)
    return m


def summary(mask):
    rows = [y for y, n in enumerate(mask) if n]
    if not rows: return None
    return {'width': max(mask), 'top': rows[0], 'bottom': rows[-1], 'rows': len(rows),
            'pixels': sum(mask)}


def make_placeholder(path, stage, side):
    from PIL import Image, ImageDraw, ImageFont
    img = Image.new('RGBA', (STRIP_W, STRIP_H), (255, 0, 255, 255))
    d = ImageDraw.Draw(img)
    try: font = ImageFont.truetype('DejaVuSansMono-Bold.ttf', 30)
    except OSError:
        font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', 30)
    chars = list('0x%02X' % stage) + [None, side]   # None: a gap between the code and the side
    step = 36; total = step * len(chars) - 6
    y = (STRIP_H - total) // 2
    for ch in chars:
        if ch:
            l, t, r, b = d.textbbox((0, 0), ch, font=font)
            d.text(((STRIP_W - (r - l)) // 2 - l, y - t), ch, font=font, fill=(0, 255, 255, 255))
        y += step
    img.save(path, optimize=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stage_extensions'))
    ap.add_argument('-j', type=int, default=2)
    ap.add_argument('--placeholders', action='store_true',
                    help='no longer used: tools/measure_stage_layers.py writes the placeholders, one per layer')
    ap.add_argument('--masks', metavar='DIR', help='also write each strip\'s black area as a PNG (white = shown)')
    a = ap.parse_args()
    jobs = [(k, s, u) for k in range(1, 39) for s in 'LR' for u in (False, True)]
    with concurrent.futures.ThreadPoolExecutor(a.j) as pool:
        res = dict(zip(jobs, pool.map(lambda j: run(a.exe, a.work, a.out, *j), jobs)))
    if a.masks: os.makedirs(a.masks, exist_ok=True)
    print('| # | Menu name | Stage | Left file | Shown on the left | Right file | Shown on the right |')
    print('|---|---|---|---|---|---|---|')
    for k in range(1, 39):
        cells = []
        stage = res[(k, 'L', False)]['stage']
        for side in 'LR':
            g, u = res[(k, side, False)], res[(k, side, True)]
            lim = g['left'] if side == 'L' else g['right']
            if g['camx'] != lim or u['camx'] != lim or g['camy'] != 0 or u['camy'] != CAM_RISE:
                print('warning: %s %s: camera x %d / %d (limit %d), y %d / %d' % (
                    ms.MENU_NAMES[k - 1], side, g['camx'], u['camx'], lim, g['camy'], u['camy']), file=sys.stderr)
            mask = strip_mask(side, g, u); s = summary(mask)
            name = '%s/widescreen_%s.png' % (asset_layout.stage(stage).split('/', 1)[1], 'left' if side == 'L' else 'right')
            if a.placeholders:
                sys.exit('--placeholders: use tools/measure_stage_layers.py, which writes them per layer (STAGE_EXTENSIONS.md)')
                dst = os.path.join(PLACEHOLDER_DIR, 'stages', *name.split('/'))
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                make_placeholder(dst, stage, side)
            if a.masks:
                from PIL import Image
                im = Image.new('L', (STRIP_W, STRIP_H), 0)
                for y, n in enumerate(mask):
                    for i in range(n): im.putpixel((i if side == 'L' else STRIP_W - 1 - i, y), 255)
                im.save(os.path.join(a.masks, name.replace('/', '_')))
            cells += ['`%s`' % name, 'nothing (covered by the stage)' if not s else
                      '%d px wide, rows %d - %d' % (s['width'], s['top'], s['bottom'])]
        print('| %d | %s | 0x%02X | %s | %s | %s | %s |' % (k, ms.MENU_NAMES[k - 1], stage, *cells))


if __name__ == '__main__':
    main()
