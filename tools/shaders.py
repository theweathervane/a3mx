#!/usr/bin/env python3
"""The shaders SCREEN SETTING > FILTER draws with: libretro's slang-shaders, at the commit pinned in
tools/data/pins.json, into the tools folder (.tools/slang-shaders/, or under A3MX_TOOLS_DIR).

    python tools/shaders.py            download them if they aren't there (about 55 MB unpacked), then
                                       make the catalogue (below)
    python tools/shaders.py --check    exit 0 if they are there and the presets FILTER uses match
                                       their SHA-256, 1 if not (nothing is downloaded)
    python tools/shaders.py --catalogue   make the catalogue (again), from what is there
        [--game EXE] [--librashader FILE]   the game and the library to test the presets with, if they
                                       are not where they are looked for (build/, beside the game;
                                       the library where the game itself looks for it)

The catalogue (.tools/slang-shaders/a3mx_catalogue.tsv) is the list of every preset FILTER offers
besides its own eleven, by category (CRT, SCANLINES, HANDHELD, NTSC, EDGE SMOOTHING, ...). Each
candidate is loaded and run for a few frames by the game itself (a3mx --filter-test: a small window,
for some minutes): one that librashader can't read, or whose shaders don't build for OpenGL, is left
out. Never offered, so never tested: bezel/ (Mega Bezel: a layout and textures of its own), hdr/ (an
HDR display), test/, spec/, include/, stereoscopic-3d/, downsample/ and subframe-bfi/ (needs subframes,
not drawn yet). Those folders (but include/, which the others use, and the small subframe-bfi/) are not
even unpacked from the download, except by the git fallback. Without the game or the library no
catalogue is made, and FILTER offers its own eleven only.

Only asked for by the player (the launcher's Dependencies tab, or this command): the game runs
without them, and FILTER then shows the picture plain. Each shader keeps its own licence, in its own
files; nothing of them is in this repository.

The shader library itself (librashader) is not fetched here: see docs/DISPLAY.md, "FILTER".
Standard library only.
"""
import hashlib, io, os, re, shutil, subprocess, sys, time, zipfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fetch, pins

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.environ.get('A3MX_TOOLS_DIR') or os.path.join(ROOT, '.tools')
DEST = os.path.join(TOOLS, 'slang-shaders')
CATALOGUE = 'a3mx_catalogue.tsv'
SKIP = ('bezel', 'hdr', 'test', 'spec', 'include', 'stereoscopic-3d', 'downsample', 'subframe-bfi')
NOT_UNPACKED = ('bezel', 'hdr', 'test', 'spec', 'stereoscopic-3d', 'downsample')   # (include/ is used by the rest; subframe-bfi/ is small)


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
            if rel.split('/', 1)[0] in NOT_UNPACKED: continue                 # never offered by FILTER
            path = os.path.normpath(os.path.join(dst, *rel.split('/')))
            if not path.startswith(os.path.normpath(dst) + os.sep): continue     # never outside DST
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'wb') as o: o.write(z.read(m))


# ---------------------------------------------------------------- the catalogue
NICE = {   # top folder: FILTER's category. FILTER shows them in this order (others after, by name)
    'crt': 'CRT', 'scanlines': 'SCANLINES', 'handheld': 'HANDHELD', 'ntsc': 'NTSC', 'pal': 'PAL',
    'vhs': 'VHS', 'edge-smoothing': 'EDGE SMOOTHING', 'pixel-art-scaling': 'PIXEL ART',
    'interpolation': 'INTERPOLATION', 'anti-aliasing': 'ANTI-ALIASING', 'presets': 'PRESETS',
    'border': 'BORDER', 'blurs': 'BLUR', 'sharpen': 'SHARPEN', 'dithering': 'DITHERING',
    'deinterlacing': 'DEINTERLACING', 'denoisers': 'DENOISE', 'motionblur': 'MOTION BLUR',
    'motion-interpolation': 'MOTION BLUR', 'nes_raw_palette': 'NES PALETTE', 'reshade': 'RESHADE PORTS',
    'subframe-bfi': 'SUBFRAME', 'misc': 'MISC', 'gpu': 'GPU', 'cel': 'CEL', 'film': 'FILM',
    'deblur': 'DEBLUR', 'warp': 'WARP', 'quad': 'QUAD', 'anamorphic': 'ANAMORPHIC', 'linear': 'LINEAR',
    'procedural': 'PROCEDURAL', 'auto-box': 'AUTO BOX', '': 'MISC',
}
ORDER = list(dict.fromkeys(NICE.values()))
HEAVY_MS, HEAVY_PASSES = 3.0, 10       # heavy: a frame at 1904 x 1080 slower than this when tested, or this many passes


def preset_files(path, seen=None):
    """(the files PATH needs: its shaders, textures and the presets it #references, or None if one is
    missing; its number of passes)"""
    seen = seen if seen is not None else set()
    if path in seen: return [], 0
    seen.add(path)
    try: text = open(path, encoding='utf-8', errors='replace').read()
    except OSError: return None, 0
    base, need, passes = os.path.dirname(path), [], 0
    for line in text.splitlines():
        line = line.split('//', 1)[0].strip()
        m = re.match(r'#reference\s+"?([^"]+?)"?\s*$', line)
        if m:
            sub, n = preset_files(os.path.normpath(os.path.join(base, m.group(1))), seen)
            if sub is None: return None, 0
            need += sub; passes = max(passes, n); continue
        m = re.match(r'shaders\s*=\s*"?(\d+)', line)
        if m: passes = max(passes, int(m.group(1))); continue
        m = re.match(r'[A-Za-z_][A-Za-z0-9_]*\s*=\s*"?([^"]+\.(?:slang|png|jpg|jpeg|bmp|tga))"?\s*$', line, re.I)
        if m: need.append(os.path.normpath(os.path.join(base, m.group(1))))
    for f in need:
        if not os.path.isfile(f): return None, 0
    return need, passes


def clean(s):
    return re.sub(r'\s+', ' ', re.sub(r"[^A-Z0-9 .+\-()/'&!,]", ' ', s.upper().replace('_', ' '))).strip()


def display_name(rel):
    """FILTER's name for a preset: its file name in capitals (the game's fonts have no small letters),
    after the folder it is a variant of when the name doesn't say (crt/shaders/crt-yah/presets/
    base-medium.slangp: CRT-YAH BASE-MEDIUM)"""
    parts = rel.split('/')
    stem = clean(os.path.splitext(parts[-1])[0])
    for d in reversed(parts[1:-1]):
        if d.lower() in ('shaders', 'shader', 'presets', 'preset', 'slang', 'variants', 'extras'): continue
        if clean(d) not in stem: stem = clean(d) + ' ' + stem
        break
    return stem


def candidates(dst):
    """[(path under DST, top folder, passes)]: every preset outside SKIP whose files are all there"""
    out = []
    for top, dirs, files in os.walk(dst):
        rel_top = os.path.relpath(top, dst).replace('\\', '/')
        first = '' if rel_top == '.' else rel_top.split('/')[0]
        if first in SKIP: dirs[:] = []; continue
        dirs.sort()
        for f in sorted(files):
            if not f.endswith('.slangp'): continue
            rel = f if rel_top == '.' else rel_top + '/' + f
            need, passes = preset_files(os.path.join(dst, *rel.split('/')))
            if need is not None: out.append((rel, first, passes))
    return out


def find_game(arg):
    if arg: return os.path.abspath(arg) if os.path.isfile(arg) else None
    exe = 'a3mx.exe' if os.name == 'nt' else 'a3mx'
    for p in (os.path.join(ROOT, 'build', exe), os.path.join(ROOT, exe), os.path.join(os.path.dirname(TOOLS), exe)):
        if os.path.isfile(p): return p
    return None


def run_tests(game, lib, dst, todo):
    """Each of TODO (paths under DST) through the game's --filter-test: {path: (ok, build ms, frame ms)},
    or None if the game could not test. It writes a line as each is done; one that brings it down is
    counted as failed, and the game is started again for the rest."""
    res, work = {}, os.path.join(TOOLS, 'filter-test')
    os.makedirs(work, exist_ok=True)
    lst, out = os.path.join(work, 'list.txt'), os.path.join(work, 'results.tsv')
    left = list(todo)
    while left:
        with open(lst, 'w', encoding='utf-8', newline='\n') as o: o.write('\n'.join(left) + '\n')
        if os.path.exists(out): os.remove(out)
        cmd = [game, '--filter-test', lst, out, '--shaders', dst, '--log', os.path.join(work, 'a3mx.log')]
        if lib: cmd += ['--librashader', lib]
        print('  testing %d presets with %s (a small window opens for a while: leave it be)' % (len(left), game), flush=True)
        r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        done = set()
        if os.path.exists(out):
            for line in open(out, encoding='utf-8', errors='replace'):
                f = line.rstrip('\n').split('\t')
                if len(f) >= 2 and f[0] == '*':        # the game could not test at all
                    print('ERROR: %s' % f[1]); return None
                if len(f) < 2: continue
                res[f[0]] = (f[1] == 'ok', float(f[2]) if f[1] == 'ok' else 0, float(f[3]) if f[1] == 'ok' else 0)
                done.add(f[0])
        if not done:
            print('ERROR: %s --filter-test tested nothing (exit %d; its log: %s)' % (game, r.returncode, os.path.join(work, 'a3mx.log')))
            return None
        rest = [p for p in left if p not in done]
        if rest and r.returncode != 0:                 # it stopped on rest[0]: that one fails, then the others
            print('  the game stopped on %s: left out' % rest[0], flush=True)
            res[rest[0]] = (False, 0, 0)
            rest = rest[1:]
        left = rest
    return res


def catalogue(dst, game_arg=None, lib=None):
    """Write DST/a3mx_catalogue.tsv: 0, or 1 when it could not be made (FILTER then offers its own eleven)."""
    game = find_game(game_arg)
    if not game:
        print('  No FILTER catalogue yet: the game is not built (build it, then: python tools/shaders.py --catalogue).')
        print('  Until then FILTER offers its own eleven filters.')
        return 1
    cands = candidates(dst)
    print('==> The FILTER catalogue: %d candidate presets in %s' % (len(cands), dst), flush=True)
    t0 = time.time()
    res = run_tests(game, lib, dst, [c[0] for c in cands])
    if res is None: return 1
    rows, names = [], set()
    for rel, top, passes in cands:
        ok, build_ms, frame_ms = res.get(rel, (False, 0, 0))
        if not ok: continue
        cat = NICE.get(top, top.upper().replace('-', ' ').replace('_', ' '))
        name = display_name(rel)
        if (cat, name) in names:                       # the same file name in two subfolders: say which
            parts = rel.split('/')
            if len(parts) > 2: name = display_name(parts[-2]) + ' ' + name
        names.add((cat, name))
        heavy = frame_ms > HEAVY_MS or passes >= HEAVY_PASSES
        rows.append((cat, name, rel, 'heavy' if heavy else '', passes, build_ms, frame_ms))
    rank = lambda c: (ORDER.index(c) if c in ORDER else len(ORDER), c)
    rows.sort(key=lambda r: (rank(r[0]), r[1]))
    with open(os.path.join(dst, CATALOGUE), 'w', encoding='utf-8', newline='\n') as o:
        o.write('# A3MX: the presets SCREEN SETTING > FILTER offers, made by tools/shaders.py --catalogue on %s.\n' % time.strftime('%Y-%m-%d'))
        o.write('# Each was loaded and run by the game (a3mx --filter-test). heavy: a frame slower than %.1f ms at\n' % HEAVY_MS)
        o.write('# 1904 x 1080 on the machine that made this, or %d passes or more.\n' % HEAVY_PASSES)
        o.write('# Tab-separated: category, name, preset (under this folder), flags, passes, build ms, frame ms\n')
        for r in rows: o.write('%s\t%s\t%s\t%s\t%d\t%.0f\t%.2f\n' % r)
    per = {}
    for r in rows: per[r[0]] = per.get(r[0], 0) + 1
    print('  %d presets in %d categories (%d of %d left out: they failed), in %.0f s:' % (
          len(rows), len(per), len(cands) - len(rows), len(cands), time.time() - t0))
    print('  ' + ', '.join('%s %d' % kv for kv in sorted(per.items(), key=lambda kv: rank(kv[0]))))
    print('  in %s' % os.path.join(dst, CATALOGUE))
    return 0


def arg_after(name):
    a = sys.argv[1:]
    return a[a.index(name) + 1] if name in a and a.index(name) + 1 < len(a) else None


def main():
    pin = pins.source('slang-shaders')
    if '--check' in sys.argv[1:]:
        ok = verified(DEST, pin)
        print('slang-shaders: %s' % ('here, at %s (SHA-256 checked)' % pin['commit'][:12] if ok else 'not in %s' % DEST))
        return 0 if ok else 1
    if '--catalogue' in sys.argv[1:]:
        if not verified(DEST, pin):
            print('slang-shaders: not in %s (python tools/shaders.py downloads them)' % DEST)
            return 1
        return catalogue(DEST, arg_after('--game'), arg_after('--librashader'))
    if verified(DEST, pin):
        print('slang-shaders: already in %s (%s, SHA-256 checked)' % (DEST, pin['commit'][:12]))
        if not os.path.isfile(os.path.join(DEST, CATALOGUE)): catalogue(DEST, arg_after('--game'), arg_after('--librashader'))
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
    catalogue(DEST, arg_after('--game'), arg_after('--librashader'))   # (without it FILTER offers its own eleven)
    return 0


if __name__ == '__main__':
    sys.exit(main())
