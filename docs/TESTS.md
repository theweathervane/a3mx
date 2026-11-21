# Tests

How to run the tests, step by step, then what each test checks, why, and how long it takes. The tests
need the release build and your disc's files in `work/` ([BUILDING.md](BUILDING.md) sets both up).

## Running the tests

### 1. Build

From the project folder (`C:\_Repos\a3mx` or wherever you cloned it):

```
cmake --build --preset release
```

`build.cmd` / `./build.sh` do the same and more. Don't rebuild while tests are running: they use the
program in `build/`, and a rebuild changes it under them.

### 2. A quick check (about 5 minutes)

Two short scenarios first. If these fail, there's no point running the rest:

```
ctest --preset release -j 2 -R "boot_to_select|arcade_fight"
```

`-R` picks tests whose names match. `-j 2` runs two at once.

### 3. The whole suite

```
ctest --preset release -j 2
```

**How many at once (`-j`).** Each parity test runs the game twice side by side (the original and the
decompiled code), so `-j 2` keeps about four cores busy. On a laptop, stay at `-j 1` or `-j 2`. Higher
numbers finish sooner, but hours of every core flat out can overheat a laptop: a black screen or a
shutdown part way through means "lower `-j`", not "the tests are broken". A desktop can take about
half its core count (`echo %NUMBER_OF_PROCESSORS%` on Windows, `nproc` on Linux, `sysctl -n hw.ncpu` on
a Mac).

The whole suite is about 2½ hours on a 2-core machine at `-j 2`; a desktop is three to five times
quicker.

### 4. Saving the results to a file

ctest can print to the screen and write a log at the same time, the same way on every system:

```
ctest --preset release -j 2 --output-log test_run.txt
```

Or with your shell's tools:

| System | Screen and file | File only |
|---|---|---|
| Windows, Command Prompt | `--output-log test_run.txt` (above) | `ctest --preset release -j 2 > test_run.txt 2>&1` |
| Windows, PowerShell | `ctest --preset release -j 2 2>&1 \| Tee-Object test_run.txt` | `ctest --preset release -j 2 *> test_run.txt` |
| Linux, Mac | `ctest --preset release -j 2 2>&1 \| tee test_run.txt` | `ctest --preset release -j 2 > test_run.txt 2>&1` |

The file lands in the folder you ran the command from.

### What you'll see

Each test prints a line when it starts and one when it ends. Tests run side by side, so they finish out
of order:

```
Test project C:/_Repos/a3mx/build
      Start 12: parity_arcade_match
      Start 27: parity_network_match
 1/13 Test #27: parity_network_match .............   Passed  334.49 sec
      Start 15: parity_attract_mode
 2/13 Test #12: parity_arcade_match ..............   Passed  355.04 sec
 ...
13/13 Test #17: parity_boot_to_select ............   Passed   81.68 sec

100% tests passed, 0 tests failed out of 13

Label Time Summary:
parity    = 2613.34 sec*proc (13 tests)

Total Test time (real) = 1318.70 sec
```

A failed parity test says `***Failed`. Its message, shown at the end of the run (or in the log), names
the first frame that differs, for example:

```
 4/13 Test #15: parity_attract_mode ..............***Failed  712.30 sec
...
PARITY FAIL in attract_mode at frame 5894: memory differ.
  Bisect with --reference-only ADDR[,ADDR] (hex) on the decomp run, and compare screens with
  --dump DIR --dump-from 5889 --dump-every 1 on both runs.
```

The frame number says how far into the scenario things went wrong; [DECOMPILING.md](DECOMPILING.md) has
what to do next. To run only the tests that failed last time:

```
ctest --preset release --rerun-failed --output-on-failure
```

### Other ways to run them

- `build.cmd --test` / `./build.sh --test`: builds, then runs every test, one at a time.
- The launcher's **Run the tests after building**.
- In a container (a Docker host or Portainer, with your ISO mounted read-only):
  [docker/tests/README.md](../docker/tests/README.md).
- A list of what there is, without running anything: `ctest --preset release -N`.

### Which build runs which tests

| Build | What it is | Needs your disc | Tests |
|---|---|---|---|
| `ci` preset (`build-ci/`) | every hand-written file compiled against a stand-in for the generated code; this is what the GitLab pipeline builds | no | `pacing_rules`, `png_cases`, `png_decoder` |
| `headless` or `release` preset | the real game | yes (`work/`) | everything below |

The times in the tables below are from a 2-core cloud machine with `--fast` (the game not waiting for
the clock).

## Why parity

The decompilation replaces the translated program one function at a time. A function is only merged
when a test proves it behaves exactly as the original: that is what **parity** means here. Each parity
scenario plays the game twice from the same scripted input, once with the decompiled functions and
once with `--reference` (the translated original of each), and compares a hash of the program's
memory and of the picture **on every frame**. The first frame that differs fails the test and names
the frame. Extensions run in both, so they are compared too.

A scenario also records which decompiled functions it ran (`--decomp-report`), which is how
`tools/progress.py` knows a function is verified.

A scenario with a `wide` line is a **WIDE NATIVE check** instead: it plays twice with the decompiled
code, SCREEN SIZE as the game sets it and WIDE NATIVE, and the four fighters (0x092E3EA0 - 0x092E5100,
the fourth hash in `--hash-log`) must be the same in every frame. WIDE NATIVE only shows more of the
screen; it must never change the fight. What it draws is checked by eye.

## Tests that need no disc

| Test | Checks | Why | Time |
|---|---|---|---|
| `pacing_rules` | FRAME TIMING's rules against simulated monitors (`tests/pacing_test.c`): SMOOTH and ORIGINAL never change on their own, AUTO chooses only on the MODE MENU and on leaving SCREEN SETTING | the SMOOTH / ORIGINAL flicker fixed in 0.60.0 could only be seen on particular monitors; this reproduces it anywhere | under 1 s |
| `png_cases` | writes PNGs the way image editors do (every colour type, interlaced, odd sizes) | the fixture for the two tests below | 1 s |
| `png_decoder` (`ci` only) | the project's PNG reader on those files | edited assets come back as PNGs from many programs | 1 s |

## Tests that need the disc

| Test | Checks | Why | Time |
|---|---|---|---|
| `difftest` | every decompiled function against its original on 20,000 random inputs each | covers code paths no scenario reaches | 1 to 3 min |
| `asset_pac` | archives rebuilt from `assets/` are byte-identical to the disc's | unedited assets must change nothing | 1 to 2 min |
| `pac_roundtrip` | every `.pac` unpacked and packed again is identical | the archive format is understood completely | 1 min |
| `assets_in_game` | the game run from `assets/` matches the run from the disc, frame for frame | loading from folders must not change the game | 5 to 10 min |
| `stage_strips` | with WIDE NATIVE and Stage Extensions on, the stage's edge is not black | the strips once silently stopped drawing after a merge | 4 min |

`asset_pac`, `assets_in_game` and `stage_strips` need `assets/us` extracted (`python tools/extract_assets.py`).

## Parity scenarios (`tests/scenarios/*.txt`, each `parity_<name>`)

Parity runs have the `shaders` feature off (`tests/parity.py`): the scenarios' menu scripts count
SCREEN SETTING's rows without FILTER. FILTER itself draws after the game's picture is finished and
touches nothing the hashes cover.

| Scenario | Plays | Proves | Time |
|---|---|---|---|
| `boot_to_select` | boot, logos, attract, title, MODE MENU, Arcade's character select | the boot path and first menus | 3 min |
| `attract_mode` | boot and press nothing: all four demos and the cycle back | the demo fights and timers | 6 min |
| `mode_menu` | the MODE MENU left alone, then every direction | menu timers, cursor, the bar's bubbles and timing badge | 4 min |
| `mode_tour` | into one mode after another and back with F2 | each mode's entry and the quit key | 6 min |
| `stick_menus` | the analogue stick through menus and the select | stick-to-direction handling | 4 min |
| `select_choices` | the character select's cursor, random box, ISM and speed | the select screen's logic | 5 min |
| `arcade_fight` | Arcade's first fight with moves, specials and a CPU opponent | the fight loop | 7 min |
| `arcade_other_fighter` | the same fight as E. HONDA (one box right on the select screen) against DAN | fighter and stage code RYU's first fight doesn't run | 7 min |
| `arcade_match` | a whole match and what follows it | round ends, win screens | 8 min |
| `arcade_ending` | the final fight, the ending, the save error questions | endings and the save path's failures | 8 min |
| `arcade_ending_full` | the final fight won and Ryu's ending played, not skipped | the ending task and its tile layers (0x0881B7C8) | 8 min |
| `arcade_rival_dialogue` | the first fight met as the rival fight: the two portraits and their lines before it | the dialogue's text boxes (0x0880C1AC / 0x088108D8) | 7 min |
| `super_finish` | Free Battle, both fighters players: a KO by Ryu's Shinku Hadoken | the super finish's backdrop (0x0880C208) | 3 min |
| `arcade_join_select` | Arcade with 2P PRIORITY PLAYER: pad 2 joins with Start on the select screen, picks Ken and jumps in the fight | a pad that PRIORITY gave a fighter keeps it when it joins (issue #17) | 6 min |
| `free_battle_join_in` | Free Battle with 2P PRIORITY PLAYER and pad 2 idle | PRIORITY leaves a box that can be joined with Start to the computer (issue #17) | 5 min |
| `boot_to_menu` | the `boot_to_menu` feature: straight to the MODE MENU after the load, then into Arcade's select | the boot path skipping the title and PRESS START | 1 min |
| `select_start_menu` | SELECT + START in Arcade's first fight | back to the MODE MENU from player 1's pad, no pause menu | 3 min |
| `quit_to_menu` | pause, QUIT GAME, back to the MODE MENU | leaving a fight cleanly | 5 min |
| `free_battle`, `free_battle_rows`, `free_battle_team`, `free_battle_dramatic`, `free_battle_rdramatic` | Free Battle's select in each kind, every row moved | the multi-player select and its turns | 5 min each |
| `option_tour` | every OPTION MODE submenu with the extensions | the option screens and our additions | 6 min |
| `option_tour_original` | the same with `--no-extensions` | the game's own option screens | 5 min |
| `screen_setting` | SCREEN SETTING's rows, VSYNC and FRAME TIMING included | our rows behave as the game's do | 3 min |
| `save_load` | SAVE, LOAD and DELETE | the silent save path | 4 min |
| `pause_config`, `pause_config_original`, `pause_config_players` | the pause menu's BUTTON CONFIG, with and without extensions, for players 2 and 3 | per-player button config | 5 min each |
| `wide_fight` | Arcade's first fight, WIDE NATIVE off and on (`wide`) | widescreen changes nothing about the fight | 7 min |
| `wide_walls` | Free Battle, WIDE NATIVE off and on (`wide`), Blanka against Blanka, player 1 at the left wall then walking right | a fighter at the wall plays the same while drawn into the left margin (the cell drawers' left limit; the drawing by eye) | 7 min |
| `wide_left_wall` | Free Battle on Ryu's stage, WIDE NATIVE off and on (`wide`), the camera at its left limit | the fight is the same with the tile drawers' extra left columns wrapping round within their row (layer 0 starts at map column 1 there; the drawing by eye) | 6 min |
| `network_match` | NETWORK > VS MODE against the virtual peer: the menus, IP OPTION, the select and a whole match | the network screens' rows, panels and message boxes | 8 min |
| `disc_faults` | boot with a disc whose reads fail or stall | the loader's retry paths | 4 min |
| `no_extensions` | the game with every feature off | the PSP program alone still runs, and matches | 2 min |

The whole suite is about 2½ hours at `-j2` on that 2-core machine. Before a change to `src/core` or
`src/extensions` is called ready, run the shortest scenario that reaches the code first
(`ctest -R "boot_to_select|arcade_fight" -j2`), then the lot. Don't rebuild a folder while its tests
run: configure a second one (`cmake --preset headless -B build-h2`).

## The GPU renderer (`-DA3MX_EGL=ON` builds, Linux)

The parity tests draw with the software renderer. A build configured with `-DA3MX_EGL=ON` (headless
OpenGL through EGL; Mesa's llvmpipe is enough, no graphics card needed) adds two tests of the GPU
renderer (`ge_gl.c`), label `gl` (`ctest -L gl`):

| Test | Checks | Why | Time |
|---|---|---|---|
| `gl_renderer_normal`, `gl_renderer_wide` | `arcade_fight` with the software renderer and the GPU renderer at scales 1 and 2, in NORMAL and in WIDE NATIVE | the renderer never changes the game (every frame's memory and fighters the same), and draws what the software renderer draws (every 30th frame, within a tolerance) | 6 min each |

`tests/gl_test.py` compares the pictures after averaging 4 x 4 squares, a channel allowed to differ by
32 and at most 0.75% of the squares over that: the renderers differ a little at triangle and text edges
(worst measured 0.05% at scale 1, 0.39% at scale 2), and a single sprite left out is about 1.8%. A
failure writes the worst frames as software | GPU | difference PNGs into `build-*/gl_test/`. Any
scenario can be run the same way: `python tests/gl_test.py --exe build-egl/a3mx --work work --scenario
tests/scenarios/attract_mode.txt --size wide --scales 2 --every 10`.

## What no test covers

- **The GPU renderer on a real GPU and on Windows.** `gl_renderer_*` (below) run it through Mesa's
  llvmpipe on Linux. A change to `ge_gl.c` can also be compared old build against new, frame for frame:
  `python tools/compare_gl_frames.py` with two `A3MX_EGL` builds.
- **FILTER's shaders** (`shader_gl.c`): they need a GPU, librashader and the shader files.
- **The launcher.** It takes its own screenshots (`--screenshot`), and `python tools/screenshots.py
  --check` fails when one is out of date, but nothing presses its buttons.
- **Real monitors and controllers.** `pacing_rules` simulates monitors; controller mapping is checked by
  hand (the launcher's **Test**).
- **Features switched off one at a time.** Only "all on" and "all off" have scenarios.

## Watching a scenario

The tests above run **headless**: no window, no GPU, nothing on screen. They play a scenario's button
presses as fast as they can and compare memory with the disc's own run. That's why they run fine over
SSH or on a server.

`tools/watch_scenario.py` is the other way round. It plays the same button presses **in a real window,
at the game's own speed**, so you can see what happens. It compares nothing, it isn't part of `ctest`,
and it needs what the game itself needs: a desktop and a graphics card (the GPU renderer, OpenGL). On a
machine with no screen, use `ctest` instead.

### 1. Build the game

The watch tool runs `build/a3mx.exe` (Windows) or `build/a3mx` (Linux, Mac), the normal release build:

```
cmake --build --preset release
```

### 2. Watch one

From the project folder:

| | Command |
|---|---|
| Windows (cmd or PowerShell) | `python tools\watch_scenario.py arcade_fight` |
| Linux / Mac | `python3 tools/watch_scenario.py arcade_fight` |

A window opens and the game plays itself: boot, menus, the character select, the fight. The window
closes by itself when the scenario's frames run out. Close it sooner to stop.

It opens **in a window** by default. For full screen:

```
python tools/watch_scenario.py arcade_fight --fullscreen
```

F11 or Alt+Enter switch between the two while it plays.

Any name in `tests/watch/` (the endings and super finishes, below) or `tests/scenarios/` (the parity
scenarios) works, without the `.txt`. `--list` shows them all with a line on each:

```
python tools/watch_scenario.py --list
```

### 3. Options

| Option | What it does |
|---|---|
| `--screen S` | SCREEN SIZE: `NORMAL` (4:3, the default here), `WIDE` (stretched) or `NATIVE` (WIDE NATIVE). Case doesn't matter |
| `--character NAME` | player 1's character, in the scenarios with a `select` line (all of `tests/watch/`). `--character list` shows the names. Player 2 stays as the scenario has it |
| `--fullscreen` | full screen instead of a window |
| `--skip-to F` | run at full speed up to frame F, then at normal speed. The watch scenarios skip their menus this way already; `--skip-to 0` watches from the start |
| `--frames N` | stop after N frames instead of the scenario's own count |
| `--renderer soft` | the software renderer instead of the GPU one (slower; for comparing the two) |
| `--assets` | load the files in `assets/` (the stage strips) |
| `--list` | the scenarios there are |

Examples:

```
python tools/watch_scenario.py ending_arcade --character chun-li --screen native
python tools/watch_scenario.py super_kind_7 --character sakura --screen wide --fullscreen
python tools/watch_scenario.py ending_arcade --character list
```

To go through every ending in WIDE NATIVE, one after another (Windows cmd):

```
for %s in (arcade kumite dramatic rdramatic variable final) do python tools\watch_scenario.py ending_%s --screen native
```

On Linux or Mac:

```
for s in arcade kumite dramatic rdramatic variable final; do python3 tools/watch_scenario.py ending_$s --screen native; done
```

### 4. The endings (`tests/watch/`)

Each one plays a mode to its end in a few minutes. Menus and the character select go by at full speed;
the last fight starts at normal speed. The game is told the fight is the last one, the opponent's
computer control is switched off so they stand still, their health is kept at one point, and your
character walks up and jabs: a hit or two ends it. Then the ending plays out: the congratulations,
the story or the credits, GAME OVER and back to the MODE MENU, where the window closes.

| Scenario | What you see | `--character` |
|---|---|---|
| `ending_arcade` | Arcade: the last fight, your character's ending and the credits | yes |
| `ending_kumite` | VS. 100 Kumite: STAGE 100, ALL CLEAR, the credits and 100 WINS 0 LOSE | yes |
| `ending_dramatic` | Dramatic Battle: two on one against the boss, ALL CLEAR and the credits | yes |
| `ending_rdramatic` | Reverse Dramatic Battle: one against two, ALL CLEAR and the credits | yes |
| `ending_variable` | Variable Battle: a tag team, ALL CLEAR and the credits | yes |
| `ending_final` | Final Battle: M. Bison, then your character's ending and the credits | yes |

Survival has no ending of its own (it ends on a score screen when you lose), so it has no scenario
here. World Tour, Training, Free Battle and the rest don't end.

The fight itself is the scenario's own button presses (walk forward, two jabs, again), not you: the
pad does nothing useful while it plays.

### 5. The super finishes (`tests/watch/super_kind_1` - `super_kind_8`)

A two-player Free Battle where player 2 stands still with one point of health, and player 1 throws a
super combo as soon as the fight starts: a super finish, with its backdrop. Each scenario holds the
backdrop to one kind, so you see each picture without waiting for the game to choose it. Player 1 wins
both rounds this way (two super finishes), then the win screen.

| Scenario | Backdrop |
|---|---|
| `super_kind_1` | slanted lines |
| `super_kind_2` | slanted lines, the other way |
| `super_kind_3` | vertical lines |
| `super_kind_4` | vertical lines, the second set |
| `super_kind_5` | speed lines |
| `super_kind_6` | splash |
| `super_kind_7` | burst |
| `super_kind_8` | swirl |

`--character` works here too: the tool knows each character's super motion (a table in
`watch_scenario.py`, `SUPERS`) and throws it for whoever you pick. For JUNI and ZANGIEF none is known
yet, so their round ends with a normal hit and no backdrop.

```
python tools/watch_scenario.py super_kind_6 --character "dee jay" --screen native
```

`tests/scenarios/super_finish` is the parity test the super finishes come from: Ryu, the game's own
backdrop.

