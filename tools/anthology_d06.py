#!/usr/bin/env python3
"""H5b: how A3MX's per-character data file (06_data) is built from the arcade's tables.

The format (from the decomp's unpack_offsets, 0x0886E638 in src/core/fighter/state.c):
  0x000  u32 0, u32 size of the data after the header
  0x008  s32 offset[64]   from the end of the header (file + 0x208)
  0x108  s32 count[64]    0: a table; -1: a list of frame numbers ending in -1; > 0: that many words
                          naming other entries
  0x208  the data; it starts with the 10-byte animation frames (entries 1-4 point into it)

Where the entries come from in Alpha 3's overlay (zero3_usa.bin, X[253]; docs/HYPER_ALPHA.md, H5):
  frames, 1-5  0x67DA78's block: the animations (converted, see psp_frames)
  6-10         0x67D678's block: hit-box lists 1-4 and the attack records (list 5)
  11, 12, 13   the 0x67D6F8, 0x67D778, 0x67FCF8 blocks
  15, 16       parts of the 0x67F9F8 block
  17, 18, 19   the 0x67FEF8, 0x67FD78, 0x67FDF8 blocks
  22-45        0x67D878's block: 8-byte records, the fourth halfword divided by 20

  python tools/anthology_d06.py check [--iso ISO] [--a3mx assets/us]
compares what this module builds from zero3 with A3MX's own files for the 32 Alpha 3 characters.
Nothing here writes assets yet: the writer comes once every entry is accounted for.
"""
import argparse
import glob
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import anthology as an  # noqa: E402

ZERO3 = 253
ZERO3_LOAD = 0x1F0000
ZERO3_CODE_END = 0x583C50
HEADER = 0x208

ANIM_TABLE = 0x67DA78           # per character: the animation block
BOX_TABLE = 0x67D678            # per character: hit boxes and attack records
WHOLE_BLOCKS = {11: 0x67D6F8, 12: 0x67D778, 13: 0x67FCF8, 17: 0x67FEF8, 18: 0x67FD78, 19: 0x67FDF8}
# A3MX's entries 1-4 take the four slices in this order of the block's first four pointers
LIST_FOR_SLICE = (1, 3, 2, 4)


class Image:
    def __init__(self, data, load):
        self.z, self.load = data, load

    def u32(self, a):
        return struct.unpack_from("<I", self.z, a - self.load)[0]

    def u16(self, a):
        return struct.unpack_from("<H", self.z, a - self.load)[0]

    def s16(self, a):
        return struct.unpack_from("<h", self.z, a - self.load)[0]

    def bytes(self, a, n):
        return bytes(self.z[a - self.load:a - self.load + n])


def read_06(path):
    r = open(path, "rb").read()
    offs = struct.unpack_from("<64i", r, 8)
    counts = struct.unpack_from("<64i", r, 0x108)
    return r, r[HEADER:], offs, counts


def frame_list(data, off):
    out = []
    while True:
        v = struct.unpack_from("<i", data, off)[0]
        if v == -1:
            return out
        out.append(v)
        off += 4


# ------------------------------------------------------------------ the animations
def anim_frames(img, a):
    """An arcade animation: 20-byte frames until one whose duration has bit 0x8000 (then a u32 loop
    pointer may follow) or 0x4000 (no loop)."""
    out = []
    while True:
        out.append(a)
        if img.u16(a) & 0xC000:
            return out
        a += 20


def assign_frames(img, slices):
    """Frame numbers as the PSP gives them: animations in order of first reference (slice by slice,
    entry by entry); an animation that starts inside another referenced one shares its frames."""
    refs, seen = [], set()
    for s, cnt in slices:
        for k in range(cnt):
            o = img.u16(s + 2 * k)
            if o and s + o not in seen:
                seen.add(s + o)
                refs.append(s + o)
    frames_of = {a: anim_frames(img, a) for a in refs}
    inside = {a for a in refs if any(b != a and a in frames_of[b][1:] for b in refs)}
    number, frames = {}, []
    for a in refs:
        if a in inside or a in number:
            continue
        for f in frames_of[a]:
            if f in number:
                break
            number[f] = len(frames)
            frames.append(f)
    lists = []
    for s, cnt in slices:
        lists.append([number.get(s + img.u16(s + 2 * k)) if img.u16(s + 2 * k) else None
                      for k in range(cnt)])
    return frames, lists, number


def sprite_records(img, start, stop=None):
    """The arcade's sprite records from start: u16 2, u16, u16 n, padding to 4, u32 tiles, (n + 1) x 4
    bytes. A frame's sprite is the record's number in this run."""
    recs, a, i = {}, start, 0
    while (stop is None or a < stop) and a + 8 < img.load + len(img.z) and img.u16(a) == 2:
        n = img.u16(a + 4)
        p = a + 6 + (2 if (a + 6) % 4 else 0)
        recs[a] = i
        i += 1
        a = p + 4 + 4 * (n + 1)
    return recs


def psp_frames(img, slices, sprite_index):
    """Arcade animations -> the PSP's frame table (10 bytes: u16 duration, u16 the arcade's +2, u16
    sprite, s16 advance, u16 tail) and entry 5 (each distinct 12-byte arcade tail once, in order of
    first use). advance: 1, 0 on a last frame, or the loop as a jump back."""
    frames, lists, number = assign_frames(img, slices)
    tails, fb, tb = {}, bytearray(), bytearray()
    for fa in frames:
        dur, f1, spr = struct.unpack_from("<HHI", img.z, fa - img.load)
        tail = img.bytes(fa + 8, 12)
        if tail not in tails:
            tails[tail] = len(tails)
            tb += tail
        if dur & 0x8000:
            lp = img.u32(fa + 20)
            adv = -((fa - lp) // 20) if lp in number and lp <= fa else 0
        elif dur & 0x4000:
            adv = 0
        else:
            adv = 1
        fb += struct.pack("<HHHhH", dur, f1, sprite_index(spr), adv, tails[tail])
    return bytes(fb), bytes(tb), lists, frames


# ------------------------------------------------------------------ the check against A3MX
def compare(a, b):
    n = min(len(a), len(b)) // 2
    return sum(a[2 * i:2 * i + 2] == b[2 * i:2 * i + 2] for i in range(n)), n


def check_character(img, ch, path):
    r, data, offs, counts = read_06(path)
    lists_psp = {n: frame_list(data, offs[n]) for n in (1, 2, 3, 4)}
    blk = img.u32(ANIM_TABLE + 4 * ch)
    g = [img.u32(blk + 4 * i) for i in range(4)]
    order = list(zip(LIST_FOR_SLICE, g))
    frames, lists, number = assign_frames(img, [(s, len(lists_psp[n])) for n, s in order])
    list_same = sum(a == b for (n, _), L in zip(order, lists) for a, b in zip(L, lists_psp[n]))
    list_total = sum(len(lists_psp[n]) for n in lists_psp)

    # animations whose frames match, field by field (the sprite number left out: see the docs)
    nfr = offs[1] // 10 if offs[1] else 0
    tails = data[offs[5]:]
    anim_same = anim_total = 0
    for n, s in order:
        for k, pi in enumerate(lists_psp[n]):
            o = img.u16(s + 2 * k)
            if not o:
                continue
            anim_total += 1
            ok = True
            fr = anim_frames(img, s + o)
            for j, fa in enumerate(fr):
                if pi + j >= nfr:
                    ok = False
                    break
                p = struct.unpack_from("<HhHhH", data, 10 * (pi + j))
                dur, f1 = img.u16(fa), img.u16(fa + 2)
                tail = img.bytes(fa + 8, 12)
                if dur & 0x8000:
                    lp = img.u32(fa + 20)
                    want = -((fa - lp) // 20) if s + o <= lp <= fa else 0
                else:
                    want = 0 if dur & 0x4000 else 1
                if (dur, f1, want, tail) != (p[0], p[1] & 0xFFFF, p[3], tails[12 * p[4]:12 * p[4] + 12]):
                    ok = False
                    break
            anim_same += ok

    # the copied entries
    copied = {}
    sub = img.u32(img.u32(BOX_TABLE + 4 * ch))
    starts = sorted({o for o in offs[1:] if o} | {0, struct.unpack_from("<I", r, 4)[0]})
    def size(n):
        i = starts.index(offs[n])
        return starts[i + 1] - offs[n]
    for n in range(6, 11):
        copied[n] = compare(data[offs[n]:offs[n] + size(n)], img.bytes(sub + img.s16(sub + 2 * (n - 6)), size(n)))
    for n, t in WHOLE_BLOCKS.items():
        copied[n] = compare(data[offs[n]:offs[n] + size(n)], img.bytes(img.u32(t + 4 * ch), size(n)))
    return (list_same, list_total), (anim_same, anim_total), copied


def check(args):
    iso = an.Iso(an.find_iso(args.iso))
    base, entries = an.afs_entries(iso, "X_DATA.BIN")
    off, size = entries[ZERO3]
    data = an.unlz16(iso.read(base + off, size))
    if an.mwo3_header(data)["name"] != "zero3_usa.bin":
        sys.exit("X[253] is not zero3_usa.bin: a different disc?")
    img = Image(data, ZERO3_LOAD)
    tot = [0, 0, 0, 0]
    exact_entries = {}
    for ch in range(0x20):
        hits = glob.glob(os.path.join(args.a3mx, "characters", f"{ch:02x}_*", "fighter", "data", "06_data_*.bin"))
        if not hits:
            continue
        (ls, lt), (am, at), copied = check_character(img, ch, hits[0])
        tot[0] += ls; tot[1] += lt; tot[2] += am; tot[3] += at
        for n, (e, t) in copied.items():
            exact_entries.setdefault(n, [0, 0])
            exact_entries[n][0] += e == t
            exact_entries[n][1] += 1
        print(f"{ch:#04x} {os.path.basename(os.path.dirname(os.path.dirname(os.path.dirname(hits[0])))):14s} "
              f"animation starts {ls}/{lt}, animations {am}/{at}")
    print(f"all: animation starts {tot[0]}/{tot[1]}, animations matching frame for frame {tot[2]}/{tot[3]}")
    print("copied entries, characters exact: " +
          ", ".join(f"{n}: {e}/{t}" for n, (e, t) in sorted(exact_entries.items())))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("check", help="compare zero3 -> 06_data with A3MX's own files")
    c.add_argument("--iso")
    c.add_argument("--a3mx", default=os.path.join("assets", "us"))
    c.set_defaults(func=check)
    a = ap.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
