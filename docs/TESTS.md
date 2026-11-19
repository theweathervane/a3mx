# Tests

What is tested, why, and how long it takes. Run them with `ctest` from a build folder
(`ctest --test-dir build-headless -j2`), or `build.cmd --test` / `./build.sh --test`, or the launcher's
**Run the tests after building**.

To run them in a container instead (a Docker host or Portainer, with your ISO mounted read-only):
[docker/tests/README.md](../docker/tests/README.md).

Two builds run tests:

| Build | What it is | Needs your disc | Tests |
|---|---|---|---|
| `ci` preset (`build-ci/`) | every hand-written file compiled against a stand-in for the generated code; this is what the GitLab pipeline builds | no | `pacing_rules`, `png_cases`, `png_decoder` |
| `headless` or `release` preset | the real game | yes (`work/`) | everything below |

The times are from a 2-core cloud machine with `--fast` (the game not waiting for the clock); a
desktop PC is three to five times quicker. Run with `-j2` or more: the tests are independent.

## Why parity

The decompilation replaces the translated program one function at a time. A function is only merged
when a test proves it behaves exactly as the original: that is what **parity** means here. Each parity
scenario plays the game twice from the same scripted input, once with the decompiled functions and
once with `--reference` (the translated original of each), and compares a hash of the program's
memory and of the picture **on every frame**. The first frame that differs fails the test and names
the frame. Extensions run in both, so they are compared too.

A scenario also records which decompiled functions it ran (`--decomp-report`), which is how
`tools/progress.py` knows a function is verified.

A scenario with a `wide` line is a **WIDE FORCED check** instead: it plays twice with the decompiled
code, SCREEN SIZE as the game sets it and WIDE FORCED, and the four fighters (0x092E3EA0 - 0x092E5100,
the fourth hash in `--hash-log`) must be the same in every frame. WIDE FORCED only shows more of the
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
| `stage_strips` | with WIDE FORCED and Stage Extensions on, the stage's edge is not black | the strips once silently stopped drawing after a merge | 4 min |

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
| `super_finish` | a KO by Ryu's Shinku Hadoken | the super finish's backdrop (0x0880C208) | 4 min |
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
| `wide_fight` | Arcade's first fight, WIDE FORCED off and on (`wide`) | widescreen changes nothing about the fight | 7 min |
| `wide_walls` | Free Battle, WIDE FORCED off and on (`wide`), Blanka against Blanka, player 1 at the left wall then walking right | a fighter at the wall plays the same while drawn into the left margin (the cell drawers' left limit; the drawing by eye) | 7 min |
| `wide_left_wall` | Free Battle on Ryu's stage, WIDE FORCED off and on (`wide`), the camera at its left limit | the fight is the same with the tile drawers' extra left columns wrapping round within their row (layer 0 starts at map column 1 there; the drawing by eye) | 6 min |
| `network_match` | NETWORK > VS MODE against the virtual peer: the menus, IP OPTION, the select and a whole match | the network screens' rows, panels and message boxes | 8 min |
| `disc_faults` | boot with a disc whose reads fail or stall | the loader's retry paths | 4 min |
| `no_extensions` | the game with every feature off | the PSP program alone still runs, and matches | 2 min |

The whole suite is about 2½ hours at `-j2` on that 2-core machine. Before a change to `src/core` or
`src/extensions` is called ready, run the shortest scenario that reaches the code first
(`ctest -R "boot_to_select|arcade_fight" -j2`), then the lot. Don't rebuild a folder while its tests
run: configure a second one (`cmake --preset headless -B build-h2`).

## What no test covers

- **The GPU renderer.** ctest runs the software renderer. For a change to `ge_gl.c`, compare frames by
  hand: `python tools/compare_gl_frames.py` with an `A3MX_EGL` build.
- **FILTER's shaders** (`shader_gl.c`): they need a GPU, librashader and the shader files.
- **The launcher.** It takes its own screenshots (`--screenshot`), and `python tools/screenshots.py
  --check` fails when one is out of date, but nothing presses its buttons.
- **Real monitors and controllers.** `pacing_rules` simulates monitors; controller mapping is checked by
  hand (the launcher's **Test**).
- **Features switched off one at a time.** Only "all on" and "all off" have scenarios.
