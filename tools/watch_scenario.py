#!/usr/bin/env python3
"""Play a test scenario in a window, in real time, to look at it (nothing is compared).

Usage:
  python tools/watch_scenario.py SCENARIO [--exe build/a3mx.exe] [--work work] [--wide]
                                 [--super-kind N] [--renderer opengl|soft] [--frames N] [--assets]

SCENARIO is a file in tests/scenarios (a name like super_finish is enough). Its inputs, pokes and
config lines are used as the parity tests use them; the game runs at its own speed in a window, and
closes after the scenario's frames (or --frames).

  --wide          WIDE NATIVE on (as a `wide` scenario's second run)
  --super-kind N  the super finish's backdrop forced to kind N (BATTLE + 43, 0x092E1253) while it is
                  shown: 1 / 2 slanted lines, 3 vertical lines, 5 speed lines, 6 splash, 7 burst
                  (0, 9 and 10 are the same picture), 8 swirl, in frames --super-from - --super-to
                  (3830 - 3990, super_finish's finish). Use with super_finish.
  --assets        load assets/ (the stage strips); the parity tests run without them
  --skip-to F     race through the scenario up to frame F, then play at the game's speed (with
                  --super-kind it skips to 3700, just before the super is thrown, by default)

Example, every super finish backdrop in WIDE NATIVE, one after another:
  for k in 1 2 3 5 6 7 8; do python tools/watch_scenario.py super_finish --wide --super-kind $k; done
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tests'))
import parity  # noqa: E402


def screen_lines(sc, wide):
    """SCREEN SIZE for a run: WIDE NATIVE for `wide`, else NORMAL (a new save is WIDE NATIVE since
    0.99.21), unless the scenario's own config sets it"""
    if any(l.replace(' ', '').lower().startswith('screen_size=') for l in sc['config']): return []
    return ['[screen setting]', 'screen_size=' + ('WIDE NATIVE' if wide else 'NORMAL')]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('scenario')
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx.exe' if os.name == 'nt' else 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--wide', action='store_true')
    ap.add_argument('--super-kind', type=int)
    ap.add_argument('--super-from', type=int, default=3830, help='frames to force it in (super_finish: 3830 - 3990)')
    ap.add_argument('--super-to', type=int, default=3990)
    ap.add_argument('--renderer')
    ap.add_argument('--frames', type=int)
    ap.add_argument('--skip-to', type=int, help='run at full speed up to this frame, then at the game\'s speed '
                    '(with --super-kind: 3700, just before the super; 0 to watch it all)')
    ap.add_argument('--assets', action='store_true')
    a = ap.parse_args()

    path = a.scenario
    if not os.path.exists(path):
        path = os.path.join(ROOT, 'tests', 'scenarios', a.scenario + ('' if a.scenario.endswith('.txt') else '.txt'))
    sc = parity.read_scenario(path)

    save = tempfile.mkdtemp(prefix='a3mx_watch_')
    config = sc['config'] + screen_lines(sc, a.wide)
    if config:
        with open(os.path.join(save, 'a3mx.cfg'), 'w', encoding='utf-8') as f: f.write('\n'.join(config) + '\n')

    cmd = [a.exe, '--work', a.work, '--save-dir', save, '--frames', str(a.frames or sc['frames']),
           '--features', 'default,-shaders'] + sc['args']
    if not a.assets: cmd.append('--no-assets')
    skip = a.skip_to if a.skip_to is not None else (a.super_from - 130 if a.super_kind is not None else 0)
    if skip: cmd += ['--fast-until', str(skip)]
    if a.renderer: cmd += ['--renderer', a.renderer]
    if a.super_kind is not None:
        cmd += ['--poke', '%d-%d:092E1253=%X/1' % (a.super_from, a.super_to, a.super_kind)]
    if sc['input']: cmd += ['--input', sc['input']]
    print(' '.join(cmd[:8]), '...', flush=True)
    return subprocess.call(cmd)


if __name__ == '__main__':
    sys.exit(main())
