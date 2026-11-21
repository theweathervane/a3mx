#!/usr/bin/env python3
"""Play a test scenario in a window, in real time, to look at it (nothing is compared).

Usage:
  python tools/watch_scenario.py SCENARIO [--exe build/a3mx.exe] [--work work] [--wide] [--fullscreen]
                                 [--super-kind N] [--renderer opengl|soft] [--frames N] [--assets]

SCENARIO is a file in tests/scenarios or tests/watch (a name like super_finish is enough). Its inputs,
pokes and config lines are used as the parity tests use them; the game runs at its own speed in a
window, and closes after the scenario's frames (or --frames).

  --fullscreen    open full screen (the default here is a window; F11 or Alt+Enter switch while it plays)
  --wide          WIDE NATIVE on (as a `wide` scenario's second run)
  --super-kind N  the super finish's backdrop forced to kind N (BATTLE + 43, 0x092E1253) while it is
                  shown: 1 / 2 slanted lines, 3 vertical lines, 5 speed lines, 6 splash, 7 burst
                  (0, 9 and 10 are the same picture), 8 swirl, in frames --super-from - --super-to
                  (3830 - 3990, super_finish's finish). Use with super_finish.
  --assets        load assets/ (the stage strips); the parity tests run without them
  --skip-to F     race through the scenario up to frame F, then play at the game's speed (with
                  --super-kind it skips to 3700, just before the super is thrown; a scenario's own
                  `watch F` line sets it otherwise; 0 to watch it all)
  --character C   in a scenario with a `select F` line (the endings in tests/watch), pick C on the
                  character select instead of RYU: the cursor is moved there before the pick at F, and
                  every input and poke after F waits for it. C is a name as the select screen shows it
                  (KEN, CHUN-LI, M.BISON, T.HAWK, ...; case and spaces don't matter); --character list
                  shows them all

Example, every super finish backdrop in WIDE NATIVE, one after another:
  for k in 1 2 3 5 6 7 8; do python tools/watch_scenario.py super_finish --wide --super-kind $k; done
"""
import argparse, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tests'))
import parity  # noqa: E402


# Arcade's character select (0x08A26F60, 7 x 7): each character's box as (column, row). The cursor starts
# on RYU (0, 3); the names are the ones the select screen shows.
GRID = {
    'RYU': (0, 3), 'E.HONDA': (1, 3), 'CODY': (2, 3), 'KARIN': (3, 3), 'VEGA': (4, 3), 'BLANKA': (5, 3),
    'KEN': (6, 3),
    'DEE JAY': (0, 2), 'DAN': (1, 2), 'GUY': (2, 2), 'R.MIKA': (3, 2), 'BIRDIE': (4, 2), 'AKUMA': (5, 2),
    'EVIL RYU': (6, 2),
    'GUILE': (1, 1), 'CHARLIE': (2, 1), 'CHUN-LI': (3, 1), 'SAGAT': (4, 1), 'FEI LONG': (5, 1),
    'M.BISON': (3, 0), 'RANDOM': (2, 0),
    'JULI': (0, 4), 'DHALSIM': (1, 4), 'SODOM': (2, 4), 'CAMMY': (3, 4), 'ADON': (4, 4), 'ZANGIEF': (5, 4),
    'JUNI': (6, 4),
    'YUN': (0, 5), 'T.HAWK': (1, 5), 'ROLENTO': (2, 5), 'SAKURA': (3, 5), 'GEN': (4, 5), 'BALROG': (5, 5),
    'INGRID': (6, 5),
    'EAGLE': (2, 6), 'ROSE': (3, 6), 'MAKI': (4, 6),
}
MOVE_GAP = 30           # frames between two cursor moves (each press held 6)


def find_character(name):
    key = re.sub(r'[\s._-]', '', name.upper())
    for n in GRID:
        if re.sub(r'[\s._-]', '', n) == key: return n
    for n in GRID:                                  # HONDA for E.HONDA, MIKA for R.MIKA, BISON, HAWK
        if re.sub(r'[\s._-]', '', n).endswith(key): return n
    raise SystemExit('no character %r: --character list shows the names' % name)


def pick_character(sc, name):
    """Move the cursor from RYU to NAME before the pick at the scenario's `select` frame: along RYU's row
    first, then up or down (every box on those paths is a character). Everything from that frame on
    (inputs and pokes) is moved later by the time the moves take."""
    if 'select' not in sc: raise SystemExit('this scenario has no `select` line: --character needs one')
    f = sc['select']
    col, row = GRID[find_character(name)]
    moves = ['right'] * col + (['up'] * (3 - row) if row < 3 else ['down'] * (row - 3))
    shift = len(moves) * MOVE_GAP + (MOVE_GAP if moves else 0)
    later = lambda t: t + shift if t >= f else t
    events = []
    for ev in (sc['input'] or '').split(';'):
        t, _, what = ev.partition('=')
        events.append((later(int(t)), what))
    events += [(f + i * MOVE_GAP + d, m if d == 0 else '') for i, m in enumerate(moves) for d in (0, 6)]
    events.sort(key=lambda e: e[0])
    sc['input'] = ';'.join('%d=%s' % e for e in events)
    args = []
    for x in sc['args']:
        m = re.match(r'^(\d+)-(\d+):(.*)$', x)
        if args and args[-1] == '--poke' and m:
            x = '%d-%d:%s' % (later(int(m.group(1))), later(int(m.group(2))), m.group(3))
        args.append(x)
    sc['args'] = args
    sc['frames'] += shift
    if 'watch' in sc: sc['watch'] = later(sc['watch'])


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
    ap.add_argument('--fullscreen', action='store_true', help='full screen (default: a window)')
    ap.add_argument('--super-kind', type=int)
    ap.add_argument('--super-from', type=int, default=3830, help='frames to force it in (super_finish: 3830 - 3990)')
    ap.add_argument('--super-to', type=int, default=3990)
    ap.add_argument('--renderer')
    ap.add_argument('--frames', type=int)
    ap.add_argument('--skip-to', type=int, help='run at full speed up to this frame, then at the game\'s speed '
                    '(with --super-kind: 3700, just before the super; 0 to watch it all)')
    ap.add_argument('--assets', action='store_true')
    ap.add_argument('--character', help='the character to pick (scenarios with a `select` line); list to see them')
    ap.add_argument('--print-input', action='store_true', help=argparse.SUPPRESS)
    a = ap.parse_args()
    if a.character and a.character.lower() == 'list':
        print('  '.join(sorted(GRID))); return 0

    path = a.scenario
    if not os.path.exists(path):
        name = a.scenario + ('' if a.scenario.endswith('.txt') else '.txt')
        path = os.path.join(ROOT, 'tests', 'scenarios', name)
        if not os.path.exists(path): path = os.path.join(ROOT, 'tests', 'watch', name)
    sc = parity.read_scenario(path)
    if a.character: pick_character(sc, a.character)
    if a.print_input:                       # for running the same thing headless
        print(sc['frames']); print(sc['input']); print(' '.join(sc['args'])); return 0

    save = tempfile.mkdtemp(prefix='a3mx_watch_')
    config = sc['config'] + screen_lines(sc, a.wide)
    if config:
        with open(os.path.join(save, 'a3mx.cfg'), 'w', encoding='utf-8') as f: f.write('\n'.join(config) + '\n')

    cmd = [a.exe, '--work', a.work, '--save-dir', save, '--frames', str(a.frames or sc['frames']),
           '--features', 'default,-shaders'] + sc['args']
    if not a.assets: cmd.append('--no-assets')
    cmd.append('--fullscreen' if a.fullscreen else '--windowed')
    skip = a.skip_to if a.skip_to is not None else (a.super_from - 130 if a.super_kind is not None
                                                    else sc.get('watch', 0))
    if skip: cmd += ['--fast-until', str(skip)]
    if a.renderer: cmd += ['--renderer', a.renderer]
    if a.super_kind is not None:
        cmd += ['--poke', '%d-%d:092E1253=%X/1' % (a.super_from, a.super_to, a.super_kind)]
    if sc['input']: cmd += ['--input', sc['input']]
    print(' '.join(cmd[:8]), '...', flush=True)
    return subprocess.call(cmd)


if __name__ == '__main__':
    sys.exit(main())
