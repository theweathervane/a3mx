#!/usr/bin/env python3
"""librashader, the library SCREEN SETTING > FILTER draws its shaders with, kept in the tools folder
(.tools/librashader/, or under A3MX_TOOLS_DIR), where the game looks for it. Its source is the commit
pinned in tools/data/pins.json ("librashader": the tag librashader-v0.12.1, the one
third_party/librashader/'s headers are from), built with only the OpenGL runtime (the one the game
uses: no Direct3D 9 / D3DX9_43, Direct3D 11 / 12 or Vulkan in it).

Built once by the project's pipelines and published (the pin's "prebuilt": a URL and the SHA-256 of
the file there, per system), it is downloaded and checked against that SHA-256: no Rust needed. Only
where there is none for this system, or with --build, is it compiled here with Rust.

    python tools/librashader.py                 download it (or build it) if it isn't there
    python tools/librashader.py --build         build it from the source, even where it could be downloaded
    python tools/librashader.py --install-rust  the same as no option, installing Rust first if it has to build
    python tools/librashader.py --out DIR       put the library in DIR instead (the pipelines: the file to publish)
    python tools/librashader.py --check         exit 0 if the library is there, 1 if not (nothing is fetched)
    python tools/librashader.py --rust          say whether Rust (and on Windows its C++ linker) is there

Building needs Rust 1.88 or later (cargo: on the PATH, in ~/.cargo/bin where rustup puts it, or in
.tools/rust/) and, on Windows, Microsoft's C++ build tools (Visual Studio Build Tools, "Desktop
development with C++"), which Rust uses to link. --install-rust runs Rust's own installer (rustup, from
https://rustup.rs: not pinned here, as it changes with each Rust release) into the tools folder
(.tools/rust/), leaving the home folder and the PATH alone: deleting .tools/rust removes it. The C++
build tools are not installed by this script: the error says where to get them. The game runs without
the library; FILTER then shows the picture plain. The pipelines build it, keep .tools/librashader/ in
their cache (keyed on this file and pins.json), put the library in each download, and fail a release
without it.

A source folder you unpacked yourself is used as it is if it is the pinned one: any .tools/librashader-*
folder (librashader-src/, or librashader-librashader-v0.12.1/ as GitHub's zip unpacks), or --source DIR.
Its Cargo.lock is checked against pins.json, and cargo checks every crate it downloads against the
checksum Cargo.lock gives (--locked), so the build uses exactly the pinned code. Standard library only (and cargo).
"""
import io, os, shutil, subprocess, sys, zipfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fetch, pins

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.environ.get('A3MX_TOOLS_DIR') or os.path.join(ROOT, '.tools')
DEST = os.path.join(TOOLS, 'librashader')
# cargo's runtimes (librashader-capi's Cargo.toml): OpenGL for the OpenGL renderer, and since 0.176.0 the API
# the SDL GPU renderer uses on this system (Vulkan; Direct3D 12 on Windows; Metal on macOS). Never D3D9 /
# D3D11 (they need D3DX9_43 and the Visual C++ runtime). D3D12 compiles its shaders with DXC while the game
# runs: dxcompiler.dll and dxil.dll go beside the game (tools/build.py, from the pinned dxc release).
_FEATURES = ('runtime-opengl,runtime-vulkan,runtime-d3d12' if os.name == 'nt' else
             'runtime-opengl,runtime-metal' if sys.platform == 'darwin' else 'runtime-opengl,runtime-vulkan')
RUNTIMES = ['--no-default-features', '--features', _FEATURES]
BUILD_KIND = _FEATURES.replace('runtime-', '').replace(',', '+')       # in the stamp: what was built (opengl+vk+d3d12 ...)
RUST = os.path.join(TOOLS, 'rust')                                    # --install-rust puts Rust here (cargo/, rustup/)
SYSTEM = ('windows-x64' if os.name == 'nt' else 'macos-arm64' if sys.platform == 'darwin' else 'linux-x64')
SRC = os.path.join(TOOLS, 'librashader-src')
LIB = 'librashader.dll' if os.name == 'nt' else 'librashader.dylib' if sys.platform == 'darwin' else 'librashader.so'
BUILT = ('librashader.dll',) if os.name == 'nt' else ('liblibrashader.dylib', 'librashader.dylib') if sys.platform == 'darwin' \
        else ('liblibrashader.so', 'librashader.so')


def verified(src, pin):
    """The source is the pinned one: each file pins.json names has its SHA-256 (line endings as git stores them)."""
    for rel, want in pin['files'].items():
        path = os.path.join(src, *rel.split('/'))
        if not os.path.isfile(path): return False
        import hashlib
        if hashlib.sha256(open(path, 'rb').read().replace(b'\r\n', b'\n')).hexdigest() != want: return False
    return True


def get_source(pin):
    url = '%s/archive/%s.zip' % (pin['repo'], pin['commit'])
    print('  downloading %s' % url, flush=True)
    shutil.rmtree(SRC, ignore_errors=True)
    try:
        data = fetch.fetch(url, timeout=600)
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            for m in z.infolist():
                rel = m.filename.split('/', 1)[1] if '/' in m.filename else ''
                if not rel or m.is_dir(): continue
                path = os.path.normpath(os.path.join(SRC, *rel.split('/')))
                if not path.startswith(os.path.normpath(SRC) + os.sep): continue   # never outside SRC
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, 'wb') as o: o.write(z.read(m))
    except Exception as ex:                                  # (a network that only lets git through)
        print('  zip download failed (%s): trying git' % ex, flush=True)
        shutil.rmtree(SRC, ignore_errors=True)
        subprocess.check_call(['git', 'init', '-q', SRC])
        subprocess.check_call(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', pin['repo'] + '.git', pin['commit']])
        subprocess.check_call(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'])


def prebuilt(pin):
    """(url, sha256) of the published library for this system, or None"""
    p = (pin.get('prebuilt') or {}).get(SYSTEM) or {}
    url, sha = p.get('url', ''), p.get('sha256', '')
    if p.get('kind', 'opengl') != BUILD_KIND: return None      # (one built with other runtimes: built here instead)
    return (url, sha) if url and len(sha) == 64 else None


def download_prebuilt(pin, dest):
    """the published library into DEST, if its SHA-256 is the pinned one; True when it is there"""
    got = prebuilt(pin)
    if not got: return False
    url, sha = got
    print('==> librashader: downloading the one the pipelines built (%s)' % url, flush=True)
    try: data = fetch.fetch(url, timeout=600)
    except Exception as ex:
        print('  the download failed (%s): building it instead' % ex)
        return False
    import hashlib
    if hashlib.sha256(data).hexdigest() != sha:
        print('  what was downloaded is not the file pinned (SHA-256 %s, pinned %s): not used' % (hashlib.sha256(data).hexdigest(), sha))
        return False
    os.makedirs(dest, exist_ok=True)
    with open(os.path.join(dest, LIB), 'wb') as f: f.write(data)
    with open(os.path.join(dest, 'SOURCE.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('librashader, built unchanged (OpenGL runtime only) from %s at commit %s by the A3MX pipelines;\n'
                'downloaded from %s, SHA-256 %s. Licence: MPL-2.0 (%s/blob/%s/LICENSE.md).\n'
                % (pin['repo'], pin['commit'], url, sha, pin['repo'], pin['commit']))
    with open(os.path.join(dest, 'a3mx-built.txt'), 'w', encoding='utf-8', newline='\n') as f: f.write('%s %s\n' % (pin['commit'], BUILD_KIND))
    print('  %s is in %s (SHA-256 checked)' % (LIB, dest))
    return True


def find_cargo():
    """cargo: on the PATH, or where rustup puts it (a Rust installed a moment ago isn't on this PATH yet)."""
    c = shutil.which('cargo')
    if c: return c
    exe = 'cargo.exe' if os.name == 'nt' else 'cargo'
    home = os.environ.get('CARGO_HOME') or os.path.join(os.path.expanduser('~'), '.cargo')
    for c in (os.path.join(home, 'bin', exe), os.path.join(RUST, 'cargo', 'bin', exe)):   # (or installed into the tools folder)
        if os.path.isfile(c): return c
    return None


def msvc_found():
    """Windows: Microsoft's C++ build tools, which Rust links with (vswhere comes with every Visual Studio
    installer since 2017). None when it can't tell."""
    if os.name != 'nt': return True
    vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'), 'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if not os.path.isfile(vswhere): return False
    try:
        r = subprocess.run([vswhere, '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                            '-property', 'installationPath'], capture_output=True, text=True, timeout=60)
        return bool(r.stdout.strip())
    except Exception: return None


MSVC_HELP = ('Rust on Windows links with Microsoft\'s C++ build tools, which were not found. Install "Build Tools for '
             'Visual Studio" from https://visualstudio.microsoft.com/visual-cpp-build-tools/ and tick "Desktop development '
             'with C++", then try again.')


def install_rust():
    """Rust's own installer (rustup), into the tools folder (.tools/rust: cargo/ and rustup/), nothing in the home
    folder or on the PATH: deleting .tools/rust removes it."""
    print('==> Rust: installing it with rustup (https://rustup.rs) into %s' % RUST, flush=True)
    os.environ['CARGO_HOME'] = os.path.join(RUST, 'cargo'); os.environ['RUSTUP_HOME'] = os.path.join(RUST, 'rustup')
    os.makedirs(TOOLS, exist_ok=True)
    if os.name == 'nt':
        exe = os.path.join(TOOLS, 'rustup-init.exe')
        fetch.fetch_to('https://win.rustup.rs/x86_64', exe, timeout=600)
        cmd = [exe, '-y', '--profile', 'minimal', '--no-modify-path']
    else:
        sh = os.path.join(TOOLS, 'rustup-init.sh')
        fetch.fetch_to('https://sh.rustup.rs', sh, timeout=600)
        cmd = ['sh', sh, '-y', '--profile', 'minimal', '--no-modify-path']
    try: r = subprocess.run(cmd)
    finally:
        for f in ('rustup-init.exe', 'rustup-init.sh'):
            try: os.remove(os.path.join(TOOLS, f))
            except OSError: pass
    if r.returncode != 0:
        print('ERROR: Rust\'s installer failed (above).')
        return None
    return find_cargo()


def report_rust():
    cargo = find_cargo()
    ver = ''
    if cargo:
        try: ver = subprocess.run([cargo, '--version'], capture_output=True, text=True, timeout=60).stdout.strip()
        except Exception: pass
    print('rust: %s' % ('%s (%s)' % (ver or 'cargo', cargo) if cargo else 'not found'))
    if os.name == 'nt':
        m = msvc_found()
        print('msvc: %s' % ('found' if m else 'not found' if m is False else 'unknown'))
    return 0 if cargo and msvc_found() is not False else 1


def find_source(pin, given):
    """A folder holding the pinned source: --source, else .tools/librashader-src or any .tools/librashader-*."""
    if given: return given if verified(given, pin) else None
    cands = [SRC] + sorted(os.path.join(TOOLS, n) for n in (os.listdir(TOOLS) if os.path.isdir(TOOLS) else [])
                           if n.startswith('librashader-') and n != 'librashader-src')
    return next((c for c in cands if os.path.isdir(c) and verified(c, pin)), None)


def stale(dest=DEST):
    """The runtimes the library in DEST was made with when they are fewer than this system's (before 0.176.0 the
    stamp said opengl: FILTER then works on OpenGL only), else None (none there, put there by hand, or up to date)."""
    stamp_path = os.path.join(dest, 'a3mx-built.txt')
    if not os.path.isfile(os.path.join(dest, LIB)) or not os.path.isfile(stamp_path): return None
    parts = open(stamp_path, encoding='utf-8').read().split()
    return parts[1] if len(parts) == 2 and parts[1] != BUILD_KIND else None


def main(install_rust=False):
    argv = sys.argv[1:]
    install_rust = install_rust or '--install-rust' in argv
    if '--rust' in argv: return report_rust()
    pin = pins.source('librashader')
    dest = argv[argv.index('--out') + 1] if '--out' in argv and argv.index('--out') + 1 < len(argv) else DEST
    stamp_path = os.path.join(dest, 'a3mx-built.txt')
    want = '%s %s' % (pin['commit'], BUILD_KIND)
    here = os.path.isfile(os.path.join(dest, LIB))
    if '--check' in argv:
        st = open(stamp_path, encoding='utf-8').read().strip() if os.path.isfile(stamp_path) else ''
        how = 'from %s' % st if st else 'put there by hand'
        print('librashader: %s' % ('%s in %s (%s)' % (LIB, dest, how) if here else 'no %s in %s' % (LIB, dest)))
        return 0 if here else 1
    stamp = open(stamp_path, encoding='utf-8').read().strip() if os.path.isfile(stamp_path) else None
    if here and stamp == want and '--rebuild' not in argv:
        print('librashader: already in %s (%s, %s)' % (dest, pin['commit'][:12], BUILD_KIND))
        return 0
    if here and stamp is None and '--rebuild' not in argv:
        print('librashader: %s is already in %s (not put there by this script; --rebuild to replace it)' % (LIB, dest))
        return 0
    if '--build' not in argv and download_prebuilt(pin, dest): return 0
    print('==> librashader, built once (%s at %s)' % (pin['repo'], pin['commit'][:12]), flush=True)
    cargo = find_cargo()
    if not cargo and install_rust: cargo = install_rust_now()
    if not cargo:
        print('ERROR: there is no published librashader for %s to download, and Rust is needed to build it (cargo was '
              'not found). Install it from https://rustup.rs (1.88 or later), or run this again with --install-rust. '
              'Or put a %s you have in %s. The game runs without it; FILTER then does nothing.' % (SYSTEM, LIB, DEST))
        return 1
    print('  Rust: %s' % cargo, flush=True)
    if msvc_found() is False:
        print('ERROR: ' + MSVC_HELP)
        return 1
    given = argv[argv.index('--source') + 1] if '--source' in argv and argv.index('--source') + 1 < len(argv) else None
    src = find_source(pin, given)
    if not src:
        if given:
            print('ERROR: %s is not librashader at the pinned commit (tools/data/pins.json).' % given)
            return 1
        os.makedirs(TOOLS, exist_ok=True)
        get_source(pin)
        if not verified(SRC, pin):
            shutil.rmtree(SRC, ignore_errors=True)
            print('ERROR: what was downloaded does not match the SHA-256s in tools/data/pins.json, so it was removed.')
            return 1
        src = SRC
    print('  source: %s (SHA-256 checked)' % src, flush=True)
    # its own build script, as its README says; --locked: exactly the crates its Cargo.lock names
    env = dict(os.environ)
    env['PATH'] = os.path.dirname(cargo) + os.pathsep + env.get('PATH', '')     # rustc beside cargo, for a Rust installed just now
    if os.path.normcase(os.path.abspath(cargo)).startswith(os.path.normcase(os.path.abspath(RUST))):   # Rust in the tools folder
        env['CARGO_HOME'] = os.path.join(RUST, 'cargo'); env['RUSTUP_HOME'] = os.path.join(RUST, 'rustup')
    if os.name == 'nt':   # the C and C++ runtimes linked in: the library needs no Visual C++ Redistributable
        env['RUSTFLAGS'] = (env.get('RUSTFLAGS', '') + ' -C target-feature=+crt-static').strip()
    # (after the second --: cargo's own flags, OpenGL only; --locked for that build too)
    r = subprocess.run([cargo, 'run', '--locked', '-p', 'librashader-build-script', '--', '--profile', 'optimized', '--', '--locked'] + RUNTIMES,
                       cwd=src, env=env)
    if r.returncode != 0:
        print('ERROR: the librashader build failed (above).')
        return 1
    out = next((os.path.join(src, 'target', 'optimized', n) for n in BUILT
                if os.path.isfile(os.path.join(src, 'target', 'optimized', n))), None)
    if not out:
        print('ERROR: the build finished but no %s is in %s' % (' / '.join(BUILT), os.path.join(src, 'target', 'optimized')))
        return 1
    os.makedirs(dest, exist_ok=True)
    shutil.copyfile(out, os.path.join(dest, LIB))
    for lic in ('LICENSE.md', 'LICENSE-GPL.md'):              # MPL-2.0 (or GPL-3.0): the notice goes with the library
        if os.path.isfile(os.path.join(src, lic)): shutil.copyfile(os.path.join(src, lic), os.path.join(dest, lic))
    with open(os.path.join(dest, 'SOURCE.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('librashader, built unchanged (OpenGL runtime only) from %s at commit %s\n(tools/librashader.py). Licence: LICENSE.md here.\n'
                % (pin['repo'], pin['commit']))
    with open(stamp_path, 'w', encoding='utf-8', newline='\n') as f: f.write(want + '\n')
    sha = pins.sha256_file(os.path.join(dest, LIB))
    print('  %s is in %s (%d bytes, SHA-256 %s)' % (LIB, dest, os.path.getsize(os.path.join(dest, LIB)), sha))
    print('  to publish it: pins.json, sources "librashader", "prebuilt": {"%s": {"url": ..., "sha256": "%s"}}' % (SYSTEM, sha))
    return 0


install_rust_now = install_rust                # (main's argument has the same name)


if __name__ == '__main__':
    sys.exit(main())
