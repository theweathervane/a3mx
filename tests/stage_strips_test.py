#!/usr/bin/env python3
"""WIDE FORCED's stage extension strips are drawn (src/extensions/stagestrips.c, docs/STAGE_EXTENSIONS.md).

A Free Battle on CHARLIE's stage (st03) in WIDE FORCED with both fighters pinned at the left wall, with
assets/ and --custom-assets (the parity tests run with --no-assets, so they can't see the strips).
Checks that:
  - the strips are loaded ("stage strips: stage 0x03: N strips" in the log);
  - the screen's left edge, black without them, is covered: no black in the 34 columns the stage's
    art leaves empty (rows 60 - 200);
  - layer 0's bright green filler columns (screen x 34 - 45) are painted over by its strip, drawn in
    front of the layer ("layer0_left_front");
  - without --custom-assets (the launcher's Custom Assets unchecked, the default) none is loaded.
0.50.0 was merged with the tile drawers' wrappers lost, so no strip was ever drawn and no test noticed.

Usage: stage_strips_test.py --exe build/a3mx --work work --assets assets/us [--out DIR]
"""
import argparse, os, re, shutil, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import measure_stages as ms

CHARLIE = 6                                   # its place in FREE BATTLE's stage row


def run(a, d, custom):
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    open(os.path.join(d, 'a3mx.cfg'), 'w').write('[screen setting]\nscreen_size=WIDE FORCED\n')
    inp, f = ms.script(CHARLIE)
    p, e = f + 1500, f + 1900
    cmd = [a.exe, '--work', a.work, '--assets', a.assets, '--save-dir', d, '--log', os.path.join(d, 'log'),
           '--headless', '--fast', '--renderer', 'soft', '--frames', str(e + 1), '--input', inp,
           '--dump', d, '--dump-from', str(e), '--dump-every', '1'] + (['--custom-assets'] if custom else [])
    for addr in (ms.P1_X, ms.P2_X): cmd += ['--poke', '%d-%d:%08X=118/2' % (p, e, addr)]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True)
    return open(os.path.join(d, 'log'), errors='replace').read(), os.path.join(d, 'frame_%05d.png' % e)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--assets', required=True)
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stage_strips_test'))
    a = ap.parse_args()
    from PIL import Image
    import concurrent.futures
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        on = pool.submit(run, a, os.path.join(a.out, 'on'), True)
        off = pool.submit(run, a, os.path.join(a.out, 'off'), False)
        (log, shot), (log_off, _) = on.result(), off.result()
    fails = []
    m = re.search(r'stage strips: stage 0x03: (\d+) strips', log)
    if not m or int(m.group(1)) == 0: fails.append('no strips loaded for stage 0x03')
    if 'stage strips: stage' in log_off: fails.append('strips loaded without --custom-assets')
    if not os.path.exists(shot): fails.append('no frame dumped')
    else:
        im = Image.open(shot).convert('RGB')
        blacks = sum(1 for y in range(60, 200) for x in range(34) if im.getpixel((x, y)) == (0, 0, 0))
        greens = sum(1 for y in range(60, 200) for x in range(34, 46)
                     if (lambda q: q[1] >= 100 and q[0] <= 60 and q[2] <= 60)(im.getpixel((x, y))))
        print('black pixels at the edge: %d, green filler pixels: %d' % (blacks, greens))
        if blacks > 20: fails.append('%d black pixels where the strips go' % blacks)
        if greens > 20: fails.append('%d green filler pixels not painted over' % greens)
    for x in fails: print('FAIL:', x)
    if not fails: print('stage strips: ok')
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
