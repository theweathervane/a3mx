#!/usr/bin/env python3
"""The notes of a GitLab release (the release job in .gitlab-ci.yml): how to get and build it on each system,
then what's new for players, then the technical details, from every CHANGELOG.md section since the
previous release.

Each section has a `### For players` part (plain bullets: what a player notices) and a `### Technical`
part (a section with neither, a flat list of bullets as the changelog has been written since 0.55.0, is all
"for players"). The notes list every version's For players bullets together, newest first, under "What's new",
then each version's Technical part under its number. A section with neither heading counts as
technical only.

Usage: release_notes.py VERSION [--since PREVIOUS] > notes.md
       release_notes.py --check VERSION [--since PREVIOUS]
       release_notes.py --nightly VERSION-nightly.DATE+COMMIT [--since PREVIOUS] > notes.md

A release covers every version after PREVIOUS (the last release's tag, without its v) up to VERSION,
newest first, so versions merged between releases are not lost from the notes. Without --since (no
release yet) it covers the whole changelog. --nightly writes the rolling nightly pre-release's notes:
a warning that it is not a supported release, then what has changed since the last release. --check only validates: VERSION is MAJOR.MINOR.PATCH,
newer than PREVIOUS, and has a CHANGELOG.md section; it prints why not and exits 1.
"""
import argparse, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def on_github():
    """In a GitHub Actions run (the mirror's workflow, .github/workflows/ci.yml)."""
    return os.environ.get('GITHUB_ACTIONS') == 'true' and bool(os.environ.get('GITHUB_REPOSITORY'))


def project_url():
    """The project's address: CI_PROJECT_URL in a GitLab pipeline, the mirror's in a GitHub Actions run
    (so a GitHub release's links stay on GitHub), otherwise the project's own."""
    if on_github():
        return (os.environ.get('GITHUB_SERVER_URL', 'https://github.com') + '/' + os.environ['GITHUB_REPOSITORY']).rstrip('/')
    return os.environ.get('CI_PROJECT_URL', 'https://gitlab.com/theweathervane-group/a3mx').rstrip('/')


def header(url, name, ref):
    """HEADER for this release: the project's address, file names and ref. HEADER's links to files are
    written GitLab's way (/-/blob/); GitHub's have no /-/."""
    text = HEADER.replace('{url}/-/blob/', '{url}/blob/') if on_github() else HEADER
    return text.replace('{url}', url).replace('{name}', name).replace('{ref}', ref)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

HEADER = """> **No game is included.** You build it from your own copy of the game: the US PSP disc, *A3MX* (`ULUS-10062`), as an ISO. Until the decomp is complete, the parts not yet decompiled are translated from your disc when you build, so a ready-made program cannot be offered.

**Download the zip for your system** below (`a3mx-{name}-windows-x64.zip`, `-linux-x64.zip` or `-macos-arm64.zip`) and unzip it. Then open **`a3mx-launcher`** in the unzipped folder, choose your ISO with **Browse...** (or put it in `imports/psp/`) and press **Run**. The launcher is prebuilt; it builds the game on your machine from your disc, with its progress in the console underneath.

| System | Once, before the first Run | Notes |
|---|---|---|
| **Windows** (x64) | nothing: Python and the build tools are downloaded into `.tools\\` the first time | The launcher isn't signed: if Windows SmartScreen stops it, choose **More info > Run anyway**. |
| **Linux** (x64) | `sudo apt install cmake ninja-build clang python3` plus SDL's libraries ([list]({url}/-/blob/{ref}/docs/BUILDING.md#tools)) | Built on Ubuntu 24.04. On an older system, build the launcher yourself with `./launcher.sh`. |
| **macOS** (Apple Silicon) | `xcode-select --install` (Apple's command line tools); CMake and Ninja are downloaded into `.tools/` by the build | If a release has no macOS zip (its build didn't finish), take **Source code (zip)** and run `./launcher.sh`. Open **A3MX Launcher.app** (native on Apple silicon, macOS 11 or later; no Rosetta). It isn't notarised by Apple, so macOS blocks it the first time: open it once, then **System Settings > Privacy & Security > Open Anyway** (or right-click it and choose **Open**). Or skip the download's launcher: `./launcher.sh` builds one on your Mac, which macOS doesn't block. |

The first build extracts and checks your disc and compiles everything (a few minutes). After that, Run starts the game straight away (it rebuilds only what changed), or play with `build/a3mx --work work` (`build\\a3mx.exe` on Windows). Prefer the command line? `build.cmd --run` / `./build.sh --run` do the same. **Source code (zip)** is the same source without the launcher. Details: [Building]({url}/-/blob/{ref}/docs/BUILDING.md).

**Building from the source** (a clone, or **Source code (zip)**): `launcher.cmd` on Windows, `./launcher.sh` on Linux and macOS, builds the launcher and opens it; its **Dependencies** tab lists everything the build downloads, with each SHA-256, and downloads nothing until you agree. Nothing compiled is committed to the repository ([DEPENDENCIES.md]({url}/-/blob/{ref}/docs/DEPENDENCIES.md)).

"""


SEMVER = re.compile(r'^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$')


def key(v):
    m = SEMVER.match(v)
    return tuple(int(x) for x in m.groups()) if m else None


def sections(changelog):
    """[(version, text)] in the file's order (newest first)."""
    out, cur, lines = [], None, []
    for line in open(changelog, encoding='utf-8'):
        if line.startswith('## '):
            if cur: out.append((cur, ''.join(lines).strip()))
            cur, lines = line[3:].strip(), []
            continue
        if cur: lines.append(line)
    if cur: out.append((cur, ''.join(lines).strip()))
    return out


def split(text):
    """(for_players, technical) of one section's text"""
    m_p = re.search(r'^### For players[ \t]*\n', text, re.M)
    m_t = re.search(r'^### Technical[ \t]*\n', text, re.M)
    if not m_p and not m_t: return text.strip(), ''   # a flat list of bullets (0.55.0 on): all of it is for players
    if not m_p:
        tech = text[m_t.end():] if m_t else text
        return '', tech.strip()
    end_p = m_t.start() if m_t and m_t.start() > m_p.start() else len(text)
    players = text[m_p.end():end_p].strip()
    tech = text[m_t.end():].strip() if m_t else ''
    return players, tech


def check(v, since, secs):
    if not key(v): return 'VERSION %r is not MAJOR.MINOR.PATCH (e.g. 1.2.0)' % v
    if since and not key(since): return 'the previous release %r is not MAJOR.MINOR.PATCH' % since
    if since and key(v) <= key(since): return 'VERSION %s is not newer than the last release, v%s' % (v, since)
    if v not in dict(secs): return 'CHANGELOG.md has no section for %s' % v
    if not split(dict(secs)[v])[0]:
        return "CHANGELOG.md's section for %s is empty: list what changed, as bullets" % v
    return None


def body_for(chosen, title):
    """What's new (every version's For players part, newest first), then the technical details."""
    players = [split(t)[0] for _, t in chosen]
    players = [p for p in players if p and not p.startswith('- Nothing you')]
    out = '## %s\n\n' % title
    out += ('\n'.join(players) + '\n\n') if players else "Nothing a player will notice: behind-the-scenes work only.\n\n"
    if not any(split(t)[1] for _, t in chosen): return out   # flat sections only: nothing more to say
    out += '## Technical details\n\n'
    if len(chosen) > 1: out += '%d versions, newest first.\n\n' % len(chosen)
    out += ''.join('### %s\n\n%s\n\n' % (n, split(t)[1] or '(nothing technical)') for n, t in chosen)
    return out


NIGHTLY = """> **Nightly build: not an official release.** Built automatically from `main` as it was on {date} (commit [`{commit}`]({url}/-/commit/{commit})), after passing the same builds as a release. It is for trying what's new; it may be broken, and it isn't supported. For the supported version, use the [latest release]({url}/-/releases/permalink/latest). The next nightly replaces this one.
>
> Bug reports from it are welcome: the launcher's bug button fills in its version, `v{v}`, so we know which build it was.

"""


def nightly_notes(v, since, secs):
    m = re.match(r'^(\d+\.\d+\.\d+)-nightly\.(\d{4})(\d{2})(\d{2})\+([0-9a-f]+)$', v)
    if not m: sys.exit('release_notes.py: %r is not MAJOR.MINOR.PATCH-nightly.YYYYMMDD+COMMIT' % v)
    base, date, commit = m.group(1), '%s-%s-%s' % m.group(2, 3, 4), m.group(5)
    url = project_url()
    head = NIGHTLY.replace('{url}', url).replace('{v}', v).replace('{date}', date).replace('{commit}', commit)
    how = header(url, v.replace('+', '.'), commit)
    chosen = [(n, t) for n, t in secs if key(n) and key(n) <= key(base) and (not since or key(n) > key(since))]
    chosen.sort(key=lambda x: key(x[0]), reverse=True)
    if not chosen: return head + how + 'Nothing in CHANGELOG.md since the last release.\n'
    title = "What's new since %s" % ('the last release, v' + since if since else 'the start (there is no release yet)')
    return head + how + body_for(chosen, title)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version'); ap.add_argument('--since'); ap.add_argument('--check', action='store_true')
    ap.add_argument('--nightly', action='store_true')
    a = ap.parse_args()
    v, since = a.version.strip(), (a.since or '').strip().lstrip('v') or None
    secs = sections(os.path.join(ROOT, 'CHANGELOG.md'))
    if a.nightly:
        sys.stdout.write(nightly_notes(v, since, secs)); return
    err = check(v, since, secs)
    if err:
        print('release_notes.py: ' + err, file=sys.stderr); sys.exit(1)
    if a.check: return
    chosen = [(n, t) for n, t in secs if key(n) and key(n) <= key(v) and (not since or key(n) > key(since))]
    chosen.sort(key=lambda x: key(x[0]), reverse=True)
    title = "What's new in v%s" % v if len(chosen) == 1 else "What's new since v%s" % since if since else "What's new"
    body = body_for(chosen, title)
    sys.stdout.write(header(project_url(), v, 'v' + v) + body)


if __name__ == '__main__':
    main()
