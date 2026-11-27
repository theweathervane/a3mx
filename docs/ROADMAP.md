# Roadmap

**Where things stand:** the boot, title, MODE MENU, task scheduler, loader thread, the fight's
loop, set-up, most of its screens and its per-frame steps, the end of every frame, the sprite and object
drawing, the fight's HUD drawing, the character select screen's cursors, the Survival / Free Battle select (all that the scenarios reach),
OPTION MODE's task, GAME OPTION, BUTTON CONFIG and the pause menu's BUTTON CONFIG, the stage's background tile drawers and its objects,
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
Overall  [████████████████░░░░░░░░░░░░░░░░░░░░░░░░]  41.87% decompiled

Functions                                [███░░░░░░░]  38.47%  5,724 / 14,881 functions
Instructions                             [████░░░░░░]  41.87%  202,734 / 484,213 instructions

By area (instructions):
  game (modes, menus, fighters, battle)  [█████░░░░░]  57.53%  117,320 / 203,929
  wireless (ad-hoc) VS                   [██████░░░░]  68.05%  2,532 / 3,721
  game (to be mapped)                    [███░░░░░░░]  31.82%  81,782 / 257,012
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
- [x] The GPU renderer's draws batched (~750 a frame in a fight sent as ~100), and a ctest for it against
      the software renderer in both screen sizes (`gl_renderer_*`, `-DA3MX_EGL=ON` builds) (0.101.0)

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
- [x] A CPU fighter's script restarted (0x0887A1D8), a value scaled by a setting (0x08873094), the CPU's
      chance for characters 6 / 15 / 20, jittered copies placed by the character, the queues' records
      (0x08809EF0 / 0x08809F48), the director by + 93, and fifteen more screen objects (0.75.0)
- [x] The fight frame's end (0x0882453C), damage by both fighters' values (0x08872F68), the gauge at
      + 700 (0x08872080), the highest count at + 357, each fighter noting the opponent's side, the small
      objects' and queues' resets, a drawn-with-margin check (0x088214E8), and ten more screen and intro
      objects (0.76.0)
- [x] A hit's damage (0x08872B54), a fighter's speeds by direction (0x08862068), the character's values
      (0x088597F0), a CPU script picked by the opponent (0x0887B308), rows laid out by a character's
      width, and five more screen objects (0.77.0)
- [x] The random numbers' twist (0x08820A68), a pick at random from 41 (0x0883021C), the camera layer
      following the height (0x0881E538), a CPU fighter's chance check (0x0887B6B8), and six more screen
      objects (0.78.0)
- [x] The score added (0x088211CC), the held-direction history (0x08858D54), a CPU state from its script
      word (0x0887B528), offsets unpacked (0x0886E638), the 0x168-byte pools and the fight's tables
      cleared, and three more screen objects (0.79.0)
- [x] The frames both fighters are held on a hit (0x0887223C), a button attack (0x0886BE50), a script's
      routine 1.5 (0x08878798), score by a record (0x0885847C), the characters met (0x088644F4), and four
      more screen and director objects (0.80.0)
- [x] The sides down (0x088262C0), the fighters' starting places (0x08864BC8), the round's pick
      (0x0882FC2C), team score (0x08821080), the move slots' search (0x08867A60), the gauges' markers, and
      four more screen objects (0.81.0)
- [x] A sprite part's place, scaled and turned (0x0880D214), the camera following the span (0x0881E320),
      damage by the hits in a row (0x08873160), a hit's pad effects (0x08874CDC), a count's reward
      (0x088585B0), the growing value's limit (0x08836AA8), and fourteen objects by layout (0.82.0)
- [x] Forty small steps from the wider profile (menus, endings, Free Battle, World Tour): `object/small_steps.c`,
      GAME's and World Tour's records cleared, a fighter's edges, script steps (0.84.0)
- [x] Forty-five more small steps: the director's step tables, decimal digits, pad checks, a free slot of
      eight, counters and speeds (0.87.0)
- [x] Forty-five more: screen objects' moves, count-downs and fades, step tables by GAME's state, text from
      lists, an opponent's score by a count (0.88.0)
- [x] Forty-one more (`object/batch_steps.c`): steps through tables (with the bottom text, or freeing the
      object at GAME + 4 = 0), the CPU's script frame and a fighter's routine by human / CPU, places from
      tables, bounces and wrap-arounds, the frame record of a fighter, counters (0.103.0)
- [x] Twenty larger ones (`object/batch_large.c`): an object drawn with its parts (0x0880BA94, the hottest
      left), the fading shape (0x0880DA78), objects put on their lists and made from the twelve 0x168-byte
      slots, hit boxes meeting, the fighters pushed apart, a fighter's walk step and box range, set-ups by
      screen, a particle field, objects moving to a target and made from a record's list (0.104.0)
- [x] Eighteen more (`object/batch_large2.c`): the four fighters drawn, a guard's damage cut and the
      guard choice, a fighter's round reset, effects made beside a fighter, name entry, the fighters'
      labels, objects that move, turn and bounce at a target, set-ups by type and name boxes (0.105.0)
- [x] Forty more (`object/batch_mid.c`): the CPU's script commands by level, fighters' input records and
      press counters, the two side records cleared at a fight's start, objects' trails, timed lists, steps
      while a parent lives, counters and set-ups (0.106.0)
- [x] Thirty-four more (`object/batch_mid2.c`): the reset before a fight, a fighter's script start and
      routine, the VS screen's lists, objects placed by parent or screen, texts and shapes, a free colour
      taken for a character (0.107.0)
- [x] Twenty-five more (`object/batch_mid3.c`): the CPU's script commands by the opponent's state, press
      counters, the intro's pieces moving in, objects made beside others, the particle field's start (0.108.0)
- [x] Twenty-three more (`object/batch_mid4.c`): the stage and fighters picked at random, a fighter's
      first state, the throw range check, numbers as pictures, panels and the intro's owner (0.109.0)
- [x] Nineteen more (`object/batch_mid5.c`): the first object list's run, objects made from a
      description, a fighter's input for its team, three press counters, a file loaded and waited for (0.110.0)
- [x] Nineteen more (`object/batch_mid6.c`): the random numbers' tempering, the fighters' routine
      tables, the score for a hit, script commands that set the CPU's state, a frame record stepped on (0.111.0)
- [x] Sixteen more (`object/batch_mid7.c`): the list at 0x0950FFB0 run before the first, the CPU's first
      script at random, a fighter's pad byte for its team, particles and panels (0.112.0)
- [x] Twelve more (`object/batch_mid8.c`): a record's timed lines, a fighter's input recorded for its
      team, an effect following a fighter's frame, pieces' counters and set-ups (0.113.0)
- [x] Eight more (`object/batch_mid9.c`): a fighter's damage scaled by move and state, the reward count,
      the intro's owner by stage, option panels and camera lines (0.114.0)
- [x] Ten more (`object/batch_mid10.c`): a throw's hold, the continue checks, the special match for
      character 9, the demo screen's set-up, panels and a row of particles (0.115.0)
- [x] Seven more (`object/batch_mid11.c`): a fight's two fighters paired, the CPU's aggression nudge,
      a chain of four objects, the camera layers from their tables and panels by type (0.116.0)
- [x] Twenty-five more (`object/batch_mid12.c`): the CPU's throw or attack on its opponent, its first
      script, the fight reset, the camera lines' starts, particles and panels (0.121.0)
- [x] Ten larger (`object/batch_large3.c`): a hit's amount and spark, life by mode at a round's start,
      the select cursor's set-up, the entries into the grid and their names, a screen's objects (0.122.0)
- [x] Twelve more (`object/batch_large4.c`): the ending task, the fight resets, a throw allowed, getting up
      early, a hit's score, the score panel's digits, the entries' panel (0.123.0)
- [x] Ten more (`object/batch_large5.c`): a hit's damage, its effect step, a character's data unpacked,
      whether a button counts, the name panel, screens resetting the fight (0.124.0)
- [x] Ten more (`object/batch_large6.c`): six characters' set-ups, another's lists unpacked, a score into the
      rankings, the name panel's count (0.125.0)
- [x] Forty-four small ones (`object/batch_steps2.c`, 14 step dispatchers in `steps.c`), from the profile
      refreshed at 0.125.0: moves and count-downs, the network session's list and checksum (0.126.0)
- [x] Forty-four more (`object/batch_steps3.c`, 16 step dispatchers in `steps.c`): the rest of 9 - 16
      instructions: moves and count-downs, fighters' script starts and a button check, the network
      session's list, place and scan, this PSP's address and nickname (0.131.0)
- [x] Fifty-six more (`object/batch_steps4.c`, 2 step dispatchers in `steps.c`), 56 of the 70 ready of 17 - 24
      instructions: objects sliding, rising and swinging, the ending's next animation, fighters' script
      branches by ISM, the ad-hoc link's flags, connect and matching end, the session's members (0.132.0)
- [x] Thirty-five more (`object/batch_steps5.c`): the last 15 of 17 - 24 instructions and the first 20 of
      25 - 40: objects bouncing, rising, sliding and starting animations, fades for a side, script branches
      by health, the CPU's random choices, the session's members by address, its lock and error codes (0.133.0)
- [x] Twenty-five more of 25 - 40 instructions (`object/batch_steps6.c`): objects stepping while the screen
      allows, made at intro stages, framed until a record's count; the text records; a fighter's target
      and the side ahead on rounds; the ad-hoc link's close and parameters, the network's result (0.134.0)
- [x] Twenty-six more of 25 - 40 instructions (`object/batch_steps7.c`): objects stepping by a record's
      count, set from their owner, animated by a level; box settings; fighters' script counts and branches,
      a CPU's special opponents; the ad-hoc link's events, thread end and target, the session's agreement (0.135.0)
- [x] The last 23 of 25 - 40 instructions (`object/batch_steps8.c`): pool 7's objects run, objects placed by
      the camera and sliding to a stop, box settings, the pause values, a layout's fade levels; fighters'
      button strength, target pairs and trails; the ad-hoc link's thread end and members, the network texts (0.136.0)
- [x] Eighteen more (`object/batch_steps9.c`): the 12 of 9 - 40 made ready by the batches before and the
      first 6 of 41 - 60: fade levels, fighters' trails in turn, objects scrolling with the camera, ending
      objects moving the camera layers, the network menu's head row and message panel, the session's lists (0.137.0)
- [x] Thirty more (`object/batch_steps10.c`): the last 4 of 9 - 40 and 26 of 41 - 60: objects queued by
      layer and following a fighter's picture, slides and timers, effects at a fighter's side, targets by
      layout, the check that lets a fighter start a move, the session's lists under its lock (0.139.0)
- [x] Twenty-one more (`object/batch_steps11.c`): the last 3 of 9 - 40 and the other 18 of 41 - 60: objects
      following a fighter, fighters' tables and followers, CPU levels by layout, turning toward the nearest
      fighter, the frame's run by screen, the session's members, records, thread and message box (0.140.0)
- [x] Thirteen more (`object/batch_steps12.c`): the 5 of 9 - 60 made ready and the first 8 of 61 - 100: a
      fighter's reset when free or past its gauge, a CPU script's special by ISM, two boxes overlapping, the gauge's
      digits, the fight frame's objects, the network members advanced and the network thread's loop (0.141.0)
- [x] Eleven more (`object/batch_steps13.c`): the 3 of 9 - 60 made ready and the next 8 of 61 - 100: the
      gauge's digits object, followers stepping their owner's records, objects moving to a target, round markers,
      a row's shades, the network thread's starts and entry, a name from the MAC address (0.142.0)
- [x] Seven more (`object/batch_steps14.c`): the 1 of 9 - 60 made ready and 6 of 61 - 100: the network
      thread's outer entry, the heap's allocation, a screen's banner by kind, a picture by slot, CPU levels for a
      versus fight, a round's end by record, 17 objects by layout (0.143.0)
- [x] Eight more (`object/batch_steps15.c`): the 4 of 9 - 60 made ready and 4 of 61 - 100: pictures by
      type and slot, CPU levels for a fight, the network scan's results and a record added to a network list,
      characters' colours made to differ, a CPU's character swapped, a hold counter with its text (0.144.0)
- [x] Five more (`object/batch_steps16.c`): the last of 61 - 100: each fighter's pause check, the fighters'
      flag fading the fight, the edges of three fighters for the camera, a network menu item placed, four flying
      objects by row (0.145.0)
- [x] Four more (`object/batch_steps17.c`): the 1 of 9 - 100 made ready and the first 3 of 101 - 200: the
      camera edges of fighters 1 - 3, the hit checks between the two hit lists, the network matching started by
      mode, a banner's fade (0.146.0)
- [x] Four more (`object/batch_steps18.c`): the 2 of 9 - 100 made ready and the next 2 of 101 - 200: a row
      object's step by type, the network matching for a thread's record, the pause menu's texts, a text
      scroller (0.147.0)
- [x] Three more (`object/batch_steps19.c`): the 1 of 9 - 100 made ready and the next 2 of 101 - 200: a text
      scroller's start, the fade for flagged fighters by layout, the network menu's status line (0.148.0)
- [x] Three more (`object/batch_steps20.c`): the next of 101 - 200: a network menu row's step, a critter
      wandering a stage, the select grid's cells barred or shown with the entry rows (0.149.0)
- [x] Two more (`object/batch_steps21.c`): the next of 101 - 200: a round's results scored (vital bonus, time,
      level, the highest combo) and a fighter's flag +772 by matchups (0.150.0)
- [x] Three more (`object/batch_steps22.c`): the last ready of 101 - 200: a fighter's ranking records written
      after the name entry, the fight's name plates made by layout, a name plate's start (0.151.0)
- [x] One more (`object/batch_steps23.c`): the first of 201 - 400: a ranking row's cells (place, name with
      its shadow, score or count) (0.152.0)
- [x] Two more (`object/batch_steps24.c`): a ranking row's start and the next of 201 - 400: two fighters'
      bodies pushed apart (walls, in the air, half each) (0.153.0)
- [x] Two more (`object/batch_steps25.c`): the first ranking row's start, the fighters pushed apart in pairs
      by layout (0.154.0)
- [x] One more (`object/batch_steps26.c`): the next of 201 - 400: a network menu item's step (status bytes,
      shown and run on the chosen row when its option is allowed) (0.155.0)
- [x] One more (`object/batch_steps27.c`): the next of 201 - 400: the fighters matched to a list of three
      characters, each match given its row of values (0.156.0)
- [x] One more (`object/batch_steps28.c`): the next of 201 - 400: the fight HUD's stun gauges for fighters
      1 and 2 (shades, lengths, flashing marks and palette) (0.157.0)
- [x] One more (`object/batch_steps29.c`): the last of 201 - 400: the fight HUD's pop-up cells for both
      sides (set off by a fighter's +378 / +379 / +380, held by a timer, then coloured) (0.158.0)
- [x] One more (`object/batch_steps30.c`): of 401 - 3000: a menu row object's setup by its kind (+92,
      17 kinds: place, size, shades, flags); every kind checked by forcing it in `mode_tour` (0.159.0)
- [x] One more (`object/batch_steps31.c`): the last ready one of 401 - 3000: the fight HUD's health bars
      (who each side shows by BATTLE +56, the red part catching up, lengths, colours, dimmed while halted)
      (0.160.0)
- [x] The ready list refilled (`tests/scenarios/fighters_*.txt`: all 37 characters' specials and supers,
      2,285 more translated functions reached, 950 ready); 328 of the smallest (2 - 12 instructions) in
      `object/batch_tiny1.c` - `batch_tiny3.c` (0.164.0)
- [x] 286 more step dispatchers, most of them characters' move steps (`object/steps_fighters.c`) (0.166.0)

To do, in rough order:
- [ ] The other 964 step dispatchers (875 plain, 69 whole-frame, 20 with an owner), as scenarios reach them (each is one line in `steps.c`; the
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
  - [x] The tile drawers 0x0881AB28 (32-pixel tiles) and 0x0881B268 (16-pixel), with WIDE NATIVE's
        wider grid as native code; scenario `wide_fight` (0.49.0)
  - [x] The stage's objects (`src/core/stage/objects.c`): the maker 0x0884AE40 (a table of 16-byte
        records per stage) and the routines of every kind the 34 stages use, with their helpers;
        `tools/stage_objects.py` lists the records; scenarios `stage_objects_01` - `34`, one Free Battle
        per stage (0.89.0)
  - [x] The background's master 0x08819CFC and the rest of the layer drawers: 16-pixel record builders
        0x0881A8E0, 0x0881BD28 (RYU's and CHUN-LI's lagging top rows), 0x0881C354 (GUILE's strips),
        BISON's picture layers 0x08819498 / 0x0881C0C8, and the floor band 0x0881C5A8 with CHARLIE's
        and BISON's own (`background.c`, `floor.c`; 0.90.0). Every stage's layers, floor and objects
        are now native but for the unreached 0x0881A2C8 path below
  - [x] The camera's frame 0x0881D1A8 (every layer's follow, the floor's lean, the stages' own camera
        movement), each stage's setup (0x08819A20 and the 23 stage routines) and its palette and tile
        animators (`camera_frame.c`, `setup.c`, `animators.c`; 0.91.0)
  - [x] The select / map / profile / VS screens' backdrops (0x0881CBDC, 0x0881CD20, 0x0881CFD0, 0x08819144, 0x0881A040; 0.93.0)
  - [x] 0x0881B7C8 (0.99.0; the endings, `arcade_ending_full`)
  - [ ] its caller 0x0881A2C8 (from the ending task's 0x089DE2C0 / 0x089DE390)
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

- [ ] Up to three local players, join-in (EX. SETTING PRIORITY, Start on select screens, JOIN / WAIT).
      Now: hooks.c, selectscreen.c. Needs: input records and select screens.
- [ ] Network modes on one PC (EX. SETTING > VIRTUAL PSPS, called COUCH CO-OP before 0.127.0). Now: vpeer.c, netplay.c, hle_net.c.
      Needs: the wireless block.
- [ ] L2 / R2 buttons, PLAYER CONFIG for four pads, pause BUTTON CONFIG rows. Now: playercfg.c,
      exsetting.c. Needs: fighter input.
  - [x] The option screens' part: exsetting.c wrapped around the decompiled OPTION MODE and pause
        BUTTON CONFIG (0.42.0)
- [x] EX. SETTING screen, OPTION MODE sixth item: exsetting.c (and tiles.c), wrapped around the
      decompiled OPTION MODE (0.42.0)
  - [x] Pages (0.127.0; five since 0.128.0) with the side arrows and page number, per-page DEFAULT, row
        descriptions, greyed rows in font colours of our own (0.128.1)
- [x] Cheats and Unlocks (0.127.0 - 0.128.0, the `cheats_unlocks` feature: cheats_unlocks.c): the unlocks
      one by one (forced ones never saved; the shared Arcade bit gated in cursor.c, battle_select.c,
      mode_menu.c), HEALTH / GUARD / SUPER per fighter slot, World Tour and EDIT MODE EXP with Lv 32
  - [ ] 4P's rows, once there is a fourth fighter
- [ ] Silent saves, AUTO SAVE always on. Now: sysdata.c, savestore.c, settings.c.
      Needs: the save task.
  - [x] Silent when a save fails too: no "continue without auto-saving?" (0.24.0; the list of save
        messages is in [SAVES.md](SAVES.md#save-messages-kept-off-screen))
- [x] WIDE NATIVE as a real native setting (0.99.1): every function Rickow's 16:9 patch changed is native
      (0.99.0), and the patch is no longer downloaded or translated in. The WIDE scenarios play with it on
      and off and compare the fighters frame by frame (TESTS.md). Not reached by a scenario yet: 0x088D6D3C, three network menu rows, two of Cammy's CPU states.
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
- [x] #53: each character's sprites and animation frames as PNG with a `metadata.json` (frames,
      offsets, timings), in `characters/<nn>_<name>/fighter/sprites/` ([SPRITE_FORMAT.md](SPRITE_FORMAT.md);
      YUN's frames still to find)
- [x] Sprites loaded from `characters/<nn>_<name>/fighter/sprites/`: an edited picture PNG or colour in
      `metadata.json` is in the game, built into the fighter's archive as it loads
      ([SPRITE_FORMAT.md](SPRITE_FORMAT.md), "Edited sprites") (0.165.0)
- [ ] Higher-resolution sprite replacements (need the drawers to take larger cells); sprites of other
      sets (effects, screens); picture sizes and origins edited in `metadata.json`
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
[STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md). Rule for every item: WIDE NATIVE must never change move
timings or balance; parity with it on matches it off, frame for frame, in every fighter's state.

- [x] Draw body parts past the invisible walls (Blanka's shoulder vanishing at the wall), keeping the walls
- [x] Endings: characters crossing the old 4:3 edge drawn whole, character art over a stage kept at its size
      (0.129.0); stage scenes the ending cut to 4:3 drawn on from the stage's own archive (0.130.0,
      `tools/ending_stages.py`)
- [ ] Stop pop-in and flicker of assets and characters at the screen edges
- [ ] Hide off-screen loading in the pre-battle sequences
- [ ] Fill the MODE MENU background's missing left side
- [ ] Stage art at the camera limits: **stage extensions** ([STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md)).
      Decided: extend the art, keep the limits (pulling them in moves the walls and changes play).
  - [x] Size and placement measured for all 38 stages (`tools/measure_stage_extensions.py`); 76
        magenta placeholders (48 × 288, stage number and side in cyan) committed in
        `assets/extensions/stages/` (moved there, beside the stages' own folders, in 0.37.0)
  - [x] Draw them, WIDE NATIVE only, each behind its own layer: `stagestrips.c` around the decompiled
        tile drawers 0x0881AB28 / 0x0881B268, placed from each layer's scroll, so the camera didn't need
        decompiling first (0.50.0)
  - [ ] A WIDE NATIVE scenario with screenshots of the strips in place, plus the fighters'-state parity
        check below
  - [ ] Paint the 76 strips (any time, in parallel: a good job for a collaborator; magenta = still to do)
  - [x] One strip per layer, joined to each layer's own scroll, with placeholders measured per layer
        (`tools/measure_stage_layers.py`) (0.50.0)
- [ ] Akuma's, E. Ryu's and S. Akuma's caves: the orange placeholders don't meet the black strips in the last
      background layer (check whether that black is the layer's own art, and the strips' placement)
- [x] Art pack: each stage layer whole as its own PNG, from the disc's tile maps (tools/stage_layers.py),
      beside the screenshots at the walls
- [ ] WIDE NATIVE outside the fight (the maintainer's screenshots, 2026-11-18): MODE MENU and FREE BATTLE
      MODE menu backdrops in the left margin; the attract sequence's green band and bars; the wipe into
      the VS screen (margins covered, the stray "F" frame hidden); the VS screen's side bars in the
      Versus colour and moving with it, the name bars longer (Dramatic's two as well); the stage fade
      into the fight; ROUND / FIGHT banners; the KNOCK OUT ticker; the map screen's top banner.
      Since then: EDIT MODE's gradient and WORLD TOUR's map background into the margins.
      Done: the title's backdrop and full-screen fills (0.99.2); the MODE MENU, OPTION MODE and FREE
      BATTLE MODE backdrops and title bars (0.99.3); portraits to the edges on the select, VS and map
      screens, the VS name bars (0.99.4); the select screen's name ticker (0.99.5); the wipe into the
      fight and the stray "F" (0.99.8); the ROUND band, the FIGHT FIGHT rows and the KNOCK OUT row
      (0.99.9); EDIT MODE's and WORLD TOUR's gradients (0.99.10); the A / X / V wipe's shapes and FIGHT's box
      (0.99.11); the map's ladder and beam, GAME OVER, the continue countdown (0.99.12); the profile and
      win screens (0.99.13, 0.99.14); the rival dialogue, the select's
      bottom ticker, the super finish (0.99.15). Left: the attract
      sequence.
- [x] WIDE NATIVE on compared against it off (0.99.1): `wide_fight`, `wide_walls` and `wide_left_wall`
      play twice, and the four fighters must be the same in every frame

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

EX. SETTING's 1P / 2P / 3P PRIORITY rows say PLAYER or COM, but that is not what decides whether a pad
can join: Start joins on the select screens that allow it whatever PRIORITY says, and PRIORITY PLAYER
instead hands a pad the fighter the computer would have played. The rows should say what they do.
Can start once phase 4's join-in item is native code; the rule itself is in [PLAYERS.md](PLAYERS.md).

- [ ] Document the runtime rule completely in `PLAYERS.md`, from the native code:
  - [ ] What PRIORITY PLAYER / COM changes, mode by mode, and what joining with Start does on its own
  - [ ] Which modes let a second or third pad join with Start (Arcade's new challenger, the Dramatic,
        Variable and Free Battle select screens) and which never do (World Tour, Training, R.Dramatic
        Battle from the MODE MENU), and why
  - [ ] Plugging a pad in or pulling it out mid-mode: what happens to the fighter it was playing
  - [x] Bring the page's file paths up to date (it still names `src/runtime/`)
- [ ] Audit the names and labels: the JOIN / WAIT boxes, the CPU2 / CPU3 names, VIRTUAL PSPS
  - [x] JOIN-IN HUMAN / CPU renamed 1P / 2P / 3P PRIORITY PLAYER / COM, as the game's own COM
        (0.86.0; the old `a3mx.cfg` lines are still read)
- [ ] Free Battle (Arcade and Team): a MODE row on the 2P side too, so the second side's MODE can be
      chosen for the computer, or by player 2 once they join
- [ ] Training for two players ([issue #18](https://github.com/theweathervane/a3mx/issues/18)), once
      Training's task and menus are decompiled:
  - [ ] Training: the other pad can JOIN (2P's box when player 1 started, 1P's when player 2 did), and a
        MODE row on that side as in Free Battle
  - [ ] Training's pause menu: whoever presses Start sets CPU OPTIONS for the other side
  - [ ] CPU OPTIONS > ACTION gets PLAYER (the other side's pad plays it), set by itself when a pad has
        joined; with nobody joined it works as PRIORITY PLAYER does
  - [ ] Training's REC with a joined player: the other side's own pad does the recording; without one,
        the game's own REC (the starter takes the dummy over while recording). PLAY and the other
        actions override the joined player's inputs
- [ ] Propose new names or a new layout for EX. SETTING (for example, one row per pad: "takes the
      CPU's fighter: YES / NO"), agree it, and implement it, with the saved setting carried over
- [ ] A parity scenario per changed row: with every row at its default, every mode still plays as on a PSP

## Phase 11: Virtual PSPs architecture review

VIRTUAL PSPS (COUCH CO-OP before 0.127.0) plays the game's network modes on one PC: the other PSPs are virtual players in the same
process (`vpeer.c`, `netplay.c`, `hle_net.c`), and pads 2 and 3 drive network players 2 and 3. Local
play therefore depends on emulating the wireless layer. Needs: phase 4's network item.

- [ ] Audit how VIRTUAL PSPS hooks players 1–3's input into the network modes: the virtual players,
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
      path from virtual PSPs's
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
