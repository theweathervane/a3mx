# Widescreen (WIDE NATIVE)

WIDE NATIVE is the project's own 16:9 mode, switched at run time in game (SCREEN SETTING > SCREEN
SIZE): 16:9 with the 4:3 play area. It started as the SFA3MAX 16:9 patch by Rickow
([acknowledgments](../ACKNOWLEDGMENTS.md)), which was made for PPSSPP: it patches immediates and jumps
into code caves. Until 0.99.1 the build downloaded it and translated it in as an overlay. Every
function it changed is now decompiled and reads `rt_widescreen` itself, and the patch is no longer
downloaded. This page collects what works, what doesn't, and the rules for refining it.

## Where it lives

- `src/extensions/widescreen.c`: the SCREEN SIZE setting and switching (`rt_widescreen`);
  `src/extensions/screensetting.c` draws SCREEN SETTING (WIDE NATIVE as the size's name).
- What the patch changed, now native (the patch's own words: [PSP_PROGRAM_MAP.md](PSP_PROGRAM_MAP.md)
  "Widescreen patch"):
  - the background tile drawers 0x0881AB28, 0x0881B268 (0.49.0) and 0x0881B7C8 (0.99.0, the endings)
    in `src/core/stage/tiles.c`: two (16-pixel tiles: three) tiles further left and 17 (31) columns;
  - the screen's setup, its extra text layer and the pillarbox (0x0880B928, 0x0880F160, 0x0880F390 in
    `src/core/object/screen.c`): the right edge at 480, no wallpaper bars;
  - the 32-pixel paletted sprite cells 0x08811BCC (`src/core/object/sprite_grid.c`): the left limit -96;
  - the network menus' rows and cursor bar (0x088EC074 - 0x088EC724, `src/core/network/menu.c`), the
    network screens' panels and message boxes (0x088D6BC0, 0x088D6D3C, 0x088E3B8C, 0x088E3BEC,
    0x088E3DB0) and two of Cammy's CPU routine states that pass the screen's width on (0x0895FD00,
    0x0895FDB0) in `src/core/network/screens.c`. No scenario reaches 0x088D6D3C, three of the menu rows
    (ids 0, 9 and 10) or the two Cammy states yet.
- What the project adds beyond the patch:
  - the other three sprite cell drawers (0x08810D84, 0x088112A8, 0x088117D0) have the -96 left limit
    too, so fighters are drawn into the left margin (Blanka's shoulder, 0.95.0);
  - the two "drawn when on the screen" tests objects use (0x08821424, 0x088214E8, `object/frame.c`)
    take in 48 more pixels each side, so props don't pop in (0.97.0);
  - the tile drawers' extra left columns wrap round within their row (0.97.0; 0x0881B7C8 in 0.99.1):
    the patch moved the map pointer back, which read the end of the row above when the grid's first
    map column was smaller (layer 0 at the left wall on Ryu, Akuma, Sagat, Dhalsim, Balrog and Cammy's
    stages, layer 2 on Karin's and Hawk's, the attract sequence's panels);
  - twins for the stage objects that swap props just off the 4:3 screen
    (`src/extensions/widescreen_twins.c`, 0.98.0);
  - full-width draws carried into the margins (`src/platform/ge_soft.c`, `wide_margins`, 0.99.2): a
    single rectangle the game draws across exactly the 4:3 picture (the title's backdrop, full-screen
    fills, fades and flashes) gets a piece in each margin repeating its edge texel column or colour.
    Renderer only: game memory is untouched. Banners, wipes and menu backdrops built from many pieces
    are not covered by it and need their own changes (the list in ROADMAP.md). Since 0.99.3 a draw
    gets a piece on each side it stops short of, even if the other side reaches the screen's edge;
    since 0.99.10 a rectangle shaded top to bottom (EDIT MODE's green, WORLD TOUR's blue behind the
    map) gets a piece shaded the same way, not one flat colour;
  - the menus' tiled backdrops (`src/core/object/draw_kinds.c`, kind 2, 0.99.3): an object whose
    parts repeat across gets more copies of its end columns until they reach the 16:9 sides;
  - side pieces at the screen's edges (0.99.4): the sprite drawer (`src/core/object/sprite.c`) moves
    the drawing of the sprites `widescreen_edge_shift` names (`src/extensions/widescreen.c`) out by
    the margin: the select screen's portrait, small character and MODE / SPEED boxes; the VS screen's
    portraits (its name bars are drawn a second time moved out); the map screen's portrait;
  - the fight's HUD (0.99.35) stays on the 4:3 picture: what its pictures and bars have past the 4:3
    sides (the super / ISM bars' ends at the bottom, art longer than the 4:3 screen that its sides hid)
    is cut off there (`src/core/object/hud.c`, decompiled for this);
  - the fight's banners (0.99.9): the ROUND banner's band carries on into both margins (its right-end
    cells, `src/core/object/sprite_grid.c`), and the FIGHT FIGHT rows and the KNOCK OUT row get one
    more piece past an end that has reached the 4:3 edge (`src/core/object/sprite_drawers.c`), both
    for the sprites `widescreen_edge_cells` / `widescreen_edge_row` name.
    Since 0.99.11 FIGHT's translucent box (its after-image) carries on the same way, and since 0.99.12
    the GAME OVER band and the continue countdown's bar (left only); the sides are strips of one texel
    column, the ROUND band's left side the dimmer step of its glow after the D (0.99.22: bright, dimmer,
    line, as on the right), and the continue count gets its earlier numbers;
  - the profile screen before the map (0.99.13): its header and text box drawn widened (each half
    moved 48 out, the plain middle stretched), its name plate's box and the box behind the text (plain
    rectangles) moved and widened with them (0.99.14), its side columns at the edges;
    the win screen after a fight: its side columns at the edges, the band under the quote wider;
  - the dialogue before a rival or final fight (0.99.15): the portraits at the edges (flush since
    0.99.22: they stand 16 in on the 4:3 screen, so they move 64), each line centred where WIDE shows it
    (0.99.36; it had moved with its speaker's portrait);
  - the super finish's backdrop (`src/core/object/super_backdrop.c`, 0.99.22): the vertical lines
    (kinds 3, 4) and the speed lines (5) mirror their side cells out; every other kind (slanted lines,
    splash, burst, swirl) is drawn whole 1.25 times over-sized about the screen's middle, so its lines
    and rings fill the 16:9 screen unbroken (28 rows are lost above and below);
  - the attract mode (0.99.23): the QSound screen's background (0.99.29: also while it fades in and out), and the word screens' banners (the
    top word's, and the band that lights up behind the middle or bottom word) carry on, their side cells
    mirrored out (`widescreen_edge_cells` 16, `sprite_grid.c` mode 4, as many cells as the margin
    needs); GAME OVER at the end of an ending (with TRY AGAIN) has its band carried on like the other;
  - the profile screen's text box (0.99.28): the dark blue box behind the text stops at the frame's red
    curve instead of being carried on to the screen's left edge, so the frame's corner left of the box is
    see-through and the box stands clear of the left column;
  - more of the attract mode (0.99.24): the score ranking's RANKING columns at the screen's sides (0.99.32:
    its score bars 48 longer at each end, in their end colours, so the ladder still meets them; 0.99.33:
    the box behind the first place's portrait 48 wider each side, under their coloured panels); the
    green and red speed-line screens behind the close-ups carried on (mirrored); on the red ones the
    STREET FIGHTER ALPHA 3 strip starts at the left side and is drawn again after itself, and the rows
    of a fighter's name get one more name at an end; the faces band behind a word is drawn again a band
    to each side (half a face from its other end in each margin); the earth band, already 512 wide, is
    left as drawn (0.99.23's mirroring had bent its horizon);
  - the attract mode's name rows over black (0.99.25): the names and fighters' outlines are holes in a
    black layer (`object/mask_layer.c`, decompiled for this): holes are cut out to x -64 - 447, the
    left margin's at the ends of the layer's rows, which the text layer draws there (0.99.30: its main
    part in two halves, so the renderer's margin fill no longer covers them); the coloured panels
    behind carry on round (`widescreen_edge_row` 3); the blue screens' name banners get one more name;
    the flames before M. Bison (a title backdrop of kind 5 at scroll 0, standing on a whole band of its
    map) carry on round their 12 tiles; the scene that scrolls from Ryu under the clouds up to M. Bison
    is not carried on (0.99.29: it showed the wrong parts of the map at the sides) but its backdrop,
    M. Bison's layer and the eagle are drawn 1.25 times over-sized about the screen's middle (0.99.31),
    so the clouds and M. Bison fill the screen; Ryu, in front, stays as drawn;
  - the ALPHA letter tiles before the title (0.99.32): once in, the grid's last two columns are drawn
    again before its first and its first two after its last, so it reaches both sides;
  - the attract mode's Akuma screens (0.99.29): the blue swirl behind his back and the burst behind the
    red kanji are drawn whole 1.25 times over-sized about the screen's middle, as the super finish's
    swirl and burst are (`widescreen_oversize`);
  - the endings (0.99.22): a scene of the ending's 24-tile map standing still on one of its halves has
    its own columns mirrored out at its sides instead of the other scene's (`src/core/stage/tiles.c`),
    and the black sides that narrow a scene to a panel reach the screen's sides;
  - the map screen (0.99.12): the ladder moves to the edge away from the middle; the beam, where its
    picture ends in a margin (toward England, Italy, Hong Kong, Thailand and the like), has its noise
    carried on to the screen's side as it is, not scaled, cut to the beam's edges (0.99.34; it was drawn
    again scaled about its point, which blew up the lit wings in it);
  - the wipes' flat shapes (`src/core/object/shapes.c`, 0.99.8, redone in 0.99.11): a quad with an
    edge on or past the 4:3 picture's side gets a rectangle in its colour out to the screen's side; the
    quad keeps its corners, so the A, X and V into the fight keep their 4:3 slopes;
- Testing: `wide_fight`, `wide_walls` and `wide_left_wall` play twice, WIDE NATIVE off and on, and the
  four fighters must be the same in every frame ([TESTS.md](TESTS.md)). What is drawn is checked by eye.
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
  extensions, but new art must be drawn behind the stage in WIDE NATIVE only: [STAGE_EXTENSIONS.md](STAGE_EXTENSIONS.md).

## What to ensure

**WIDE NATIVE must not change the timing of any move**, or anything else that affects the game balance. It
changes what is drawn, never how the game plays: the WIDE scenarios check that every fighter's state
matches WIDE NATIVE off, frame for frame (`wide` in a scenario, `tests/parity.py`), and a revision must
keep that.

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
- **Objects that change props at a line (twins, 0.98.0).** To save sprites, some stage objects play
  two props ("swap at the line", `stage/objects.c` swap_kind: kinds 4, 6, 11, 29, 31, 33, 39, 52, 58,
  82, 84, on Ken, Akuma, Adon, Sagat, Dan, Sakura, Zangief, E. Ryu, Mika, S. Akuma and Karin 2's
  stages): one place while the fighters' layer is left of a line, another past it, the change timed
  just off the 4:3 screen. In 16:9 it happens in view (Dan's crow appears on the sign the moment the
  camera moves off its starting place, x 448, which is the line). `extensions/widescreen_twins.c` gives
  each, in WIDE NATIVE, a twin showing the other look, so both are there while in view. It runs around
  the decompiled kinds (`SFA_FUNC_WRAPPED`); NORMAL is unchanged byte for byte.
- **The MODE MENU's left side** is not a tile layer: its background is the menu's own objects (panels
  and bands that drift and wrap round within the 4:3 width, `object/drift.c`), so nothing is placed in
  the left margin. Widening their layout in WIDE NATIVE, or pillarboxing the menu, is still to do.
- **Only widen in fights**, and in modes allowed one by one. That would fix the MODE MENU logo and the
  pre-battle loading in one go: pillarbox those screens instead of widening them.
- **Blanka's shoulder (fixed).** Body parts at the invisible wall were cut at the old 4:3 edge: the
  sprite cell drawers chosen by 0x0880E150 skip a cell that starts left of minus its width, and the
  patch moved that limit to -96 in only one of the four (0x08811BCC). A fighter's parts go through
  three of them, so all three needed it (found with `--watch-pc` on each drawer's left test, then
  checked in a run). The walls are unchanged: only drawing is.
- **The shape of a native implementation.** One place the drawers ask each frame (their `Wide`
  struct: the margin, each layer's extra scroll, a per-sprite offset that keeps the HUD centred), in
  place of the patch's constants, now that they are all native. With the margin at 0 everything is the
  original, so parity holds with it off.