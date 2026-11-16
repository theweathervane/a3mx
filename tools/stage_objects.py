#!/usr/bin/env python3
"""List each stage's objects: the records the game makes them from (src/core/stage/objects.c).

Every stage has a list of 16-byte records (table 0x08A143D8, by stage number), read once when the stage
is set up: one object per record, its routine picked by its kind from the table 0x08A101F0. These are
the stage's moving or animated pieces that are not its tile layers: Ryu's straw and rocks, Ken's crowd,
Charlie's planes, Guile's trees and helicopter...

Usage: python tools/stage_objects.py [--work work] [--json] [--markdown] [--stage 0x00]
US program only (ULUS-10062's addresses).
"""
import argparse, json, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = 0x08804000
STAGE_OBJECTS, KIND_ROUTINES = 0x08A143D8, 0x08A101F0
NO_OBJECTS = range(0x25, 0x29)          # BATTLE1 - 4
NAMES = ['RYU', 'KEN', 'AKUMA', 'CHARLIE', 'CHUN-LI', 'ADON', 'SODOM', 'GUY', 'BIRDIE', 'ROSE', 'BISON',
         'SAGAT', 'DAN', 'SAKURA', 'ROLENTO', 'DHALSIM', 'ZANGIEF', 'GEN', 'CHUN-LI (2)', 'GEN (2)',
         'SODOM (2)', 'BALROG', 'CAMMY', 'E.RYU', 'HONDA', 'BLANKA', 'MIKA', 'CODY', 'VEGA', 'KARIN',
         'BISON (2)', 'JULI', 'GUILE', 'FEI LONG', 'DEEJAY', 'HAWK', 'S.AKUMA', 'BATTLE1', 'BATTLE2',
         'BATTLE3', 'BATTLE4', 'KARIN (2)']
FIELDS = ['kind', 'type', 'layer', 'value_78', 'camera', 'value_11', 'x', 'y', 'value_26', 'value_24', 'depth']


def records(image, stage):
    w = lambda a: struct.unpack_from('<I', image, a - BASE)[0]
    if stage in NO_OBJECTS: return []
    p, out = w(STAGE_OBJECTS + 4 * stage), []
    while True:
        b = image[p - BASE:p - BASE + 16]
        kind, typ, layer, v78, camera, v11 = b[0] & 0x7F, b[1], b[2], b[3], b[4], b[5]
        x, y, v26, v24, depth = struct.unpack_from('<hhhhh', b, 6)
        out.append(dict(zip(FIELDS, (kind, typ, layer, v78, camera, v11, x, y, v26, v24, depth)),
                        routine='0x%08X' % w(KIND_ROUTINES + 4 * kind)))
        if b[0] & 0x80: return out
        p += 16


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--work', default=os.path.join(ROOT, 'work'))
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--markdown', action='store_true')
    ap.add_argument('--stage', help='one stage number (hex or decimal)')
    a = ap.parse_args()
    image = open(os.path.join(a.work, 'analysis.image.bin'), 'rb').read()
    stages = [int(a.stage, 0)] if a.stage else range(42)
    data = {'0x%02X' % s: records(image, s) for s in stages}
    if a.json:
        json.dump(data, sys.stdout, indent=1); print(); return
    if a.markdown:
        print('| Stage | Name | Objects (kind / type: x, y, layer, camera) |')
        print('|---|---|---|')
        for s in stages:
            recs = data['0x%02X' % s]
            cell = '; '.join('%d/%d: %d, %d, L%d, c%d' % (r['kind'], r['type'], r['x'], r['y'], r['layer'], r['camera'])
                             for r in recs) or 'none'
            print('| 0x%02X | %s | %s |' % (s, NAMES[s], cell))
        return
    for s in stages:
        recs = data['0x%02X' % s]
        print('0x%02X %s: %d object%s' % (s, NAMES[s], len(recs), '' if len(recs) == 1 else 's'))
        for r in recs:
            print('   kind %2d (%s) type %2d  x %5d y %4d  layer %d camera %d  +78 %3d +11 %3d +26 %d +24 %d depth %d' % (
                r['kind'], r['routine'], r['type'], r['x'], r['y'], r['layer'], r['camera'], r['value_78'],
                r['value_11'], r['value_26'], r['value_24'], r['depth']))


if __name__ == '__main__':
    main()
