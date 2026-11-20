#!/usr/bin/env python3
"""The GPU renderer (src/platform/ge_gl.c) against the software renderer, on one scenario.

The parity tests run the software renderer only. This runs a scenario with the software renderer and
with the GPU renderer at each scale asked for (default 1 and 2), headless through EGL (a build made
with -DA3MX_EGL=ON; Mesa's llvmpipe is enough), in one screen size (NORMAL or WIDE NATIVE), and checks:

  - the game is the same: every frame's memory and fighters' hashes match the software run's (the
    renderer must never change what the game does);
  - the pictures are close: every dumped frame of each GPU run, scaled down to 480 x 272, is compared
    with the software run's. The two renderers differ slightly (filtering, the edges of triangles and
    of upscaled text), so both pictures are averaged over --block x --block squares first, which evens
    out edge noise but not missing, misplaced or wrongly blended drawing: a frame fails when more than
    --max-bad percent of those squares differ by more than --tolerance in a colour channel. Measured on
    arcade_fight (Mesa 25.2 llvmpipe): at most 0.05% at scale 1 and 0.39% at scale 2; one 40 x 60
    sprite left out is 1.8%.

On a failure the worst frames are written as software | GPU | difference PNGs into the output folder.

    python tests/gl_test.py --exe build-egl/a3mx --work work --scenario tests/scenarios/arcade_fight.txt --size wide
"""
import argparse, os, shutil, struct, subprocess, sys, time, zlib

try:
    import numpy as np
except ImportError:          # the pure-Python path is slower but gives the same answers
    np = None


def read_scenario(path):
    sc = {'frames': 600, 'input': None, 'args': [], 'config': []}
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        key, _, val = line.partition(' ')
        if key == 'frames': sc['frames'] = int(val)
        elif key == 'input': sc['input'] = val
        elif key == 'args': sc['args'] = val.split()
        elif key == 'config': sc['config'].append(val.strip())
    return sc


def read_png(path):
    """The game's own PNGs (src/platform/png.c): RGBA8, filter 0 on every row. Returns (w, h, bytes)."""
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', path
    i, w, h, idat = 8, 0, 0, b''
    while i < len(data):
        n, = struct.unpack('>I', data[i:i + 4]); kind = data[i + 4:i + 8]; body = data[i + 8:i + 8 + n]
        if kind == b'IHDR': w, h = struct.unpack('>II', body[:8])
        elif kind == b'IDAT': idat += body
        i += 12 + n
    raw = zlib.decompress(idat)
    row = w * 4 + 1
    assert all(raw[y * row] == 0 for y in range(h)), path + ': not written by the game (filtered rows)'
    return w, h, b''.join(raw[y * row + 1:(y + 1) * row] for y in range(h))


def write_png(path, w, h, rgb):
    raw = b''.join(b'\0' + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    def chunk(k, b): return struct.pack('>I', len(b)) + k + b + struct.pack('>I', zlib.crc32(k + b) & 0xFFFFFFFF)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                           chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def to_rgb(w, h, rgba, scale):
    """RGB of a frame at 480 x 272: a GPU frame at scale S is averaged over S x S blocks"""
    if np is not None:
        a = np.frombuffer(rgba, np.uint8).reshape(h, w, 4)[:, :, :3].astype(np.uint16)
        if scale > 1:
            a = a.reshape(h // scale, scale, w // scale, scale, 3).sum(axis=(1, 3)) // (scale * scale)
        return a.astype(np.uint8)
    rgb = bytes(b for i, b in enumerate(rgba) if i % 4 != 3)
    if scale == 1:
        return rgb
    ow, oh, out = w // scale, h // scale, bytearray(w // scale * (h // scale) * 3)
    for y in range(oh):
        for x in range(ow):
            for c in range(3):
                s = 0
                for dy in range(scale):
                    base = ((y * scale + dy) * w + x * scale) * 3 + c
                    s += sum(rgb[base + 3 * dx] for dx in range(scale))
                out[(y * ow + x) * 3 + c] = s // (scale * scale)
    return bytes(out)


def blocks(a, k):
    """a 480 x 272 RGB picture averaged over k x k squares"""
    if k <= 1:
        return a
    if np is not None:
        h, w, _ = a.shape
        return (a.reshape(h // k, k, w // k, k, 3).astype(np.uint32).sum(axis=(1, 3)) // (k * k)).astype(np.uint8)
    W, H = 480, 272
    out = bytearray((W // k) * (H // k) * 3)
    for y in range(H // k):
        for x in range(W // k):
            for c in range(3):
                out[(y * (W // k) + x) * 3 + c] = sum(a[((y * k + dy) * W + x * k + dx) * 3 + c] for dy in range(k) for dx in range(k)) // (k * k)
    return bytes(out)


def bad_pixels(a, b, tol):
    """how many pixels differ by more than tol in some channel, and the difference picture"""
    if np is not None:
        d = np.abs(a.astype(np.int16) - b.astype(np.int16)).max(axis=2)
        return int((d > tol).sum()), d.astype(np.uint8)
    n, diff = 0, bytearray(len(a) // 3)
    for i in range(0, len(a), 3):
        m = max(abs(a[i] - b[i]), abs(a[i + 1] - b[i + 1]), abs(a[i + 2] - b[i + 2]))
        diff[i // 3] = m
        if m > tol: n += 1
    return n, diff


def side_by_side(path, soft, gpu, diff):
    """software | GPU | difference (white where over the tolerance's scale), 1440 x 272"""
    W, H = 480, 272
    if np is not None:
        d = np.repeat(np.minimum(diff.astype(np.uint16) * 4, 255).astype(np.uint8)[:, :, None], 3, axis=2)
        write_png(path, W * 3, H, np.concatenate([soft, gpu, d], axis=1).tobytes())
        return
    rows = []
    for y in range(H):
        d = bytes(min(255, diff[y * W + x] * 4) for x in range(W) for _ in range(3))
        rows.append(soft[y * W * 3:(y + 1) * W * 3] + gpu[y * W * 3:(y + 1) * W * 3] + d)
    write_png(path, W * 3, H, b''.join(rows))


def start(a, sc, out, name, renderer, scale):
    d = os.path.join(out, name)
    shutil.rmtree(d, ignore_errors=True); os.makedirs(os.path.join(d, 'dump'))
    size = 'WIDE NATIVE' if a.size == 'wide' else 'NORMAL'
    config = [l for l in sc['config'] if not l.replace(' ', '').lower().startswith('screen_size=')]
    with open(os.path.join(d, 'a3mx.cfg'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(config + ['[screen setting]', 'screen_size=' + size]) + '\n')
    cmd = [a.exe, '--work', a.work, '--save-dir', d, '--log', os.path.join(d, 'run.log'), '--headless', '--fast',
           '--renderer', renderer, '--frames', str(sc['frames']), '--hash-log', os.path.join(d, 'hashes.txt'),
           '--no-assets', '--features', 'default,-shaders', '--dump', os.path.join(d, 'dump'),
           '--dump-every', str(a.every), '--perf-log', os.path.join(d, 'perf.csv')] + sc['args']
    if scale: cmd += ['--scale', str(scale)]
    if sc['input']: cmd += ['--input', sc['input']]
    return cmd, subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)


def hashes(path):
    """frame -> (memory, fighters): the picture's own hash is left out, as the GPU renderer doesn't draw into game memory"""
    rows = {}
    for line in open(path):
        f, state, fb, fighters = line.split()
        rows[int(f)] = (state, fighters)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--exe', required=True); ap.add_argument('--work', required=True)
    ap.add_argument('--scenario', required=True); ap.add_argument('--out', default='gl_test')
    ap.add_argument('--size', choices=('normal', 'wide'), default='normal')
    ap.add_argument('--scales', default='1,2', help='GPU renderer scales to run (default 1,2)')
    ap.add_argument('--every', type=int, default=30, help='dump every Nth frame (default 30)')
    ap.add_argument('--block', type=int, default=4, help='compare averages over squares this wide (default 4)')
    ap.add_argument('--tolerance', type=int, default=32, help='a colour channel may differ by this much (default 32)')
    ap.add_argument('--max-bad', type=float, default=0.75, help='percent of a frame\'s squares allowed over the tolerance (default 0.75)')
    a = ap.parse_args()
    t0 = time.time()
    sc = read_scenario(a.scenario)
    name = os.path.splitext(os.path.basename(a.scenario))[0] + '_' + a.size
    out = os.path.join(a.out, name)
    scales = [int(s) for s in a.scales.split(',') if s]
    runs = [('soft', 'soft', 0)] + [('gl_x%d' % s, 'gl', s) for s in scales]
    # two at a time: llvmpipe uses every core it is given, and the CI machines have two
    procs = []
    for i in range(0, len(runs), 2):
        batch = [(r, start(a, sc, out, *r)) for r in runs[i:i + 2]]
        for r, (cmd, p) in batch:
            if p.wait() != 0:
                print('%s run failed (exit %d): %s\n  log: %s' % (r[0], p.returncode, ' '.join(cmd), os.path.join(out, r[0], 'run.log')))
                return 1
        procs += batch
    for r in runs[1:]:
        log = open(os.path.join(out, r[0], 'run.log'), errors='replace').read()
        if 'GL: ' not in log or 'GPU renderer unavailable' in log:
            print('%s: the GPU renderer did not start (no OpenGL 3.3 through EGL?): see %s' % (r[0], os.path.join(out, r[0], 'run.log')))
            return 1

    ref = hashes(os.path.join(out, 'soft', 'hashes.txt'))
    if len(ref) < sc['frames'] - 5:
        print('software run stopped early: %d of %d frames' % (len(ref), sc['frames'])); return 1
    failed = 0
    for r in runs[1:]:
        h = hashes(os.path.join(out, r[0], 'hashes.txt'))
        if len(h) != len(ref):
            print('GL FAIL (%s, %s): %d frames against the software run\'s %d' % (name, r[0], len(h), len(ref))); failed = 1; continue
        diff = next((f for f in sorted(ref) if ref[f] != h.get(f)), None)
        if diff is not None:
            what = ' and '.join(w for w, i in (('memory', 0), ('fighters', 1)) if ref[diff][i] != h[diff][i])
            print('GL FAIL (%s, %s): the game differs at frame %d (%s): the renderer changed what the game does' % (name, r[0], diff, what))
            failed = 1

    sdir = os.path.join(out, 'soft', 'dump')
    frames = sorted(os.listdir(sdir))
    if not frames:
        print('no frames dumped'); return 1
    soft = {}
    for fn in frames:
        w, h, px = read_png(os.path.join(sdir, fn))
        soft[fn] = to_rgb(w, h, px, 1)
    squares = (480 // a.block) * (272 // a.block)
    limit = squares * a.max_bad / 100.0
    soft_b = {fn: blocks(px, a.block) for fn, px in soft.items()}
    for r in runs[1:]:
        gdir = os.path.join(out, r[0], 'dump')
        missing = [fn for fn in frames if not os.path.exists(os.path.join(gdir, fn))]
        if missing:
            print('GL FAIL (%s, %s): %d frames not dumped, first %s' % (name, r[0], len(missing), missing[0])); failed = 1; continue
        worst = []
        for fn in frames:
            w, h, px = read_png(os.path.join(gdir, fn))
            if (w, h) != (480 * r[2], 272 * r[2]):
                print('GL FAIL (%s, %s): %s is %dx%d, not %dx%d' % (name, r[0], fn, w, h, 480 * r[2], 272 * r[2])); failed = 1; break
            g = to_rgb(w, h, px, r[2])
            n, _ = bad_pixels(soft_b[fn], blocks(g, a.block), a.tolerance)
            worst.append((n, fn, g))
        worst.sort(key=lambda x: -x[0])
        bad = [x for x in worst if x[0] > limit]
        top = worst[0]
        print('%s %s: %d frames, worst %s with %.2f%% of %dx%d squares over %d%s' % (
            name, r[0], len(worst), top[1], top[0] * 100.0 / squares, a.block, a.block, a.tolerance,
            '' if not bad else ' -- %d frames over %.1f%%' % (len(bad), a.max_bad)))
        if bad:
            failed = 1
            for n, fn, g in bad[:5]:
                p = os.path.join(out, '%s_%s' % (r[0], fn))
                side_by_side(p, soft[fn], g, bad_pixels(soft[fn], g, a.tolerance)[1])
                print('  %s: %.2f%% (software | GPU | difference: %s)' % (fn, n * 100.0 / squares, p))
    if not failed:
        print('GL OK: %s, the game identical in all %d frames with %s, pictures within tolerance (%.0fs)'
              % (name, len(ref), ', '.join(r[0] for r in runs[1:]), time.time() - t0))
    return failed


if __name__ == '__main__':
    sys.exit(main())
