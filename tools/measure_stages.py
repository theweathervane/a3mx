#!/usr/bin/env python3
"""Measure every selectable stage: its number, the camera limits, and how far its art reaches in
WIDE FORCED. Writes the table in docs/STAGES.md.

For each of the 38 stages it starts a Free Battle on that stage (headless, scripted buttons), then:
  - reads the stage number and the main layer's camera limits from memory (--snap);
  - runs again in WIDE FORCED with both fighters held at the left, then the right edge (--poke), so
    the camera sits at its limit, and counts the black columns at that edge of the screen: where the
    stage's art does not reach the wider view.

Usage: python tools/measure_stages.py [--exe build/a3mx] [--work work] [--out DIR] [-j 2]
US program only (the addresses below are ULUS-10062's).
"""
import argparse, concurrent.futures, os, shutil, struct, subprocess, sys, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
USER_LO = 0x08800000
STAGE_NUMBER = 0x092E1228       # s16: the stage being fought on
MAIN_LAYER = 0x092E88A0         # the fighters' layer: +34 camera x (screen left), +68 / +70 limits
P1_X, P2_X = 0x092E3EB2, 0x092E434A
MENU_NAMES = ['RYU 1', 'RYU 2', 'KEN', 'AKUMA 1', 'AKUMA 2', 'CHARLIE', 'CHUNLI', 'ADON', 'SODOM', 'GUY',
              'BIRDIE', 'ROSE', 'SAGAT', 'DAN', 'SAKURA', 'ROLENTO', 'DHALSIM', 'ZANGIEF', 'GEN', 'BALROG',
              'CAMMY', 'HONDA', 'BLANKA', 'MIKA', 'CODY', 'VEGA', 'KARIN 1', 'KARIN 2', 'BISON1', 'BISON2',
              'GUILE', 'FEILONG', 'DEEJAY', 'HAWK', 'BATTLE1', 'BATTLE2', 'BATTLE3', 'BATTLE4']


def script(k):
    """Buttons: MODE MENU > FREE BATTLE > ARCADE, seven confirms to reach STAGE, Down k times, go."""
    s = ['1200=start', '1206=', '1500=start', '1506=', '1600=down', '1606=', '1640=down', '1646=',
         '1680=down', '1686=', '1760=cross', '1766=', '2100=cross', '2106=']
    f = 2300
    for _ in range(7): s += ['%d=cross' % f, '%d=' % (f + 6)]; f += 120
    f = 3200
    for _ in range(k): s += ['%d=down' % f, '%d=' % (f + 4)]; f += 24
    f += 60; s += ['%d=cross' % f, '%d=' % (f + 6)]
    return ';'.join(s), f


def read_png_rgb(path):
    """Minimal PNG reader for the runtime's own 8-bit RGBA output (no filters but those zlib + PNG use)."""
    b = open(path, 'rb').read(); pos = 8; idat = b''; w = h = 0
    while pos < len(b):
        n = struct.unpack('>I', b[pos:pos + 4])[0]; tag = b[pos + 4:pos + 8]; data = b[pos + 8:pos + 8 + n]; pos += 12 + n
        if tag == b'IHDR': w, h = struct.unpack('>II', data[:8])
        elif tag == b'IDAT': idat += data
    raw = zlib.decompress(idat); bpp = 4; stride = w * bpp; rows = []; prev = bytearray(stride); i = 0
    for _ in range(h):
        ft = raw[i]; line = bytearray(raw[i + 1:i + 1 + stride]); i += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0; up = prev[x]; ul = prev[x - bpp] if x >= bpp else 0
            if ft == 1: line[x] = (line[x] + a) & 255
            elif ft == 2: line[x] = (line[x] + up) & 255
            elif ft == 3: line[x] = (line[x] + ((a + up) >> 1)) & 255
            elif ft == 4:
                p = a + up - ul; pa, pb, pc = abs(p - a), abs(p - up), abs(p - ul)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else up if pb <= pc else ul)) & 255
        rows.append(bytes(line)); prev = line
    return w, h, rows


def black_margin(png, side, y0=60, y1=230):
    """(widest, narrowest) run of pure black columns at that edge over the play area's rows."""
    w, h, rows = read_png_rgb(png); widest, narrowest = 0, 999
    for y in range(y0, min(y1, h)):
        r = rows[y]; n = 0
        xs = range(w) if side == 'L' else range(w - 1, -1, -1)
        for x in xs:
            if r[4 * x] == 0 and r[4 * x + 1] == 0 and r[4 * x + 2] == 0: n += 1
            else: break
        widest = max(widest, n); narrowest = min(narrowest, n)
    return widest, narrowest


def run(exe, work, out, k, side):
    d = os.path.join(out, '%02d%s' % (k, side)); shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    open(os.path.join(d, 'a3mx.cfg'), 'w').write('[screen setting]\nscreen_size=WIDE FORCED\n')
    inp, f = script(k); p, e = f + 1500, f + 1900
    x = '3E8' if side == 'R' else '118'            # 1000 / 280: pushed against that wall
    cmd = [exe, '--work', work, '--save-dir', d, '--log', os.path.join(d, 'log'), '--headless', '--fast',
           '--renderer', 'lazy', '--frames', str(e + 1), '--input', inp,
           '--poke', '%d:%08X=%s/2' % (p, P1_X, x), '--poke', '%d:%08X=%s/2' % (p, P2_X, x),
           '--dump', d, '--dump-from', str(e), '--dump-every', str(e), '--snap', '%d:%s' % (e, os.path.join(d, 'mem.bin'))]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True)
    mem = open(os.path.join(d, 'mem.bin'), 'rb').read()
    h = lambda a: struct.unpack_from('<h', mem, a - USER_LO)[0]
    png = [os.path.join(d, n) for n in os.listdir(d) if n.endswith('.png')][0]
    return {'stage': h(STAGE_NUMBER), 'left': h(MAIN_LAYER + 68), 'right': h(MAIN_LAYER + 70),
            'camera': h(MAIN_LAYER + 34), 'margin': black_margin(png, side)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stages'))
    ap.add_argument('-j', type=int, default=2)
    a = ap.parse_args()
    jobs = [(k, s) for k in range(1, 39) for s in 'LR']
    with concurrent.futures.ThreadPoolExecutor(a.j) as pool:
        res = dict(zip(jobs, pool.map(lambda j: run(a.exe, a.work, a.out, *j), jobs)))
    print('| # | Menu name | Stage | Archive | Left limit | Right limit | Width | WIDE FORCED black, left (widest / narrowest) | right (widest / narrowest) |')
    print('|---|---|---|---|---|---|---|---|---|')
    for k in range(1, 39):
        L, R = res[(k, 'L')], res[(k, 'R')]
        bad = '' if (L['camera'], R['camera']) == (L['left'], R['right']) else ' (camera did not reach the limit)'
        print('| %d | %s | 0x%02X | st%02x.pac | %d | %d | %d | %d / %d | %d / %d |%s' % (
            k, MENU_NAMES[k - 1], L['stage'], L['stage'], L['left'], L['right'], L['right'] - L['left'] + 384,
            L['margin'][0], L['margin'][1], R['margin'][0], R['margin'][1], bad))


if __name__ == '__main__':
    main()
