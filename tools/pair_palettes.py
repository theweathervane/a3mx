#!/usr/bin/env python3
"""Pair the indexed textures with their palettes, by watching the game draw them.

A 4 or 8-bit texture holds colour indices; the colours are chosen when it is drawn, from palette data
that can be anywhere: a palette entry, the stage layout's data entry, or the program itself. Nothing
in the archives says which, so this runs the game and records what each indexed texture is actually
drawn with (--palette-census), then finds those colours in the disc's files.

  census   run the game (headless, nothing drawn) through:
             - the test scenarios (tests/scenarios/*.txt),
             - a Free Battle on every stage, the fighters pushed to both walls,
             - Arcade with every character slot to the end (the opponent knocked out each round,
               P1's health held), which plays every ending,
           recording each texture / colours pair drawn. Takes a while (-j to run several at once).
  pair     match the records with the extracted archives and the program; write
           tools/data/palette_pairs.json: for each indexed texture, where its colours are (archive,
           entry, byte offset; or a program address). References only: no game data is written.

Index 0 is drawn transparent whatever the palette holds, so it is left out of the matching. Of the
palettes a texture is drawn with, fade steps (darker copies, down to black, which some screens keep as
data) are passed over: the pairing keeps the most varied, brightest ones, and of those (palette
animation frames: lights, water) the one drawn most often.

Usage:
    python tools/pair_palettes.py census [--exe build/a3mx] [--work work] [--out build/census] [-j 2] [--arcade 03,05]
    python tools/pair_palettes.py pair [--work work] [--census build/census] [--out tools/data/palette_pairs.json]
"""
import argparse, collections, concurrent.futures, glob, json, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import pac
import measure_stages

PROGRAM_BASE = 0x08804000      # where work/analysis.image.bin is loaded
P1_CHARACTER = 0x092E3FF2      # u8: P1's character, read when the fight is set up (US program)
P1_HEALTH, P2_HEALTH = 0x092E3F10, 0x092E43A8
CHARACTERS = 0x2A              # slots 0x00 - 0x29 (docs/STAGES.md: the name table)
M64 = (1 << 64) - 1


def ge_hash(b):
    """src/platform/ge_soft.c ge_hash"""
    h, n, i = (0x9E3779B97F4A7C15 ^ len(b)) & M64, len(b), 0
    while n - i >= 8:
        h = ((h ^ int.from_bytes(b[i:i + 8], 'little')) * 0xFF51AFD7ED558CCD) & M64
        h ^= h >> 32
        i += 8
    while i < n:
        h = ((h ^ b[i]) * 0x100000001B3) & M64
        i += 1
    return h


# ---------------------------------------------------------------- census
def run_game(a, name, frames, inp, extra):
    d = os.path.join(a.out, name)
    os.makedirs(d, exist_ok=True)
    cmd = [a.exe, '--work', a.work, '--save-dir', d, '--log', os.path.join(d, 'log'), '--headless', '--fast',
           '--renderer', 'none', '--no-assets', '--frames', str(frames), '--palette-census', os.path.join(a.out, name + '.txt')]
    if inp: cmd += ['--input', inp]
    r = subprocess.run(cmd + extra, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    return name, r.returncode


def scenario_job(path):
    sc = {'frames': 600, 'input': None, 'args': []}
    for line in open(path, encoding='utf-8'):
        line = line.split('#', 1)[0].strip()
        if not line: continue
        k, _, v = line.partition(' ')
        if k == 'frames': sc['frames'] = int(v)
        elif k == 'input': sc['input'] = v.strip()
        elif k == 'args': sc['args'] += v.split()
        elif k == 'stick': sc['args'] += ['--stick', v.strip()]
    return 'scenario_' + os.path.splitext(os.path.basename(path))[0], sc['frames'], sc['input'], sc['args']


def stage_job(k):
    inp, f = measure_stages.script(k)
    left, right = f + 700, f + 1100
    pokes = []
    for at, x in ((left, '118'), (right, '3E8')):            # both fighters to one wall, then the other
        pokes += ['--poke', '%d:%08X=%s/2' % (at, measure_stages.P1_X, x), '--poke', '%d:%08X=%s/2' % (at, measure_stages.P2_X, x)]
    return 'stage_%02d' % k, f + 1500, inp, pokes


def arcade_job(ch, frames=240000):
    """Arcade with character slot ch to its ending. P1's health is held at 90 and the opponent's at 1, and
    the attack buttons are pressed throughout: a hit is a knockout, and a round nobody wins by a hit
    is P1's on time. (Setting the opponent's health to 0 instead skips the hit, and some fights then
    never finish their knockout.)"""
    head = open(os.path.join(ROOT, 'tests', 'scenarios', 'arcade_fight.txt')).read().split('input ', 1)[1].split(';4100=')[0]
    presses = []
    for n, f in enumerate(range(4100, frames, 45)):            # cross gets through every screen; square, a punch
        presses += ['%d=%s' % (f, 'cross' if n % 2 == 0 else 'square'), '%d=' % (f + 4)]
    presses += ['%d=start;%d=' % (f, f + 4) for f in range(4115, frames, 600)]
    pokes = ['--poke', '2101-3000:%08X=%X/1' % (P1_CHARACTER, ch),
             '--poke', '4100:%08X=90/2' % P1_HEALTH, '--poke', '4100:%08X=1/2' % P2_HEALTH]
    return 'arcade_%02x' % ch, frames, head + ';' + ';'.join(presses), pokes


def census(a):
    os.makedirs(a.out, exist_ok=True)
    if a.arcade:                                              # just these characters' Arcade runs (hex slots)
        jobs = [arcade_job(int(ch, 16)) for ch in a.arcade.split(',')]
    else:
        jobs = [scenario_job(p) for p in sorted(glob.glob(os.path.join(ROOT, 'tests', 'scenarios', '*.txt')))]
        jobs += [stage_job(k) for k in range(1, len(measure_stages.MENU_NAMES) + 1)]
        jobs += [arcade_job(ch) for ch in range(CHARACTERS)]
    done = 0
    with concurrent.futures.ThreadPoolExecutor(a.j) as ex:
        for name, rc in ex.map(lambda j: run_game(a, *j), jobs):
            done += 1
            print('%3d/%d %s%s' % (done, len(jobs), name, '' if rc == 0 else '  (exit %d: no record from it)' % rc)); sys.stdout.flush()


# ---------------------------------------------------------------- pairing
def load_disc(work):
    entries = {}
    for f in sorted(glob.glob(os.path.join(work, 'iso', 'PSP_GAME', 'USRDIR', '*', 'data', '*.pac'))):
        name = os.path.splitext(os.path.basename(f))[0]
        for i, (t, e) in enumerate(pac.read(f)): entries[(name, i)] = (t, e)
    return entries


def indexed_textures(entries):
    """ge_hash of the pixels -> [(archive, index)] for every 4 / 8-bit texture"""
    out = collections.defaultdict(list)
    for (name, i), (t, e) in entries.items():
        if t & 0xFFFF0000 not in (0x10000000, 0x00800000): continue
        hd = pac.pvr_header(e)
        if not hd or hd[2] not in (5, 7): continue
        off, _, df, w, h = hd
        out[ge_hash(e[off:off + (w * h // 2 if df == 5 else w * h)])].append((name, i))
    return out


def colours_of(rec):
    """the colours a record holds, as the little-endian bytes they are stored as"""
    hexs = rec[9]
    return b''.join(int(hexs[k:k + 4], 16).to_bytes(2, 'little') for k in range(0, len(hexs), 4))


def pair(a):
    entries = load_disc(a.work)
    textures = indexed_textures(entries)
    image = open(os.path.join(a.work, 'analysis.image.bin'), 'rb').read()
    by_archive = collections.defaultdict(list)
    for key in entries: by_archive[key[0]].append(key)
    palettes = [k for k, (t, _) in entries.items() if t & 0xFFFF0000 == 0x04000000]

    def find(data, colours):
        j = data.find(colours)
        while j >= 0 and j % 2: j = data.find(colours, j + 1)
        return j

    cache = {}
    def locate(colours, archive):
        """where colours[1:] are stored: this archive's entries first, then palette entries, then any
        entry, then the program"""
        key = (colours, archive)
        if key in cache: return cache[key]
        found = None
        for k in by_archive[archive] + palettes + list(entries):
            j = find(entries[k][1], colours[2:])
            if j >= 2: found = {'archive': k[0], 'entry': k[1], 'offset': j - 2}; break
        if not found:
            j = find(image, colours[2:])
            if j >= 2: found = {'program': '0x%08X' % (PROGRAM_BASE + j - 2)}
        cache[key] = found
        return found

    tally = collections.defaultdict(collections.Counter)      # texture -> source -> draws
    looks = {}                                                # source -> (distinct colours, brightness)
    files = sorted(glob.glob(os.path.join(a.census, '*.txt')))
    if not files: raise SystemExit('no census in %s: run "pair_palettes.py census" first' % a.census)
    for path in files:
        for line in open(path):
            if line.startswith('#'): continue
            rec = line.split()
            for tex in textures.get(int(rec[0], 16), []):
                colours = colours_of(rec)
                src = locate(colours, tex[0])
                if not src: continue
                key = json.dumps(src, sort_keys=True)
                tally[tex][key] += int(rec[7])
                if key not in looks:
                    vals = [int.from_bytes(colours[k:k + 2], 'little') for k in range(2, len(colours), 2)]
                    looks[key] = (len(set(vals)), sum((v & 31) + (v >> 5 & 31) + (v >> 10 & 31) for v in vals))
    out = {}
    for (name, i), counts in sorted(tally.items()):
        # Fades: a screen fading in or out draws its textures in darker and darker copies of the
        # palette, and some screens keep those steps (down to plain black) as data, so they are found
        # too and can be drawn more often than the real colours. Keep the candidates near the most
        # varied and brightest, then the most drawn of them (palette animation frames are alike in
        # both, so they still compete on draws).
        top_distinct = max(looks[k][0] for k in counts)
        varied = [k for k in counts if looks[k][0] >= max(2, 0.75 * top_distinct)] or list(counts)
        top_bright = max(looks[k][1] for k in varied)
        bright = [k for k in varied if looks[k][1] >= 0.85 * top_bright]
        src = max(bright, key=lambda k: (counts[k], looks[k][1]))
        out['%s/%d' % (name, i)] = dict(json.loads(src), draws=counts[src], others=len(counts) - 1)
    total = sum(len(v) for v in textures.values())
    doc = {'format': 1,
           'about': 'Where the colours of each indexed texture are, found by tools/pair_palettes.py from what the game '
                    'draws (US program). offset: bytes into the entry; program: the address in the program. Index 0 '
                    'is drawn transparent. others: how many other palettes it was also drawn with (palette animation, '
                    'fade steps); the one kept is the most varied and brightest, then the most drawn.',
           'paired': len(out), 'indexed_textures': total, 'textures': out}
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    json.dump(doc, open(a.out, 'w'), indent=1, sort_keys=False)
    missing = sorted('%s/%d' % t for v in textures.values() for t in v if '%s/%d' % t not in out)
    print('%d of %d indexed textures paired (%s)' % (len(out), total, a.out))
    if missing: print('never drawn in the census: ' + ', '.join(missing))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)
    c = sub.add_parser('census'); c.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx.exe' if os.name == 'nt' else 'a3mx'))
    c.add_argument('--work', default=os.path.join(ROOT, 'work')); c.add_argument('--out', default=os.path.join(ROOT, 'build', 'census'))
    c.add_argument('-j', type=int, default=2)
    c.add_argument('--arcade', help='only the Arcade runs of these character slots, hex, comma-separated (e.g. 03,05)')
    p = sub.add_parser('pair'); p.add_argument('--work', default=os.path.join(ROOT, 'work'))
    p.add_argument('--census', default=os.path.join(ROOT, 'build', 'census'))
    p.add_argument('--out', default=os.path.join(ROOT, 'tools', 'data', 'palette_pairs.json'))
    a = ap.parse_args()
    census(a) if a.cmd == 'census' else pair(a)


if __name__ == '__main__':
    main()
