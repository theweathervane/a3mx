#!/usr/bin/env python3
"""Survey of the Street Fighter Alpha Anthology (PS2) disc, for the Hyper Street Fighter Alpha spike.

Spike tool (docs/HYPER_ALPHA.md). It reads the user's own disc image and never writes into the repo:

    python tools/anthology.py survey  [--iso PATH] [--out work/anthology/survey.json] [--hash]
    python tools/anthology.py music   [--iso PATH] [--out assets/us/anthology/music] [--start N] [--limit N]
    python tools/anthology.py unpack  [--iso PATH] [--out work/anthology/x] [INDEX ...]
    python tools/anthology.py state   FILE.p2s ...   (a PCSX2 save state of Hyper: each fighter's version)

What it knows about the disc (SLUS-21317):
- SLUS_213.17 is the PS2 program: Capcom's CPS2 emulator and menus, with CRI ADX for music.
- X_DATA.BIN and Y_DATA.BIN are AFS archives (no name table; entries by index).
- X holds TIM2 pictures, MOMO containers, the games (one "MWo3" overlay each: MIPS code the 68000
  program was statically recompiled into, plus its data; named sub_usa.bin and zero1_usa.bin ..
  inside), graphics chunks, the QSound Z80 driver (starts F3 ED 56 31, "/CPS2" string) and QSound
  sample ROMs (4 or 8 MiB).
- Entries beginning with a 16-bit flag word before "MWo3" / "MOMO" are packed with the halfword LZ
  in SLUS_213.17 at 0x148698 (unlz16 below).
- Y holds 588 CRI ADX streams (48 kHz stereo): 297 loop (the music, about 347 minutes), 291 are
  short one-shots.
"""
import argparse
import bisect
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


def unlz16(src):
    """The halfword LZ of SLUS_213.17 (0x148698). 16-bit flag words, most significant bit first.
    Clear bit: copy one halfword. Set bit: a u16 v follows; length = v >> 11 halfwords (0: the
    length is the next u16) and distance = v & 0x7FF halfwords back. Distance 0 writes `length`
    zero halfwords, and length 0 with distance 0 ends the stream."""
    out = bytearray()
    i, mask, flags, n = 0, 0, 0, len(src)
    while i + 1 < n:
        if mask == 0:
            flags = src[i] | src[i + 1] << 8
            i += 2
            mask = 0x8000
        if flags & mask:
            v = src[i] | src[i + 1] << 8
            i += 2
            length, dist = v >> 11, v & 0x7FF
            if length == 0:
                dist, length = v, src[i] | src[i + 1] << 8
                i += 2
            if dist == 0:
                if length == 0:
                    break
                out += bytes(2 * length)
            else:
                p = len(out) - 2 * dist
                for k in range(length):
                    out += out[p + 2 * k:p + 2 * k + 2]
        else:
            out += src[i:i + 2]
            i += 2
        mask >>= 1
    return bytes(out)


def mwo3_header(data):
    """Header of an unpacked MWo3 overlay: index, load address, end address, and the name."""
    index, base, f2, f3, f4, end = struct.unpack_from("<6I", data, 4)
    name = data[0x20:0x40].split(b"\0")[0].decode("ascii", "replace")
    return {"index": index, "base": base, "end": end, "fields": [f2, f3, f4], "name": name}


def classify(head):
    if head[:2] == b"\x80\x00" and head[4] == 3:
        return "adx", None
    if head[2:6] == b"TIM2":
        return "tim2", None
    if head[2:6] == b"MWo3":
        return "mwo3", None
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
            if kind == "mwo3":  # the readable name is in the packed stream; unpack the start for it
                note = mwo3_header(unlz16(iso.read(base + off, 0x200)) + bytes(0x40))["name"]
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
    print("Game overlays (MWo3, packed):")
    for row in out["archives"]["X_DATA.BIN"]["entries"]:
        if row["kind"] == "mwo3":
            print(f"  X[{row['index']}] {row.get('note')} ({row['size']} bytes packed)")
    print(f"wrote {args.out}")


def adx_info(head):
    """Channels, rate, samples and the loop (start, end samples) from a CRI ADX header (v3 or v4)."""
    off = struct.unpack_from(">H", head, 2)[0]
    channels, rate, samples = head[7], struct.unpack_from(">I", head, 8)[0], struct.unpack_from(">I", head, 12)[0]
    version = head[0x12]
    loop = None
    if version == 4 and off >= 0x34 and struct.unpack_from(">I", head, 0x24)[0] == 1:
        loop = (struct.unpack_from(">I", head, 0x28)[0], struct.unpack_from(">I", head, 0x30)[0])
    elif version == 3 and off >= 0x28 and struct.unpack_from(">I", head, 0x18)[0] == 1:
        loop = (struct.unpack_from(">I", head, 0x1C)[0], struct.unpack_from(">I", head, 0x24)[0])
    return {"channels": channels, "rate": rate, "samples": samples, "loop": loop}


def music(args):
    """Y_DATA.BIN's ADX streams to OGG Vorbis. Looping tracks keep their loop as LOOPSTART /
    LOOPLENGTH tags (in samples), the convention most players and engines read; an index.json lists
    every track. Tracks already converted are skipped, so a long run can be resumed."""
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        sys.exit("ffmpeg not found (it decodes CRI ADX)")
    iso = Iso(find_iso(args.iso))
    base, entries = afs_entries(iso, "Y_DATA.BIN")
    os.makedirs(args.out, exist_ok=True)
    index_path = os.path.join(args.out, "index.json")
    index = {}
    if os.path.exists(index_path):
        with open(index_path) as f:
            index = {t["index"]: t for t in json.load(f)["tracks"]}
    last = len(entries) if not args.limit else min(len(entries), args.start + args.limit)
    for idx in range(args.start, last):
        off, size = entries[idx]
        info = adx_info(iso.read(base + off, 0x40))
        name = f"y{idx:03d}.ogg"
        dst = os.path.join(args.out, name)
        if not os.path.exists(dst) or args.seconds:
            cmd = [ffmpeg, "-v", "error", "-y", "-f", "adx", "-i", "pipe:0"]
            if args.seconds:
                cmd += ["-t", str(args.seconds)]
            if info["loop"]:
                start, end = info["loop"]
                cmd += ["-metadata", f"LOOPSTART={start}", "-metadata", f"LOOPLENGTH={end - start}"]
            tmp = dst + ".part.ogg"
            subprocess.run(cmd + ["-c:a", "libvorbis", "-q:a", "6", tmp],
                           input=iso.read(base + off, size), check=True)
            os.replace(tmp, dst)
            print(dst)
        index[idx] = {"index": idx, "file": name, "seconds": round(info["samples"] / info["rate"], 3),
                      **info}
    with open(index_path, "w") as f:
        json.dump({"source": "Y_DATA.BIN", "tracks": [index[k] for k in sorted(index)]}, f, indent=1)
    print(f"wrote {index_path} ({len(index)} tracks)")


def unpack(args):
    iso = Iso(find_iso(args.iso))
    base, entries = afs_entries(iso, "X_DATA.BIN")
    os.makedirs(args.out, exist_ok=True)
    wanted = args.index or [k for k, (off, size) in enumerate(entries)
                            if iso.read(base + off + 2, 4) in (b"MWo3", b"MOMO")]
    for idx in wanted:
        off, size = entries[idx]
        data = unlz16(iso.read(base + off, size))
        note = ""
        if data[:4] == b"MWo3":
            h = mwo3_header(data)
            note = f"{h['name']} at {h['base']:#x}-{h['end']:#x}"
        dst = os.path.join(args.out, f"x{idx:03d}.bin")
        with open(dst, "wb") as f:
            f.write(data)
        print(f"X[{idx}] {size} -> {len(data)} bytes {note}")


# Hyper Street Fighter Alpha, read from PCSX2 save states (2026-11-15). The arcade RAM (0xFF0000) is at
# 0xB80000 in the PS2's memory; the fighters' objects are at 0xFF8400, 0xFF8800 (0xB88400, 0xB88800).
# +0x102 is the character-version number, +0xD3 its version flags (from the table at 0x5C2490 in
# zero9_usa.bin, by +0x102, via +0x117). +0x118 is 1 with AUTO guard and +0x32F the AUTO count on the HUD.
# Alpha 3's modes are one byte each: MAZI +0xEC (copy +0x15A), SAIKYO +0xEB (+0x131), CLASSICAL +0xF0
# (+0x15E). +0x132 is the ISM (-1, 0, 1; 0 = A-ism; CLASSICAL forces -1). zero9's 0x2891A0 writes the
# S-ISMs: green +0x122 = 0x82 and ISM 0, blue 0x02 and ISM -1, red 0x80 and ISM 0, pink = CLASSICAL with
# +0x15E = 0x81 and ISM -1 (back to plain CLASSICAL if the character is not one of the SF2 fighters,
# mask 0x13218C13 by character). 0x295F40 then sets +0x123 from +0x122: 3 if positive, 2 if negative, 4 if
# negative with bit 0x02 (so green 4, blue 3, red 2). 0x4B6BE0 sets +0xD3 from the table and, for pink,
# ORs in 0x08 (the Classical / Champion Edition bit the Alpha 2 blue variants already have) and 0x80.
HYPER_FIGHTERS = (0xB88400, 0xB88800)
HYPER_VERSIONS = [  # (first number, count, name, flags)
    (0x00, 0x30, "Alpha 3", 0x10),
    (0x30, 0x10, "Alpha", 0x01),
    (0x40, 0x20, "Alpha 2", 0x22),
    (0x60, 0x20, "Alpha 2 Gold", 0x06),
]
FLAG_BITS = {0x01: "Alpha", 0x02: "Alpha 2 family", 0x04: "Gold", 0x08: "Classical (CE) version",
             0x10: "Alpha 3", 0x20: "Alpha 2 (not Gold)", 0x40: "?", 0x80: "pink S-ISM"}
HYPER_MODES = ((0xEC, "MAZI"), (0xEB, "SAIKYO"), (0xF0, "CLASSICAL"))
ISMS = {0xFF: "-1", 0x00: "0 (A)", 0x01: "1"}
SISM_SEEN = {(0x82, 0x04): "green", (0x02, 0x03): "blue", (0x80, 0x02): "red"}


def zip_member(path, name):
    """A member of a PCSX2 save state (a zip whose members are zstd-compressed, method 93)."""
    import zipfile
    z = zipfile.ZipFile(path)
    info = z.getinfo(name)
    if info.compress_type in (0, 8):
        return z.read(name)
    with open(path, "rb") as f:
        f.seek(info.header_offset)
        head = f.read(30)
        n, e = struct.unpack_from("<HH", head, 26)
        f.seek(info.header_offset + 30 + n + e)
        raw = f.read(info.compress_size)
    try:
        import zstandard
        return zstandard.ZstdDecompressor().decompress(raw, max_output_size=info.file_size)
    except ImportError:
        pass
    try:
        import ctypes, ctypes.util
        lib = ctypes.CDLL(ctypes.util.find_library("zstd") or "libzstd.so.1")
        lib.ZSTD_decompress.restype = ctypes.c_size_t
        lib.ZSTD_isError.restype = ctypes.c_uint
        out = ctypes.create_string_buffer(info.file_size)
        r = lib.ZSTD_decompress(out, info.file_size, raw, len(raw))
        if not lib.ZSTD_isError(r):
            return out.raw[:r]
    except OSError:
        pass
    zstd = shutil.which("zstd")
    if zstd:
        return subprocess.run([zstd, "-d", "-c"], input=raw, capture_output=True, check=True).stdout
    sys.exit("can't read the save state: no zstd (pip install zstandard, or install zstd)")


def describe_version(number, flags):
    for first, count, name, base_flags in HYPER_VERSIONS:
        if first <= number < first + count:
            char = number - first
            extra = " (Classical / blue variant)" if flags & 0x08 and name.startswith("Alpha 2") else ""
            return f"{name}{extra}, character slot {char:#04x}"
    return "not a known character-version number"


def state(args):
    for path in args.file:
        mem = zip_member(path, "eeMemory.bin")
        print(os.path.basename(path))
        for i, base in enumerate(HYPER_FIGHTERS):
            number, flags = mem[base + 0x102], mem[base + 0xD3]
            bits = ", ".join(v for b, v in FLAG_BITS.items() if flags & b) or "none"
            left = mem[base + 0x32F]
            guard = (("AUTO (no limit)" if left == 255 else f"AUTO ({left} left)") if mem[base + 0x118]
                     else "MANUAL / not chosen")
            print(f"  fighter {i + 1}: +0x102 = {number:#04x}  +0xD3 = {flags:#04x} ({bits})  "
                  f"-> {describe_version(number, flags)}; guard {guard}")
            modes = [f"{n} ({mem[base + o]:#04x})" for o, n in HYPER_MODES if mem[base + o]] or ["NORMAL"]
            s1, s2 = mem[base + 0x122], mem[base + 0x123]
            seen = SISM_SEEN.get((s1, s2), {(0, 0): "none", (0, 0xFF): "none (CLASSICAL's values)"}.get((s1, s2), "unknown"))
            if mem[base + 0xF0] & 0x80:
                seen = "pink (CLASSICAL with bit 0x80)"
            print(f"    mode {', '.join(modes)}; +0x122/+0x123 = {s1:#04x}/{s2:#04x} (S-ISM: {seen}); "
                  f"ISM (+0x132) = {ISMS.get(mem[base + 0x132], hex(mem[base + 0x132]))}")


# sub_usa.bin (X[203], loaded at 0x1C8000) picks the music: its routine 0x1CA3E0 returns
# Y = table[game][command] (u16), with the tables' addresses by game (1-9) at 0x1D4CF0. Game 9's list
# is Hyper's (Alpha 3's commands, then Alpha's, Alpha 2's and game 4's themes). The lists overlap, so
# each is read up to the next list's start; trailing zeros are dropped.
SUB_USA = 203
MUSIC_TABLES = 0x1D4CF0
MUSIC_GAMES = 9


def music_tables(args):
    iso = Iso(find_iso(args.iso))
    base, entries = afs_entries(iso, "X_DATA.BIN")
    off, size = entries[SUB_USA]
    data = unlz16(iso.read(base + off, size))
    h = mwo3_header(data)
    if h["name"] != "sub_usa.bin":
        sys.exit(f"X[{SUB_USA}] is {h['name']!r}, not sub_usa.bin: a different disc?")
    load = h["base"]
    ptrs = struct.unpack_from(f"<{MUSIC_GAMES}I", data, MUSIC_TABLES - load)
    starts = sorted(set(ptrs)) + [MUSIC_TABLES - 8]
    tables, used = {}, set()
    for game, p in enumerate(ptrs, 1):
        nxt = min(q for q in starts if q > p)
        ys = list(struct.unpack_from(f"<{(nxt - p) // 2}H", data, p - load))
        while ys and ys[-1] == 0:
            ys.pop()
        tables[str(game)] = {"address": hex(p), "y": ys}
        used.update(y for y in ys if y)
        print(f"game {game}: {len(ys)} commands, Y {min(ys)}-{max(ys)}")
    out = {"source": "sub_usa.bin 0x1CA3E0: Y = table[game][command]; tables by game at 0x1D4CF0",
           "tables": tables, "unreferenced_y": [y for y in range(588) if y not in used]}
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w") as f:
        json.dump(out, f, indent=1)
    print(f"{len(out['unreferenced_y'])} of 588 tracks are in no list -> {args.out}")


# Hyper's character versions (H5). zero9_usa.bin (X[544], loads at 0x1F0000) widened Alpha 3's
# per-character tables (32 entries, 0x67D678 ... in zero3) to 128 entries indexed by the version number
# (+0x102). The tables below give each version its own block; see docs/HYPER_ALPHA.md "H5".
ZERO9 = 544
VERSION_TABLES = {  # address: (name, what it holds)
    0x6452E0: ("palette", "colours: rows of 15 CPS2 colours (0RGB, 4 bits each) + a pad word"),
    0x6A7A94: ("t6a7a94", "lists of pointers (A3MX 06_data section 7's source)"),
    0x6A6BD4: ("t6a6bd4", "hit boxes and attack records (06_data sections 8-13)"),
    0x6A6DD4: ("t6a6dd4", "06_data sections 14-20"),
    0x6A6FD4: ("t6a6fd4", "small per-version records"),
    0x6AD414: ("t6ad414", "Alpha 3's 0x67F9F8"),
    0x6AD814: ("t6ad814", "Alpha 3's 0x67FAF8"),
    0x6ADA14: ("t6ada14", "Alpha 3's 0x67FB78"),
    0x6ADC14: ("t6adc14", "Alpha 3's 0x67FBF8"),
    0x6ADE14: ("t6ade14", "Alpha 3's 0x67FC78"),
    0x6AE614: ("t6ae614", "Alpha 3's 0x67FE78"),
    0x75C478: ("hud0", "HUD sprite strip (likely the gauge frame)"),
    0x75C67C: ("hud1", "HUD sprite strip"),
    0x75C880: ("hud2", "HUD sprite strip"),
    0x75CA84: ("hud3", "HUD sprite strip"),
}
VERSION_FLAGS_TABLE = 0x5C2490   # byte per version number: the +0xD3 flags
A3_DEFAULT_BLOCK = 0x6AF9CC      # Alpha 3 Ryu's t6a6bd4 block: unused slots point here
CHARACTER_NAMES = [  # A3MX's character numbering (assets/us/characters/<nn>_<name>)
    "ryu", "ken", "akuma", "charlie", "chunli", "adon", "sodom", "guy", "birdie", "rose", "bison",
    "sagat", "dan", "sakura", "rolento", "dhalsim", "zangief", "gen", "chunli", "gen", "sodom",
    "balrog", "cammy", "evil_ryu", "honda", "blanka", "mika", "cody", "vega", "karin", "juli", "juni"]
VERSION_GROUPS = ((0x30, "alpha"), (0x40, "alpha2"), (0x60, "alpha2gold"))


def _u32(data, load, addr):
    return struct.unpack_from("<I", data, addr - load)[0]


def palette_to_a3mx(block, transparent=0):
    """Hyper's palette rows (15 colours 0x0RGB + a pad word) -> A3MX's fighter-c layout: ABGR1555 rows of
    16, entry 0 the transparent colour, 4-bit channels doubled (checked against A3MX's own files)."""
    words = struct.unpack(f"<{len(block) // 2}H", block)
    out = []
    for row in range(len(words) // 16):
        out.append(transparent)
        for c in words[row * 16:row * 16 + 15]:
            r, g, b = (c >> 8) & 15, (c >> 4) & 15, c & 15
            out.append(0x8000 | (b * 2) << 10 | (g * 2) << 5 | (r * 2))
    return struct.pack(f"<{len(out)}H", *out)


def write_palette_png(path, abgr, scale=8):
    import zlib
    words = struct.unpack(f"<{len(abgr) // 2}H", abgr)
    rows = len(words) // 16
    raw = bytearray()
    for y in range(rows * scale):
        raw.append(0)
        for x in range(16 * scale):
            c = words[(y // scale) * 16 + x // scale]
            r, g, b = (c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3
            raw += bytes((r, g, b, 255 if (c & 0x8000) else 0))
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 16 * scale, rows * scale, 8, 6, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def _a3mx_palette(a3mx, char):
    """A3MX's own palette file for an Alpha 3 character, if the assets are there."""
    import glob
    hits = glob.glob(os.path.join(a3mx, "characters", f"{char:02x}_*", "fighter", "fighter_data",
                                  "02_fighter-c_*.bin"))
    return open(hits[0], "rb").read() if hits else None


def versions(args):
    iso = Iso(find_iso(args.iso))
    base, entries = afs_entries(iso, "X_DATA.BIN")
    off, size = entries[ZERO9]
    data = unlz16(iso.read(base + off, size))
    h = mwo3_header(data)
    if h["name"] != "zero9_usa.bin":
        sys.exit(f"X[{ZERO9}] is {h['name']!r}, not zero9_usa.bin: a different disc?")
    convert_versions(data, h["base"], args.out, args.a3mx)


def convert_versions(data, load, out_root, a3mx):
    tables = {t: [_u32(data, load, t + 4 * v) for v in range(128)] for t in VERSION_TABLES}
    flags = data[VERSION_FLAGS_TABLE - load:VERSION_FLAGS_TABLE - load + 128]
    # a block runs to the next address any version table points at (or a table itself starts)
    starts = sorted({a for col in tables.values() for a in col} | set(VERSION_TABLES)
                    | {t + 512 for t in VERSION_TABLES} | {load + len(data)})
    def extent(a):
        i = bisect.bisect_right(starts, a)
        return min(starts[i] - a, 0x10000)

    # Alpha 3: check the conversion against A3MX's own palettes, row by row at the CPS2's precision
    # (4 bits a channel; MAX nudged a few low bits). The rows come in sets of five (one set a button
    # colour); for most characters rows 3 and 4 of each set differ in A3MX, the other three match.
    first_ok = rows_same = rows_total = chars = 0
    for char in range(0x20):
        mine = _a3mx_palette(a3mx, char) if a3mx else None
        if not mine:
            continue
        a = tables[0x6452E0][char]
        conv = palette_to_a3mx(data[a - load:a - load + extent(a)], struct.unpack_from("<H", mine)[0])
        same = []
        for r in range(min(len(conv), len(mine)) // 32):
            x = struct.unpack_from("<16H", conv, 32 * r)[1:]
            y = struct.unpack_from("<16H", mine, 32 * r)[1:]
            same.append(all((p >> 1) & 0x3DEF == (q >> 1) & 0x3DEF for p, q in zip(x, y)))
        chars += 1
        rows_total += len(same)
        rows_same += sum(same)
        first_ok += all(ok for r, ok in enumerate(same) if r % 5 < 3)
    if chars:
        print(f"palette check: {rows_same} of {rows_total} Alpha 3 rows equal A3MX's; rows 0-2 of every set of "
              f"five equal for {first_ok} of {chars} characters (rows 3-4 differ in A3MX for most)")

    manifest = {"source": "zero9_usa.bin version tables (docs/HYPER_ALPHA.md, H5)",
                "tables": {f"{t:#x}": {"name": n, "holds": w} for t, (n, w) in VERSION_TABLES.items()},
                "versions": []}
    written = 0
    for v in range(0x30, 0x80):
        group_first, group = max((g for g in VERSION_GROUPS if g[0] <= v), key=lambda g: g[0])
        slot = v - group_first
        classical = bool(flags[v] & 0x08)
        # the character: slots below 0x12 are A3MX's numbering; Classical and later slots are named after
        # the lower slot in the group whose move blocks (or t6ad814 blocks) they share
        char, kind = (slot, "") if slot < 0x12 and not classical else (None, "")
        if char is None:
            for t in (0x6A6BD4, 0x6AD814):
                same = [s for s in range(group_first, v) if tables[t][s] == tables[t][v]]
                if same:
                    char = same[0] - group_first
                    break
            kind = "_classical" if classical else f"_alt{v:02x}"
        # a copy: its move blocks and its colours are both a lower slot's (not necessarily the same one)
        lower = range(group_first, v)
        dup = (any(tables[0x6A6BD4][s] == tables[0x6A6BD4][v] for s in lower)
               and any(tables[0x6452E0][s] == tables[0x6452E0][v] for s in lower))
        a3 = char if char is not None else slot
        own = [t for t in VERSION_TABLES if tables[t][v] != tables[t][a3]]
        unused = bool(dup) or (tables[0x6A6BD4][v] == A3_DEFAULT_BLOCK and v != group_first) or not own
        if char is None:
            name, folder = f"extra_{v:02x}", os.path.join(out_root, "characters", "extra", f"{group}_{v:02x}")
        else:
            name = CHARACTER_NAMES[char]
            folder = os.path.join(out_root, "characters", f"{char:02x}_{name}", group + kind)
        char = a3
        entry = {"number": f"{v:#04x}", "group": group, "character": f"{char:#04x}", "name": name + kind,
                 "classical": classical, "flags": f"{flags[v]:#04x}", "probably_unused": unused,
                 "blocks": {}}
        dst = None if unused else folder
        if dst:
            os.makedirs(os.path.join(dst, "raw"), exist_ok=True)
        for t, (tname, _) in VERSION_TABLES.items():
            a = tables[t][v]
            n = extent(a)
            blk = data[a - load:a - load + n]
            entry["blocks"][tname] = {"address": f"{a:#x}", "bytes": n,
                                      "shared_with_alpha3": a == tables[t][char]}
            if not dst:
                continue
            if t == 0x6452E0:
                mine = _a3mx_palette(a3mx, char) if a3mx else None
                abgr = palette_to_a3mx(blk, struct.unpack_from("<H", mine)[0] if mine else 0)
                with open(os.path.join(dst, "palette.bin"), "wb") as f:
                    f.write(abgr)
                write_palette_png(os.path.join(dst, "palette.png"), abgr)
            elif a != tables[t][char]:
                with open(os.path.join(dst, "raw", f"{tname}.bin"), "wb") as f:
                    f.write(blk)
        if dst:
            with open(os.path.join(dst, "version.json"), "w") as f:
                json.dump(entry, f, indent=1)
            written += 1
        manifest["versions"].append(entry)
    os.makedirs(out_root, exist_ok=True)
    with open(os.path.join(out_root, "versions.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    print(f"{written} versions written under {out_root} (of 80 version numbers; the rest look unused)")



def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("survey", help="list the disc's archives and what each entry is")
    s.add_argument("--iso")
    s.add_argument("--out", default=os.path.join("work", "anthology", "survey.json"))
    s.add_argument("--hash", action="store_true", help="SHA-1 the whole image (about 30 s)")
    s.set_defaults(func=survey)
    m = sub.add_parser("music", help="decode Y_DATA.BIN's ADX streams to OGG, keeping loop points")
    m.add_argument("--iso")
    m.add_argument("--out", default=os.path.join("assets", "us", "anthology", "music"))
    m.add_argument("--start", type=int, default=0, help="first track")
    m.add_argument("--limit", type=int, default=0, help="number of tracks (default: all)")
    m.add_argument("--seconds", type=float, default=0)
    m.set_defaults(func=music)
    u = sub.add_parser("unpack", help="unpack X_DATA.BIN entries packed with the halfword LZ")
    u.add_argument("--iso")
    u.add_argument("--out", default=os.path.join("work", "anthology", "x"))
    u.add_argument("index", nargs="*", type=int, help="entry numbers (default: every MWo3 / MOMO entry)")
    u.set_defaults(func=unpack)
    mt = sub.add_parser("music-tables", help="which Y_DATA tracks each game's music commands play")
    mt.add_argument("--iso")
    mt.add_argument("--out", default=os.path.join("tools", "data", "anthology_music_tables.json"))
    mt.set_defaults(func=music_tables)
    ve = sub.add_parser("versions", help="Hyper's character versions: palettes (A3MX format) and raw data blocks")
    ve.add_argument("--iso")
    ve.add_argument("--out", default=os.path.join("assets", "us", "anthology"))
    ve.add_argument("--a3mx", default=os.path.join("assets", "us"),
                    help="A3MX's extracted assets, to take each character's transparent colour and check palettes")
    ve.set_defaults(func=versions)
    st = sub.add_parser("state", help="each fighter's version in PCSX2 save states of Hyper")
    st.add_argument("file", nargs="+")
    st.set_defaults(func=state)
    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
