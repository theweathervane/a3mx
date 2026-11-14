#!/usr/bin/env python3
"""Survey of the Street Fighter Alpha Anthology (PS2) disc, for the Hyper Street Fighter Alpha spike.

Spike tool (docs/HYPER_ALPHA.md). It reads the user's own disc image and never writes into the repo:

    python tools/anthology.py survey  [--iso PATH] [--out work/anthology/survey.json] [--hash]
    python tools/anthology.py music   [--iso PATH] [--out work/anthology/music] [--limit N] [--seconds S]

What it knows about the disc (SLUS-21317):
- SLUS_213.17 is the PS2 program: Capcom's CPS2 emulator and menus, with CRI ADX for music.
- X_DATA.BIN and Y_DATA.BIN are AFS archives (no name table; entries by index).
- X holds TIM2 pictures, MOMO containers, the CPS2 games' ROMs (one "MWo3" container per 68000
  program, named b_usa.bin and ro1_usa.bin .. ro9_usa.bin inside), graphics chunks, the QSound Z80
  driver (starts F3 ED 56 31, "/CPS2" string) and QSound sample ROMs (4 or 8 MiB).
- Y holds 588 CRI ADX streams (48 kHz stereo music).
"""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys

SECTOR = 2048
DEFAULT_ISO_DIR = os.path.join("imports", "ps2")
KNOWN_SHA1 = {"906b23e96a591d0bdc87201dacdef1ff7c875494": "Street Fighter Alpha Anthology (US) SLUS-21317 v1.01"}


def find_iso(path):
    if path:
        return path
    if os.path.isdir(DEFAULT_ISO_DIR):
        for name in sorted(os.listdir(DEFAULT_ISO_DIR)):
            if name.lower().endswith(".iso"):
                return os.path.join(DEFAULT_ISO_DIR, name)
    sys.exit(f"no .iso in {DEFAULT_ISO_DIR}/; pass --iso")


class Iso:
    """Just enough ISO 9660 to list the root directory (the Anthology keeps everything there)."""

    def __init__(self, path):
        self.f = open(path, "rb")
        pvd = self.read(16 * SECTOR, SECTOR)
        if pvd[1:6] != b"CD001":
            sys.exit(f"{path}: not an ISO 9660 image")
        root = pvd[156:190]
        lba, size = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
        self.files = {}
        data = self.read(lba * SECTOR, size)
        i = 0
        while i < len(data):
            n = data[i]
            if n == 0:
                i = (i // SECTOR + 1) * SECTOR
                continue
            rec = data[i:i + n]
            i += n
            name = rec[33:33 + rec[32]]
            if name in (b"\0", b"\1") or rec[25] & 2:
                continue
            self.files[name.decode("ascii", "replace").split(";")[0]] = (
                struct.unpack_from("<I", rec, 2)[0] * SECTOR, struct.unpack_from("<I", rec, 10)[0])

    def read(self, offset, size):
        self.f.seek(offset)
        return self.f.read(size)

    def file(self, name):
        off, size = self.files[name]
        return self.read(off, size)


def afs_entries(iso, name):
    base, _ = iso.files[name]
    head = iso.read(base, 8)
    if head[:4] != b"AFS\0":
        sys.exit(f"{name}: not an AFS archive")
    count = struct.unpack_from("<I", head, 4)[0]
    table = iso.read(base + 8, count * 8)
    return base, [struct.unpack_from("<II", table, k * 8) for k in range(count)]


def classify(head):
    if head[:2] == b"\x80\x00" and head[4] == 3:
        return "adx", None
    if head[2:6] == b"TIM2":
        return "tim2", None
    if head[2:6] == b"MWo3":
        inner = head[0x24:0x40].split(b"\0")[0].decode("ascii", "replace")
        return "mwo3", inner
    if head[0:4] == b"MOMO" or head[2:6] == b"MOMO":
        return "momo", None
    if head[:4] == b"\xf3\xed\x56\x31" and b"CPS2" in head:
        return "qsound_z80", head[0x10:0x28].decode("ascii", "replace").strip()
    return "raw", None


def survey(args):
    path = find_iso(args.iso)
    iso = Iso(path)
    cnf = iso.file("SYSTEM.CNF").decode("ascii", "replace")
    out = {"iso": os.path.basename(path), "system_cnf": cnf.strip().splitlines(),
           "root": {k: v[1] for k, v in sorted(iso.files.items())}, "archives": {}}
    if args.hash:
        h = hashlib.sha1()
        with open(path, "rb") as f:
            for block in iter(lambda: f.read(1 << 24), b""):
                h.update(block)
        out["sha1"] = h.hexdigest()
        out["known_as"] = KNOWN_SHA1.get(out["sha1"], "unknown dump")
    for arc in ("X_DATA.BIN", "Y_DATA.BIN"):
        base, entries = afs_entries(iso, arc)
        rows, totals = [], {}
        for idx, (off, size) in enumerate(entries):
            head = iso.read(base + off, 0x40)
            kind, note = classify(head)
            if kind == "raw" and size in (4 << 20, 8 << 20):
                kind = "qsound_samples?"
            rows.append({"index": idx, "offset": off, "size": size, "kind": kind,
                         **({"note": note} if note else {})})
            t = totals.setdefault(kind, [0, 0])
            t[0] += 1
            t[1] += size
        out["archives"][arc] = {"entries": rows,
                                "totals": {k: {"count": c, "bytes": b} for k, (c, b) in sorted(totals.items())}}
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w") as f:
        json.dump(out, f, indent=1)
    for arc, info in out["archives"].items():
        print(arc)
        for k, t in info["totals"].items():
            print(f"  {k:16s} {t['count']:4d} entries {t['bytes'] / 1e6:9.1f} MB")
    print("68000 programs (MWo3):")
    for row in out["archives"]["X_DATA.BIN"]["entries"]:
        if row["kind"] == "mwo3":
            print(f"  X[{row['index']}] {row.get('note')} ({row['size']} bytes packed)")
    print(f"wrote {args.out}")


def music(args):
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        sys.exit("ffmpeg not found (it decodes CRI ADX)")
    iso = Iso(find_iso(args.iso))
    base, entries = afs_entries(iso, "Y_DATA.BIN")
    os.makedirs(args.out, exist_ok=True)
    for idx, (off, size) in enumerate(entries[:args.limit] if args.limit else entries):
        dst = os.path.join(args.out, f"y{idx:03d}.ogg")
        cmd = [ffmpeg, "-v", "error", "-y", "-f", "adx", "-i", "pipe:0"]
        if args.seconds:
            cmd += ["-t", str(args.seconds)]
        subprocess.run(cmd + ["-c:a", "libvorbis", "-q:a", "6", dst], input=iso.read(base + off, size), check=True)
        print(dst)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("survey", help="list the disc's archives and what each entry is")
    s.add_argument("--iso")
    s.add_argument("--out", default=os.path.join("work", "anthology", "survey.json"))
    s.add_argument("--hash", action="store_true", help="SHA-1 the whole image (about 30 s)")
    s.set_defaults(func=survey)
    m = sub.add_parser("music", help="decode Y_DATA.BIN's ADX streams to OGG (loop points are not kept yet)")
    m.add_argument("--iso")
    m.add_argument("--out", default=os.path.join("work", "anthology", "music"))
    m.add_argument("--limit", type=int, default=0)
    m.add_argument("--seconds", type=float, default=0)
    m.set_defaults(func=music)
    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
