#!/usr/bin/env python3
"""Writes PNG files the way image editors do, for the C decoder's test (tests/asset_pac_test.c):
every colour type and bit depth, every row filter, stored / fixed / dynamic deflate blocks, tRNS.
Each NAME.png comes with NAME.rgba: the pixels it must decode to (8-bit RGBA, rows top to bottom).

Usage: png_cases.py OUTDIR      (standard library only)
"""
import os, struct, sys, zlib


def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)


def paeth(a, b, c):
    p = a + b - c; pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def filtered(rows, bpp, filters):
    out, prev = b'', bytes(len(rows[0]))
    for y, row in enumerate(rows):
        ft = filters[y % len(filters)]
        f = bytearray()
        for x, v in enumerate(row):
            a = row[x - bpp] if x >= bpp else 0; b = prev[x]; c = prev[x - bpp] if x >= bpp else 0
            f.append((v - (0, a, b, (a + b) >> 1, paeth(a, b, c))[ft]) & 255)
        out += bytes([ft]) + bytes(f); prev = row
    return out


def write(path, w, h, ctype, depth, rows, expect, filters=(0,), level=9, plte=None, trns=None, split_idat=False):
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    bpp = max(1, channels * depth // 8)
    data = zlib.compress(filtered(rows, bpp, filters), level)
    out = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, depth, ctype, 0, 0, 0))
    out += chunk(b'tEXt', b'Software\0an image editor')            # chunks the decoder must skip
    if plte: out += chunk(b'PLTE', plte)
    if trns is not None: out += chunk(b'tRNS', trns)
    if split_idat:
        half = len(data) // 2
        out += chunk(b'IDAT', data[:half]) + chunk(b'IDAT', data[half:])
    else:
        out += chunk(b'IDAT', data)
    out += chunk(b'IEND', b'')
    open(path + '.png', 'wb').write(out)
    open(path + '.rgba', 'wb').write(bytes(expect))


def pack_bits(vals, depth):
    out, acc, n = bytearray(), 0, 0
    for v in vals:
        acc = (acc << depth) | v; n += depth
        if n == 8: out.append(acc); acc, n = 0, 0
    if n: out.append(acc << (8 - n))
    return bytes(out)


def main():
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    w, h = 37, 23                        # odd sizes: partial bytes at the end of rows
    px = [((x * 7 + y * 3) & 255, (x * 13) & 255, (y * 11) & 255, (x * y * 5 + 3) & 255) for y in range(h) for x in range(w)]
    for filters, level, tag in (((0,), 9, 'f0'), ((1,), 9, 'f1'), ((2,), 9, 'f2'), ((3,), 9, 'f3'), ((4,), 9, 'f4'),
                                ((0, 1, 2, 3, 4), 9, 'mixed'), ((4,), 0, 'stored'), ((1,), 1, 'fast')):
        rows = [bytes(v for p in px[y * w:(y + 1) * w] for v in p) for y in range(h)]
        write(os.path.join(out, 'rgba8_' + tag), w, h, 6, 8, rows, [v for p in px for v in p], filters, level)
    # RGB, RGB with tRNS, 16-bit RGBA
    rows = [bytes(v for p in px[y * w:(y + 1) * w] for v in p[:3]) for y in range(h)]
    write(os.path.join(out, 'rgb8'), w, h, 2, 8, rows, [v for p in px for v in p[:3] + (255,)], (4, 1))
    key = px[5][:3]
    write(os.path.join(out, 'rgb8_trns'), w, h, 2, 8, rows,
          [v for p in px for v in p[:3] + ((0,) if p[:3] == key else (255,))], (2,), trns=struct.pack('>HHH', *key))
    rows = [b''.join(struct.pack('>HHHH', *(c * 256 + (c ^ 0x5A) for c in p)) for p in px[y * w:(y + 1) * w]) for y in range(h)]
    write(os.path.join(out, 'rgba16'), w, h, 6, 16, rows, [v for p in px for v in p], (3,), split_idat=True)
    # grey: 1, 2, 4, 8, 16 bits; grey + alpha
    for depth in (1, 2, 4, 8, 16):
        m = (1 << depth) - 1
        vals = [[((x + y) * (2731 if depth == 16 else 1)) & m for x in range(w)] for y in range(h)]
        rows = [pack_bits(r, depth) if depth < 8 else bytes(r) if depth == 8 else b''.join(struct.pack('>H', v) for v in r) for r in vals]
        g = lambda v: v * 255 // m if depth <= 8 else v >> 8
        write(os.path.join(out, 'grey%d' % depth), w, h, 0, depth, rows, [c for r in vals for v in r for c in (g(v),) * 3 + (255,)], (1, 4))
    rows = [bytes(v for p in px[y * w:(y + 1) * w] for v in (p[0], p[3])) for y in range(h)]
    write(os.path.join(out, 'greyalpha8'), w, h, 4, 8, rows, [c for p in px for c in (p[0],) * 3 + (p[3],)], (4,))
    # palette: 4 and 8 bits, with tRNS
    pal = [((i * 40) & 255, (i * 90) & 255, (255 - i * 15) & 255) for i in range(16)]
    alpha = [255 - i * 16 for i in range(10)]
    for depth in (4, 8):
        vals = [[(x * 3 + y) & 15 for x in range(w)] for y in range(h)]
        rows = [pack_bits(r, depth) if depth < 8 else bytes(r) for r in vals]
        exp = [c for r in vals for i in r for c in pal[i] + ((alpha[i] if i < len(alpha) else 255),)]
        write(os.path.join(out, 'palette%d' % depth), w, h, 3, depth, rows, exp, (0,),
              plte=b''.join(bytes(c) for c in pal), trns=bytes(alpha))
    # a big one: dynamic Huffman blocks with long matches
    W, H = 256, 256
    big = [((x // 8) * 9 & 255, (y // 4) * 5 & 255, 128, 255) for y in range(H) for x in range(W)]
    rows = [bytes(v for p in big[y * W:(y + 1) * W] for v in p) for y in range(H)]
    write(os.path.join(out, 'big'), W, H, 6, 8, rows, [v for p in big for v in p], (0, 2))
    print('%d PNG cases in %s' % (len([f for f in os.listdir(out) if f.endswith('.png')]), out))


if __name__ == '__main__':
    main()
