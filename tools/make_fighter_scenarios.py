#!/usr/bin/env python3
"""Writes tests/scenarios/fighters_NN_A_B.txt: one parity scenario for each pair of characters, so the
parity suite reaches every character's own special moves and super combos (decomp plan D1).

Each scenario is Free Battle with player 2 a human (2P PRIORITY PLAYER), as super_finish: player 1
picks character A and player 2 character B on the select screen (the cursor moved from RYU), both
with the game's one-button moves (PLAYER CONFIG, docs/PLAYER_CONFIG.md) on all nine buttons:

    square SPECIAL1 M   triangle SPECIAL2 M   l SPECIAL3 M   cross SPECIAL4 M   circle SPECIAL5 M
    r SPECIAL6 M        select S.COMBO1 LV3   l2 S.COMBO2 LV3   r2 S.COMBO3 LV3

and Cheats and Unlocks keeping both fighters' health and gauges full (TIME INFINITE), so nobody is
knocked out. In the fight each button is pressed by player 1, then by player 2, twice through: at the
distance the fight starts at, then after both have walked in. A number past the end of a character's
list does nothing (the game skips it).

Usage: python tools/make_fighter_scenarios.py [--check]   (--check: fail if a file would change)
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from watch_scenario import GRIDS, MOVE_GAP

# Free Battle's grid, in reading order, without RANDOM: 37 characters. The last one has no partner and
# fights the first.
ORDER = [n for n, _ in sorted(GRIDS['free'].items(), key=lambda kv: (kv[1][1], kv[1][0])) if n != 'RANDOM']
BUTTONS = [('square', 'SPECIAL1 M'), ('triangle', 'SPECIAL2 M'), ('l', 'SPECIAL3 M'), ('cross', 'SPECIAL4 M'),
           ('circle', 'SPECIAL5 M'), ('r', 'SPECIAL6 M'), ('select', 'S.COMBO1 LV3'), ('l2', 'S.COMBO2 LV3'),
           ('r2', 'S.COMBO3 LV3')]
SLOT = 180          # frames for one button: player 1 presses at 0, player 2 at SLOT / 2
WALK = 60           # frames both walk in between the two passes
FIGHT = 3800        # the first press, before the select moves are added (a fight starts at 3300 - 3700)

# super_finish's menus and rows: player 2 joins at 2180 and picks at 2240, player 1 picks at 2300
BASE = ('1200=start;1206=;1500=start;1506=;1600=down;1606=;1640=down;1646=;1680=down;1686=;1760=cross;1766=;'
        '2100=cross;2106=;2180=p2:start;2186=p2:;2240=p2:cross;2246=p2:;2300=cross;2306=;2360=p2:cross;2366=p2:;'
        '2420=cross;2426=;2480=p2:cross;2486=p2:;2540=cross;2546=;2600=p2:cross;2606=p2:;2660=cross;2666=;'
        '2720=p2:cross;2726=p2:;2780=cross;2786=;2840=p2:cross;2846=p2:;2900=cross;2906=;2960=p2:cross;2966=p2:;'
        '3020=cross;3026=;3080=p2:cross;3086=p2:;3200=down;3200=p2:cross;3204=;3206=p2:;3284=cross;3290=')
# Where each player's cursor moves go (in BASE's frames): player 1's before its pick at 2300; player 2's
# after player 1's ISM at 2540 (player 2's box shows WAIT until then) and before its pick at 2600.
# Everything from there on is moved later by the moves' time.
P1_MOVES, P2_MOVES = 2230, 2570


def moves(name):
    col, row = GRIDS['free'][name]
    return ['right'] * col + ['down'] * row


def insert(ev, at, mv, pad):
    """MV's cursor moves for PAD at frame AT, with everything from AT on moved later; returns the shift"""
    if not mv: return 0
    shift = (len(mv) + 1) * MOVE_GAP
    ev[:] = [(t + shift if t >= at else t, what) for t, what in ev]
    for i, m in enumerate(mv): ev += [(at + i * MOVE_GAP, pad + m), (at + i * MOVE_GAP + 6, pad)]
    return shift


def scenario(n, a, b):
    ev = []
    for e in BASE.split(';'):
        t, _, what = e.partition('=')
        ev.append((int(t), what))
    s1 = insert(ev, P1_MOVES, moves(a), '')
    s2 = insert(ev, P2_MOVES + s1, moves(b), 'p2:')
    t = FIGHT + s1 + s2
    for walk in (False, True):
        if walk:
            ev += [(t, 'right'), (t, 'p2:left'), (t + WALK, ''), (t + WALK, 'p2:')]
            t += WALK + 30
        for btn, _ in BUTTONS:
            ev += [(t, btn), (t + 6, ''), (t + SLOT // 2, 'p2:' + btn), (t + SLOT // 2 + 6, 'p2:')]
            t += SLOT
    ev.sort(key=lambda e: e[0])
    frames = t + 60
    pretty = lambda s: s.replace('EHONDA', 'E. HONDA').replace('RMIKA', 'R. MIKA').replace('MBISON', 'M. BISON') \
        .replace('THAWK', 'T. HAWK').replace('EVILRYU', 'EVIL RYU').replace('FEILONG', 'FEI LONG') \
        .replace('CHUNLI', 'CHUN-LI').replace('DEEJAY', 'DEE JAY')
    lines = [
        '# Every special move and super combo of %s (player 1) and %s (player 2): Free Battle with 2P' % (pretty(a), pretty(b)),
        '# PRIORITY PLAYER (super_finish\'s menus), both with the one-button moves SPECIAL1 - 6 M and S.COMBO1 - 3',
        '# LV3 on their nine buttons, health and gauges kept full. Written by tools/make_fighter_scenarios.py.',
        'frames %d' % frames,
        'args --p2 human',
        'config [ex setting]',
        'config p2_priority=PLAYER',
        'config [cheats and unlocks]',
        'config p1_health=INFINITE', 'config p2_health=INFINITE',
        'config p1_super=INFINITE', 'config p2_super=INFINITE',
        'config [game option]',
        'config time=INFINITE',
    ]
    for p in (1, 2):
        lines.append('config [p%d config]' % p)
        lines += ['config %s=%s' % (btn, att) for btn, att in BUTTONS]
    lines.append('input ' + ';'.join('%d=%s' % e for e in ev))
    name = 'fighters_%02d_%s_%s.txt' % (n, a.lower(), b.lower())
    return name, '\n'.join(lines) + '\n'


def main():
    check = '--check' in sys.argv
    pairs = [ORDER[i:i + 2] for i in range(0, len(ORDER), 2)]
    if len(pairs[-1]) == 1: pairs[-1].append(ORDER[0])
    bad = 0
    for n, (a, b) in enumerate(pairs, 1):
        name, text = scenario(n, a, b)
        path = os.path.join(ROOT, 'tests', 'scenarios', name)
        old = open(path, encoding='utf-8').read() if os.path.exists(path) else None
        if old == text: continue
        if check: print('out of date: ' + path); bad = 1; continue
        with open(path, 'w', encoding='utf-8', newline='\n') as f: f.write(text)
        print('wrote ' + path)
    return bad


if __name__ == '__main__':
    sys.exit(main())
