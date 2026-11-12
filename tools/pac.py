#!/usr/bin/env python3
"""The game's .pac archives: read, unpack, convert, and pack them back.

Format (docs/PAC_FORMAT.md):
    0x000  u32 count         number of entries
    0x004  u32 data_size     bytes after the 2 KB header (the file is 0x800 + data_size long)
    0x008  count x { u32 type, u32 size }
    0x800  entry data, in order, each starting on a 2 KB boundary (zero padded)

The type word says where the game puts the entry: its high 16 bits are one flag (the kind), its low
16 bits a slot / page number the loader uses (see KINDS).

Usage:
    python tools/pac.py list FILE.pac
    python tools/pac.py unpack FILE.pac OUTDIR [work/analysis.image.bin]
                                                      (raw entries + converted ones + manifest.json;
                                                       indexed textures in colour where
                                                       tools/data/palette_pairs.json pairs them)
    python tools/pac.py pack OUTDIR FILE.pac          (rebuild from a manifest.json and raw entries)
Standard library only.
"""
import json, os, struct, sys, zlib

SECTOR = 0x800

# kind (type & 0xFFFF0000) -> (short name, what it is, as far as is known)
KINDS = {
    0x00000000: ('data', 'misc data (stage and screen layouts, tables)'),
    0x00010000: ('text', 'text: bios, win quotes, ending scripts'),
    0x00020000: ('data20000', 'small tables'),
    0x00100000: ('fighter-a', 'fighter data, part A (per character)'),
    0x00200000: ('fighter-sprites', 'fighter data, part B: the largest block of a character (sprites / animation)'),
    0x00400000: ('fighter-c', 'fighter data, part C'),
    0x01000000: ('data1000000', 'data (index / layout for the following entries)'),
    0x02000000: ('data2000000', 'data (sprite / animation sets)'),
    0x04000000: ('palette', 'palettes (CLUTs) for the 4 and 8-bit textures'),
    0x10000000: ('texture', 'textures: PowerVR header (GBIX / PVRT), linear pixels'),
    0x20000000: ('sound', 'sound: SAS bank header (PPHD) and samples, MIDI sequences (MThd)'),
    0x40000000: ('voice', 'voices: SAS bank header (PPHD) and samples'),
    0x80000000: ('data80000000', 'data'),
}


def kind_name(t):
    return KINDS.get(t & 0xFFFF0000, ('kind%08X' % (t & 0xFFFF0000), ''))[0]


def align(n):
    return (n + SECTOR - 1) & ~(SECTOR - 1)


def read(path_or_bytes, with_padding=False):
    """[(type, bytes)] from a .pac file or its bytes; with_padding: [(type, bytes, padding)], the
    padding being the bytes between the entry's end and the next 2 KB boundary. Capcom's packer left
    stale buffer contents there, so an exact rebuild needs them."""
    b = path_or_bytes if isinstance(path_or_bytes, (bytes, bytearray)) else open(path_or_bytes, 'rb').read()
    count, data_size = struct.unpack_from('<II', b, 0)
    if 8 + 8 * count > SECTOR or SECTOR + data_size != len(b):
        raise ValueError('not a .pac archive (count %d, data size %#x, file %#x)' % (count, data_size, len(b)))
    entries, off = [], SECTOR
    for i in range(count):
        t, size = struct.unpack_from('<II', b, 8 + 8 * i)
        e = bytes(b[off:off + size]); pad = bytes(b[off + size:off + align(size)])
        entries.append((t, e, pad) if with_padding else (t, e))
        off += align(size)
    return entries


def build(entries):
    """The bytes of a .pac holding [(type, bytes)] or [(type, bytes, padding)]. Padding is used when
    it fits the entry's last sector exactly (an unchanged entry); otherwise zeros."""
    if 8 + 8 * len(entries) > SECTOR: raise ValueError('too many entries for one header sector')
    head = struct.pack('<II', len(entries), sum(align(len(e[1])) for e in entries))
    head += b''.join(struct.pack('<II', e[0], len(e[1])) for e in entries)
    out = bytearray(head.ljust(SECTOR, b'\0'))
    for e in entries:
        d = e[1]; pad = e[2] if len(e) > 2 else b''
        out += d + (pad if len(d) + len(pad) == align(len(d)) else b'\0' * (align(len(d)) - len(d)))
    return bytes(out)


# ---------------------------------------------------------------- textures
def pvr_header(e):
    """(offset of pixels, pixel format, data format, width, height), or None."""
    p = e.find(b'PVRT', 0, 32)
    if p < 0: return None
    _, pf, df, _, w, h = struct.unpack_from('<IBBHHH', e, p + 4)
    return p + 16, pf, df, w, h


TEXTURE_FORMATS = {1: ('16-bit', 16), 5: ('4-bit indexed', 4), 7: ('8-bit indexed', 8)}
# The PSP's 16-bit colours (its GE formats 5551, 565, 4444) keep red in the low bits: written from the
# top bit down they are ABGR1555, BGR565 and ABGR4444. The PowerVR header's pixel format numbers them
# 0, 1, 2. Channels widen to 8 bits as the PSP does (the high bits repeated in the low ones), so a PNG
# shows exactly the colours the game draws, and narrowing back (src/platform/asset_pac.c) is exact.
PIXEL_FORMATS = {0: 'ABGR1555', 1: 'BGR565', 2: 'ABGR4444'}


def rgba16(v, pf):
    def w5(x): return x << 3 | x >> 2
    def w6(x): return x << 2 | x >> 4
    if pf == 0: return (w5(v & 31), w5(v >> 5 & 31), w5(v >> 10 & 31), 255 if v & 0x8000 else 0)
    if pf == 1: return (w5(v & 31), w6(v >> 5 & 63), w5(v >> 11 & 31), 255)
    return ((v & 15) * 17, (v >> 4 & 15) * 17, (v >> 8 & 15) * 17, (v >> 12 & 15) * 17)


def png(path, w, h, rows, color_type, palette=None, trns=None):
    """Write a PNG: rows are bytes per row (8-bit RGBA for type 6, indices for type 3)."""
    def chunk(tag, data): return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)
    bit_depth = 8
    out = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, bit_depth, color_type, 0, 0, 0))
    if palette is not None: out += chunk(b'PLTE', palette)
    if trns is not None: out += chunk(b'tRNS', trns)
    out += chunk(b'IDAT', zlib.compress(b''.join(b'\0' + r for r in rows), 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(out)


GREY16 = bytes(v for i in range(16) for v in (i * 17,) * 3)
GREY256 = bytes(v for i in range(256) for v in (i,) * 3)


def texture_to_png(e, path, palette=None):
    """Converts a texture entry; returns a description, or None if it is not one this knows. An indexed
    texture is written in its colours when its palette is given (16 / 256 16-bit colours, the
    PSP's ABGR1555; index 0 transparent, as the game draws it), otherwise in a grey ramp that holds
    the indices."""
    hd = pvr_header(e)
    if not hd: return None
    off, pf, df, w, h = hd
    if df not in TEXTURE_FORMATS or w == 0 or h == 0: return None
    d = e[off:]
    if df == 1:
        rows = []
        for y in range(h):
            row = bytearray()
            for x in range(w): row += bytes(rgba16(struct.unpack_from('<H', d, (y * w + x) * 2)[0], pf))
            rows.append(bytes(row))
        png(path, w, h, rows, 6)
    else:
        rows = []
        for y in range(h):
            if df == 5: row = bytes((d[(y * w + x) >> 1] >> (((y * w + x) & 1) * 4)) & 15 for x in range(w))
            else: row = d[y * w:(y + 1) * w]
            rows.append(bytes(row))
        if palette:
            cols = [rgba16(v, 0) for v in palette]
            plte = b''.join(bytes(c[:3]) for c in cols)
            trns = bytes([0] + [c[3] for c in cols[1:]])
            png(path, w, h, rows, 3, plte, trns)
        else:
            # the palette is chosen at draw time; a grey ramp stands in (the indices are exact)
            png(path, w, h, rows, 3, GREY16 if df == 5 else GREY256)
    what = PIXEL_FORMATS.get(pf, 'pf%d' % pf) if df == 1 else ('(own palette)' if palette else '(palette not known: grey indices)')
    return '%dx%d %s %s' % (w, h, TEXTURE_FORMATS[df][0], what)


def palette_to_png(e, path):
    """A palette entry as a strip of 16 colours per row (16-bit ABGR1555 colours, as the textures')."""
    n = len(e) // 2
    if n == 0 or len(e) % 2: return None
    cols = [rgba16(struct.unpack_from('<H', e, i * 2)[0], 0) for i in range(n)]
    w = 16; h = (n + w - 1) // w
    rows = [b''.join(bytes(cols[y * w + x]) if y * w + x < n else b'\0\0\0\0' for x in range(w)) for y in range(h)]
    png(path, w, h, rows, 6)
    return '%d colours (ABGR1555)' % n


def text_view(e):
    """Text entries: printable characters as they are, everything else as \\xNN."""
    return ''.join(chr(c) if 32 <= c < 127 else ('\n' if c == 10 else '\\x%02X' % c) for c in e)


# ---------------------------------------------------------------- unpack / pack
def magic_ext(t, e):
    m = e[:4]
    if m == b'MThd': return 'mid'
    if m == b'PPHD': return 'phd'
    if m in (b'GBIX', b'PVRT'): return 'pvr'
    return 'bin'


# ---------------------------------------------------------------- palettes of the indexed textures
class Palettes:
    """Where each indexed texture's colours are (tools/data/palette_pairs.json, made by
    tools/pair_palettes.py) and reading them: from the disc's archives (the folder the archive being
    unpacked is in) or the program (work/analysis.image.bin)."""
    def __init__(self, pairs_path=None, program_image=None):
        pairs_path = pairs_path or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data', 'palette_pairs.json')
        self.pairs = json.load(open(pairs_path))['textures'] if os.path.exists(pairs_path) else {}
        self.image = open(program_image, 'rb').read() if program_image and os.path.exists(program_image) else None
        self.archives = {}

    def colours(self, archive, index, data_dir, count):
        src = self.pairs.get('%s/%d' % (archive, index))
        if not src: return None, None
        if 'program' in src:
            if self.image is None: return None, None
            o = int(src['program'], 16) - 0x08804000
            b = self.image[o:o + 2 * count]
        else:
            name = src['archive']
            if name not in self.archives:
                p = os.path.join(data_dir, name + '.pac')
                self.archives[name] = read(p) if os.path.exists(p) else None
            ents = self.archives[name]
            if not ents or src['entry'] >= len(ents): return None, None
            b = ents[src['entry']][1][src['offset']:src['offset'] + 2 * count]
        if len(b) != 2 * count: return None, None
        return list(struct.unpack('<%dH' % count, b)), {k: v for k, v in src.items() if k in ('archive', 'entry', 'offset', 'program')}


def unpack(pac_path, out_dir, convert=True, palettes=None, by_kind=True):
    """Unpack an archive into OUT_DIR: manifest.json at the top, each entry's files in a subfolder for
    its kind (textures/, palettes/, text/, sound/, voices/, fighter_data/, data/: tools/asset_layout.py),
    or all at the top with by_kind=False. The manifest names every file by its path in the folder."""
    entries = read(pac_path, with_padding=True)
    os.makedirs(out_dir, exist_ok=True)
    sub = None
    if by_kind:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import asset_layout
        sub = asset_layout.kind_folder
    name = os.path.splitext(os.path.basename(pac_path))[0]
    manifest = {'archive': os.path.basename(pac_path), 'entries': []}
    for i, (t, e, pad) in enumerate(entries):
        base = '%02d_%s_%08X' % (i, kind_name(t), t)
        if sub:                                 # posix separators: the manifest is the same on every system
            base = sub(kind_name(t)) + '/' + base
            os.makedirs(os.path.join(out_dir, sub(kind_name(t))), exist_ok=True)
        raw = base + '.' + magic_ext(t, e)
        open(os.path.join(out_dir, raw), 'wb').write(e)
        item = {'index': i, 'type': '0x%08X' % t, 'kind': kind_name(t), 'size': len(e), 'raw': raw}
        if pad.strip(b'\0'):                    # stale bytes after the entry: kept for exact rebuilds
            open(os.path.join(out_dir, base + '.pad'), 'wb').write(pad)
            item['padding'] = base + '.pad'
        if convert:
            kind = t & 0xFFFF0000
            if kind in (0x10000000, 0x00800000):
                hd = pvr_header(e)
                cols = src = None
                if hd and hd[2] in (5, 7) and palettes:
                    cols, src = palettes.colours(name, i, os.path.dirname(os.path.abspath(pac_path)), 16 if hd[2] == 5 else 256)
                desc = texture_to_png(e, os.path.join(out_dir, base + '.png'), cols)
                if desc:
                    item['png'] = base + '.png'; item['format'] = desc
                    if cols:                     # for painting in colour: converted back to the nearest index
                        item['palette'] = src
                        item['palette_colours'] = ''.join('%04X' % v for v in cols)
            elif kind == 0x04000000:
                desc = palette_to_png(e, os.path.join(out_dir, base + '.png'))
                if desc: item['png'] = base + '.png'; item['format'] = desc
            elif kind == 0x00010000:
                open(os.path.join(out_dir, base + '.txt'), 'w', encoding='utf-8').write(text_view(e))
                item['text'] = base + '.txt'
        manifest['entries'].append(item)
    json.dump(manifest, open(os.path.join(out_dir, 'manifest.json'), 'w'), indent=1)
    return manifest


def pack(in_dir, pac_path):
    """Rebuild an archive from an unpacked folder: each entry from its raw file, in manifest order.
    (Edit a raw file, or replace it, then pack; converted PNG / TXT views are not read back yet.)"""
    m = json.load(open(os.path.join(in_dir, 'manifest.json')))
    def rd(name): return open(os.path.join(in_dir, name), 'rb').read() if name else b''
    entries = [(int(it['type'], 16), rd(it['raw']), rd(it.get('padding'))) for it in m['entries']]
    open(pac_path, 'wb').write(build(entries))
    return len(entries)


def main():
    a = sys.argv[1:]
    if len(a) >= 2 and a[0] == 'list':
        for i, (t, e) in enumerate(read(a[1])):
            hd = pvr_header(e) if t & 0xFFFF0000 == 0x10000000 else None
            extra = ('  %dx%d %s' % (hd[3], hd[4], TEXTURE_FORMATS.get(hd[2], ('?',))[0])) if hd else ''
            print('%3d  %08X  %-16s %8d  %s%s' % (i, t, kind_name(t), len(e), e[:4].hex(), extra))
    elif len(a) in (3, 4) and a[0] == 'unpack':     # [PROGRAM_IMAGE]: work/analysis.image.bin, for palettes kept in the program
        print('%d entries' % len(unpack(a[1], a[2], palettes=Palettes(program_image=a[3] if len(a) == 4 else None))['entries']))
    elif len(a) == 3 and a[0] == 'pack':
        print('%d entries' % pack(a[1], a[2]))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
