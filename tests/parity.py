#!/usr/bin/env python3
"""Parity test: the decompiled functions must leave the game exactly as the originals do.

Runs one scenario twice, headless and deterministic (--fast, software renderer, fresh saves, every
archive from the disc: --no-assets):
  reference   every decompiled function replaced by its translated original (--reference)
  decomp      the hand-written code in src/core
and compares, frame by frame, a hash of the program's memory (code, data, bss) and of the frame
buffer. The first frame that differs fails the test, with how to look into it.

Usage: parity.py --exe build/a3mx --work work --scenario tests/scenarios/NAME.txt [--out DIR]

A scenario file:
    # comment
    frames 3000
    input 1200=start;1206=;1800=cross;1806=
    stick 1300=128,0;1310=128,128   (optional: analogue stick by frame, LX,LY 0-255)
    args --p2 cpu                (optional extra arguments for both runs)
    config [screen setting]      (optional: lines of the runs' a3mx.cfg, in order, written to
    config screen_size=WIDE FORCED    both save folders before they start)
"""
import argparse, os, shutil, subprocess, sys, time


def read_scenario(path):
    sc = {'frames': 600, 'input': None, 'args': [], 'config': []}
    for line in open(path, encoding='utf-8'):
        line = line.split('#', 1)[0].strip()
        if not line: continue
        key, _, val = line.partition(' ')
        if key == 'frames': sc['frames'] = int(val)
        elif key == 'input': sc['input'] = val.strip()
        elif key == 'args': sc['args'] += val.split()
        elif key == 'stick': sc['args'] += ['--stick', val.strip()]
        elif key == 'config': sc['config'].append(val.strip())
        else: raise SystemExit('%s: unknown line %r' % (path, line))
    return sc


def start(exe, work, sc, out, mode, extra):
    d = os.path.join(out, mode)
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    if sc['config']:
        with open(os.path.join(d, 'a3mx.cfg'), 'w', encoding='utf-8') as f: f.write('\n'.join(sc['config']) + '\n')
    cmd = [exe, '--work', work, '--save-dir', d, '--log', os.path.join(d, 'run.log'),
           '--headless', '--fast', '--renderer', 'soft', '--frames', str(sc['frames']),
           '--hash-log', os.path.join(d, 'hashes.txt'), '--no-assets',
           # the scenarios' menu scripts count SCREEN SETTING's rows without FILTER (0.60.0): the shaders
           # feature is off here (a scenario's own --features or --no-extensions, coming later, wins)
           '--features', 'default,-shaders'] + sc['args'] + extra
    if sc['input']: cmd += ['--input', sc['input']]
    return cmd, subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)


def load(path):
    rows = []
    for line in open(path):
        f, state, fb = line.split()
        rows.append((int(f), state, fb))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True); ap.add_argument('--work', required=True)
    ap.add_argument('--scenario', required=True); ap.add_argument('--out', default='parity')
    a = ap.parse_args()
    sc = read_scenario(a.scenario)
    name = os.path.splitext(os.path.basename(a.scenario))[0]
    out = os.path.join(a.out, name)
    t0 = time.time()
    report = os.path.join(out, 'decomp', 'decomp_report.txt')
    runs = {m: start(a.exe, a.work, sc, out, m, extra) for m, extra in
            (('reference', ['--reference']), ('decomp', ['--decomp-report', os.path.abspath(report)]))}
    for m, (cmd, p) in runs.items():
        if p.wait() != 0:
            print('%s run failed (exit %d): %s\n  log: %s' % (m, p.returncode, ' '.join(cmd), os.path.join(out, m, 'run.log')))
            return 1
    ref, dec = load(os.path.join(out, 'reference', 'hashes.txt')), load(os.path.join(out, 'decomp', 'hashes.txt'))
    if len(ref) < sc['frames'] - 5:
        print('reference run stopped early: %d of %d frames' % (len(ref), sc['frames'])); return 1
    for r, d in zip(ref, dec):
        if r != d:
            what = ' and '.join(w for w, x, y in (('memory', r[1], d[1]), ('picture', r[2], d[2])) if x != y)
            print('PARITY FAIL in %s at frame %d: %s differ.' % (name, r[0], what))
            print('  Bisect with --reference-only ADDR[,ADDR] (hex) on the decomp run, and compare screens with')
            print('  --dump DIR --dump-from %d --dump-every 1 on both runs.' % max(0, r[0] - 5))
            return 1
    if len(ref) != len(dec):
        print('PARITY FAIL in %s: %d frames against %d' % (name, len(dec), len(ref))); return 1
    ran = [l.split() for l in open(report)] if os.path.exists(report) else []
    used = sum(1 for r in ran if int(r[1]) > 0)
    print('parity OK: %s, %d frames identical (%.0fs); %d of %d decompiled functions ran in it'
          % (name, len(ref), time.time() - t0, used, len(ran)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
