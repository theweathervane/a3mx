#!/usr/bin/env python3
"""A release's download for one system: the source tree with the prebuilt launcher at its top.

    python tools/package_launcher.py --system windows-x64 --launcher build-launcher/a3mx-launcher.exe --out dist

writes dist/a3mx-<VERSION>-<system>.zip holding a3mx-<VERSION>/ (the files of HEAD,
from git archive) plus a3mx-launcher(.exe) beside build.cmd / build.sh (on macOS, as the app
"A3MX Launcher.app"), and an empty imports/psp/
with a note saying the ISO goes there. Unzipped, the launcher
finds tools/build.py next to it, so a player opens it, picks their ISO and presses Run: the build
happens on their machine from their own disc. The launcher holds no game code,
which is what lets it ship prebuilt. Each build job in .gitlab-ci.yml runs this for its
system, and the release job attaches the zips to the release. Standard library only.
"""
import argparse, io, os, shutil, subprocess, sys, tarfile, tempfile, time, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = 'A3MX Launcher.app'   # macOS: the launcher as an app (tools/build.py knows this name)

PLIST = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>A3MX Launcher</string>
  <key>CFBundleDisplayName</key><string>A3MX Launcher</string>
  <key>CFBundleIdentifier</key><string>com.gitlab.theweathervane-group.a3mx-launcher</string>
  <key>CFBundleExecutable</key><string>a3mx-launcher</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>{version}</string>
  <key>CFBundleVersion</key><string>{version}</string>
  <key>CFBundleIconFile</key><string>launcher</string>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
  <key>LSArchitecturePriority</key><array><string>arm64</string></array>
  <key>NSHighResolutionCapable</key><true/>
  <key>LSApplicationCategoryType</key><string>public.app-category.games</string>
</dict>
</plist>
"""


def make_app(launcher, version, dest):
    """macOS: DEST/<APP>, the launcher as a double-clickable app: Contents/MacOS/a3mx-launcher, an
    Info.plist (macOS 11 and later, arm64), the spanner icon (res/launcher_512.png, through sips and
    iconutil), and an ad-hoc signature over the whole bundle (codesign -s -), which Apple silicon needs
    for the app to run at all. Not notarised: the first time, macOS asks to allow it (README). The
    launcher finds the folder above the app (find_root), so the app stays at the top of the download."""
    app = os.path.join(dest, APP)
    macos, res = os.path.join(app, 'Contents', 'MacOS'), os.path.join(app, 'Contents', 'Resources')
    os.makedirs(macos); os.makedirs(res)
    shutil.copy2(launcher, os.path.join(macos, 'a3mx-launcher'))
    os.chmod(os.path.join(macos, 'a3mx-launcher'), 0o755)
    short = version.split('-')[0]                                   # CFBundle versions are numbers only
    with open(os.path.join(app, 'Contents', 'Info.plist'), 'w') as f: f.write(PLIST.replace('{version}', short))
    if shutil.which('sips') and shutil.which('iconutil'):
        with tempfile.TemporaryDirectory() as t:
            iconset = os.path.join(t, 'launcher.iconset'); os.makedirs(iconset)
            png = os.path.join(ROOT, 'res', 'launcher_512.png')
            for size in (16, 32, 128, 256, 512):
                for scale in (1, 2):
                    px = size * scale
                    if px > 512: continue
                    name = 'icon_%dx%d%s.png' % (size, size, '@2x' if scale == 2 else '')
                    subprocess.run(['sips', '-z', str(px), str(px), png, '--out', os.path.join(iconset, name)], check=True, capture_output=True)
            subprocess.run(['iconutil', '-c', 'icns', iconset, '-o', os.path.join(res, 'launcher.icns')], check=True)
    else: print('package_launcher.py: no sips / iconutil, so the app has no icon')
    if shutil.which('codesign'):
        subprocess.run(['codesign', '--force', '--deep', '--sign', '-', '--timestamp=none', app], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', app], check=True)
    else: print('package_launcher.py: no codesign: the app is unsigned and will not open on Apple silicon')
    return app


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--system', required=True, help='windows-x64, linux-x64, macos-arm64, ...')
    ap.add_argument('--launcher', required=True, help='the built launcher (build-launcher/a3mx-launcher[.exe])')
    ap.add_argument('--out', default='dist')
    ap.add_argument('--ref', default='HEAD', help='what to archive (default HEAD)')
    ap.add_argument('--standalone', action='store_true',
                    help='also write the launcher alone: a3mx-launcher-<VERSION>-<system>[.exe], or .zip holding the .app on macOS')
    ap.add_argument('--librashader', metavar='DIR',
                    help="the FILTER setting's library as tools/librashader.py built it (.tools/librashader): put in the "
                         "download at the same place, with its licence. Left out, with a note, if DIR has no library")
    a = ap.parse_args()

    # VERSION as it is now: a nightly's has -nightly.<date>+<commit> (written by the pipeline before the
    # build), and the download carries that file instead of the committed one, so a player's own build
    # of a nightly calls itself one. File names have no '+' (web addresses would change it): '.' instead.
    version = open(os.path.join(ROOT, 'VERSION')).read().strip()
    top = 'a3mx-%s' % version.replace('+', '.')
    if not os.path.isfile(a.launcher): sys.exit('package_launcher.py: no launcher at %s' % a.launcher)
    os.makedirs(a.out, exist_ok=True)
    out = os.path.join(a.out, '%s-%s.zip' % (top, a.system))

    tar = subprocess.run(['git', '-C', ROOT, 'archive', '--format=tar', '--prefix=%s/' % top, a.ref],
                         check=True, capture_output=True).stdout
    when = time.localtime(int(subprocess.run(['git', '-C', ROOT, 'log', '-1', '--format=%ct', a.ref],
                                             check=True, capture_output=True, text=True).stdout.strip()))[:6]
    n = 0
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z, tarfile.open(fileobj=io.BytesIO(tar)) as t:
        for m in t.getmembers():
            if not (m.isfile() or m.isdir()): continue
            info = zipfile.ZipInfo(m.name + ('/' if m.isdir() else ''), when)
            info.create_system = 3                                  # Unix: keep the modes (build.sh stays executable)
            info.external_attr = ((m.mode & 0o777) | (0o040000 if m.isdir() else 0o100000)) << 16
            if m.isdir(): z.writestr(info, b'')
            else:
                info.compress_type = zipfile.ZIP_DEFLATED
                data = t.extractfile(m).read()
                if m.name == top + '/VERSION': data = (version + '\n').encode()
                z.writestr(info, data); n += 1
        note = ('Put your A3MX ISO here (the US disc, ULUS-10062; any file name),\n'
                'or choose it anywhere with Browse... in the launcher.\n')
        for d in ('imports/', 'imports/psp/'):                     # git-ignored, so not in the archive
            info = zipfile.ZipInfo('%s/%s' % (top, d), when); info.create_system = 3; info.external_attr = 0o040755 << 16
            z.writestr(info, b'')
        info = zipfile.ZipInfo('%s/imports/psp/PUT_YOUR_ISO_HERE.txt' % top, when)
        info.create_system = 3; info.external_attr = 0o100644 << 16; info.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(info, note.replace('\n', '\r\n') if a.system.startswith('windows') else note)
        def add(rel, path):
            info = zipfile.ZipInfo('%s/%s' % (top, rel), when)
            info.create_system = 3
            mode = os.stat(path).st_mode & 0o777
            info.external_attr = ((0o755 if mode & 0o111 else 0o644) | 0o100000) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, open(path, 'rb').read())
        # librashader, built by the pipeline (tools/librashader.py): where the game looks for it, with its
        # licence and where its source is (MPL-2.0)
        lib_dir = a.librashader if a.librashader and os.path.isdir(a.librashader) else None
        libs = [f for f in sorted(os.listdir(lib_dir)) if f != 'a3mx-built.txt'] if lib_dir else []
        if any(f.startswith('librashader.') for f in libs):
            for f in libs: add('.tools/librashader/' + f, os.path.join(lib_dir, f))
            print('package_launcher.py: librashader included (%s)' % ', '.join(libs))
        elif a.librashader: print('package_launcher.py: no librashader in %s: the download goes out without it' % a.librashader)
        if a.system.startswith('macos'):
            with tempfile.TemporaryDirectory() as t:
                app = make_app(a.launcher, version, t)
                for d, _, files in os.walk(app):
                    for f in sorted(files):
                        p = os.path.join(d, f)
                        add(os.path.relpath(p, t).replace(os.sep, '/'), p)
            name = APP
        else:
            name = os.path.basename(a.launcher)
            info = zipfile.ZipInfo('%s/%s' % (top, name), when)
            info.create_system = 3
            info.external_attr = (0o100755) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, open(a.launcher, 'rb').read())
    print('%s: %d files and %s (%.1f MB)' % (out, n, name, os.path.getsize(out) / 1e6))
    if a.standalone:
        # the launcher alone, named so it is clear what it is and what it runs on
        base = 'a3mx-launcher-%s-%s' % (version.replace('+', '.'), a.system)
        if a.system.startswith('macos'):
            alone = os.path.join(a.out, base + '.zip')
            with tempfile.TemporaryDirectory() as t, zipfile.ZipFile(alone, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z2:
                app = make_app(a.launcher, version, t)
                for d, _, files in os.walk(app):
                    for f in sorted(files):
                        p = os.path.join(d, f)
                        info = zipfile.ZipInfo(os.path.relpath(p, t).replace(os.sep, '/'), when)
                        info.create_system = 3
                        info.external_attr = ((0o755 if os.stat(p).st_mode & 0o111 else 0o644) | 0o100000) << 16
                        info.compress_type = zipfile.ZIP_DEFLATED
                        z2.writestr(info, open(p, 'rb').read())
        else:
            alone = os.path.join(a.out, base + ('.exe' if a.system.startswith('windows') else ''))
            shutil.copy2(a.launcher, alone)
        print('%s (%.1f MB)' % (alone, os.path.getsize(alone) / 1e6))


if __name__ == '__main__':
    main()
