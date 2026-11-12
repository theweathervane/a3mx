#!/usr/bin/env python3
"""librashader, the library SCREEN SETTING > FILTER draws its shaders with, built once and kept: its
source at the commit pinned in tools/data/pins.json ("librashader": the tag librashader-v0.12.1, the
one third_party/librashader/'s headers are from), compiled with Rust, and the library put in the tools
folder (.tools/librashader/, or under A3MX_TOOLS_DIR), where the game looks for it.

    python tools/librashader.py                 build it if it isn't there (ten minutes or more)
    python tools/librashader.py --install-rust  the same, installing Rust first if it isn't there
    python tools/librashader.py --check         exit 0 if the library is there, 1 if not (nothing is built)
    python tools/librashader.py --rust          say whether Rust (and on Windows its C++ linker) is there

Needs Rust 1.88 or later (cargo: on the PATH, or in ~/.cargo/bin where rustup puts it) and, on Windows,
Microsoft's C++ build tools (Visual Studio Build Tools, "Desktop development with C++"), which Rust
uses to link. --install-rust runs Rust's own installer (rustup, from https://rustup.rs: not pinned here,
as it changes with each Rust release), which puts Rust in your user profile (~/.cargo, ~/.rustup) and on
your PATH; it can be removed with `rustup self uninstall`. The C++ build tools are not installed by this
script: the error says where to get them. The game runs without the library; FILTER then shows the
picture plain. Nobody has to run this: the pipelines do, keep .tools/librashader/ in their cache
(keyed on this file and pins.json) and put the library in each download.

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
SRC = os.path.join(TOOLS, 'librashader-src')
LIB = 'librashader.dll' if os.name == 'nt' else 'librashader.dylib' if sys.platform == 'darwin' else 'librashader.so'
BUILT = ('librashader.dll',) if os.name == 'nt' else ('liblibrashader.dylib', 'librashader.dylib') if sys.platform == 'darwin' \
        else ('liblibrashader.so', 'librashader.so')
STAMP = os.path.join(DEST, 'a3mx-built.txt')


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


def find_cargo():
    """cargo: on the PATH, or where rustup puts it (a Rust installed a moment ago isn't on this PATH yet)."""
    c = shutil.which('cargo')
    if c: return c
    home = os.environ.get('CARGO_HOME') or os.path.join(os.path.expanduser('~'), '.cargo')
    c = os.path.join(home, 'bin', 'cargo.exe' if os.name == 'nt' else 'cargo')
    return c if os.path.isfile(c) else None


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
    """Rust's own installer (rustup), for this user only: Rust into ~/.cargo and ~/.rustup, and onto the PATH."""
    print('==> Rust: installing it with rustup (https://rustup.rs), for your user only', flush=True)
    os.makedirs(TOOLS, exist_ok=True)
    if os.name == 'nt':
        exe = os.path.join(TOOLS, 'rustup-init.exe')
        fetch.fetch_to('https://win.rustup.rs/x86_64', exe, timeout=600)
        cmd = [exe, '-y', '--profile', 'minimal']
    else:
        sh = os.path.join(TOOLS, 'rustup-init.sh')
        fetch.fetch_to('https://sh.rustup.rs', sh, timeout=600)
        cmd = ['sh', sh, '-y', '--profile', 'minimal']
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


def main(install_rust=False):
    argv = sys.argv[1:]
    install_rust = install_rust or '--install-rust' in argv
    if '--rust' in argv: return report_rust()
    pin = pins.source('librashader')
    here = os.path.isfile(os.path.join(DEST, LIB))
    if '--check' in argv:
        stamp = open(STAMP, encoding='utf-8').read().strip() if os.path.isfile(STAMP) else ''
        how = 'built here from %s' % stamp[:12] if stamp else 'put there by hand'
        print('librashader: %s' % ('%s in %s (%s)' % (LIB, DEST, how) if here else 'no %s in %s' % (LIB, DEST)))
        return 0 if here else 1
    if here and os.path.isfile(STAMP) and open(STAMP, encoding='utf-8').read().strip() == pin['commit']:
        print('librashader: already built in %s (%s)' % (DEST, pin['commit'][:12]))
        return 0
    if here and not os.path.isfile(STAMP) and '--rebuild' not in argv:
        print('librashader: %s is already in %s (not built by this script; --rebuild to build it again)' % (LIB, DEST))
        return 0
    print('==> librashader, built once (%s at %s)' % (pin['repo'], pin['commit'][:12]), flush=True)
    cargo = find_cargo()
    if not cargo and install_rust: cargo = install_rust_now()
    if not cargo:
        print('ERROR: Rust is needed to build librashader (cargo was not found). Install it from https://rustup.rs '
              '(1.88 or later), or run this again with --install-rust. The game runs without it; FILTER then does nothing.')
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
    r = subprocess.run([cargo, 'run', '--locked', '-p', 'librashader-build-script', '--', '--profile', 'optimized'], cwd=src, env=env)
    if r.returncode != 0:
        print('ERROR: the librashader build failed (above).')
        return 1
    out = next((os.path.join(src, 'target', 'optimized', n) for n in BUILT
                if os.path.isfile(os.path.join(src, 'target', 'optimized', n))), None)
    if not out:
        print('ERROR: the build finished but no %s is in %s' % (' / '.join(BUILT), os.path.join(src, 'target', 'optimized')))
        return 1
    os.makedirs(DEST, exist_ok=True)
    shutil.copyfile(out, os.path.join(DEST, LIB))
    for lic in ('LICENSE.md', 'LICENSE-GPL.md'):              # MPL-2.0 (or GPL-3.0): the notice goes with the library
        if os.path.isfile(os.path.join(src, lic)): shutil.copyfile(os.path.join(src, lic), os.path.join(DEST, lic))
    with open(os.path.join(DEST, 'SOURCE.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('librashader, built unchanged from %s at commit %s\n(tools/librashader.py). Licence: LICENSE.md here.\n'
                % (pin['repo'], pin['commit']))
    with open(STAMP, 'w', encoding='utf-8', newline='\n') as f: f.write(pin['commit'] + '\n')
    print('  %s is in %s (%d bytes, SHA-256 %s)' % (LIB, DEST, os.path.getsize(os.path.join(DEST, LIB)),
                                                 pins.sha256_file(os.path.join(DEST, LIB))))
    return 0


install_rust_now = install_rust                # (main's argument has the same name)


if __name__ == '__main__':
    sys.exit(main())
