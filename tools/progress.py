#!/usr/bin/env python3
"""How much of the game is decompiled: functions and instructions written by hand in src/core
(SFA_FUNC) against everything in the PSP program.

Usage: python tools/progress.py [--work work] [--build build] [--by-region] [--markdown] [--list] [--update-docs]

--update-docs rewrites the progress block (between the <!-- progress --> markers) in README.md and
docs/ROADMAP.md: run it with every change that decompiles functions. The block starts with the overall
bar: the share of the program's instructions that is hand-written C.

"Verified" means checked against the original by the last test run: called in a parity scenario
(build/parity/*/decomp/decomp_report.txt) or passed the difftest (build/difftest.log).
"""
import argparse, glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'recomp'))
from psp_recomp import load_decomp   # noqa: E402

# Rough areas of the program (docs/PSP_PROGRAM_MAP.md), refined as they are understood.
REGIONS = [
    (0x08804000, 0x088DE4A0, 'game (modes, menus, fighters, battle)'),
    (0x088DE4A0, 0x088E1EC4, 'wireless (ad-hoc) VS'),
    (0x088E1EC4, 0x089E347C, 'game (to be mapped)'),
    (0x089E347C, 0x089E6000, 'system: save data, graphics, music'),
    (0x089E6000, 0x089FC354, 'system: C library, sound, OS glue'),
]


DOCS = ['README.md', os.path.join('docs', 'ROADMAP.md')]
START, END = '<!-- progress:start -->', '<!-- progress:end -->'


OVERALL_WIDTH = 40   # blocks in the overall bar: 2.5% each


def bar(done, total, width=10):
    """[███████░░░] for 75%: a block for each full tenth (or full 1 / width), so the bar is full only
    when everything is."""
    full = done * width // total if total else 0
    return '[' + '\u2588' * full + '\u2591' * (width - full) + ']'


def progress_block(funcs, done, size):
    """The text between the markers: functions and instructions overall, then by area."""
    def row(label, d, t, unit):
        return '%-40s %s %6.2f%%  %s / %s %s' % (label, bar(d, t), 100.0 * d / t if t else 0, f'{d:,}', f'{t:,}', unit)
    fs = [f['addr'] for f in funcs]
    total_i = sum(size.values()); done_i = sum(size.get(x, 0) for x in done)
    # The overall figure is the share of the program's instructions: a long function counts for more
    # than a short one, as it does in the work. Its bar is wider (2.5% a block) so progress shows sooner.
    overall = 'Overall  %s  %.2f%% decompiled' % (bar(done_i, total_i, OVERALL_WIDTH), 100.0 * done_i / total_i if total_i else 0)
    lines = [overall, '',
             row('Functions', len(done), len(fs), 'functions'),
             row('Instructions', done_i, total_i, 'instructions'), '', 'By area (instructions):']
    for lo, hi, name in REGIONS:
        a = [x for x in fs if lo <= x < hi]
        lines.append(row('  ' + name, sum(size[x] for x in a if x in done), sum(size[x] for x in a), ''))
    return '```\n' + '\n'.join(l.rstrip() for l in lines) + '\n```'


def update_docs(block):
    for rel in DOCS:
        path = os.path.join(ROOT, rel)
        text = open(path, encoding='utf-8').read()
        i, j = text.find(START), text.find(END)
        if i < 0 or j < i:
            sys.exit('progress.py: %s has no %s ... %s block' % (rel, START, END))
        new = text[:i + len(START)] + '\n' + block + '\n' + text[j:]
        if new != text:
            open(path, 'w', encoding='utf-8', newline='\n').write(new)
        print('%s: %s' % (rel, 'updated' if new != text else 'already up to date'))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--by-region', action='store_true')
    ap.add_argument('--markdown', action='store_true')
    ap.add_argument('--build', default=os.path.join(ROOT, 'build'))
    ap.add_argument('--list', action='store_true', help='list every decompiled function and how it was verified')
    ap.add_argument('--update-docs', action='store_true', help='rewrite the progress bars in README.md and docs/ROADMAP.md')
    a = ap.parse_args()
    A = json.load(open(os.path.join(a.work, 'analysis.json')))
    funcs = A['functions']
    done = load_decomp(os.path.join(ROOT, 'src', 'core'))
    # instructions per function: up to the next function's start (the analysis' ranges overlap
    # where code is shared by fall-through)
    starts = sorted(f['addr'] for f in funcs); his = {f['addr']: f['hi'] for f in funcs}
    size = {x: (min(his[x], starts[i + 1]) if i + 1 < len(starts) else his[x]) // 4 - x // 4 for i, x in enumerate(starts)}
    total_i = sum(size.values()); done_i = sum(size.get(x, 0) for x in done)
    line = '%d / %d functions (%.2f%%), %d / %d instructions (%.2f%%)' % (
        len(done), len(funcs), 100.0 * len(done) / len(funcs), done_i, total_i, 100.0 * done_i / total_i)
    print(('**Decompiled:** ' if a.markdown else 'decompiled: ') + line)
    print(('**Overall:** ' if a.markdown else 'overall:    ') + bar(done_i, total_i, OVERALL_WIDTH) + ' %.2f%%' % (100.0 * done_i / total_i))
    print(('**Progress:** ' if a.markdown else 'progress:   ') + bar(len(done), len(funcs)) + ' functions, ' + bar(done_i, total_i) + ' instructions')
    if a.update_docs:
        update_docs(progress_block(funcs, done, size))
    ran, diffed = set(), set()
    for rep in glob.glob(os.path.join(a.build, 'parity', '*', 'decomp', 'decomp_report.txt')):
        for l in open(rep):
            f = l.split()
            if len(f) >= 2 and int(f[1]) > 0: ran.add(int(f[0], 16))
    log = os.path.join(a.build, 'difftest.log')
    if os.path.exists(log):
        diffed = {int(m, 16) for m in re.findall(r'difftest ok\s+([0-9A-F]{8})', open(log).read())}
    verified = (ran | diffed) & set(done)
    print(('**Verified:** ' if a.markdown else 'verified:   ') + '%d of %d decompiled functions (%d run by parity scenarios, %d by the difftest)'
          % (len(verified), len(done), len(ran & set(done)), len(diffed & set(done))))
    if a.list:
        for x in sorted(done):
            how = ', '.join(w for w, ok in (('scenario', x in ran), ('difftest', x in diffed)) if ok) or 'NOT VERIFIED'
            print('  %08X %-28s %-20s %s' % (x, done[x][1], how, os.path.relpath(done[x][0], ROOT)))
    if a.by_region:
        if a.markdown: print('\n| Area | Functions | Instructions |\n|---|---|---|')
        for lo, hi, name in REGIONS:
            fs = [f['addr'] for f in funcs if lo <= f['addr'] < hi]
            d = [x for x in fs if x in done]
            ti = sum(size[x] for x in fs); di = sum(size[x] for x in d)
            row = (name, len(d), len(fs), di, ti, 100.0 * di / ti if ti else 0)
            print(('| %s | %d / %d | %d / %d (%.2f%%) |' if a.markdown else '  %-40s %5d / %5d functions  %7d / %7d instructions (%.2f%%)') % row)


if __name__ == '__main__':
    main()
