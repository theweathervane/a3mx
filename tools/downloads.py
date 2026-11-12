#!/usr/bin/env python3
"""What the build downloads on this system, and where it goes, from tools/data/pins.json (the same
list build.py and CMake check every download against). For create-launcher's consent page:
create-launcher.sh shows this before anything is downloaded; create-launcher.exe reads pins.json
itself (tools/create-launcher/create_launcher.c), since on Windows there may be no Python yet.

    python3 tools/downloads.py             the list, as text
    python3 tools/downloads.py --clean     as if .tools/ and the build folders had been deleted first
    python3 tools/downloads.py --needed    exit 0 if something would be downloaded, else 1 (no output)
    python3 tools/downloads.py --system Windows|Darwin|Linux    another system's list

Standard library only.
"""
import argparse, json, os, platform, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINS = os.path.join(ROOT, 'tools', 'data', 'pins.json')
SDL_WHEN = 'its source, by CMake, the first time the launcher or the game is built'
SDL_DEST = 'build-launcher/_deps and build/_deps'
# Rickow's 16:9 patch: a pinned source (commit + file hashes), downloaded by tools/extract_assets.py
WS = ('widescreen', 'work/sfa3-PSP-widescreen', 'sfa3_ws_patternpatcher.py',
      'when your disc is first extracted, for WIDE FORCED (not with --no-widescreen)')

# (pin id, folder, the file that shows it is unpacked (tools/build.py's PROBES), when; None = to download)
# SDL has no probe: it is "here" when its source is in either build folder (cmake/SDL3.cmake).
WANTED = {
    'Windows': [('python-embed', '.tools/python', 'python.exe',
                 'only if this PC has no Python 3.9+ that can check HTTPS certificates'),
                ('llvm-mingw', '.tools/llvm-mingw', 'bin/clang.exe', None),
                ('cmake', '.tools/cmake', 'bin/cmake.exe', None),
                ('ninja', '.tools/ninja', 'ninja.exe', None),
                ('sdl3', SDL_DEST, None, SDL_WHEN), WS],
    'Darwin': [('cmake-macos', '.tools/cmake-macos', 'CMake.app/Contents/bin/cmake', None),
               ('ninja-macos', '.tools/ninja-macos', 'ninja', None),
               ('sdl3', SDL_DEST, None, SDL_WHEN), WS],
    # Linux uses the system's CMake, Ninja and compiler (build.py says which to install): only SDL and the patch
    'Linux': [('sdl3', SDL_DEST, None, SDL_WHEN), WS],
}
# what isn't downloaded, but is needed (shown under the list)
NOT_DOWNLOADED = {
    'Darwin': "Apple's command line tools (Clang, the SDK, Python 3) come from Apple, not from this list: "
              "if they're missing, macOS offers to install them.",
    'Linux': 'CMake 3.25+, Ninja, a C/C++ compiler and Python 3.9+ come from your system: '
             'if one is missing, the build says which package to install. Nothing is installed by this project.',
}


def here(dest, probe, clean):
    if clean: return False
    if probe: return os.path.exists(os.path.join(ROOT, dest, probe))
    return any(os.path.isdir(os.path.join(ROOT, d, '_deps', 'sdl3-src')) for d in ('build-launcher', 'build'))


def pin_list():
    """pins.json's downloads, and its sources (a repository at a commit, checked file by file) in the
    same shape: the zip of that commit, and the SHA-256 of its first checked file."""
    data = json.load(open(PINS, encoding='utf-8'))
    pins = {d['id']: dict(d) for d in data['downloads']}
    for s in data.get('sources', []):
        name, sha = next(iter(s['files'].items()))
        pins[s['id']] = {'id': s['id'], 'name': s['name'], 'version': 'commit ' + s['commit'][:12],
                         'url': '%s/archive/%s.zip' % (s['repo'], s['commit']), 'sha256': sha, 'sha_of': name}
    return pins


def entries(system, clean):
    pins = pin_list()
    for pid, dest, probe, when in WANTED[system]:
        d = pins[pid]
        yield d, dest, here(dest, probe, clean), when, probe


def needed(system, clean):
    """Ask first when a build tool would be downloaded; on Linux, where only SDL is, when SDL would be."""
    for d, dest, is_here, when, probe in entries(system, clean):
        if is_here or d['id'] == 'python-embed': continue        # Python: only build.cmd can tell if it's needed
        if probe or system == 'Linux': return True
    return False


def text(system, clean):
    sep = '\\' if system == 'Windows' else '/'
    out = []
    for d, dest, is_here, when, probe in entries(system, clean):
        name = d['name'].split(' (')[0]
        state = 'already here, not downloaded again' if is_here else when or 'to download'
        into = ' and '.join(ROOT + sep + p.replace('/', sep) for p in dest.split(' and '))
        out += ['%s %s' % (name, d['version']), '    ' + state, '    from:     ' + d['url'],
                '    SHA-256:  ' + d['sha256'] + ('  (of %s)' % d['sha_of'] if 'sha_of' in d else ''),
                '    into:     ' + into, '']
    if system in NOT_DOWNLOADED: out += [NOT_DOWNLOADED[system], '']
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--system', default=platform.system(), choices=sorted(WANTED))
    ap.add_argument('--clean', action='store_true')
    ap.add_argument('--needed', action='store_true')
    a = ap.parse_args()
    if a.needed: sys.exit(0 if needed(a.system, a.clean) else 1)
    sys.stdout.write(text(a.system, a.clean))


if __name__ == '__main__':
    main()
