#!/usr/bin/env python3
"""The screenshots in the documentation (docs/images/): take them again, or check they're up to date.

    python tools/screenshots.py              take every one this system can (Windows: all of them)
    python tools/screenshots.py ID ...       only these (ids in tools/data/screenshots.json)
    python tools/screenshots.py --check      exit 1 if one is out of date, missing or not linked

Each screenshot is listed in tools/data/screenshots.json with what it shows and the source files it
depends on. Taking it records a SHA-256 of those files; --check recomputes it, so a change to the
launcher (or to the downloads in pins.json, which its Dependencies tab lists) shows the
screenshot is out of date. No git history needed, so it works in shallow clones too.

The rule (CONTRIBUTING.md): a pull request that changes what a screenshot shows takes it again in the
same pull request, and --check passes before every release.

The launcher takes its own screenshots (--screenshot FILE.bmp, --tab NAME for one of its tabs), from
build-launcher/ or a release's ready-built one. This turns its BMPs into PNGs.
Standard library only.
"""
import argparse, hashlib, json, os, platform, struct, subprocess, sys, tempfile, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIST = os.path.join(ROOT, 'tools', 'data', 'screenshots.json')
WINDOWS = os.name == 'nt'
EXE = '.exe' if WINDOWS else ''


# ---------------------------------------------------------------- what each screenshot depends on
def tracked(rel):
    """The files under rel that git tracks (all files if this isn't a git checkout)."""
    try:
        out = subprocess.run(['git', 'ls-files', '-z', '--', rel], cwd=ROOT, capture_output=True, check=True).stdout
        files = [f for f in out.decode('utf-8').split('\0') if f]
        if files: return sorted(files)
    except (OSError, subprocess.CalledProcessError):
        pass
    p = os.path.join(ROOT, rel)
    if os.path.isfile(p): return [rel]
    return sorted(os.path.relpath(os.path.join(d, f), ROOT).replace(os.sep, '/') for d, _, fs in os.walk(p) for f in fs)


def source_hash(sources):
    """One SHA-256 over the sources. Text is hashed with \\n line ends (a Windows checkout has \\r\\n).
    "FILE#KEY" hashes only that top-level key of a JSON file (pins.json#downloads: not the exe's own hash)."""
    h = hashlib.sha256()
    for src in sources:
        rel, _, key = src.partition('#')
        for f in tracked(rel):
            data = open(os.path.join(ROOT, f), 'rb').read()
            if key:
                data = json.dumps(json.loads(data.decode('utf-8'))[key], sort_keys=True).encode('utf-8')
            elif b'\0' not in data:
                data = data.replace(b'\r\n', b'\n')
            h.update(f.encode('utf-8') + b'\0' + (key.encode('utf-8') + b'\0' if key else b'') + data + b'\0')
    return h.hexdigest()


# ---------------------------------------------------------------- BMP to PNG
def read_bmp(path):
    """(width, height, rows of RGB bytes, top first) from an uncompressed 24- or 32-bit BMP."""
    d = open(path, 'rb').read()
    if d[:2] != b'BM': raise ValueError('%s is not a BMP' % path)
    off = struct.unpack_from('<I', d, 10)[0]
    hsize, w, h, _, bpp, comp = struct.unpack_from('<IiiHHI', d, 14)
    if bpp not in (24, 32) or comp not in (0, 3): raise ValueError('%s: %d-bit, compression %d not handled' % (path, bpp, comp))
    masks = (0x00FF0000, 0x0000FF00, 0x000000FF)
    if comp == 3:
        masks = struct.unpack_from('<III', d, 14 + hsize) if hsize == 40 else struct.unpack_from('<III', d, 54)
    shift = lambda m: (m & -m).bit_length() - 1
    stride = (w * bpp // 8 + 3) & ~3
    top_down = h < 0
    h = abs(h)
    rows = []
    for y in range(h):
        r = d[off + y * stride: off + y * stride + w * bpp // 8]
        if bpp == 24:
            px = bytearray(w * 3)
            px[0::3], px[1::3], px[2::3] = r[2::3], r[1::3], r[0::3]
        else:
            px = bytearray()
            for (v,) in struct.iter_unpack('<I', r):
                px += bytes(((v & m) >> shift(m)) & 0xFF for m in masks)
        rows.append(bytes(px))
    if not top_down: rows.reverse()
    return w, h, rows


def write_png(path, w, h, rows):
    def chunk(t, b): return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xFFFFFFFF)
    raw = b''.join(b'\0' + r for r in rows)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) \
        + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(png)


# ---------------------------------------------------------------- taking them
def launcher():
    for p in ('build-launcher/a3mx-launcher' + EXE, 'a3mx-launcher' + EXE,
              'A3MX Launcher.app/Contents/MacOS/a3mx-launcher'):
        if os.path.isfile(os.path.join(ROOT, p)): return os.path.join(ROOT, p)
    sys.exit('No launcher to take screenshots with: build it first (launcher.cmd / ./launcher.sh).')


def command(entry, bmp):
    """The program and arguments that save this screenshot to bmp. Relative paths only, run from the
    top of the checkout, so no personal folder shows in the picture."""
    take = entry['take']
    if take['program'] == 'launcher':
        # its own settings and an empty user folder, so the picture shows the defaults and no personal path
        return [launcher(), '--settings', 'work/screenshot.ini', '--user-dir', 'work/screenshot-user'] + take.get('args', []) + ['--screenshot', bmp, '--frames', '30']
    raise ValueError('unknown program %r' % take['program'])


def can_take(entry):
    return entry['take'].get('system', 'any') in ('any', platform.system())


def take(entry):
    with tempfile.TemporaryDirectory() as tmp:
        bmp = os.path.join(tmp, 'shot.bmp')
        env = dict(os.environ)
        if not WINDOWS and platform.system() != 'Darwin' and not env.get('DISPLAY') and not env.get('WAYLAND_DISPLAY'):
            env.update(SDL_VIDEODRIVER='offscreen', SDL_RENDER_DRIVER='software')     # no screen: draw offscreen
        os.makedirs(os.path.join(ROOT, 'work'), exist_ok=True)
        ini = os.path.join(ROOT, 'work', 'screenshot.ini')
        if os.path.exists(ini): os.remove(ini)                  # the launcher's defaults, not your settings
        cmd = command(entry, bmp)
        r = subprocess.run(cmd, cwd=ROOT, env=env, timeout=120)
        if r.returncode != 0 or not os.path.exists(bmp):
            sys.exit('%s: %s exited with %d and saved nothing' % (entry['id'], os.path.basename(cmd[0]), r.returncode))
        w, h, rows = read_bmp(bmp)
        out = os.path.join(ROOT, entry['image'])
        os.makedirs(os.path.dirname(out), exist_ok=True)
        write_png(out, w, h, rows)
        if os.path.exists(ini): os.remove(ini)
    entry['size'] = [w, h]
    entry['sources_sha256'] = source_hash(entry['sources'])
    print('%s: %s (%d x %d)' % (entry['id'], entry['image'], w, h))


# ---------------------------------------------------------------- checking them
def linked_images():
    """docs/images/... paths that any tracked Markdown file links to."""
    found = set()
    for f in tracked('.'):
        if not f.endswith('.md'): continue
        text = open(os.path.join(ROOT, f), encoding='utf-8', errors='replace').read()
        for part in text.split('images/')[1:]:
            name = part.split(')')[0].split('"')[0].split(' ')[0]
            found.add('docs/images/' + name)
    return found


def check(data):
    problems = []
    listed = {e['image'] for e in data['screenshots']} | set(data.get('artwork', []))   # artwork: drawn, not taken
    linked = linked_images()
    for e in data['screenshots']:
        if not os.path.exists(os.path.join(ROOT, e['image'])):
            problems.append('%s: %s is missing' % (e['id'], e['image']))
        elif e.get('sources_sha256') != source_hash(e['sources']):
            problems.append('%s: out of date (%s changed since it was taken); take it again: python tools/screenshots.py %s%s'
                            % (e['id'], ', '.join(e['sources']), e['id'],
                               '' if e['take'].get('system', 'any') == 'any' else ' (on %s)' % e['take']['system']))
        if e['image'] not in linked:
            problems.append('%s: no document links to %s' % (e['id'], e['image']))
    images = os.path.join(ROOT, 'docs', 'images')
    for f in sorted(os.listdir(images)) if os.path.isdir(images) else []:
        if 'docs/images/' + f not in listed:
            problems.append('docs/images/%s is not in tools/data/screenshots.json' % f)
    for p in problems: print(p)
    if problems: sys.exit(1)
    print('screenshots: all %d up to date and linked' % len(data['screenshots']))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('ids', nargs='*', help='only these screenshots')
    ap.add_argument('--check', action='store_true', help='exit 1 if a screenshot is out of date, missing or not linked')
    a = ap.parse_args()
    data = json.load(open(LIST, encoding='utf-8'))
    if a.check: return check(data)
    todo = [e for e in data['screenshots'] if not a.ids or e['id'] in a.ids]
    unknown = set(a.ids) - {e['id'] for e in data['screenshots']}
    if unknown: sys.exit('unknown screenshot id: %s' % ', '.join(sorted(unknown)))
    for e in todo:
        if can_take(e): take(e)
        else: print('%s: skipped (taken on %s)' % (e['id'], e['take']['system']))
    with open(LIST, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, indent=2)
        f.write('\n')


if __name__ == '__main__':
    main()
