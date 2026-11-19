#!/usr/bin/env python3
"""Play a test scenario in a window, in real time, to look at it (nothing is compared).

Usage:
  python tools/watch_scenario.py SCENARIO [--exe build/a3mx.exe] [--work work] [--wide]
                                 [--super-kind N] [--renderer opengl|soft] [--frames N] [--assets]

SCENARIO is a file in tests/scenarios (a name like super_finish is enough). Its inputs, pokes and
config lines are used as the parity tests use them; the game runs at its own speed in a window, and
closes after the scenario's frames (or --frames).

  --wide          WIDE FORCED on (as a `wide` scenario's second run)
  --super-kind N  the super finish's backdrop forced to kind N (BATTLE + 43, 0x092E1253) while it is
                  shown: 1 / 2 slanted lines, 3 vertical lines, 5 speed lines, 6 splash, 7 burst
                  (0, 9 and 10 are the same picture), 8 swirl, in frames --super-from - --super-to
                  (4380 - 4520, super_finish's finish). Use with super_finish.
  --assets        load assets/ (the stage strips); the parity tests run without them

Example, every super finish backdrop in WIDE FORCED, one after another:
  for k in 1 2 3 5 6 7 8; do python tools/watch_scenario.py super_finish --wide --super-kind $k; done
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tests'))
import parity  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('scenario')
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx.exe' if os.name == 'nt' else 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--wide', action='store_true')
    ap.add_argument('--super-kind', type=int)
    ap.add_argument('--super-from', type=int, default=4380, help='frames to force it in (super_finish: 4380 - 4520)')
    ap.add_argument('--super-to', type=int, default=4520)
    ap.add_argument('--renderer')
    ap.add_argument('--frames', type=int)
    ap.add_argument('--assets', action='store_true')
    a = ap.parse_args()

    path = a.scenario
    if not os.path.exists(path):
        path = os.path.join(ROOT, 'tests', 'scenarios', a.scenario + ('' if a.scenario.endswith('.txt') else '.txt'))
    sc = parity.read_scenario(path)

    save = tempfile.mkdtemp(prefix='a3mx_watch_')
    config = sc['config'] + (['[screen setting]', 'screen_size=WIDE FORCED'] if a.wide else [])
    if config:
        with open(os.path.join(save, 'a3mx.cfg'), 'w', encoding='utf-8') as f: f.write('\n'.join(config) + '\n')

    cmd = [a.exe, '--work', a.work, '--save-dir', save, '--frames', str(a.frames or sc['frames']),
           '--features', 'default,-shaders'] + sc['args']
    if not a.assets: cmd.append('--no-assets')
    if a.renderer: cmd += ['--renderer', a.renderer]
    if a.super_kind is not None:
        cmd += ['--poke', '%d-%d:092E1253=%X/1' % (a.super_from, a.super_to, a.super_kind)]
    if sc['input']: cmd += ['--input', sc['input']]
    print(' '.join(cmd[:8]), '...', flush=True)
    return subprocess.call(cmd)


if __name__ == '__main__':
    sys.exit(main())
