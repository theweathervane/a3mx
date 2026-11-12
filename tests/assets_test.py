#!/usr/bin/env python3
"""Loading from assets/ in the game (docs/ASSET_LOADING.md), on the arcade_fight scenario (Ryu
against Dan on Dan's stage, st0c) up to the fight:

  identity  every archive from an unedited assets/ folder: the same game, frame for frame, as from
            the disc (memory and picture hashes), with every archive open served from assets/
  texture   one texture of st0c (entry 7) repainted in its PNG (solid red, saved after the raw file):
            converted back to ABGR1555 (the PSP's 5551), its new pixels are where the old ones were in memory, the
            stage looks different once it is on screen, and nothing else in memory differs but the
            loaders' read buffer
  larger    Ryu's last fighter-data entry (pl00_u entry 2) made 8 KB longer: it loads whole, nothing
            else in memory moves (only the loader's file position differs), the picture is the same
            on every frame

Usage: assets_test.py --exe build/a3mx --work work --assets assets/us [--out DIR]
"""
import argparse, json, os, shutil, subprocess, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'tools'))
import pac

FRAMES, SNAP = 4400, 4390
USER_RAM = 0x08800000
CHUNK_BUFFER, CHUNK_BYTES = 0x08E76E20, 0x8000        # src/core/loader/loader.h
LOAD_REQUESTS, REQUEST_BYTES = 0x08E76088, 216 * 16
TEXTURE = ('st0c', 7)
LARGER = ('pl00_u', 2)
GROWTH = (b'A3MX-ASSET-GROWTH-TEST!!' * 400)[:8192]


def scenario_input(root):
    for line in open(os.path.join(root, 'tests', 'scenarios', 'arcade_fight.txt')):
        if line.startswith('input '): return line.split(' ', 1)[1].strip()
    raise SystemExit('arcade_fight.txt has no input line')


def start(a, out, name, assets):
    d = os.path.join(out, name); shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    cmd = [a.exe, '--work', a.work, '--save-dir', d, '--log', os.path.join(d, 'run.log'), '--headless', '--fast',
           '--renderer', 'soft', '--frames', str(FRAMES), '--hash-log', os.path.join(d, 'hashes.txt'),
           '--snap', '%d:%s' % (SNAP, os.path.join(d, 'ram.bin')), '--input', a.input]
    cmd += ['--assets', assets] if assets else ['--no-assets']
    return name, cmd, subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)


def run_all(a, out, runs):
    """Two at a time; {name: (hashes, ram, log)}"""
    done = {}
    for i in range(0, len(runs), 2):
        procs = [start(a, out, *r) for r in runs[i:i + 2]]
        for name, cmd, p in procs:
            d = os.path.join(out, name)
            if p.wait() != 0: raise SystemExit('FAIL: %s run failed (exit %d): %s\n  log: %s' % (name, p.returncode, ' '.join(cmd), os.path.join(d, 'run.log')))
            hashes = [l.split() for l in open(os.path.join(d, 'hashes.txt'))]
            if len(hashes) < FRAMES - 5: raise SystemExit('FAIL: %s stopped at frame %d' % (name, len(hashes)))
            done[name] = (hashes, open(os.path.join(d, 'ram.bin'), 'rb').read(), open(os.path.join(d, 'run.log'), encoding='utf-8', errors='replace').read())
    return done


def diff_ranges(x, y):
    out, step = [], 4096
    for o in range(0, min(len(x), len(y)), step):
        if x[o:o + step] == y[o:o + step]: continue
        for k in range(o, min(o + step, len(x))):
            if x[k] != y[k]:
                if out and k - out[-1][1] <= 64: out[-1][1] = k + 1
                else: out.append([k, k + 1])
    return out


def within(rng, lo, size):
    return USER_RAM + rng[0] >= lo and USER_RAM + rng[1] <= lo + size


def archive_dir(root, archive):
    """An archive's folder in an assets folder, where its index.json puts it (assets/LAYOUT.md)."""
    rel = json.load(open(os.path.join(root, 'index.json')))['archives'][archive]
    return os.path.join(root, *rel.split('/'))


def entry_folder(src, dst, archive):
    """A copy of one archive's folder in an assets folder of its own, with an index.json naming only
    it: every other archive then comes from the disc. Returns its manifest and its folder."""
    rel = json.load(open(os.path.join(src, 'index.json')))['archives'][archive]
    folder = os.path.join(dst, *rel.split('/'))
    shutil.copytree(archive_dir(src, archive), folder)
    os.makedirs(dst, exist_ok=True)
    json.dump({'archives': {archive: rel}}, open(os.path.join(dst, 'index.json'), 'w'))
    return json.load(open(os.path.join(folder, 'manifest.json'))), folder


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True); ap.add_argument('--work', required=True)
    ap.add_argument('--assets', required=True); ap.add_argument('--out', default='assets_test')
    a = ap.parse_args()
    a.input = scenario_input(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if not os.path.exists(os.path.join(a.assets, 'index.json')):
        print('no index.json in %s: run tools/extract_assets.py (again, if it was extracted before 0.37.0)' % a.assets); return 1
    t0 = time.time()
    out = os.path.abspath(a.out); shutil.rmtree(out, ignore_errors=True); os.makedirs(out)

    # the modded folders: only the archive being changed, so every other one comes from the disc
    tex_root, big_root = os.path.join(out, 'assets_texture'), os.path.join(out, 'assets_larger')
    m, tex_dir = entry_folder(a.assets, tex_root, TEXTURE[0])
    item = m['entries'][TEXTURE[1]]
    tex_path = os.path.join(tex_dir, *item['raw'].split('/'))
    old_tex = open(tex_path, 'rb').read()
    if old_tex[:4] != b'GBIX' or 'ABGR1555' not in item.get('format', ''): print('FAIL: %s is not an ABGR1555 texture (extracted before 0.5.0? run tools/extract_assets.py)' % tex_path); return 1
    _, _, _, w, h = pac.pvr_header(old_tex)
    pixels_at = 0x20                                       # GBIX and PVRT headers
    png_path = os.path.join(tex_dir, *item['png'].split('/'))
    pac.png(png_path, w, h, [bytes((255, 0, 0, 255)) * w] * h, 6)     # solid red
    t = os.path.getmtime(tex_path); os.utime(png_path, (t + 10, t + 10))
    new_tex = old_tex[:pixels_at] + b'\x1F\x80' * (w * h) + old_tex[pixels_at + 2 * w * h:]   # red in ABGR1555: 0x801F
    m, big_dir = entry_folder(a.assets, big_root, LARGER[0])
    big_path = os.path.join(big_dir, *m['entries'][LARGER[1]]['raw'].split('/'))
    old_big = open(big_path, 'rb').read()
    open(big_path, 'wb').write(old_big + GROWTH)

    r = run_all(a, out, [('disc', None), ('identity', a.assets), ('texture', tex_root), ('larger', big_root)])
    disc_h, disc_ram, _ = r['disc']
    bad = 0

    def fail(msg):
        nonlocal bad; bad += 1; print('FAIL:', msg)

    # identity
    h, ram, log = r['identity']
    first = next((x[0] for x, y in zip(disc_h, h) if x != y), None)
    if first is not None or len(h) != len(disc_h): fail('identity: differs from the disc run at frame %s' % first)
    if ram != disc_ram: fail('identity: memory at frame %d differs' % SNAP)
    served = [l for l in log.splitlines() if 'archive opens served from' in l]
    if not served or ' 0 archive opens' in served[-1] or not served[-1].rstrip().endswith(' 0 from the disc instead'):
        fail('identity: archives were not all served from assets/: %s' % (served[-1] if served else 'no report'))
    other = [l for l in log.splitlines() if 'assets: ' in l and 'archives with a folder in' not in l and 'archive opens served' not in l]
    if other: fail('identity: the log reports a difference or a fallback:\n  ' + '\n  '.join(other))

    # texture
    h, ram, log = r['texture']
    old_px, new_px = old_tex[pixels_at:], new_tex[pixels_at:]
    homes, k = [], disc_ram.find(old_px)
    while k >= 0: homes.append(k); k = disc_ram.find(old_px, k + 1)
    if not homes: fail('texture: the original pixels are not in memory at frame %d (scenario changed?)' % SNAP)
    for k in homes:
        if ram[k:k + len(new_px)] != new_px: fail('texture: at %08X the new pixels did not arrive' % (USER_RAM + k))
    if old_px in ram: fail('texture: the old pixels are still in memory')
    for rng in diff_ranges(disc_ram, ram):
        if within(rng, CHUNK_BUFFER, CHUNK_BYTES): continue
        if any(rng[0] >= k and rng[1] <= k + len(new_px) for k in homes): continue
        fail('texture: memory %08X-%08X differs, outside the texture and the read buffer' % (USER_RAM + rng[0], USER_RAM + rng[1]))
    changed = sum(1 for x, y in zip(disc_h, h) if x[2] != y[2])
    if not changed: fail('texture: the picture never changed')
    if 'entry %d (' % TEXTURE[1] not in log or 'entry %d from %s' % (TEXTURE[1], item['png']) not in log:
        fail('texture: the log does not say the entry came from its PNG')

    # larger
    h, ram, log = r['larger']
    k = disc_ram.find(old_big)
    if k < 0: fail('larger: the original entry is not in memory (scenario changed?)')
    elif ram[k:k + len(old_big) + len(GROWTH)] != old_big + GROWTH: fail('larger: the longer entry did not arrive whole at %08X' % (USER_RAM + k))
    for rng in diff_ranges(disc_ram, ram):
        if k >= 0 and rng[0] >= k + len(old_big) and rng[1] <= k + len(old_big) + len(GROWTH): continue
        if within(rng, LOAD_REQUESTS, REQUEST_BYTES): continue
        fail('larger: memory %08X-%08X differs' % (USER_RAM + rng[0], USER_RAM + rng[1]))
    pic = next((x[0] for x, y in zip(disc_h, h) if x[2] != y[2]), None)
    if pic is not None: fail('larger: the picture differs from frame %s' % pic)
    if '%d -> %d bytes' % (len(old_big), len(old_big) + len(GROWTH)) not in log: fail('larger: the log does not give the new size')

    if bad: print('assets test: %d failures (runs in %s)' % (bad, out)); return 1
    print('assets OK: identical over %d frames; texture from its PNG in %d places, on screen in %d frames; %d bytes longer entry, picture unchanged (%.0fs)'
          % (len(disc_h), len(homes), changed, len(GROWTH), time.time() - t0))
    shutil.rmtree(out, ignore_errors=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
