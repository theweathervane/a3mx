#!/usr/bin/env python3
"""SDL3, built once and kept: the pinned release (tools/data/pins.json, "sdl3") compiled as a static
library and installed into the tools folder (.tools/sdl3/, or under A3MX_TOOLS_DIR).

    python tools/sdl3.py            build it if it isn't there (a few minutes)
    python tools/sdl3.py --check    exit 0 if it is there and is the pinned version, 1 if not

cmake/SDL3.cmake uses it when it is there and otherwise downloads and compiles SDL3 inside each build
folder, as it always has. So nobody has to run this: it is for the pipelines, which keep .tools/sdl3/
in their cache (keyed on this file, cmake/SDL3.cmake and pins.json) instead of compiling SDL3 in every
run, and for anyone who builds into several folders and would rather compile it once.

Built with the same compiler and settings as the game (tools/build.py's tools; on macOS for macOS 11
and Apple silicon, as CMakeLists.txt sets). A3MX_SDL3_CMAKE_ARGS adds CMake options (a Linux system
without some of SDL's optional libraries: -DSDL_WAYLAND=OFF ...). Standard library only.
"""
import os, shutil, subprocess, sys, tarfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build, fetch, pins

ROOT = build.ROOT
DEST = os.path.join(build.TOOLS, 'sdl3')
STAMP = os.path.join(DEST, 'a3mx-built.txt')


def wanted():
    p = pins.by_id(pins.load(), 'downloads', 'sdl3')
    return '%s %s' % (p['version'], p['sha256'])


def built():
    try: return open(STAMP, encoding='utf-8').read().strip() == wanted()
    except OSError: return False


def main():
    if '--check' in sys.argv[1:]:
        print('SDL3: %s' % ('built, in %s' % DEST if built() else 'not built in %s' % DEST))
        return 0 if built() else 1
    if built():
        print('SDL3: already built in %s' % DEST)
        return 0
    build.step('SDL3, built once (tools/sdl3.py)')
    env, cmake, _ = build.tools()
    url, sha = pins.download('sdl3')
    work = os.path.join(build.TOOLS, 'sdl3-build')
    shutil.rmtree(work, ignore_errors=True); shutil.rmtree(DEST, ignore_errors=True)
    os.makedirs(work)
    archive = os.path.join(work, 'sdl3.tar.gz')
    build.say('  downloading %s' % url)
    fetch.fetch_to(url, archive)
    if pins.sha256_file(archive) != sha:
        shutil.rmtree(work, ignore_errors=True)
        build.fail('SDL3: the download does not match the SHA-256 in tools/data/pins.json, so it was removed.')
    with tarfile.open(archive) as t:
        top = t.getnames()[0].split('/')[0]
        try: t.extractall(work, filter='data')
        except TypeError: t.extractall(work)                # Python before 3.12
    src = os.path.join(work, top)
    cmd = [cmake, '-S', src, '-B', os.path.join(work, 'b'), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
           '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++', '-DCMAKE_INSTALL_PREFIX=' + DEST,
           '-DSDL_SHARED=OFF', '-DSDL_STATIC=ON', '-DSDL_TEST_LIBRARY=OFF', '-DSDL_TESTS=OFF', '-DSDL_EXAMPLES=OFF']
    if build.MACOS: cmd += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0', '-DCMAKE_OSX_ARCHITECTURES=arm64']
    cmd += os.environ.get('A3MX_SDL3_CMAKE_ARGS', '').split()    # a system missing some of SDL's optional libraries: -DSDL_WAYLAND=OFF ...
    build.run(cmd, env)
    build.run([cmake, '--build', os.path.join(work, 'b')], env)
    build.run([cmake, '--install', os.path.join(work, 'b')], env)
    with open(STAMP, 'w', encoding='utf-8', newline='\n') as f: f.write(wanted() + '\n')
    shutil.rmtree(work, ignore_errors=True)                 # the installed library is all that is kept
    build.say('  SDL3 is in %s: builds use it from now on (cmake/SDL3.cmake)' % DEST)
    return 0


if __name__ == '__main__':
    sys.exit(main())
