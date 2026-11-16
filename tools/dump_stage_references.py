#!/usr/bin/env python3
"""Dump reference pictures for painting the stage extensions (docs/STAGE_EXTENSIONS.md): what each stage
looks like at its walls in WIDE FORCED, whole and layer by layer, so an artist can continue each layer's
art into its strip.

For every stage, both walls, on the ground and high in the air (the camera 16 px up), it writes:
  <side>_<ground|air>.png            the screen as the game draws it (no strips), 480 x 272
  <side>_<ground|air>_layer<n>.png   layer n alone, transparent where it draws nothing
into assets/<region>/stages/<nn>_<name>/widescreen_reference/. These come from your disc's art: they
stay in your assets folder, which git ignores, and are never committed or shared.

The runs are tools/measure_stage_layers.py's (the same frames it measures from).

Usage: python tools/dump_stage_references.py [--exe build/a3mx] [--stages 1,6] [-j 2]
US program only. Needs Pillow.
"""
import argparse, concurrent.futures, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure_stage_layers as msl
import measure_stages as ms
import asset_layout

ROOT = msl.ROOT


def layer_alone(shot, none):
    """The layer's own pixels: transparent where the frame is the same as with no layer at all."""
    from PIL import Image
    out = Image.new('RGBA', shot.size, (0, 0, 0, 0))
    ps, pn, po = shot.load(), none.load(), out.load()
    for y in range(shot.size[1]):
        for x in range(shot.size[0]):
            if ps[x, y] != pn[x, y]: po[x, y] = ps[x, y] + (255,)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'stage_references'))
    ap.add_argument('--assets', default=os.path.join(ROOT, 'assets', 'us'))
    ap.add_argument('--stages', help='menu numbers (1-38), comma-separated; default all')
    ap.add_argument('-j', type=int, default=2)
    a = ap.parse_args()
    ks = [int(s) for s in a.stages.split(',')] if a.stages else list(range(1, 39))
    jobs = [(k, s, u) for k in ks for s in 'LR' for u in (False, True)]
    with concurrent.futures.ThreadPoolExecutor(a.j) as pool:
        res = dict(zip(jobs, pool.map(lambda j: msl.run(a.exe, a.work, a.out, *j), jobs)))
    for (k, side, lifted), r in sorted(res.items()):
        folder = os.path.join(a.assets, *asset_layout.stage(r['stage']).split('/'), 'widescreen_reference')
        os.makedirs(folder, exist_ok=True)
        tag = '%s_%s' % ('left' if side == 'L' else 'right', 'air' if lifted else 'ground')
        shots = r['shots']
        if None in shots: shots[None].save(os.path.join(folder, tag + '.png'))
        none = shots.get('none')
        for n in sorted(r['depth']):
            if n in shots and none is not None:
                layer_alone(shots[n], none).save(os.path.join(folder, '%s_layer%d.png' % (tag, n)), optimize=True)
        if not lifted and side == 'L':
            print('%2d %-9s -> %s' % (k, ms.MENU_NAMES[k - 1], os.path.relpath(folder, ROOT)), flush=True)


if __name__ == '__main__':
    main()
