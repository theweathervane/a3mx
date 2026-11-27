# Changelog

## 0.168.1
### Technical
- README.md / docs/ROADMAP.md: the progress bars brought up to 0.168.0 (`tools/progress.py --update-docs`); the 0.168.0 entry below.

## 0.168.0
### For players
- **More of the game decompiled (325 functions, 41.46% of the program now):** every super-combo start, command and air throws with their holds and landings, the whole hit test (hit and hurt boxes, a hit landing on the defender, two hits clashing, the attacker / defender pairing), command-input steps, special jumps, special and super cancels, and more of the move starts and menus. Nothing plays differently.

### Technical
- `src/core/object/`: `batch_moves259.c` - `batch_moves265.c`, `batch_menu263.c`, `batch_input266.c`, `batch_input267.c`, `batch_cmd268.c`, `batch_pads269.c`, `batch_target270.c`, `batch_grabs271.c`, `batch_misc272.c`, `batch_hit273.c`, `batch_holds274.c`, `batch_jumps275.c`, `batch_input276.c`, `super_start.h` (new); `batch_supers258.c` now has all 75 super-combo starts. docs/PSP_PROGRAM_MAP.md: a row for each batch.
- Checked: parity (memory hashes, lazy renderer) passed for all 89 scenarios after each batch. About 16 small leaves (among them 0x08826260, 0x0882A410, 0x08873850, 0x088693B0, 0x08870AF8) did not run in any scenario and were checked against the original code only.

## 0.167.0
### For players
- **Most extensions are switched in the game now, so everyone runs the same build:** OPTION MODE > EX. SETTING has two new pages. **EX. GAME** has Boot to the MODE MENU (off by default; from the next start), Mode Menu Shortcut (was the F2 / SELECT + START key), Shared Entry Pool (was I-ISM for both sides; on by default), 1P - 3P PRIORITY and VIRTUAL PSPS. **EX. DISPLAY** has MODE MENU icons, ISM presentation, Scaled endings and Stage Extensions. They take effect at once and are kept in `a3mx.cfg`.
- **GAME OPTION > DEFAULT keeps MODE SELECT ON,** as a new save has it (the game's own default is OFF).
- **New: SCREEN SETTING > FILTER has a screen of its own, with every shader that works:** Cross on FILTER opens a list by category (FAVOURITES: the eleven as before; then CRT, SCANLINES, HANDHELD, NTSC, EDGE SMOOTHING ... from libretro's slang-shaders, 858 presets that the game loads and builds; CUSTOM: your own `*.slangp` in `configs/shaders/`). L / R (or the side arrows' left / right) change the category, Cross chooses and goes back, Circle cancels. **SCREEN SETTING > PREVIEW** starts a real one-round fight between two random computer fighters, HUD shown, drawn through the filter (Circle, or the round's end, comes back). `tools/shaders.py` tests every preset through the game after downloading (`--catalogue` to do it again) and no longer unpacks folders that are never offered (Mega Bezel, HDR, ...). ReShade isn't part of it: run it over the window yourself (docs/DISPLAY.md).
- **New: FILTER in the pause menu,** so the filter can be changed mid-fight: Cross opens the same FILTER screen over the paused fight (whoever paused), Cross chooses, Circle cancels.
- **FILTER: a preset's own pictures (console borders, a VHS's lettering) are the right way up,** and a preset no longer leaves the game's menus broken after it (the graphics state it changes is put back after every frame).
- **librashader (FILTER's library) is built with OpenGL only** (no DirectX 9 / D3DX9_43 or Visual C++ runtime needed) and, once published by the pipeline, downloaded instead of built: no Rust needed. The launcher finds it in more places (`.tools/`, beside the game or the launcher), links to rustup.rs, and installs Rust into the tools folder if it has to build it. A release fails without it.
- **SCREEN SETTING's greyed rows** (HUD MESSAGES outside WIDE NATIVE, VSYNC when it can't sync) use EX. SETTING's greyed colours.
- **New: EX. FIGHTERS > GUARD OFF:** each fighter's GUARD row has OFF after NONE: no guard gauge, as before Alpha 3. That fighter's guard is never damaged, so it never crushes, and its guard gauge and GUARD label aren't drawn (both sides' when both are OFF). Not in network battles, like the other cheats.
- **New: EX. DISPLAY > HEALTH GRADIENT:** OFF draws the health bars in one solid yellow (the PSP's own, from the middle end of its gradient), as the arcade game and the earlier games do; ON (the default) keeps the PSP's green-to-yellow shading. Kept in `a3mx.cfg` (`[ex setting] health_gradient`).
- **Each EX. SETTING page has its own banner** in place of "EX. SETTING": EX. GAME, EX. DISPLAY, EX. UNLOCKS, EX. FIGHTERS, EX. EXP.
- **New: SCREEN SETTING > HUD MESSAGES (WIDE NATIVE):** EDGES moves the fight's messages (combo counts such as "14 HIT MARVELOUS!!!" with their rating, LEVEL n SUPER COMBO FINISH, FIRST ATTACK, COUNTER HIT, REVERSAL ...) on both sides, after the KO too, and Survival's timer out to the 16:9 screen's edges, each toward its own side and as far in from it as it is from the 4:3 picture's. EDGES by default; greyed out unless SCREEN SIZE is WIDE NATIVE. Health bars, gauges and portraits stay where they are.
- **The launcher's Extensions tab is down to four switches:** PSP original (no extension at all), Enhanced saves (was Silent saves), Extra menus and Local multiplayer. Widescreen and Shaders are no longer switches: SCREEN SETTING's SCREEN SIZE (WIDE NATIVE) and FILTER choose them, as before. Cheats and Unlocks are part of Extra menus. A choice of Boot to the MODE MENU made in the launcher before isn't carried over: set it in EX. GAME.

### Technical
- FILTER: `shader_list.c` (the favourites, the catalogue `a3mx_catalogue.tsv`, the player's folder), `FilterEntry` / `filter_get` / `filter_find` (shader.h); `a3mx --filter-test LIST OUT` and `--dump-filtered`; `filterscreen.c`, `filterpreview.c` (Free Battle's ARCADE-MODE with its kind menu and select answered in code, player 1's fighter given to the computer; pads filtered by `filterpreview_pad`), `exsetting_other_arrows` (EX. SETTING's side arrows and SCORE RANKING's up / down arrows for other OPTION MODE screens), `pausefilter.c` (hooks 0x08834AC8; wraps 0x08834FF0 and 0x088889A0). `quitkey_task_wait` now runs with any extension (it asks quit_key and the preview).
- `ext_features.h`: each feature has `where` (`WHERE_LAUNCHER`, `WHERE_EX_GAME`, `WHERE_EX_DISPLAY`, `WHERE_BUILT_IN`, replacing `section` / `FEATURE_SECTIONS`) and an `a3mx.cfg` key; `rt_feature_setting`, `features_from_settings`, `features_setting_changed`, `features_spec`. The enum is in the new order; names are unchanged. `boot_to_menu` and `quit_key` need `menus`, as does `cheats_unlocks`.
- `--features`: new keyword `settings` (the EX. SETTING features as `a3mx.cfg` has them; a feature named in the list wins for this run). No `--features` is `default,settings`; the launcher passes `saves,menus,local_multiplayer,settings` (or `none`). A list without `settings`, as the tests pass, is exact as before. `--record` stores `features_spec()`, so a replay reads its own copy of `a3mx.cfg` the same way; `--custom-assets` wins over the setting.
- `a3mx.cfg` `[ex setting]`: `boot_to_mode_menu`, `mode_menu_shortcut`, `shared_entry_pool`, `mode_menu_icons`, `ism_presentation`, `scaled_endings`, `stage_extensions`.
- `exsetting_game_option_default` wraps 0x088B67B8 (GAME OPTION > DEFAULT): MODE SELECT (system data + 516) back to 1 after it. The launcher's Extensions tab points to OPTION MODE's menus instead of listing them.
- `exsetting.c`: pages `PG_GAME`, `PG_DISPLAY` first, the cheats' four after them with `cheats_unlocks`; titles per page; the text pool holds 256 strings. Launcher settings: `psp_original`, and `feature.*` only for the launcher's three.
- `a3mx.cfg` `[screen setting]`: `hud_messages` (`4:3` / `EDGES`, `rt_hud_edges`). `screensetting.c`: row `R_HUD` after FILTER (eight rows with FILTER, seven without; DEFAULT and EXIT one lower). `widescreen.c`: `widescreen_edge_shift` moves the 8-pool's kinds 2 / 3 (types 0 / 1: left / right) by the margin in the fight with it on; `widescreen_hud_text`, called around the two text passes (`selectscreen.c`'s hooks of 0x08812520 / 0x0880B098), moves Survival's timer text and puts the queue's x back after.
- `tools/librashader.py`: OpenGL-only build (`--no-default-features --features runtime-opengl`, static C runtime on Windows), `--out`, the published `prebuilt` file (pins.json) downloaded first; `--install-rust` into `.tools/rust`. `package_launcher.py --require-librashader` (tags). GitLab: manual `librashader:publish-*` jobs.
- `pausefilter.c` wraps 0x08834AC8, 0x08834FF0, 0x088889A0 and 0x088888E8 (the panel); `filterscreen_open_pause`. `shader_gl.c`: the picture goes in top row first; `put_back_state` after each filtered frame.
- Checked: built and run on Windows; parity pause_config, pause_config_original, pause_config_players, screen_setting, option_tour, option_tour_original, mode_tour, no_extensions, select_start_menu, quit_to_menu and difftest passed; FILTER, PREVIEW and the pause FILTER screen tried in the game (C:\_Notes screenshots).

## 0.166.1
### For players
- **WIDE NATIVE: the character select screen's "1P RYU" and "2P KEN" name rows reach both sides of the screen.** They used to stop before the right edge, so names popped in and out there.

### Technical
- `src/extensions/widescreen.c`: the name rows (kind 16 type 6, drawer 2; player 1's at y 220, player 2's at y 36) scroll and jump back by 64 units. They are no longer moved out as the other type 6 pieces (that pushed both rows left), and are drawn again 64 units each way (`widescreen_edge_copy`).
- Checked: parity arcade_fight, arcade_join_select, no_extensions passed; the WIDE NATIVE check wide_fight passed; the select screen with both players joined looked at by eye.

## 0.166.0
### For players
- **More of the game decompiled (82 functions, 38.60% of the program now):** special-move follow-ups and their effects, a mashed special and a mashed hold throw, the fighter's fall and knockdown recovery, the first super-combo starts, and three menu cursors (a three-item menu, a six-item menu and a grid). Nothing plays differently.

### Technical
- `src/core/object/`: `batch_moves244.c` - `batch_moves257.c`, `batch_menu251.c`, `batch_menu252.c`, `batch_menu256.c`, `batch_supers258.c` (new). docs/PSP_PROGRAM_MAP.md: a row for each batch.
- Checked: parity (memory hashes, lazy renderer) passed for all 89 scenarios after each batch. 0x0886D5D0 did not run in any scenario, the knockdown recovery ran twice and each super start once or twice.

## 0.165.0
### For players
- **More of the game decompiled (1,595 functions, 37.88% of the program now):** the fighters' moves step by step (special-move starts and their follow-ups, the super gauge's gains, throws, dashes, jumps and landings, projectiles and effects), the round flow, the HUD, crouching, damage into vitality, the fighter animation step, the sound cue queue, the wireless (ad-hoc) VS link and the stage's start and load. Nothing plays differently.

### Technical
- `src/core/object/`: `batch_moves140.c` - `batch_moves243.c`, `batch_tiny33.c` - `batch_tiny109.c`, `batch_net113.c` - `batch_net126.c`, `batch_sound127.c` - `batch_sound134.c`, `batch_round*.c`, `batch_anim*.c`, `batch_crouch*.c`, `batch_hud*.c`, `batch_parts*.c`, `batch_starts*.c`, `batch_dmg129.c`, `batch_menu*.c`, `batch_rect*.c`, `batch_text*.c` (new); `src/core/stage/stage_load111.c`, `stage_start110.c` (new). docs/PSP_PROGRAM_MAP.md: a row for each batch.
- Checked: parity (memory hashes, lazy renderer) passed for all 89 scenarios after each batch.

## 0.164.0
### For players
- **Edited fighter sprites are in the game:** repaint a picture in your assets folder (`characters/<nn>_<name>/fighter/sprites/pictures/`, from the art pack) or change a colour in its `metadata.json`, and the game uses it the next time it loads that fighter. Nothing to run. A picture that can't be used (another size, two palettes in one 32 x 32 cell) is left as it was, and the log says why.
- **More of the game decompiled (1,151 functions):** the character select, VS and menu screens (portraits, name plates, the arcade route's opponents, the CPU's picks), the pads' auto-repeat, the super gauge, how each character's moves step from one stage to the next, and many small pieces of the fighters' moves, effects and objects. Nothing plays differently.

### Technical
- `src/platform/asset_pac.c`: a fighter archive's picture list, tile set and palettes built from edited sprite PNGs and colours as the archive opens (docs/SPRITE_FORMAT.md, "Edited sprites"); `asset_sprite_picture`. Unedited files still give the disc's archive byte for byte. `tools/sprites.py`: `metadata.json` records `pictures_written`. `tests/asset_pac_test.c`: sprite PNGs read back for all 43 fighter archives; `sprite_edits`.
- `tests/scenarios/fighters_01_ryu_ehonda.txt` ... `fighters_19_ingrid_ryu.txt` (new, written by `tools/make_fighter_scenarios.py`): every character's special moves and super combos, two characters a Free Battle with PLAYER CONFIG's one-button moves, so the parity suite reaches each character's own code.
- `src/core/object/steps_fighters.c` (new): 286 step dispatchers; `batch_tiny1.c` - `batch_tiny32.c` (new): 725 small functions; `batch_select1.c` - `batch_select32.c` (new): 140 functions of the select, VS and menu screens.
- Checked: parity (memory hashes, lazy renderer) passed for all 89 scenarios after each batch, and each of the 1,151 new functions ran in at least one; asset_pac, assets_in_game, stage_strips.
## 0.163.1
### For players
- **Boot to the MODE MENU is off by default again:** the game opens on the title, as on the PSP. A launcher that already saved its settings keeps your choice; press Defaults on the Extensions tab to take the new one.
- **The fighters' sprites as pictures:** the art pack (launcher: Build art pack) now also writes every fighter's sprites as PNG, with their colours and animation frames, into your assets folder (`characters/<nn>_<name>/fighter/sprites/`). The game doesn't load edited sprites yet.
- **The art pack works without Pillow,** as on a PC whose only Python is the one the build downloads.

### Technical
- `docs/SPRITE_FORMAT.md` (new): pictures, LZ-packed tiles, palettes and animation frames of the fighter archives; `tools/sprites.py` (new): `verify` against a RAM snapshot's picture cache, `picture`, `extract`; run by `tools/art_pack.py` (`--no-sprites`, `--sprites-only`) and `tools/extract_assets.py --sprites`.
- `boot_to_menu` off by default (`ext_features.h`).
- `tests/stage_strips_test.py` and the art pack no longer need Pillow (`measure_stages.RgbPng`).
- Checked: `sprites.py verify` (52 cached pictures identical), assets_in_game, stage_strips (with Pillow hidden too), parity boot_to_menu and arcade_fight.

## 0.163.0
### For players
- **More of the game decompiled:** how sprites, text and shapes are handed to the 2D library to be drawn. Nothing plays differently.

### Technical
- `src/core/syslib/draw2d_draws.c` (new): 0x089E1950, 0x089E10D8, 0x089E1618; `src/core/syslib/draw2d.c`: 0x089E155C, 0x089E1234, 0x089E1464.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, attract_mode, arcade_ending, super_finish, mode_tour, free_battle_team, boot_to_select, save_load, network_match, free_battle_dramatic, arcade_rival_dialogue, no_extensions, stage_objects_22_honda, stage_objects_31_guile, stage_objects_29_bison1, wide_fight, arcade_match and option_tour; two paths no scenario takes forced with pokes, identical to the original.

## 0.162.0
### For players
- **More of the game decompiled:** the rest of the sprite drawers (rows, pieces with their own palettes, scaled and turned pieces) and a fighter's extra picture. Nothing plays differently.

### Technical
- `src/core/object/sprite_drawers_more.c` (new): sprite drawers 4, 5, 7, 8 / 9, 10, 12 (0x0880C7D8, 0x0880C930, 0x0880CC08, 0x0880CCEC, 0x0880CE74, 0x0880D0F4) and 0x0880D68C.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, arcade_ending, super_finish, attract_mode, mode_tour, free_battle_team, boot_to_select, save_load, network_match, free_battle_dramatic, arcade_rival_dialogue, no_extensions, stage_objects_22_honda, stage_objects_31_guile, stage_objects_29_bison1, wide_fight and arcade_match; drawers 4, 8 / 9 and 10, which no scenario uses, forced with pokes, identical to the original.

## 0.161.0
### For players
- **More of the game decompiled:** how sprites are unpacked into the picture cache and drawn from it. Nothing plays differently.

### Technical
- `src/core/object/sprite_cache.c` (new): 0x0880E150, 0x0880E9E0, 0x0880EE34, 0x0880EFB8, 0x088D2B64 (the tiles' LZ unpacker).
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, attract_mode, arcade_ending, super_finish, mode_tour, free_battle_team, boot_to_select, save_load, network_match, free_battle_dramatic, arcade_rival_dialogue, no_extensions, stage_objects_22_honda, stage_objects_31_guile, wide_fight and arcade_match; a full cache and the palette kinds no scenario uses forced with pokes, identical to the original.

## 0.160.1
### For players
- **Tests fixed:** two asset tests that had stopped reaching their fight. Nothing plays differently.

### Technical
- `assets_in_game` and `stage_strips` run with the parity tests' features (`TEST_FEATURES`): since 0.127.0 the game boots straight to the MODE MENU, so their inputs, which start from the title, ended in the attract mode's demo fights and the tests failed.

## 0.160.0
### For players
- **More of the game decompiled:** the fight HUD's health bars. Nothing plays differently.

### Technical
- `src/core/object/batch_steps31.c` (new): 0x0888F0EC.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, arcade_match, network_match, free_battle_team, pause_config, free_battle_dramatic, free_battle_rdramatic, super_finish, arcade_rival_dialogue, stage_objects_01_ryu_1, quit_to_menu and wide_fight; three paths forced with pokes, identical to the original.

## 0.159.0
### For players
- **More of the game decompiled:** the setup of the menus' row objects. Nothing plays differently.

### Technical
- `src/core/object/batch_steps30.c` (new): 0x088CFCCC.
- Checked: parity (memory hashes, lazy renderer) passed for mode_tour, mode_menu, option_tour, boot_to_menu, save_load, select_choices, free_battle and arcade_fight; the kinds no scenario sets forced with pokes in mode_tour, identical to the original.

## 0.158.0
### For players
- **More of the game decompiled:** the fight HUD's pop-up cells. Nothing plays differently.

### Technical
- `src/core/object/batch_steps29.c` (new): 0x0888F9D4.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, arcade_match, super_finish, network_match, free_battle, attract_mode, free_battle_team, arcade_rival_dialogue, boot_to_select, arcade_ending, no_extensions, free_battle_dramatic and wide_fight; one path forced with pokes in arcade_match, identical to the original.

## 0.157.0
### For players
- **More of the game decompiled:** the fight HUD's stun gauges. Nothing plays differently.

### Technical
- `src/core/object/batch_steps28.c` (new): 0x0888FF38.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, super_finish, wide_fight, free_battle, arcade_match, network_match, attract_mode, free_battle_team, arcade_rival_dialogue, boot_to_select, arcade_ending, no_extensions and free_battle_dramatic; the full-gauge path forced with a poke in arcade_fight, identical to the original.

## 0.156.0
### For players
- **More of the game decompiled:** the fighters matched to a list of characters at a fight's start. Nothing plays differently.

### Technical
- `src/core/object/batch_steps27.c` (new): 0x088158A4.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, arcade_match, free_battle_dramatic, free_battle_rdramatic, free_battle_team, network_match, attract_mode, boot_to_select, arcade_ending, super_finish, no_extensions and arcade_other_fighter.

## 0.155.0
### For players
- **More of the game decompiled:** the network menu's items (which show on the chosen row and run their option) are now readable code. Nothing plays differently.

### Technical
- `src/core/object/batch_steps26.c` (new): 0x088ED1C8.
- Checked: reached in network_match; parity (memory hashes, lazy renderer) passed for network_match, arcade_fight, attract_mode, boot_to_select, mode_tour, no_extensions, arcade_match, option_tour, arcade_ending and free_battle_team.

## 0.154.0
### For players
- **More of the game decompiled:** the pairs of fighters pushed apart by the fight's layout, and the first ranking row's start. Nothing plays differently.

### Technical
- `src/core/object/batch_steps25.c` (new): 0x088BD158 and 0x08873D5C.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, attract_mode, free_battle_dramatic, free_battle_rdramatic, wide_walls, free_battle_team, arcade_match, network_match, super_finish, arcade_other_fighter, boot_to_select, arcade_ending, mode_tour, no_extensions and option_tour.

## 0.153.0
### For players
- **More of the game decompiled:** fighters pushing each other apart (at walls, in the air or half each) and a ranking row's start are now readable code. Nothing plays differently.

### Technical
- `src/core/object/batch_steps24.c` (new): 0x088BCDD8 and 0x08875FB8.
- Checked: parity (memory hashes, lazy renderer) passed for arcade_fight, attract_mode, wide_walls, wide_left_wall, wide_fight, free_battle_dramatic, free_battle_rdramatic, free_battle_team, arcade_match, network_match, super_finish, arcade_other_fighter, arcade_ending_full, arcade_rival_dialogue, free_battle, stage_objects_01_ryu_1, stage_objects_15_sakura, boot_to_select, arcade_ending, mode_tour, no_extensions and option_tour.

## 0.152.0
### For players
- **More of the game decompiled:** the score ranking's rows (place, name, score) are now built by readable code. Nothing plays differently.

### Technical
- `src/core/object/batch_steps23.c` (new): the first ready function of 201 - 400 instructions (0x088BD4A0).
- Checked: reached in attract_mode; parity (memory hashes, lazy renderer) passed for attract_mode, arcade_fight, network_match, arcade_match, boot_to_select, arcade_ending, mode_tour, free_battle_team, no_extensions, arcade_other_fighter, option_tour, super_finish, free_battle_dramatic and arcade_rival_dialogue.

## 0.151.0
### For players
- **More of the game decompiled:** 3 more functions: the ranking records written after the name entry, the fight's name plates and a name plate's start. Nothing plays differently.

### Technical
- `src/core/object/batch_steps22.c` (new): the last 3 ready functions of 101 - 200 instructions.
- Checked: all 3 reached in the parity runs; parity (memory hashes, lazy renderer) passed for arcade_match, network_match, arcade_fight, arcade_ending_full, free_battle_dramatic, free_battle_rdramatic, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, arcade_rival_dialogue, super_finish, pause_config_original, arcade_join_select and free_battle.

## 0.150.0
### For players
- **More of the game decompiled:** 2 more functions: a round's results being scored (vital bonus, time, level and best combo) and a fighter's matchup flag. Nothing plays differently.

### Technical
- `src/core/object/batch_steps21.c` (new): 2 ready functions of 101 - 200 instructions.
- Checked: both reached in the parity runs; parity (memory hashes, lazy renderer) passed for arcade_fight, arcade_match, network_match, arcade_ending_full, free_battle_dramatic, free_battle_rdramatic, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, arcade_rival_dialogue, super_finish, pause_config_original, arcade_join_select and free_battle.

## 0.149.0
### For players
- **More of the game decompiled:** 3 more functions: a network menu row's step, a critter wandering a stage and the select grid's cells. Nothing plays differently.

### Technical
- `src/core/object/batch_steps20.c` (new): 3 ready functions of 101 - 200 instructions.
- Checked: all 3 reached in the parity runs; parity (memory hashes, lazy renderer) passed for network_match, stage_objects_15_sakura, boot_to_select, stage_objects_34_hawk, stage_objects_31_guile, arcade_join_select, arcade_fight, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, arcade_rival_dialogue, arcade_match, super_finish, free_battle_dramatic and pause_config_original.

## 0.148.0
### For players
- **More of the game decompiled:** 3 more functions: a text scroller's start, the fade for flagged fighters on unpausing and the network menu's status line. Nothing plays differently.

### Technical
- `src/core/object/batch_steps19.c` (new): 1 ready function of 9 - 100 instructions and 2 of 101 - 200.
- Checked: all 3 reached in the parity runs; parity (memory hashes, lazy renderer) passed for network_match, super_finish, arcade_fight, arcade_ending_full, free_battle_dramatic, arcade_match, pause_config, pause_config_original, pause_config_players, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour and arcade_rival_dialogue.

## 0.147.0
### For players
- **More of the game decompiled:** 4 more functions: a row object's step, the network matching for a thread, the pause menu's texts and a text scroller. Nothing plays differently.

### Technical
- `src/core/object/batch_steps18.c` (new): 2 ready functions of 9 - 100 instructions and 2 of 101 - 200.
- Checked: all 4 reached in the parity runs; parity (memory hashes, lazy renderer) passed for network_match, arcade_match, pause_config_original, arcade_ending_full, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, super_finish, arcade_rival_dialogue, pause_config, pause_config_players and free_battle_dramatic.

## 0.146.0
### For players
- **More of the game decompiled:** 4 more functions: the camera's edges for three fighters, the hit checks between objects, the network matching's start and a banner's fade. Nothing plays differently.

### Technical
- `src/core/object/batch_steps17.c` (new): 1 ready function of 9 - 100 instructions and 3 of 101 - 200. `src/core/psp_os.h`: wrappers for sceNetAdhocMatchingInit, sceNetAdhocMatchingCreate and sceNetAdhocMatchingStart.
- Checked: all 4 reached in the parity runs; parity (memory hashes, lazy renderer) passed for arcade_fight, network_match, free_battle_rdramatic, arcade_ending_full, free_battle_dramatic, arcade_match, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, super_finish, arcade_rival_dialogue, pause_config_original, arcade_join_select and stage_objects_31_guile.

## 0.145.0
### For players
- **More of the game decompiled:** 5 more functions, among them each fighter's pause check, the camera's edges for three fighters and the flocks flying over a stage. Nothing plays differently.

### Technical
- `src/core/object/batch_steps16.c` (new): the last 5 ready functions of 61 - 100 instructions.
- Checked: all 5 reached in the parity runs; parity (memory hashes, lazy renderer) passed for arcade_fight, network_match, stage_objects_31_guile, free_battle_dramatic, free_battle_rdramatic, arcade_match, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, no_extensions, option_tour, super_finish, arcade_rival_dialogue, pause_config_original, arcade_join_select, stage_objects_15_sakura and arcade_ending_full.

## 0.144.0
### For players
- **More of the game decompiled:** 8 more functions, among them pictures by slot, CPU levels for a fight, a CPU's character swap, fighters' colours kept apart and the network scan. Nothing plays differently.

### Technical
- `src/core/object/batch_steps15.c` (new): 4 ready functions of 9 - 60 instructions and 4 of 61 - 100. `src/core/psp_os.h`: a wrapper for sceNetAdhocctlGetScanInfo.
- Checked: the coverage pass maps all 8 to scenarios; parity (memory hashes, lazy renderer) passed for arcade_rival_dialogue, network_match, arcade_match, free_battle_dramatic, free_battle_rdramatic, pause_config, pause_config_original, pause_config_players, arcade_ending, arcade_fight, arcade_join_select, arcade_other_fighter, boot_to_select, attract_mode, mode_tour, free_battle_team, no_extensions, option_tour, super_finish and arcade_ending_full.

## 0.143.0
### For players
- **More of the game decompiled:** 7 more functions, among them the heap's allocation, a screen's banner, CPU levels for a versus fight, a round's end and the network thread's entry. Nothing plays differently.

### Technical
- `src/core/object/batch_steps14.c` (new): 1 ready function of 9 - 60 instructions and 6 of 61 - 100. `src/core/psp_os.h`: a wrapper for sceKernelExitThread.
- Checked: the coverage pass maps all 7 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, free_battle_dramatic, pause_config_players, arcade_ending_full, free_battle_rdramatic, arcade_join_select, arcade_rival_dialogue, arcade_match, super_finish, arcade_ending, arcade_fight, boot_to_select, attract_mode, mode_tour, free_battle_team, no_extensions, arcade_other_fighter, option_tour and pause_config.

## 0.142.0
### For players
- **More of the game decompiled:** 11 more functions, among them the gauge's digits, followers stepping their owner's records, round markers, and the network threads' start and end. Nothing plays differently.

### Technical
- `src/core/object/batch_steps13.c` (new): 3 ready functions of 9 - 60 instructions and 8 of 61 - 100. `src/core/psp_os.h`: wrappers for sceNetAdhocctlDelHandler, sceNetAdhocctlTerm, sceNetAdhocTerm and sceNetTerm.
- Checked: the coverage pass maps all 11 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_match, arcade_rival_dialogue, arcade_ending_full, super_finish, stage_objects_34_hawk, pause_config, pause_config_original, pause_config_players, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, no_extensions, arcade_other_fighter, free_battle_dramatic, free_battle_rows and option_tour.

## 0.141.0
### For players
- **More of the game decompiled:** 13 more functions, among them a fighter's reset when free or past its gauge, a CPU script's specials by ISM, two hit boxes overlapping, the gauge's digits, the fight frame's objects and the network thread's loop. Nothing plays differently.

### Technical
- `src/core/object/batch_steps12.c` (new): the 5 ready functions of 9 - 60 instructions and the first 8 of 61 - 100. `src/core/psp_os.h`: a wrapper for sceNetAdhocctlGetState.
- Checked: the coverage pass maps all 13 to scenarios; parity (memory hashes, lazy renderer) passed for arcade_rival_dialogue, arcade_match, free_battle_dramatic, network_match, stage_objects_15_sakura, quit_to_menu, arcade_ending_full, super_finish, stage_objects_31_guile, select_start_menu, pause_config, pause_config_original, pause_config_players, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter and no_extensions.

## 0.140.0
### For players
- **More of the game decompiled:** 21 more small functions, among them objects following a fighter, fighters' tables, CPU levels by the fight's layout, a fighter turning toward the nearest, the frame's run by screen, and the network session's members, thread and message box. Nothing plays differently.

### Technical
- `src/core/object/batch_steps11.c` (new): the last 3 ready functions of 9 - 40 instructions and the other 18 of 41 - 60.
- Checked: the coverage pass maps all 21 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, free_battle_rows, free_battle_dramatic, arcade_join_select, arcade_match, stage_objects_15_sakura, pause_config_original, super_finish, free_battle_rdramatic, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, arcade_rival_dialogue, free_battle and select_start_menu.

## 0.139.0
### For players
- **More of the game decompiled:** 30 more small functions, among them objects queued by layer and following a fighter's picture, slides and timers, effects at a fighter's side, the check that lets a fighter start a move, and the network session's lists. Nothing plays differently.

### Technical
- `src/core/object/batch_steps10.c` (new): the last 4 ready functions of 9 - 40 instructions and 26 of 41 - 60.
- Checked: the coverage pass maps all 30 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, super_finish, arcade_match, free_battle_dramatic, arcade_rival_dialogue, free_battle_rows, stage_objects_31_guile, stage_objects_19_gen, arcade_join_select, select_start_menu, arcade_fight, boot_to_select, attract_mode, arcade_ending, pause_config_players, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions and stage_objects_01_ryu_1.

## 0.138.0
### For players
- **Art pack: every stage layer whole.** The art pack (launcher: Build art pack, or `python tools/build.py --art-pack`) now has a `layers/` folder per stage: each of the stage's layers drawn out in full from your disc, one PNG per layer numbered as the game numbers them (`layer0` the back, `layer1` the fighters', `layer2`, `layer3` the floor band, Charlie's `layer4`), one PNG a frame for animated layers. Most are 1024 x 512, more than the camera ever shows, so whoever paints the Stage Extensions has the whole layer to trace from, not just screenshots at the walls.

### Technical
- `tools/stage_layers.py` (new): renders each stage's tile maps (stNN.pac entry 0) with its page 0xC textures, choosing the drawer per layer as core/stage/background.c and camera_frame.c do (16- or 32-pixel tiles, Balrog's and Guile's own maps, Guile's strips, Bison's pictures and animated layer 2), plus the floor band flat (floor.c: slot 0, height from the lean table 0x08A09268) and Charlie's floor picture; writes `layers.json` per stage. Standard library only; all 38 stages in about 11 s.
- `tools/art_pack.py`: writes `layers/` beside `reference/` and `paint/`. Docs: STAGE_EXTENSIONS.md, ROADMAP (art pack item done).
- Left out: Bison's sky ripple and the floor's perspective lean (drawn flat); stage objects (sprites) are not layers.

## 0.137.0
### For players
- **More of the game decompiled:** 18 more small functions, among them fade levels, the fighters' trails, objects scrolling with the camera, ending scenes moving the camera layers, and the network menu's head row and message panel. Nothing plays differently.

### Technical
- `src/core/object/batch_steps9.c` (new): the 12 functions of 9 - 40 instructions made ready by the earlier batches and the first 6 of 41 - 60.
- Checked: the coverage pass maps all 18 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_ending_full, stage_objects_19_gen, free_battle_dramatic, free_battle_rdramatic, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, free_battle, arcade_rival_dialogue, arcade_match, super_finish, pause_config_original and free_battle_rows.

## 0.136.0
### For players
- **More of the game decompiled:** 23 more small functions, among them objects placed by the camera and sliding to a stop, the pause menu's values, the fade levels for a fight's layout, fighters' button strength, targets and trails, and the network menu's option texts. Nothing plays differently.

### Technical
- `src/core/object/batch_steps8.c` (new): the last 23 ready functions of 25 - 40 instructions. `src/core/psp_os.h`: a wrapper for sceKernelGetThreadId.
- Checked: the coverage pass maps all 23 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, pause_config_original, free_battle_dramatic, super_finish, arcade_match, free_battle_rows, arcade_ending_full, free_battle_rdramatic, stage_objects_19_gen, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, free_battle and arcade_rival_dialogue.

## 0.135.0
### For players
- **More of the game decompiled:** 26 more small functions, among them objects stepping in time with an ending's scenes, set from their owner and shown by a level, box settings, fighters' script branches and a CPU's special opponents, and the network link's events, thread end and pairing. Nothing plays differently.

### Technical
- `src/core/object/batch_steps7.c` (new): 26 of the ready functions of 25 - 40 instructions. `src/core/psp_os.h`: wrappers for sceNetFreeThreadinfo, sceKernelWaitThreadEnd, sceKernelTerminateThread, sceKernelDeleteThread and sceNetAdhocMatchingSelectTarget.
- Checked: the coverage pass maps all 26 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, pause_config_players, arcade_match, arcade_ending_full, free_battle_rows, pause_config_original, arcade_rival_dialogue, stage_objects_34_hawk, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, free_battle, super_finish and stage_objects_15_sakura.

## 0.134.0
### For players
- **More of the game decompiled:** 25 more small functions, among them objects stepping while the screen allows it and appearing at the intro's stages, the text records, a fighter's choice of target and the side ahead on rounds, and the network link's close and the network's error results. Nothing plays differently.

### Technical
- `src/core/object/batch_steps6.c` (new): 25 of the ready functions of 25 - 40 instructions. `src/core/psp_os.h`: wrappers for sceNetAdhocctlDisconnect and sceNetAdhocctlGetParameter.
- Checked: the coverage pass maps all 25 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_match, arcade_ending_full, stage_objects_15_sakura, pause_config_original, free_battle_dramatic, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, free_battle, arcade_rival_dialogue, super_finish, stage_objects_19_gen and free_battle_rdramatic.

## 0.133.4
### For players
- **WIDE NATIVE, E. Honda's ending:** the floor past the dojo's right side is now E. Honda's own stage floor, so the ring's boards curve on as they do in a fight, instead of the dojo's last column of floor stretched out.

### Technical
- `extensions/scaled_endings_stages.c` `floor_texel`: from `floor_row` down, the strips take the stage's floor band as `core/stage/floor.c`'s stage_floor (0x0881C5A8) lays it out with no shake: page 0xC slot 0 of the stage's archive (the game's floor texture 0xA0) in four strips 256 pixels wide from the layer's x 128, strip i from texture row 64 i + 16 at y 208 to 64 i + 64 at y 256. Measured in a fight on E. Honda's stage (stage_objects_22_honda): band height 208 (0x092E5F78 + 130), layer 1's scroll. The 0.133.3 edge-column fill is gone. `tools/ending_stages.py`: SCENE_VIEW's third value now means this.
- Found: the dojo's ring floor isn't a repaint: it is E. Honda's stage floor (slot 0 holds the ring).
- Checked: Final Battle endings against 0.133.2 for E. Honda and the other stage-band endings (Ryu, Ken, Sagat, Sakura, Guy, Maki, Dan): only E. Honda's changes; the ring continues across the join by eye.

## 0.133.3
### For players
- **WIDE NATIVE, E. Honda's ending:** the dojo is no longer scaled. It is E. Honda's stage repainted, so past its right side his stage carries on (the shop and the sign beyond the post box, the floor's boards continuing), and the camera starts a little further right, so its pan ends on the dojo's own art with no black edge. His last picture over the dojo shows it the same way.

### Technical
- `tools/ending_stages.py`: `FORCED` bands given by hand where the 16-pixel block match can't tell (end18's dojo, layer 1 rows 0 - 15: st18's layers 0 / 2 (32) / 2 (16) / 1 at the same columns, its front layer flattened onto the back one, so only a third of its blocks match one layer); `SCENE_VIEW` per ending (wide, shift, floor_row) written into the table; up to 12 palettes a band (st18 has 9).
- `extensions/scaled_endings_stages.c`: `scaled_endings_stage_view` (a `wide` band in this layer 1 map, and its shift); strips placed with the shift; from `floor_row` down, the strips carry on the scene's own edge column (read from endNN_u.pac). `core/task/ending.c`: such a scene is drawn as it is (`scene_scaled` 0) with `tiles_ending_shift`; `core/stage/tiles.c` moves the layers' quads left by it, only in an ending's frame (`background.c` / `tiles.c` call `tiles_ending_quad` when it is set).
- Found: the dojo's map in memory is end18's (columns 18 - 51), not the first archive's layout guessed from the character list; the stage's floor (`core/stage/floor.c`) starts at row 16, below the scene.
- Checked: every character's Final Battle ending against 0.133.2 every 50 frames: only E. Honda's (9100 - 9950, 10250 - 10400) changes; no bare column through the dojo's pan. Parity: arcade_ending.

## 0.133.2
### For players
- **WIDE NATIVE endings, from the review of all of them:**
  - **Juni (Chun-Li's ending):** her picture over the last fight's stage now fills the screen as it does later in the ending. Scaled scenes now keep their top and lose their bottom under the text band (before, they were scaled about their middle, cutting heads at the top): Juni's legs go, T. Hawk is seen whole on the cliff.
  - **E. Honda's dojo:** no longer jumps to a smaller size as the camera pans, and no black edge at the left when it stops.
  - **Evil Ryu, Eagle, Balrog:** the character art and last pictures standing on the stage keep their size (Evil Ryu no longer grows while he slides in). So do the other last pictures that reach a 4:3 side but don't fill it: Adon, Blanka, R. Mika, Rose and Sodom.
  - **Dee Jay:** the blue burst beside his punch is gone (the black sides now reach the screen's edges: a square picture), and the green speed lines behind him reach the screen's sides.

### Technical
- `core/object/sprite_drawers.c`: an ending's scaled pictures about (192, `ENDING_TOP_Y` 8); `core/stage/tiles.c` mode 2 about (192, `SCENE_TOP`). Pictures over a scaled stage (`scaled_endings_oversize` 2) measured each frame (`measure_across`): grow when they cover 0 - 384, or as a kind 73 type 0 backdrop reaching a side that is under 288 across (Karin's truck) or beside a whole one growing (Evil Ryu's cave); else 1:1 (was: grow when reaching a side). Over a stage drawn as it is, a kind 73 type 0 backdrop (4) covering 0 - 384 but not already -48 - 432 is scaled (5). `mirrored_out`: Dee Jay's 112 - 272 backdrop on a scene with no layer shown, the frame's only scaled backdrop, drawn mirrored / plain out to 0 - 384.
- `core/task/ending.c` `scene_kept`: a scene scaled for bare columns (`scene_scaled` 2) stays scaled until a cut (kind or shown layers change, a layer jumps over 64, a gap in the ending's frames); a scene drawn as it is may still turn scaled. `scaled_endings_bare`: no layer shown.
- `extensions/widescreen.c` `widescreen_plain_adjust`: an ending's type 1 black side of any width under 384 at a 4:3 side reaches the 16:9 side (was 96 only).
- Checked: every character's Final Battle ending against 0.133.1 every 50 frames, with the new rules on the old middle anchor (only the endings above and the last pictures listed change), then by eye with the top anchor. 4:3 (arcade_ending_full hashes) and the scaled endings off unchanged. Parity: arcade_ending, wide_fight.

## 0.133.1
### For players
- **WIDE NATIVE, Ken's ending:** Charlie in his cockpit now fills the screen as he does in Chun-Li's ending (it is the same picture). In Ken's ending it stands at another place in the ending's map, with the clouds scrolling past behind it, so it was shown at the 4:3 size with the clouds carried on at the sides.

### Technical
- `core/task/ending.c` `scene_scaled`: in the endings' own map (ending kind 2), layer 1 not moving across (`held`: as the frame before, or just cut to) with no art past its 4:3 sides (`layer_wide` false) makes the scene scaled, whatever scrolls behind it. The other rules are as before.
- Checked: every character's Final Battle ending (ending_final) against 0.133.0 every 50 frames: only Ken's cockpit frames (6300 - 6700) change. Parity (lazy renderer): arcade_ending, arcade_ending_full, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.133.0
### For players
- **More of the game decompiled:** 35 more small functions, among them objects bouncing, sliding and starting their animations, the screen fades for one side, fighters' script branches on health, the CPU's random choices, and the network session's members, lock and error codes. Nothing plays differently.

### Technical
- `src/core/object/batch_steps5.c` (new): the last 15 ready functions of 17 - 24 instructions and the first 20 of 25 - 40. `src/core/psp_os.h`: wrappers for sceKernelWaitSema and sceKernelSignalSema.
- Checked: the coverage pass maps all 35 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_match, stage_objects_19_gen, free_battle_rows, arcade_ending_full, pause_config_original, super_finish, stage_objects_34_hawk, arcade_other_fighter, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, option_tour, no_extensions, free_battle, arcade_rival_dialogue and free_battle_rdramatic.

## 0.132.0
### For players
- **More of the game decompiled:** 56 more small functions, among them objects sliding, rising and counting down, the ending's next animation, fighters' script branches by ISM, and the network link's flags, group connect and matching end. Nothing plays differently.

### Technical
- `src/core/object/batch_steps4.c` (new): 54 of the 70 ready functions of 17 - 24 instructions. `src/core/object/steps.c`: 2 more step dispatchers (0x088884FC, 0x08961910). `src/core/psp_os.h`: wrappers for sceNetAdhocctlConnect and sceNetAdhocMatchingStop / Delete / Term.
- Checked: the coverage pass maps all 56 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_match, arcade_ending_full, super_finish, stage_objects_19_gen, pause_config_original, free_battle_rdramatic, stage_objects_15_sakura, arcade_rival_dialogue, stage_objects_34_hawk, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions and free_battle.

## 0.131.0
### For players
- **More of the game decompiled:** 44 more small functions, among them 16 step dispatchers, objects moving by their speeds and counting down, fighters' script starts, the network session's list and group scan, and this PSP's address and nickname. Nothing plays differently.

### Technical
- `src/core/object/batch_steps3.c` (new): the other 28 ready functions of 9 - 16 instructions (with 0x088DE8F0, left out of 0.126.0). `src/core/object/steps.c`: 16 more step dispatchers (0x088E2F48, 0x088EFF88, 0x088F5B18, 0x08905E14, 0x0895CD24, 0x0895CE40, 0x0895D8A0, 0x0895DAAC, 0x0895E42C, 0x089610B0, 0x08961154, 0x089615F8, 0x08961778, 0x089CF384, 0x089DA5DC, 0x089DDE90). `src/core/psp_os.h`: wrappers for sceNetGetLocalEtherAddr, sceUtilityGetSystemParamString and sceNetAdhocctlScan.
- Checked: the coverage pass maps all 44 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_ending_full, arcade_match, free_battle_rows, super_finish, pause_config_original, stage_objects_03_ken, stage_objects_19_gen, arcade_join_select, arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, option_tour, no_extensions, free_battle and arcade_rival_dialogue.

## 0.130.4
### For players
- **The ending watch scenarios go straight to the fight:** in `ending_arcade`, `ending_final`, `ending_rdramatic` and `ending_variable` the confirm presses no longer run on into the fight (they landed as a string of attacks before anyone met). From the moment the fight can be played both characters walk towards each other, then player 1 jabs; the first hit wins.
- **`--no-scaled-endings`** for `tools/watch_scenario.py`: the endings with black sides, as with Scaled endings off (the same as `--features ...,-scaled_endings`).

### Technical
- The fight's first playable frame (GAME + 2 = 2, + 4 = 3, + 6 = 3) measured per scenario with RYU: arcade 3788, rdramatic 2800, variable 2891, final 2905. Inputs from 60 frames before it to the end of the old walk / jab cycle are replaced: right and pad 2 left at it, right released 60 frames on, then every 60 frames square, square 18 later, right for 24. `ending_kumite` and `ending_dramatic` already walked in from before the fight and are unchanged.
- Checked: RYU and ZANGIEF in all four: the fight is won 60 - 70 frames after it can be played, the characters meeting first; Sagat's Final Battle ending with `-scaled_endings` (black sides).

## 0.130.3
### For players
- **WIDE NATIVE, Sakura's ending:** Ryu lying at the left edge is drawn whole.
- **Final Battle's watch scenario no longer shows a pre-fight dialogue the game never has.** `ending_final` forced the fight's number to 9 (Arcade's last fight), which made Final Battle start Arcade's M. Bison dialogue with both portraits left at the top. Final Battle in real play has no dialogue, so nothing in the game needed fixing; 0.130.2's change for those lines is taken back.

### Technical
- `core/object/sprite.c`: `INTO` 16 (was 32): Ryu's hand reaches 29 px into the picture in Sakura's ending; Cody's waiting partner 8.
- `extensions/widescreen.c`: `widescreen_text_shift` back as in 0.130.1.
- `tests/watch/ending_final.txt`: the `092E123C=9` poke removed (Chun-Li's two-fight Final Battle still reaches M. Bison and her ending without it).
- Checked: Sakura's Final Battle ending (Ryu at the left edge), Cody's (partner hidden until he walks in), Chun-Li's and Sakura's Final Battle without the poke (no dialogue, ending reached); Arcade's last fight forced to 9 before the map shows the opponent's portrait bottom-right, as the rival fight. Parity (lazy renderer): arcade_ending, arcade_ending_full, arcade_rival_dialogue, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.130.2
### For players
- **WIDE NATIVE endings: no pop-in when the camera pans.** A character carried into view by a pan (Ryu lying on the ground, Sagat behind M. Bison in Sagat's ending) is drawn whole as soon as it is on the screen, and stays whole when the pan stops at the old 4:3 edge. Someone waiting off the picture to walk in still stays hidden until they move.
- **Final Battle's pre-fight dialogue in WIDE NATIVE:** the lines at the bottom of the screen are now placed as Arcade's are. Final Battle shows both portraits at the top, so these lines weren't recognised as dialogue and stayed where the 4:3 screen had them.

### Technical
- `core/object/sprite.c`: an ending's sprite is "seen" once its anchor moves on the screen (walking, or with the camera: `DS_POS_X` after the anchor, so a pan counts though the sprite's own x stays) or its cells reach 32 px into the 4:3 picture; from then on it is drawn across the 16:9 screen for the rest of the scene (an entry resets when the sprite isn't drawn for 2 frames or its kind / type changes). Replaces 0.130.1's "still for 30 frames" test, which cut sprites carried in by a pan.
- `extensions/widescreen.c` `widescreen_text_shift`: with no dialogue portrait (kind 14 type 6) in a line's half of the screen, any portrait shown makes it dialogue.
- Checked: Sagat's Final Battle ending through the pan (frames 4560 - 5000); Cody's partner still hidden until he walks in; E. Honda's ending; Final Battle's dialogue (Sagat) beside Arcade's (arcade_rival_dialogue). Parity (lazy renderer): arcade_ending, arcade_ending_full, arcade_rival_dialogue, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.130.1
### For players
- **WIDE NATIVE endings: characters and scenery standing at the old 4:3 edge are drawn whole.** Sagat walking in behind M. Bison in his ending, E. Honda reaching in from the left, Ryu lying at the edge in Sakura's ending, and the temple and hill on the left of Ryu's sunset stage (Ryu's, Sagat's and Sakura's endings) were still cut at the old edge: they were placed just past it and stood still, so they were taken for someone waiting off-screen. Someone wholly off the picture still stays hidden until they move.

### Technical
- `core/object/sprite.c`: an ending's sprite placed outside 0 - 384 that hasn't moved for 30 frames is cut at 4:3 only if, as drawn last frame, its cells reached less than 32 pixels into the 4:3 picture (`INTO`: a cell's edge is often transparent: Cody's partner's reach 8). `sprite_grid.c` records the leftmost and rightmost x of a sprite's cells while it is drawn (`sprite_grid_extent_*`), before any cut. The temple and hill are an ending sprite (kind 79 type 18 at x -56), not part of the stage layers.
- `tools/ending_stages.py`: each of the stage's layers in a band now has its own offset (`StageSource` dx / dy), and plain blocks (4 colours or more) count, so a sky band can be matched; one more band (ending 16 on st16). The table is otherwise as before.
- `docs/TESTS.md`: `ending_final` as the quickest way to an ending.
- Checked: every character's Final Battle ending (ending_final) in WIDE NATIVE against 0.130.0, every 50 frames; 6 endings differ (Ryu, Sagat, Sakura, E. Honda, Cody, Charlie) and each was looked at. Parity (lazy renderer): arcade_ending, arcade_ending_full, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.130.0
### For players
- **WIDE NATIVE endings on Ryu's stage show the whole stage:** in Ryu's, Sagat's and Sakura's endings the sunset stage no longer stops at the old 4:3 edges (the black block bottom left, the flat orange on the right): the rocks, trees and cliffs carry on to the screen's sides, taken from Ryu's stage itself.

### Technical
- Decompiled 0x08819300 (`core/stage/background.c`, `draw_layer_16_tall`): the endings' layer 1, a 16-pixel layer from a map of 64 x 64 tiles scrolled by its own +18 / +98. Wrapped for the scaled endings (`scaled_endings_stage_layer`).
- The ending's own copy of a stage scene (endNN_u.pac's maps and textures) holds only the 24 columns of the 4:3 picture. `tools/ending_stages.py` compares every ending's layer bands with every stage's layers, 16 x 16 blocks as colours, and writes `src/extensions/scaled_endings_stages.h`: the band (checked by a crc32 of its map words), the stage, its offset and which of the stage's layers in which order make the ending's tiles (Ryu's: layer 2's 32-pixel tiles under layer 1). Kept only when they give back at least 60 % of the band's blocks: 7 bands (endings 00, 0B and 0D on st00; 07 and 27 on st07; 05 on st05; 28 on st21).
- `src/extensions/scaled_endings_stages.c`: after 0x08819300 draws, in a scene shown wide (tiles_ending_mode 0), a band of the table found in the layer's map gets 8 columns each side drawn from the stage's archive into a texture of our own (5551; the 4-bit textures in tools/data/palette_pairs.json's colours) and drawn at the layer's depth, placed by its scroll. `rt_archive_read` (platform/hle_io.c) reads a data/ archive whole as the game's own opens would (assets/ first). Only the drawing changes.
- Checked: Ryu's, Sagat's and Sakura's stage scenes drawn in WIDE NATIVE; every character's Arcade ending run and its log read: the fill is drawn for Ryu, Sagat, Sakura, Guy and Maki only (Guy's and Maki's columns fall off the screen). Parity (lazy renderer): arcade_ending, arcade_ending_full, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.129.0
### For players
- **WIDE NATIVE endings, fading out after a button press:** pressing a button during an ending no longer flashes the scene back to 4:3 with black sides while it fades; it fades as it was shown.
- **Characters walking in or out of an ending's scene are drawn whole:** Guile, M. Bison, Chun-Li, Charlie's plane and the rest are no longer cut off at the old 4:3 edge as they cross it. Someone waiting off-screen to walk in still stays out of sight until they move.
- **Character art over a stage background keeps its size:** the last portrait before THANK YOU FOR PLAYING and scenes such as Sagat and Ryu under the setting sun now scale only the background to fill the screen; the character is drawn at its own size, so heads and shoulders are no longer cut off. Art drawn right to the old 4:3 edges (Evil Ryu, Rose) still grows with the background, so its straight edge stays off-screen. The endings' own drawn scenes (close-ups, statues, bursts, Charlie in his cockpit) still fill the screen as one picture.

### Technical
- `core/task/ending.c`: 0x089DE390 (the draw-only step after a button press) loops over many frames in one call; `widescreen_ending_more()` (widescreen.c; stand-in in extensions_off.c) marks each of them as an ending's frame, so `widescreen_in_ending` stays true through the fade.
- `core/object/sprite.c` `ending_sides`: with the scaled endings on, an ending's sprites are clipped to the 16:9 screen, not the 4:3 picture; a sprite whose x is outside 0 - 384 and hasn't moved for 30 frames (`parked`, tracked by sprite address) is still clipped at 4:3.
- `extensions/scaled_endings.c` `scaled_endings_oversize`: pictures (kinds 73 / 77 / 78) grow with a scaled scene only in ending kind 2 (0x09358BE0 + 308: the endings' 24-tile map); over kinds 0 / 1 (a stage's layers) the picture is measured first (`sprite_grid_measure`: its cells' leftmost and rightmost x, nothing drawn) and kept at its size unless it reaches a 4:3 side (`sprite_ending_oversize` 2 -> 1 or 3, decided once for the sprite and what is linked to it).
- Checked: every character's Arcade ending (ending_arcade, 37 characters) drawn in WIDE NATIVE with the old and new builds every 50 frames, the differing frames listed and a sample of each character's looked at; the button-press fade (Guile, frame 11000). Parity (lazy renderer): arcade_ending, arcade_ending_full, attract_mode, no_extensions; WIDE NATIVE check: wide_fight, wide_walls.

## 0.128.2
### For players
- **The documents brought up to date with EX. SETTING's pages:** PLAYERS.md (every page and row, the unlocks and how the game earns them), SAVES.md (forced unlocks are never saved; `[cheats and unlocks]` and `[progress]` in a3mx.cfg; DELETE resets every page), BUILDING.md, KNOWN_ISSUES.md (N8: NETWORK with VIRTUAL PSPS off, worked round), ARCHITECTURE.md, LAYOUT.MD and ROADMAP.md.

### Technical
- Docs only; nothing in the program changed.

## 0.128.1
### For players
- **EX. SETTING's greyed rows (4P) in a new grey:** solid grey with a dark outline, and under the cursor a lighter grey with a dark red outline. No gradient. The cursor can now land on them; their values can't be changed.

### Technical
- `exsetting.c`: font 1 gets two colours of its own, 14 and 15, for good (the first time EX. SETTING opens): a ninth texture page (page table entry 0x3F2) and a ninth descriptor in a copy of the font list (0x08A05080), made from colour 10's glyphs (14: dark outline, grey inside; 15: colour 9's red outline, light grey inside). Colour k of font 1 is at rows 144 k + 32 to + 80 of the font's pages, so 14 and 15 fall in the ninth.
- Checked in WIDE NATIVE (page 3/5, a greyed row unselected and selected); parity for option_tour and attract_mode.

## 0.128.0
### For players
- **EX. SETTING has five pages with Cheats and Unlocks:** 1/5 the unlocks, 2/5 1P and 2P HEALTH / GUARD / SUPER, 3/5 3P's (4P shown greyed out: there is no fourth fighter yet), 4/5 World Tour and EDIT MODE, 5/5 PRIORITY and VIRTUAL PSPS.
- **The unlocks one by one** (page 1/5, all ON by default): SHIN AKUMA, FINAL M.BISON and EX BALROG (SELECT held when choosing them), SHIN AKUMA BATTLE (L + R on FINAL BATTLE), BATTLE VARIANTS (L + R on DRAMATIC, VARIABLE and R.DRAMATIC), EDIT MODE POINTS (9999) and ISM-PLUS (all 26). UNLOCK ALL is gone. A forced unlock is no longer written into your save: OFF goes back to what your save has really earned.
- **EDIT MODE EXP, X-ISM, A-ISM and V-ISM EXP** (page 4/5): MAX gives the character EDIT MODE is making full EXP and Lv 32 (that ISM's Lv 32, MASTER).
- **VIRTUAL PSPS OFF:** the MODE MENU's NETWORK tile is grey and can't be chosen (going in with the network stubbed crashed the game).
- Every row is kept in a3mx.cfg (`[cheats and unlocks]`: `shin_akuma`, `final_m_bison`, `ex_balrog`, `shin_akuma_battle`, `battle_variants`, `edit_mode_points`, `ism_plus`, `edit_exp` ...); `unlock_all` is no longer read.

### Technical
- The unlock bits found (docs/PSP_PROGRAM_MAP.md): the system data's +0x52 bits by mode cleared (0x0882AE20) and Arcade (0x089DE194), their readers (SELECT's extras, the MODE MENU's L + R variants, EDIT MODE's points at 0x088D43BC), +0x50 bits 0 - 2, the ISM-Pluses (+0x200, 0x0883B800). No character is hidden in MAX (the random box's pool has none).
- `cheats_unlocks.c`: the unlock rows force bits each frame from the moment the save is in; the bits the game earns are followed (hooks on 0x0882AE20 and 0x0883B800, a wrapper on 0x089DE194: the forced bits are off while they run), and `cheats_unlocks_unforce` puts the options piece back to the earned bits wherever it is saved (savestore.c's image, a3mx.dat / a3mx.cfg, and hle_sys.c's memory-stick save). A load takes the forced bits off until it is in. The one Arcade bit serves four rows: `src/platform/unlocks.h`'s `rt_unlock_blocked` (0 without the extensions) is asked by cursor.c, battle_select.c and mode_menu.c for the ones that are OFF and not earned.
- EDIT MODE (mode 14) edits the entry record 0x09358528 + 0x354 n: the EDIT MODE rows set its EXP and levels there (not its STATUS).
- `exsetting.c`: five pages; rows can be greyed (colour 8) and passed by the cursor; `exsetting_mode_menu` (from padicons_mode_menu, before 0x0888C5B0) greys texture page 0x30's NETWORK tile (176 x 31 at y 192; kept and put back when VIRTUAL PSPS is on again) and drops Cross / START on it.
- Checked: SELECT on Akuma gives Shin Akuma with SHIN AKUMA ON and Akuma with it OFF while FINAL M.BISON keeps the bit forced; with the defaults the memory has every bit and a3mx.cfg's `[progress]` keeps the earned 0000 / 00011B2F; all rows OFF leaves the memory as the save; EDIT MODE EXP and A-ISM EXP MAX set the record's EXP and Lv 32; NETWORK greyed, bright under the cursor, Cross and START do nothing; EX. SETTING's five pages in WIDE NATIVE. Parity (lazy renderer) for attract_mode, arcade_fight, option_tour, arcade_ending, mode_tour, save_load, select_choices, free_battle_rows and no_extensions.

## 0.127.2
### For players
- **WIDE NATIVE, M. Bison's stages, corrected:** the trees at the screen's sides now sway with the rest of the background at both walls, with no seam and no flicker. 0.121.1 fixed only the left side, and with the wrong picture.

### Technical
- `core/stage/tiles.c`: the animated 32-pixel layers (kinds 0xDA / 0xEE, stages 0x0A / 0x1E) keep each map row as one strip of pictures; a frame's window starts 24 columns on from the last's and its own picture is the window's columns 8 - 31 (0 - 7 are the previous frame's end, frame 0's empty; 32 on the next frame's start). In WIDE NATIVE their columns now wrap round within the frame's own picture (`anim_wrap` in draw_grid), on both sides; 0.121.1's frame-10 tiles for frame 0 are gone. NORMAL and WIDE draw as before.
- Checked: the map's tile words logged for every frame; stage_objects_29_bison1 drawn in WIDE NATIVE over twelve frames at each wall (fighters held at the right wall with --poke), the joins' colour step (16:9 x 39 at the left wall, x 442 at the right) no bigger than the art's own tile edges; parity passed for stage_objects_29_bison1, stage_objects_30_bison2 and wide_fight.

## 0.127.1
### For players
- **WORLD TOUR EXP MAX now maxes the character too:** Lv 32 and every STATUS value (ATTACK, SC ATTACK, DEFENSE, GAUGE, STAMINA) at its limit, at once; X-, A- and V-ISM EXP MAX put that ISM at Lv 32 (MASTER). Before, the EXP was full but the levels stayed at 1. SAVE the World Tour data to keep it.
- **LOAD DATA in World Tour and LOAD in ENTRY MODE:** the confirm sound is no longer cut short.

### Technical
- `cheats_unlocks.c`: the World Tour entry's levels (+4 the total, +5 / +6 / +7 X-, A-, V-ISM, level - 1, 31 = Lv 32) and its five STATUS values (+0x26 - +0x2E) set to their limits (+0x30 - +0x38), found by poking and looking at World Tour's status screen.
- `sysdata.c`: the save / load task's off-screen hold is paced in World Tour and ENTRY MODE (mode 3 / 4), and lasts 40 frames after the load (~9 frames): frames run at full speed had played the confirm sound faster than real time, and the audio queue dropped the rest.
- Checked: the status screen with WORLD TOUR EXP and A-ISM EXP MAX (Lv 32, A-ISM Lv 32 MASTER, STATUS 120 / 120 / 120 / 120 / 140); a World Tour SAVE then LOAD DATA still loads; parity for mode_tour, save_load and arcade_fight. The sound itself can't be heard here: please listen.

## 0.127.0
### For players
- **Cheats and Unlocks:** OPTION MODE > EX. SETTING now has pages. Page 1/3: 1P, 2P and 3P HEALTH (NORMAL, INFINITE, HALF, NONE: one hit knocks out), GUARD (NORMAL, INFINITE, HALF, NONE: the next guard crushes) and SUPER (NORMAL, INFINITE, LV1 - LV3, NONE), for each fighter slot, the computer's too, in every mode but network battles. Page 2/3: World Tour EXP and its X-, A- and V-ISM EXP at MAX, and UNLOCK ALL (what the launcher's *Everything unlocked* did, on by default). Page 3/3: the options EX. SETTING always had.
- **EX. SETTING's pages:** side arrows and the page number above the version, as on SCORE RANKING; L / R turn the page anywhere, left / right on DEFAULT and EXIT. DEFAULT puts back only the page you are on. A line or two under the rows says what the row under the cursor does. In NORMAL, WIDE and WIDE NATIVE. With the Cheats and Unlocks extension off, EX. SETTING is only its own page, 1/1.
- **COUCH CO-OP is now VIRTUAL PSPS:** NETWORK finds pads 2 and 3 (or the computer) as other PSPs, all on this PC.
- **The launcher's Extensions tab in three sections:** Core (Silent saves, Boot to the MODE MENU, MODE MENU icons, the F2 / SELECT + START key, Extra menus and settings), Display (Widescreen, I-ISM ranking and wipe, Scaled endings, Stage Extensions, Shaders) and Extra (Cheats and Unlocks, Local multiplayer, I-ISM for both sides).
- **I-ISM is now two extensions:** *I-ISM ranking and wipe* (`i_ism_display`: the I-ISM score ranking in the attract mode and the yellow I / gold logo wipe into the fight), now **off by default**, and *I-ISM for both sides* (`i_ism_select`: an entry character can be picked for the other side where a mode allows it), on by default.
- **Boot to the MODE MENU is now on by default.**
- The MODE MENU bar extension is now called *MODE MENU icons*.
- Not carried over (none needed before 1.0): your Extensions ticks for `hud`, `i_ism` and `unlocks`, and `couch_coop` in a3mx.cfg (now `virtual_psps`): check the tab and EX. SETTING once.

### Technical
- `ext_features.h`: `hud` renamed `mode_menu_icons`; `i_ism` split into `i_ism_display` and `i_ism_select`; `unlocks` replaced by `cheats_unlocks` (`FEAT_CHEATS_UNLOCKS`, on by default); the enum and table in the launcher's order, each feature with a `section` (`FEATURE_SECTIONS`: Core, Display, Extra), drawn as headings. Defaults: `boot_to_menu` on, `i_ism_display` off. Old names are refused.
- `src/extensions/i_ism.c` split into `i_ism_display.c` (wrappers `i_ism_display_*`) and `i_ism_select.c` (`i_ism_select_bselect_grid`); the wrapper names in `src/core` follow.
- `src/extensions/cheats_unlocks.c` (new): `cheats_unlocks_frame`, from `sysdata_frame`, while a round is on (GAME + 2 = 2, + 6 = 3: not the attract demos) and the mode isn't 1 or 12. Fighter offsets measured on the PSP program: health +0x70, full +0x72, super gauge +0x16E (0 - 144, levels of 48), guard gauge size +0x22C / damage +0x22D; World Tour EXP in the entry each side plays as (0x09358528 + 0x354 n: +8 the total, +0xC / +0x10 / +0x14 the ISMs'). Settings in a3mx.cfg's `[cheats and unlocks]` (settings.c); SAVE LOAD > DELETE resets them. `sysdata.c` holds the unlocks only with the feature and UNLOCK ALL on.
- `exsetting.c`: EX. SETTING as pages of rows (label, value, names, description); strings for the text queue kept in a pool of ours; the side arrows drawn into our texture page and shown by two objects like L2's 2; rows 14 apart from y 0x1C; the green bar placed on the row. `rt_couch_coop` renamed `rt_virtual_psps`.
- `--features` with an unknown name lists the names from the table; the features' description buffer is 256 bytes.
- Tests keep running what they ran before: `tests/parity.py` `TEST_FEATURES` = `default,-shaders,-boot_to_menu,i_ism_display` (also `tools/watch_scenario.py`'s default and `tests/gl_test.py`); `boot_to_menu.txt` adds `i_ism_display`; the stage measuring tools turn `boot_to_menu` off.
- Checked: parity (memory hashes, lazy renderer) for option_tour, attract_mode, free_battle_rows, arcade_fight and save_load; EX. SETTING's pages looked at in NORMAL, WIDE and WIDE NATIVE, and as 1/1; the cheats in arcade_fight (INFINITE health, LV2 super, HALF health, NONE guard and super all held) and World Tour EXP MAX in a World Tour fight.

## 0.126.0
### For players
- **More of the game decompiled:** 44 more small functions, among them 14 step dispatchers, objects moving by their speeds and counting down, the network session's list, checksum and flags, and a fighter's script start. Nothing plays differently.

### Technical
- **The batch tool's profile refreshed:** the list of translated functions the 69 scenarios reach (decomp-tools prof/union.txt, last made at 0.82.0) was made again with a counting build: 2,051 reached, 781 of them new to the list, so 307 functions of 9 - 400 instructions are ready instead of 12. The function count itself (14,881) was checked against the program: the functions cover all its code, with at most about 10 small ones merged into others.
- `src/core/object/batch_steps2.c` (new): 30 of the 87 ready functions of 9 - 16 instructions. `src/core/object/steps.c`: 14 more step dispatchers (0x08824F28, 0x08831B34, 0x08832C28, 0x08845530, 0x0884559C, 0x08874400, 0x0887E950, 0x0887F250, 0x0888A364, 0x0888A920, 0x088EB67C, 0x088EB6B0, 0x088EBEC4, 0x088EF988). Left out: 0x088DE8F0 (two system calls, sceNetGetLocalEtherAddr and sceUtilityGetSystemParamString, with no wrapper yet).
- Checked: the coverage pass maps all 44 to scenarios; parity (memory hashes, lazy renderer) passed for network_match, arcade_rival_dialogue, arcade_match, free_battle_rows, stage_objects_03_ken, arcade_ending_full, super_finish, free_battle_dramatic, stage_objects_15_sakura, pause_config_original, stage_objects_19_gen, attract_mode, arcade_fight and no_extensions.

## 0.125.0
### For players
- **More of the game decompiled:** 10 more functions, among them six characters' set-ups when a fight starts (their move tables, shadows and companion objects), another character's data unpacked, a score put into the attract mode's rankings, and a fighter's name panel. Nothing plays differently.

### Technical
- `src/core/object/batch_large6.c` (new): the eight made ready by 0.124.0 (0x08896160, 0x08891FA0, 0x088ED71C, 0x08929B8C, 0x08908F68, 0x0890DEC4, 0x0899A430, 0x08982ECC) and two of 166 - 168 instructions (0x08892D6C, 0x0896A6C4). Tables in docs/PSP_PROGRAM_MAP.md.
- Checked: the coverage pass maps all ten to scenarios (arcade_match, attract_mode and arcade_other_fighter run them all); parity (memory hashes, lazy renderer) passed for arcade_match, attract_mode, arcade_other_fighter, arcade_ending, free_battle_team, free_battle_dramatic, network_match, arcade_fight, arcade_rival_dialogue, arcade_ending_full, super_finish and no_extensions, and the paired Survival R.DRAMATIC run.

## 0.124.0
### For players
- **More of the game decompiled:** 10 more functions, among them how much damage a hit does (by character, level, combo, health and the hit's strength), a hit's effect and its score, a character's data unpacked when it is loaded, whether a button counts for a fighter (Training's guard settings included), a fighter's name and win count on screen, and screens that reset the fight as they start. Nothing plays differently.

### Technical
- `src/core/object/batch_large5.c` (new): the four made ready by 0.123.0 and 0x08857BD0 (0x08896190, 0x089DE3F4, 0x088B2FE8, 0x088BE118, 0x08857BD0), and five of 146 - 159 instructions (0x08872D20, 0x0886EB48, 0x08988FAC, 0x088758E8, 0x08892020). Tables in docs/PSP_PROGRAM_MAP.md.
- Checked: the coverage pass maps all ten to scenarios (arcade_ending and attract_mode run them all); parity (memory hashes, lazy renderer) passed for arcade_ending, attract_mode, arcade_fight, arcade_match, arcade_other_fighter, network_match, super_finish, free_battle_team, free_battle_dramatic, arcade_ending_full, arcade_rival_dialogue, no_extensions and stage_objects_13_sagat, and the paired Survival R.DRAMATIC run.

## 0.123.0
### For players
- **More of the game decompiled:** 12 more functions, among them the ending task itself, the fight's resets, when a fighter may throw and when one gets up early (a human by buttons or Training's setting, the computer by its level), the score a hit is worth, the score panel's digits and the entries' panel. Nothing plays differently.

### Technical
- `src/core/object/batch_large4.c` (new): the eight functions made ready by 0.122.0 (0x08821FC8, 0x08821F9C, 0x0885EB2C, 0x088B4D88, 0x08821FEC, 0x089DE0E8, 0x088CE274, 0x08862484) and four of 137 - 142 instructions (0x088586C8, 0x08887EE8, 0x08857810, 0x08866A84). Tables in docs/PSP_PROGRAM_MAP.md.
- Checked: the coverage pass maps all twelve to scenarios (arcade_ending, attract_mode and mode_tour run them all); parity (memory hashes, lazy renderer) passed for arcade_ending, attract_mode, mode_tour, arcade_match, arcade_fight, arcade_other_fighter, network_match, super_finish, arcade_rival_dialogue, arcade_ending_full, free_battle_team, free_battle_dramatic and no_extensions, and a paired Survival R.DRAMATIC run (6,000 frames, NORMAL).

## 0.122.0
### For players
- **More of the game decompiled:** 10 larger functions, among them how much a hit takes off and the spark it makes, a fighter's life at the start of a round in each mode (Survival's kinds, TEAM, network battles), the select screen's cursor set up for a player and the entry characters put into its grid, the entries' names and numbers, and a screen's set of objects. Nothing plays differently.

### Technical
- `src/core/object/batch_large3.c` (new): the ten smallest of the 31 ready functions of 61 - 400 instructions (`ready.py 61 400`, 93 - 136 instructions): 0x0885EB74, 0x088B1ED0, 0x08871AC4, 0x08865BEC, 0x08875DBC, 0x088CE33C, 0x088939DC, 0x088625C0, 0x08896DE8, 0x088CD7E4. Tables in docs/PSP_PROGRAM_MAP.md.
- Checked: the coverage pass maps all ten to scenarios (arcade_other_fighter, mode_tour and no_extensions run them all); parity (memory hashes, lazy renderer) passed for arcade_other_fighter, mode_tour, no_extensions, attract_mode, arcade_fight, arcade_join_select, network_match, free_battle_team, select_choices, arcade_ending, free_battle_dramatic, arcade_rival_dialogue, arcade_match and super_finish; also a Survival R.DRAMATIC run (6,000 frames, reference and decomp paired, NORMAL), where 0x08865BEC takes its Survival branch.

## 0.121.2
### For players
- **WIDE NATIVE, Survival's results after a round:** the four rows (TIME, the ISM's score, MAX COMBO, VITAL) now fly in to the screen's right edge, as the five-row panel of the other modes does, instead of stopping at the old 4:3 side. This is the panel of every Survival variation (R.DRAMATIC and R.DRAMATIC with lives included) and of Free Battle's TEAM.

### Technical
- `extensions/widescreen.c`: `panel_piece` takes the round-win panel's other pair of parts too. 0x08855BD8 picks them from 0x08A16DF8: kind 1 types 9 / 15 (five rows), or 8 / 7 (four rows) in mode 7 (Survival, all eight variations), mode 6 and mode 0x10 with BATTLE + 159 = 2 (Free Battle TEAM); the table has no other pair. Both stop at x 240, so the same shift (-48 at x -144 to +48 at x 240) applies.
- Checked: Survival R.DRAMATIC drawn in WIDE NATIVE through the panel (frames 4200 - 4330 of a headless run, opponents held at 1 health); found by logging the pool's kind 1 objects with their maker (removed). Parity passed for wide_fight, free_battle_team and arcade_fight.

## 0.121.1
### For players
- **WIDE NATIVE, M. Bison's stages:** the trees and the ground's dark band at the screen's left side no longer blink out for a moment every second or so near the left wall (M. Bison's first stage, as in Final Battle, and his second).

### Technical
- `core/stage/tiles.c`: the animated 32-pixel layers (kinds 0xDA / 0xEE, stages 0x0A / 0x1E: 11 frames 24 map words apart in each 0x110-word row) leave map columns 0 - 7 empty in frame 0, columns the 4:3 screen never reaches; WIDE NATIVE's two extra columns on the left did, for 6 frames in every 66. An empty extra tile there now comes from frame 10, the frame before. NORMAL and WIDE draw as before.
- Checked: frame 0 of both layers logged (columns 0 - 7 empty, the other ten frames full); stage_objects_29_bison1 drawn in WIDE NATIVE before and after; parity passed for stage_objects_29_bison1, stage_objects_30_bison2 and wide_fight.

## 0.121.0
### For players
- **More of the game decompiled:** 25 more functions, among them the CPU choosing a throw or an attack against its opponent, the start of the CPU's scripts, the fight's objects reset before each fight, the camera lines' starts, and particles and panels set up. Nothing plays differently.
- **I-ISM, checked:** player 2 can already pick an entry character on Arcade's select screen (after joining with Start) and play it; no change was needed.

### Technical
- `src/core/object/batch_mid12.c` (new): the 25 ready functions of 11 - 76 instructions (`ready.py 9 80`): 0x089CFFC4, 0x088B4C40, 0x089E0144, 0x088AAD0C, 0x088ADEF0, 0x0892EEE8, 0x08869A88, 0x0887E420, 0x08821F58, 0x0885EE74, 0x088B4F2C, 0x088790C4, 0x088BD180, 0x089DEC88, 0x088788D4, 0x0887B7FC, 0x089CF6A8, 0x089E0054, 0x089E00CC, 0x089E3400, 0x08877448, 0x088B40E0, 0x08822040, 0x088ABC3C, 0x088B5074. Tables in docs/PSP_PROGRAM_MAP.md.
- Checked: the coverage pass maps all 25 to scenarios (attract_mode, arcade_ending and arcade_other_fighter run them all); parity (memory hashes, lazy renderer) passed for attract_mode, arcade_other_fighter, arcade_ending, arcade_fight, boot_to_select, arcade_match, network_match, super_finish, free_battle, free_battle_team, mode_tour, arcade_join_select, no_extensions, option_tour, stage_objects_10_guy and free_battle_dramatic.

## 0.120.0
### For players
- **WIDE NATIVE, endings: the new Scaled endings extension** (on by default; launcher **Extensions** tab, `--features ...,-scaled_endings` to turn it off). A scene with its own art past the 4:3 sides (scrolling skies, stages) shows it; any other is drawn 1.25 times larger, layers and pictures together, as the super finish's backdrops are, so it fills the 16:9 screen. Profile pictures move to the screen's edges.
- **With Scaled endings off** the endings keep black sides: nothing of the scenes, nor anything waiting off the 4:3 screen, shows in the margins, and nothing moves.
- **No more mirrored endings:** the speed-line and vertical-line scenes are no longer carried on mirrored; they are scaled (or black-sided) like the rest.
- **WIDE NATIVE, the endings' lines:** before the credits they are laid out across the screen, centred, with longer lines; while the staff credits roll they move into the left margin, giving the credits more room.

### Technical
- `src/core/task/ending.c` (new): the ending's task and its layers decompiled: 0x089DE2C0 (the step), 0x089DE390 (the draw-only step), 0x089DE444 / 0x089DE4B4 (the layers' places), 0x089DE7C0 (timed fade-out, 0x0887C708, sound 0xB4), 0x0881A2C8 (the three layers by the ending's kind, 0x09358BE0 + 308: 0 a stage's, 1 16/32-pixel layers, 2 the 24-tile map 0x0881B7C8; maps from 0x08A07918).
- `extensions/widescreen.c`: an ending's frame is noted (0x089DE2C0 / 0x089DE390 wrapped, `widescreen_in_ending`); the staff credits are known by a text asked for through the text queue (0x0880A6CC, `core/text/queue.c`) above y 240.
- `extensions/scaled_endings.c` (new feature `scaled_endings`, needs `widescreen`). The scene decision is in `ending.c`'s 0x0881A2C8: scaled when a layer stands still on a multiple of 384 (its map's 4:3 pictures side by side), or when no tile layer has art past both 4:3 sides (each layer's columns counted first, `tiles_counting` in `tiles.c`); then the tile layers, the picture layers (`background.c`) and the sprites of kinds 73, 77 and 78 with what is linked to them (`sprite.c`, `sprite_drawers.c`) are drawn 1.25 times larger about (192, 72); profile pictures (kind 74, x 64 / 320) move 48 out. Off: `tiles_ending_quad` cuts every layer quad at the 4:3 sides.
- `core/object/sprite.c`: in an ending a sprite tree is cut at the 4:3 sides (`sprite_grid_clip_x`), a moved profile picture at the 16:9 edge on its side.
- `core/stage/tiles.c`: 0x0881B7C8's mirroring (0.99.22) removed; `scene_mirror` is only 0x0881AB28's kind 5 now (its columns round its 12).
- `core/object/text_boxes.c`: an ending's lines: while the credits roll 40 further left; otherwise joined, wrapped at 44 characters and centred on x 192, only the characters typed so far shown.
- Checked: parity arcade_ending, arcade_ending_full (the ending task and layers decompiled), arcade_fight, arcade_rival_dialogue, attract_mode, no_extensions and wide_fight; by eye (headless, every 20 frames) Akuma's, M. Bison's, Chun-Li's, Sakura's, Ryu's and Ken's arcade endings with Scaled endings on, Chun-Li's off.

## 0.119.1
### For players
- **The I-ISM ranking's plates:** the small plates behind the faces on the rows are I-ISM's yellow, where they were V-ISM's blue-purple. The faces keep their own colours (0.119.0 turned some of their purples gold too).

### Technical
- The plates are flat translucent boxes, not pictures: `object/batch_large2.c`'s 0x088BDBB8 (`setup_name_box`) colours them by the row's ISM (+92: 1 red E05000, 2 green 50E000, 3 blue 5050E0, at half alpha). It is wrapped (`i_ism_name_box`), and on the I-ISM ranking its colour becomes E0D020. The row palettes 56 - 63 are no longer recoloured.
- Checked: parity passed for attract_mode and no_extensions.

## 0.119.0
### For players
- **The I-ISM ranking:** the game keeps the entry characters' records (their own score table) but the attract mode never showed them. With the `i_ism` feature the rankings after the third demo now go X-ISM, A-ISM, V-ISM, then I-ISM: "I-ISM SCORE RANKING" in gold, the bars and band in I-ISM's yellow. `tests/watch/i_ism_ranking` jumps straight to it.

### Technical
- `src/core/object/ranking.c` (new): 0x088BC164 decompiled, the ranking screen's start: the table of the attract cycle (0x09358400) into 0x09525DE8, the background layer, the first place's portrait colours and the screen's five palettes by the kind's ISM (+14 % 3). The rankings: system data 0x08A05AA8, eight tables of 38 records of 0x14 bytes; 0 - 2 X / A / V scores, 3 - 5 their VS wins, 6 / 7 the entries' (the record writer 0x08892A98 sends an entry's run there).
- `extensions/i_ism.c`: the attract cycle (title.c's 0x0888B8D4 wrapped) goes 0, 1, 2, 6; while table 6 is shown its title's letter (picture 0x2C, V) becomes the unused Z (0x2B), whose cached cell (0x088112A8 wrapped, `sprite_grid.c`) gets an I copied from the "-I" cell of "-ISM"; the screen's palettes, and the rows' as they come in (14 - 19, 56 - 63), turned gold by hue.
- Checked: parity passed for attract_mode (the ranking start runs twice) and no_extensions; a 24,000-frame attract run with Dan's save reaches the I-ISM ranking fourth.
- Known: the small plates behind the faces on the rows stay V-ISM's blue-purple.

## 0.118.6
### For players
- **The I-ISM wipe after a win too:** the wipe from the win screen to the map is the same logo (the single object's type 0x0E); an entry who wins now gets the yellow I and the gold logo there as well, where it was A-ISM's green columns with a leftover gold logo.
- **Not in World Tour:** there an entry's ISM is the one chosen to level up, so the wipes keep that ISM's look, as the game has it.

### Technical
- `src/extensions/i_ism.c`: the fading shapes of types 0x0D and 0x0E; `wipe_is_entry` is false in mode 3 (World Tour).
- Checked: Arcade with Dan's entry 1, the second fight's wipe and the wipe after the win.

## 0.118.5
### For players
- **The other side can play an entry character (I-ISM):** in Free Battle (and the other modes on that select screen: Survival, Training ...), the box for player 2 or the computer can now go onto the entries' row and pick one. The game skipped those boxes for a side no player had joined. Part of the `i_ism` feature; a network battle keeps the game's own rule (each side has its own entries there).
- `tests/watch/i_ism_cpu_entry` shows it: the computer as your entry 1, I-ISM on the VS screen and the yellow I wipe. The watch tool brings in your save for every `i_ism_*` scenario.

### Technical
- `src/core/select/battle_select.c`: 0x088A6A10 (the grid's cursor) wrapped by `i_ism_bselect_grid`, which opens the grid's lock (0x08A2B3A9) on row 6, the entries', while the cursor moves and puts it back after.
- Checked: parity passed for free_battle, free_battle_rows, free_battle_team, free_battle_join_in and no_extensions; the pick and the wipe looked at with Dan's save.

## 0.118.4
### For players
- **The I-ISM wipe's I leaves a band at the top as at the bottom,** as X-ISM's X does: its top serif moved 8 down (the serifs keep their widths; the stem is 8 shorter).

### Technical
- `src/extensions/i_ism.c`: the I's quads (top band to 24, top serif 24 - 56).

## 0.118.3
### For players
- **The I-ISM logo's corner pixels in the outline's hue:** purple #943184 and pink #C64273 (Dan's picks from the World Tour map's NEXT STAGE bar), as X-ISM's are a darker orange and V-ISM's a darker cyan, where they were a muddy blend of the outline and the gold.

### Technical
- `src/extensions/i_ism.c`: GOLD entries 2 and 3.

## 0.118.2
### For players
- **The I-ISM logo's gradient as deep as the other ISMs':** X-ISM's, A-ISM's and V-ISM's run from nearly black to the wipe's colour, so the gold one now runs from a dark brown (#311800) through #C87B01 to the yellow (#F3E40C); the outline is magenta (#B53184, from the World Tour map's NEXT STAGE bar), a bright contrasting line as their orange, yellow-green and cyan are.

### Technical
- `src/extensions/i_ism.c`: the GOLD table.

## 0.118.1
### For players
- **The I-ISM logo's gold, as Dan's second mock-up:** the gradient now runs from #C87B01 to the wipe's own yellow (#F3E40C) in the middle, as the green logo's runs into the wipe's green (ALPHA's H matches the background); the letters' outline is a vibrant purple (#943184) instead of a dull maroon, and their corner pixels are halfway between the outline and the fill, as A-ISM's are. The wipe's yellow is #F3E40C.

### Technical
- `src/extensions/i_ism.c`: the GOLD table and I_YELLOW.

## 0.118.0
### For players
- **I-ISM has its own wipe into the fight (new `i_ism` feature, on by default):** between the VS screen and the fight, an entry character's columns now roll into a yellow I and the STREET FIGHTER ALPHA3 logo slides in gold, where the game gave it its ISM's wipe (an A-ISM entry got A-ISM's green). The I and its yellow were measured from Dan's WIDE NATIVE mock-up, and it works in NORMAL, WIDE and WIDE NATIVE. Off in the launcher's Extensions tab (I-ISM extras) or with `--features default,-i_ism`.
- **Watch scenarios with your own entries:** `--character ENTRY1` - `ENTRY6` picks an entry box on Arcade's select, and the watch tool then plays with a copy of your own save (also `--my-save`; your save isn't changed). `tests/watch/i_ism_wipe` shows the new wipe. `--features` passes the game's features (`-i_ism` shows the original).

### Technical
- `src/core/object/screen_steps.c`: 0x08885A44 decompiled, the logo's start (the single object's step, type 0xD): its six slots from 0x08A257B8, the wipe's shapes and the logo's palette (from 0x08A25826, into palette 0x1E) by the ISM of the fighter the wipe is for (fighter 3 with BATTLE + 56 = 3, fighter 2 with BATTLE + 48, else fighter 1; X-ISM -1, A-ISM 0, V-ISM 1). Wrapped by `extensions/i_ism.c`.
- `object/shapes.c`: the quad drawing split out as `shapes_draw` (the shapes' colours are 0xRRGGBB); 0x0880E434 and 0x0880DA78 (`batch_large.c`) are wrapped by `i_ism.c`, which draws A's three columns as the I while an entry's wipe is shown. An entry is told as the VS screen's ISM label tells it: its player (+337, 0 or 1) has one in use at 0x09358528 + 0x354 n.
- `src/platform/ext_features.*`: the `i_ism` feature (FEAT_I_ISM, functions `i_ism_*`).
- Checked: parity passed for attract_mode, arcade_match, free_battle_dramatic, arcade_fight, super_finish and no_extensions (the logo's start runs in the first five); the wipe looked at with Dan's save (entry 1, I-ISM Ryu) in NORMAL and WIDE NATIVE.

## 0.117.4
### For players
- **The ending scenarios work with any character:** with CHUNLI the opponent still fought back and the round timed out (your run). Now the opponents' computer control is off from before the fight, pad 2 walks them into player 1, and ROUNDS is set to 1, so a jab or two ends it. Final Battle handles the characters whose Final Battle is two fights (CHUNLI: the Dolls, then M. Bison).
- **Watch scenarios play the same on every machine:** the game reads the date, so two runs drifted apart within seconds and a scenario written here could go differently on yours. The watch tool now gives the game a fixed date.
- The super finishes are one round too.

### Technical
- `--date SECONDS` (`src/platform/main.c`, docs/BUILDING.md): the date at frame 0, then the game's own time since, as `--replay` does. `tools/watch_scenario.py` passes 1767225600 (2026-01-01).
- `tests/watch/ending_*.txt`: `config rounds=1`; the CPU flag (fighter + 0x175 = 0), health caps and controller slots (+ 2 = 1) from the fight-number poke on; `--p2 human` and pad 2 holding left through the fight; the fight's input cleared of the original scenario's moves (a jump in ending_arcade swapped the fighters once the character pick delayed it).
- Checked: all six endings headless to the MODE MENU with RYU, CHUNLI and GUILE; super_kind_7 with CHUNLI; two runs of one scenario a minute apart give the same pictures.

## 0.117.3
### For players
- **Character names without spaces or punctuation:** `--character` takes CHUNLI, RMIKA, FEILONG, EHONDA, MBISON, THAWK, DEEJAY, EVILRYU and so on (any case). docs/TESTS.md lists all 38.

### Technical
- `tools/watch_scenario.py`: the names in `GRIDS`, `SUPERS` and `CLOSE_IN` are written without spaces or punctuation; typed ones are still matched with them taken out.

## 0.117.2
### For players
- **The super finishes are watch scenarios of their own:** `tests/watch/super_kind_1` - `super_kind_8`, one per backdrop (slanted lines both ways, vertical lines, a second set of vertical lines, speed lines, splash, burst, swirl). `--super-kind`, `--super-from` and `--super-to` are gone.
- **`--character` works in them too:** the watch tool knows each character's super motion and throws it for whoever you pick, two super finishes a match. JUNI and ZANGIEF have none yet (a normal KO).
- **`--screen NORMAL|WIDE|NATIVE` replaces `--wide`,** so plain WIDE can be watched too. **`--list`** shows every scenario with a line on each.
- **Known issue added (WIDESCREEN.md):** the endings are not fixed for WIDE NATIVE yet; they are being checked by eye with the ending scenarios.

### Technical
- `tools/watch_scenario.py`: Free Battle's select grid (7 x 6, RYU at the top left; names read off the VS screen) beside Arcade's; a scenario's `select FRAME [GRID] [NAME]` line picks the grid; `supers FROM TO` adds player 1's super every 180 frames (`SUPERS`, found by trying QCF QCF / QCB QCB / HCB HCB / charge with punch and kick for each character; `CLOSE_IN` walks in first for the short ones). Scenarios are looked up in `tests/watch` first.
- `tests/parity.py` reads `select` as text and `supers`, both ignored by parity.
- The super scenarios keep player 2's health at most 1 and the backdrop's kind (BATTLE + 43) from 3650 to 5200, as a fight starts later for characters whose data loads longer.
- Checked: super_kind_7 with every character headless: a super finish for all but JUNI and ZANGIEF; the endings unchanged. No game code changed.

## 0.117.1
### For players
- **The ending scenarios no longer depend on winning a real fight:** the opponents' computer control is switched off for the last fight, so they stand still, and your character walks up and jabs. Before, the computer could still beat a weaker character in Kumite (on your machine, Sakura) and the ending never came.

### Technical
- `tests/watch/ending_*.txt`: each opponent's computer-controlled flag (fighter + 0x175) poked to 0 through the fight; in Dramatic, Reverse Dramatic and Variable their controller slot (+ 2) to 3 as well, since they share player 1's slot and would otherwise copy its buttons. The fight's input is now right for 24 frames and two squares, every 60 frames.
- Checked: all six played headless to the MODE MENU with RYU and with Sakura, Cammy, Zangief, Guile, Zangief and Chun-Li.

## 0.117.0
### For players
- **Watch any mode's ending:** `tests/watch/` has six scenarios that play a mode to its end in a few minutes, in a window: `ending_arcade`, `ending_kumite` (STAGE 100 and 100 WINS), `ending_dramatic`, `ending_rdramatic`, `ending_variable` and `ending_final`. The menus go by at full speed, the last fight is won with a hit or two, then the ending, the credits and GAME OVER. `--character NAME` picks who plays (`--character list` shows the names). Watch-only: not parity tests.
- **watch_scenario.py opens in a window by default;** `--fullscreen` for full screen (F11 or Alt+Enter switch while it plays).
- **docs/TESTS.md starts with a step-by-step guide:** building, a quick check, the whole suite, saving the results to a file on Windows, Linux and Mac, what a pass and a failure look like, and how watching a scenario differs from the tests (a real window and the GPU, not headless).

### Technical
- `--poke F-G:ADDR<=VALUE[/SIZE]` lowers a value to VALUE (signed) only while it is above, so a KO'd fighter isn't revived the way a plain poke would (`src/platform/main.c`, docs/BUILDING.md).
- Scenario files take `select F` (the frame of the character pick, for `--character`) and `watch F` (where watching starts at normal speed); `tests/parity.py` reads and ignores both.
- `tools/watch_scenario.py`: `--fullscreen`, `--character` (the select grid at 0x08A26F60 by name; moves the cursor before the pick and shifts later inputs, pokes and frames), scenarios looked up in `tests/scenarios` then `tests/watch`, the default skip from `watch`.
- The endings use BATTLE + 20 (0x092E123C: the fight number; 9 makes the current fight the final one in Arcade, Dramatic, Variable and Final Battle; Kumite's stage count from 0) and fighter 1 + 426 (0x092E404A, Kumite's wins). Survival has no ending (it ends on a score screen), so it has none.
- Checked: each ending played headless to the MODE MENU with RYU and with another character (Chun-Li, Zangief, Guile, Sakura, Cammy; Arcade with Zangief); parity is unchanged (no game code changed).

## 0.116.0
### For players
- **More of the game decompiled:** 7 more functions, among them the pairing of a fight's two fighters (who faces whom, each player's colour), the CPU's aggression nudged at random, a chain of four objects, the camera layers set from their tables, and panels by type. Nothing plays differently.

### Technical
- `src/core/object/batch_mid11.c` (new): 7 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x088CE18C, 0x088B3A84, 0x088B3258, 0x089DE52C, 0x089DDA68, 0x088BE69C, 0x0885CC1C. Tables in docs/PSP_PROGRAM_MAP.md.
- These became ready once 0.106.0 - 0.115.0 had decompiled their callees.
- Checked: the coverage pass maps all 7 to scenarios (attract_mode, arcade_ending and mode_tour run them all); parity passed for attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat

## 0.115.0
### For players
- **More of the game decompiled:** 10 more functions, among them a fighter put into a throw's hold, the continue checks between rounds, the special match for character 9, the demo screen's set-up, and panels lit by mode. Nothing plays differently.

### Technical
- `src/core/object/batch_mid10.c` (new): 10 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x08817950, 0x0883436C, 0x0883311C, 0x0882659C, 0x0886ABB4, 0x08879EB4, 0x08889968, 0x08889200, 0x088B4014, 0x0889B90C. Tables in docs/PSP_PROGRAM_MAP.md.
- These became ready once 0.106.0 - 0.114.0 had decompiled their callees.
- Checked: the coverage pass maps all 10 to scenarios (arcade_ending, attract_mode and option_tour_original run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, option_tour_original, mode_tour, arcade_other_fighter, free_battle_team, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat.

## 0.114.0
### For players
- **More of the game decompiled:** 8 more functions, among them a fighter's damage scaled by the move and its state, the reward count's shapes, the intro's owner by stage, and option panels by mode. Nothing plays differently.

### Technical
- `src/core/object/batch_mid9.c` (new): 8 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x089CD29C, 0x088B4B84, 0x08847CF8, 0x08855BD8, 0x088CF7A4, 0x089E0178, 0x08857F8C, 0x08871D60. Tables in docs/PSP_PROGRAM_MAP.md.
- These became ready once 0.106.0 - 0.113.0 had decompiled their callees.
- Checked: the coverage pass maps all 8 to scenarios (arcade_ending, attract_mode and mode_tour run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat.

## 0.113.0
### For players
- **More of the game decompiled:** 12 more functions, among them a record's timed lines, a fighter's input recorded for its team, an effect that follows a fighter's frame, counters of a screen's pieces, and particles and pieces set up. Nothing plays differently.

### Technical
- `src/core/object/batch_mid8.c` (new): 12 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x088B03C0, 0x088B0590, 0x088B0840, 0x0892ED04, 0x089CD714, 0x088B4C74, 0x088B0F28, 0x088BC944, 0x088ABD4C, 0x089DE8A4, 0x089130D4, 0x08863600. Tables in docs/PSP_PROGRAM_MAP.md.
- These became ready once 0.106.0 - 0.112.0 had decompiled their callees; div's lo / hi kept (0x088BC944).
- Checked: the coverage pass maps all 12 to scenarios (attract_mode and arcade_ending run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat.

## 0.112.0
### For players
- **More of the game decompiled:** 16 more functions, among them the second list of objects run each frame, the CPU's first script picked at random, a fighter's pad byte for its team, and particles and panels set up. Nothing plays differently.

### Technical
- `src/core/object/batch_mid7.c` (new): 16 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x08846B58, 0x089D32D4, 0x088764A8, 0x088B4618, 0x088B47B8, 0x088B3E84, 0x088726EC, 0x088377A8, 0x088B0AA8, 0x088B0DE4, 0x088B2544, 0x088B4174, 0x088EAD20, 0x0888AE14, 0x0887BC44, 0x0890EC3C. Tables in docs/PSP_PROGRAM_MAP.md.
- These became ready once 0.106.0 - 0.111.0 had decompiled their callees.
- Checked: the coverage pass maps all 16 to scenarios (attract_mode, arcade_ending and mode_tour run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat.

## 0.111.0
### For players
- **More of the game decompiled:** 19 more functions, among them the game's random numbers, the fighters' routine tables for a fight, the score for a hit, the CPU's script commands that set its state, a choice on the select screen, and objects' colours from the palette. Nothing plays differently.

### Technical
- `src/core/object/batch_mid6.c` (new): 19 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x08820BE0, 0x088288F0, 0x088782C8, 0x088783B0, 0x08871CA4, 0x088734B4, 0x08887D54, 0x088A44B8, 0x088AA2BC, 0x088BCFC0, 0x088569F0, 0x08892438, 0x088B544C, 0x0896A424, 0x089DE194, 0x089DEA64, 0x0880D3B0, 0x088AA184, 0x08821AA8. Tables in docs/PSP_PROGRAM_MAP.md.
- 0x08820BE0 is the Mersenne twister's tempering (the twist 0x08820A68 was already decompiled), one of the busiest functions left (~38,000 calls in the profile).
- Checked: the coverage pass maps all 19 to scenarios (attract_mode, arcade_ending, arcade_other_fighter and free_battle_team run them all); since 0x08820BE0 runs nearly everywhere, parity ran wider: arcade_fight, boot_to_select, attract_mode, arcade_ending, arcade_other_fighter, free_battle_team, mode_tour, arcade_match, option_tour, network_match, free_battle_dramatic, super_finish and stage_objects_13_sagat all passed.

## 0.110.0
### For players
- **More of the game decompiled:** 19 more functions, among them the run of the first object list, objects made from a description, a fighter's input copied to its team mates, three press counters at once, a file loaded and waited for, and screen objects' set-ups. Nothing plays differently.

### Technical
- `src/core/object/batch_mid5.c` (new): 19 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x08878B88, 0x088728B0, 0x08869AC8, 0x088BC6A4, 0x089E3364, 0x08809934, 0x08821384, 0x08846BBC, 0x0887E7AC, 0x088B14D8, 0x088B4230, 0x088B4DD4, 0x08837854, 0x0884A0B4, 0x088814A0, 0x088BD99C, 0x089D9C04, 0x088230F4, 0x088B0D08. Tables in docs/PSP_PROGRAM_MAP.md.
- 0x089E3364's switch read from its table at 0x08A01DE8; mult's lo / hi kept (0x08821384).
- Checked: the coverage pass maps all 19 to scenarios (arcade_ending, attract_mode, mode_tour and arcade_other_fighter run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match and option_tour.

## 0.109.0
### For players
- **More of the game decompiled:** 23 more functions, among them the stage picked at random for a fight, a fighter's first state, a throw's range check, numbers drawn as pictures, and screen panels and their companions. Nothing plays differently.

### Technical
- `src/core/object/batch_mid4.c` (new): 23 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x088AAC80, 0x088ABB18, 0x088B5AA0, 0x088BCA0C, 0x088576EC, 0x088BCB9C, 0x08855CB0, 0x0885777C, 0x0888AB48, 0x0888ABDC, 0x088AA398, 0x088AAD44, 0x088B1078, 0x088B2204, 0x088B2298, 0x088BC868, 0x088BD1D4, 0x088BE4E0, 0x0892B31C, 0x088CF598, 0x08871E40, 0x0887B9EC, 0x088BE178. Tables in docs/PSP_PROGRAM_MAP.md.
- 0x088576EC keeps div / divu's lo / hi as the original leaves them.
- Checked: the coverage pass maps all 23 to scenarios (attract_mode, arcade_ending and mode_tour run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, arcade_match and option_tour, with 0.107.0 - 0.109.0 built and checked together (each batch's functions are independent of the later ones).

## 0.108.0
### For players
- **More of the game decompiled:** 25 more functions, among them the CPU's script commands that test its opponent, the fighters' press counters, the intro's pieces coming in, the particle field's start, and screen panels flashing. Nothing plays differently.

### Technical
- `src/core/object/batch_mid3.c` (new): 25 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x08877B58, 0x088781C4, 0x08878F40, 0x088793A8, 0x08834558, 0x08834430, 0x08855470, 0x08856C60, 0x088572E4, 0x088AADD8, 0x088CFB7C, 0x089CF0A4, 0x08863748, 0x088637CC, 0x088CE97C, 0x08855774, 0x0887D738, 0x0889C04C, 0x088AB150, 0x088AC210, 0x088B5334, 0x088BD268, 0x088CEA34, 0x088CF880, 0x088D06B0. Tables in docs/PSP_PROGRAM_MAP.md.
- mult's lo / hi kept for the side record (0x088D06B0).
- Checked: the coverage pass maps all 25 to scenarios (attract_mode, arcade_ending and mode_tour run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, arcade_match and option_tour, with 0.107.0 - 0.109.0 built and checked together (each batch's functions are independent of the later ones).

## 0.107.0
### For players
- **More of the game decompiled:** 34 more functions, among them the reset of objects and tables before a fight, the CPU script's opening for a fighter, fighters kept on screen by their routine, the VS screen's lists, and screen objects' places, counters, shapes and texts. Nothing plays differently.

### Technical
- `src/core/object/batch_mid2.c` (new): 34 more of the ready functions of 25 - 42 instructions (`ready.py 25 60`): 0x088B4F70, 0x088BBFA0, 0x088BDB2C, 0x08822144, 0x088369C8, 0x0883A6E0, 0x088558C0, 0x08856DA4, 0x08857CFC, 0x08881644, 0x088B0C94, 0x088BC070, 0x088BC740, 0x088CED2C, 0x088CFBFC, 0x088D0738, 0x089DEBE8, 0x0880D5B4, 0x088557FC, 0x08878D30, 0x0887B418, 0x0885F050, 0x0887D7F8, 0x0892EE70, 0x089DE978, 0x0887BBC8, 0x088A377C, 0x088AB0A4, 0x088B265C, 0x088B4498, 0x088B54FC, 0x088BCCF4, 0x088CEE04, 0x089D3174. Tables in docs/PSP_PROGRAM_MAP.md.
- mult's lo / hi kept for the side records (0x088D0738).
- Checked: the coverage pass maps all 34 to scenarios (attract_mode, arcade_ending, mode_tour and free_battle_team run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle_team, arcade_other_fighter, arcade_match and option_tour, with 0.107.0 - 0.109.0 built and checked together (each batch's functions are independent of the later ones).

## 0.106.0
### For players
- **More of the game decompiled:** 40 more functions, among them the CPU's script commands that test its level, fighters' input records and press counters, the reset of the two side records when a fight starts, and many screen objects' counters, trails and set-ups. Nothing plays differently.

### Technical
- `src/core/object/batch_mid.c` (new): 40 of the ready functions of 25 - 28 instructions (`ready.py 25 60`): 0x08848050, 0x08856AA0, 0x08878ACC, 0x08887CBC, 0x088B4514, 0x088CF908, 0x088D42F0, 0x08998214, 0x089D99E8, 0x089DFC40, 0x089E044C, 0x088237DC, 0x08837950, 0x088B08F8, 0x088B3040, 0x088B30A8, 0x088BBDFC, 0x088BC4B8, 0x088BCC8C, 0x088BD8B0, 0x088CF73C, 0x089DDB68, 0x08847FE4, 0x0886B468, 0x088768F0, 0x088A2660, 0x088AA9BC, 0x088B4FD4, 0x088BC7FC, 0x088CE120, 0x0882BD04, 0x088344E8, 0x088636D8, 0x08869B70, 0x08877FBC, 0x088815D4, 0x088A904C, 0x088AE240, 0x088B29C0, 0x088B4EBC. Tables in docs/PSP_PROGRAM_MAP.md.
- 0x089E044C's switch read from its table at 0x08A01D70 (0 and 15, 2 and 10, the rest); mult's and divu's lo / hi kept (0x08848050, 0x088B4EBC).
- 0x088B4514 (the trail) first failed attract_mode at frame 5894: a branch's delay slot sets the register the empty-slot mark then uses (the slot two on, not +72's). Found by bisecting with `--reference-only`.
- Checked: the coverage pass maps all 40 to scenarios (attract_mode, arcade_ending, mode_tour, arcade_other_fighter and free_battle_team run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, free_battle_team, arcade_match and option_tour.

## 0.105.0
### For players
- **More of the game decompiled:** 18 more functions, among them drawing the four fighters, how much damage a guard takes off and when a fighter guards, a fighter's reset for each round, name entry, and the fighters' labels before a fight. Nothing plays differently.

### Technical
- `src/core/object/batch_large2.c` (new): the 6 small functions that became ready after 0.104.0 (0x088B0960, 0x088B48C4, 0x088BC044, 0x0887684C, 0x0880BA48, 0x088813EC) and 12 of the larger ready ones (0x088B5578, 0x088AED5C, 0x088BDBB8, 0x0887DD64, 0x0885ECE8, 0x088AE5A8, 0x0889229C, 0x088980F4, 0x08871ED0, 0x0886731C, 0x08897F34, 0x088B56E4). Left for later: 0x088386B0 (recursive, a dozen registers) and 0x0880E9E0 (allocates sprite cells and waits a frame when none are free).
- 0x088AED5C's switch read from its table at 0x089FF750 (types 0 - 3 and 8, 4 - 6 and 9, 7).
- Checked: the coverage pass maps all 18 to scenarios (attract_mode and arcade_ending run them all); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, arcade_other_fighter, arcade_match, free_battle and stage_objects_10_guy.

## 0.104.0
### For players
- **More of the game decompiled:** 20 larger functions, among them drawing every object with its parts (one of the busiest functions in the game), hit boxes meeting, the fighters pushed apart, a fighter's walk, and the attract mode's and menus' set-ups. Nothing plays differently.

### Technical
- `src/core/object/batch_large.c` (new): 20 of the ready functions of 61 - 100 instructions (`ready.py 61 400`): 0x0880BA94 (an object drawn with its parts, ~217,000 calls in the profile), 0x0880DA78 (the fading shape, translated beside `shapes.c` until now), 0x08821600, 0x08822D2C, 0x08856B38, 0x088627D0, 0x08862FF0, 0x08863E1C, 0x08873728, 0x08873BF0, 0x088AF9DC, 0x088B2A5C, 0x088B42D0, 0x088B520C, 0x088BD334, 0x088BE210, 0x088CF388, 0x088D0354, 0x089DFA54, 0x089DFED4. Tables in docs/PSP_PROGRAM_MAP.md.
- 0x089DFED4's first object takes the caller's s0 as its + 88, as the original does.
- Checked: the coverage pass maps all 20 to scenarios (attract_mode, arcade_ending, mode_tour and arcade_other_fighter run them all; 0x0880BA94 runs in every one of the 67 scenarios the pass covers, all but the three `wide_*` ones); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, arcade_other_fighter, arcade_match, free_battle_team, option_tour, save_load and stage_objects_05_akuma_2.

## 0.103.0
### For players
- **More of the game decompiled:** 41 more of its functions are now hand-written C, among them the CPU's script frame, a fighter's routine by human or CPU, steps through the screens' tables, and objects' places, bounces and counters. Nothing plays differently.

### Technical
- `src/core/object/batch_steps.c` (new): 41 functions of 11 - 24 instructions, all the ready ones (every callee decompiled) in the wider profile: the step tables 0x08A16DA8, 0x08A16F14, 0x08A24B5C, 0x08A36F24 / 6F40 (with the bottom text), 0x08A3AEF8 / AFC0 (or the object freed), 0x08AB76D8 / 76BC (then the object lists and the frame's end) and 0x08AB7A28; fighters' 0x088773E8, 0x0885EACC, 0x0887A2A4, 0x0887A0B8, 0x08876894, 0x08987118, 0x088BE788; and 26 objects' and records' steps. Tables in docs/PSP_PROGRAM_MAP.md.
- Registers a callee could read are set as the original leaves them (the arguments, and a1 - a3 / t0 at calls through tables); s0 / s1 as the original sets them around calls; mult's lo / hi kept (0x088CEABC).
- Checked: a coverage pass over every scenario (decomp report, renderer lazy) maps each of the 41 to the scenarios that run it (all reached; attract_mode, arcade_ending and mode_tour between them run all 41); parity passed for arcade_fight, boot_to_select, attract_mode, arcade_ending, mode_tour, free_battle, arcade_other_fighter and select_choices.

## 0.102.5
### Release summary
- **More of the game is decompiled:** 13.1% of it is now hand-written C, up from 6.3% in v0.65.1 (1,758 of 14,881 functions), each one checked frame by frame against the original.
- **WIDE NATIVE widescreen is built in and on by default:** the game's own 16:9 mode, now part of the decompiled code (no patch downloaded). Menus, select and VS screens, fights and their HUD, the attract mode and endings fill the wide screen, and the fights play exactly as in 4:3. Stage Extensions and an art pack for painting the stages' edges are there to try (experimental).
- **A new launcher layout:** tabs across the top with small pictures, an About tab, and an Inputs tab where each of four players gets the keyboard, a controller or both (player 1 on the keyboard to start, so input mappers only get what you choose). **Configure** sets every key and button, and **Test** lights up each button as you press it.
- **Smoother on the GPU:** the renderer sends its draws in batches, about seven times fewer calls a frame in a fight.
- **Reporting a problem** from the launcher fills in an issue on GitLab, GitHub or Gitea, with your logs gathered in one zip.
- **Smaller additions:** SELECT + START goes back to the MODE MENU; EX. SETTING's PRIORITY rows say PLAYER or COM; a simpler Build / Run tab.

### For players
- **Docs:** the README points to **Report a bug** on the launcher's About tab; DISPLAY.md lists SCREEN SIZE (default WIDE NATIVE); WIDESCREEN.md says WIDE NATIVE is the default.

### Technical
- `tools/release_notes.py`: a release's own section can have a `### Release summary`; then the release notes are that summary and a link to CHANGELOG.md, instead of every version's bullets since the last release (this release gathers 88 versions since v0.65.1). Without one, unchanged.
- Release check: the full test suite (77 tests) passed on the maintainer's PC on 0.101.0 (2026-11-20); 0.102.x changed only the launcher and docs. The launcher screenshots retaken on Windows (5b6e494); `tools/screenshots.py --check` passes.

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