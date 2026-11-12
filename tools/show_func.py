#!/usr/bin/env python3
"""Show a game function as translated, with who calls it and what it calls: the starting point for
decompiling it.

Usage: python tools/show_func.py ADDR [ADDR...] [--build build] [--work work] [--callers]

ADDR is hex (089F3430 or 0x089F3430). Reads build/generated/code_*.c (run a build first) and
work/analysis.json.
"""
import argparse, glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNC_RE = re.compile(r'^void (orig_)?f_([0-9A-F]{8})\(Cpu \*c\) \{\n(.*?)^\}', re.M | re.S)


def load(build):
    src = {}
    for fn in sorted(glob.glob(os.path.join(build, 'generated', 'code_*.c'))):
        for m in FUNC_RE.finditer(open(fn, encoding='utf-8').read()):
            src[int(m.group(2), 16)] = (bool(m.group(1)), m.group(3))
    if not src: sys.exit('show_func.py: no translated code in %s/generated: build first' % build)
    return src


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('addrs', nargs='+')
    ap.add_argument('--build', default=os.path.join(ROOT, 'build'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--callers', action='store_true', help='also print the callers\' code')
    a = ap.parse_args()
    src = load(a.build)
    A = json.load(open(os.path.join(a.work, 'analysis.json')))
    info = {f['addr']: f for f in A['functions']}
    call_re = re.compile(r'\b(f_[0-9A-F]{8}|hle_\w+)\(c\)')
    callers = {}
    for addr, (_, body) in src.items():
        for t in set(call_re.findall(body)):
            if t.startswith('f_'): callers.setdefault(int(t[2:], 16), []).append(addr)
    for s in a.addrs:
        addr = int(s, 16)
        if addr not in src: print('%08X: not a function start' % addr); continue
        orig, body = src[addr]; f = info.get(addr, {})
        print('=' * 100)
        print('%08X  range %08X-%08X  found by %s%s' % (addr, f.get('lo', 0), f.get('hi', 0), ','.join(f.get('why', [])),
                                                     '  [decompiled: this is orig_f_]' if orig else ''))
        print('calls:     ', ' '.join(sorted(set(call_re.findall(body)))) or '-')
        cs = sorted(callers.get(addr, []))
        print('called by: ', ' '.join('%08X' % x for x in cs[:40]) + (' ... (%d)' % len(cs) if len(cs) > 40 else '') or '-')
        print('-' * 100); print(body.rstrip())
        if a.callers:
            for c in cs[:5]:
                print('--- caller %08X' % c); print(src[c][1].rstrip())


if __name__ == '__main__':
    main()
