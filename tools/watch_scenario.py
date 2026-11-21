#!/usr/bin/env python3
"""Play a scenario in a window, in real time, to look at it (nothing is compared).

Usage:
  python tools/watch_scenario.py SCENARIO [--screen NORMAL|WIDE|NATIVE] [--fullscreen]
                                 [--character C] [--skip-to F] [--frames N]
                                 [--renderer opengl|soft] [--assets] [--exe build/a3mx.exe] [--work work]
  python tools/watch_scenario.py --list

SCENARIO is a file in tests/watch (the endings, the super finishes) or tests/scenarios (the parity
scenarios); the name without .txt is enough. Its inputs, pokes and config lines are used as the parity
tests use them; the game runs at its own speed in a window and closes after the scenario's frames.

  --list          the scenarios in tests/watch and tests/scenarios
  --screen S      SCREEN SIZE: NORMAL (4:3, the default here), WIDE (stretched) or NATIVE (WIDE NATIVE)
  --fullscreen    open full screen (the default is a window; F11 or Alt+Enter switch while it plays)
  --character C   in a scenario with a `select` line, pick C for player 1 instead of the scenario's own
                  choice: the cursor is moved there before the pick, and every input and poke after it
                  waits for the moves. C is a name without spaces or punctuation (KEN, CHUNLI, MBISON,
                  THAWK, ...; any case); --character list shows them, and docs/TESTS.md lists them
  --skip-to F     race through the scenario up to frame F, then play at the game's speed (the
                  scenario's `watch F` line by default; 0 to watch it all)
  --assets        load assets/ (the stage strips); the parity tests run without them

A scenario's `select FRAME [GRID] [NAME]` line says where the pick is made: GRID is `arcade` (the
default; Arcade, Kumite, the Dramatic modes, Variable, Final Battle) or `free` (Free Battle's select),
NAME the character it picks without --character (RYU when left out).

Example, a super finish's burst backdrop in WIDE NATIVE, with Ken:
  python tools/watch_scenario.py super_kind_7 --screen native --character ken
"""
import argparse, glob, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tests'))
import parity  # noqa: E402

DATE = 1767225600    # 2026-01-01: the date the game is given (--date), so every run plays alike
MOVE_GAP = 30       # frames between cursor moves on the select screen

# The two character selects: each character's box as (column, row), named as the screens show them
# without spaces or punctuation.
# Arcade's (0x08A26F60, 7 x 7) starts on RYU (0, 3); Free Battle's (7 x 6) on RYU (0, 0), both players.
GRIDS = {
    'arcade': {
        'RYU': (0, 3), 'EHONDA': (1, 3), 'CODY': (2, 3), 'KARIN': (3, 3), 'VEGA': (4, 3), 'BLANKA': (5, 3),
        'KEN': (6, 3),
        'DEEJAY': (0, 2), 'DAN': (1, 2), 'GUY': (2, 2), 'RMIKA': (3, 2), 'BIRDIE': (4, 2), 'AKUMA': (5, 2),
        'EVILRYU': (6, 2),
        'GUILE': (1, 1), 'CHARLIE': (2, 1), 'CHUNLI': (3, 1), 'SAGAT': (4, 1), 'FEILONG': (5, 1),
        'MBISON': (3, 0), 'RANDOM': (2, 0),
        'JULI': (0, 4), 'DHALSIM': (1, 4), 'SODOM': (2, 4), 'CAMMY': (3, 4), 'ADON': (4, 4), 'ZANGIEF': (5, 4),
        'JUNI': (6, 4),
        'YUN': (0, 5), 'THAWK': (1, 5), 'ROLENTO': (2, 5), 'SAKURA': (3, 5), 'GEN': (4, 5), 'BALROG': (5, 5),
        'INGRID': (6, 5),
        'EAGLE': (2, 6), 'ROSE': (3, 6), 'MAKI': (4, 6),
    },
    'free': {
        'RYU': (0, 0), 'EHONDA': (1, 0), 'BLANKA': (2, 0), 'CHUNLI': (3, 0), 'ZANGIEF': (4, 0),
        'DHALSIM': (5, 0), 'KEN': (6, 0),
        'AKUMA': (0, 1), 'CHARLIE': (1, 1), 'CAMMY': (2, 1), 'BALROG': (3, 1), 'VEGA': (4, 1), 'SAGAT': (5, 1),
        'MBISON': (6, 1),
        'ADON': (0, 2), 'GEN': (1, 2), 'BIRDIE': (2, 2), 'CODY': (3, 2), 'GUY': (4, 2), 'ROLENTO': (5, 2),
        'SODOM': (6, 2),
        'ROSE': (0, 3), 'SAKURA': (1, 3), 'KARIN': (2, 3), 'DAN': (3, 3), 'RMIKA': (4, 3), 'JULI': (5, 3),
        'JUNI': (6, 3),
        'EVILRYU': (1, 4), 'FEILONG': (2, 4), 'DEEJAY': (3, 4), 'THAWK': (4, 4), 'GUILE': (5, 4),
        'EAGLE': (1, 5), 'MAKI': (2, 5), 'RANDOM': (3, 5), 'YUN': (4, 5), 'INGRID': (5, 5),
    },
}


def moves_to(grid, name):
    """The cursor moves from RYU to NAME: Arcade's along RYU's row first, then up or down; Free Battle's
    right, then down. Every box on those paths is a character."""
    col, row = GRIDS[grid][name]
    if grid == 'arcade': return ['right'] * col + (['up'] * (3 - row) if row < 3 else ['down'] * (row - 3))
    return ['right'] * col + ['down'] * row


def find_character(name):
    key = re.sub(r'[\s._-]', '', name.upper())
    names = GRIDS['arcade']
    for n in names:
        if re.sub(r'[\s._-]', '', n) == key: return n
    for n in names:                                  # HONDA for EHONDA, MIKA for RMIKA, BISON, HAWK
        if re.sub(r'[\s._-]', '', n).endswith(key): return n
    raise SystemExit('no character %r: --character list shows the names' % name)


# Each character's super combo, as player 1 throws it facing right: QCF QCF / QCB QCB / HCB HCB, or a
# charge (back held, then forward, back, forward), then a punch
# (square) or a kick (cross). Found by trying each motion with each character. JUNI and ZANGIEF have none
# here yet: their tries end in a normal KO.
SUPERS = {
    'qcf P': ['RYU', 'KEN', 'AKUMA', 'BIRDIE', 'CODY', 'DAN', 'EAGLE', 'EVILRYU', 'FEILONG', 'GUY', 'INGRID',
              'KARIN', 'MAKI', 'RMIKA', 'SAGAT', 'SAKURA', 'SODOM', 'THAWK', 'YUN', 'JUNI', 'ZANGIEF', 'RANDOM'],
    'qcf K': ['ADON', 'CAMMY', 'JULI', 'ROLENTO'],
    'qcb P': ['DHALSIM', 'ROSE'],
    'hcb P': ['GEN'],
    'charge P': ['BALROG', 'BLANKA', 'CHARLIE', 'EHONDA', 'GUILE'],
    'charge K': ['CHUNLI', 'DEEJAY', 'MBISON', 'VEGA'],
}
SUPER_GAP = 180     # frames between tries
CLOSE_IN = ['BALROG', 'CHUNLI', 'FEILONG', 'GUILE', 'THAWK', 'VEGA']   # supers that only reach up close: walk in first


def super_motion(name):
    kind, btn = next(k for k, v in SUPERS.items() if name in v).split()
    btn = 'square' if btn == 'P' else 'cross'
    if kind == 'charge':
        steps = [(0, 'down+left' if name in CLOSE_IN else 'left')] + [(60 + 5 * i, d) for i, d in enumerate(['right', 'left', 'right'])]
        last = steps[-1][0]
    else:
        dirs = {'qcf': ['down', 'down+right', 'right'], 'qcb': ['down', 'down+left', 'left'],
                'hcb': ['right', 'down+right', 'down', 'down+left', 'left']}[kind] * 2
        steps = [(4 * i, d) for i, d in enumerate(dirs)]
        last = steps[-1][0]
    return steps + [(last + 4, steps[-1][1] + '+' + btn), (last + 12, '')]


def add_supers(sc, name):
    """A scenario's `supers FROM TO` line: player 1 throws NAME's super every SUPER_GAP frames between
    FROM and TO (walking in first for the ones that only reach up close)."""
    a, b = map(int, sc['supers'].split())
    events = [e for e in (sc['input'] or '').split(';') if e]
    for t in range(a, b, SUPER_GAP):
        walk = 50 if name in CLOSE_IN else 0
        if walk: events.append('%d=right' % t)
        events += ['%d=%s' % (t + walk + d, m) for d, m in super_motion(name)]
    events.sort(key=lambda e: int(e.split('=')[0]))
    sc['input'] = ';'.join(events)


def parse_select(val):
    """`FRAME [GRID] [NAME...]` -> (frame, grid, name)"""
    parts = val.split()
    grid = parts[1].lower() if len(parts) > 1 and parts[1].lower() in GRIDS else 'arcade'
    name = ' '.join(parts[2 if len(parts) > 1 and parts[1].lower() in GRIDS else 1:]) or 'RYU'
    return int(parts[0]), grid, find_character(name)


def pick_character(sc, name):
    """Move player 1's cursor from RYU to NAME before the pick at the scenario's `select` frame.
    Everything from that frame on (inputs, pokes, the frame count and the watch frame) is moved later
    by the time the moves take."""
    f, grid, default = parse_select(sc['select'])
    moves = moves_to(grid, find_character(name) if name else default)
    if not moves: return
    shift = len(moves) * MOVE_GAP + MOVE_GAP
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
        mm = re.match(r'^(\d+)-(\d+):(.*)$', x)
        if args and args[-1] == '--poke' and mm:
            x = '%d-%d:%s' % (later(int(mm.group(1))), later(int(mm.group(2))), mm.group(3))
        args.append(x)
    sc['args'] = args
    sc['frames'] += shift
    if 'watch' in sc: sc['watch'] = later(sc['watch'])


SCREEN = {'NORMAL': 'NORMAL', 'WIDE': 'WIDE', 'NATIVE': 'WIDE NATIVE'}


def screen_lines(sc, screen):
    """SCREEN SIZE for a run, unless the scenario's own config sets it"""
    if any(l.replace(' ', '').lower().startswith('screen_size=') for l in sc['config']): return []
    return ['[screen setting]', 'screen_size=' + SCREEN[screen.upper()]]


def scenario_path(name):
    if os.path.exists(name): return name
    name += '' if name.endswith('.txt') else '.txt'
    for d in ('watch', 'scenarios'):
        p = os.path.join(ROOT, 'tests', d, name)
        if os.path.exists(p): return p
    raise SystemExit('no scenario %r: --list shows them' % name)


def list_scenarios():
    for d in ('watch', 'scenarios'):
        print('tests/%s:' % d)
        for p in sorted(glob.glob(os.path.join(ROOT, 'tests', d, '*.txt'))):
            first = open(p, encoding='utf-8').readline().lstrip('# ').strip()
            print('  %-28s %s' % (os.path.basename(p)[:-4], first[:90]))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('scenario', nargs='?')
    ap.add_argument('--list', action='store_true', help='the scenarios there are')
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx.exe' if os.name == 'nt' else 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--screen', default='NORMAL', type=str.upper, choices=sorted(SCREEN),
                    help='SCREEN SIZE: NORMAL (default), WIDE or NATIVE (WIDE NATIVE)')
    ap.add_argument('--fullscreen', action='store_true', help='full screen (default: a window)')
    ap.add_argument('--renderer')
    ap.add_argument('--frames', type=int)
    ap.add_argument('--skip-to', type=int, help='run at full speed up to this frame, then at the game\'s speed '
                    '(default: the scenario\'s watch line; 0 to watch it all)')
    ap.add_argument('--assets', action='store_true')
    ap.add_argument('--character', help='player 1\'s character (scenarios with a `select` line); list to see them')
    ap.add_argument('--print-input', action='store_true', help=argparse.SUPPRESS)
    a = ap.parse_args()
    if (a.character or '').lower() == 'list':
        print('  '.join(sorted(GRIDS['arcade']))); return 0
    if a.list or not a.scenario:
        list_scenarios(); return 0

    sc = parity.read_scenario(scenario_path(a.scenario))
    if a.character and 'select' not in sc:
        raise SystemExit('this scenario has no `select` line: --character needs one')
    if 'supers' in sc:
        add_supers(sc, find_character(a.character) if a.character else parse_select(sc['select'])[2])
    if a.character: pick_character(sc, a.character)
    sc['args'] += ['--date', str(DATE)]
    config = sc['config'] + screen_lines(sc, a.screen)
    if a.print_input:                       # for running the same thing headless
        print(sc['frames']); print(sc['input']); print(' '.join(sc['args'])); print('|'.join(config)); return 0

    save = tempfile.mkdtemp(prefix='a3mx_watch_')
    if config:
        with open(os.path.join(save, 'a3mx.cfg'), 'w', encoding='utf-8') as f: f.write('\n'.join(config) + '\n')

    cmd = [a.exe, '--work', a.work, '--save-dir', save, '--frames', str(a.frames or sc['frames']),
           '--features', 'default,-shaders'] + sc['args']
    if not a.assets: cmd.append('--no-assets')
    cmd.append('--fullscreen' if a.fullscreen else '--windowed')
    skip = a.skip_to if a.skip_to is not None else sc.get('watch', 0)
    if skip: cmd += ['--fast-until', str(skip)]
    if a.renderer: cmd += ['--renderer', a.renderer]
    if sc['input']: cmd += ['--input', sc['input']]
    print(' '.join(cmd[:8]), '...', flush=True)
    return subprocess.call(cmd)


if __name__ == '__main__':
    sys.exit(main())
