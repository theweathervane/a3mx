# The `anthology` extension: design (spike)

How Hyper Street Fighter Alpha's character versions and S-ISMs, and the Anthology's music, would come
into A3MX as an **experimental** extension. What Hyper is and what is on the Anthology disc:
[HYPER_ALPHA.md](HYPER_ALPHA.md). This page is the plan for building it.

> **Status: experimental, in the game.** What is built is switched in OPTION MODE > EX. SETTING >
> EX. ANTHOLOGY: ANTHOLOGY DATA (FOUND / NOT FOUND, whether the launcher has imported your Anthology
> disc: Paths tab, Show optional images), then ANTHOLOGY, T.HAWK WALKBACK, ALPHA SELECT, S-ISM SELECT
> and S-ISM PRESENTATION, all OFF by default and greyed while NOT FOUND (their saved values kept). It
> is experimental: the versions are chosen and shown, but they play as Alpha 3 until the rules they
> need are decompiled, and S-ISM D keeps its supers for now. With ANTHOLOGY off, or no disc
> imported, the game plays as without it. The plan below is kept as written; where the build differs
> (the switch is in the game, not the launcher), the game's own comments say so.

## Principles

- **A feature like the others, off by default.** A new entry in the feature table
  (`src/platform/ext_features.h`), `anthology`, marked experimental like Stage Extensions. With it off,
  every frame is the same as without it, and the parity tests don't change.
- **Your disc, read in place.** It needs your own *Street Fighter Alpha Anthology* (PS2) as well as the
  PSP disc. Nothing from it is committed or shipped; the launcher extracts it the way it extracts the PSP
  disc.
- **Data in `assets/`, rules in code.** Character versions are tables in A3MX's own formats (Hyper's
  tables already are), so they are files under `assets/us/anthology/`, editable and moddable. Only the
  rules that change how a fighter plays are code.
- **No change to the PSP game's memory when it's off**, and as little as possible when it's on: a
  fighter's version is kept in the extension's own state, not in a spare byte of the PSP's fighter.

## 1. The feature

`src/platform/ext_features.h` gets a tenth entry, added at the end so saved launcher settings keep
their meaning:

```c
FEAT_ANTHOLOGY,      /* src/extensions/anthology/ */
...
{ "anthology", "Hyper Street Fighter Alpha (Anthology disc)",
  "Character versions from Street Fighter Alpha Anthology's Hyper Street Fighter Alpha (Alpha, Alpha 2,\n"
  "Alpha 2 Gold, Alpha 3, and the S-ISMs) on the select screens, and the Anthology's music.\n"
  "Needs your own Anthology disc (PS2), extracted by the launcher. Experimental.", 0, -1 },
```

- Off by default, needs no other feature. At start the game checks for `assets/us/anthology/index.json`;
  without it the feature switches itself off and the log says why (as `features_set` does for a missing
  dependency).
- Its hooks are named `anthology_...`, so `ext_features.c`'s prefix table puts them under the feature.
- `--features default,anthology` turns it on from the command line; `logs/last_run.txt` lists it.

## 2. The launcher

**Paths tab.** A second, optional disc row under the PSP one:

| Row | Default | Check |
|---|---|---|
| Anthology disc (PS2, optional) | the `.iso` in `imports/ps2/` | `SYSTEM.CNF` says `SLUS_213.17`, and that program's SHA-1 is `d3fd9235468c8301575767b8f5878a6f85d83df5` (US v1.01). The whole image's SHA-1 (`906b23e96a591d0bdc87201dacdef1ff7c875494`) is checked in the background, only to say whether it is the known dump. |

`disc_check.cpp` learns this disc the same way it knows the PSP ones: a table row and an ISO 9660 read
of the root directory (the Anthology keeps everything there). Other regions (the PAL release
SLES-54085, Japan's *Fighters Generation*) are recognised and reported as not supported yet.

**In the game, not the launcher (0.167.0).** Since main moved the extensions into OPTION MODE > EX.
SETTING, there is no Extensions-tab checkbox. EX. SETTING has a page of its own, **EX. ANTHOLOGY**
(after EX. DISPLAY): **ANTHOLOGY**, the whole extension (`[ex setting] anthology`, at once, as EX.
DISPLAY's rows; `--features -anthology` still wins for a run), then T.HAWK WALKBACK, ALPHA SELECT and
S-ISM SELECT (`hawk_walkback`, `alpha_select`, `sism_select`), all ON by default, each greyed and shown
OFF while it can't apply (the disc not imported; ANTHOLOGY OFF; S-ISM SELECT also without GAME OPTION >
MODE SELECT). The Paths tab says when the disc is
imported.

**Build / Run.** When the disc is chosen and good, `tools/build.py` adds a step after
the PSP extraction: `python tools/anthology.py extract` (below). It runs once; `assets/us/anthology/
manifest.json` records the disc and what was extracted, and a different disc or a newer extractor
runs it again, as `extract_reason` does for the PSP disc. Nothing is downloaded, so the consent page
doesn't change.

## 3. Extraction: `tools/anthology.py extract`

Built from the spike's commands (`survey`, `unpack`, `music`). Output, laid out by what it is (the
`assets/` rules in assets/LAYOUT.md; folder names in `tools/asset_layout.py`, the game reads
`index.json`):

```
assets/us/anthology/
  index.json, manifest.json
  music/            y000.ogg ... with LOOPSTART / LOOPLENGTH, index.json (game and command per track,
                    from tools/data/anthology_music_tables.json: `anthology.py music-tables`)
  screens/hyper_select/   the version labels, mode boxes, S-ISM text, per-version gauges (PNG)
  versions.json     every version number: group, character, Classical, flags, blocks
  characters/<nn>_<name>/<version>/   palette.bin (A3MX's fighter-c layout), palette.png,
                    version.json, raw/ (Hyper's blocks); later the tables in the formats of
                    characters/<nn>_<name>/fighter/data/ (06_data). <version>: alpha, alpha2,
                    alpha2gold, *_classical, *_altNN; characters/extra/ for the slots not yet named
work/anthology/     the unpacked Hyper overlay and other working files (not assets: not edited)
```

- The extractor checks the disc, unpacks the Hyper overlay (`zero9_usa.bin`), and runs a **converter**
  per character version: it follows Hyper's pointers to that version's tables and writes them in A3MX's
  layout. Every table it writes is checked against a known value (lesson M1: a converter is tested
  against a known value, not only against itself).
- Only the tracks Hyper's music list names are extracted by default (the list holds Alpha 3's, Alpha's,
  Alpha 2's and game 4's themes; 404 of the 588 tracks are in no game's list); `--all-music` keeps
  everything.

## 4. In the game: `src/extensions/anthology/`

| File | Does |
|---|---|
| `anthology.c` | The feature's start: reads `assets/us/anthology/index.json`, loads the version tables named there, keeps each fighter's version (`anthology_version[fighter]`: A3, A1, A2, A2G, S-ISM green / blue / pink / red, MANUAL / AUTO) |
| `select.c` | The select screen additions (5) |
| `versions.c` | At a fighter's set-up, its tables point at the chosen version's instead of the PSP's (the PSP loads a fighter's tables from its `pl<nn>` archive; the hook swaps the entries the version replaces) |
| `rules.c` | The rules that differ by version (6), each a `SFA_FUNC_WRAPPED` hook that does nothing for an Alpha 3 fighter |
| `music.c` | Optional: an Anthology track for a stage or screen, from `assets/us/anthology/music/index.json` |
| `hud.c` | The per-version gauges and the S-ISM single-level gauge |

**Where Hyper's rules are.** Hyper tests the version flag byte at +0xD3 of its objects (the fighter's
own, and its effects', which copy their owner's) in 799 functions; `tools/data/hyper_d3_sites.json` lists them with the bits each tests.
That list is the work list for `rules.c`: each entry is read, the rule it applies is written down, and
the matching place in A3MX's (decompiled) fighter code gets the hook. Confirmed from save states: a fighter's +0x102 is
its character-version number and +0xD3 its flags: 0x10 Alpha 3, 0x01 Alpha, 0x22 Alpha 2, 0x06 Alpha 2
Gold, + 0x08 a Classical (Champion Edition) version (the Alpha 2 blue slots, and the pink S-ISM). +0x132 is
the ISM. The S-ISMs are marked in +0x122 / +0x123 (green 0x82 / 4, blue 0x02 / 3, red 0x80 / 2) and, for
pink, CLASSICAL's mode byte +0xF0 with bit 0x80 and +0xD3 bits 0x08 and 0x80 (zero9 0x2891A0, 0x295F40,
0x4B6BE0). Each version's move and attack data is reached through 128-entry tables by version number
(0x6A6BD4 and others). AUTO guard is +0x118 with
its count at +0x32F; details in HYPER_ALPHA.md.
A3MX's `anthology_version[fighter]` can use the same numbering, so Hyper's tables index straight in.

**Dependence on the decomp.** Hooks go on decompiled functions. The select screen is largely native
already; the fighters' move and hit code (H4's rules) mostly isn't. So the first steps (1–3, the select
screen, the data swap) can be built now, and each rule waits for its function to be decompiled.

## 5. The select screen (the maintainer's scope, 2026-11-15)

**In scope:**

| Where | Modes | Select screen (code) |
|---|---|---|
| MODE MENU | ARCADE, R.DRAMATIC BATTLE, VS 100 KUMITE, FINAL BATTLE | the character select: `src/core/select/cursor.c` / `screen.c` (each player's cursor runs states 0–4: character, style, ISM, speed, done) |
| FREE BATTLE | ARCADE, R.DRAMATIC BATTLE | the Free Battle / Survival / Training select: `battle_select.c` / `battle_display.c` |
| SURVIVAL | ARCADE, ORIGINAL, BOSS, INFINITE, R.DRAMATIC, R.DRAMATIC (lives) | `battle_select.c` |
| TRAINING | | `battle_select.c` |

**Not decided yet (left alone for now):** DRAMATIC and VARIABLE on the MODE MENU, Survival VARIABLE and
DRAMATIC, Free Battle TEAM, VARIABLE and DRAMATIC BATTLE. Any two characters can be picked there that share
one super gauge, and the versions' gauges differ (Alpha's three levels, the S-ISMs' one), so those modes need
a rule for mixed gauges before versions can be offered in them.

**The character select (MODE MENU modes, Free Battle ARCADE / R.DRAMATIC):**

Now: SELECT PLAYER > MODE SELECT > ISM SELECT > SPEED SELECT.
New: SELECT PLAYER > **ALPHA SELECT** > MODE SELECT > ISM SELECT > SPEED SELECT.

1. **ALPHA SELECT** (new state between the character and the style): ALPHA, ALPHA2, ALPHA2G, ALPHA2GC,
   ALPHA3, in the classic green font, with a vertical ticker-tape title graphic "ALPHA SELECT" like the
   existing steps' (the maintainer's mock: `docs/images/anthology/ticker_alpha_select_mock.png`, dark
   pixel letters running down a grey-blue strip). **ALPHA2GC** is the Classical (blue) version as its own
   choice, so no Start press is needed; it is offered for the seven Classical characters (Ryu, Ken, Chun-Li,
   Sagat, M. Bison, Zangief, Dhalsim: Gold slots 0x75, 0x76, 0x78–0x7C) and drawn in blue. Only the versions
   the character has are listed (Alpha for Alpha 1's 13, Alpha 2 / Gold for their rosters, Alpha 3 for all);
   a character with Alpha 3 only skips the step.
2. **MODE SELECT**: only after ALPHA3, and only when MODE SELECT is on in GAME SETTING (the extension turns
   it on by default). Eight choices instead of four: NORMAL, MAZI, SAIKYO, CLASSICAL and the four S-ISMs
   (green, blue, pink, red), shown as a **3-item carousel**: the selected tile full size in the middle and on
   top, the one before and the one after at about 80–85 % behind its top and bottom edges, dimmed or
   desaturated; up / down rotate. It starts on NORMAL. Pink S-ISM is only for the SF2 fighters (Hyper's mask
   0x13218C13: Ryu, Ken, Chun-Li, M. Bison, Sagat, Dhalsim, Zangief, Balrog, E. Honda, Blanka, Vega), so the
   carousel has seven choices for everyone else (maintainer confirmed). The game's style step today steps through the styles
   `styles_offered` (0x088A4F1C) allows, drawn as four stacked titles; the carousel replaces that drawing
   when the feature is on. Tile look (the maintainer's mock: `docs/images/anthology/mode_tile_sism_mock.jpg`):
   a "MODE" header in light grey above a framed tile, the mode name inside in large pixel letters; the mock is
   the green S-ISM (green name, green frame). Each S-ISM tile is drawn in its own colour (green, blue, pink,
   red; maintainer confirmed).
3. **ISM SELECT**: MANUAL / AUTO (guard) after ALPHA, ALPHA2, ALPHA2G and ALPHA2GC (blue font for ALPHA2GC);
   after ALPHA3 with NORMAL, MAZI, SAIKYO or CLASSICAL the game's own X / A / V as now (CLASSICAL forces X
   as the game does). **After an S-ISM tile there is no ISM step** (maintainer, 2026-11-15): the ISM is the
   one that S-ISM has in Hyper (0x2891A0: green and red A, blue and pink X) and the cursor goes straight to
   SPEED SELECT. This is the game's own "ISM decided" path, which an entry (EDIT) or World Tour character
   already takes in Arcade (no ISM step): `ism_decided()` in `cursor.c` (+ `F_ISM_FIXED`, an entry box, a
   random box) and `ism_enter()` (0x088A382C) in `battle_select.c`. The extension adds "an S-ISM is chosen"
   as one more decided case rather than a new skip.
4. SPEED SELECT as now.

**Survival and Training (the option-row select):**
- **TYPE**: a new row above MODE: ALPHA, ALPHA2, ALPHA2G, ALPHA2GC (the seven, blue font), ALPHA3.
- **MODE**: the existing rolling row gains S-ISM G, S-ISM B, S-ISM P, S-ISM R (only with TYPE = ALPHA3;
  S-ISM P only for the SF2 fighters).
- **ISM**: MANUAL / AUTO for ALPHA, ALPHA2, ALPHA2G, ALPHA2GC; unchanged for ALPHA3, except that with an
  S-ISM MODE the row behaves as it does for an entry character today: shown, then skipped automatically
  to SPEED (the game shows A-ISM there for an entry; for an S-ISM it shows the S-ISM's ISM).

**Everywhere:** the versus screen shows the version under each name ("ALPHA2G", "S-ISM"). For an S-ISM
the ISM line follows the game's entry-character precedent (an entry shows "I-ISM" in yellow there): the
S-ISM's label in its own colour in place of the ISM (versus-screen drawing code not yet located); the choices are
kept per player (`anthology_version[fighter]`) and saved with the other select-screen memory.

Layout and art: the maintainer's design (above) and graphics (`docs/images/anthology/`). New labels and
the carousel tiles are drawn the way `selectscreen.c` draws JOIN and WAIT (textured quads through the
game's 2D library), from PNGs in `assets/us/anthology/screens/`.

**Stages.** The stage follows the version, as in Hyper (0x2949E0 picks the stage set from the version:
Alpha → the Great Wall, Alpha 2 / Gold → Venezuela, Alpha 3 → the character's own). Those two stages are
**out of scope** (the maintainer, 2026-11-26: the spike is the modes and the Alpha versions only): every
version fights on the character's Alpha 3 stage.

### Built (2026-11-24, `src/extensions/anthology_select.c`)

The select-screen part of E1, behind EX. SETTING > EX. ANTHOLOGY's **ALPHA SELECT** and **S-ISM SELECT** (both ON by
default, greyed without the Anthology disc imported, kept in a3mx.cfg as `alpha_select` / `sism_select`):

- Character select (cursor.c wrapped at 0x08894644, 0x08894990, 0x0889507C, 0x08895248, 0x08894C74; the
  sprite cache at 0x0880E150 / 0x0880EE34): ALPHA SELECT held at the end of the character's wait, before
  the game's MODE panel comes in, as Hyper shows it (the maintainer's videos): the versions in the game's
  font 1 (green; ALPHA2GC blue), a white arrow, and an ALPHA SELECT ticker lettered from the same font; then
  MANUAL / AUTO (GUARD SELECT) for the older versions, which decides MODE (NORMAL) and the ISM (A). MODE
  SELECT as a three-tile carousel drawn with the game's own tiles: the panel's eight draws (frame picture
  139 and names 140 - 143 of slot 8's list) are kept by style each frame (and still run, out of sight, as
  the next piece's palette base follows from them) and drawn again where the carousel puts them, the front
  one full size, the neighbours at 80 % behind and fainter, the game's arrow moved to the front one. An
  S-ISM tile is its colour's frame with a name of ours made from the game's letters (S of SAIKYO, I and M
  of MAZI). No ISM step after an S-ISM (green / red A, blue / pink X, through 0x08895564).
- Survival and Training (battle_select.c wrapped at 0x088A2870, 0x088A34A8, 0x088A35E4, 0x088A382C,
  0x088A3914, 0x088A60F8, 0x088A4D30): a TYPE row above MODE with a blank line between (blinking while chosen; ALPHA2GC in blue; it
  takes MODE's place for the older versions), MODE going on to S-ISM G / B / P / R in their colours,
  MANUAL / AUTO in the ISM row, no ISM row after an S-ISM. Survival's two-against-one and team fights are
  left out, as are Dramatic, Variable, World Tour and network play.
- The choices are kept per player and logged; **the fight is still Alpha 3** (E2, E3 next).

Not done yet: the versus screen's version labels; player 2's side checked by eye (drawn mirrored).

## 6. Tests

- **Off changes nothing:** the existing parity scenarios run with the feature off (it is off by default);
  `no_extensions` as before.
- **No disc:** with the feature asked for and no `assets/us/anthology/`, the game starts, the log says the
  feature is off and why, and every frame matches a run without it.
- **Extraction:** `tools/anthology.py extract --check` on the maintainer's disc: every converted table
  against its known values; music lengths and loops against the ADX headers (as done for H1).
- **Select screen:** a scenario with the feature on that picks ALPHA2G Ryu and checks the screen
  (`--dump`) and the chosen version in the log.
- **Per rule:** a scenario per version rule once it is hooked, with frame data recorded from the
  Anthology run in an emulator as the reference (HYPER_ALPHA.md).

## 7. Order of work

| Step | What | Depends on |
|---|---|---|
| E0 | The feature entry, the launcher's disc row and check, `anthology.py extract` (music, art, the overlay), the extension's skeleton (loads the index, logs, does nothing else) | nothing |
| E1 | Select screens as in 5: ALPHA SELECT, the MODE carousel with the S-ISMs, MANUAL / AUTO, Survival / Training TYPE and MODE rows, versus-screen text, stored versions | E0, the maintainer's art |
| E2 | One version end to end: Alpha 1 Ryu's tables converted and swapped in (moves as data; rules still Alpha 3's) | E1, H0+ (his tables mapped) |
| E3 | The rules, one by one from `hyper_d3_sites.json`, as their functions are decompiled | E2, the decomp |
| E4 | The other character versions (converter runs; each checked) | E2 |
| E5 | ~~The Great Wall and Venezuela stages~~: out of scope (2026-11-26) | |

## Open questions for the maintainer

Answered 2026-11-15: which modes (5, in scope), and versions inside the existing modes (no separate
HYPER entry).

1. The Anthology's music: an option to play its tracks on A3MX's stages, or only with Hyper versions?
2. Japan's *Street Fighter Zero: Fighters Generation* disc: support later, like the JP PSP disc?
3. **Sprites (from H5):** Hyper's older versions use sprite records of their own. Match them to A3MX's
   sprites by picture (versions drawn with A3MX's own sprites), or bring Hyper's sprites in as the
   extension's own set? (HYPER_ALPHA.md, "The sprites are the real problem".)
4. Alpha 2 (not Gold) also has Classical Zangief and Dhalsim slots (0x55, 0x56) on the disc: offer an
   ALPHA2C for those two, or Classical only under Gold as ALPHA2GC?
5. The modes left out (Dramatic, Variable, Team): when they come, should a pair with different gauges be
   refused, or should each fighter keep its own gauge?
