#!/usr/bin/env python3
"""Build an art pack for painting the Stage Extensions (WIDE NATIVE), from your own disc.

EXPERIMENTAL. Each layer whole comes from the disc (tools/stage_layers.py: the stage's own tile maps and
textures, one PNG per layer numbered as the game numbers them), the wall screenshots from short runs.

An artist needs the stage art to trace from and the placeholders to paint over. This puts both in one
folder (and a zip of it), one folder per stage:

  art-pack/<region>/README.txt
  art-pack/<region>/<nn>_<name>/layers/      each layer whole, from the disc: layer0.png, layer1.png (the
                                             fighters'), layer2.png, layer3.png (the floor band, flat) ...,
                                             and layers.json (where each sits, how the game draws it)
  art-pack/<region>/<nn>_<name>/reference/   the stage at each wall in WIDE NATIVE, on the ground and in the
                                             air: the whole screen and each layer alone (transparent where it
                                             draws nothing); from tools/dump_stage_references.py
  art-pack/<region>/<nn>_<name>/paint/       the placeholders to paint (copies of assets/extensions/stages/
                                             <nn>_<name>/: the layer strips, the foreground objects' pieces,
                                             widescreen.json)
  art-pack/a3mx-art-pack-<region>.zip

The references are made from your disc's art: the pack is for you and whoever you ask to paint, not to
publish. art-pack/ is git-ignored. Painted files go back into assets/extensions/stages/<nn>_<name>/ under
the same names (docs/STAGE_EXTENSIONS.md).

Usage: python tools/art_pack.py [--exe build/a3mx] [--stages 1,6] [-j 2]
       python tools/build.py --art-pack        (after building; the launcher's "Build the art pack")
US program only for now. Needs Pillow (installed with pip if missing).
"""
import argparse, os, shutil, subprocess, sys, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)


def need_pillow():
    try:
        import PIL  # noqa: F401
        return
    except ImportError:
        pass
    print('Pillow is needed for the art pack; installing it with pip ...', flush=True)
    r = subprocess.run([sys.executable, '-m', 'pip', 'install', '--user', 'pillow'])
    if r.returncode != 0:
        sys.exit('Could not install Pillow (python -m pip install pillow). Install it and try again.')
    import site, importlib
    importlib.reload(site)


README = """A3MX art pack ({region})
=====================

Made by tools/art_pack.py from your own disc. For painting the Stage Extensions: the art WIDE NATIVE shows
at the sides of each stage, where the original 4:3 art runs out. Not for publishing: the references are the
game's art.

Each stage folder has:
  layers/     each of the stage's layers whole, as its tile map holds it (most are 1024 x 512, more than
              the 768 x 272 the camera ever shows), straight from the disc: layer<n>.png, numbered as the
              game numbers them (0 the back, 1 the fighters', 2; 3 the floor band, drawn flat; CHARLIE has a
              4). Animated layers are one PNG a frame (layer<n>_frame<ff>.png). Transparent where the layer
              has nothing. layers.json says where each sits and what's left out (BISON's sky is drawn
              without its ripple, the floor without its lean). Stage objects (sprites) are not layers.
  reference/  what the stage looks like at its left and right walls, with the camera on the ground and
              16 px up (air), 480 x 272: <side>_<height>.png is the whole screen, <side>_<height>_layer<n>.png
              one layer on its own (transparent where it draws nothing).
  paint/      the files to paint, as the game loads them:
              widescreen_layer<n>_<left|right>.png   a layer's strip: 64 x 288. 48 columns are the new edge;
                    8 reach under the layer's own art (inwards) and 8 past the screen's edge (outwards), so
                    there is never a gap. Row r is the layer's y r. Paint what continues that layer; leave the
                    rest transparent.
              widescreen_object_<kind>_<type>_<left|right>.png   a piece of art pinned to an object drawn in
                    front of the fighters (Ryu's straw, Adon's leaves ...); it moves with the object.
              widescreen.json   where each one goes. Keep it with them.
              The placeholders are flat colours with a label; anything that isn't stays as painted when the
              tools run again.

When painted, copy paint/ back into assets/extensions/stages/<same folder>/ and turn on Stage Extensions
(launcher: Extensions) with WIDE NATIVE. docs/STAGE_EXTENSIONS.md has the details.
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'a3mx.exe' if os.name == 'nt' else 'a3mx'))
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--region', default='us')
    ap.add_argument('--out', default=os.path.join(ROOT, 'art-pack'))
    ap.add_argument('--stages', help='menu numbers (1-38), comma-separated; default all')
    ap.add_argument('-j', type=int, default=2)
    ap.add_argument('--no-zip', action='store_true')
    a = ap.parse_args()
    if a.region != 'us': sys.exit('The art pack is US only for now.')
    if not os.path.exists(a.exe): sys.exit('No game at %s: build it first (python tools/build.py).' % a.exe)
    need_pillow()
    import dump_stage_references as dsr
    import asset_layout, measure_stages as ms

    assets = os.path.join(ROOT, 'assets', a.region)
    print('Rendering each stage at its walls (4 short runs a stage) ...', flush=True)
    sys.argv = [sys.argv[0], '--exe', a.exe, '--work', a.work, '--assets', assets, '-j', str(a.j)] + \
               (['--stages', a.stages] if a.stages else [])
    dsr.main()

    import json, stage_layers
    disc = os.path.join(a.work, 'iso', 'PSP_GAME', 'USRDIR', 'usa', 'data')
    pairs = json.load(open(os.path.join(ROOT, 'tools', 'data', 'palette_pairs.json')))['textures']
    img = os.path.join(a.work, 'analysis.image.bin')
    image = open(img, 'rb').read() if os.path.exists(img) else None

    out = os.path.join(a.out, a.region)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    open(os.path.join(out, 'README.txt'), 'w', newline='\r\n').write(README.format(region=a.region.upper()))
    n = 0
    for stage in range(0x2A):
        try: rel = asset_layout.stage(stage)         # stages/<nn>_<name>
        except KeyError: continue                    # numbers with no stage of their own
        ref = os.path.join(assets, *rel.split('/'), 'widescreen_reference')
        paint = os.path.join(ROOT, 'assets', 'extensions', *rel.split('/'))
        if not os.path.isdir(ref) and not os.path.isdir(paint): continue
        dest = os.path.join(out, rel.split('/')[-1])
        if os.path.isdir(dest): continue               # two stage numbers sharing one folder
        if os.path.isdir(ref): shutil.copytree(ref, os.path.join(dest, 'reference'))
        if os.path.isdir(paint): shutil.copytree(paint, os.path.join(dest, 'paint'))
        if os.path.exists(os.path.join(disc, 'st%02x.pac' % stage)):
            stage_layers.write_stage(disc, stage, os.path.join(dest, 'layers'), pairs, image)
        n += 1
    print('Art pack: %d stages in %s' % (n, os.path.relpath(out, ROOT)))
    if not a.no_zip:
        z = os.path.join(a.out, 'a3mx-art-pack-%s.zip' % a.region)
        with zipfile.ZipFile(z, 'w', zipfile.ZIP_DEFLATED) as f:
            for d, _, files in os.walk(out):
                for name in files:
                    p = os.path.join(d, name)
                    f.write(p, os.path.relpath(p, a.out))
        print('Zipped: %s' % os.path.relpath(z, ROOT))


if __name__ == '__main__':
    main()
