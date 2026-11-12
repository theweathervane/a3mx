#!/usr/bin/env python3
"""Compare what two builds of the game draw with the GPU renderer, frame by frame.

The parity tests run the software renderer, so a change to the GPU renderer (src/platform/ge_gl.c) is
not covered by ctest. This runs one scenario (tests/scenarios/NAME.txt) with both builds, headless,
dumps every Nth frame as PNG, and reports the frames that differ. It also checks that the game's state
(the hash log's first hash) is the same in both runs.

The builds need headless OpenGL: configure with -DA3MX_EGL=ON (Linux, Mesa; llvmpipe is fine).

    python tools/compare_gl_frames.py --old OLD/build-egl/a3mx --new build-egl/a3mx arcade_fight --every 2
"""
import argparse, filecmp, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def scenario(name):
    sc = {'frames': 3000, 'input': '', 'stick': '', 'args': []}
    for line in open(os.path.join(ROOT, 'tests', 'scenarios', name + '.txt'), encoding='utf-8'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        key, _, value = line.partition(' ')
        if key == 'frames':
            sc['frames'] = int(value)
        elif key in ('input', 'stick'):
            sc[key] = value
        elif key == 'args':
            sc['args'] = value.split()
    return sc


def run(exe, sc, out, a):
    os.makedirs(os.path.join(out, 'dump'))
    cmd = [exe, '--work', a.work, '--save-dir', out, '--headless', '--fast', '--scale', str(a.scale),
           '--frames', str(sc['frames']), '--no-assets', '--input', sc['input'], '--dump', os.path.join(out, 'dump'),
           '--dump-every', str(a.every), '--hash-log', os.path.join(out, 'hashes.txt'), '--log', os.path.join(out, 'log.txt')]
    if sc['stick']:
        cmd += ['--stick', sc['stick']]
    return subprocess.Popen(cmd + sc['args'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    p = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    p.add_argument('scenario')
    p.add_argument('--old', required=True, help='the build before the change')
    p.add_argument('--new', required=True, help='the build after it')
    p.add_argument('--every', type=int, default=3, help='dump every Nth frame (default 3)')
    p.add_argument('--scale', type=int, default=1)
    p.add_argument('--work', default=os.path.join(ROOT, 'work'))
    a = p.parse_args()
    sc = scenario(a.scenario)
    tmp = tempfile.mkdtemp(prefix='glcmp-')
    try:
        procs = [run(a.old, sc, os.path.join(tmp, 'old'), a), run(a.new, sc, os.path.join(tmp, 'new'), a)]
        for pr in procs:
            pr.wait()
        old, new = os.path.join(tmp, 'old', 'dump'), os.path.join(tmp, 'new', 'dump')
        names = sorted(os.listdir(old))
        differ = [n for n in names if not os.path.exists(os.path.join(new, n)) or not filecmp.cmp(os.path.join(old, n), os.path.join(new, n), shallow=False)]
        extra = sorted(set(os.listdir(new)) - set(names))
        state = [open(os.path.join(tmp, v, 'hashes.txt')).read().split('\n') for v in ('old', 'new')]
        state = [[l.split(' ')[:2] for l in s if l] for s in state]
        print(f'{a.scenario}: {len(names)} frames compared, {len(differ)} differ, {len(extra)} only in the new run; '
              f'game state {"the same" if state[0] == state[1] else "DIFFERENT"}')
        for n in differ[:10]:
            print('  differs:', n)
        return 0 if not differ and not extra and state[0] == state[1] else 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
