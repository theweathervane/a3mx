#!/usr/bin/env python3
"""The project's pinned dependencies (tools/data/pins.json): check them, write their doc, download them.

    python tools/pins.py --check       CI: every pin agrees everywhere it is used, docs/DEPENDENCIES.md is
                                       up to date, and the vendored folders are as recorded
    python tools/pins.py --write       after changing a pin or vendored code: record the
                                       folders' and the exe's SHA-256 in pins.json and rewrite the doc
    python tools/pins.py --download    fetch every download and the widescreen patch and check their SHA-256
                                       (run it after changing a download in pins.json)

Where each pin is used:
  - tools/build.py and tools/extract_assets.py read pins.json, and so does the launcher's Dependencies tab
    (src/launcher/deps.cpp), which shows the list before anything is downloaded.
  - build.cmd (no Python yet) and cmake/SDL3.cmake (CMake) carry their own copy of the URL and SHA-256;
    --check makes sure those copies match pins.json.
Standard library only.
"""
import argparse, hashlib, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINS = os.path.join(ROOT, 'tools', 'data', 'pins.json')
DOC = os.path.join(ROOT, 'docs', 'DEPENDENCIES.md')
HEX64 = re.compile(r'^[0-9a-f]{64}$')


def load():
    with open(PINS, encoding='utf-8') as f: return json.load(f)


def by_id(pins, kind, id_):
    for p in pins[kind]:
        if p.get('id') == id_: return p
    raise KeyError('%s %s is not in tools/data/pins.json' % (kind, id_))


def download(id_):
    """(url, sha256) of a download, for the tools that fetch it."""
    p = by_id(load(), 'downloads', id_)
    return p['url'], p['sha256']


def source(id_):
    return by_id(load(), 'sources', id_)


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    return h.hexdigest()


def tree_sha256(rel):
    """A folder's contents as one SHA-256: each file's path and SHA-256, in order. Only the files git
    tracks, and text with its line endings as git stores them (LF), so a Windows checkout gives the
    same value."""
    try:
        out = subprocess.run(['git', 'ls-files', '-z', '--', rel], cwd=ROOT, capture_output=True, check=True).stdout
        files = sorted(f for f in out.decode('utf-8').split('\0') if f)
    except (OSError, subprocess.CalledProcessError):
        files = sorted(os.path.relpath(os.path.join(d, n), ROOT).replace(os.sep, '/')
                       for d, _, ns in os.walk(os.path.join(ROOT, rel)) for n in ns)
    h = hashlib.sha256()
    for f in files:
        if not os.path.isfile(os.path.join(ROOT, f)): continue      # listed by git but deleted: not part of it
        data = open(os.path.join(ROOT, f), 'rb').read()
        if b'\0' not in data: data = data.replace(b'\r\n', b'\n')
        h.update(f.encode('utf-8') + b'\0' + hashlib.sha256(data).hexdigest().encode() + b'\n')
    return h.hexdigest()


# ---------------------------------------------------------------- the doc
def render(pins):
    L = ['# Pinned dependencies', '',
         'Everything the build downloads and every source folder copied into this repository, each pinned to',
         'one exact file or commit. **This page is written from',
         '[tools/data/pins.json](../tools/data/pins.json) by `python tools/pins.py --write`; edit that, not this.**',
         'CI fails if the two disagree, or if any file that uses a pin has a different one (`tools/pins.py --check`).',
         '`--download` fetches each download and checks its SHA-256: run it after changing one.', '',
         'To check a download yourself: `certutil -hashfile FILE SHA256` (Windows), `sha256sum FILE` (Linux),',
         '`shasum -a 256 FILE` (macOS).', '',
         '## Downloads', '',
         'Each is checked against its SHA-256 before it is used; a mismatch stops the build.', '',
         '| What | Version | Used by | SHA-256 |', '|---|---|---|---|']
    for p in pins['downloads']:
        L.append('| [%s](%s) | %s | %s | `%s` |' % (p['name'], p['url'], p['version'], p['used_by'], p['sha256']))
    L += ['', '## Downloaded source', '',
          'Pinned to a commit; the files used are checked against their SHA-256 before anything runs.', '',
          '| What | Commit | File | SHA-256 | Used by |', '|---|---|---|---|---|']
    for p in pins['sources']:
        for f, h in sorted(p['files'].items()):
            L.append('| [%s](%s) | [`%s`](%s/tree/%s) | `%s` | `%s` | %s |' % (p['name'], p['repo'], p['commit'][:12], p['repo'], p['commit'], f, h, p['used_by']))
    L += ['', '## Vendored source (copied into this repository)', '',
          'Each folder says where it came from in its own `VENDORED` file. Its SHA-256 here covers every file in it',
          '(each path and file SHA-256, in order), so a change to vendored code has to be deliberate: update the',
          'folder, then `tools/pins.py --write`.', '',
          '| What | Folder | Upstream | Folder SHA-256 |', '|---|---|---|---|']
    for p in pins['vendored']:
        L.append('| %s | `%s/` | %s | `%s` |' % (p['name'], p['path'], p['upstream'], p['tree_sha256']))
    L += ['', '## Nothing compiled is committed', '',
          'The repository holds no program built in advance (create-launcher.exe was the last, removed in 0.60.0).',
          'The launcher that a release download carries is built by the release pipeline from the tagged source',
          '([BUILDING.md](BUILDING.md)); it is not signed, so Windows SmartScreen and macOS Gatekeeper may warn the',
          'first time it is opened.', '',
          '## Not pinned here', '',
          '- **Linux and macOS build tools** (CMake, Ninja, Clang, Python, SDL\'s system libraries): your',
          '  system\'s packages, from its package manager.',
          '- **Your disc** is checked by its own checksums (`tools/extract_assets.py`, [BUILDING.md](BUILDING.md)).', '']
    return '\n'.join(L)


# ---------------------------------------------------------------- checks
def check(pins):
    bad = []
    def need(cond, msg):
        if not cond: bad.append(msg)
    for p in pins['downloads']:
        need(HEX64.match(p['sha256']) and p['sha256'] != '0' * 64, 'downloads %s: no SHA-256 recorded (run --download, then put the one it prints in pins.json)' % p['id'])
    py = by_id(pins, 'downloads', 'python-embed')
    cmd = open(os.path.join(ROOT, 'build.cmd'), encoding='utf-8').read()
    need('set "PY_URL=%s"' % py['url'] in cmd, 'build.cmd: PY_URL is not %s' % py['url'])
    need('set "PY_SHA256=%s"' % py['sha256'] in cmd, 'build.cmd: PY_SHA256 is not %s' % py['sha256'])
    sdl = by_id(pins, 'downloads', 'sdl3')
    cm = open(os.path.join(ROOT, 'cmake', 'SDL3.cmake'), encoding='utf-8').read()
    need('URL %s' % sdl['url'] in cm, 'cmake/SDL3.cmake: URL is not %s' % sdl['url'])
    need('URL_HASH SHA256=%s' % sdl['sha256'] in cm, 'cmake/SDL3.cmake: URL_HASH is not SHA256=%s' % sdl['sha256'])
    need('find_package(SDL3 %s EXACT' % sdl['version'] in cm, 'cmake/SDL3.cmake: find_package(SDL3 ...) is not version %s (the copy tools/sdl3.py builds once)' % sdl['version'])
    for s in pins['sources']:
        need(re.match(r'^[0-9a-f]{40}$', s['commit']), 'sources %s: the commit must be a full 40-digit SHA-1' % s['id'])
        for f, h in s['files'].items(): need(HEX64.match(h), 'sources %s: %s has no SHA-256' % (s['id'], f))
    for v in pins['vendored']:
        need(v['tree_sha256'] == tree_sha256(v['path']), '%s/: its files have changed since pins.json was written (tools/pins.py --write, if that was meant)' % v['path'])
    doc = open(DOC, encoding='utf-8').read() if os.path.exists(DOC) else ''
    need(doc == render(pins), 'docs/DEPENDENCIES.md is out of date: python tools/pins.py --write')
    return bad


def write(pins):
    for v in pins['vendored']: v['tree_sha256'] = tree_sha256(v['path'])
    with open(PINS, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(pins, f, indent=2, ensure_ascii=False); f.write('\n')
    with open(DOC, 'w', encoding='utf-8', newline='\n') as f: f.write(render(pins))


def download_all(pins):
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import fetch
    bad = []
    for p in pins['downloads']:
        data = fetch.fetch(p['url'], timeout=600)
        h = hashlib.sha256(data).hexdigest()
        ok = h == p['sha256']
        print('%-17s %s  %s (%d bytes)' % (p['id'], 'ok      ' if ok else 'MISMATCH', h, len(data)), flush=True)
        if not ok: bad.append(p['id'])
    for s in pins['sources']:
        import io, zipfile
        data = fetch.fetch('%s/archive/%s.zip' % (s['repo'], s['commit']), timeout=120)
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            names = {n.split('/', 1)[1]: n for n in z.namelist() if '/' in n}
            for f, want in sorted(s['files'].items()):
                h = hashlib.sha256(z.read(names[f])).hexdigest() if f in names else 'missing'
                ok = h == want
                print('%-17s %s  %s %s' % (s['id'], 'ok      ' if ok else 'MISMATCH', h, f), flush=True)
                if not ok: bad.append('%s/%s' % (s['id'], f))
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--check', action='store_true'); g.add_argument('--write', action='store_true')
    g.add_argument('--download', action='store_true')
    a = ap.parse_args()
    pins = load()
    if a.write:
        write(pins); print('wrote tools/data/pins.json and docs/DEPENDENCIES.md'); return
    bad = check(pins) if a.check else download_all(pins)
    for b in bad: print('FAILED: ' + b)
    if bad: sys.exit(1)
    print('pins: all %s' % ('agree' if a.check else 'downloads match their SHA-256'))


if __name__ == '__main__':
    main()
