#!/usr/bin/env python3
"""A stand-in for the generated funcs.h, for building without the disc (A3MX_DISC_FREE, CI).

The real funcs.h is written by psp_recomp.py from the PSP program and declares every translated
function. Without the disc there is no program, so this declares only the names the hand-written code
uses (f_XXXXXXXX, orig_f_XXXXXXXX, hle_*), found by scanning src/. That is enough to compile every
hand-written file and check it on every platform; nothing is linked into a game.

Hooked functions (tools/data/hooks.txt) also get their orig_f_ / hook_f_ / f_ names, as some files build those
names with macros.

Usage: stub_funcs.py SRC_DIR OUT_DIR [HOOKS_TXT]
"""
import os, re, sys

NAMES = re.compile(r'\b((?:orig_|hook_)?f_[0-9A-F]{8}|hle_[A-Za-z0-9_]+)\b')


def main():
    src, out = sys.argv[1], sys.argv[2]
    names = set()
    for root, _, files in os.walk(src):
        for f in files:
            if f.endswith(('.c', '.h', '.cpp')):
                names.update(NAMES.findall(open(os.path.join(root, f), encoding='utf-8', errors='replace').read()))
    hooks = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data', 'hooks.txt')
    if os.path.exists(hooks):
        for line in open(hooks):
            m = re.match(r'\s*([0-9A-Fa-f]{8})\b', line)
            if m: names.update(('f_' + m.group(1).upper(), 'orig_f_' + m.group(1).upper(), 'hook_f_' + m.group(1).upper()))
    os.makedirs(out, exist_ok=True)
    text = ('#pragma once\n/* stand-in for the generated funcs.h (tools/recomp/stub_funcs.py): disc-free build */\n'
            '#include "cpu.h"\nextern int rt_widescreen;\n' + ''.join('void %s(Cpu *c);\n' % n for n in sorted(names)))
    path = os.path.join(out, 'funcs.h')
    if not os.path.exists(path) or open(path).read() != text:
        open(path, 'w').write(text)
    print('stand-in funcs.h: %d names' % len(names))


if __name__ == '__main__':
    main()
