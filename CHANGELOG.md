# Changelog

## 0.102.4
### For players
- **Launcher, Inputs > Configure:** **Close** now sits below a line with space above it, away from the **Default All** / **Unset All** buttons, so Unset All isn't pressed by mistake.

### Technical
- `configure_window()`: a gap (`em * 1.2` and the item spacing) with a `Separator` between the Default / Unset All row and Close; the window's height and the table's footer allow for it.

## 0.102.3
### For players
- **Launcher, Inputs > Configure:** the table no longer jumps while you test. The pad icon used to be taller than a row, so a row grew while its button was held and everything under it moved; and the hint beside **Test** wrapped to a second line during a test, moving the whole table. Both now keep their size.
- No more red "MESSAGE FROM DEAR IMGUI" box on the Circle row: the key **X** and the unset **X** beside it no longer clash.

### Technical
- `configure_window()`: the icon column is a fixed `Dummy` the height of a row, the icon painted into it with `AddImage` (whole multiples of its pixels, no taller than a row), never laid out; the Test hint is one line in both states. The unset button is `X##unset` and each binding button `<label>###bind`, so a key or button named like another control has its own ID (Dear ImGui's ID-conflict warning, `ConfigDebugHighlightIdConflicts`).
- Checked: a scratch build faking held buttons on every other row; the table's rows identical to the idle picture, pixel for pixel. Built on Linux, no warnings.

## 0.102.2
### For players
- **Launcher, Inputs > Configure:** two headings, **Test input** (with **Test**) and **Configuration** (the buttons). During a test the player's icon lights in a column of its own, the same place on every row, beside the button's name (now right-aligned); the key or controller button that is pressing it turns orange, its cell too.
- **Default All** and **Unset All** under each column: keys under Keyboard, controller buttons under Controller. (Default keys / Default buttons are gone.)
- **Dependencies, Consent:** a new heading at the bottom says what you have agreed to (or that nothing is agreed yet, and which button at the top does it), and holds the FILTER shaders and librashader buttons, each its own agreement. The agree button stays at the top.
- **The console** is in a frame of its own with a **Console** heading, like the tabs' page. The bar between them is thicker and orange (brighter while you drag it).
- **About:** "A3MX - Launcher" in bold; the line "A decompilation of the PSP game..." is gone. Hover over the A3MX picture to see the version at a glance.
- **Hover text everywhere:** every button, link, box and tab now says in a line what it does (the tabs, Open / Browse on Paths, the downloads' and librashader's windows, the report window, the extra options, Close both / Cancel, and the rest).

### Technical
- `launcher.cpp` `configure_window()`: four columns (icon, name right-aligned, Keyboard, Controller); `TableSetBgColor(CellBg)` and the button in `ImGuiCol_CheckMark` (with `DisabledAlpha` 1 so it shows while the cells are disabled during the test) for the side that is down (`button_down` bit 1 keys, 2 controller); the column positions are kept from the first row so the Default All / Unset All pairs line up under them; Close on its own row. Sized for the two headings and the extra row.
- `bold`: a bold system font (segoeuib.ttf, Arial Bold, DejaVuSans-Bold, LiberationSans-Bold), else the title drawn twice a pixel apart. `TAB_TIPS` for the tabs; tooltips added to ~30 controls (`SetItemTooltip`, shown on hover and while disabled). Dear ImGui draws its own controls, so screen readers can't read them: the hover text is for people, not assistive software.
- `deps_tab`: the FILTER extras and the consent line moved under `SeparatorText("Consent")`, shown on every check.
- The console: `BeginChild("console", Borders)` with `SeparatorText("Console")` around the log; the split bar 9 px, drawn 3 px in `CheckMark` at 60 % (100 % on hover or drag).
- Checked: the launcher built on Linux with no warnings; Configure, About, Dependencies (Consent, with a taller window) and the console looked at in screenshots. The orange key highlight needs a real key press: not seen here.

## 0.102.1
### For players
- **Launcher, Inputs > Configure:** while **Test** runs, the keys are only the player's: the arrows, Enter, Space and Esc no longer move around, press buttons or close the window (stop the test with the mouse). The window now opens near the top and tall enough to show every button without scrolling.
- **Launcher, a tidier top:** the version has moved from the top right to the **About** tab, so the picture and tabs sit closer together. Each page's grey title is gone; Dependencies, Inputs, Extensions and Build / Run each start with a heading instead, and Build / Run's **Game** section is now called **Run**.
- **About:** "A3MX - Launcher" with the version under it; **Report a bug** and **Request a feature** are links, each on its own line (the bug icon is gone).
- **Inputs:** the text under Current now says what the numbers are: the players the game will have, as the MODE MENU shows them, a controller counting only if it is still connected when the game starts.

### Technical
- `launcher.cpp`: while `ctl_test`, the event loop keeps key-down and text events from ImGui (key-ups still go, so no key stays held); Esc doesn't close Configure during a test. `configure_window()` sized from the rows (`CB_COUNT` + header, Test, the bottom buttons), placed `em * 0.75` from the top, clamped to the launcher's height. `header()` without the version line; the page's title and separator removed from `frame()`; `SeparatorText` at the top of `deps_tab`, `inputs_tab`, `extensions_tab`, `run_tab`. `draw_bug` and `bug_dark` removed (icons.cpp / .h).
- Checked: the launcher built on Linux (no warnings); every tab and Configure looked at (light and dark). The screenshots are still to be retaken on Windows.

## 0.102.0
### For players
- **Launcher, tabs across the top:** the tabs now run along the top beside the A3MX picture, each with a small pixel picture over its name (a folder for Paths, chain links for Dependencies, a controller for Inputs, a tick for Extensions, a disc for Build / Run, an i for About). The chosen tab's picture and underline are orange. The version is in the top right corner.
- **About is a tab** (was a button and a window): the project's pages, the **bug** icon and **Report a bug**, **Request a feature** and the licences are all there.
- **Inputs, Configure:** a player's keys and controller buttons, **Default keys** / **Default buttons** and **Test** are now in their own window, opened with **Configure** under **Controller**. It can be resized; its size is kept until the launcher closes, not saved.

### Technical
- `launcher.cpp`: `sidebar()` replaced by `header()` (picture, tabs as `Selectable`s with the picture and name drawn over them, version top right; the picture narrows when the tabs need the room); `T_ABOUT` / `--tab about`; `about_window()` became `about_tab()`, `report_window()` is called from `frame()`; `--show about` now opens the tab. `configure_window()`: a `BeginPopupModal` with `ImGuiWindowFlags_NoSavedSettings` and `ImGuiCond_FirstUseEver` size (kept for the session only); Esc closes it unless a key is being waited for; closing stops Test. `--show configure` (with `--tab inputs`) opens it for screenshots.
- `icons.cpp`: `draw_tab_icon` (six 13-pixel pictures, whole-pixel squares), `tab_icon_pixel`.
- Screenshots: `launcher-inputs-configure` and `launcher-about` added to `tools/data/screenshots.json` (linked from BUILDING.md); `docs/images/a3mx_sm.png` (README's) listed as artwork. The launcher's screenshots are to be retaken on Windows (`python tools/screenshots.py`): until then `--check` reports them out of date and the two new ones missing.
- Docs: BUILDING.md (the launcher's parts, the About row), INPUTS.md (Configure), REPORTING_ISSUES.md (the bug is on the About tab).
- Checked: the launcher built on Linux (clang, SDL3 3.4.16 without X11 / Wayland), every tab and the Configure window looked at in screenshots, light and dark. Not built or run on Windows here.

## 0.101.0
### For players
- **GPU renderer:** the game's draws are now sent to the graphics card in batches. In a fight the PSP's ~750 draws a frame go out as about 100, so far less time goes on the driver's overhead for each one. The picture is unchanged.
- The bug report's `last_run.txt` (and `--perf-log`) now say how many draws a frame the game made and how many the GPU renderer sent them in.

### Technical
- `platform/ge_gl.c`: a draw's triangles join the open batch while its render target, scissor, texture and sampling, shader settings, blending and colour mask (`DrawState`) match the batch's; otherwise the batch is drawn first (one `glDrawArrays`). The batch is also drawn before a texture upload or delete (`upload_rows`, `big_free`), presenting, `ge_gl_read`, `ge_gl_finish` and the frame's end. With one batch open, render-to-texture always sees every earlier draw. Per-draw uniform and state calls are gone with it.
- `platform/perf.c`: `perf_count_draws`; CSV columns `draws`, `draw_calls`; `draws=` / `draw_calls=` in `last_run.txt`; the summary line adds "N draws in M draw calls a frame".
- **The GPU renderer has a test (concern C15):** `tests/gl_test.py` runs a scenario with the software renderer and the GPU renderer at scales 1 and 2 (headless EGL, llvmpipe is enough) in NORMAL or WIDE NATIVE. Every frame's memory and fighters must match the software run's, and every 30th frame's picture must match it within a tolerance (4 x 4 averages, 32 a channel, at most 0.75% of squares; measured worst 0.05% at scale 1, 0.39% at scale 2). ctest `gl_renderer_normal` and `gl_renderer_wide` (label `gl`, `arcade_fight`, ~6 min each on 2 cores) exist only in a build with `-DA3MX_EGL=ON`.
- Checked: `tools/compare_gl_frames.py` old against new, `arcade_fight` every 4th frame: all 1,500 identical, game state the same. `arcade_fight` in a fight (llvmpipe, scale 1): 758 draws in 104 calls a frame; the render part's CPU time 7.3 → 2.7 ms (llvmpipe's own drawing moves to the present, so its frame totals say nothing about a real GPU). `gl_renderer_normal`, `gl_renderer_wide` pass; `attract_mode` in WIDE NATIVE at scale 2 too (12,000 frames, worst 0.07%).
- **WIDE NATIVE's margins through OpenGL** (`ge_soft.c` `wide_margins`, unchecked until now): the same as the software renderer's in `arcade_fight` (VS screen, wipe, ROUND band, fight) and the whole attract sequence. The ROUND band reaches the left edge with both.

## 0.100.3
### For players
- **WIDE NATIVE, score ranking:** on the A-ISM (green) and V-ISM (blue) rankings the see-through box behind each fighter's face now moves with the face, as on the X-ISM ranking since 0.100.2.

### Technical
- `extensions/widescreen.c` `widescreen_plain_adjust`: the box behind a ranking row's profile is a plain rectangle 96 wide of type 1 (X-ISM), 2 (A-ISM) or 3 (V-ISM); 0.100.2 matched only type 1.
- Checked: parity (memory hashes, renderer lazy) attract_mode; by eye: A-ISM list and first place (frames 11100, 11300), V-ISM (16800, 17000), 4:3 against WIDE NATIVE.

## 0.100.2
### For players
- **WIDE NATIVE, score ranking (attract mode):** on each score bar the place ("1st", "20th") and the fighter's profile now sit at the bar's ends, as far from them as on the 4:3 screen; the score moves a little right so the gaps between place, name, score and profile come out nearly even.

### Technical
- `extensions/widescreen.c`: `RANK_SHIFT` { -48, 0, 12, 48 } px (place, name, score, profile). The text sprite (kind 35 type 10, first place 11; drawer 6) and its shadow (kind 34 type 4) move by piece, grouped by the piece's x (place <= -112, name < -40, score) through the new `widescreen_piece_shift` (`core/object/sprite_drawers.c` `draw_pieces_from`). The profile: the face (kind 35 type 1, `widescreen_edge_shift`), the name under it (kind 34 type 5, linked, the new `widescreen_linked_shift`, added to the parent's in `core/object/sprite.c`) and the translucent box behind (plain rectangle type 1, 96 wide, `widescreen_plain_adjust`). Stand-ins in `platform/extensions_off.c`.
- Measured (screen px, 4:3 → WIDE NATIVE): bar end to place 10 → 10; place / name / score / profile gaps 14 / 47 / 23 → 62 / 59 / 59 (first place), 14 / 50 / 14 → 62 / 62 / 50 (other rows).
- Checked: parity (memory hashes, renderer lazy) attract_mode; wide_fight WIDE NATIVE off / on; by eye: the ranking's list and first place, slide-in and slide-out (frames 4400 - 5600).

## 0.100.1
### For players
- **Inputs:** the launcher's Inputs tab now saves to `configs/inputs.cfg` (was `controllers.cfg`). An old `controllers.cfg` is no longer read: set your players up again in the Inputs tab (a new setup starts with player 1 on the keyboard and players 2 to 4 on None). You can delete the old file.

### Technical
- `controllers.cfg` renamed `inputs.cfg` in the launcher (`controls_path`, the bug report's zip, the Paths tab text), the game (`host_sdl.c` `byod_load`), `platform/controls.h` and the docs (INPUTS.md, BUILDING.md, REPORTING_ISSUES.md). No fallback to the old name (no compatibility shims before 1.0). The launcher's summary line says "inputs default" when there is no file.
- Checked: g++ syntax check of `launcher.cpp`; `host_sdl.c` not compiled here (a rename of a string only). Parity unaffected: the tests read no inputs file (`--save-dir`).

## 0.100.0
The WIDE NATIVE presentation work of 0.99.x is finished. From here the minor version goes up until the decompilation is complete.

### For players
- **Launcher, Inputs tab (was Controllers):** a new setup now has **player 1 on the keyboard and players 2 to 4 set to None**. No controller arrangement is assumed any more, so input mappers and overlays (reWASD, Steam Input) only get what you set up yourself (issue #20). An existing `controllers.cfg` keeps what it says.
- **Current** (was "Now:") is at the top of the tab with the larger bubbles; under the new **Configuration** heading the chosen player's **Pad icon** comes first, small, with no text beside it. A player set to None shows a greyed-out **0** bubble.
- Changing a player to Controller or Keyboard/Controller gives it the usual buttons to start from.
- The unset button is now an orange **X** in white, like the radio buttons.
- **Default keys** no longer says two players can't share a key: they can, and nothing stops it.

### Technical
- `platform/controls.h` `controls_defaults`: player 1 `CT_KEYBOARD` (usual keys, usual pad buttons kept for later), players 2 - 4 `CT_NONE`; the game's no-file log line in `host_sdl.c` to match. Runs with `--save-dir` (the tests) read no file and are unchanged.
- `platform/padicon_art.h`: a light grey bubble with a 0, `PADICON_NONE` (index 4), drawn only by the launcher; the game's pad icons still use 0 - 3.
- `launcher.cpp`: tab renamed Inputs (`--tab inputs`; `--tab controllers` removed); `bubble[5]`; the control type combo fills `CONTROL_DEFAULT_PAD` for a controller type with no buttons; the unset button styled from `ImGuiCol_CheckMark`.
- Renamed to match the tab, with nothing kept under the old names: `T_INPUTS` / `inputs_tab`, docs/CONTROLLERS.md → docs/INPUTS.md, the screenshot `launcher-controllers` → `launcher-inputs` (docs/images/launcher-inputs.png); BUILDING.md updated.
- Checked: g++ syntax check of `launcher.cpp`; the bubbles drawn from `padicon_art.h` by eye. Not run here: a launcher build, the screenshot retake (`python tools/screenshots.py launcher-inputs`), parity (no game logic changed).

## 0.99.37
### For players
- **WIDE NATIVE, GUILE's stage:** no more pale, broken column at the left edge as the train's strips go by: the train carries on to the screen's left edge. (The black block at the bottom left, below the floor, is still there: left for later.)

### Technical
- `stage/tiles.c` + `stage/background.c`: GUILE's strips layer (0x0881B268 kind 0x90, three 16-column strips picked by the table 0x08A08BDD) starts 3 tiles further left in WIDE NATIVE; left of the first strip's column 0 those tiles were read from the words before the strip (garbage). They now come from the end of the strip before it in the table (`stage_strip_before`, set by 0x0881C354 while it draws).
- Not changed: GUILE's floor (kind 0xC0) has no tiles left of the 4:3 picture in its bottom rows (black, hidden by the super bar's over-long end until 0.99.35); the maintainer will come back to it.
- Checked: parity (memory hashes, renderer lazy) stage_objects_31_guile, arcade_fight, attract_mode; wide_fight and wide_left_wall WIDE NATIVE off / on; by eye over GUILE's fight (frames 4700 - 5600).

## 0.99.36
### For players
- **WIDE NATIVE, rival dialogue:** the lines before a rival or final fight are centred as in WIDE (each line's middle where WIDE shows it), instead of being pushed out toward their speaker's portrait.

### Technical
- `object/text_boxes.c` (already decompiled): in a fight's dialogue each line's shift is worked out from its own length: its middle at 1.25 times its place in the 4:3 picture, as WIDE draws it (`line_shift`); the 64-unit push toward the portrait (`widescreen_text_shift`) is now only the test for "a dialogue line".
- Checked: parity (memory hashes, renderer lazy) arcade_rival_dialogue, attract_mode; wide_fight WIDE NATIVE off / on; by eye against WIDE, frames 3800 - 4250 of arcade_rival_dialogue, both halves of the screen.

## 0.99.35
### For players
- **WIDE NATIVE, fights:** the super / ISM bars at the bottom no longer stick out past the line of the health bars: their ends, longer than the 4:3 screen and hidden by its sides there, are cut off at the 4:3 sides, so the HUD lines up as on the 4:3 screen.
- **OPTION MODE > SCREEN SETTING:** DEFAULT now sets SCREEN SIZE to WIDE NATIVE (as a new save starts).

### Technical
- Decompiled the fight's HUD: 0x0880DC08, 0x0880DC88, 0x0880DE2C, 0x0880DF98, 0x0880DFC8 (the parts 0x0880DB78 calls), 0x0880E038 (an element as a picture) and 0x08810B74 (an element as a shaded rectangle) into `object/hud.c`. Parity holds in arcade_fight (also with the software renderer, pictures included), arcade_rival_dialogue, attract_mode, option_tour, option_tour_original, screen_setting.
- WIDE NATIVE: the HUD's pictures are cut at the 4:3 sides by a new x clip in `object/sprite_grid.c` (`sprite_grid_clip_x`, texels cut with them); its shaded rectangles in `hud.c`, their corner colours mixed at the cut.
- `extensions/screensetting.c`: DEFAULT also calls `widescreen_set(2)`.
- Checked: wide_fight WIDE NATIVE off / on; by eye: the fight HUD in WIDE NATIVE (frames 4200, 4500), SCREEN SETTING's DEFAULT from NORMAL.

## 0.99.34
### For players
- **WIDE NATIVE, map screen:** where the beam to the next country runs off the side of the screen (England, Italy, Hong Kong, Thailand and others in those directions), only its moving noise carries on to the edge, at its own size. Before, the end of the beam was a blown-up copy that showed parts of the wings' picture enlarged.

### Technical
- `object/sprite_drawers.c` `wide_beam`: no scaled copies. For a beam picture (kind 19 type 2, anchor 3; pictures 44, 46, 50, 52 and their flips) whose far end is already past the 4:3 side (16 - 32 px into a margin), the rest of the margin gets a picture made on the stack: cells of the far-end column (inner ones, attribute bit 0x20 clear, not the two rows nearest the wing side), one column further out and one row more on the side it widens to.
- `object/sprite_grid.c` `sprite_grid_wedge`: those cells drawn in 2-px slices cut to the wedge (the level of the point and the slanted edge, 0.513 screen px a px, measured on the pictures; 44 / 50 between them, 46 / 52 beyond the slanted one), texels 1:1.
- Checked: parity (memory hashes, renderer lazy) wide_fight (WIDE NATIVE off / on), arcade_fight, attract_mode; by eye: pictures 42 - 52 forced at the map's point (a test-only override, not kept), both sides.

## 0.99.33
### For players
- **WIDE NATIVE, attract mode:** on the score ranking's first place, the see-through box behind the portrait reaches under the coloured panels at the RANKING columns again, as on the 4:3 screen, instead of stopping short with a black gap at each side.

### Technical
- `widescreen_plain_adjust`: the score ranking (GAME + 0 = 4), the plain rectangle of type 0, 288 wide (translucent white, x 54 - 342): 48 out each side. The coloured panels are part of the RANKING column sprites (kind 35 types 3, 4), moved 48 out since 0.99.24, and covered the box's ends on the 4:3 screen.
- Checked: parity (memory hashes, renderer lazy) attract_mode; wide_fight WIDE NATIVE off / on; by eye: the first place 5360 - 5680.

## 0.99.32
### For players
- **WIDE NATIVE, attract mode:** on the score ranking, the score bars are longer at both ends (in their own end colours, the stepped gradient as drawn), so the ladder meets the RANKING columns at the screen's sides as it does on the 4:3 screen, for every ISM colour. The ALPHA letter tiles before the title have no black side columns once they have come in: the grid carries on round to both edges.

### Technical
- `object/sprite_grid.c` `sprite_grid_bar_ends`: the first cell (left cap) moved left, the last (right cap) right, the gaps filled with the second cell's first texel column and the next-to-last cell's last, stretched. Set by drawer 3 (`object/sprite_drawers.c`) from `widescreen_bar_ends` (GAME + 0 = 4, kind 35 type 9: 48).
- Drawer 1: `widescreen_column_copies` (GAME + 0 = 5, kind 125 type 9, ten columns of letter tiles): once the leftmost column is within two spacings of the left side, the last two columns are drawn again before the first and the first two after the last, at the spacing of the end they join. The first attract cycle's scattered tiles are unchanged (pixel-identical).
- Stand-ins for both in `platform/extensions_off.c`.
- Checked: parity (memory hashes, renderer lazy) attract_mode, arcade_fight; wide_fight WIDE NATIVE off / on; by eye: the ranking in X-, A- and V-ISM colours, the ALPHA tiles 11558 - 11576.

## 0.99.31
### For players
- **WIDE NATIVE, attract mode:** the scene that goes from Ryu under the clouds up to M. Bison fills the screen: the clouds, the red behind M. Bison, M. Bison and the eagle above him are drawn larger (1.25 times, as Akuma's screens are), instead of the black sides 0.99.29 put back. Ryu stays his own size.

### Technical
- `stage/tiles.c`: tiles of a grid with `scene_oversize` are drawn 1.25 times over-sized about (192, 120), edges on whole pixels (`oversize()`), for the attract mode's kind 5 backdrop at v scroll under 768 (frames 1580 - 1607, 7403 - 7430, 13117 - 13144, 18891 - 18918 of attract_mode) and the 16-pixel layer of kind 3 (the clouds' and M. Bison's; the attract mode has kind 3 only there). Ryu's close-up (kind 4) is left alone. `stage_scene_oversized()`: either was drawn this frame or the last.
- `object/sprite_grid.c` 0x08811BCC: while `stage_scene_oversized()`, the list's first picture (the eagle) is drawn with the same scale; Ryu's hair and headband, the list's other pictures, are not.
- Reviewed by the maintainer on a branch first. Checked: parity (memory hashes, renderer lazy) attract_mode; wide_fight WIDE NATIVE off / on; by eye: the scene in all four attract cycles, the flames before M. Bison and the screen after the scene.

## 0.99.30
### For players
- **WIDE NATIVE, attract mode:** on the black screens with the scrolling names (CHUN-LI, CHARLIE, DHALSIM, ZANGIEF) and the fighters' shadows, the names and shadows now reach the left edge of the screen. Before, the left strip was black or showed broken lines.

### Technical
- `object/screen.c` (the extra text layer, the mask layer's picture): in WIDE NATIVE its main quad is drawn to x 432 (texels 0 - 431) in two halves. Across the whole 4:3 picture, the renderer's margin fill (`platform/ge_soft.c` `wide_margins_rect`) stretched its first texel column over the left margin, covering the margin's own quad (texels 464 - 511, 0.99.25) but for lines where that column was a hole; it also ran past the screen's right edge.
- Checked: parity (memory hashes, renderer lazy) attract_mode, arcade_fight; wide_fight WIDE NATIVE off / on; by eye: the name-row screens 1640 - 1710 and 1850 - 1940, and the mask layer in memory against the screen.

## 0.99.29
### For players
- **WIDE NATIVE, attract mode:** the QSound screen's background stays at full width while it fades in and out. Akuma's screens fill the screen: the blue swirl behind his back and the burst behind the red kanji are drawn larger, as the super finish's swirl and burst are. The scene that goes from Ryu under the clouds up to M. Bison is back to black sides (filling them showed the wrong parts of the picture); the flames before M. Bison still fill the screen.
- **OPTION MODE:** leaving with Circle no longer swaps the menu for the game's old five buttons as it fades out; it fades out as it looked.

### Technical
- `widescreen_edge_cells`: the QSound screen (kind 122) for GAME + 2 = 6 - 8 (fading in, shown, fading out), not just 7.
- `stage/tiles.c`: kind 5's wrap (`scene_mirror` 2) only for a layer standing on a whole band (v scroll, +98, a multiple of 256, not scrolling: moved 0 or 64+ since the frame before). The flames step between bands (1792, 2048, 2304); the Ryu / M. Bison scene scrolls through them (315, 207, 137).
- `object/sprite_drawers.c` drawer 0: a sprite `widescreen_oversize` names (attract, kind 54 types 6, 8, 9: Akuma's burst and swirl) is drawn whole 1.25 times over-sized about (192, 112), as `super_backdrop.c` does (stand-in in `platform/extensions_off.c`).
- `extensions/exsetting.c`: Circle on the option menu moves OPTION MODE to state 2 (the fade out) and the menu's frame stops running, so `exsetting_frame` tore down our pictures, the sixth button and the lay-out while the buttons were still fading out. It now keeps them while OPT + 0 = 2 and the buttons are there.
- Checked: parity (memory hashes, renderer lazy) attract_mode, option_tour, option_tour_original, save_load, screen_setting, arcade_fight; wide_fight WIDE NATIVE off / on; by eye: QSound fades, Akuma's screens (two attract cycles), the flames (three), the Ryu / M. Bison scene, the ALPHA tiles before the title, OPTION MODE's Circle fade.

## 0.99.28
### For players
- **WIDE NATIVE, character profile:** the red square 0.99.27 added left of the text box is gone (it stayed put while the frame slid in and out). The dark blue that filled the frame's corner left of the text box is gone too, so the box now starts at the frame's red curve with the background showing beside it.

### Technical
- Removes 0.99.27's red patch (`fill_profile_socket` in `object/sprite_drawers.c`): it was drawn at a fixed screen position, not with the sliding frame.
- The navy in the socket was not the frame's texture: the box behind the text (kind 0 plain rectangle, type 1, 304 wide), widened 48 each side, spans the whole 4:3 picture, so the renderer's margin fill (`platform/ge_soft.c` `wide_margins_rect`) carried it on to x 0. `object/draw_kinds.c` now draws it in two halves when `widescreen_plain_halves` (`extensions/widescreen.c`; stand-in in `platform/extensions_off.c`) says so; neither half spans the picture, so neither is carried on.
- Checked: wide_fight (WIDE NATIVE off / on, fighters identical in all 6000 frames); the profile screen by eye in slide-in, steady and slide-out frames, and no seam where the halves meet.

## 0.99.27
### For players
- **WIDE NATIVE, character profile:** the left column is back to its full length; instead the text box no longer touches it, with the frame's red between them.

### Technical
- Reverts 0.99.26's column cut (`sprite_grid_clip_bottom` and `widescreen_edge_row` 4 removed). `object/sprite_drawers.c` paints only the socket's end between the column and the box (x -16 - 0, rows 183 - 201) red after the widened frame (kind 113 type 1).
- Checked: wide_fight parity and WIDE NATIVE off / on (fighters identical); the profile screen by eye.

## 0.99.26
### For players
- **WIDE NATIVE, character profile:** the left column no longer runs down beside the text box; the box sits inside the frame, with the frame's red at its left as on the right.

### Technical
- `object/sprite_grid.c`: `sprite_grid_clip_bottom` cuts cells (and their texels) below a 2D-library row; `object/sprite_drawers.c` sets it to 183 for the profile screen's left column (kind 113 type 3, drawer 6; `widescreen_edge_row` 4) and paints the frame's socket (x -48 - 0, rows 183 - 201) red after the widened frame (kind 113 type 1).
- Checked: wide_fight parity and WIDE NATIVE off / on (fighters identical); the profile screen by eye.

## 0.99.25
### For players
- **WIDE NATIVE, attract mode:** the scrolling name rows (CHUN-LI, CHARLIE, DHALSIM ...) run to the screen's edges, and the fighters' outlines slide in from the edges instead of being cut at the old 4:3 sides. The name banners above and below the blue close-ups (DEE JAY, EAGLE ...) no longer leave a gap at the left. The flames before M. Bison's entrance fill the screen.

### Technical
- Decompiled sprite drawer 14 (0x0880D468) and the mask layer's cell writer 0x08811FF4 (`object/mask_layer.c`): parity holds in attract_mode (both run there) and the other scenarios.
- WIDE NATIVE: the cell writer takes cells from x -64 to 447 and puts texels left of the 4:3 picture at the end of their row (texels 464 - 511); `object/screen.c`'s text layer draws those in the left margin.
- `object/sprite_drawers.c`: `widescreen_edge_row` 3, `wide_row_wrap` (the last piece's picture before the first, the first's after the last), for the coloured panels behind the name rows (kind 126 type 5, drawer 6); row 1 for the blue screens' name banners (kind 50 type 7).
- `stage/tiles.c` 0x0881AB28: kind 5 (title and attract backdrops, a 16-tile map) at scroll 0 takes the columns past the 4:3 sides round within its 12 (`scene_mirror` 2). `widescreen_edge_copy`: the flame sprites at the ends (kind 52 types 2, 3) drawn again a screen's width over.
- Checked: parity for attract_mode, arcade_match, arcade_ending_full; WIDE NATIVE off / on fighters identical for attract_mode and arcade_ending_full; the screens by eye.

## 0.99.24
### For players
- **WIDE NATIVE, attract mode:**
  - The score ranking's RANKING columns sit at the screen's edges.
  - The green and red speed-line backgrounds behind the fighters' close-ups fill the screen.
  - On the red ones, the STREET FIGHTER ALPHA 3 strip starts at the screen's edge and repeats, and the rows of the fighter's name leave no gap.
  - The earth band behind TRIUMPH / OR / DIE shows its own sky and horizon again (0.99.23 had mirrored it into a V).
  - The faces band behind GO FOR BROKE / FACE TO FACE shows half of another fighter's portrait at each side instead of a mirror image.

### Technical
- `extensions/widescreen.c`: `widescreen_edge_shift` for the ranking (GAME + 0 = 4, kind 35 types 3 / 4) and the red screens' strip (kind 48 type 3, -48 px); `widescreen_edge_copy` for that strip (again at +432 px, its picture being 480 wide) and the faces band (kind 54 type 3 with a 24-column picture, at ±384 px; new `picture_cols`); `widescreen_edge_cells` 16 for the green (kind 127 type 6) and red (kind 54 type 5) backgrounds, no longer for kind 54 type 3; `widescreen_edge_row` 1 for the name rows (kind 48 type 4, drawer 1).
- `object/sprite_grid.c` mode 4: a side the picture already covers to the screen's edge is left alone (the earth band, x -64 - 448).
- Checked: attract_mode parity; WIDE NATIVE off / on fighters identical for attract_mode and super_finish; each screen by eye.

## 0.99.23
### For players
- **Stage Extensions stay on after the KO:** the stage's edges are still drawn at the end of a round, on the win screen and on the continue screen, and in the attract mode's demo fights.
- **WIDE NATIVE: the GAME OVER after finishing Arcade Mode** (with TRY AGAIN) no longer has white bars at the sides; its band runs to the edges like the other GAME OVER.
- **WIDE NATIVE, attract mode:** the QSound screen's background fills the screen, and the banners behind TRIUMPH, BORN, GO, FACE ... and the bands that light up behind the words below run to the edges.

### Technical
- `extensions/stagestrips.c`: one gate, `stage_shown()`, for the strips and the foreground pieces: a stage set up and either the attract demo (GAME + 0 / + 2 / + 4 = 3 / 2 / 1, no fight step there) or the fight step within 2 frames with GAME + 2 / + 4 = 2 / 3 - 4 or GAME + 2 = 3 (win and continue screens). The select, map, profile and VS screens stay out (C29); `stage_strips` passes.
- `extensions/widescreen.c` `widescreen_edge_cells`: GAME OVER's band also at GAME + 0 = 3, + 2 = 0 (the end of an ending); new answer 16 (side cells mirrored, `sprite_drawers.c` → `sprite_grid.c` mode 4) for the QSound screen (GAME + 0 / + 2 = 1 / 7, kind 122) and the attract word screens (GAME + 0 = 2, kind 51 types 0 - 2, kind 54 type 3).
- `object/sprite_grid.c` mode 4, mirroring: side cells alternate mirrored / as they are until the 48-pixel margin is covered (three 16-pixel cells on the QSound screen; two 32-pixel cells as before).
- Checked: parity for attract_mode and arcade_match; WIDE NATIVE off / on fighters identical for attract_mode, arcade_match and super_finish; `stage_strips`; the strips after the KO, on the win and continue screens and in the demo, GAME OVER after the ending, the QSound and word screens by eye.

## 0.99.22
### For players
- **WIDE NATIVE: the super finish's backdrops fill the screen unbroken.** The slanted lines, splash, burst and swirl are drawn slightly larger, centred, so their lines and rings carry on to the screen's sides with no seams or pulled pixels. The vertical lines' second kind now carries on the same way as the first.
- **WIDE NATIVE: the endings' speed-line and vertical-line backgrounds carry on into the sides** instead of showing a strip of the other background, and a scene narrowed to a panel keeps black sides out to the screen's edges.
- **WIDE NATIVE: the ROUND banner's glow fades away on the left** as it does after the D on the right.
- **WIDE NATIVE: the portraits in the dialogue before a rival fight sit right at the screen's edges.**

### Technical
- `object/super_backdrop.c` (0x0880C208): kinds 3 - 5 use `sprite_grid.c` mode 4 (mirrored side cells; kind 4 was taking the edge's colour); the rest are drawn with `sprite_grid_scale` 1.25 about (192, 112) and the new `sprite_grid_scale_all` (the whole scaled cell, not only its margin parts). 32-pixel cells become 40 on whole pixels. Mode 4's strips 1 and 2 are now unused by it.
- `object/sprite_grid.c` mode 2 (ROUND): the left margin gets picture x 300 - 304 (the glow's dimmer step; past 304 is the hole for the 1's bar) drawn 16 wide, then the strip.
- `stage/tiles.c` 0x0881B7C8: with the scroll on 0 or 384 and standing still (or just cut to), the extra columns mirror the 12 columns on screen (`scene_mirror`, `scene_flip` toggling a tile's flip). A scrolling scene still wraps.
- `extensions/widescreen.c`: `dialogue_units()` (64 instead of 48) for the dialogue's portraits and lines; `widescreen_plain_adjust`: on screen 1 (the endings) a plain rectangle of type 1, 96 wide, at the 4:3 side is widened to the 16:9 side.
- Checked: parity for super_finish, arcade_rival_dialogue, arcade_ending_full; WIDE NATIVE off / on fighters identical for all three; every backdrop kind, ROUND, the dialogue and the ending scenes by eye.

## 0.99.21
### For players
- **WIDE FORCED is now called WIDE NATIVE,** in SCREEN SETTING, the settings file and the docs.
- **A new save starts in WIDE NATIVE.** With no `a3mx.cfg` yet, SCREEN SIZE is WIDE NATIVE; an existing save keeps what it has, and `screen_size=WIDE FORCED` in an older `a3mx.cfg` still reads as WIDE NATIVE.

### Technical
- Renamed everywhere but past CHANGELOG entries (code comments, `screensetting.c`'s value name, same length so the green bar is unchanged; `settings.c`, `widescreen.c`'s log, the features list, the launcher, tests, tools, docs).
- `extensions/settings.c`: no `a3mx.cfg` at load sets SCREEN SIZE to WIDE NATIVE (pending until the options block exists); `screen_size` also accepts `WIDE FORCED`.
- `tests/parity.py`, `tools/watch_scenario.py`: `screen_lines()` writes `screen_size=NORMAL` for narrow runs (WIDE NATIVE for `wide`) unless the scenario sets it, so the scenarios still start as before.
- Checked: a run with no save starts WIDE NATIVE and writes it; an old `WIDE FORCED` cfg loads as WIDE NATIVE; screen_setting and save_load parity.

## 0.99.20
### For players
- **Testers: the super finish test is quicker to watch.** `python tools/watch_scenario.py super_finish --wide --super-kind N` now plays a two-player Free Battle and reaches the super in a few seconds.

### Technical
- `super_finish` redone in Free Battle with 2P PRIORITY PLAYER: pad 2 joins on the select screen and confirms its rows, so both fighters are players and stand still; player 2's health set to 1 once the fight starts, then player 1's Shinku Hadoken. The finish is at frames 3855 - 3970 (was 4395 - 4515), and the CPU can no longer parry it.
- `tools/watch_scenario.py`: `--super-kind` forces frames 3830 - 3990 and skips to 3700 by default.
- Checked: super_finish parity and WIDE FORCED off / on (fighters identical); all seven backdrops by eye.

## 0.99.19
### For players
- **`--fast-until N`:** runs the game as fast as it can up to frame N, then at its own speed, to watch a late part of a scripted run without waiting.

### Technical
- `platform/main.c`: `--fast-until N` (as `--fast`, then the wall clock from frame N on; the pacing resyncs as on any switch).
- `tools/watch_scenario.py` (since ad1b601): `--skip-to F` passes it; with `--super-kind` it skips to 80 frames before the forced backdrop (4300 in `super_finish`, where the CPU's health is already 1), so each backdrop is on screen after a few seconds.
- `docs/TESTS.md`: "Watching a scenario".

## 0.99.18
### For players
- **WIDE FORCED: the super finish's slanted lines carry on in the same direction into the sides,** instead of turning back on themselves.
- **WIDE FORCED: the splash backdrop takes its background colour to the sides** like the burst and swirl, with no pattern carried on.

### Technical
- `object/sprite_grid.c` mode 4, `sprite_grid_wide_strip` 2: the side cell drawn again a cell out and two, moved 16 up or down a cell along the lines' slope (`sprite_grid_wide_dy`), over a margins-only pass of the edge's colour (`sprite_grid_margins_only`) for the band left open; `object/super_backdrop.c`: kinds 1 (rising, -16) and 2 (falling, +16) use it, kind 6 the edge's colour.
- Checked: super_finish parity and WIDE FORCED off / on (fighters identical); all seven backdrops by eye.

## 0.99.17
### For players
- **WIDE FORCED: the super finish's burst and swirl backdrops carry their colour out to the screen's sides,** instead of repeating a piece of the pattern there. The speed lines, zig-zags, vertical lines and splash keep their pattern carried on, mirrored.

### Technical
- `object/super_backdrop.c` / `sprite_grid.c`: for kinds 0, 7, 9, 10 (picture 39, the burst) and 8 (51, the swirl) mode 4 stretches the edge cell's texel column two in from the edge (`sprite_grid_wide_strip`) instead of mirroring the cell.
- Checked: super_finish parity and WIDE FORCED off / on (fighters identical); every backdrop kind by eye (BATTLE + 43 poked to 1, 2, 3, 5, 6, 7, 8 in a test run).

## 0.99.16
### For players
- **WIDE FORCED: WIN, PERFECT and the results panel fly across the whole screen and stop at its edges.** WIN and PERFECT (with WIN's band) come in from the right side and stop at the left edge; the COMPLETE / MAX COMBO / VITAL / TIME VITAL / SCORE panel comes in from the left and stops at the right edge, its bars reaching it.

### Technical
- `extensions/widescreen.c`: at the end of a round won (fight step 3), WIN (kind 0 type 2) and PERFECT (kind 1 type 12) are moved by an amount that follows their place (-48 at their stop, x 112, 0 at the middle, +48 at x 384), the panel (kind 1 types 9, 15) likewise (-48 at x -144, +48 at its stop, x 240); WIN's band and the panel's bars (plain rectangles of the third list, types 1 and 2 - 6) move with them.
- Checked: arcade_match parity; wide_fight and arcade_match with WIDE FORCED off and on (fighters identical); the round's end by eye.

## 0.99.15
### For players
- **WIDE FORCED: the dialogue before a rival or final fight uses the whole screen.** Both portraits sit at the screen's edges, and each line moves with its speaker's portrait.
- **WIDE FORCED: the character select's bottom STREET FIGHTER ALPHA3 ticker no longer pops in** at the left side.
- **WIDE FORCED: a KO by a super combo shows the green speed lines right across the screen.**

### Technical
- Decompiled: 0x0880C1AC / 0x088108D8, the text pass's three text boxes (the dialogue's lines; new `object/text_boxes.c`), and 0x0880C208, the super finish's backdrop (new `object/super_backdrop.c`).
- `extensions/widescreen.c`: the dialogue portraits (kind 14 type 6, fight step 3) move away from the middle, `widescreen_text_shift` moves a text box with the portrait in its half; the select screen's bottom ticker (kind 16 type 5, y 24, scrolling left, period 480) is drawn again at ±480.
- `object/sprite_grid.c`: mode 4, cells at the 4:3 picture's sides mirrored out then repeated (the backdrop is two 6 × 7 pictures of 32-pixel cells).
- New scenarios `arcade_rival_dialogue` (the first fight's number held at 8 from before the map, so it is met as the rival fight) and `super_finish` (a KO by Ryu's Shinku Hadoken).
- Checked: arcade_fight, arcade_match, arcade_rival_dialogue and super_finish parity (the three new functions ran); wide_fight and both new scenarios with WIDE FORCED off and on (fighters identical); each screen by eye.

## 0.99.14
### For players
- **WIDE FORCED: the profile screen's name plate box moves out with the header,** instead of a dark blue box being left behind where it was.
- **WIDE FORCED: the box behind the profile text reaches the ends of the text box.**
- **WIDE FORCED: the profile screen's side columns are layered as in WIDE:** the right one is no longer cut short at the top.

### Technical
- `object/draw_kinds.c` / `extensions/widescreen.c`: `widescreen_plain_widen` became `widescreen_plain_adjust` (left and right moves for a plain rectangle of the third object list): on the profile screen the name plate's box (type 0, 112 wide) moves 48 left and the translucent box behind the text (type 1, 304 wide) widens 48 each side; picked by size, as the map screen (the same step) has a full-width bar of type 0.
- `object/sprite_grid.c`: in the widened drawing, a picture narrower than the 4:3 picture moves whole with the half its middle is in; the side columns' row clip (0.99.13) dropped, now that the columns sit at the edges clear of the plate and the box.
- Checked: arcade_fight and arcade_match parity, wide_fight (fighters identical); the profile and map screens by eye.

## 0.99.13
### For players
- **WIDE FORCED: the continue screen's countdown bar no longer flickers at the left side.**
- **WIDE FORCED: the profile screen before the map reaches the screen's sides.** The header with the name plate and the text box at the bottom stretch out to the edges (the text stays centred, the portrait where it was), and the scrolling side columns sit at the edges.
- **WIDE FORCED: the win screen's side columns sit at the screen's edges,** with the band under the quote reaching them.

### Technical
- Decompiled: sprite drawers 3 (0x0880C70C, one picture by the sprite's +84) and 6 (0x0880CAA8, pieces from the sprite's +156), in `object/sprite_drawers.c`.
- `object/sprite_grid.c`: the continue bar's left side (mode 3) is drawn for each row's first drawn cell and for bars starting at the left edge (its frames are 23 or 24 cells, so the width test dropped it every few frames); a widened pass (`sprite_grid_half`: left half 48 left, right half 48 right, the middle cell's first texel column stretched between) for the profile screen's header and text box; a row clip (`sprite_grid_clip_top` / `_bottom`) keeps its side columns between the two.
- `extensions/widescreen.c`: the profile screen's columns (kind 113 types 3, 4) and the win screen's (kind 18 types 4, 6) move away from the middle; `widescreen_widen`, `widescreen_clip` and `widescreen_plain_widen` (the win screen's band, a plain rectangle in `object/draw_kinds.c`); stand-ins in `extensions_off.c`.
- Checked: arcade_fight and arcade_match parity (both new drawers ran), wide_fight and wide_walls (fighters identical); the continue bar frame by frame, the profile and win screens by eye.

## 0.99.12
### For players
- **WIDE FORCED: the ROUND banner's glow tapers away on the left** as it does on the right after the D, instead of stopping short.
- **WIDE FORCED: FIGHT's translucent box has no notches in the sides.** The sides now repeat a clean part of the box instead of the edge of the T.
- **WIDE FORCED: the NEXT STAGE ladder sits at the screen's edge,** with the next opponent's name: on the right when player 1 started the game, on the left for player 2.
- **WIDE FORCED: the map's beam carries on to the screen's sides** along its own edges, instead of being cut off where its picture ends.
- **WIDE FORCED: the GAME OVER band runs right across the screen,** with no white squares at the sides.
- **WIDE FORCED: the continue screen's countdown bar reaches the left side,** with the numbers before it shown there too (down to 0.0).

### Technical
- `object/sprite_grid.c`: the band continuation reworked. The right side (and the left, for FIGHT's box and the GAME OVER band) is one strip of a single texel column from the row's last cell; the ROUND band's left side mirrors its glow after the D (picture x 284 - 304); the continue bar fills the left side only. Only pictures at least 384 wide are continued. A scaled pass (`sprite_grid_scale`) draws a picture's cells scaled about a point, cut to the margins.
- `object/sprite_drawers.c`: the continue count (kind 13 type 3) gets the numbers before its first, from its digit pictures (572 + d, 582 + d); the map's beam (kind 19 type 2 on anchor 3) is drawn three times scaled (2.6, 1.9, 1.4) about its place behind itself, in the margins only.
- `extensions/widescreen.c`: the map screen's ladder (kind 19 types 4 - 6) moves away from the middle like the portrait; GAME OVER (GAME + 2 = 4, kind 13 types 4 - 6) and the continue screen (GAME + 2 = 3) are picked; `widescreen_beam` added (stand-in in `extensions_off.c`).
- Checked: arcade_fight and arcade_match parity; wide_fight and wide_walls (fighters identical); ROUND, FIGHT, the map with each player side, GAME OVER and the continue countdown by eye.

## 0.99.11
### For players
- **WIDE FORCED: the A, X and V before the fight keep their shape.** The ISM letter that wipes in after the VS screen was stretched out to the screen's sides (too wide, the wrong angle); it is drawn as on the 4:3 screen now, with its colour filling the sides.
- **WIDE FORCED: FIGHT's translucent box runs right across the screen** as it flies in and settles, instead of stopping at the old 4:3 edges.

### Technical
- `object/shapes.c` (0x0880E434): no longer moves corners at the 4:3 edge out by 48 (0.99.8), which changed the slopes; a quad whose edge lies on or past the 4:3 picture's side gets a rectangle in its colour from that edge to the screen's side, as tall as the edge.
- `object/sprite_grid.c`: the band continuation repeats each row's last cell to the right and its first position's left, so a band that has moved off centre (FIGHT's after-image trails it) gets no overlapping copies. `widescreen_edge_cells` now says which pieces: the ROUND banner's solid ones, FIGHT's half-alpha after-image.
- Checked: arcade_fight parity, wide_fight (fighters identical); the A by eye, and the X and V with their shapes swapped in for a test run; ROUND and FIGHT frame by frame.

## 0.99.10
### For players
- **WIDE FORCED: EDIT MODE's green background and WORLD TOUR's blue background behind the map reach the screen's sides,** shaded like the rest instead of a flat dark strip (EDIT MODE) or nothing (WORLD TOUR).

### Technical
- `platform/ge_soft.c` `wide_margins`: a rectangle across the 4:3 picture whose edge is shaded top to bottom gets its margin pieces as shaded strips with the edge's top and bottom colours, instead of one sprite in the bottom colour. Renderer only, both the software and OpenGL paths (OpenGL not checked here).
- Checked: mode_tour parity; EDIT MODE's status screen and the WORLD TOUR map by eye, against the build before.

## 0.99.9
### For players
- **WIDE FORCED: the ROUND banner's dark band runs right across the screen,** instead of stopping at the old 4:3 edges.
- **WIDE FORCED: the FIGHT FIGHT rows and the scrolling KNOCK OUT row reach the screen's sides.** The KNOCK OUT row used to stop short of the right edge.

### Technical
- Decompiled: three sprite drawers, 0x0880C4C8 (one picture), 0x0880C598 (a set of pieces from 0x092E3E98) and 0x0880CF60 (a set of pieces at one place, half-alpha after-images), new `object/sprite_drawers.c`.
- WIDE FORCED: `widescreen_edge_cells` (the ROUND banner, kind 0 type 0): the band's right-end cells are drawn again out into both margins (`sprite_grid.c`; the left end has the R in it). `widescreen_edge_row` (FIGHT FIGHT kind 1 type 4, KNOCK OUT kind 0 type 3): a row of one piece at even steps gets one more piece past an end that has reached the 4:3 edge, so nothing is added ahead of a row still sliding in. FIGHT itself and its after-image are left as they are. Only the drawing changes.
- `platform/extensions_off.c`: stand-ins for `widescreen_edge_shift` / `_copy` (since 0.99.4) and the two new ones, so a build without the extensions links.
- Checked: arcade_fight and arcade_match parity (all three drawers ran), wide_fight and wide_walls (fighters identical); ROUND, FIGHT, KO, PERFECT and ROUND 2 by eye.

## 0.99.8
### For players
- **WIDE FORCED: the green wipe from the VS screen into the fight covers the whole screen,** and the green shapes that fade into the stage reach the sides too.
- **WIDE FORCED: no stray "F" over the VS screen.** The word FIGHTER waits just past the old 4:3 edge to slide in; it is no longer drawn there.

### Technical
- Decompiled: 0x0880E434, the flat-colour shapes of the wipes (new `object/shapes.c`), and 0x0880D848 / 0x0880D8C8 / 0x0880D988, the single object at 0x092E5100 and its six slots, which carries the logo between the VS screen and the fight (new `object/single_draw.c`).
- WIDE FORCED: a shape corner on or past the 4:3 edge goes 48 further out; a slot's picture waiting at x 384 or more is not drawn until it moves in. Only the drawing changes.
- Checked: arcade_fight, attract_mode and arcade_match parity (all four functions ran), wide_fight (fighters identical), the wipe by eye.

## 0.99.7
### For players
- **WIDE FORCED: the map screen's portrait is right when player 2 starts.** With PLAYER SIDE 2P (Arcade or Dramatic Battle), the portrait sits at the right edge instead of being pulled left over the banner.

### Technical
- `extensions/widescreen.c`: the map screen's portrait tree (kind 19 type 7) moves away from the middle like the others, rather than always left.
- Checked: wide_fight (fighters identical); the select, ISM, profile and map screens with `player_side=2P` by eye.

## 0.99.6
### For players
- **WIDE FORCED: the character select's chosen MODE and the ISM select move out with the rest.** The highlighted MODE box, the ISM column's chosen panel and its big letter, and the name under the portrait now sit at the edge with the boxes beside them.

### Technical
- `extensions/widescreen.c`: on the select screen, select task type 6 (the chosen MODE / ISM panel and the name strip, with its linked parts) and the ISM panel's parts (object kind 33) move away from the middle by the margin too.
- Checked: wide_fight (fighters identical); MODE, ISM and SPEED select by eye.

## 0.99.5
### For players
- **WIDE FORCED: the character select's name ticker no longer pops in.** The scrolling "1P RYU" strip at the top now runs right across the screen, instead of each word appearing whole in the right margin.

### Technical
- The ticker (select task kind 16 type 5, the top one) is one picture that scrolls and jumps back by 320 when it has gone that far, too short for the 16:9 sides. In WIDE FORCED it is drawn one period further each way as well (`widescreen_edge_copy` in `extensions/widescreen.c`, now up to two extra drawings per sprite tree in `object/sprite.c`).
- Checked: wide_fight (fighters identical) and the ticker frame by frame by eye.

## 0.99.4
### For players
- **WIDE FORCED: portraits sit at the screen's edges.** On the character select screen the portrait, the small character and the MODE / SPEED boxes move out to the edge; on the VS screen each portrait moves to its edge (whichever side started, and Dramatic Battle's two as well) and the blue name bars reach the edges; on the map screen (NEXT STAGE) the portrait moves to the left edge to match.

### Technical
- `object/sprite.c`: the sprite drawer asks `widescreen_edge_shift` (in `extensions/widescreen.c`) for each sprite tree it draws, and moves the tree's drawing by that much (a linked sprite takes its parent's); `widescreen_edge_copy` asks for a second drawing moved out. Only the drawing moves: game memory keeps the 4:3 places.
- Which sprites: the select tasks (object kind 16) of types 2 - 4 on the select screen; on the VS screen (kind 14, battle screen step 2) the portraits (type 0) moved, the name bars (type 2) drawn again moved out; on the map screen (kind 19, step 1) the portrait's tree (type 7, with the scrolling banner text). Each moves away from the middle by the 48-pixel margin.
- Checked: wide_fight (fighters identical), arcade_fight and select_choices parity, and by eye the select, map and VS screens in Arcade and the VS screen in Dramatic Battle.

## 0.99.3
### For players
- **WIDE FORCED: the MODE MENU, OPTION MODE and FREE BATTLE MODE menus fill the screen.** The tiled ALPHA-logo background now continues into the left margin, and the menus' title bars reach both sides.

### Technical
- `object/draw_kinds.c` (kind 2, a set of parts): with `rt_widescreen`, an object whose parts repeat across (the same image again one width on: the menus' 256 x 256 logo tiles from x 0 to 768) but stop short of the 16:9 sides gets more copies of its end columns until they reach x -48 / 432. The scrolling bands already reached and are left alone.
- `platform/ge_soft.c`: a full-width draw now gets its margin piece on each side it stops short of, even if the other side already reaches the screen's edge (the menus' title bars ran from the 4:3 edge to past the right of the screen).
- Checked: wide_fight (fighters identical), mode_menu and option_tour parity, and the three menus by eye.

## 0.99.2
### For players
- **WIDE FORCED: screens drawn across the whole 4:3 picture now reach the sides.** The title screen's light blue now fills the screen, and full-screen fills, fades and flashes cover the margins too. Banners, wipes, the VS screen's bars and the menus' backdrops are drawn in pieces and are still to do.

### Technical
- `platform/ge_soft.c` (both renderers): with `rt_widescreen`, a single sprite or quad spanning exactly the 4:3 picture (x 48 - 432 of the 480-wide screen) is followed by a piece in each margin repeating its edge texel column (or flat colour). Nothing in game memory changes, so the WIDE FORCED fighter check is unaffected. Checked by eye on the attract sequence and Arcade's first fight (software renderer; the OpenGL path draws the same pieces through `ge_gl_draw`, not checked here).

## 0.99.1
### For players
- **WIDE FORCED is now entirely the project's own code: Rickow's 16:9 patch is no longer downloaded.** Extracting your disc fetches nothing for it any more, and removes the old download from `work/`. WIDE FORCED looks and plays as before.
- **WIDE FORCED: the endings' backgrounds continue properly into the left margin.** Where they used to show a strip of the row above (Ryu's ending), the extra columns now wrap round within their row, as the fight backgrounds have since 0.97.0.

### Technical
- Dropped: the patch's pin and download (`tools/data/pins.json`, `extract_assets.py`, `downloads.py`, the launcher's dependency list), `work/boot.widescreen.json` and the translator's switched overlay (`psp_analyze.py` / `psp_recomp.py`: no more `rt_widescreen` in the generated code, no `rt_widescreen_available`), `tools/data/widescreen_more.json`, and `--no-widescreen` (`build.py`, `extract_assets.py`, the launcher).
- The WIDE scenarios (`wide_fight`, `wide_walls`, `wide_left_wall`) are now a WIDE FORCED check: with a `wide` line, `tests/parity.py` plays the scenario twice with the decompiled code, WIDE FORCED off and on, and the four fighters must be the same in every frame (`--hash-log` writes their hash as a fourth column). What WIDE FORCED draws is checked by eye.
- `stage/tiles.c`: 0x0881B7C8's extra left columns wrap round within the row (seen in Ryu's ending).
- Checked: all 75 tests (ctest), and the WIDE FORCED check on network_match and arcade_ending_full as well.
- Docs: WIDESCREEN.md rewritten for the native WIDE FORCED; TESTS.md, BUILDING.md, PSP_PROGRAM_MAP.md, ROADMAP.md, DEPENDENCIES.md and ACKNOWLEDGMENTS.md updated.

## 0.99.0
### For players
- Nothing changes in play. All of the 16:9 patch's work is now the project's own code, a step towards dropping the patch download.

### Technical
- Native now, each choosing its width with `rt_widescreen` as the patch does: the third tile drawer 0x0881B7C8 (`stage/tiles.c`, used only by the endings); the screen's setup, extra text layer and pillarbox (0x0880B928, 0x0880F160, 0x0880F390, new `object/screen.c`); the 32-pixel paletted sprite cells 0x08811BCC (`sprite_grid.c`, the last of the four cell drawers); the network menus' eleven rows (new `network/menu.c`); the network screens' panels and message boxes, and two states of Cammy's CPU routine that pass the screen's width on (new `network/screens.c`).
- New scenarios `network_match` (NETWORK > VS MODE against the virtual peer, through the menus and a whole match) and `arcade_ending_full` (Ryu's ending, not skipped). Checked with them, arcade_fight, wide_fight, wide_walls, wide_left_wall, attract_mode (WIDE FORCED off and on), mode_tour, free_battle and free_battle_team.
- Not reached by a scenario yet, though native: 0x088D6D3C, three network menu rows and the two Cammy states.

## 0.98.0
### For players
- **WIDE FORCED: stage props that used to appear from nowhere are now always there.** To save sprites, the arcade moves some props between two places (Dan's crow between a perch and the sign, and props on Ken, Akuma, Adon, Sagat, Sakura, Zangief, E. Ryu, Mika, S. Akuma and Karin 2's stages), timing the move just off the 4:3 screen. On the wider screen you could see it happen. Each now has a twin, so both places show their prop while in view.

### Technical
- New `extensions/widescreen_twins.c` around the eleven "swap at the line" kinds (`stage/objects.c`, now `SFA_FUNC_WRAPPED`): in WIDE FORCED a copy of the object in memory of its own takes the other look, steps its animation when the object's would, and is queued for drawing after it. Checked with wide_fight and arcade_fight (NORMAL byte for byte as before), and on eleven stages by eye.
- To come back to: on Akuma's, E. Ryu's and S. Akuma's stages the orange Stage Extension placeholders don't meet the black strips in the last background layer (maybe that black is the layer's own art).

## 0.97.0
### For players
- **WIDE FORCED: objects at the screen's edges no longer pop in.** Stage props and other objects were only drawn once they were near the old 4:3 picture, so they appeared suddenly in the extra 48 pixels at each side (the people in the background on Guy's stage). They are now drawn as soon as they reach the wider view.
- **WIDE FORCED: no more misplaced art at the left edge of the tile backgrounds.** The leftmost columns could show a strip of the background one row out of place (the panel behind Sakura in the attract sequence, and the far background at the left wall on Ryu's, Akuma's, Sagat's, Dhalsim's, Balrog's, Cammy's, Karin's and Hawk's stages). They now continue the right row.

### Technical
- `object/frame.c`: 0x08821424 and 0x088214E8 (an object drawn when on the screen, and with its own margin) take in 48 more pixels each side with `rt_widescreen`; five words in `widescreen_more.json` do the same for the reference run.
- `stage/tiles.c`: the patch starts each row 2 (3) tiles further left by moving the map pointer back, reading the row above when the first map column was smaller; it now wraps round within the row. The patch's jumps in 0x0881AB28 / 0x0881B268 go to added code of the project's own (`caves` in `widescreen_more.json`, at 0x08800100: translated only, never in game memory); `psp_analyze.py` takes added code and patch-word overrides, each checked.
- New scenario `wide_left_wall` (Ryu's stage, camera at its left limit). Checked with wide_fight, wide_walls, wide_left_wall and arcade_fight.
- Found, not fixed yet: Dan's crate post is a "swap at the line" object (kind 31) that changes props just off the 4:3 screen, so the change shows in 16:9 (eleven kinds do this); the MODE MENU's left side is the menu's own objects, not tiles (WIDESCREEN.md).

## 0.96.1
### For players
- **The art pack is marked experimental** (Build art pack (experimental), under advanced settings). It gives screenshots of each stage at its walls, whole and one layer at a time, not each stage layer whole as its own picture yet. That is on the roadmap.

## 0.96.0
### For players
- **A simpler Build / Run tab.** The launcher now opens with only the settings most players need: full screen, logging, renderer, resolution and GPU, and Run builds Release and starts the game. **Show advanced settings** (off every time the launcher opens) brings back the presets, the build's own choices (Extract the disc again, Build art pack, the tests, Run after building) and the extra game options.
- **Record Output and Play Output** (advanced presets), for chasing problems. Record Output records the whole session (every button on every pad, the starting settings and save) into your logs folder's `recordings/`, and can also log every read and write of the game memory you choose (addresses or ranges, in an **Edit...** window; forgotten when the launcher closes). Play Output plays a recording back on any PC with the same disc, from the recording's own copy of the settings and save, and says whether it matched the recording all the way or from which frame it didn't.

### Technical
- New `src/platform/record.c`: `--record FILE` / `--replay FILE` (any build). Each frame's `host_poll` pads, the pads plugged in (`host_pad_connected`, which a replay answers) and F2; `a3mx.cfg` / `a3mx.dat` as they started; features, `--p2` / `--p3`, frame timing; the date (`sceKernelLibcTime` / `gettimeofday` give the recorded start plus virtual time while recording or replaying); a memory hash every 60 frames, compared by the replay. A replay writes the recording's files to `<recording>-replay/` and stops at its last frame.
- `--watch` repeatable (up to 16 ranges), `--watch-log FILE`; the hits go to `watch.log` in the logs folder with their frame. Builds without watchpoints now warn when given `--watch` options.
- New `watch` preset (`build-watch/`, `A3MX_WATCH`, no tests); `build.py --preset watch`. Record Output and Play Output build it.

## 0.95.0
### For players
- **WIDE FORCED: fighters are drawn in full at the left wall.** Body parts that went past the old 4:3 edge, such as Blanka's shoulder and back or Zangief's back when walking back into the wall, were cut off and flickered there. They are now drawn into the extra 48 pixels on the left. The walls don't move: only what is drawn changes, so every move and its timing stay as they were.

### Technical
- The cause: the sprite cell drawers that 0x0880E150 chooses from skip a cell starting left of minus its width. The 16:9 patch widens that limit to -96 in 0x08811BCC only, and a fighter's parts go through the other three as well (found with `--watch-pc` on each drawer's left test).
- New `src/core/object/sprite_grid.c`: 0x08810D84 and 0x088112A8 (16- and 32-pixel cells from the picture cache) and 0x088117D0 (16-pixel cells, each with its own palette), with the left limit at -96 while `rt_widescreen` is set.
- New `tools/data/widescreen_more.json`: the project's own words on top of the patch's overlay, merged by `tools/recomp/psp_analyze.py` (which stops if a word it replaces isn't the one listed), so the reference run matches the decompiled code with WIDE FORCED on. The first three are the same left limit in those drawers. CMake retranslates when it changes; no new extraction needed.
- New scenario `wide_walls`: Free Battle in WIDE FORCED, Blanka against Blanka, player 1 at the left wall and then walking right. Checked with difftest, wide_fight, wide_walls, arcade_fight and arcade_other_fighter.

## 0.94.0
### For players
- **Art pack for painting the Stage Extensions.** The launcher's new **Build the art pack** option (in Build) renders every stage at its walls from your own disc and puts the pictures together with all the placeholders to paint (the layer strips and the foreground objects' pieces), one folder per stage with a README, in `art-pack/` and a zip you can hand to an artist. It's made from your disc's art, so it's for you and your artist, not for publishing.

### Technical
- New `tools/art_pack.py` (`reference/` from `dump_stage_references.py`, `paint/` copied from `assets/extensions/stages/`); `tools/build.py --art-pack` runs it after building; the launcher's one-go checkbox passes it. Pillow is installed with pip if missing. `art-pack/` added to .gitignore.

## 0.93.0
### For players
- **More of the game decompiled: the select, map, profile and VS screens' backdrops** (the stage's layers shown behind those menus) now run as hand-written C. With this, every part of the stage code the game runs is native.
- **Reference pictures for painting the stage extensions:** `python tools/dump_stage_references.py` writes, from your own disc, what each stage looks like at both walls in WIDE FORCED (whole, and each layer on its own with transparency) into `assets/us/stages/<stage>/widescreen_reference/`. They stay in your assets folder (git ignores it) and are never committed.

### Technical
- `background.c`: 0x0881A040 (select screens), 0x08819144 (VS), 0x0881CFD0 (a 160 × 32 picture). `setup.c`: 0x0881CBDC (map), 0x0881CD20 (profile). Checked with arcade_match, boot_to_select, free_battle, select_choices and mode_tour (all five run in them).
- New `tools/dump_stage_references.py` (the runs of `measure_stage_layers.py`): 4 frames per stage and each layer alone.
- Left translated in the stage area: 0x0881A2C8 / 0x0881B7C8, which no scenario reaches.

## 0.92.0
### For players
- **Stage Extensions (WIDE FORCED): placeholders for the objects in front of the fighters.** Ryu's straw, Adon's leaves, Sodom's crossing object, Guy's, Sagat's and Rolento's pieces along the bottom, Dan's, Cody's, Guile's and Hawk's foreground objects are sprites, not layers, so the layer strips couldn't extend them. Each now has a yellow placeholder at each side, pinned to the object so it moves with it, and drawn in front of the fighters like the object. These are placeholders only: an artist paints them (STAGE_EXTENSIONS.md says how).

### Technical
- `src/extensions/stagestrips.c`: `stagestrips_object` around the sprite drawer 0x0880C308 (`object/sprite.c`, now `SFA_FUNC_WRAPPED`) draws a foreground stage object's pieces (`widescreen_object_<kind>_<type>_<side>.png`, placed by `widescreen.json`) right after it, from its position as the sprite drawer works it out, at its depth; packed into the strips' texture below the strips. `A3MX_OBJ_MEASURE` hides an object in chosen frames and logs every foreground object's position, for the new `tools/measure_stage_objects.py`, which wrote the 42 pieces.

## 0.91.0
### For players
- **Stage Extensions (WIDE FORCED): the strips stay joined to their layers.** Each strip now moves exactly with the art it extends. Before, most strips followed a slightly different scroll value than the art and slid against it by up to about 10 pixels as the camera left the wall. Ryu's and Chun-Li's stages, whose top rows move separately, get the two parts of each strip placed separately. Checked on every stage with the camera at the wall and 16, 32 and 52 pixels away.
- **More of the game decompiled: the camera and the stages' own animation.** The camera's per-frame update (every layer following the fighters, the floor's perspective, and the stage-specific movement on Gen's, Vega's, Guile's, Karin's, Chun-Li's, Zangief's, Rolento's and Bison's stages), each stage's setup, and the palette and tile animators it creates (flickering lights, Ken's crowd, Balrog's casino, Honda's bath and the rest) now run as hand-written C. Nothing changes in play.

### Technical
- `src/extensions/stagestrips.c`: strips are placed by the scroll the layer's drawer actually uses (0x0881B268's record: column + part-tile, unwrapped near +18; BISON's pictures: the travel / half of +8 less +16), and a split layer (kinds 0xA0 / 0xA4) draws its strip rows per band, the top band at `"layer1_<side>_top"`. In measuring mode the extension logs that scroll each frame; `measure_stage_layers.py` uses it instead of +18. Re-measured (227 placeholders, unchanged images).
- New `src/core/stage/camera_frame.c` (14 functions): 0x0881D1A8 and its layers 0x0881D244 / 0x0881D960 / 0x0881DC6C, the lean 0x0881D348, placing 0x0881ECA4, following 0x0881E1B4 / 0x0881E0D8, and the stage-specific 0x0881D4D8, 0x0881D588, 0x0881D5F4, 0x0881D6EC, 0x0881D768, 0x0881D8A8.
- New `src/core/stage/animators.c` (17 functions): the map animators 0x08819A68 / 0x08819C2C / 0x08819C74 and the palette animators 0x0881F1FC, 0x0881EEF4 ... 0x0881F5D4, 0x0881EBA8.
- New `src/core/stage/setup.c` (29 functions): 0x08819A20 and the 23 stages' setups (table 0x08A08A14), 0x08819984, the fighters' palettes by layout 0x0881F6A0 / 0x0881F794, the select / VS backdrop palettes 0x0881CE4C / 0x0881CEF4.
- **Build: `-fno-strict-aliasing`** for every C file (not MSVC). Decompiled code reads and writes the same game memory through pointers of different sizes (a 16.16 word and its whole-pixel halfword); the compiler was free to reorder those, and SODOM's crossing object (0.89.0) read its x before the store that moved it, one frame late at the stage's edge. It only showed in a build whose optimisation happened to reorder (the new stage scenario run for 0.91.0).
- Left translated in the stage area: 0x0881CBDC, 0x0881CD20, 0x0881CFD0, 0x08819144, 0x0881A040 (the select / VS screens' backdrops; no fight reaches them) and 0x0881A2C8 / 0x0881B7C8 (no scenario reaches them).

## 0.90.0
### For players
- **More of the game decompiled: the stage backgrounds.** The routine that draws a stage's scrolling layers each frame, every layer drawer it picks from (including Ryu's and Chun-Li's lagging top rows, Guile's strips and Bison's mountains and rippling sky) and the floor band along the bottom of most stages (with Charlie's and Bison's own) now run as hand-written C. Together with 0.89.0's objects, everything a stage draws in a fight is native. Nothing changes in play.
- **Stage Extensions (WIDE FORCED): no gaps at the edges.** Every layer of every stage now has a placeholder strip at both walls where its art stops short of the edge (227 placeholders, up from 158), each continuing its own layer in whole rows and reaching 8 columns under that layer's art and 8 past the screen's edge, so there is a small overlap instead of a gap at any camera position. Bison's sky and mountains have strips for the first time (they were drawn by routines the extension couldn't join). Placeholders only: nobody has painted them yet.

### Technical
- New `src/core/stage/background.c`: the master 0x08819CFC (layer 1, the stage's own routine, layer 0, layer 2 by layer 1's +123 bits, the drawer chosen by the layer's kind), the 16-pixel record builders 0x0881A8E0, 0x0881BD28 (kinds 0xA0 / 0xA4 split into two calls of 0x0881B268) and 0x0881C354, and the picture layers 0x08819498 / 0x0881C0C8. New `src/core/stage/floor.c`: the floor band 0x0881C5A8 and CHARLIE's 0x088196DC and BISON's 0x0881C82C. All 34 `stage_objects_*` scenarios pass.
- `stagestrips.c` joins the two picture drawers (`SFA_FUNC_WRAPPED`, `stagestrips_picture`) and takes strips 48 - 64 wide.
- `tools/measure_stage_layers.py`: placeholders are 64 × 288 (`OVERLAP` 8 under the art, `OUTER` 8 past the edge), filled in whole rows grown by 2 (`VPAD`): every row where the layer's art runs into the edge (`continued()`), plus every row of an unexplained gap (`attribute()`, as before). Re-measured all 38 stages.
- Measured what is drawn in front of the fighters (STAGES.md): no layer or floor band (depths 5, 9, 13; the fighters 16). The 2D library sends each frame's draws by depth 0 - 23 in submission order (0x089E4408), so only stage objects in sprite layer 0 at depth 16, queued after the fighters, are in front: RYU's straw, ADON, SODOM, GUY, SAGAT, DAN, ROLENTO, CODY, GUILE and HAWK have some.

## 0.89.0
### For players
- **More of the game decompiled: every stage's objects.** The animated and moving pieces of the stages that are not their scrolling layers (Ryu's straw, Ken's and Chun-Li's crowds, Guile's helicopter, Bison's storm and falling leaves, Balrog's casino, Fei Long's crowd, Honda's figure and the rest: 34 stages) now run as hand-written C. Nothing changes in play.
- **Balrog's stage:** its animated background tiles no longer write to the wrong place in memory (a slip in the tile drawer decompiled in 0.49.0, only reached on that stage).

### Technical
- New `src/core/stage/objects.c` (92 functions): the maker 0x0884AE40, which reads each stage's list of 16-byte records (table 0x08A143D8: kind, type, layer, camera, x, y...) into pool-5 objects, and the routines of every kind the stages use (table 0x08A101F0), most of them one of four shapes (*follow*, *plain*, *pause at the line*, *swap at the line*) with their own tables and values, the rest of their own (GUILE, BISON, BALROG, FEI LONG, HONDA, KARIN, SODOM...), with their helpers. Also `object_move` 0x08821044 (`object/motion.c`) and an empty function 0x0884D0B4.
- `src/core/stage/tiles.c`: 0x0881B268's animation for kind 0x85 keeps its countdown and frame in layer 0's record (0x092E8800 + 60 / + 131) as the original does; since 0.49.0 it used 0x092F8800. Found by the new BALROG scenario.
- `tools/stage_objects.py`: lists every stage's object records (`--stage`, `--markdown`, `--json`). STAGES.md has a section on stage objects; PSP_PROGRAM_MAP.md the record format and the kinds.
- 34 new parity scenarios `stage_objects_01_ryu_1` ... `stage_objects_34_hawk`: a Free Battle on each stage into 600 frames of the fight.
- `--reference-only` takes up to 4096 addresses (it silently used only the first 64, so bisecting with a long list gave wrong answers).

## 0.88.2
### For players
- **Stage Extensions' placeholders re-measured:** the coloured edge strips are now on the plane each gap really continues (on RYU 1 the sky's strip is the sky layer's, for one). The edges they cover are the same; a strip painted from now on joins the right layer. If you have painted one, it is kept as it is.

### Technical
- `tools/measure_stage_layers.py` turns the `stage_extensions` feature on for its runs (`--features default,stage_extensions`). Since 0.60.0 made it a feature, off by default, the measuring hook never ran, so a re-measure found nothing and would have removed every unpainted placeholder. A run where no layer was measured now stops before writing anything.
- Re-measured all 38 stages (158 placeholders, none painted). The layers' depths are now read in the fight itself (0.88.1's gate): before, they were read on the screens before the fight, which use the same layers for their own backdrops, and STAGE_EXTENSIONS.md said layer 2 was always the farthest and layer 0 the nearest. It depends on the stage (RYU 1: 0 sky, 2 castle, 1 floor; ADON: 2, 1, 0). Table and text updated.
- `src/platform/main.c`: "extensions: off" was logged on every run without `--filter` (its `else` belonged to the wrong `if`).

## 0.88.1
### For players
- **Stage Extensions: no more coloured strips on the select, map, profile and VS screens.** With WIDE FORCED and Stage Extensions on, the next stage's edge strips (the coloured placeholders) were drawn in the middle of the character select screen, Arcade's world map, the fighter's profile and the VS screen. They now appear only in the fight itself.

### Technical
- `src/extensions/stagestrips.c`: the fight gate also needs the battle task at its fight screen (GAME + 2 = 2, GAME + 4 = 3). The select screen (GAME + 2 = 1) and the map, profile and VS screens (2 / 1 - 2) run inside the same per-frame fight step (0x08823674) and draw their backdrops with the stage's camera layers, so the timestamp alone passed (CONCERNS C29, first half). Checked in Arcade, Free Battle and Training.
- `stage_strips` test: frames from the select screen's STAGE row up to the fight must be the same with and without the strips (0.88.0 fails it: 98 of 126 frames differ).

## 0.88.0
- **More of the game decompiled:** 45 more small functions are hand-written C (`object/small_steps.c`): screen objects that move until a mark, count down, then fade out or move on, the step tables run by GAME's state and by the objects' own fields, text from two of the game's lists, which fighter a layout picks, an opponent's score by a count, and a fighter's CPU script read or started again. Nothing changes in play.

## 0.87.0
- **More of the game decompiled:** 45 more small functions are hand-written C (`object/small_steps.c`): counters and waits that move objects' steps on, speeds and places copied or moved, the step tables run by the director, the game's step and BATTLE's, pad checks, a number turned into decimal digits for drawing, a flag bit set in a record, a free slot taken from a list of eight, and a few of a fighter's settings at a round's start. Nothing changes in play.

## 0.86.0
- **EX. SETTING: 1P / 2P / 3P PRIORITY, PLAYER or COM:** the rows that were 1P / 2P / 3P JOIN-IN, HUMAN or CPU, are renamed for what they decide: whether a fighter nobody started with goes to the player on that pad first (PLAYER) or to the computer (COM, the game's own word). Nothing else changes: joining with Start works as before, whatever the rows say. `a3mx.cfg` now writes `p1_priority=PLAYER` / `COM`; files from earlier versions, with `p1_join_in=HUMAN` / `CPU`, are still read.

## 0.85.0
- **SELECT + START: back to the MODE MENU:** player 1 holding SELECT and pressing START goes straight back to the MODE MENU without saving, as F2 does, so a pad alone can get back quickly. That START doesn't also open the pause menu. Part of the "F2 / SELECT + START: back to the MODE MENU" extension (quit_key), on by default.
- **Boot to the MODE MENU (new extension, off by default):** with "Boot to the MODE MENU" ticked in the launcher's Extensions tab (`--features default,boot_to_menu`), the game opens straight on the MODE MENU with your settings and save loaded. The boot screens and the load run off screen at full speed, so nothing is shown until the menu: no NOW LOADING, title screen, logos, story, attract demos or PRESS START. If the save can't be loaded, the game's own question is shown, then the MODE MENU. Only at boot: left alone, the MODE MENU still goes back to the title as before.
- **Launcher:** the Extensions tab's names no longer overlap a long title, and only Stage Extensions is marked experimental. Its screenshot needs retaking.

## 0.84.0
- **More of the game decompiled:** 40 more functions are hand-written C, most of them small steps that many objects of the menus and screens share (counters, swaps, objects made beside others, waits on the game's state), from the wider set of modes now profiled. They also include GAME's and World Tour's records being cleared, a fighter's edges for the camera, a few of a fighter's script steps and a network session flag. Nothing changes in play.

## 0.83.0
- **Report a problem on GitLab, GitHub or Gitea:** the launcher's bug button (and **About > Report a bug**) now opens a **Report** window. Choose the project's home on GitLab or either of its mirrors, GitHub or Gitea (issues on all three are read and handled); the launcher remembers the choice. **Open the form** opens a new issue there, filled in as before (on GitHub, in the bug form's fields), and now also makes **`a3mx-report.zip`** in your logs folder and opens the folder, so attaching your logs is one drag: `a3mx.log`, `last_run.txt`, `launcher.log` (this session and the one before), `launcher.ini` and `controllers.cfg`. Your home folder's path is written as `~` in the form and the zip. The window warns when Logging is OFF (the log would be empty), with a button to set it to INFO, and when the game is still running. **Request a feature** goes to the site you chose too. About lists the mirrors.
- **Issue forms on every site:** GitHub (`.github/ISSUE_TEMPLATE/`: bug and feature forms, and links to the other two sites), Gitea (`.gitea/ISSUE_TEMPLATE/`) and GitLab (`.gitlab/issue_templates/`). README, CONTRIBUTING, BUILDING and REPORTING_ISSUES say where the project and its mirrors are, and that issues can be opened on any of them. `.recomp.json` lists the Gitea mirror and the mirrors' report links.
- New `src/launcher/report.cpp`: the three sites' addresses, the report's Markdown, the home folder made private, and a small .zip writer (stored, no compression). `--show report` opens the window, for screenshots.

## 0.82.1
- **Player 2 joining Arcade on the select screen is no longer played by the computer (issue #17):** with 2P JOIN-IN on HUMAN, pad 2 already plays the opponent before it presses Start; pressing Start on the select screen then put the computer flag back on that fighter, so the computer played player 2's character for the whole fight. The fighter now stays player 2's. The same applies in every mode where a pad that JOIN-IN gave a fighter then joins with Start. New scenario `arcade_join_select`.
- **Free Battle keeps its computer opponent with 2P JOIN-IN on HUMAN (issue #17):** JOIN-IN no longer takes over a box a pad can join with Start on the select screen (every Free Battle box, and the partner in Survival Dramatic and Variable). Those stay the computer's until a pad presses Start, so with a second pad plugged in you can still play Free Battle against the computer, and pad 2 joins by pressing Start as before. JOIN-IN still lets a pad take over fighters it can't otherwise reach (World Tour, R.Dramatic Battle, Arcade's opponents, Survival's opponents). New scenario `free_battle_join_in`.

## 0.82.0
- **More of the game decompiled:** 7 more functions are hand-written C. They include where each part of a sprite goes once it is scaled and turned (run tens of thousands of times a fight), the camera following the fighters across the stage, damage lessened by the number of hits in a row, the pad effects of a hit, a reward and score for a count, a value kept within a limit by the level and the round, and fourteen objects made by the layout. Nothing changes in play.

## 0.81.0
- **More of the game decompiled:** 10 more functions are hand-written C. They include which side has lost all its fighters, the fighters' starting places, the round's stage pick in modes that change it, score for a team, a search through a fighter's move slots, markers on two gauges, and more objects of the screens around a fight (one travelling to the stage's place, sliding panels by side). Nothing changes in play.

## 0.80.0
- **More of the game decompiled:** 9 more functions are hand-written C. They include how long both fighters are held when a hit lands, a human fighter's punch or kick starting an attack, a CPU script starting an attack routine, score from a record, a value set by which two characters meet, and more objects of the screens around a fight (the round's pictures, a row of nine by a character's width, records lit for the mode, the director's start). Nothing changes in play.

## 0.79.1
- **A Gitea CI workflow:** `.gitea/workflows/ci.yml` runs the same pins check and Linux build/test as the GitLab and GitHub pipelines, for the project's Gitea mirrors. No change for players.

## 0.79.0
- **More of the game decompiled:** 9 more functions are hand-written C. They include adding to a player's score (kept to 9,999,999, shared with a partner in the team modes and counted into World Tour's total), a fighter's held-direction history, a CPU fighter's state read from its script, offsets unpacked from a character's data, two object pools and the fight's tables cleared, and more objects of the screens around a fight. Nothing changes in play.

## 0.78.0
- **More of the game decompiled:** 10 more functions are hand-written C. They include the game's random numbers being made anew (the Mersenne Twister's twist), a value picked at random from a table of 41 with a few left out, the camera layer following the fighters' height, whether a CPU fighter may take its chance on its opponent, and more objects of the screens around a fight (nineteen made at once, a bobbing record, scrolling layers hidden by the screen's state). Nothing changes in play.

## 0.77.0
- **More of the game decompiled:** 11 more functions are hand-written C. They include how much damage a hit does (through the ISM, the CPU's level and the hit's kind), a fighter's walking speeds by the direction it holds, the character's own values loaded at the start, how a CPU fighter picks a script by what its opponent is doing, and rows of pictures laid out by a character's width on the screens around a fight. Nothing changes in play.

## 0.76.0
- **More of the game decompiled:** 19 more functions are hand-written C. They include the last part of every fight frame, damage adjusted by both fighters' values, a gauge a fighter fills when hit, a count whose highest is kept for the fighters, each fighter noting which side its opponent is on, the small objects' and the queues' resets, an object drawn with a margin of its own, and more objects of the screens around a fight and of the intro. Nothing changes in play.

## 0.75.0
- **More of the game decompiled:** 25 more functions are hand-written C. They include how a CPU fighter's script starts again from the beginning, a value adjusted by one of the game's settings, the chance some CPU characters take by their level, a fighter's jittered copies, the director's step by its count, and more objects of the screens around a fight: lines of text in the stage's colour, objects following a fighter's picture, and rows of objects that arrive and report in. Nothing changes in play.

## 0.74.0
- **More of the game decompiled:** 56 more functions are hand-written C. They include the 2D library's texture lists and palettes, the game's zeroed memory allocation, the save files' names and the settings handed to the PSP's save dialogue, the network session's small checks and its start-up from scratch (still never reached offline), a fighter starting a move when an attack button is pressed, and more objects of the screens around a fight. Nothing changes in play.

## 0.73.0
- **More of the game decompiled:** 21 more functions are hand-written C: the CPU's first frame (picking its script and running the first command), a human fighter's last button press being kept, two more branches in the CPU's scripts, a thrown object swinging as it falls, and more objects of the screens around a fight and of the intro. Nothing changes in play.

## 0.72.1
- **Player 1 is Keyboard/Controller by default (issue #15):** with no `controllers.cfg` yet, player 1 now has the usual keys and the first controller plugged in, sharing one slot, and players 2 to 4 are Automatic. With no controller plugged in, player 1 just has the keys. Settings you've already saved are kept.
- **Any controller:** a Controller or Keyboard/Controller player can choose **Any (the first free one)** in the launcher's Controllers tab: the first controller no other player has chosen, with that player's buttons. If your keyboard also shows up as a controller, choose your pad for player 1 instead.

## 0.72.0
- **More of the game decompiled:** 27 more functions are hand-written C: how the CPU steps through its scripts, a fighter's move state being cleared, the button just pressed read as strength and punch or kick, objects drawn only while they are on the screen, sixteen records reset, and more of the screens' director and the objects of the screens and the intro. Nothing changes in play.

## 0.71.0
- **More of the game decompiled:** 36 more functions are hand-written C: the branches in the CPU's scripts (at random, by what the opponent is doing, by a count), a fighter being kept on the screen at the edges, the speeds a fighter is given from the game's tables, the screens' director emptying the object pools, and more objects of the screens around a fight and of the intro. Nothing changes in play.

## 0.70.0
- **More of the game decompiled:** 44 more functions are hand-written C: the emptying of the five object pools, the queues beside the drawing ring, a record's name, more of how the CPU waits and reads its scripts, and thirty-five more objects of the screens and the fight's intro (following another object, sliding and scrolling, rows of objects made one by one, animations picked at random). Nothing changes in play.

## 0.69.0
- **More of the game decompiled:** 40 more functions are hand-written C: how the CPU picks, restarts and reads its scripts, clearing the network record and its small steps, more of the routines that direct a sequence of screens, and the objects of the versus screen (arriving at their places, lighting up for the side that is choosing, their rows). Nothing changes in play.

## 0.68.0
- **More of the game decompiled:** 58 more functions are hand-written C: the seeding of the game's random numbers, the record that directs a sequence of screens, damage adjusted by ISM and by the hit, which way a fighter falls when hit, a fighter's ISM data, sliding panels and list rows, and more of the intro's and the screens' objects. Nothing changes in play.

## 0.67.0
- **More of the game decompiled:** 71 more functions are hand-written C, taking the total past a thousand: the objects of a fight's intro (the pieces that slide in before "FIGHT"), the calls into each character's own routines, more of the fighters' script commands, the walking speed, and more steps of the screens' objects. Nothing changes in play.

## 0.66.0
- **More of the game decompiled:** 72 more functions are hand-written C: the object pools (making and freeing objects), what is cleared before a screen or a fight, the camera's and the trails' routines by who is fighting whom, the stage's own routine, and thirty-nine steps of the objects on the screens around a fight (waiting on their fighter, moving to their place, starting animations). Nothing changes in play.

## 0.65.2
- **macOS: the launcher finds its folder again:** opened from the unzipped download, macOS ran it from a temporary copy elsewhere (App Translocation), so it found no folder to build in and said nothing was needed. It now asks macOS where it really is. If it still can't find its folder, the Dependencies tab and the console say what to do instead of "nothing is needed".
- **Releases hold only the full downloads:** the launcher on its own (`a3mx-launcher-...`) is no longer added to a release or the nightly on GitHub, since it can't build anything without the folder around it. Download `a3mx-<version>-<system>.zip`.

## 0.65.1
- **Linux: missing libraries named before the build (issue #2):** the build and the launcher's Dependencies tab now check for pkg-config and the window (X11 or Wayland) and sound (ALSA, PulseAudio or PipeWire) development libraries, and say which are missing with the line that installs them, instead of stopping in SDL's configure with "SDL could not find X11 or Wayland development libraries".
- **Files dated in the future no longer break the build (issue #2):** files unpacked on a PC whose clock or time zone is behind the one that made them are dated now before CMake runs, so Ninja no longer finds the build out of date every time. Your ISOs, `assets/` and `saves/` are never touched.

## 0.65.0
- **More of the game decompiled:** 46 more functions are hand-written C: a fighter's input each frame (turned to face its opponent, with what was just pressed and released), the routine that runs a fighter's script, pushing the opponent back, the wall jump check, small adjustments made when a hit lands, more of the steps objects share, two more object lists, and the camera's choice of routine by which fighters it follows. Nothing changes in play.

## 0.64.0
- **More of the game decompiled:** 25 more functions are hand-written C: how an object is queued for drawing and drawn, the step of its animation, the background layers following the camera, the span two fighters cover, making an object from a pool, the fade's dimming of colours, the fighters' trail each frame, the round clock's digits, and more of a fighter's frame (its distance from the opponent, how pressed it is, the punch-and-kick checks). Nothing changes in play.

## 0.63.0
- **More of the game decompiled:** 22 more functions are hand-written C: the steps objects' routines share (moving by a speed towards a target, falling to the floor, scrolling round, waiting on a timer, following their owner, taking the stage's colour) and five small helpers. Nothing changes in play.

## 0.62.0
- **More of the game decompiled:** 58 more functions are hand-written C: 38 more step dispatchers (most of them an object's whole frame: its step, then its animation and drawing), the camera's sideways steps, the start of an object's animation, the trail a fighter leaves, falling stage objects wrapping round the screen, and thirteen small steps and questions of a fighter's frame. Nothing changes in play.
- **New test scenario:** `arcade_other_fighter` plays an Arcade fight as E. HONDA against DAN, so the tests cover fighter and stage code that RYU's first fight doesn't.

## 0.61.0
- **More of the game decompiled:** 322 more functions are hand-written C: 268 step dispatchers (the small functions that run an object's, fighter's or screen's current step), the text queue every screen draws its text through, the draw depth table, what a fighter's player is holding, the questions the game asks about the network state, and 40 functions the game calls that do nothing. Nothing changes in play: every one is checked against the original by the tests.
- **Overall progress bar:** the progress box in [README.md](./README.md) and [docs/ROADMAP.md](docs/ROADMAP.md) now starts with one overall bar and percentage (the share of the program's instructions that is decompiled), above the functions, instructions and by-area bars.

## 0.60.0
- **New Launcher:** One launcher replaces create-launcher and the old launcher: a sidebar with Paths, Dependencies, Controllers, Extensions and Build / Run tabs, and the console at the bottom of every tab (also kept in `logs/launcher.log`), see [docs/BUILDING.md](docs/BUILDING.md).
- **Bring your own device (BYOD):** The Controllers tab maps keyboard keys and a controller to each of 4 players outside the game, with a Test button. Player 1 has the keyboard by default; until you change something the game still assigns devices automatically, see [docs/INPUTS.md](docs/INPUTS.md).
- **Extensions individually switchable:** Silent saves, Everything unlocked, Extra menus, Local multiplayer, MODE MENU bar, F2 quit key, Widescreen, Stage Extensions (was Custom Assets) and Shaders each have their own switch (`--features`). All off is the PSP game as it was. The last run's features are written to `logs/last_run.txt`.
- **Shader support:** New FILTER option in SCREEN SETTING using librashader: five CRTs (Curved, Lottes, Royale, Guest, Light), four scanline styles (Light, Strong, OSSC, Dark), xBRZ and ScaleFX. Saved in `a3mx.cfg`. Needs the shaders (a button on the Dependencies tab) and the librashader library (in each release's download, or built from the Dependencies tab), see [docs/DISPLAY.md](docs/DISPLAY.md).

## 0.55.0
- **Initial decompilation:** Some menus and drawers are fully decompiled, see progress on [README.md](./README.md).
- **Basic keyboard / pad Support:** Player 1 keyboard support OR Player 1, Player 2 and Player 3 pad support.
- **In-Game Pad Config:** Support for config for 4 pads, with L1 and L2 reinstated. Thes can also can be configured during battle also. Pad numbers will be shown on the top left corner in Mode Mode, for more info see [docs/PLAYER_CONFIG.md](docs/PLAYER_CONFIG.md).
- **WIDE FORCED mode:** Mode added temporarily using Rickow's 16:9 patch PSP (when decompile completes of these drawers, will move to own implementation).
- **Local Versus:** Local versus modes leveraging simulated network architecture within a single process, where input pads determine player routing.
- **2 and 3 Player support:** In mode that accept inputs, players can play on the same screen without the need to use network mode, for more info see [README.md](./README.md)/
- **Variable Battle 2 Player support:** Tagging will now work for two players (same side).
- **Unlocked Content:** Everything is unlocked right from the start, including all modes and I-ISM for edit mode. Hidden extras (all unlocked): hold L + R while confirming Dramatic Battle, Variable Battle, R.Dramatic Battle or Final Battle on the MODE MENU for its alternative version; hold SELECT while choosing Akuma (Shin Akuma), M. Bison (Final Bison) or Balrog's second slot.
- **EX. SETTING:** New option menu for managing some new settings.
- **Override CPU:** Allow you override the default behaviour of Player 1, Player 2 or Player 3, e.g. enabling Player 2 to HUMAN in World Tour Mode would override the CPU (see EX. SETTING).
- **Updated Save Feature:** Extended saving system to store new settings (works silently with no nag screens).
- **Import / Export Save for PSP:** Allows you import or export your save to PSP (untested).
- **Updated screen s
- **Log Events:** Launcher now has log events and frame capture for debugging to monitor hitches, pinned dependancies and better first run experience, revised also for MAC.
- **New Screen Settings:** Selection of VSYNC and FRAME TIMING: AUTO, ORIGINAL (59.94HZ), SMOOTH (60.00HZ). (Note: VRR or manual refresh rate needs to be set for ORIGINAL), A badge will appear in MODE MENU to tell you what mode is enabled, changing to Windowed may also change this setting).
- **Support for high refresh rate monitors:** 120HZ, 240HZ, 480HZ (240HZ and 480HZ untested)
- **Custom Assets (Experimental):** When Custom Assets enabled with WIDE FORCED, placer holder assets will be displayed. (This is an experimental feature currently), for more info see [assets/LAYOUT.md](../assets/LAYOUT.md) and [docs/STAGE_EXTENSIONS.md](docs/STAGE_EXTENSIONS.md)/
- **Frame Timings:** Option to select ORIGINAL PSP frame times or SMOOTH for modern displays. Note: this is highly depedennt on your screen setup, if you are getting hitches see [docs/DISPLAY.md](docs/DISPLAY.md) before logging issues.