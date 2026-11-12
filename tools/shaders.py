#!/usr/bin/env python3
"""The shaders SCREEN SETTING > FILTER draws with: libretro's slang-shaders, at the commit pinned in
tools/data/pins.json, into the tools folder (.tools/slang-shaders/, or under A3MX_TOOLS_DIR).

    python tools/shaders.py            download them if they aren't there (about 85 MB unpacked)
    python tools/shaders.py --check    exit 0 if they are there and the presets FILTER uses match
                                       their SHA-256, 1 if not (nothing is downloaded)

Only asked for by the player (the launcher's Dependencies tab, or this command): the game runs
without them, and FILTER then shows the picture plain. Each shader keeps its own licence, in its own
files; nothing of them is in this repository.

The shader library itself (librashader) is not fetched here: see docs/DISPLAY.md, "FILTER".
Standard library only.
"""
import hashlib, io, os, shutil, subprocess, sys, zipfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fetch, pins

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.environ.get('A3MX_TOOLS_DIR') or os.path.join(ROOT, '.tools')
DEST = os.path.join(TOOLS, 'slang-shaders')


def verified(dst, pin):
    """Every preset FILTER uses is there with the pinned SHA-256."""
    for rel, want in pin['files'].items():
        path = os.path.join(dst, *rel.split('/'))
        if not os.path.isfile(path) or pins.sha256_file(path) != want: return False
    return True


def download(dst, pin):
    url = '%s/archive/%s.zip' % (pin['repo'], pin['commit'])
    print('  downloading %s' % url, flush=True)
    data = fetch.fetch(url, timeout=600)
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        for m in z.infolist():
            rel = m.filename.split('/', 1)[1] if '/' in m.filename else ''
            if not rel or m.is_dir(): continue
            path = os.path.normpath(os.path.join(dst, *rel.split('/')))
            if not path.startswith(os.path.normpath(dst) + os.sep): continue     # never outside DST
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'wb') as o: o.write(z.read(m))


def main():
    pin = pins.source('slang-shaders')
    if '--check' in sys.argv[1:]:
        ok = verified(DEST, pin)
        print('slang-shaders: %s' % ('here, at %s (SHA-256 checked)' % pin['commit'][:12] if ok else 'not in %s' % DEST))
        return 0 if ok else 1
    if verified(DEST, pin):
        print('slang-shaders: already in %s (%s, SHA-256 checked)' % (DEST, pin['commit'][:12]))
        return 0
    print('==> The FILTER setting\'s shaders (%s at %s)' % (pin['repo'], pin['commit'][:12]), flush=True)
    shutil.rmtree(DEST, ignore_errors=True)
    os.makedirs(TOOLS, exist_ok=True)
    try: download(DEST, pin)
    except Exception as ex:                                  # (a network that only lets git through)
        print('  zip download failed (%s): trying git' % ex, flush=True)
        shutil.rmtree(DEST, ignore_errors=True)
        subprocess.check_call(['git', 'init', '-q', DEST])
        subprocess.check_call(['git', '-C', DEST, 'fetch', '-q', '--depth', '1', pin['repo'] + '.git', pin['commit']])
        subprocess.check_call(['git', '-C', DEST, 'checkout', '-q', 'FETCH_HEAD'])
    if not verified(DEST, pin):
        shutil.rmtree(DEST, ignore_errors=True)
        print('ERROR: what was downloaded does not match the SHA-256 in tools/data/pins.json, so it was removed.')
        return 1
    print('  in %s (SHA-256 checked). FILTER can use them from the next start of the game.' % DEST)
    return 0


if __name__ == '__main__':
    sys.exit(main())
