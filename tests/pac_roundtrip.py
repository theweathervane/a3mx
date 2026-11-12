#!/usr/bin/env python3
"""Every .pac archive on the disc must come back byte for byte from tools/pac.py, both directly
(read -> build) and through an unpacked folder (unpack -> pack), so a modded asset folder can always
be packed back exactly where it was not changed.

Usage: pac_roundtrip.py --work work
"""
import argparse, glob, os, shutil, sys, tempfile
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'tools'))
import pac


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--work', required=True); a = ap.parse_args()
    files = sorted(glob.glob(os.path.join(a.work, 'iso', 'PSP_GAME', 'USRDIR', '*', 'data', '*.pac')))
    if not files: print('no .pac archives under %s/iso' % a.work); return 1
    tmp = tempfile.mkdtemp(); bad = 0
    try:
        for f in files:
            b = open(f, 'rb').read()
            if pac.build(pac.read(b, with_padding=True)) != b:
                print('FAIL (read/build)', os.path.basename(f)); bad += 1; continue
            d = os.path.join(tmp, 'x'); shutil.rmtree(d, ignore_errors=True)
            pac.unpack(f, d, convert=False); pac.pack(d, os.path.join(tmp, 'x.pac'))
            if open(os.path.join(tmp, 'x.pac'), 'rb').read() != b:
                print('FAIL (unpack/pack)', os.path.basename(f)); bad += 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print('pac round trip: %d of %d archives exact' % (len(files) - bad, len(files)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
