# Contributing

Issues and merge requests are welcome. To report a problem, see
[docs/REPORTING_ISSUES.md](docs/REPORTING_ISSUES.md) (what to include, and how to attach your log).

## Before you start

- **Never commit game data:** no disc images, extracted files, `work/`, `assets/` (except its README,
  LAYOUT.md and `extensions/`, which hold new art, not the game's),
  `build/` or anything built from them. The `.gitignore` covers the usual ones; check `git status`
  before you commit.
- **Keep `assets/` in order:** new files go where [assets/LAYOUT.md](assets/LAYOUT.md) says, and
  names come from `tools/asset_layout.py` only.
- For anything bigger than a small fix or a batch of decompiled functions, open an issue first so we
  can agree on the approach.
- Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how the pieces fit,
  [docs/DECOMPILING.md](docs/DECOMPILING.md) for how to decompile. If something worries
  you or looks wrong, open an issue.

## Branches

`main` is always buildable and passes `ctest`. Contributors change it through merge requests; work on a
branch named for the kind of change, in lower case with hyphens:

| Prefix | For | Example |
|---|---|---|
| `decomp/` | decompiled functions | `decomp/libc-string` |
| `feature/` | something new | `feature/pac-extractor` |
| `fix/` | a bug fix | `fix/button-mapping` |
| `docs/` | documentation only | `docs/program-map-sound` |
| `chore/` | build, tooling, tidying | `chore/build-script` |

Keep a branch to one change, and keep it short-lived: branch from the latest `main`, and rebase or
merge `main` in if it moves on.

## Commits

- Small commits with a short summary line (under about 72 characters), then a blank line and a few
  lines on *why*, if it is not obvious.
- No generated code, logs, screenshots of copyrighted material or personal paths in commits.

## Merge requests

1. Push your branch (to your fork, if you do not have write access) and open a merge request into
   `main`. Say what it changes and how you tested it.
2. CI doesn't run on merge requests; the maintainer may start the pipeline on your branch, which
   builds on Windows, Linux and macOS and runs the tests that need no disc. The parity tests need the
   disc, so run them yourself: for decompiled code, give the `ctest --preset release` (or
   `build.sh --test`) result and the scenario that runs your functions (add one if none does). For
   features and fixes: which mode, which controller, and screenshots where they help.
3. Update the docs the change touches (README, `docs/`, `tools/data/hooks.txt` for a new hook,
   `docs/PSP_PROGRAM_MAP.md` for new findings about the program), and take again any screenshot it
   changes: `python tools/screenshots.py --check` says which are out of date, and
   `python tools/screenshots.py ID` takes one ([BUILDING.md](docs/BUILDING.md#screenshots)).

The maintainer reviews, may ask for changes, and merges. Merging is not a release, and only the
maintainer pushes to the project.

## Code

- **Decompiled code** (`src/core/`): C11, one `SFA_FUNC` per game function, readable implementation
  behind a thin adapter. Exact behaviour, quirks included; game bugs are fixed only in
  `src/extensions/`, behind a setting. Full rules: [docs/DECOMPILING.md](docs/DECOMPILING.md).
- **Platform** (`src/platform/`): C for the runtime, C++ only where vendored code needs it.
- **Extensions** (`src/extensions/`): the extensions from the static recomp, and new ones. A game function replaced by
  native code is listed in `tools/data/hooks.txt`, named `hook_f_XXXXXXXX`, and calls `orig_f_XXXXXXXX`
  around its own changes. Everything in `src/extensions/` must be optional: the game has to build and
  run without it (`--no-extensions`, `-DA3MX_EXTENSIONS=OFF`); the platform calls into it only when
  `rt_extensions` is set, and `src/platform/extensions_off.c` stubs any name it refers to.
- **Tools** (`tools/`): Python 3.9+, standard library only.
- **Dependencies:** every download and vendored folder is pinned in
  [tools/data/pins.json](tools/data/pins.json) by SHA-256 (downloaded source by commit), listed in
  [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md). To add or update one, change `pins.json` (and the copy in
  `build.cmd` or `cmake/SDL3.cmake` if it has one), run `python tools/pins.py --write`, and commit both.
  The pipeline's `pins` job fails until everything agrees; after changing a download, run
  `python tools/pins.py --download` to fetch and check it. Never use a download, a branch tip or a tag without a hash.
- Comments say what the game's addresses are and why a change is made. Known problems go in
  [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).

## Releases

Releases are made by hand, by the maintainer (there is no nightly for now):

- Every merge request raises `VERSION` (MAJOR.MINOR.PATCH) and adds its section at the top of
  [CHANGELOG.md](CHANGELOG.md): one bullet per change, a bold lead, then what a player will notice
  (`python tools/release_notes.py --check VERSION`). The game and the launcher show the version.
- Before a release, `python tools/screenshots.py --check` must pass, so the documentation's
  screenshots show what is being released.
- The maintainer tags the commit on `main` as `v<VERSION>` and pushes the tag; the pipeline builds all
  three systems and makes the release, with notes from every changelog section since the last one
  ([docs/BUILDING.md](docs/BUILDING.md#releasing)).

## Reporting a bug

Use the bug report form. Include the version (bottom corner of OPTION MODE > EX. SETTING), the mode,
your controllers, the steps to make it happen, and `logs/a3mx.log` if you can.

## Licence

By contributing you agree that your contribution is licensed under the project's licence,
GPL-3.0-or-later ([LICENSE](LICENSE)).
