#!/usr/bin/env python3
"""One command from your disc to the game, on Windows, Linux and macOS.

    build.cmd [options]          Windows (fetches Python first if you have none)
    ./build.sh [options]         Linux, macOS
    python tools/build.py [options]

Steps, stopping at the first that fails:
  1. tools: CMake 3.25+, Ninja and Clang.
       Windows: downloaded once into .tools/ (LLVM-MinGW, CMake, Ninja; each checked against its
                SHA-256), unless already there.
       macOS:   CMake and Ninja downloaded the same way (0.52.0); Clang is Apple's command line
                tools, which it asks macOS to install if they're missing (xcode-select --install).
       Linux:   yours (it says what to install if one is missing), and SDL3's development libraries
                for the window and sound, checked with pkg-config before CMake runs (issue #2).
  2. your disc: python tools/extract_assets.py, the first time (or with --extract): checks the ISO
     in imports/psp/ (or --iso) and extracts it into work/ and assets/.
  3. cmake --preset <preset>, then cmake --build --preset <preset> (CMakePresets.json).
  4. with --test, the tests (ctest); with --run, the game.

The launcher (a3mx-launcher, src/launcher/) is a window over these same options: launcher.cmd or
./launcher.sh builds it (--launcher: no disc or Python tools needed for that) and opens it.

Options:
  --preset release|debug|headless   which build (default release: build/)
  --iso PATH          your ISO, if it is not in imports/psp/ (extracted again only if it is not the
                      one already in work/)
  --extract           extract the disc again (after changing the analysis tools)
  --no-widescreen     extract without the 16:9 patch (no WIDE FORCED)
  --no-extensions     build without the extensions carried over from the static recomp: the PSP
                      program alone (its own menus and saves, one pad), into <preset dir>-noext/
                      (a normal build can also run that way: a3mx --no-extensions)
  --test              run the tests after building
  --run               start the game after building
  -- ARGS             with --run, the rest goes to the game, e.g. -- --log-level debug --renderer soft
  --launcher          build the launcher (build-launcher/, preset launcher) and open it
  --clean             start from nothing: delete the build folders, .tools/ and what's in work/
                      first, then carry on as usual. Kept in .tools/: librashader and its source
                      folders (librashader-*), and FILTER's shaders, which are slow to make again
                      (--clean-all deletes those too). Saves and settings are in your a3mx folder and are kept
                      (any still in work/, where versions before 0.44.0 kept them, are moved there).
                      With --launcher (launcher.cmd / .sh): every build* folder, build-launcher/
                      included. Without: every build* folder but
                      that one. Never touched: imports/ (your ISOs), assets/ (your edits), the code.
  --librashader       only build FILTER's library from its pinned source (tools/librashader.py; needs Rust),
                      then stop. With --install-rust: install Rust first if it isn't there (rustup)
  --shaders           only download the FILTER setting's shaders (tools/shaders.py: libretro's
                      slang-shaders at the pinned commit, about 85 MB), then stop
  --tools-only        just step 1
Before CMake runs, any file in the checkout dated in the future (a clock or time zone behind the
machine that made the files) is dated now, or Ninja would never see the build as up to date (issue #2).
Standard library only.
"""
import argparse, hashlib, json, os, platform, shutil, subprocess, sys, time, zipfile
import fetch                     # tools/fetch.py: downloads that work with any Python (issue #52)
import pins                      # tools/pins.py: every pinned download's URL and SHA-256 (tools/data/pins.json)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# the build tools' folder: .tools/ here, or wherever A3MX_TOOLS_DIR says (the launcher's Paths tab)
TOOLS = os.environ.get('A3MX_TOOLS_DIR') or os.path.join(ROOT, '.tools')
TOOLS_OURS = os.path.normcase(os.path.abspath(TOOLS)) == os.path.normcase(os.path.join(ROOT, '.tools'))
WINDOWS = os.name == 'nt'

# Windows build tools, pinned in tools/data/pins.json (the same versions CI uses): the file that shows
# each one is unpacked
PROBES = {'llvm-mingw': 'bin/clang.exe', 'cmake': 'bin/cmake.exe', 'ninja': 'ninja.exe',
          'cmake-macos': 'CMake.app/Contents/bin/cmake', 'ninja-macos': 'ninja'}
MACOS = sys.platform == 'darwin'
LINUX = sys.platform.startswith('linux')

INSTALL_HINTS = {
    'Linux': 'Debian / Ubuntu:\n'
             '    sudo apt install cmake ninja-build clang python3 pkg-config libx11-dev libxext-dev libxrandr-dev libxcursor-dev \\\n'
             '         libxi-dev libxss-dev libxkbcommon-dev libwayland-dev wayland-protocols libegl-dev libgl-dev \\\n'
             '         libasound2-dev libpulse-dev libudev-dev libxtst-dev libxfixes-dev\n'
             'Fedora:  sudo dnf install cmake ninja-build clang python3 pkgconf-pkg-config libX11-devel libXext-devel libXrandr-devel \\\n'
             '         libXcursor-devel libXi-devel libXScrnSaver-devel libxkbcommon-devel wayland-devel mesa-libEGL-devel \\\n'
             '         mesa-libGL-devel alsa-lib-devel pulseaudio-libs-devel systemd-devel libXtst-devel libXfixes-devel\n'
             'Arch:    sudo pacman -S cmake ninja clang python pkgconf libx11 libxext libxrandr libxcursor libxi libxss libxkbcommon \\\n'
             '         wayland wayland-protocols mesa alsa-lib libpulse libxtst libxfixes',
    'Darwin': 'macOS:   xcode-select --install          (Apple\'s command line tools: Clang, Python 3)',
}


def say(msg):
    print(msg); sys.stdout.flush()


def step(title):
    say('\n==> ' + title)


def fail(msg):
    say('FAILED: ' + msg)
    sys.exit(1)


def run(cmd, env=None):
    say('> ' + ' '.join(os.path.relpath(c, ROOT) if os.path.isabs(c) and c.startswith(ROOT) else c for c in cmd))
    r = subprocess.run(cmd, cwd=ROOT, env=env)
    if r.returncode != 0: fail('%s (exit code %d)' % (os.path.basename(cmd[0]), r.returncode))


# ---------------------------------------------------------------- tools
def download(name):
    """Windows and macOS: a pinned tool into .tools/<name>/, once (a .zip or a .tar.gz)."""
    (url, sha), probe = pins.download(name), PROBES[name]
    dest = os.path.join(TOOLS, name)
    if os.path.exists(os.path.join(dest, probe)): return dest
    os.makedirs(TOOLS, exist_ok=True)
    url_ = pins.download(name)[0]
    archive = dest + ('.tar.gz' if url_.endswith('.tar.gz') else '.zip')
    say('  downloading %s (%s)' % (name, url))
    try: fetch.fetch_to(url, archive)
    except fetch.FetchError as ex: fail('could not download %s: %s' % (name, ex))
    except OSError as ex: fail('could not download %s (%s); check your connection and try again' % (name, ex))
    h = hashlib.sha256(open(archive, 'rb').read()).hexdigest()
    if h != sha:
        os.remove(archive)
        fail('%s download does not match its SHA-256 (got %s); try again, or report it' % (name, h))
    say('  unpacking %s' % name)
    tmp = dest + '.tmp'
    shutil.rmtree(tmp, ignore_errors=True); shutil.rmtree(dest, ignore_errors=True)
    if archive.endswith('.tar.gz'):
        import tarfile
        with tarfile.open(archive) as t:
            try: t.extractall(tmp, filter='tar')        # keeps the programs' exec bits; refuses paths outside tmp
            except TypeError: t.extractall(tmp)          # (a Python without extraction filters)
    else:
        zipfile.ZipFile(archive).extractall(tmp)
        if not WINDOWS:                                  # zipfile drops the exec bit
            for d, _, fs in os.walk(tmp):
                for f in fs: os.chmod(os.path.join(d, f), 0o755)
    inner = os.listdir(tmp)
    if len(inner) == 1 and os.path.isdir(os.path.join(tmp, inner[0])): shutil.move(os.path.join(tmp, inner[0]), dest); shutil.rmtree(tmp)
    else: os.rename(tmp, dest)
    os.remove(archive)
    return dest


def version_of(exe):
    try:
        out = subprocess.run([exe, '--version'], capture_output=True, text=True).stdout.split()
        for w in out:
            parts = w.split('-')[0].split('.')
            if len(parts) >= 2 and parts[0].isdigit() and parts[1].isdigit(): return tuple(int(p) for p in parts[:2] if p.isdigit())
    except OSError:
        pass
    return None


def tools():
    """The environment for CMake: PATH with the tools first. Returns (env, cmake, ctest)."""
    env = dict(os.environ)
    if WINDOWS:
        if platform.machine().lower() not in ('amd64', 'x86_64'):
            say('  note: the downloaded tools are x86-64 builds (Windows on ARM runs them emulated)')
        dirs = [os.path.join(download('llvm-mingw'), 'bin'), download('ninja'), os.path.join(download('cmake'), 'bin')]
        env['PATH'] = os.pathsep.join(dirs + [env.get('PATH', '')])
        cmake = os.path.join(dirs[2], 'cmake.exe'); ctest = os.path.join(dirs[2], 'ctest.exe')
        say('  LLVM-MinGW, CMake and Ninja: .tools/')
        return env, cmake, ctest
    if MACOS:
        # Clang (and the SDK) can only come from Apple: its command line tools, about 1 GB. Without them
        # even /usr/bin/python3 is a stub, but build.sh got this far, so Python is there.
        if subprocess.run(['xcode-select', '-p'], capture_output=True).returncode != 0 or not shutil.which('clang'):
            subprocess.run(['xcode-select', '--install'], capture_output=True)   # macOS's own install window
            fail("Apple's command line tools (Clang) aren't installed. macOS should now be showing a window to install "
                 "them (or run: xcode-select --install). When that has finished, run this again.")
        cm = download('cmake-macos'); nj = download('ninja-macos')
        dirs = [nj, os.path.join(cm, 'CMake.app', 'Contents', 'bin')]
        env['PATH'] = os.pathsep.join(dirs + [env.get('PATH', '')])
        say('  CMake and Ninja: .tools/; Clang: Apple\'s command line tools')
        return env, os.path.join(dirs[1], 'cmake'), os.path.join(dirs[1], 'ctest')
    missing = []
    cmake = shutil.which('cmake'); v = version_of(cmake) if cmake else None
    if not v or v < (3, 25): missing.append('CMake 3.25 or later' + (' (found %d.%d)' % v if v else ''))
    if not shutil.which('ninja'): missing.append('Ninja')
    if not shutil.which('clang') or not shutil.which('clang++'): missing.append('Clang')
    if missing:
        fail('missing: %s. Install them, then run this again.\n%s' % (', '.join(missing), INSTALL_HINTS.get(platform.system(), '')))
    say('  CMake %d.%d, Ninja, Clang: found' % v)
    return env, cmake, shutil.which('ctest') or 'ctest'


def pkg_config(name):
    return subprocess.run(['pkg-config', '--exists', name], capture_output=True).returncode == 0


def linux_libraries():
    """Linux: SDL3's development libraries for the window and sound. Without them SDL's own configure
    stops with \"SDL could not find X11 or Wayland development libraries\" after CMake, Ninja and
    Clang have all been found, so nothing says what to install (issue #2). Checked here first, the
    same way SDL looks for them (pkg-config)."""
    hint = INSTALL_HINTS['Linux']
    if not shutil.which('pkg-config'):
        fail('missing: pkg-config, which the build uses to find the window and sound libraries. '
             'Install it and the libraries, then run this again.\n' + hint)
    x11 = pkg_config('x11') and pkg_config('xext')
    wayland = all(pkg_config(n) for n in ('wayland-client', 'wayland-egl', 'wayland-cursor', 'xkbcommon'))
    if not x11 and not wayland:
        fail('missing: the window libraries (the X11 or Wayland development packages); without them the '
             'game and the launcher can\'t open a window. Install them, then run this again.\n' + hint)
    sound = [n for n, p in (('ALSA', 'alsa'), ('PulseAudio', 'libpulse'), ('PipeWire', 'libpipewire-0.3')) if pkg_config(p)]
    if not sound:
        fail('missing: the sound libraries (the ALSA or PulseAudio development packages); without them the '
             'game would build with no sound. Install them, then run this again.\n' + hint)
    say('  window: %s; sound: %s' % (' and '.join(n for n, ok in (('X11', x11), ('Wayland', wayland)) if ok),
                                    ', '.join(sound)))


def future_dates():
    """Date now any file in the checkout dated in the future. Files unpacked on a PC whose clock or time
    zone is behind the one that made them (a downloaded zip, a copied folder) can be, and Ninja then
    finds the build out of date on every run and can stop with \"still dirty after 100 tries\" (issue
    #2). Not touched: .git, the build folders, .tools/, imports/ (your ISOs: their date is how
    work/'s extraction is recognised), assets/ and saves/ (an edited asset is told by its date)."""
    now = time.time()
    skip = {'.git', '.tools', 'imports', 'assets', 'saves'}
    fixed = []
    for d, dirs, files in os.walk(ROOT):
        if d == ROOT: dirs[:] = [x for x in dirs if x not in skip and not x.startswith('build')]
        for f in files + dirs:
            p = os.path.join(d, f)
            try:
                if os.path.islink(p) or os.path.getmtime(p) <= now + 2: continue    # 2 s: FAT's and zip's steps
                os.utime(p, (now, now))
                fixed.append(p)
            except OSError:
                pass
    if fixed:
        say('  %d file%s dated in the future (this PC\'s clock or time zone is behind the one that made '
            'them): dated now, e.g. %s' % (len(fixed), '' if len(fixed) == 1 else 's', os.path.relpath(fixed[0], ROOT)))


# ---------------------------------------------------------------- the build
def extract_reason(a, work):
    """Why the disc must be extracted (again), or None if work/ already holds what was asked for."""
    if a.extract: return 'asked for (--extract)'
    if not os.path.exists(os.path.join(work, 'analysis.image.bin')): return 'not extracted yet'
    try: disc = json.load(open(os.path.join(work, 'disc.json')))
    except (OSError, ValueError): return 'work/disc.json is missing'
    if a.iso:
        if not os.path.isfile(a.iso): fail('no ISO at %s' % a.iso)
        same = (disc.get('iso_path') == os.path.abspath(a.iso) and disc.get('iso_size') == os.path.getsize(a.iso)
                and disc.get('iso_mtime') == int(os.path.getmtime(a.iso)))
        if not same:
            h = hashlib.sha1()
            with open(a.iso, 'rb') as f:
                for block in iter(lambda: f.read(1 << 20), b''): h.update(block)
            if h.hexdigest() != disc.get('iso_sha1'): return 'a different ISO (%s)' % os.path.basename(a.iso)
            disc.update(iso_path=os.path.abspath(a.iso), iso_size=os.path.getsize(a.iso), iso_mtime=int(os.path.getmtime(a.iso)))
            json.dump(disc, open(os.path.join(work, 'disc.json'), 'w'), indent=1)    # the same disc: no hashing next time
    has_ws = os.path.exists(os.path.join(work, 'boot.widescreen.json'))
    if a.no_widescreen and has_ws: return 'without the widescreen patch this time (--no-widescreen)'
    if not a.no_widescreen and not has_ws and disc.get('region', 'us') == 'us': return 'with the widescreen patch this time'
    assets = os.path.join(ROOT, 'assets', disc.get('region', 'us'))
    if os.path.isdir(os.path.join(assets, 'pac')) and not os.path.exists(os.path.join(assets, 'index.json')):
        return 'to move assets/ to the 0.37.0 layout (your edits are carried over; assets/LAYOUT.md)'
    return None


# ---------------------------------------------------------------- --clean
# The player's saves and settings, which the game keeps in the player's own folder since 0.60.0
# (src/platform/userdir.h: %APPDATA%\a3mx and its equivalents), in saves/ from 0.44.0 and in work/ before
# (src/extensions/savestore.c, settings.c, exsetting.c's .bak copies and IMPORT / EXPORT folders; ms0/
# is the memory stick of --no-extensions). The game moves them out of work/ on its first run
# (move_saves_from in src/platform/main.c: the same list); --clean does the same before emptying work/.
SAVE_NAMES = ('a3mx.cfg', 'a3mx.dat', 'a3mx.cfg.bak', 'a3mx.dat.bak', 'sfa3max.cfg', 'sfa3max.dat',
              'ms0', 'import', 'export')
CLEAN_KEEP = ()


def remove(path):
    """Delete a file or folder for --clean; a link is removed, never followed. Read-only files too."""
    if os.path.islink(path) or os.path.isfile(path):
        os.chmod(path, 0o666) if not os.path.islink(path) else None
        os.remove(path)
        return
    def again(fn, p, _exc):              # read-only files (Windows): make them writable and try again
        os.chmod(p, 0o777)
        fn(p)
    if sys.version_info >= (3, 12): shutil.rmtree(path, onexc=again)
    else: shutil.rmtree(path, onerror=again)


def kept_tool(name):
    """What --clean leaves in .tools/: librashader (built with Rust, and maybe by hand), its source folders
    and FILTER's shaders. Everything else there is downloaded again in a minute or two."""
    return name in ('librashader', 'slang-shaders') or name.startswith('librashader-')


def clean(for_launcher, all_too=False):
    step('Clean: starting from nothing (--clean)')
    targets = []
    for name in sorted(os.listdir(ROOT)):
        if not (name == 'build' or name.startswith('build-')) or not os.path.isdir(os.path.join(ROOT, name)): continue
        if not for_launcher and name == 'build-launcher': continue
        targets.append(name)
    here = os.path.dirname(os.path.abspath(sys.executable))
    if not TOOLS_OURS: say('  %s (A3MX_TOOLS_DIR): kept; delete it yourself to download the tools again' % TOOLS)
    elif os.path.isdir(TOOLS):
        if os.path.normcase(here).startswith(os.path.normcase(TOOLS + os.sep)):
            say('  .tools/: kept, this Python runs from it (build.cmd --clean deletes it before starting Python)')
        else:
            kept_tools = []
            for name in sorted(os.listdir(TOOLS)):
                if not all_too and kept_tool(name): kept_tools.append(name); continue
                targets.append(os.path.join('.tools', name))
            if kept_tools: say('  .tools/: kept %s (slow to make again; --clean-all deletes them too)' % ', '.join(kept_tools))
    work, saves = os.path.join(ROOT, 'work'), os.path.join(ROOT, 'saves')
    kept = []
    if os.path.isdir(work):
        for name in sorted(os.listdir(work)):
            if name in SAVE_NAMES:          # saves from before 0.44.0: into saves/, never over one there
                if os.path.lexists(os.path.join(saves, name)):
                    kept.append(os.path.join('work', name))
                    continue
                os.makedirs(saves, exist_ok=True)
                shutil.move(os.path.join(work, name), os.path.join(saves, name))
                say('  moved work/%s to saves/%s' % (name, name))
            elif name in CLEAN_KEEP: pass
            else: targets.append(os.path.join('work', name))
    if not targets: say('  nothing to delete')
    failed = []
    for t in targets:
        say('  deleting %s' % t)
        try: remove(os.path.join(ROOT, t))
        except OSError as ex: failed.append('%s (%s)' % (t, ex.strerror or ex))
    if kept: say('  kept in work/: %s (saves/ has its own; move it there by hand if it is the one you want)' % ', '.join(kept))
    if os.path.isdir(saves): say('  saves/ (saves and settings from before 0.60.0; the game moves them to your a3mx folder): kept')
    say('  your a3mx folder (saves, configs, logs: docs/SAVES.md): not touched')
    if failed:
        fail('could not delete %s. Close the game, the launcher and anything else using those folders '
             '(a terminal or an editor open in one of them), then try again.' % '; '.join(failed))


def launcher(env, cmake):
    """--launcher: open the launcher; the prebuilt one a release download has beside this folder's
    build.cmd, or else build it on its own (preset launcher: no disc needed)."""
    name = 'a3mx-launcher.exe' if WINDOWS else 'a3mx-launcher'
    exe = os.path.join(ROOT, name)
    app = os.path.join(ROOT, 'A3MX Launcher.app')     # macOS downloads (package_launcher.py)
    if sys.platform == 'darwin' and os.path.isdir(app):
        say('\n==> The launcher: the prebuilt %s' % os.path.basename(app))
        subprocess.Popen(['open', app], cwd=ROOT, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return
    if os.path.isfile(exe):
        say('\n==> The launcher: the prebuilt %s' % name)
    else:
        if LINUX: linux_libraries()
        future_dates()
        step('Configure and build the launcher (preset launcher: build-launcher/)')
        run([cmake, '--preset', 'launcher'], env)
        run([cmake, '--build', '--preset', 'launcher'], env)
        exe = os.path.join(ROOT, 'build-launcher', name)
        say('\nBuilt: %s' % os.path.relpath(exe, ROOT))
    flags = 0x00000008 | 0x00000200 if WINDOWS else 0          # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
    subprocess.Popen([exe], cwd=ROOT, creationflags=flags, start_new_session=not WINDOWS,
                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser(description='Build A3MX from your disc.')
    ap.add_argument('--preset', default='release', choices=['release', 'debug', 'headless'])
    ap.add_argument('--iso'); ap.add_argument('--extract', action='store_true')
    ap.add_argument('--no-widescreen', action='store_true')
    ap.add_argument('--no-extensions', action='store_true')
    ap.add_argument('--test', action='store_true'); ap.add_argument('--run', action='store_true')
    ap.add_argument('--tools-only', action='store_true')
    ap.add_argument('--launcher', action='store_true')
    ap.add_argument('--shaders', action='store_true')
    ap.add_argument('--clean', action='store_true'); ap.add_argument('--clean-all', action='store_true')
    ap.add_argument('--librashader', action='store_true'); ap.add_argument('--install-rust', action='store_true')
    argv = sys.argv[1:]
    game_args = argv[argv.index('--') + 1:] if '--' in argv else []      # after --: for the game (--run)
    if '--' in argv: argv = argv[:argv.index('--')]
    a = ap.parse_args(argv)
    if sys.version_info < (3, 9): fail('Python 3.9 or later is needed')

    if a.shaders:                                 # nothing else: no build tools are needed for it
        import shaders
        sys.exit(shaders.main())
    if a.librashader:                             # FILTER's library: built from its pinned source with Rust
        import librashader
        sys.exit(librashader.main(install_rust=a.install_rust))
    if a.clean or a.clean_all: clean(a.launcher, a.clean_all)
    step('Build tools')
    env, cmake, ctest = tools()
    if a.tools_only:
        return
    if LINUX and not a.launcher and a.preset != 'headless': linux_libraries()     # headless has no window or sound
    if a.launcher:
        launcher(env, cmake)
        return

    work = os.path.join(ROOT, 'work')
    reason = extract_reason(a, work)
    if reason:
        step('Your disc: check and extract (tools/extract_assets.py): %s' % reason)
        cmd = [sys.executable, os.path.join(ROOT, 'tools', 'extract_assets.py')]
        if a.iso: cmd += ['--iso', a.iso]
        if a.no_widescreen: cmd += ['--no-widescreen']
        run(cmd, env)
    else:
        say('\n==> Your disc: already extracted into work/ (--extract to do it again)')

    future_dates()
    out = {'release': 'build', 'debug': 'build-debug', 'headless': 'build-headless'}[a.preset]
    if a.no_extensions:
        out += '-noext'
        step('Configure and build (preset %s, no extensions: %s/)' % (a.preset, out))
        run([cmake, '--preset', a.preset, '-B', os.path.join(ROOT, out), '-DA3MX_EXTENSIONS=OFF'], env)
        run([cmake, '--build', os.path.join(ROOT, out)], env)
    else:
        step('Configure and build (preset %s)' % a.preset)
        run([cmake, '--preset', a.preset, '-DA3MX_EXTENSIONS=ON'], env)
        run([cmake, '--build', '--preset', a.preset], env)

    exe = os.path.join(ROOT, out, 'a3mx.exe' if WINDOWS else 'a3mx')
    if a.test:
        step('Tests')
        run([ctest, '--test-dir', os.path.join(ROOT, out), '--output-on-failure'], env)
    say('\nBuilt: %s\nPlay:  %s --work work' % (os.path.relpath(exe, ROOT), os.path.relpath(exe, ROOT)))
    if a.run:
        step('Run' + (': ' + ' '.join(game_args) if game_args else ''))
        sys.exit(subprocess.run([exe, '--work', work] + game_args, cwd=ROOT).returncode)


if __name__ == '__main__':
    main()
