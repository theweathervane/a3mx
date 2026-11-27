#!/usr/bin/env python3
"""Apply a unified diff to a source tree, strictly: every hunk's context must match exactly (no fuzz,
no offsets beyond searching for the context), or nothing is written and it fails. Used for the
project's patches to its dependencies (cmake/patches/), so building needs neither git nor patch.

    python tools/apply_patch.py PATCH DIR       apply (again is fine: a patch already applied is left be)

Text before the first '--- ' line is a description, and is skipped. Standard library only.
"""
import os, sys


def parse(text):
    files, cur, hunk = [], None, None
    for line in text.split('\n'):
        if line.startswith('--- '):
            cur = {'path': None, 'hunks': []}; files.append(cur); hunk = None
        elif line.startswith('+++ ') and cur is not None:
            cur['path'] = line[4:].split('\t')[0].strip()
        elif line.startswith('@@') and cur is not None:
            hunk = {'old': [], 'new': []}; cur['hunks'].append(hunk)
        elif hunk is not None and line[:1] in (' ', '-', '+'):
            if line[0] in ' -': hunk['old'].append(line[1:])
            if line[0] in ' +': hunk['new'].append(line[1:])
        elif hunk is not None and line == '':
            continue
    return files


def apply(patch, root):
    files = parse(open(patch, encoding='utf-8').read().replace('\r\n', '\n'))
    out = {}
    for f in files:
        path = os.path.join(root, *f['path'].split('/'))
        raw = open(path, 'rb').read().decode('utf-8')
        crlf = '\r\n' in raw
        text = raw.replace('\r\n', '\n')
        for h in f['hunks']:
            old, new = '\n'.join(h['old']) + '\n', '\n'.join(h['new']) + '\n'
            if text.count(new) == 1 and text.count(old) == 0:
                continue                                   # (applied already)
            if text.count(old) != 1:
                print('apply_patch: %s: a hunk does not match (found %d times): %s' % (f['path'], text.count(old), patch))
                return 1
            text = text.replace(old, new, 1)
        out[path] = text.replace('\n', '\r\n') if crlf else text
    for path, text in out.items():
        with open(path, 'wb') as o: o.write(text.encode('utf-8'))
    print('apply_patch: %s applied to %s (%d files)' % (os.path.basename(patch), root, len(out)))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(2)
    sys.exit(apply(sys.argv[1], sys.argv[2]))
