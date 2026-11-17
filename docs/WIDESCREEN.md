# Widescreen (WIDE FORCED)

WIDE FORCED is using the SFA3MAX 16:9 patch by Rickow ([acknowledgments](../ACKNOWLEDGMENTS.md)), switched at run time in
game (SCREEN SETTING > SCREEN SIZE).

It was designed for PPSSPP, not for a decomp: it patches immediates and jumps into code caves. This page
collects what works, what doesn't, and the rules for refining it here, in proper code, once the
parts of the game it touches are decompiled.

## Where it lives

- `src/extensions/widescreen.c`: the SCREEN SIZE setting and switching; `src/extensions/screensetting.c`
  draws SCREEN SETTING (WIDE FORCED as the size's name).
- The patch itself (MIT, Rickow): `tools/extract_assets.py` downloads it at the commit pinned in `tools/data/pins.json`, runs its
  patcher only if the file's SHA-256 matches, and writes the result to `work/boot.widescreen.json`.
  Once WIDE FORCED is fully native, the dependancy will be dropped;  the translator compiles both versions of each patched word (`rt_widescreen` picks one).
- **In native code so far (0.49.0):** the background tile drawers 0x0881AB28 and 0x0881B268
  (`src/core/stage/tiles.c`), which draw two and three tiles further left and 17 and 31 columns while
  `rt_widescreen` is set, as the patch does. The `wide_fight` scenario checks them against the
  patched translation with WIDE FORCED on. 0x0881B7C8 stays translated (nothing reaches it yet).
- **The project's own words:** `tools/data/widescreen_more.json` adds words to the patch's overlay
  (the translator merges them, and stops if a word it replaces is not the one listed). The first three
  widen the left limit of the sprite cell drawers 0x08810D84, 0x088112A8 and 0x088117D0 to -96, as the
  patch does in their sibling 0x08811BCC, so fighters are drawn into the left margin. The drawers are
  decompiled with the same limit (`src/core/object/sprite_grid.c`); the `wide_walls` scenario checks
  them with a fighter at the left wall.
  The next five widen the two "drawn when on the screen" tests objects use (0x08821424, 0x088214E8)
  by 48 pixels each side, so an object whose art reaches into the margins is drawn there instead of
  popping in near the 4:3 picture (the foreground posts on Guy's and Dan's stages); `frame.c` does the
  same. `wide_fight` (on Dan's stage) checks it.
  The last four send the patch's jumps in the tile drawers 0x0881AB28 / 0x0881B268 to added code of the
  project's own (`caves`, at 0x08800100: translated only, never loaded into game memory). The patch
  starts each row 2 (3) tiles further left by moving the map pointer back, which read the end of the
  row above when the grid's first map column was smaller than that (layer 0 at the left wall on Ryu,
  Akuma, Sagat, Dhalsim, Balrog and Cammy's stages, layer 2 on Karin's and Hawk's, and the attract
  sequence's panels). It now wraps round within the row, as the arcade's tile layers do; `tiles.c` does
  the same, and `wide_left_wall` checks it.
- What it changes: [PSP_PROGRAM_MAP.md](PSP_PROGRAM_MAP.md) "Widescreen patch" (viewport 384 -> 480,
  camera centre -192 -> -240, the pillarbox at 0x0880F390, the background tile drawers 0x0881AB28 /
  0x0881B268 / 0x0881B7C8, code caves in `.data`).
- The camera and its limits: [STAGES.md](STAGES.md).

## What is working

- **The bars are correctly centred:** during fights, in the middle of the screen.
- **The play area stays 4:3, as intended:** Fights are meant to be played at the original width, so
  characters should never reach the edges of the wide screen, and they don't. The invisible walls are
  enforced correctly: the distance the two characters can be apart is unchanged. That matters for
  moves that use the walls, such as Vega's (US version) and Rolento's wall jumps, which would break if
  characters could move to the edges.

## Known issues

- **Pop-in and flicker at the edges:** The CPS2 original had a tile limitation, so assets and animated
  characters at the edges flicker and pop in as they load. Objects (stage props, effects) no longer pop
  in at the edges since 0.97.0: the tests that drew them only near the 4:3 picture take in the margins.
  What is left is the tile layers' columns past their tile map (see "Off-screen tiles" below).
- **Off-screen loading is visible in the pre-battle sequences:**
- **MODE MENU background tiles** (the Street Fighter Alpha logo) are missing on the left side; they
  extend to the right correctly.
- **Stage art runs out at the camera limits:** on many stages: a black strip of up to 48 pixels at the
  edge. Measured stage by stage in [STAGES.md](STAGES.md#widescreen). This can be filled by stage
  extensions, but new art must be drawn behind the stage in WIDE FORCED only: [STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md).

## What to ensure

**WIDE FORCED must not change the timing of any move**, or anything else that affects the game balance. It
changes what is drawn, never how the game plays: the parity scenarios run with WIDE FORCED on should
match it off in every fighter's state, frame for frame, and a revision must keep that.

## Observations from other projects

[CPS-2 Recomp](https://github.com/tmcleroy/cps2recomp) (GPL-3.0, like this project, so its code can be
used with credit) has 16:9 working in 16 fighting games, its SFA3 among them. It solves most of the
problems listed above. What we can take from it, as read from the maintainer's notes on it and its
code (`video/video.h`, `fight/wide_ends.h`, `games/sfa3/src/widescreen.cpp`):

- **The same goal as ours:** Its "invisible walls" mode widens only the picture: the camera limits and
  the walls stay where they are, so the rule in "What to ensure" holds. Its "played wide" mode, which
  moves the walls, is off by default there. If we ever add one, it must be a separate option.
- **Stage ends without new art: `ViewShift`.** Near a camera limit, the wide view stays on the stage's
  art, and the 4:3 play area moves off-centre by up to the margin (48 px here). It's about 10 lines and
  needs no painted strips.

### Known issues
-  It shows wrong art on the side opposite the end the view is held at (stale off-screen tiles) (Dan's stage).

## What is being attempted
- **New stage art:** Stages were measured ([STAGES.md](STAGES.md)) and we paint the strips for each parallax layer.
  76 stage extension strips have been created and added to repo as placeholders, see ([STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md)).
- **Reviewing FarScroll:** `FarScroll` still worth a look at making the parallax layers draw
  little slower still, so they reach the end of their art when the camera reaches its limit) is the
  fix for the parallax drift that STAGE_EXTENSIONS.md leaves for later. A good example is the background on
  Ryus's stage which causes the border not to meet the new assets. Note: Per-stage numbers are measured
  by a tool, as `tools/measure_stage_extensions.py` already does here.
- **Off-screen tiles: not an issue here.** CPS-2 Recomp rebuilds edge tiles because the arcade streams
  its tile maps. On the PSP each stage layer's whole map is in memory (0x092E8A00, 0x2000 bytes a layer:
  32 columns, or 16 for kind 5, which repeats across anyway); only the map animators rewrite it. What
  was wrong at the edge was the patch's start further left not wrapping (fixed in 0.97.0, above).
- **The MODE MENU's left side** is not a tile layer: its background is the menu's own objects (panels
  and bands that drift and wrap round within the 4:3 width, `object/drift.c`), so nothing is placed in
  the left margin. Widening their layout in WIDE FORCED, or pillarboxing the menu, is still to do.
- **Only widen in fights**, and in modes allowed one by one. That would fix the MODE MENU logo and the
  pre-battle loading in one go: pillarbox those screens instead of widening them.
- **Blanka's shoulder (fixed).** Body parts at the invisible wall were cut at the old 4:3 edge: the
  sprite cell drawers chosen by 0x0880E150 skip a cell that starts left of minus its width, and the
  patch moved that limit to -96 in only one of the four (0x08811BCC). A fighter's parts go through
  three of them, so all three needed it (found with `--watch-pc` on each drawer's left test, then
  checked in a run). The walls are unchanged: only drawing is.
- **The shape of a native implementation.** One place the drawers ask each frame (their `Wide`
  struct: the margin, each layer's extra scroll, a per-sprite offset that keeps the HUD centred), in
  place of patched constants. With the margin at 0 everything is the original, so parity holds with
  it off, and the reference run (the patch's translation) checks it with it on until the patch is
  dropped.
- **Checked by a recorded run with widescreen on.** We have none yet: a parity scenario with WIDE
  FORCED on is the first thing to add before moving the patch's behaviour into native code.