#!/usr/bin/env python3
"""Extract and verify the game from your own disc image.

Usage:  python tools/extract_assets.py [--region us|jp] [--iso PATH] [--work DIR] [--no-widescreen] [--verify-only]

  --region us   A3MX, ULUS-10062 (the default, and the one that builds today)
  --region jp   Street Fighter Zero 3 Double Upper, ULJM-05082 (extracted and verified; not buildable yet)

Without --iso, every *.iso in imports/psp/ is checked and the one for the region is used, whatever its
file name. The disc and its program are checked against the known SHA-1s below; an unknown disc stops
here, because the analysis, the hooks and the decompiled code are all written for one exact program.

Writes (all git-ignored):
  work/iso/...                 the disc's files                         (work-jp/ for --region jp)
  work/boot.elf                the game program (BOOT.BIN, unencrypted)
  work/boot.widescreen.json    the SFA3MAX 16:9 patch as an overlay, switched at run time
  work/imports.json            the OS functions the program calls
  work/analysis.json           functions, switch tables, code pointers
  work/analysis.image.bin      the relocated program image the runtime loads
  work/disc.json               which disc this is, and its checksums
  assets/<region>/music/*.at3  the music tracks, as on the disc (decoding to WAV / OGG comes later)
  assets/<region>/<folder>/    each .pac archive unpacked into a folder placed by what it holds
                               (characters/00_ryu/fighter/, stages/0c_dan/, screens/title/ ...;
                               tools/asset_layout.py, assets/LAYOUT.md): raw entries, PNG textures and
                               palettes, TXT text, in a subfolder per kind, and a manifest.json
                               (tools/pac.py packs it back; docs/PAC_FORMAT.md)
  assets/<region>/index.json   which folder holds which archive: what the game reads
Edits in folders from before 0.37.0 (assets/<region>/pac/<archive>/) are carried over into the new
folders, and the old pac/ folder is renamed pac.before-0.37/ for you to delete.

CMake then translates the program (tools/recomp/psp_recomp.py) as part of the build.
Needs Python 3.9+ only.
"""
import argparse, glob, hashlib, json, os, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, 'tools', 'recomp')

# Every disc the project knows. The ISO checksum is informative (dumps can differ in padding);
# the BOOT.BIN checksum is the one that must match.
DISCS = {
    'us': {'serial': 'ULUS-10062', 'version': '1.00', 'title': 'A3MX', 'data_dir': 'usa',
           'iso_sha1': '321ff2143b4c3e8aa24efcf52ffe577c9739f57b',
           'boot_sha1': '3fd0abf7cc65742233e1b7787997fa3e20152d98', 'buildable': True},
    'jp': {'serial': 'ULJM-05082', 'version': '1.01', 'title': 'Street Fighter Zero 3 Double Upper', 'data_dir': 'jpn',
           'iso_sha1': '86b75c0e299400722d5e4e1f7077730da0fbac11',
           'boot_sha1': 'd16de5a2bcd610a4841a15427b3716d33a4089bc', 'buildable': False},
}
# Known but not handled here yet: the European disc, A3MX ULES-00235, ISO sha1
# 6562c25bf07bc81ebcf7375105ec5bcce7daedc3, BOOT.BIN sha1 dc789772c15b2c9e1daa41ef7cfab1b5a65f512e
# (the launcher recognises it and shows its flag; src/launcher/disc_check.cpp).
SECTOR = 2048


class Progress:
    """An ASCII progress bar in the style of the roadmap's: [████████░░░░░░░░░░░░]  41%  128 / 313 archives.

    In a terminal, or under the launcher (A3MX_PROGRESS=1, which shows a carriage return as "rewrite
    this line"), the bar is redrawn in place as it fills; anywhere else (a log file, CI) a line is
    printed at each tenth, so the output stays short."""
    WIDTH = 20

    def __init__(self, label, total, unit=''):
        self.label, self.total, self.unit = label, max(total, 1), unit
        self.live = sys.stdout.isatty() or os.environ.get('A3MX_PROGRESS') == '1'
        enc = sys.stdout.encoding or 'ascii'
        try: '\u2588\u2591'.encode(enc); self.full, self.empty = '\u2588', '\u2591'
        except (UnicodeEncodeError, LookupError): self.full, self.empty = '#', '-'
        self.shown = -1
        self.update(0)

    def _count(self, n):
        if self.unit == 'MB': return '%d / %d MB' % (n >> 20, self.total >> 20)
        return '%s / %s%s' % (f'{n:,}', f'{self.total:,}', ' ' + self.unit if self.unit else '')

    def update(self, done):
        done = min(done, self.total)
        pct = done * 100 // self.total
        step = pct if self.live else pct // 10 * 10
        if step == self.shown: return
        self.shown = step
        fill = done * self.WIDTH // self.total
        line = '  %-26s [%s%s] %3d%%  %s' % (self.label, self.full * fill, self.empty * (self.WIDTH - fill), pct, self._count(done))
        if self.live: sys.stdout.write('\r' + line + ('\n' if done >= self.total else ''))
        else: sys.stdout.write(line + '\n')
        sys.stdout.flush()

    def finish(self):
        self.update(self.total)


def sha1_file(path, label=None):
    h = hashlib.sha1()
    bar = Progress(label, os.path.getsize(path), 'MB') if label else None
    done = 0
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk); done += len(chunk)
            if bar: bar.update(done)
    if bar: bar.finish()
    return h.hexdigest()


class Iso:
    """A plain ISO 9660 image, as a UMD dump is."""
    def __init__(self, path):
        self.f = open(path, 'rb')
        pvd = self.sector(16)
        if pvd[1:6] != b'CD001': raise SystemExit('extract_assets.py: %s is not an ISO 9660 disc image' % path)
        root = pvd[156:190]
        self.files = []                                  # (path, lba, size)
        self._walk(int.from_bytes(root[2:6], 'little'), int.from_bytes(root[10:14], 'little'), '')

    def sector(self, n, count=1):
        self.f.seek(n * SECTOR); return self.f.read(SECTOR * count)

    def _walk(self, lba, size, prefix):
        data = self.sector(lba, (size + SECTOR - 1) // SECTOR); pos = 0
        while pos < size:
            n = data[pos]
            if n == 0: pos = (pos // SECTOR + 1) * SECTOR; continue   # records do not cross sectors
            r = data[pos:pos + n]; pos += n
            name = r[33:33 + r[32]]
            if name in (b'\0', b'\1'): continue
            name = name.decode('ascii').split(';')[0]
            at, length = int.from_bytes(r[2:6], 'little'), int.from_bytes(r[10:14], 'little')
            if r[25] & 2: self._walk(at, length, prefix + name + '/')
            else: self.files.append((prefix + name, at, length))

    def read(self, rel):
        for p, at, length in self.files:
            if p == rel:
                self.f.seek(at * SECTOR); return self.f.read(length)
        raise KeyError(rel)

    def extract(self, out):
        bar = Progress("Copying the disc's files", sum(f[2] for f in self.files), 'MB'); done = 0
        for p, at, length in self.files:
            done += length; bar.update(done)
            dst = os.path.join(out, *p.split('/'))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            if os.path.exists(dst) and os.path.getsize(dst) == length: continue   # already there
            with open(dst, 'wb') as o:
                self.f.seek(at * SECTOR); left = length
                while left:
                    chunk = self.f.read(min(left, 1 << 20)); o.write(chunk); left -= len(chunk)
        bar.finish()
        return len(self.files)


def find_iso(region):
    """The ISO in imports/psp/ whose program is this region's, whatever it is called."""
    want = DISCS[region]['boot_sha1']
    seen = []
    for path in sorted(glob.glob(os.path.join(ROOT, 'imports', 'psp', '*.iso'))):
        try: boot = Iso(path).read('PSP_GAME/SYSDIR/BOOT.BIN')
        except (SystemExit, KeyError): continue
        sha = hashlib.sha1(boot).hexdigest(); seen.append((os.path.basename(path), sha))
        if sha == want: return path
    msg = 'extract_assets.py: no %s disc (%s) in imports/psp/.' % (DISCS[region]['title'], DISCS[region]['serial'])
    if seen: msg += ' Found: ' + ', '.join('%s (program %s)' % s for s in seen)
    raise SystemExit(msg + ' Copy your ISO there, or pass --iso.')


def run(*args):
    print('>', ' '.join(os.path.relpath(a, ROOT) if os.path.isabs(a) else a for a in args)); sys.stdout.flush()
    subprocess.check_call([sys.executable, '-I'] + list(args), stderr=subprocess.STDOUT)


# --- the widescreen patch ------------------------------------------------------------------------
# Pinned to one commit, and its patcher is run only if its SHA-256 is the one recorded (tools/data/pins.json):
# a change upstream can't change what runs here until the pin is updated.
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import pins
WS = pins.source('widescreen')
WS_REPO, WS_COMMIT = WS['repo'], WS['commit']
WS_ZIP = '%s/archive/%s.zip' % (WS_REPO, WS_COMMIT)


def widescreen_verified(dst):
    """True if every pinned file of the patch in DST has its recorded SHA-256."""
    for name, want in WS['files'].items():
        path = os.path.join(dst, name)
        if not os.path.isfile(path) or pins.sha256_file(path) != want: return False
    return True


def fetch_widescreen(dst):
    """Download the patch's pinned commit as a zip (no git needed) into DST."""
    import io, zipfile
    import fetch                          # any Python on the PATH, even one without certificates (#52)
    data = fetch.fetch(WS_ZIP, timeout=60)
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        for m in z.infolist():
            rel = m.filename.split('/', 1)[1] if '/' in m.filename else ''
            if not rel or m.is_dir(): continue
            path = os.path.join(dst, *rel.split('/'))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'wb') as o: o.write(z.read(m))


def widescreen_overlay(boot, work):
    """The SFA3MAX 16:9 patch (Rickow, MIT) as an overlay on the unpatched program, so it can be
    switched on and off while running (SCREEN SETTING > SCREEN SIZE > WIDE FORCED). The patch is
    downloaded and applied to a copy; work/boot.widescreen.json records, by load address, the words
    it changes in .text (translated both ways, chosen at run time) and the code it adds in unused
    .data (translated as code). Nothing of the patch is kept in the repository."""
    sys.path.insert(0, TOOLS)
    from psp_elf import Elf, relocate
    from psp_analyze import BASE
    out = os.path.join(work, 'boot.widescreen.json')
    if os.path.exists(out): os.remove(out)
    patcher = os.path.join(work, 'sfa3-PSP-widescreen', 'sfa3_ws_patternpatcher.py')
    try:
        dst = os.path.dirname(patcher)
        if not widescreen_verified(dst):                     # none yet, an older download, or changed
            shutil.rmtree(dst, ignore_errors=True)
            try: fetch_widescreen(dst)
            except Exception as ex:                          # (a network that only lets git through)
                print('  zip download failed (%s): trying git' % ex)
                shutil.rmtree(dst, ignore_errors=True)
                subprocess.check_call(['git', 'init', '-q', dst])
                subprocess.check_call(['git', '-C', dst, 'fetch', '-q', '--depth', '1', WS_REPO + '.git', WS_COMMIT])
                subprocess.check_call(['git', '-C', dst, 'checkout', '-q', 'FETCH_HEAD'])
            if not widescreen_verified(dst):
                raise RuntimeError('the patch at %s does not match its SHA-256 in tools/data/pins.json, so it was not run' % WS_COMMIT[:12])
        print('  widescreen patch: %s at %s (SHA-256 checked)' % (WS_REPO, WS_COMMIT[:12]))
        tmp = os.path.join(work, 'boot.widescreen.elf')
        run(patcher, boot, tmp)
    except Exception as ex:
        print('  widescreen patch not available (%s): WIDE FORCED will be off' % ex); return
    a_e, b_e = Elf(boot), Elf(tmp)
    a, _ = relocate(a_e, BASE); b, _ = relocate(b_e, BASE)
    text = a_e.by_name['.text']; tlo, thi = text['addr'], text['addr'] + text['size']
    t, caves = {}, {}
    for off in range(0, min(len(a), len(b)) - 3, 4):
        if a[off:off + 4] == b[off:off + 4]: continue
        w = int.from_bytes(b[off:off + 4], 'little')
        if tlo <= off < thi: t['0x%08X' % (BASE + off)] = w
        else:
            if a[off:off + 4] != b'\0\0\0\0': print('  widescreen patch changes data at %08X: not used' % (BASE + off)); return
            caves['0x%08X' % (BASE + off)] = w
    json.dump({'patch': WS_REPO, 'text': t, 'caves': caves}, open(out, 'w'), indent=0)
    os.remove(tmp)
    print('  widescreen overlay: %d words in .text, %d words of added code' % (len(t), len(caves)))


# --- assets --------------------------------------------------------------------------------------
def same_picture(a, b):
    """Whether two PNGs hold the same picture: the same header, palette and transparency chunks, and
    the same pixel data once decompressed, whatever zlib wrote them. Other files: the same bytes."""
    import zlib
    def parts(path):
        d = open(path, 'rb').read()
        if d[:8] != b'\x89PNG\r\n\x1a\n': return d
        out, idat, pos = [], b'', 8
        while pos + 8 <= len(d):
            n = int.from_bytes(d[pos:pos + 4], 'big'); tag = d[pos + 4:pos + 8]; body = d[pos + 8:pos + 8 + n]
            if tag == b'IDAT': idat += body
            elif tag in (b'IHDR', b'PLTE', b'tRNS'): out.append(tag + body)
            pos += 12 + n
        try: return out, zlib.decompress(idat)
        except zlib.error: return d
    try: return parts(a) == parts(b)
    except OSError: return False


def carry_over_edits(old, new, manifest):
    """Copy the files a player changed in an archive's folder from before 0.37.0 (flat, in pac/) into
    its new folder (by kind), so moving to the new layout loses no work. Returns the files carried over.

    What counts as a change is what the game itself would use (docs/ASSET_LOADING.md):
      - a raw file whose bytes differ from the disc's entry (raw files are the disc's bytes exactly);
      - a PNG (or TXT) dated a later second than its raw file (the game takes such a PNG over the raw
        file) whose picture differs from the one just extracted. Extracting can itself date a PNG a
        second after its raw file, and another Python's zlib writes the same picture with other bytes,
        so neither the date nor the bytes alone tell an edit.
    .pad files are the disc's stale bytes and are never carried. Carried files are dated after
    everything just extracted, so an edited PNG still wins over its new raw file."""
    import time
    carried = []
    for e in manifest['entries']:
        raw = e.get('raw')
        if not raw: continue
        old_raw = os.path.join(old, os.path.basename(raw))
        if not os.path.exists(old_raw): continue
        picks = []
        new_raw = os.path.join(new, *raw.split('/'))
        if open(old_raw, 'rb').read() != open(new_raw, 'rb').read(): picks.append(raw)
        raw_second = int(os.path.getmtime(old_raw))
        for key in ('png', 'text'):
            rel = e.get(key)
            src = rel and os.path.join(old, os.path.basename(rel))
            if src and os.path.exists(src) and int(os.path.getmtime(src)) > raw_second \
                    and not same_picture(src, os.path.join(new, *rel.split('/'))): picks.append(rel)
        for rel in picks:
            src, dst = os.path.join(old, os.path.basename(rel)), os.path.join(new, *rel.split('/'))
            data = open(src, 'rb').read()
            try:
                with open(dst, 'wb') as o: o.write(data)
            except OSError:                    # (a file some other program holds open on Windows)
                os.remove(dst)
                with open(dst, 'wb') as o: o.write(data)
            carried.append(dst)
    stamp = time.time() + 2
    for n, f in enumerate(carried): os.utime(f, (stamp + n, stamp + n))
    return carried


def export_assets(region, disc_root, unpack_pacs=True):
    """Copy out what is already in a standard format, and unpack each .pac archive into its folder
    under assets/<region>/ (tools/asset_layout.py; raw entries, textures and palettes as PNG, text as
    TXT, a subfolder per kind, a manifest each), then write index.json, which tells the game which
    folder holds which archive."""
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import pac, asset_layout
    d = DISCS[region]
    src = os.path.join(disc_root, 'PSP_GAME', 'USRDIR', d['data_dir'])
    out = os.path.join(ROOT, 'assets', region)
    music = os.path.join(out, 'music'); os.makedirs(music, exist_ok=True)
    tracks = sorted(glob.glob(os.path.join(src, 'bgm', '*.at3')))
    for t in tracks:
        dst = os.path.join(music, os.path.basename(t))
        if not (os.path.exists(dst) and os.path.getsize(dst) == os.path.getsize(t)): shutil.copyfile(t, dst)
    pacs = sorted(glob.glob(os.path.join(src, 'data', '*.pac')))
    archives, index = {}, {}
    old_root = os.path.join(out, 'pac')                       # the flat layout before 0.37.0
    carried = []
    palettes = pac.Palettes(program_image=os.path.join(os.path.dirname(disc_root), 'analysis.image.bin')) if region == 'us' else None   # pairs are the US program's
    if unpack_pacs:
        bar = Progress('Unpacking the archives', len(pacs), 'archives')
        for n, p in enumerate(pacs):
            name = os.path.splitext(os.path.basename(p))[0]
            rel = asset_layout.folder(name)
            if rel in index.values(): raise SystemExit('extract_assets.py: two archives would share %s' % rel)
            index[name] = rel
            dst = os.path.join(out, *rel.split('/'))
            if os.path.isdir(dst): shutil.rmtree(dst)         # no stale files from an older layout
            m = pac.unpack(p, dst, palettes=palettes)
            if os.path.isdir(os.path.join(old_root, name)):
                carried += carry_over_edits(os.path.join(old_root, name), dst, m)
            kinds = {}
            for e in m['entries']: kinds[e['kind']] = kinds.get(e['kind'], 0) + 1
            archives[name] = kinds
            bar.update(n + 1)
        bar.finish()
        json.dump({'about': 'Which folder holds which of the disc\'s archives. The game reads this to find '
                            'them (assets/LAYOUT.md); a folder can be moved if its line here is changed too.',
                   'archives': index}, open(os.path.join(out, 'index.json'), 'w'), indent=1)
        if os.path.isdir(old_root):
            moved = old_root + '.before-0.37'
            if os.path.exists(moved): shutil.rmtree(moved)
            os.rename(old_root, moved)
            print('  the old layout (pac/) is now %s: %d edited files were carried over%s. Delete it once '
                  'you are happy.' % (os.path.relpath(moved, ROOT), len(carried),
                                      ''.join('\n    ' + os.path.relpath(f, out) for f in carried[:20])))
    totals = {}
    for kinds in archives.values():
        for k, n in kinds.items(): totals[k] = totals.get(k, 0) + n
    manifest = {
        'region': region, 'serial': d['serial'],
        'music': {'format': 'ATRAC3plus (RIFF .at3)', 'files': [os.path.basename(t) for t in tracks],
                  'todo': 'decode to WAV / OGG with third_party/at3_standalone'},
        'archives': {'index': 'index.json', 'layout': 'assets/LAYOUT.md', 'count': len(archives),
                     'entries_by_kind': totals,
                     'notes': 'each archive in its own folder, its entries in a subfolder per kind: raw as on the '
                              'disc (+ .pad: stale bytes kept for exact rebuilds); textures and palettes as PNG; '
                              'text as TXT. Format: docs/PAC_FORMAT.md. Rebuild one: python tools/pac.py pack FOLDER FILE.pac'},
    }
    json.dump(manifest, open(os.path.join(out, 'manifest.json'), 'w'), indent=1)
    print('  assets/%s: %d music tracks; %d archives unpacked (%s)' % (
        region, len(tracks), len(archives), ', '.join('%d %s' % (n, k) for k, n in sorted(totals.items(), key=lambda x: -x[1])[:5])))


def main():
    ap = argparse.ArgumentParser(description='Extract and verify A3MX from your own ISO.')
    ap.add_argument('--region', choices=sorted(DISCS), default='us')
    ap.add_argument('--iso', help='the ISO (default: the matching one in imports/psp/)')
    ap.add_argument('--work', help='output folder (default work/, or work-jp/ for --region jp)')
    ap.add_argument('--no-widescreen', action='store_true', help='leave out the 16:9 patch (no WIDE FORCED)')
    ap.add_argument('--verify-only', action='store_true', help='check the disc and stop')
    ap.add_argument('--no-pac', action='store_true', help='do not unpack the .pac archives into assets/')
    a = ap.parse_args()
    d = DISCS[a.region]
    iso = a.iso or os.environ.get('A3MX_ISO') or find_iso(a.region)
    work = os.path.abspath(a.work or os.path.join(ROOT, 'work' if a.region == 'us' else 'work-' + a.region))

    print('disc', iso)
    image = Iso(iso)
    boot_bytes = image.read('PSP_GAME/SYSDIR/BOOT.BIN')
    boot_sha = hashlib.sha1(boot_bytes).hexdigest()
    if boot_sha != d['boot_sha1']:
        raise SystemExit('extract_assets.py: BOOT.BIN sha1 %s is not %s %s v%s (%s). This disc is not supported.'
                         % (boot_sha, d['title'], d['serial'], d['version'], d['boot_sha1']))
    iso_sha = sha1_file(iso, 'Checking the disc image')
    print('  %s %s v%s: program OK (%s)' % (d['title'], d['serial'], d['version'], boot_sha))
    print('  ISO sha1 %s%s' % (iso_sha, '' if iso_sha == d['iso_sha1'] else ' (a different dump of the same program: fine)'))
    if a.verify_only: return

    os.makedirs(work, exist_ok=True)
    print('extracting to', os.path.relpath(work, ROOT))
    print('  %d files' % image.extract(os.path.join(work, 'iso')))
    elf = os.path.join(work, 'boot.elf')
    open(elf, 'wb').write(boot_bytes)
    if a.no_widescreen:
        p = os.path.join(work, 'boot.widescreen.json')
        if os.path.exists(p): os.remove(p)
    elif a.region == 'us':
        widescreen_overlay(elf, work)
    run(os.path.join(TOOLS, 'psp_imports.py'), elf, '--json', os.path.join(work, 'imports.json'))
    run(os.path.join(TOOLS, 'psp_analyze.py'), elf, os.path.join(work, 'analysis.json'))
    export_assets(a.region, os.path.join(work, 'iso'), unpack_pacs=not a.no_pac)
    json.dump({'region': a.region, 'serial': d['serial'], 'version': d['version'], 'title': d['title'],
               'iso': os.path.basename(iso), 'iso_sha1': iso_sha, 'boot_sha1': boot_sha,
               # tools/build.py: whether a given ISO is the one already extracted, without hashing it again
               'iso_path': os.path.abspath(iso), 'iso_size': os.path.getsize(iso), 'iso_mtime': int(os.path.getmtime(iso))},
              open(os.path.join(work, 'disc.json'), 'w'), indent=1)
    if not d['buildable']:
        print('done. Note: the %s build is not supported yet; this extraction is for reference and asset work.' % a.region.upper())
    else:
        print('done. Next: cmake -B build -G Ninja && cmake --build build')


if __name__ == '__main__':
    main()
