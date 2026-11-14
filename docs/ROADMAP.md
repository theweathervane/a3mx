# Roadmap

**Where things stand:** the boot, title, MODE MENU, task scheduler, loader thread, the fight's
loop, set-up, most of its screens and its per-frame steps, the end of every frame, the sprite and object
drawing, the character select screen's cursors, the Survival / Free Battle select (all that the scenarios reach),
OPTION MODE's task, GAME OPTION, BUTTON CONFIG and the pause menu's BUTTON CONFIG, the stage's background tile drawers,
the step dispatchers the tests run, the text queue, objects' queueing, drawing dispatch and animation, the camera's sideways steps and the layers that follow it, the fighters' trail and the small steps and
questions about a fighter's state, its buttons, the network state, the 2D library's texture lists and
palettes, and the save files' names and parameters are
native C. Everything else still runs as translated code. The extracted assets are laid out by character, stage and screen ([assets/LAYOUT.md](../assets/LAYOUT.md)). How much is done is in the box below.

## Progress

How much of the PSP program is decompiled into hand-written C (`python tools/progress.py --by-region`).
Updated by `python tools/progress.py --update-docs` with every change that decompiles functions. The
overall bar is the share of the program's instructions that is decompiled (a long function counts for
more than a short one), 2.5% a block; on the other bars a block is a full tenth. Every decompiled
function is verified by the tests before it is merged.

<!-- progress:start -->
```
Overall  [███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]  8.31% decompiled

Functions                                [░░░░░░░░░░]   8.82%  1,313 / 14,881 functions
Instructions                             [░░░░░░░░░░]   8.31%  40,237 / 484,213 instructions

By area (instructions):
  game (modes, menus, fighters, battle)  [█░░░░░░░░░]  18.30%  37,315 / 203,929
  wireless (ad-hoc) VS                   [░░░░░░░░░░]   2.36%  88 / 3,721
  game (to be mapped)                    [░░░░░░░░░░]   0.67%  1,734 / 257,012
  system: save data, graphics, music     [░░░░░░░░░░]   0.16%  4 / 2,574
  system: C library, sound, OS glue      [░░░░░░░░░░]   6.46%  1,096 / 16,977
```
<!-- progress:end -->

A box is ticked when the work is merged and verified (run by a parity scenario or covered by a
difftest case). The version that did it is in brackets.

## Phase 1: Bootstrap (done)

- [x] Disc extraction and verification for US and JP (`tools/extract_assets.py`) (0.1.0)
- [x] Build from translated code plus hand-written functions (`SFA_FUNC`), CMake / Ninja (0.1.0)
- [x] The static recomp's runtime as `src/platform/`, its extensions as `src/extensions/` (0.1.0)
- [x] Parity tests: frame-by-frame memory and picture hashes against the originals (`ctest`) (0.1.0)
- [x] First decompiled functions: memcpy, memset, strcmp, strcpy, strlen (0.1.0)
- [x] Progress report (`tools/progress.py`) (0.1.0)
- [x] Differential tests (`--difftest`): decompiled functions against originals on random inputs (0.2.0)
- [x] Coverage: which decompiled functions each scenario runs (`--decomp-report`, `progress.py --list`) (0.2.0)
- [x] `tools/show_func.py`; `--stick` for scripted analogue input; the `stick_menus` scenario (0.2.0)

## Phase 2: The system layer

Decompile what sits between the game and the PSP's OS, and give `src/platform/` typed interfaces in
place of PSP OS calls.

Done:
- [x] C library: string and memory routines, number formatting helpers (0.2.0)
- [x] C library: the heap (Newlib's dlmalloc: malloc, free, memalign, trim, sbrk, lock) (0.2.0)
- [x] Controller read (0x088051BC) (0.2.0)
- [x] File loading (open / read / close, paths, the request ring) and the synchronous `.pac` loader (0.3.0)
- [x] The loader thread: loads characters, stages, screens and sound in the background (0.4.0)
- [x] Loading from `assets/`: archives built from their unpacked folders ([ASSET_LOADING.md](ASSET_LOADING.md)) (0.5.0)
- [x] Task scheduler and frame wait (0x088054F0): main thread, scheduler pass, task start / wait / exit / kill / restart (0.8.0)
- [x] Boot screens, screen fades, the title task and its attract mode (0.8.0)
- [x] The PC renderer's cost: frame-time logging, large textures checked by the bands a draw reads,
      auto resolution at most 4x (0.35.0, #51; platform only, the game unchanged)

To do:
- [ ] printf family (0x089F370C, 0x089F40E0, 0x089F5820)
- [ ] Soft-float and 64-bit helpers (0x089FAEC0 onwards) and maths
- [ ] Per-frame input records (0x0880509C). Hooked by `hooks.c`, so it moves together with phase 4.
- [ ] Sony's graphics library (sceGu, 0x089E6BB8 onwards) and the display / vblank glue
- [ ] Graphics submission (display lists, 0x089E4110 onwards)
- [ ] Sound: SAS voice management
- [ ] Sound: ATRAC3+ music streaming
- [ ] Save data (0x089E347C onwards)

## Phase 3: The game

Done:
- [x] The MODE MENU task (0x0888C268), its screens and cursor (0.9.0)
- [x] Back from a mode to the MODE MENU, with its auto-save (0x0888D2B0) (0.10.0)
- [x] The MODE MENU dispatch (0x0888CD94): every mode's start (0.11.0)
- [x] The fight loop (task 0) and battle task (task 2): frame skip, fighters' order, CPU routines (0.13.0)
- [x] The fight's set-up (0.14.0)
- [x] Fight screens 0, 1, 2, 8, 9, 10 (0.15.0) and 3, 4, 6, 11: continue, GAME OVER, win screen (0.16.0)
- [x] A round's start, fight screen 7 (0.17.0)
- [x] The fighters' pads, the round's per-fighter reset and step, the CPU label (0.18.0)
- [x] The three object lists and the end of a screen's frame (0.19.0)
- [x] The end of every frame: text pass, drawing objects and queued sprites by layer, display timers (0.20.0)
- [x] The drawing below the text pass: queued sprites and their anchors, every sprite queue, the fight's
      overlay, object kinds 0, 1 and 2 (0.21.0)
- [x] The rest of the fight's frame: the round's kind, screen 8 (the round won), screen 10's offline
      frame, the per-frame effects, the network's per-frame steps (0.22.0)
- [x] Fight screen 5 (a mode's end), the save error question (the MODE MENU's last function), the
      pause check with `hooks.c`'s pause on top of it; scenario `arcade_ending` (0.23.0)
- [x] The step dispatchers the scenarios run (268 of the program's 1,160: `src/core/object/steps.c`),
      the text queue (0x0880A6CC), the draw depth table (0x0880A3C4), a fighter's held directions and
      buttons (`src/core/fighter/buttons.c`), the network state's questions (`src/core/network/state.c`)
      and 40 empty functions (0.61.0)
- [x] 38 more dispatchers, among them the whole-frame kind (the step, then the object's animation and
      drawing); the camera's sideways steps (0x0881E49C, 0x0881E4E4, 0x0881EB78), an object's animation
      start (0x0882196C), the fighters' trail (0x0882D95C), falling objects (`src/core/object/drift.c`)
      and thirteen fighter steps (`src/core/fighter/state.c`); scenario `arcade_other_fighter` (0.62.0)
- [x] Seventeen steps objects' routines share (`src/core/object/motion.c`: speed and acceleration,
      targets, the floor, timers, the owner's place, the stage's colour) and five helpers
      (`src/core/misc.c`) (0.63.0)
- [x] An object queued for drawing and drawn (0x088216FC, 0x08821780, 0x08821828, 0x088215C4), its
      animation's step (0x088219B0), a fourth object list and two callback tables, the background
      layers following the camera (0x0881E058, 0x0881E148), making an object (0x08822FE4), the fade's
      dimming (0x0880E388), the picture cache's lookup (0x0880EB84), the trail's frame, the clock's
      digits and four more fighter questions (0.64.0)
- [x] A fighter's input edges (0x08859650), its script runner (0x08877564) and move routine
      (0x0886EDA8), pushing the opponent (0x08862F28), the wall jump check (0x08864140), twelve hit
      adjustments (`src/core/fighter/hit.c`), fifteen more object steps, object lists 5 and 6, the
      camera's routines by pair, conditional runners (`src/core/object/runners.c`) (0.65.0)
- [x] The object pools (four of 0xC0-byte objects: make and free), the clears before a screen or a
      fight (`src/core/reset.c`), the routines by layout and by stage (camera span and height, trails,
      the stage's own), and thirty-nine steps of the screens' objects
      (`src/core/object/screen_steps.c`) (0.66.0)
- [x] The fight intro's objects (`src/core/object/intro_steps.c`: their routines, moves and waits), the
      calls into a character's own routine table (fighter + 416: + 64, + 76, + 80, + 88 / + 92), three
      more script commands, the walking speed (0x08861DE4), the per-frame loops over the four fighters,
      and twenty more screen-object steps (0.67.0)
- [x] The random numbers' seed (0x08820B7C, a Mersenne Twister), the screens' director record
      (`src/core/object/director.c`), damage by ISM (0x08872C50) and by the hit (0x088732E0), the fall
      side (0x088721B8), a fighter's ISM data (0x08862D88), a script's block skip (0x08879D1C), the
      third list's make, the layers' clear, and twenty more object steps (0.68.0)
- [x] The CPU's scripts: one picked from the character's lists (by level and at random), restarted, its
      wait set and its words unpacked; the network record cleared and its small steps; three more of the
      director's routines; the versus screen's objects (run for a marked side, arriving and reporting, lit
      and nudged, rows) and thirteen more object steps (0.69.0)
- [x] The five object pools emptied (0x0882222C - 0x08822A18), the queues beside the ring reset, the
      CPU's wait and block leave, two fighter questions, and thirty-five more objects of the screens and
      the intro (following a record, sliding thirteen frames, scrolling round, rows made one by one,
      animations picked at random) (0.70.0)
- [x] The CPU's script branches (at random, by the opponent, by layout, by a count; a script called at
      random and started), a fighter kept on the screen (0x08862DF8) and its speeds from a record
      (0x08863168), the director clearing pools, and twenty more screen and intro objects (0.71.0)
- [x] The CPU's script step (0x088774A8) and a fighter's move state cleared (0x088647C4), the button
      just pressed as strength and kind, a fighter's table of values by state, objects drawn only when on
      the screen (0x08821424), the records reset, and twenty more director, screen and intro objects (0.72.0)
- [x] The CPU's first frame (a script picked, the first command run), a human fighter's button kept,
      two more script branches, the records' start-up, a thrown object swinging as it falls, a record
      following a list, and twelve more screen and intro objects (0.73.0)
- [x] The 2D library's texture lists and palette upload (0x089E0CF4, 0x089E0EA4, 0x089E1254), the zeroed
      allocation (0x089E04EC), three small record tables, the save files' names and dialogue settings,
      the network session's checks and full reset (0x088DBF88), a fighter's attack starting routine 1 / 5,
      and twenty-eight more screen objects (0.74.0)

To do, in rough order:
- [ ] The other 982 step dispatchers (891 plain, 70 whole-frame, 21 with an owner), as scenarios reach them (each is one line in `steps.c`; the
      modes no scenario plays yet hold most of them)
- [ ] Object kinds 3 (0x0880F820, a many-polygon shape) and 4 (0x08810128, a shaded rectangle). Need a
      scenario that draws one.
- [ ] Screen 10's network frame (0x0882485C). Needs a network battle scenario (phase 12).
- [ ] The character select screen (`src/core/select/`):
  - [x] Its frame, start and per-frame step (selectscreen.c's frame hook on top), the cursors' two-level
        dispatch, the hand-over to the partner and finishing a choice (0.25.0)
  - [x] The cursor's states: the character (grid, random boxes, entries, SELECT's extras), the play
        style, the ISM and the speed, and the choice counted (`cursor.c`, 22 functions; scenario
        `select_choices`) (0.27.0)
  - [x] The pads it reads and nine kinds of display task entry, with selectscreen.c's Dramatic Battle
        pads and own-side fix wrapped around them (`cursor.c`, `display.c`) (0.28.0)
  - [ ] The five display task entries no scenario reaches yet (0x088A78E0, 0x088A7CB4, 0x088A8038,
        0x088A81FC, 0x088A8404; still hooked by selectscreen.c): with the Survival / Free Battle select
- [ ] The Survival / Free Battle select screen (`src/core/select/battle_select.c`):
  - [x] Its step dispatch and frame: the layouts, whose cursor runs, the title, SPEED: (selectscreen.c's
        WAIT / JOIN wrapped around it), the difficulty, the entries' names, moving on once everyone
        expected has chosen (scenario `free_battle`) (0.29.0)
  - [x] Its start and end steps, the fighters' set-up, the display tasks' creation, and the cursors'
        first three states: the character, the play style and the ISM (0.30.0)
  - [x] The other cursor states: the difficulty, the speed, the stage and the end, with states 4 / 5
        (scenario `free_battle_rows`) (0.33.0)
  - [x] The boxes (0x088A60F8, 0x088A2578, 0x088A4D30, 0x088A2480), whose cursor runs in Free Battle
        (0x088A5600), the grid (0x088A6A10) and 0x088A5174, its hook now a wrapped extension; scenarios
        for TEAM, DRAMATIC and R.DRAMATIC, which also reach four more display task entries (0.38.0).
        Left: 0x088A22AC (modes 13 / 19, not reached)
  - [x] Its display tasks (0x088A6FF8 on, `battle_display.c`): the cursor frames, portraits and team
        list (0.39.0); the Dramatic kinds' portraits, the panels, the banner (0.40.0). Left, as nothing
        reaches them: 0x088A8488 - 0x088A8E64 and the hooked entry 0x088A8404
- [x] The stage select (Free Battle): the stage row of the Survival / Free Battle select, its cursor
      state 8 and the random stage (0.33.0)
- [x] The OPTION MODE screens (0x088B5F48 - 0x088BB470; everything a scenario reaches, the extensions wrapped
      around it) (0.42.0)
  - [x] What the scenarios reach (`option_mode.c`): the task, the submenus' frame, open / close and
        confirm, the values by pad, the options' flags and their copy into the game, the menu's and
        the submenus' pictures (0.41.0)
  - [x] Scenarios `option_tour` (every submenu with the extensions), `option_tour_original` (the
        game's own screens, `--no-extensions`) and `pause_config` (the pause menu's BUTTON CONFIG); GAME
        OPTION's rows, DEFAULT and text, BUTTON CONFIG's input, rows, DEFAULT and text, the pause BUTTON
        CONFIG, OPTION MODE's way out, more menu pictures (0.42.0)
  - [x] The 18 hooked functions decompiled, their hooks moved onto them as wrapped extensions
        (exsetting.c, screensetting.c, widescreen.c); scenarios `pause_config_original` and
        `pause_config_players` (0.42.0)
  - [ ] Left translated, as nothing reaches them: the rest of 0x088B5F48 - 0x088BB470 (the memory
        stick's other paths, the pause BUTTON CONFIG's up / down row 0x088B932C)
- [ ] The stage's background drawers (`src/core/stage/`):
  - [x] The tile drawers 0x0881AB28 (32-pixel tiles) and 0x0881B268 (16-pixel), with WIDE FORCED's
        wider grid as native code; scenario `wide_fight` (0.49.0)
  - [ ] 0x0881B7C8 and its caller 0x0881A2C8 (from 0x089DE2C0 / 0x089DE390): need a scenario that
        reaches them
- [ ] Battle core: rounds, timer, camera, collision (hit boxes), damage, guard, ISM rules
- [ ] Fighters: the shared move engine
- [ ] Fighters: each character's moves and AI
- [ ] Modes, one box each as they become fully native:
  - [ ] Arcade
  - [ ] World Tour
  - [ ] Dramatic / R.Dramatic
  - [ ] Variable
  - [ ] Survival / Drama Survival
  - [ ] 100 Kumite
  - [ ] Free Battle
  - [ ] Training
  - [ ] Final Battle
  - [ ] Edit
  - [ ] The wireless modes
- [ ] Parity scenarios to cover them: one per mode, one per character group

## Phase 4: Recomp extensions as native code

Every extension from the static recomp, moved from hooks into the decompiled code it touches, and
checked with the same scripted runs before and after. Each one can start once the code it touches is
decompiled.

- [ ] Up to three local players, join-in (EX. SETTING JOIN-IN, Start on select screens, JOIN / WAIT).
      Now: hooks.c, selectscreen.c. Needs: input records and select screens.
- [ ] Network modes on one PC (virtual players, COUCH CO-OP). Now: vpeer.c, netplay.c, hle_net.c.
      Needs: the wireless block.
- [ ] L2 / R2 buttons, PLAYER CONFIG for four pads, pause BUTTON CONFIG rows. Now: playercfg.c,
      exsetting.c. Needs: fighter input.
  - [x] The option screens' part: exsetting.c wrapped around the decompiled OPTION MODE and pause
        BUTTON CONFIG (0.42.0)
- [x] EX. SETTING screen, OPTION MODE sixth item: exsetting.c (and tiles.c), wrapped around the
      decompiled OPTION MODE (0.42.0)
- [ ] Silent saves, AUTO SAVE always on, everything unlocked. Now: sysdata.c, savestore.c, settings.c.
      Needs: the save task.
  - [x] Silent when a save fails too: no "continue without auto-saving?" (0.24.0; the list of save
        messages is in [SAVES.md](SAVES.md#save-messages-kept-off-screen))
- [ ] WIDE FORCED as a real native setting (Rickow's 16:9 patch is currently used today). Now: widescreen.c + translation
      overlay. Needs: the camera and background drawers completed before can be dropped.
- [x] Pad icons on the MODE MENU: padicons.c, wrapped around the decompiled MODE MENU (0.24.0).
- [x] F2 back to the MODE MENU: quitkey.c, wrapped around the decompiled frame wait (0.24.0).
- [x] Boot screen save messages: bootmsg.c, wrapped around the decompiled load screen (0.24.0).
- [x] VSYNC row in SCREEN SETTING: screensetting.c (0.35.2), wrapped around the decompiled 0x088B85BC
      (logic), 0x088B78DC (drawing) and 0x088BAA90 (its bar) (0.42.0). Left: the disabled rows' own
      grey, which needs the text pass 0x08812520.
- [x] Every extension can be switched off: `--no-extensions`, or a build with `-DA3MX_EXTENSIONS=OFF`
      (0.26.0, [BUILDING.md](BUILDING.md#without-the-extensions)). Keep it so as extensions move into
      native code: each one checks `rt_extensions`, and the `no_extensions` scenario must keep passing.

Platform features that stay in `src/platform/` (nothing to move): PSP save IMPORT / EXPORT
(savestore.c, psp_savecrypt.c); VSYNC itself (`host_set_vsync`; its menu row is the extension above), the
dual-GPU hint, the up-to-8× renderer.

## Phase 5: Assets

Done:
- [x] Unpack and pack `.pac` archives exactly; textures and palettes to PNG, text to TXT (0.3.0)
- [x] Load from `assets/` instead of the disc, with a manifest so assets can be replaced or added (0.5.0)
- [x] Load edited PNGs: 16-bit textures, indexed textures (as grey indices) and palettes (0.5.0)
- [x] Pair indexed textures with their palettes by watching the game draw them (`tools/pair_palettes.py`) (0.6.0)
- [x] Lay `assets/` out by what things are: `characters/<nn>_<name>/`, `stages/`, `screens/`, `sound/`
      ..., a subfolder per entry kind, `index.json` for the game, new art in `extensions/`
      ([assets/LAYOUT.md](../assets/LAYOUT.md), #53) (0.37.0)

To do:
- [ ] Pair the 44 ending textures still without a palette. Forcing the fight number to 9 (as
      `arcade_ending` does) reaches any character's ending quickly, for `--palette-census` runs
- [ ] Music: ATRAC3+ (`bgm/*.at3`) to WAV / OGG
- [ ] Sound banks (PPHD / VAG) to WAV
- [ ] Decode the data kinds: layouts, sprite sets, fighter data
- [ ] #53: each character's sprites and animation frames as PNG with a `metadata.json` (frames,
      offsets, timings), in `characters/<nn>_<name>/fighter/sprites/`; then higher-resolution
      replacements
- [ ] Identify the variant slots 0x12 / 0x13 / 0x14 / 0x25 and the archives still under `other/`, and
      name them in `tools/asset_layout.py`
- [ ] Name the music tracks
- [ ] Fixed buffer sizes for text, palettes and data tables, so bigger replacement assets fit
- [ ] New, editable assets for the screens the extensions draw themselves: EX. SETTING and PLAYER
      CONFIG (OPTION MODE tiles and letters lifted at run time: tiles.c, exsetting.c), JOIN / WAIT
      (own pixel font: selectscreen.c), the MODE MENU pad icons (padicons.c)
- [ ] Repack tool (assets back to the game's formats), only if something still needs it

## Phase 6: Widescreen revision

Details in [WIDESCREEN.md](WIDESCREEN.md), [STAGES.md](STAGES.md) and
[STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md). Rule for every item: WIDE FORCED must never change move
timings or balance; parity with it on matches it off, frame for frame, in every fighter's state.

- [ ] Draw body parts past the invisible walls (Blanka's shoulder vanishing at the wall), keeping the walls
- [ ] Stop pop-in and flicker of assets and characters at the screen edges
- [ ] Hide off-screen loading in the pre-battle sequences
- [ ] Fill the MODE MENU background's missing left side
- [ ] Stage art at the camera limits: **stage extensions** ([STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md)).
      Decided: extend the art, keep the limits (pulling them in moves the walls and changes play).
  - [x] Size and placement measured for all 38 stages (`tools/measure_stage_extensions.py`); 76
        magenta placeholders (48 × 288, stage number and side in cyan) committed in
        `assets/extensions/stages/` (moved there, beside the stages' own folders, in 0.37.0)
  - [x] Draw them, WIDE FORCED only, each behind its own layer: `stagestrips.c` around the decompiled
        tile drawers 0x0881AB28 / 0x0881B268, placed from each layer's scroll, so the camera didn't need
        decompiling first (0.50.0)
  - [ ] A WIDE FORCED scenario with screenshots of the strips in place, plus the fighters'-state parity
        check below
  - [ ] Paint the 76 strips (any time, in parallel: a good job for a collaborator; magenta = still to do)
  - [x] One strip per layer, joined to each layer's own scroll, with placeholders measured per layer
        (`tools/measure_stage_layers.py`) (0.50.0)
- [ ] A parity scenario with WIDE FORCED on, compared against it off. (`wide_fight` (0.49.0) runs WIDE
      FORCED on in both runs, the reference being the patched translation; the on-against-off check
      of the fighters' state is still to do)

## Phase 7: Support for Japanese and EU versions
- [ ] Map decompiled US functions onto the JP program
- [ ] Build JP from the same `src/core/` (region address table, JP strings and assets)
- [ ] JP parity scenarios
- [ ] Map decompiled US functions onto the EU program
- [ ] Build EU from the same `src/core/` (region address table, EU strings and assets)
- [ ] EU parity scenarios

## Phase 8: no translated code left

- [ ] `build/generated/` is empty, or only holds code that never runs
- [ ] All of phase 4 ticked
- [ ] Remove the interpreter / compiler from releases: the release is just the exe and the PSP program
      is only needed for its assets

## Phase 9: Setting refinement

EX. SETTING's 1P / 2P / 3P JOIN-IN rows say HUMAN or CPU, but that is not what decides whether a pad
can join: Start joins on the select screens that allow it whatever JOIN-IN says, and JOIN-IN HUMAN
instead hands a pad the fighter the computer would have played. The rows should say what they do.
Can start once phase 4's join-in item is native code; the rule itself is in [PLAYERS.md](PLAYERS.md).

- [ ] Document the runtime rule completely in `PLAYERS.md`, from the native code:
  - [ ] What JOIN-IN HUMAN / CPU changes, mode by mode, and what joining with Start does on its own
  - [ ] Which modes let a second or third pad join with Start (Arcade's new challenger, the Dramatic,
        Variable and Free Battle select screens) and which never do (World Tour, Training, R.Dramatic
        Battle from the MODE MENU), and why
  - [ ] Plugging a pad in or pulling it out mid-mode: what happens to the fighter it was playing
  - [x] Bring the page's file paths up to date (it still names `src/runtime/`)
- [ ] Audit the names and labels: the JOIN-IN rows and their HUMAN / CPU values, the JOIN / WAIT
      boxes, the CPU2 / CPU3 names, COUCH CO-OP
- [ ] Propose new names or a new layout for EX. SETTING (for example, one row per pad: "takes the
      CPU's fighter: YES / NO"), agree it, and implement it, with the saved setting carried over
- [ ] A parity scenario per changed row: with every row at its default, every mode still plays as on a PSP

## Phase 11: Couch co-op architecture review

COUCH CO-OP plays the game's network modes on one PC: the other PSPs are virtual players in the same
process (`vpeer.c`, `netplay.c`, `hle_net.c`), and pads 2 and 3 drive network players 2 and 3. Local
play therefore depends on emulating the wireless layer. Needs: phase 4's network item.

- [ ] Audit how COUCH CO-OP hooks players 1–3's input into the network modes: the virtual players,
      the emulated ad-hoc calls, which modes need how many players, the host-only CONTINUE menu
- [ ] List what only exists in the network menu (VS / TEAM / VARIABLE BATTLE, DRAMATIC BATTLE,
      LEAGUE, TOURNAMENT) and what an offline mode already offers the same way
- [ ] Decide, mode by mode, how local play should reach it without the network layer: an existing
      offline mode with join-in, the network mode's rules run locally, or kept as it is (team battle
      and the other network-only modes stay playable either way)
- [ ] Implement that, and remove or simplify the code paths that tie local co-op to the network input
      layer, keeping what phase 12 still needs
- [ ] Scenarios for each network-only mode played locally

## Phase 12: Network modes and rollback

The PSP's wireless modes call ad-hoc networking that the project stubs or emulates. Replace that with
real online play using rollback, so a match plays as if both players were on one machine. Needs: phase
11's review, and the battle core and fighters native (phase 3) so a frame can be saved, loaded and
re-run exactly.

- [ ] Review the stubbed network handlers (`hle_net.c`, `vpeer.c`, `netplay.c`): what the game sends,
      when, and which of its own sync and timeout logic can be bypassed
- [ ] Save and restore the full game state for one frame, fast enough to re-run several frames inside
      one (the RAM snapshot behind `--snap` is the starting point, cut down to what a fight changes)
- [ ] Prove determinism: the same inputs give the same per-frame hashes on Windows, Linux and macOS
- [ ] Rollback: input delay, prediction, re-simulation, and keeping sound and effects right through a
      rollback (an existing library such as GGPO or GGRS, or our own; decide after the state work)
- [ ] Connecting: host / join, lobby, connection and desync states, disconnects; a separate input
      path from couch co-op's
- [ ] Adapt the network modes' screens: the PSP's ad-hoc menus to lobby, waiting and connection screens
- [ ] Network tests: two instances on one machine with simulated latency and packet loss, compared
      frame for frame with an offline run of the same inputs

## Phase 13: Parity re-review for optimisation

Once the code is all native, look again at what can be made faster or simpler without changing a frame.
This is about the game's own code. The PC side (`src/platform/`: rendering, sound, input) can be made
faster at any time, as long as the game's state stays the same frame for frame (0.35.0 did this for
the GPU renderer).

- [ ] Profile a fight, the menus and the attract mode, and list the hot spots
- [ ] Optimise each hot spot, with the full parity suite passing before and after
- [ ] Remove workarounds kept only to match the translated code's register behaviour (mirrored
      argument registers, `call_keep` and friends) where nothing reads them any more

## Phase 14: New content / Pipedreams?

Some functionality was carried over from the recomp but never fully implemented.

- [ ] Player 4: the original game handles three sets of inputs; find out whether it can handle more
- [ ] X.DRAMATIC BATTLE: 2v2, like Street Fighter EX3, if Dramatic Battle can be mirrored
- [ ] Other versions' rules ([VERSION_DIFFERENCES.md](VERSION_DIFFERENCES.md))
- [ ] Other systems
