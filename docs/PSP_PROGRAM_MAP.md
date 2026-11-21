# PSP program map (Street Fighter Alpha 3 MAX, ULUS-10062 v1.00)

Everything here was measured with the tools in `tools/`; re-run `tools/extract_assets.py` to reproduce it.

## The disc

| File | Notes |
|---|---|
| `PSP_GAME/SYSDIR/BOOT.BIN` | **Unencrypted** relocatable program (ELF type 0xFFA0). SHA-1 `3fd0abf7cc65742233e1b7787997fa3e20152d98`. Used as the source; no decryption needed. |
| `PSP_GAME/SYSDIR/EBOOT.BIN` | Encrypted `~PSP` copy of the same program. Not needed. |
| `PSP_GAME/USRDIR/usa/module/*.prx` | Sony's encrypted libraries (ATRAC3+, SAS, MPEG, font, codecs). Replaced by native code (HLE); never loaded. |
| `PSP_GAME/USRDIR/usa/data/*.pac` | 313 game data archives (format still to be decoded). Names show the layout: `pl00`–`pl29` (42 fighter slots, plus `pl29r`), `st*` stages, `op*`/`end*`/`win*`/`bd*` per-character intros, endings, win screens and bios, `rk*`, `select_u`, `title_u`, `world_u`/`wr_scr` (World Tour), `netd_u`/`nets_u` (wireless), `msel_u` (mode select), `opt_u`, `memory_u`. |
| `PSP_GAME/USRDIR/usa/bgm/p0000.at3`… | 51 ATRAC3+ music tracks. |
| `PSP_GAME/USRDIR/dummy.bin` | 10 MB padding. |
| `PARAM.SFO` | `ULUS10062`, title `STREET FIGHTER ALPHA3 MAX`, disc version 1.00, system 2.50. Module name `ZERO3`. |

## Program layout (loaded at 0x08804000, the hardware address)

| Region | Address | Size |
|---|---|---|
| `.text` | 0x08804000 – 0x089FC354 | 2.06 MB, 516,309 instructions |
| import stubs | 0x089FC354 – 0x089FC8FC | 181 OS functions |
| `.rodata` | 0x089FCFF0 – 0x08A02DE0 | 24 KB |
| `.data` | 0x08A02DE0 – 0x08AD8F18 | 857 KB, incl. switch tables and function-pointer tables |
| `.bss` | 0x08AD8FC0 | 10.3 MB |

- Entry: `module_start` 0x08804000, `module_stop` 0x08804360.
- **Relocations are complete**: about 90,000 relocation records. Because the program had to be relocatable,
  every code address stored anywhere carries one, so the analysis sees every function pointer.
- **No VFPU (vector unit) instructions at all**, about 2,500 single-precision FPU instructions (incl. loads/stores), and a
  handful of Allegrex extras (`ext`, `ins`, `seb`, `seh`, `min`, `max`, `clz`). Branch-likely is used
  (about 6,500).
- 14,932 functions: 4,839 called directly, 9,555 only through pointer tables (Capcom-style state
  machines), the rest reached by fall-through or never called.
- 74 `switch` jump tables (578 case labels), all resolved. No unresolved indirect jumps.

## Where things are

| What | Where |
|---|---|
| Controller read (`sceCtrlReadBufferPositive`) | one function, **0x088051BC**. Raw pad buffer at 0x08AD8FD0; buttons copied to 0x08AD8FE0 and 0x08E75900; analogue stick folded into the d-pad (threshold ±64). |
| Wireless (ad-hoc) VS code | one block, **0x088DE4A0 – 0x088E1EC4** (sceNet, sceNetAdhoc, Adhocctl, Matching). PTP send at 0x088DF910 / 0x088DFEF4, receive at 0x088DFA48 / 0x088E02C0. |
| Save data (`sceUtilitySavedata*`) | 0x089E347C – 0x089E3E78 |
| Graphics (`sceGe*`) | 14 functions, 0x089E4110 – 0x089EAAD4 |
| Sound (SAS voices) | 23 functions, 0x089EC8E4 – 0x089ED018; ATRAC3+ music 0x089E5BC4 – 0x089E68A4 |
| Compiler library (libc etc.) | about 0x089E6000 onwards |

## Modes named in the program

`SURVIVAL MODE`, `VS100 KUMITE MODE`, `VARIABLE BATTLE`, `DRAMATIC BATTLE`, `R.DRAMATIC BATTLE`,
`DRAMA SURVIVAL`, `RECOVERY BATTLE`, `FREE BATTLE MODE`, `TEAM BATTLE`, `TRAINING MODE MENU`, `ISM EDIT MODE`,
`ENDING MODE`. Wireless menu: `VS MODE`, `TRAINING MODE`, `TEAM VS MODE`, `SURVIVAL MODE`,
`DRAMATIC MODE`, `VS DRAMATIC BATTLE`, `VS VARIABLE MODE`, `TOURNAMENT MODE`, `LEARGUE MODE` (sic).
The wireless modes are the ones local 2P/3P should open up.

## Widescreen patch (Rickow SFA3MAX-PSP-widescreen v1.1)

What WIDE NATIVE started from (until 0.99.1 downloaded and translated in as an overlay; every function
below is native now, `rt_widescreen` choosing the width: [WIDESCREEN.md](WIDESCREEN.md)). It applies
cleanly to this `BOOT.BIN`. Changes 35 words in `.text` (viewport 384 -> 480 and the camera
-192 -> -240 as immediates; 0x0880F390, the pillarbox, returns at once; six stores in the background
tile drawers 0x0881AB28 / 0x0881B268 / 0x0881B7C8 become jumps into caves) and adds code in caves in
unused `.data` (0x08A02ED0, 0x08A02F38, 0x08A031B4; zero in the original, though 0x08A02ED0 is also
an address the program computes). The game's own display option must be **Normal** for it, which WIDE
NATIVE sees to.

## What boot does (from the runtime's trace)

1. `module_start` creates **user_main** (prio 32, 256 KB stack), which loads Sony's libraries
   (`audiocodec`, `sc_sascore`, `libatrac3plus`, `libfont`: handled natively) and starts **GameMain**
   (0x08804B84, prio 111).
2. GameMain sets 480×272 display mode, creates the `SceGuSignal` event flag, sets the GE callbacks
   (signal 0x089EA550, finish 0x089EA5D0) and submits the first display list (0x08A020E8).
3. Double-buffered frame buffers at **0x04000000 and 0x04044000** (VRAM), width 512, pixel format 1
   (16-bit 5551).
4. Grabs one big heap ("UserSbrk", 0x8E7000 bytes at 0x09531000), reserves audio channels 0–3
   (448/256 samples), starts **sound_thread** (0x089E5BEC, prio 16) which mixes SAS voices in
   256-sample grains and outputs 4 grains at a time on channel 0.
5. Loader threads (**task_ctrl_thread**, prio 107) read the data packs: `ascii`, `vram`, `efct`,
   `sign`, `memory_u`, `pit_u`, `zett`, `ake`, `opt_u`, `se_mid`, `select_u`, `titlesnd`.
6. At about 3.6 s it checks the memory stick (`fatms0:` devctl), then settles into one display list
   per frame: presumably the title or save-check screen.

Volume setting byte: 0x08AD8040 (written by 0x089E688C; read by the sound thread).

## Save data

| Save | File | Contents |
|---|---|---|
| `ULUS100620000` | `STFALPH3.SYS` | system data (options, unlocks, records) |
| `ULUS100620001` | `STFALPH3.WTR` | World Tour |

Both files carry 0x3399 at +0x280 and have no checksum. Loading (0x08816FC4) copies them in pieces:
SYS +0x284 (0x208 bytes) → 0x08A058A0, +0x48C (0x17C0) → 0x08A05AA8, +0x1C4C (0x3C0) → 0x08A07268,
+0x200C (0x140) → 0x08A07628, +0x214C (0x13F8) → 0x09519588, +0x3544 (4) → 0x08A05894;
WTR +0x284 (0x174C) → 0x09517E38. The project keeps these files in a3mx.dat / a3mx.cfg (savestore.c,
settings.c), not on a memory stick; a PSP keeps them encrypted (see "save key" below, docs/SAVES.md).

| Address | Meaning |
|---|---|
| 0x08A058A0 + 0x4F | AUTO SAVE (OPTION > SAVE LOAD); 0x08A077CC is the session copy (cleared without a memory stick) |
| 0x08A058A0 + 0x50 | bits 0-2: modes cleared in their alternative (L + R) version |
| 0x08A058A0 + 0x52 | unlock bits (16-bit), set by 0x0882AE7C when modes are cleared; bits 0x0001 / 0x4000 gate MODE MENU alternatives (0x0888CEC4) and the select screens' extras (0x08897434: SELECT held when choosing turns Akuma (2) into Shin Akuma (0x24) and the second Balrog (0x25) into 0x15, and makes M. Bison (0xA) Final Bison (fighter + 0x1B7) in Arcade, VS and modes 9, 0x11, 0x12) |
| 0x08A058A0 + 0x68 | game time in frames |
| 0x08A058A0 (+20 per player, 3) | button config: 8 halfwords (attack for Square, Triangle, L, Cross, Circle, R, Select, unused), +0x10 COMMAND, +0x11 TYPE, +0x12 RAPID. A fighter reads its entry when the button is pressed. Attacks 0-0x36, names at 0x08A37B44 (L PUNCH ... HP + HK, NO USE, CHOUHATU = taunt, SPECIAL1 L ...); COMMAND / TYPE / RAPID names at 0x08A37A5C / 68 / 70 |
| 0x08864DFC | a fighter's buttons, once a frame (a0 = fighter): held game bits of its slot (0x08E75BA0 + 0x2C n + 4; TRAINING: fighter +362) → a held flag per button (0x08B0264E + 16 × player: 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x4000 SELECT, 0x8000 START); each held button's config attack → game bits (attacks 0-14: 0x08A19782; specials / super combos: per character lists at 0x08A1B3F8 / 0x08A1B4F4, counts at 0x08A1B4A0 / 0x08A1B59C, 8 bytes per entry: bits, move id) and the attack kept at fighter +1150 + 2 × button; bits to fighter +752 / +764. Some modes read configs at 0x09523438 (0xD0 each) instead. Hooked: L2 / R2 (playercfg.c) |
| 0x08867794 | a fighter's newly pressed buttons as a byte (bit 7 START), a0 = fighter: slot +0xC by fighter +775, or TRAINING +362 & ~+364. Read with +1150 by 0x08867A60 (one-button specials / super combos) and 0x088679BC. Hooked: L2 / R2 |
| 0x08E75BA0 slot bits | game bits: 0x10 Square, 0x20 Triangle, 0x40 L, 0x80 Cross, 0x100 Circle, 0x200 R, 0x4000 SELECT, 0x8000 START; 1/2/4/8 directions |
| 0x08A058A0 + 0x200 | ISM-Pluses earned (one bit each in bits 0-27, 26 in all; bit 0 Alpha Combo, bit 4 ∞ Guard, bit 5 Auto Guard; the mode rewards at 0x0883B884 give bits 15-27 by mode). EDIT MODE lists the rest as ???. All held on (sysdata.c) |
| 0x08A058A0 + 0x204 | GAME OPTION > MODE SELECT (1 on; file +0x488). On in the save made on first start |
| 0x08A058A0 + 0x3C.. | GAME OPTION: +0x3C difficulty 0-7, +0x3D time (30 60 99 ∞), +0x3E rounds (1 3 5), +0x40 damage 0-3 (and +0x42), +0x43 speed TURBO1-7 (0: +0x44 NORMAL / FREE1 / FREE2), +0x45 S.C. gauge (0 MAX), +0x1FC / +0x1FD BGM / S.E. volume 0-15, +0x1FF player side. Value names at 0x08A379C0 on. Kept in a3mx.cfg (settings.c) |
| 0x08A058A0 + 0x58 | SCREEN SETTING > SCREEN SIZE (NORMAL / WIDE, and +0x62); +0x1FE WALLPAPER (NONE, 01-10) |
| 0x088B85BC / 0x088B78DC | SCREEN SETTING submenu: logic (row in 0x08E75912 + 7: SCREEN SIZE, WALLPAPER, DEFAULT, EXIT; +2: 0 running, 1 closing, others opening; up / down inline, timer 0x08B02E0C +0 frames open (input from 5), +2 repeat 0x14 then 6; SCREEN SIZE 0x088B700C, WALLPAPER 0x088B7064) / drawing (rows x 0x6C, y 0x40 + 0x18 n, values x 0xDC; title 0x089FF8EC). Its bar 0x088BAA90 (object type 0xA, y = 0x08A37DA8[row] = 240 - row y). Hooked: VSYNC row (screensetting.c) |
| 0x08AEF75C | OPTION MODE's copy of the options block (0x208 bytes) |
| 0x08E75910 + 0x2D | the difficulty in effect (GAME OPTION edits it and writes 0x08A058A0 + 0x3C, 0x088B6318); the other settings in effect are near it, filled from the options block after a load (LOAD row: 0x088B73B8 → 0x088B6108) |
| 0x088B70F8 / 0x088B7AE8 | SAVE LOAD submenu: logic (rows SAVE, LOAD, DELETE, AUTO SAVE, EXIT in +7; per-row handlers 0x08A37A34) / drawing (rows at y 0x38 + 0x18 n). Hooked: IMPORT and EXPORT rows (exsetting.c) |
| 0x088BAB44 | SAVE LOAD's green bar (object kind 0xC, type 0x14), once a frame: y = 240 - the row's text y |
| 0x08AD51D0 | the save icon (ICON0.PNG, 11005 bytes) the game passes with its system data save |
| save key | the game saves with SceUtilitySavedataParam size 0x600, secure version 0 and key 006FAF01D5700002C712242EBA83FFE1 (SDK 2.5: crypt mode 3) |
| 0x088B6164 | OPTION MODE menu, once a frame. Context 0x08E75912: +0 state, +1 cursor, +5 submenu shown, +6 in a submenu. Submenus by cursor: 0x08A37A10 (start) and 0x08A37A48 (each frame) |
| 0x088B7EC0 | menu cursor up / down (a0 pad slot, a1 cursor, a2 last item) |
| 0x088BA82C | OPTION MODE button (object kind 0xC, type 0x0F + item), once a frame: bright and bigger under the cursor, dim otherwise. Layout tables 0x08A37DBA (x, y) and 0x08A37C60 (size, page, u, v) |
| 0x088BA8F4 | GAME OPTION's green bar (object kind 0xC, type 4), once a frame: y from 0x08A37D28 by the row (+7 of the option context); shown only while +5 == 1, removed when +6 == 0 |
| 0x08B0C334 | texture pages, 0x18 each (+0 address, +4 RAM copy, +0x14 description); an object's +0x3C page n is entry 0x30 + n |
| 0x0880A7B0 / 0x0880A7F4 | draw a PSP button symbol (a0: 0 Circle, 1 Triangle, 2 Square, 3 Cross, 8 L, 9 R; font 5) / SELECT (font 6) at a1, a2 |
| 0x088D2B64 | unpacks one 16x16 tile (128 bytes, 4-bit) of a sprite into a 64x64 texture; save and boot messages are such sprites (pictures of the text), drawn tile by tile. The boot screen's "Auto-load completed." lands at 0x095FF600 |
| 0x08818BA4 | Yes / No question (a0 context: +4 message, +9 cursor 1 Yes / -1 No) |
| 0x09523C60 | save / load task context: +2 state (4 = result shown, waiting for a button; 0x08817C10), +4 message (0x41 load World Tour?, 0x18 load SYSTEM DATA?, 0x11 Load completed.), +5 result |
| 0x0888D2B0 | leaving a mode for the MODE MENU: +4 state (1 runs the auto-save 0x0881725C, 2 shows the result for 120 frames, 4 fades out), result in 0x09525880 (5 saved). The only caller of the auto-save |
| 0x088335B8 | entering NETWORK: "Save SYSTEM DATA now?" (state at task +6: 1 asking, 2 yes, 3 no, 4 back) |

## Graphics usage

- Each scene is drawn at **480×224** into a 512-wide buffer at **0x04088000** (RGB5551), then drawn as one
  bilinear-filtered sprite onto the visible buffer (0x04000000 / 0x04044000), stretched to 480×272.
- Textures live in RAM and in VRAM from 0x040CC000; formats seen: 5551, CLUT4, CLUT8 (palettes 565/5551).
- Only one blend mode (src alpha, 1 − src alpha, add), alpha test "greater than 0", texture function
  modulate (mostly with texture alpha).

Measured over 1,800 frames:

- All primitives are **through mode** (2D screen coordinates): triangle strips (≈98%) and sprites.
- Two vertex formats: `0x80011E` (16-bit UV, 32-bit colour, 16-bit position) and `0x80011C` (no texture).
- No 3D transforms, lighting, depth test, splines or DXT textures.

## Input path and players (found with --watch / --snap / --poke)

1. `0x088051BC` reads the pad (`sceCtrlReadBufferPositive`), folds the stick into the d-pad, runs the
   button-config remap `0x088E3660`, stores the result at **0x08AD8FE0**.
2. `0x0880509C` (now hooked) builds three records `{slot, present, buttons}` at **0x08E75C28** and passes
   each to `0x08804F50`, which keeps per-slot controller state at **0x08E75BA0 + 0x2C × slot**
   (+4 held, +8 previous, +0xC pressed, +0x10 released, +0x14 repeat). The local pad goes only to the
   slot named by the byte at **0x08AD8FE4** (0 host, 1 guest); other slots normally come over ad-hoc.
3. **Fighter objects** at **0x092E3EA0 + 0x498 × k** (k = 0, 1, 2):

   | Offset | Meaning |
   |---|---|
   | +0x00 | active |
   | +0x02 | player number (selects the controller slot in most modes) |
   | +0x12 | X position (s16) |
   | +0x16A / +0x16C | copy of the slot's held buttons (current / previous) |
   | **+0x175** | **computer-controlled flag**: clear it and the fighter obeys its controller slot |
   | +0x2F0 / +0x2FC | decoded game input (after the per-player button config) |
   | +0x307 | controller slot index |

4. `0x08864DFC` decodes a fighter's buttons; it chooses the source by the **game-mode byte at
   0x08E75982** (seen: 0x0D/0x13 use +0x307, 2 uses +0x16A, 7/0x0F/0x10 special cases, otherwise +2).
5. CPU fighters run AI routines from a pointer table at about **0x08A1914C** instead of reading input.

Roles seen: Arcade: fighter 0 = player 1, fighter 1 = opponent. **Dramatic Battle: fighter 0 = player 1,
fighter 1 = partner, fighter 2 = the boss.** P2's Start opens the game's own "2P PAUSE" menu.

## Menus

| Address | What |
|---|---|
| 0x0888C268 | MODE MENU task (task 0's entry; decompiled in `src/core/task/mode_menu.c`, 0.9.0): runs the screen at 0x08A26C8C[GAME + 0] each frame: 0 set up (0x0888C2DC), 1 press Start (0x0888C3A0, gives up after 1800 frames: the dispatch with -1), 2 the menu (0x0888C498, steps via 0x08A26CA0: 0x0888C4CC set up, **0x0888C5B0 running**), 3 back to the title (0x0888CAFC), 4 back from a mode with auto-save (0x0888D2B0: 0 start, 1 save (0x0881725C, result in 0x09525880), 2 result shown 120 frames, 3 retry question, 4 fade to screen 0, 5 memory stick question; results 4 off, 5 saved, 0x10 not needed, 0x0C no stick, 6 and 8+ ask) |
| 0x0951CE54 | u16: set when a mode returns to the MODE MENU; with auto-save on (0x08A058EF) screen 0 goes to 4 |
| 0x0888CB7C | MODE MENU pictures: background and palettes 1-4, 8 (0x08A26BC8, 32 bytes each) and 9 (0x08A35038) through 0x0880AEA8 (palette load: a1 colours, a3 slot, a4 count of 16 colours, into 0x092DF980) |
| 0x088325DC | back to the MODE MENU (decompiled, 0.12.0; a0 = 0x08E75910): stops the voices and task 2, clears the game state but keeps +0x40 and the mode (+0x72), sets +2 = 1 so the MODE MENU opens on the menu itself; formerly noted as: stops the other tasks, resets the game state and restarts task 0 at 0x0888C268 (used by the pause menu's quit and many more). Tasks are threads; the running one's record is at *0x08E75C40 (+0 state, +4 entry; task 0 at 0x08AD8FE8, 12 bytes each); 0x088054F0 waits for the next frame, 0x08805614 restarts the running task at a0 |
| 0x0888C5B0 | MODE MENU running state, a0 = menu object (+0x72 selected mode id, +0x35 how it was chosen). Reads slots 0-2; game bits 1/2/4/8 = directions, 0x8080 confirm, 0x100 back. L + R held on confirm (game bits 0x240, pad 1 or 2) picks a mode's alternative: Dramatic (8), Variable (15) and R.Dramatic (17) with unlock bit 0x4000, Final Battle (9) with bit 0x0001 (0x0888CE80; sound 0x3A, + 0xBF = 1) |
| **0x08B02711** | MODE MENU cursor = column × 5 + row (0 ARCADE ... 9 NETWORK ... 14 SCORE RANKING); 0x08B02710 = key-repeat timer (20 frames, then every 7). Set up by finding the mode in +0x72 in the table; a mode not there leaves it at 15 |
| 0x08A26C68 | cursor → mode id (15 bytes: 0,15,2,16,5,3,14,4,18,12,7,8,17,9,11); 0x09525DD0 = which mode ids are enabled (copied from 0x08A26C77) |
| 0x088D8128 | network task (task 0's entry while in NETWORK); state at 0x09523980 (+0x14 step table, +0x18 step). 0x08A3E8F8 is set while in NETWORK, so the task resumes rather than starts afresh after a battle. Its exit to the MODE MENU is 0x088D7A08; 0x088D7EDC shuts the network down (sockets, matching, AdhocTerm, NetTerm), as the exit callback 0x08804A88 does. A network battle sets the mode (0x08E75910+0x72) to 1 |
| 0x0888CD94 | MODE MENU dispatch (decompiled, 0.11.0; a2 = 0 chosen, 1 by player 2, -1 back to the title); jump tables at 0x089FF0D0 (variants, modes 8-17) and 0x089FF0F8 (mode id 0-19). Fighting modes start task 2 at 0x08833B6C and restart task 0 at 0x088233EC; their own tasks: World Tour and NETWORK 0x08837A44, Entry 0x088CC5A4, Option 0x088B5F48, Score Ranking 0x088BEF70, Edit 0x088D41B0, mode 10 0x0881749C. Mode 12 (NETWORK) copies 0x9FC bytes of settings to 0x0951C0D0. GAME + 0x88 two players, + 0x89 a mode with its own task, + 0xBF the variant |
| 0x0888C85C / C90C / C9B0 / CA5C | MODE MENU cursor down / right / up / left (wrap round the column or row, skip disabled modes). The menu's direction bits are Capcom's: 1 right, 2 left, 4 down, 8 up |
| 0x0888CC78 | auto-save error question, Yes / No (0x09523C60 + 9 = cursor), called by 0x0888D2B0 (still translated) |
| 0x0887C4CC | sound effect (a0 = 0, a1 = id, a2 = 0xC0): 0x38 cursor, 0x39 choose, 0x3A, 0x2F back |
| 0x088DD5D0 | vertical list menu (a0 = context, cursor at +0x2E; a1 = items; a2 = slot mask): returns 1 chosen, -1 back. Network menus share context 0x09523980: called from 0x088D85C4 (network MODE MENU: VS MODE, TEAM BATTLE, VARIABLE BATTLE, DRAMATIC BATTLE), 0x088D86D8 (SINGLE, LEAGUE, TOURNAMENT), 0x088D8790 (Dramatic: HOST, GUEST) |
| 0x088EC074 … 0x088EC724 / 0x088D6BC0 / 0x088D6D3C / 0x088E3B8C / 0x088E3BEC / 0x088E3DB0 | the network screens (`network/menu.c`, `network/screens.c`): the menu rows (11, one per row id, text and number from 0x08A3F75C / 0x08A3EA34[menu + 47 × 11], the chosen row's bar 0x088D6D74); the menu's backdrop (bars at row + 56 with + 57, the dark panel with + 58, shorter when 0x088DD114 ≥ 2 in a session of kind 2 / 3); the text's black backdrop (+ 61); the message box's panel (colour 0x08A3EC44[+ 4 set]) and its opening and closing by 12 rows a frame (+ 20, done at 120: + 23). All draw with 0x0880E788 (a0 - a3 the rectangle, t0 colour, t1 depth), the screen's width 384 (WIDE NATIVE: 480) |
| 0x088DD734 | horizontal value adjust (a0 = value, a1 = max): option screens |
| 0x09524A28 | network task's yes/no dialog ("Save SYSTEM DATA now?"): +0 = 0xFF and +2 = 2 while up, +1 cursor (0 Yes, 1 No). Handled at 0x08833700; the task state (s0 + 6) becomes cursor + 2 when chosen |
| 0x088331E4 | "confirm pressed" check for a slot (0x80, or 0x280 in two-button mode) |
| 0x092E2B88 | menu/screen objects: 32 × 0x94 bytes, handler table 0x08A25AA0 by +0x5B (run from 0x08887AFC) |
| 0x08E75A18 | texture list per object layer (+0x36), 4 bytes each: {entries, count}, an entry 12 bytes with +8 the page table entry. An object's page (+0x3C) is an index into its layer's list (0x0881036C selects it with 0x089E0EA4), not 0x30 + page as such. The OPTION MODE buttons' layer 0x12 lists 0x30 - 0x34 |
| 0x08B0C334 | texture page table, 0x18 per entry: +0 texture, +4 RAM copy, +0x14 description (+0 index, +0x18 format, +0x24 / +0x28 width / height). OPTION MODE tile pictures: page 0x32 (GAME OPTION 0, BUTTON CONFIG 32, SCREEN SETTING 64, EXIT 96), 0x33 row 128 (SAVE/LOAD); MODE MENU tiles 0x30, 0x31, 0x33 (176 × 31 at 32-pixel steps) |
| 0x09512770 | objects of another system: pools of 0xC0-byte entries (+0 active, +1 shown, +2 kind, +3 type, +0x12 x, +0x16 y up from the bottom), made from the top down: 0x08822EAC (32, kind in a0), 0x08822EE4 (pool from 0x09514AB0, 16); 0x0882196C sets an animation, 0x088215C4 steps and draws. The option BUTTON CONFIG's attack pictures are kind 0x93 types 0x24 - 0x2A (handlers 0x08A380EC: 0x088BB104, 0x088BB268), animation 0x08A38010[value] |
| 0x088B8ED0 | pause menu BUTTON CONFIG (task: 0x088BA028 once a frame, context 0x08B02E24: +0 - +2 each player's row 0 - 11, +4 the player from 0x08E75910 + 0x59). Input 0x088B8FC4 / 0x088B90E4 / 0x088B9208 (players 1 - 3; up / down 0x088B9CEC, left / right 0x088B93BC through 0x088B9B30), drawing 0x088B9790 (rows 16 apart from 0x18), attack pictures kind 0 types player × 8 + 1 + row (0x088D0D64, animation 0x08A3CC78[value]), green bar kind 3 type 5 (0x08888CC0, rows from 0x08A2620C) |

The yes/no prompt at boot ("Do you wish to begin the game without auto-loading?") is a different dialog.

## Network battle protocol (recorded with A3MX_NETCAP=1)

Lobby groups: VERSUS, LEAGUEVS, TOURNAVS, TEAM, LEAGUETM, TOURNATM, DRAMATIC, VARIABLE.
Group "VERSUS" (P2P matching, mode 3) or "DRAMATIC" (parent mode 1 with children mode 2), matching
port 555, hello/request data 4 zero bytes. After matching the game leaves the lobby group, joins a
group named from the players ("9B413301") and, after a wait counter at **0x09523984** (about 600
frames, 940 in Dramatic Battle), opens PTP streams on port 1025. VS: the guest connects to the host.
Dramatic Battle: every PSP listens and connects to the next one in a ring 0 → 2 → 1 → 0; data that
must reach everyone is passed round the ring. All PTP traffic goes through 0x088DF988 (send) and
0x088DFAC0 (receive); the message layer is 0x088E0CC8 / 0x088E11C0 and the sync state machine is
around 0x088E3860 (words at 0x08A3EB98).

| Message | Meaning |
|---|---|
| 0x1AC3C501 | sent once by the side that accepted the stream |
| 228 bytes | settings block (checksum word, button layout, member list); the host's has byte 30 different |
| 0xE198DE71 / 0x53201336 / 0x7F035D72 | sync request / answer / go; who asks alternates; a receiver skips words it does not expect |
| 0x1CE14355 | phase marker, sent by both sides; afterwards the guests ask for the sync |
| other words | lockstep input: low 16 bits the raw PSP buttons, high 16 bits the frame counter (+2 per frame); each side sends its own input to every neighbour, several frames per packet |

Dramatic Battle also uses PDP port 1026: an 84-byte member list from the host (MACs and roles), then
a roll call `AC 00 sum 94 type 00 step 00` (sum = 0xA3 + type + step) that everyone repeats about
every 21 frames until all report the next step.

Names in the network screens are drawn with sceFont: the host's from the system nickname
(sceUtilityGetSystemParamString 1), the others' from sceNetAdhocctlGetNameByAddr, into a
256x32 4-bit buffer one character at a time (GetCharInfo, then GetCharGlyphImage_Clip).

After a network match, CONTINUE / SELECT / EXIT is run by 0x088E9AE0 (context 0x08E75910: +0x16
cursor, +0x14 timer from 1200, then EXIT). Only the host takes input, from the local pad; a guest
(0x09523980 + 0x2B = 2 or 4, see 0x088D5E7C) waits for the host's choice.

## Select screen players (offline)

Each player object (the fighter objects above) runs its select cursor from 0x088945C0 only while
**+0x338** is set; 0x08894140 sets it from the players present (0x08E75910 + 0x35, bit per side)
when the screen starts, and Dramatic (mode 8) / Variable (15) hand it back and forth (0x08894950,
0x08895530). A player who presses Start later joins through 0x0883465C (+0x154 = 3, +0x35 |= side)
but its +0x338 stays 0, so on the PSP the joiner was frozen: hooked to switch it on. Cursor input
comes from 0x08896AA4 (slot by side +0x151). In offline Dramatic Battle the boss (fighter 3) has
player number 0.

The Survival and Free Battle select screen (code 0x088A2xxx - 0x088A6xxx, context 0x092E1228) reads a box's
buttons from fighter +0x334's +0x16A (held) / +0x16C (last frame's): 0x088A5174 finds that fighter,
0x088A507C / 0x088A5100 read it. +0x334 is set when the screen starts (0x088A6DF4 ...) to the player who
started, so one player picks every box; hooked for the Dramatic kinds. Each fighter 0-2's
+0x16A is the held word of slot +0x307 (= +775), copied each frame by 0x08820E50. The objects' states
(+4): 1 character, 2 chosen, 3 ISM, 7 speed; Survival Dramatic goes 1P character, 2P character, ISM,
speed. 0x08E75910 + 2 is the game's phase: 0 MODE MENU, 1 select screen, 2 VS screen and battle.
Free Battle's kinds, fighters on the select screen (player numbers 0 - 3 = fighter number there): Arcade
0, 1 (2 and 3 off); Team 0, 1, starting in state 0 (team size), then 1 (characters); Variable and Dramatic
0, 1 (the team) and 2 (the opponent; 3 unused); R.Dramatic the starter (0 or 1) and 2, 3 (the opponents).
In R.Dramatic the boxes' +775 (the slot whose word is in +0x16A) is 0, 255, 1, 2: the box that reads pad k
is the one whose +775 is k, among all four. Turn order (player 1 started): starter's character and ISM,
then the partner's character (Variable / Dramatic), then the opponents' character and ISM, then the
starter's speed (state 7 / 8). Boxes measured on the screen: Arcade / Team 1P 99.5 and 2P 379.5 at 43
(fixed); R.Dramatic's opponents 379.5 and 429.5 (player 1 started) or 49.5 and 99.5, at 72.
0x088A1548 (a0 = 0x08E75910) runs once a frame there and draws SPEED: with 0x0880A6CC in font 3 (x 0x78,
y 0xC2, colour from 0x08B02716). Text coordinates are 384 x 224, scaled to the 480 x 272 screen; font 3 is
about 10.6 units a letter. Box centres (screen): Survival 1P (50, 72), 2P (100, 72); Free Battle 4 to the
left. In Free Battle the opponent's box is fighter 2, with player number 2 on the select screen (0 in the
fight).

Dramatic Battle's select screen (mode 8): per player, display tasks with a1 = the player: box 0x0889961C,
portrait 0x0889B0A4 (its set-up 0x0889B15C), sprite 0x0889A818, name strip 0x0889BFA4, and more (0x08899744,
0x08899AC8, 0x08899D28, 0x0889AA6C, 0x0889B2AC, 0x0889C23C); they run only while the player's bit is in
0x08E75910 + 0x29 | + 0x39. Their side (task +0xB: 1 left, 0 right) is `side ^ 1`, except in modes 8 and 15
where it is (0x08E75910 + 0x35) & 1 for everyone: the team's side (player 1 present: left), so a player 2
joining player 1 was drawn over player 1. The tasks of a kind run through one entry each (a0 = the task, its player
at +0x5C, which calls the task's state handler from a table): 0x08899558, 0x08899A80, 0x0889A7C8 (sprite;
also the ISM banner), 0x0889B048 (portrait; also the ISM column), 0x0889BAB0, 0x0889BF30 (name strip;
also the ISM banner), 0x0889E434, 0x088A712C, 0x088A7694, 0x088A78E0, 0x088A7CB4, 0x088A8038, 0x088A81FC,
0x088A8404 (tables 0x08A2A8A8, A8CC, A994, A9F8, AA94, AAD4, B0A0, BB08, BB4C ... BC00, BC40; the first
nine are in `src/core/select/display.c`; 0x088A81FC / 0x088A8404 also step +82 by 0x80, wrapping at
2560, and 0x088A8404 ends the task (0x088231C8) unless GAME + 196 has bit 0x80). The ISM panel's parts take their sides when they start, some from the team rule and some
from the player's side: with player 2 started and player 1 joined, the banner went left of its column.
In Dramatic Battle the player's state after choosing a character is 0.3 (+4 = 0, +5 = 3), then 2 (ISM). 0x08896AA4 (new presses) / 0x08896B18 (held) give a box its
buttons: in modes 8 and 15 both boxes read the starter's slot. The cursor goes to the other box at 0x0889494C
(modes 8 and 15). A joiner's box (+0x154 = 3) waits in state 0.0 with +0x338 = 0 until then; the waiting box
sits at (149, 47) on the screen whichever player started. The select screen runs once a frame at 0x08894214
(a0 = 0x08E75910). Free Battle's boxes when player 2 starts: the team on the right (1P 379, 2P 429), ENEMY on
the left (96); Survival's: 1P 379, 2P 430.

The 2D library (used by the text and many screens): 0x089E0CF4(a0 = texture list {descriptors, count}),
0x089E0EA4(n) picks descriptor n (12 bytes: ?, ?, page index into 0x08B0C334), 0x089E1618(a0 = {xy, colours,
uv, n}, a1 = n, a2 = flags (0x80000000 textured), f12 = layer) adds a primitive to the frame's list
(0x089E44EC; 0x08E55C40 + 0x8000 per buffer, count at 0x08E65C40), which 0x089E4408 draws layer by layer, 0
to 23 (from 0x089E47C4, the frame's render). Vertex x, y are in the text's 384 x 224 space (+0x30, -8 added).
Text is queued by 0x0880A6CC into 0x092DF240 (96 entries of 16 bytes) and drawn by 0x08812520 (or 0x0880B098):
fonts' advance / height at 0x08A03CE6 (font 0: 8 x 8, 1: 8 x 16, 2: 16 x 16, 3: 12 x 12), colours (top,
bottom vertex colour) at 0x08A03D04.

Battle setup after a select screen is 0x0882E380 (a0 = 0x08E75910). +0x39 holds a bit per player who
finished choosing (set by 0x088954E0, but only for the player who finishes second). Arcade (mode 0)
always takes the opponent from the ladder (0x0882FEB8: ladder table 0x09517988, writes the opponent's
+0x152 character, +0x173, +0x182, +0x56); other modes branch on +0x39 == 3 for player against player.
After a two-human Arcade match won by a player on a different character, the game shows that character's
profile and reason for fighting (input polled by 0x0882BB68 with mask 0x83F0).

Mode identification: game-mode byte 0x08E75982 (= 0x08E75910 + 0x72; MODE MENU ids: 0 Arcade, 2 Training,
3 World Tour, 4 Entry, 7 Survival, 8 Dramatic, 9 Final Battle, 12 Network, 14 Edit, 15 Variable,
16 Free Battle, 17 R.Dramatic, 18 VS 100 Kumite). Survival's kind: 0x092E1228 + 0x4F (1 Arcade,
2 Original, 3 Boss, 4 Dramatic, 5 Variable, 6 R.Dramatic, 7 R.Dramatic with lives, 8 Infinity).
Free Battle's kind: 0x092E1228 + 0x9F (1 Arcade, 2 Team, 3 Variable, 4 Dramatic, 5 R.Dramatic).
Fighter layout seen: 1-on-1 modes fighters 0-1 (player numbers 0, 1); Dramatic fighters 0-2 with the
boss as fighter 2 (player number 0); Variable fighters 0-2 with fighter 0 tagged out as CPU.

The fighter button reader 0x08864DFC picks the held word by mode: TRAINING the fighter's +362; modes 0xD / 0x13 slot +0x307; the Variable modes (15, Survival kind 5, Free Battle kind 3) slot 0 if player 1 is present, else slot 1, for every fighter (tag: one player plays whoever is in); otherwise slot 0 for player number 0 and slot 1 for any other.

Pause: 0x08828AF4 (a0 = the battle, a1 = a fighter on screen, called for each) pauses when Start is newly pressed on the slot in the fighter's +775; the pausing player is 1 << +775 in a0 + 0x59. A tagged-out fighter is not checked. A computer fighter's slot (its player number) also pauses, so an unused pad can pause.

## The C library (0x089E6000 onwards)

Newlib, plus Sony's libraries (sceGu graphics, SAS / ATRAC glue, thread helpers) between
0x089E6000 and about 0x089F3000. Decompiled ones are in `src/core/libc/`.

| Address | Function | Notes |
|---|---|---|
| 0x089F3224 / 0x089F32C8 | atexit / exit | table at `_impure_ptr` + 0x148 |
| 0x089F3374 / 0x089F339C / 0x089F33C0 | memalign / malloc / free | on `_impure_ptr` (0x08AD84DC) |
| 0x089F33E4 - 0x089F3540 | memcmp, memcpy, memset, strcmp, strcpy, strlen, strncpy | byte at a time |
| 0x089F35A0 / 0x089F3604 / 0x089F3670 | round digits up / strrev / itoa | for the float formatter 0x089F370C |
| 0x089F3E94, 0x089F40E0, 0x089F569C, 0x089F5820 | printf family | to decompile |
| 0x089F69F4 - 0x089F7938 | the heap: extend top, malloc_r, free_r, malloc_trim_r, memalign_r | Newlib mallocr.c, MALLOC_ALIGN 16 |
| 0x089F7BD4 / 0x089F7C10 | __malloc_lock / unlock | interrupts off; depth 0x08E758D8, saved state 0x08E758DC |
| 0x089F8E30 | _sbrk_r | calls the game's _sbrk 0x08804618; errno 0x09526410 |
| 0x089FAEC0 onwards | soft-float / 64-bit helpers | called from all over (0x089FB928 78 callers) |

Heap state: bins at 0x08AD8588 (bin_at(0): `fd` is top, `size` the bin-block bitmap; bin_at(1) the
last remainder); trim threshold 0x08AD8990, top pad 0x08AD8994, sbrk base 0x08AD8998, peaks
0x08AD899C / 0x08AD89A0, sbrked 0x08AD89A4.

The game's own allocation wrappers: 0x089E04EC (memalign + memset 0: aligned, zeroed) and 0x089E0540
(free), used by tasks and loaders all over the game.

## Input

| Address | What |
|---|---|
| 0x088051BC | pad read, once a frame (decompiled: `src/core/input/pad.c`) |
| 0x08AD8FD0 | SceCtrlData read this frame |
| 0x08AD8FE0 | buttons the game uses (stick folded into the d-pad past ±64; not while HOME / HOLD) |
| 0x08E75900 | the raw buttons, before the stick is folded in |
| 0x088E3660 | buttons -> buttons, applied last (to identify) |

## Tasks (decompiled: src/core/task/)

The game is a few tasks, each a PSP thread, run in turn once a frame by the main thread. Exactly one
runs at a time: they hand the CPU over with sceKernelWakeupThread / sceKernelSleepThread.

| Address | What |
|---|---|
| 0x08804AF8 | start-up (module_start's thread): the exit callback (0x08804A88), the main thread (id at 0x08E75904); then it only sleeps |
| 0x08804B84 | the main thread: set-up, task 0 = 0x088056A8, task 1 = 0x0880775C (the loader); then per frame: input, the scheduler pass (unless 0x08A02DE8, set while exiting), the frame finished (graphics), sound, the controller read |
| 0x08805374 | the scheduler pass: the 8 slots of `Task { u16 state, u16 wait, entry, thread }` at 0x08AD8FE8; state 1 start (a new thread: prio 0x6B, stack 128 KB), 2 wait `wait` frames, 3 has run, 4 resume; the slot being run in 0x08E75C40 |
| 0x088054C8 / 0x088054F0 | start a task in a slot / the running task waits N frames (every task loop goes through it) |
| 0x08805530 / 0x08805564 / 0x08805614 | the running task ends / kill a slot / the running task restarts at a new entry |
| 0x088056A8 | task 0's first entry: the boot screens (table at 0x08A02EB0, screen number at 0x08E75910) |
| 0x0888B398 | the title task: title screens (table at 0x08A26BA4: title, four attract demos, cycle, PRESS START, on to the MODE MENU), and a button press ending the attract mode (0x0888B92C / 0x0888BA08) |
| 0x0880AA74 / 0x0880ABD4 | start a fade task in slot 3: to black (0x0880AB04) / from black (0x0880AC18); a float level at 0x08E75B9C, 1/16 a frame; GAME + 0x5D is set while one holds the slot |
| 0x08A05888 | an s8 the program never writes (0 in the shipped game): a build option; with it set the boot screens would hold instead of setting up |
| 0x088233EC | the fight loop (task 0 in a fighting mode, `fight.c`): per pass: the frame counter GAME + 0x20 (not while GAME + 0x59 / 0x8A / 0x91 halt it), frame skip 0x088235C0, the fighters' step 0x08820C94, then 0x088D7FF4, 0x088DBC7C, the network step 0x088E3FF0, the stage effect 0x088859C8, the fight's frame 0x08823674 (0x08828C40 on screen 0, else 0x088238B4), the network text 0x088D80D8. It passes again without waiting while GAME + 0x24 is 0 or the frame is skipped |
| 0x092E1228 | BATTLE: +0 stage number, +22 game speed (& 15), +23 this frame skipped, +45 the speed applies, +52 a skipped frame done, +56 the fighters' order in modes 13 / 16 / 19, +159 Free Battle's state. Skipped frames: 0x08A0E09C[speed] has a bit per frame (GAME + 0x20 & 31) |
| 0x08833B6C | the battle task (task 2): per frame, unless GAME + 0x59: each fighter's CPU routine 0x08833BD4 (from 0x08A0F8B0[fighter + 340], skipped with GAME + 0x88, for a human fighter (+336) and in Final Battle for slots not in GAME + 0x35), then 0x088347DC (CPU common step, or Free Battle's 0x08832678 / Survival's 0x08832898) |
| 0x088E3550 | the network is up (0x09523920 + 0x30 or + 0x25) |
| 0x08828C40 | the fight's set-up (screen 0 of a fight, `fight.c`): game state defaults, the round's flags, three random numbers into BATTLE + 33 / 34 / 80, fighters 1 and 2 started (0x088236C8: slot, held buttons, the party at 0x09517A28 / 0x09517A50), CPU fighters for the slots not in GAME + 0x35 (all of them in Training): +336 = 2, +340 = 3; the game speed BATTLE + 22 from 0x08A0E748[GAME + 0x30]; Dramatic and Variable set BATTLE + 90 (2-on-1) and + 56 = 3; the round time (0x08836538: 0x08A0F9E0[GAME + 0x49]) |
| 0x088238B4 | the fight's screens after 0: 0x08A0E0DC[GAME + 2] (game, battle). 0 by mode (0x088238E8: Survival 0x0882BEA0, mode 16 0x0883087C, else 0x08823970), 1 0x088938C8, 2 steps by GAME + 4 (0x08A0E0F8), 3 by mode (0x08823A50), 4 0x0882B5FC, 5 a mode's end (0x08823B28), 6 quit (0x08828148: sound off, 0x088325DC), 7 0x08828F60 (rounds), 8 0x0882429C, 9 steps by GAME + 6 (0x08A0E180), 10 0x0882462C (steps 0x08A0E190), 11 0x08827B98 (steps 0x08A0E2E0), 12 onward 0x08823D3C / 0x08829460. Decompiled: 0, 1, 2, 8, 9, 10 (0.15.0); 3 (the continue countdown), 4 (GAME OVER), 6 (back to the MODE MENU), 11 (the win screen) (0.16.0); 7 (a round starts: 0x08828F60) (0.17.0); 5 still translated. Several of the mode routines read the third and fourth argument registers as the screen leaves them |
| 0x08821368 / 0x08820BE0 | a random number / the generator: a Mersenne Twister (624 words at 0x08B01564, index at +2496) |
| 0x088D5E44 | a network battle (mode 12, or 0x09523980 + 34) |
| 0x08820DE8 | the fighters' pads (the fighters' step, `fight.c`): each fighter's pad slot (+775) from 0x08A0E024[BATTLE + 154] (4 bytes each; the fourth fighter -1), then for the first three last frame's held buttons (+364) and this frame's (+362) from 0x08A0E074[slot] (pointers to each pad's held word, pad + 4) |
| 0x088D7FF4 | a network battle's input step (`fight.c`): the next message (0x088DC018 / 0x088D5680), or with one pending the local pads emptied (Cancel kept in 0x09523980 + 33) |
| 0x08823BEC / 0x08823C60 | each fighter's per-round reset; in Arcade a non-playing slot's 0x354 bytes at 0x09358528 + slot · 0x354 cleared |
| 0x08823D08 / 0x088303B8 | the round's step by BATTLE + 59 (0x08A0E10C) / each fighter's round step (0x08831558) |
| 0x08834888 | a CPU fighter's label (fighter + 412 into 0x08A0F858: x offset, text) |
| 0x08831558 | a fighter's round behaviour (+371, from +386): 2 where World Tour (+440), Free Battle, a network battle or a human fighter (+439) asks; character 10 by chance in Kumite and Survival's stage 8; else from 0x08A0F434[character + 42 · group], the group by BATTLE + 56, the slot (+337), BATTLE + 79 and mode 17. +430 = character 10 with behaviour 2 |
| 0x08823970 | fight screen 0 for most modes: step on, the fight's state cleared, 0x08821F9C; a continuing match (0x088D7C1C) also 0x088E2304 |
| 0x088938C8 | fight screen 1: modes 0, 8, 9, 15, 17, 18 (and 1 in a continuing match) step from 0x08A27128[GAME + 4]; others 0x088A0664 (reads a3) |
| 0x0882B204 / 0x0882B5FC | Arcade's screens 3 and 4: steps from 0x08A0EEE4 / 0x08A0EEF8 by GAME + 4, then the object lists and the end of the frame |
| 0x08837690 / 0x088379B8 / 0x08887AFC | the object lists (`src/core/object/lists.c`): 0x20 objects at 0x09512770 (0xC0 each, routines 0x08A0FDC8), 0x10 at 0x09514B70 (0xC0, 0x08A101F0), 0x20 at 0x092E2B88 (0x94, active +90, type +91, 0x08A25AA0); the loop counter is GAME + 66 |
| 0x0880B6CC | the end of a screen's frame: the text pass (0x0880B7BC, 0x0880EBF8) unless the frame is skipped, then the sprites finished (0x088224DC, or 0x088225EC while halted) |
| 0x0880B7BC | the text pass (`src/core/object/frame.c`): 40 calls in a fixed order, the HUD and text layers; 0x0880C080 (n) draws the third object list's objects on layer n by kind (+88 & 0x3F: 0x0880F4FC, 0x0881032C, 0x08810754, 0x0880F820, 0x08810128), 0x0880BCBC (n) a layer's queued sprites (counts 0x092E2488[4], entries 0x092E2490 + n · 128; 0x092E2690 / 0x092E2698 while GAME + 0x91), each drawn by 0x0880C308; 0x08812520 is hooked (selectscreen.c) |
| 0x0880BBD8 / 0x0880BD90 / 0x0880BFAC | more sprite queues by layer (`frame.c`), each like 0x0880BCBC: counts 0x092E22F8 / 0x092E2898 / 0x092E29F8, entries 0x092E2300 + n · 96 / 0x092E28A0 + n · 64 / 0x092E2A00 + n · 64; while halted 0x092E23C0 / 0x092E2920 / 0x092E2AC0 and entries 8 bytes on |
| 0x0880BE64 | one queued sprite per layer: flag 0x092E29A8 + n, entry 0x092E29B0 + n · 4 (halted 0x092E29D0 / 0x092E29D8) |
| 0x0880DB78 | the fight's overlay object at 0x092E51C0 (+0 active, +2 shown), skipped while 0x08E7602C or BATTLE + 36: layer 0 0x0880DC88 (the bottom: super / ISM bars), layer 1 0x0880DC08 (health bars), 0x0880DE2C (guard marks, stun fill), 0x0880DF98, 0x0880DFC8; its 0x40-byte elements drawn as pictures by 0x0880E038 or as shaded rectangles by 0x08810B74 (all native: `src/core/object/hud.c`) |
| 0x0880C308 | a queued sprite (`src/core/object/sprite.c`), a tree: +52 and +48 drawn first, then itself into the draw state 0x08AD9320 (+80 / +82 position, y from 0xF0 - +22; anchor 0x0880E6E8; +56 / +60 flips from +11; +48 colour from +160..163; +85 from +168; +44 from +24), drawn by 0x08A04B48[+148] (a0 draw state, a1 sprite), with 0x089E0C4C (1 / 0) around it when +169 and not 0x08E759F8; then, unless +8 = 3, +44 and the next (+40). Nothing is drawn while 0x08E7602C. +86 = 5 sets +164 |
| 0x0880E6E8 | a sprite's anchor: kind +14 (anchor kind 2 first adds +38 · (0x092E5F78 + 28) >> 16) moves the position by 0x08A04014[kind] (- x at +18, + y at +22); +15 shakes it by BATTLE + 74 |
| 0x0880F4FC / 0x0881032C / 0x08810754 / 0x08810128 | object kinds 0, 1, 2 (`src/core/object/draw_kinds.c`) and 4 (translated, no scenario reaches it): a plain rectangle (0x089E10D8), a textured one (texture 0x08E75A18[+54], palette +60, uv +64 / +76, per-corner uv at +68 / +80 when +88 & 0x80; 0x089E1950), a set of kind-1 parts (+132: 12 bytes, last has +8 set; each drawn as a kind-1 object built on the stack), a shaded rectangle (0x089E0F0C, f12 depth). Kind 3 (0x0880F820) builds a shape of many polygons, also unreached. Corners from +6 / +10 by anchor +3, +12 / +14 / +24 / +26, through the float scale 0x08E759E8 / EC (flipped, +89: F0 / F4) and offset 0x08E75A04 / 0C; colours +36 / +42 / +48 (4 corners), alpha +136. Depth 0x08AE03B8[draw state + 44] |
| 0x0880EBF8 | display timers: 0x200 slots at 0x092E8000, each a chain (+0 next) counted down at +15, unlinked when it runs out |
| 0x088224DC / 0x088225EC | the sprite counts reset for the next frame (with a0 = 0 three more), and 0x092E29A8 - AF / while halted, only 0x092E2898 - 99 |
| 0x08828F60 | a round starts (fight screen 7): the round's state cleared (BATTLE + 8 the round number from GAME + 0x4C, + 9 from 0x08A0E098[GAME + 0x2F], + 10 = 0x3B); World Tour takes the fight's kind from 0x09517A78 + 904 (1, 5, 2, other); modes 13 / 19, Survival, Free Battle (by + 159), 17, Kumite (+ 72 = 99), Training (0x0884A07C), mode 6 (a random stage of 37 from 0x08A0E750); otherwise a continuing match (0x088D7C1C or GAME + 186, or GAME + 0x39 = 3) keeps fighters' + 352 modulo 100; then 0x08823BEC, 0x0883206C, 0x08823D08, 0x088303B8 (game, battle) |
| 0x0883206C | the round's kind (`fight.c`, BATTLE + 59, with + 85 / + 104 / + 106 / + 109): early (GAME + 74, BATTLE + 61, GAME + 146) 3 for modes 13 / 19 or a continuing match, else 0; World Tour (GAME + 137) from 0x09517A78 + 904 / 910 / 940; a fight under way (BATTLE + 79: 7 Survival's pair from 0x095179F8[BATTLE + 20], 8, 6, 4 / 5, the 0x1E stage); otherwise by the fight's number BATTLE + 20: 4 → 4, 8 → 5, 9 → 6 or the character's boss (0x08A0F64C, checked by 0x08830384) as 2, and boss 0x1E as 1. The me / other fighters by GAME + 57 bit 0 |
| 0x0889562C / 0x08895BD4 | screen 8 sub-screens 0 / 1 (`fight.c`): wait while BATTLE + 61 / 159 / 90 / 79 (and some modes), then start the win (winner by GAME + 57: + 411 bit 0, voice 0x0882E110 (+338), pose 0x08813AC4 (+338, +376), text 0x109) and run 0x08A27284[GAME + 10] / 0x08A2729C[GAME + 10] (the second called with a1 = the routine) |
| 0x088246DC | screen 10's frame offline (`fight.c`): the round's clock (BATTLE + 28 seconds, + 29 ticks, + 30 rate, + 78 between seconds), GAME + 59 counted unless BATTLE + 52, the round's steps, the objects' steps (halted: 0x0888D64C, 0x088D08E8, 0x088D2B5C), 0x088289A4 / 0x08828B88, 0x0881DF10, the end of the frame. 0x0882485C is the network version (translated) |
| 0x0881DF10 | every fight frame's effects (`fight.c`): BATTLE + 74 shake (BATTLE + 35 counts down, odd counts shake by count >> 2), fighters' + 591 gathered at 0x08B01560 by BATTLE + 56 (1: fighters 1, 3, 4; 2: 2, 3, 4; 3: 1, 2, 3), camera layer + 123 = 15 unless BATTLE + 36, then 0x0881D1A8 |
| 0x088E30FC / 0x088DBC7C | the network's part of every fight frame (`fight_network.c`): packets at 0x09523920 handled by 0x08A3EBB8[packet + 212], the session failing after 1200 frames (+16 = 0x81001005), the fight halted (GAME + 0x59 / 0x91) while + 49; the network text 0x09523980's steps 0 - 6 by + 0 |
| 0x08823B28 | fight screen 5, a mode's end (`fight.c`): BATTLE + 69 (set by 0x0882AE20, from 0x088271DC once Arcade's final fight, BATTLE + 20 = 9, is won; 0x0882B394 also sets it after a continue at fight 9 with BATTLE + 85 clear) 0 / 1: the first frame 0x088162FC, 0x08815870, 0x089E6050 (2), the next starts the ending task 0x089DE0E8; otherwise 0x088D1634. Forcing BATTLE + 20 = 9 before a fight makes its win end Arcade (`arcade_ending`) |
| 0x08828AF4 | a fighter's pause check (`fight.c`, wrapped by `hooks.c`): pad PADS + 44 · fighter + 775; not while GAME + 0x59 or the player (+337) is barred by GAME + 135; pressed 0x8000 (Start) pauses (GAME + 0x59 = the pad's bit); a pad whose +0 word is 0 pauses too, with GAME + 138 and 0x08822BB0 |
| 0x0888CC78 | the save error question's Yes / No (`mode_menu.c`): cursor 0x09523C60 + 9 (1 Yes, -1 No), up / down with sound 0x38, cancel (0x100) → -1 (sound 0x2F), confirm (0x8080) → the cursor (sound 0x39) |
| 0x08894040 / 0x08894138 / 0x08894214 | the character select screen (`src/core/select/screen.c`): its step (start once GAME + 6 is 0, then the frame), the start (BATTLE + 90 or frame & 0xFF = 7: fighter + 0x338 on for the players in GAME + 53, sound 0x124), the frame (cursors via 0x088945C0, player 2 first when BATTLE + 90; names from 0x09523454 (network, 208 per side, 3 × 0x3C) or the six slots at 0x09519588 (0x354 each), text 0x08A27148 / places 0x08A27178, font 9, blinking on frame & 3; done when (GAME + 57 \| + 41) = GAME + 10: GAME + 4 on, + 43 = 1, + 72 = -1, frame \| 0x100, team modes copy + 53 to + 57 / + 41) |
| 0x088945C0 | a select cursor: while fighter + 0x338, state 0x08A271D8[+4] (me, other); states 0 - 3 step by +5 through 0x08A271EC / 0x08A27264 / 0x08A27258 / 0x08A27270; state 7 0x0889494C hands the cursor to the partner in modes 8 / 15 (GAME + 41 = 3); 0x088954E0 finishes a choice (GAME + 10 and + 57 bit 1 << slot, BATTLE + 0 = the character, 0x088969FC) |
| 0x08894644 … 0x088969FC | the select cursor's states (`src/core/select/cursor.c`; fields in `select.h`). State 0, the character: enter 0x08894644 (cursor to 0x08A270B7[character] = column, row), move 0x08894768 (on an entry's box from 0x08A26FC2 → +127 = 1-6; attack buttons 0x3F0 choose, directions move, a random box draws), grid 0x08897008 (7 × 7 s8 at 0x08A26F60: character, -1 empty, 0x40 random box for side bit 0, negative network-only boxes), random 0x08894A84 (pool 0x08A271FC of 38, hidden ones via SYS + 73 + n and 0x08A27222; ISM drawn from 0x08A27228), chosen 0x08897344 (entry copied to 0x09358528 + slot × 0x354 from the six at 0x09519588 or the network's at 0x09523454; SELECT extras; colour by button 0x20 → 2, 0x40 → 4, 0x80 → 1, 0x100 → 3, 0x200 → 5, flipped (^ 1) if the other player has the same character and colour), wait 0x08894990. State 1, the play style (+438: 0 NORMAL, 1 MAZI (+428 = 1), 2 SAIKYO (+385 = 1), 3 CLASSICAL (+431 = 1, ISM decided), 4 (+373 = -1; offered only to Final Bison by 0x088A4F1C, and masked out by 0x0889507C)): enter 0x08894FC0, choose 0x0889507C (styles offered 0x088A4F1C, by mode), chosen 0x088951DC, wait 0x08895248. State 2, the ISM (+386: -1, 0, 1): enter 0x08894C74, choose 0x08894D40, chosen 0x08894E5C (Chun-Li 4 ↔ 0x12, Sodom 6 ↔ 0x14 by ISM), decided 0x08895564, wait 0x08894F4C. State 3, the speed: enter 0x08895338, choose 0x088953CC (GAME + 20 low byte; BATTLE + 22 = 0x08A2727C[it + GAME + 118 - 1]), wait 0x088954A0. Counted by 0x088969FC: SYS + 174 + character × 8 + ISM × 2 (ISM 2 for an entry). Fighter + 78 / + 424: chosen / done bits (1 character, 2 ISM, 4 style, 8 speed); +82 the step's timer (0x258 to choose, network; 0x20 shown) |
| 0x088A0664 … 0x088A5B38 | the Survival / Free Battle select (`src/core/select/battle_select.c`): steps by GAME + 4 from 0x08A2B5E4 (0 the start's sub-steps by GAME + 6 from 0x08A2B5F4: 0x088A08E4, 0x088A0ADC, 0x088A0CE4, 0x088A0D44, 0x088A51A8; 1 0x088A12C8; 2 the frame 0x088A13F8; 3 0x088A1F98). Layout BATTLE + 56 (0 two players, 1 / 2 player 1 / 2 against FIGHTER_3 + 4, 3 two against one). Cursors by fighter + 4 from 0x08A2B7D8 while + 824, called with (fighter, other, list, list) (lists 0x09517A28 / 0x09517A50 by GAME + 53). Text colour 0x08B02716 from 0x08A2B608[mode]; titles 0x08A2B630; SPEED: 0x089FF444 and 0x08A2B688; stage 0x089FF44C ("STAGE:") and 0x08A2B428[GAME + 193], hidden by GAME + 195; entry names 0x08A2B698 at 0x08A2B6B0. Players expected done 0x088A1D30. Cursor states by fighter + 4 (steps by + 5): 1 the character 0x08A2B84C (0x088A2870, 0x088A29B4, 0x088A315C), 2 the play style 0x08A2B918 / modes 13-19 0x08A2B924 (0x088A34A8, 0x088A35E4, 0x088A373C), 3 the ISM 0x08A2B930 (0x088A382C, 0x088A3914, 0x088A3B70), 4 0x08A2B948 (0x088A3F98; flag 0x10), 5 0x08A2B960 (0x088A4410, team kinds only; flag 0x100), 6 the difficulty 0x08A2B954 (0x088A4210, 0x088A42E0: GAME + 190 0-7, 0x088A43BC; flag 0x80), 7 the speed 0x08A2B93C (0x088A3D0C, 0x088A3DE8: GAME + 124 bit 0, 0x088A3E80: BATTLE + 22 = 0x08A2B3E0[GAME + 124 + GAME + 118 - 1], 0x088A3EF8; flag 8), 8 the stage 0x08A2B96C (0x088A45B8, GAME + 194 = shown; 0x088A4650: GAME + 193 0-38, BATTLE + 24; 0x088A4760; flag 0x40), 9 done 0x088A47A0 (BATTLE + 0 / + 100 = 0x08A2B3E6[stage - 1], or [rand & 63] for RANDOM). A cursor's list (0x09517A28 / A50): + 0 last member, + 1 member now, per member + 3 character, + 6 ISM, + 15 box, + 18 entry. Fighter + 112: rows the mode skips; + 362 / + 364 the driving pad's held buttons now / before (0x088A5174 picks the fighter). Boxes 0x088A60F8 (fighter, which): places 0x08A2B4C4 (36 bytes per side: heading, then rows), headings 0x08A2BA48 (by side; + 8 the ISM names by ISM + 1; + 20 modes 13 / 19; + 40 Free Battle by box), colours 0x08A2BA78, two-digit buffers 0x08A2BA7C (five per side), rank names 0x08A2B5B0; labels "ISM" 0x089FF564, "WIN" 0x089FF568, mode 1's record " V:" / " S:" / "SS:" / " P:" / "VITAL" 0x089FF56C on (fighter + 433 - 437); style line 0x088A4D30 ("MODE", names 0x08A2BA34 by + 438), difficulty line 0x088A2578 ("DIFF", names 0x08A2B820 by GAME + 190), mode 6 / TEAM's lines 0x088A2480 (0x08A2B810, team size + 441). Free Battle's whose-turn 0x088A5600 by BATTLE + 159. Grid 0x088A6A10: boxes 0x08A2B378 (s8, 0x40 + kind for a random box, < 0 none), locks 0x08A2B3A9 (-1 open). Start: 0x088A08E4 (entry boxes 0x08A29A28, 6 bytes each), 0x088A0ADC (0x088A6C74 per fighter; display tasks 0x088A4874: kind 0x47 from 0x08A2B996 / 0x08A2B978), 0x088A0CE4, 0x088A0D44; end 0x088A1F98 |
| 0x0889472C | a character barred (GAME + 148 / + 152, bit per character) |
| 0x088D7C1C | a continuing match: NETWORK (mode 12) or GAME + 146 (`fight.c`) |
| 0x092E3F10 / 0x092E43A8 | fighters 1 and 2's health (s16). Set to 1 at a round's start, the next hit is a KO (the `arcade_match` scenario does this with `--poke`); held at 1 through the KO, it stalls the KO sequence. BATTLE + 46 and 0x09524A68 hold the round's starting time, not the running clock |

## Step dispatchers, text and small helpers (decompiled 0.61.0)

**Step dispatchers** (`src/core/object/step.h`, `steps.c`). Objects, fighters, effects, tasks and screens keep
their progress in step numbers (a routine number, then steps within it: usually the bytes at +4 to +7 of
the object; fighters also use +128, +236, +488 and +692), and each level is run by a 13-instruction
function that calls `table[step]` with the arguments it was given. The program has 1,160 of this exact
shape; the 268 the scenarios run are decompiled, one line each with the step read and the table. The
handler and the table are left in a1 / a2 (or a2 / a3), which some handlers read.

| Address | What |
|---|---|
| 0x0880A6CC | text(string, font, x, y, colour) (`src/core/text/queue.c`): takes the first free of 96 requests at 0x092DF240 (16 bytes: +0 used, +1 font, +2 colour, +3 kind = 7, +4 x, +6 y, +8 string, +12 cleared); returns the request, or 0 when the queue is full. Drawn and emptied by the text passes 0x0880B098 / 0x08812520 |
| 0x0880A3C4 | a draw depth (`src/core/stage/depth.c`): u16 at 0x08A03B10 + column × 4 + row × 2 (a0 row, a1 column) |
| 0x08861D5C / 0x088622C0 / 0x088623BC | a fighter's held directions, fighter + 752 (`src/core/fighter/buttons.c`): left / right unless up or down is held; left / right; down |
| 0x088639C0 / 0x0886A2D8 | what a fighter pressed this frame, fighter + 760: the six attack buttons (0x3F0); SELECT (0x4000) as 0 / 1 |
| 0x088D7CE0 / 0x088D5E7C | a network battle (mode 12 or 0x09523980 + 34) whose session has started (+ 42 = 1) / that waits for the host's choice (+ 43 = 2 or 4) (`src/core/network/state.c`) |
| 0x088D7E80 / 0x088D7E90 / 0x088D7F18 / 0x088D7F28 | the network state 0x09523980: + 59 read; + 60 cleared (the end of every frame); + 8 read (the session's failure); + 267 read, setting + 61 when it is set |
| 0x088E3FE0 | 0x09523960 + 25 cleared |
| `src/core/empty.c` | 40 functions that only return (steps with nothing to do, routines compiled away) |
| 0x0881E49C / 0x0881E4E4 | the camera's step right / left (`src/core/stage/camera.c`): layer + 34 moved by a1 (5 at most; a1 negative for left); answers the new x, or the limit (+ 70 right, + 68 left) once past it, leaving the stored x for the caller to correct |
| 0x0881EB78 | the higher of two fighters' y (+ 22) into `*a2` |
| 0x0882196C | an object's animation started (`src/core/object/animation.c`): + 28 = the entry, + 74 - 77 = its first four bytes, + 152 = 0 |
| 0x0882D95C | a fighter recorded in a trail buffer (`src/core/fighter/trail.c`): (fighter, buffer, index): u32 [index] = fighter + 28 (animation), s16 [+ 512] = x, s16 [+ 768] = y, u8 [+ 1024] = facing (+ 11) |
| 0x0888A1B0 / 0x0888A1EC | falling objects (0x94 bytes, `src/core/object/drift.c`): x (+ 4) += + 104, wrapping to 128.0 (256.0 for type + 92 = 0x11) at 0 or less; y (+ 8) += + 108, back to 0 at 240.0 |
| 0x088593D8 / 0x088622CC | fighter + 697 = the opponent (+ 92) is to the right, noted only when more than 24 apart; the fighter faces away (+ 697 ≠ facing + 11) (`src/core/fighter/state.c`) |
| 0x08859C10 | a fighter's slide: + 690 frames left, x moved by the s8 at 0x08A190F7 + frames, forward by facing |
| 0x08861E94 | up held (+ 752 & 8) and nothing halting (BATTLE + 13, + 14, GAME + 38) |
| 0x08863384 | the other side's first fighter (by + 337 bit 0) into `*a1` and `*a2` |
| 0x08834604 | Start pressed this frame on the pad of the fighter's side (0x08E75BA0 + 12, or + 56), unless a1 + 6, a0 + 43 or a0 + 40 |
| 0x0887BA84 | a fighter in routine 1, step 0 (+ 4, + 5) whose part (+ 6) is not 3, 5, 7 or 8 |
| 0x0885971C, 0x08872994, 0x08873D30, 0x088741C8, 0x0887BD00, 0x088347B4 | fighter fields not named yet (+ 366, + 248, + 722 - 748, + 270 / + 272 / + 296, + 251 / + 262 / + 528, + 412): see `state.c` |

**Objects' shared steps** (0.63.0, `src/core/object/motion.c`). The 0xC0-byte objects (and fighters, which begin
the same way) keep x and y as 16.16 at + 16 / + 20, speed at + 96 / + 100, acceleration at + 104 / + 108, a
goal at + 112 and the target being worked towards at + 114, a timer at + 82, the floor's y at + 132, the
owner at + 88, and frame data at + 296.

| Address | What |
|---|---|
| 0x08862004 | x, y by speed, speed by acceleration; 1 while y + frame data byte 3 is above the floor |
| 0x0889CE64 / 0x0889CEA4 | x / y by its speed, the speed by its acceleration; 1 once the speed points away from the target |
| 0x0889E704 | y by its speed, wrapped into 0 to the height in + 76 / + 77 |
| 0x088ADA3C / 0x088ADA8C | left, braking to speed 2.0 (then substep on, timer \| 0xC00) / left while the timer's high byte counts down |
| 0x088BBB88 / 0x088BBDC8 / 0x088BBBBC | right by 4 to x 48 / down by 2 to the floor (then timer 150, step on) / the timer counted down, step on at 0 |
| 0x0889E674, 0x0889F01C / 0x0889BE20 / 0x088B1454 / 0x088B2634 | waits on GAME + 20 (negative), + 22 (high byte), + 20 bit 15, + 20 bit 8 before aiming at the goal or stepping on |
| 0x0887DA58 | (object, table): the owner's place and facing, plus the table's s8 pair for the owner's frame (frame data byte 9), x mirrored |
| 0x0887DF88 | (object, n): animation 0x08A24120[n] started |
| 0x08887C24 | the object's corner colours (+ 36, + 42, + 48, four bytes each) from the 15-bit colour 0x08A25B38[BATTLE + 102] |
| 0x08809A08 | four u16 into the next of 64 entries at 0x092D9EF0 (index 0x092D9EED) (`src/core/misc.c`) |
| 0x088232B0 | a record cleared, its kind (+ 8) set |
| 0x088939AC | the mode is neither 8 nor 15 |
| 0x088ABA0C | once GAME + 16's low byte reaches 7: object + 17 on, 0x08B02800 = -0x180 |
| 0x088AC4D8 | the fighters' layer + 123 bits 0 - 2 from an object's + 78, + 79, + 80 |

**Drawing, animation and the camera's layers** (0.64.0).

| Address | What |
|---|---|
| 0x088216FC / 0x08821780 / 0x08821828 | an object added to a sprite queue at its layer (+ 9), both sets of entries and counts (`src/core/object/frame.c`): the single queue; the layers' queue (not while BATTLE + 36, unless object + 72); queue D (not while BATTLE + 36) |
| 0x088215C4 | an object drawn: + 1 = 1, then the drawer 0x08A0E080[+ 8 - 2] |
| 0x088219B0 | an animation's frame (`animation.c`): + 74 counted down; at 0 the next entry (8 bytes: + 0 frames, + 1 flags, + 4 picture, + 6 s16 entries to the next), + 152 = the picture changed; an entry with flag 0x40 ends it (+ 75 = -1) |
| 0x08821330 | 0x08A0E05C[object + 14] |
| 0x0883771C | a fourth object list (`lists.c`): 0x09515770, 42 of 0xC0, routines 0x08A10040 by + 2 |
| 0x08819BBC / 0x0881EE84 | callback tables run each frame unless BATTLE + 37 or + 52: 8 entries of 0x14 at 0x095177C8 (routine at + 16), 12 of 0x18 at 0x09517868 (+ 20) |
| 0x0881E058 / 0x0881E148 | a background layer's x / y from the camera's travel (0x092E88A0 + 32 - + 8, less 0x092E5F78 + 112 for x) × rate / 16, plus the layer's + 8 / + 12 and + 80 / + 84; + 59 = 8 or 12, + 58 = 4 or 0 by the limit + 40 / + 44 (`src/core/stage/camera.c`) |
| 0x0881E7DC | two fighters' span: the least x - (+ 232) and the greatest x + (+ 232) |
| 0x08822FE4 | an object made (`pool.c`): the first free one looking down from a0 through a3 of them, kind + 2 and type + 3 set, the pool's free count (*t0) one less |
| 0x0880E150 | a sprite's picture drawn as a grid of cells (a0 draw state 0x08AD9320, a1 the sprite, a2 the picture: +0 cells, +4 columns, +5 rows, +6 / +8 origin, a3 its base, t0 a flag): by the flag and the sprite's +164, one of 0x08810D84 (16-pixel cells from the picture cache, 16 to a page, uv 0x08A04BE0), 0x088112A8 (32-pixel, 4 to a page, uv 0x08A04C60), 0x088117D0 (16-pixel, each cell its own palette and texel cell) and 0x08811BCC (32-pixel, the same). A cell is 3 bytes (0xFF empty; the last: bit 7 / 6 flips, bit 5 blends) and is drawn unless it starts at or right of draw state + 20 or at or left of minus its width (+12 / +16); scaled sprites (+87) have 1.2 times wider cells. WIDE NATIVE: the left limit is -96 in all four (Rickow's patch had it in 0x08811BCC only); with `sprite_grid_wide_edges` set, each row's last cell is drawn again out to both margins (the ROUND band, FIGHT's translucent box). `src/core/object/sprite_grid.c` has all four |
| 0x0880C4C8 / 0x0880C598 / 0x0880C70C / 0x0880CAA8 / 0x0880CF60 | sprite drawers 0, 1 / 2, 3 (as 0, picture number at +84), 6 (as 1, pieces from +156) and 11 (`object/sprite_drawers.c`; a0 draw state, a1 sprite): the frame is (+28) + 4 (bit 15: nothing), pictures from 0x08E75AD8[+86] (12 bytes; each with a nonzero +10 goes on into the next), through 0x0880E150 with +170 / +171 copied to the draw state's +86 / +87. Drawer 0: one picture. Drawers 1, 2: pieces from 0x092E3E98[drawer - 1] (6 bytes: x, y, picture; bit 15 last, bit 14 not drawn), drawn last first at the sprite's place plus their x, y. Drawer 11: pieces from the sprite's +156, all at the sprite's place, a piece with a nonzero x at half alpha (the after-images of ROUND / FIGHT). WIDE NATIVE: bands continued (ROUND, FIGHT's after-image box, GAME OVER, the continue bar); a row of one piece at even steps (FIGHT FIGHT, KNOCK OUT) gets a piece past an end at the 4:3 edge; the continue count (kind 13 type 3, digits 572 + d / 582 + d) its earlier numbers; the map's beam (kind 19 type 2) scaled copies behind it in the margins |
| 0x0880D468 / 0x08811FF4 | sprite drawer 14 and the mask layer's cells (`object/mask_layer.c`, 0.99.25): the text pass's mask layer (512 x 224 texels at *0x092E22E4, cleared to opaque black by the screen's setup, drawn by 0x0880F160) gets holes (texel 0) where a picture's 16 x 16 cells have mask bits (32 bytes a cell, table 0x092D9D40[sprite + 86]; cell 3 bytes: number, flips; 0xFF empty); cells taken at x -16 - 383, y -16 - 223, rows 1 - 16 below the cell's top. Drawer 14: pieces at the sprite's +156, last first. Drawer 13 (0x0880D3B0, translated) uses it too: the attract mode's name rows and fighters' outlines. WIDE NATIVE: x -64 - 447, the left margin's texels at the rows' ends (464 - 511), drawn there by 0x0880F160 |
| 0x0880C1AC / 0x088108D8 / 0x0880C208 | the text pass's three text boxes (0x092E3E08, 0x30 bytes: +0 in use, +1 shown, +10 depth index, +14 first glyph, +26 x, +30 y, +35 characters, +36 string: an s8 line offset, characters, 0 = new line 16 lower; 8 × 16 glyphs from texture row 0x50) (`object/text_boxes.c`); the super finish's backdrop (BATTLE + 36 set, kind BATTLE + 43 not 0 / 11 / 12: picture 0x08A04B2C[kind] from 0x08E75AD8 + 68, through 0x08811BCC) (`object/super_backdrop.c`) WIDE NATIVE (0.99.22): the backdrop's vertical and speed lines (kinds 3 - 5) mirror their side cells out, the other kinds are drawn whole 1.25 times over-sized about (192, 112) |
| 0x0880E434 / 0x0880D848 / 0x0880D8C8 / 0x0880D988 | the wipes' flat shapes (`object/shapes.c`: a0 place +0 x, +2 y, +44 depth index; a1 shape, quads from 0x08A042C8[a1] ending 0xFFFF (4 corners: top left, top right, bottom right, bottom left; a shape is a third of the screen, 128 wide; shapes 0 - 2 X, 3 - 5 A, 6 - 8 V; WIDE NATIVE: a quad's edge on or past the 4:3 side gets a rectangle out to the screen's side), colour 0x08A04B88[a1 / 3] with alpha 0x08A04B98[a2], blended unless a2 = 7; drawn three side by side by 0x0880D5B4, faded by 0x0880DA78); the single object 0x092E5100's six slots (`object/single_draw.c`: +0x20 - +0x70, kind = top four bits of the slot record's +4: 4 / 0xB a picture 64 left and 16 above the slot from the list 0x08E75AD8 + 28, 0xA the fading shape): the logo between the VS screen and the fight |
| 0x088BC164 | the attract ranking screen starts (`object/ranking.c`, 0.119.0): table 0x08A05AA8 + cycle (0x09358400) × 0x2F8 into 0x09525DE8 (8 tables × 38 records of 0x14: +0 score, +4 name, +8 character, +9 colour, +14 kind 0 - 5, +15 new; 6 / 7 the entries', written by 0x08892A98, never shown by the game), palettes by +14 % 3 from 0x09335A00. Title band: kind 0x23 types 6 / 7, animation 0x08A38474[+14], pieces 0x08A38208 (letter pictures 0x2A X, 0x2B Z unused, 0x2C V, 0x2D A; 0x2E "-ISM") |
| 0x08885A44 | the logo between the VS screen and the fight starts (`object/screen_steps.c`, 0.118.0; the single object 0x092E5100, type 0xD): six slots from 0x08A257B8 (x, y, +10, +11, an animation from 0x08A25758; below 12 the wipe's shapes, + 0 / 3 / 6 by ISM), the logo's palette 0x08A25826 + 0 / 0x20 / 0x40 into palette 0x1E, by the ISM (X -1, A 0, V 1) of fighter 3 (BATTLE + 56 = 3), 2 (BATTLE + 48) or 1. An entry (I-ISM) plays as its own ISM; `extensions/i_ism.c` gives it a yellow I (shapes 9 - 11 of its own) and a gold palette |
| 0x0880E388 | a colour dimmed by the fade: each of R, G, B less (0xFF - level), the level from 0x092E11A0[owner + 13] or 0x092E1220; not for objects with + 85 |
| 0x0880EB84 | the picture cache (512 pointers at 0x092E8000): the entry in use (+ 14) with the same data (+ 4), + 12 and + 13; + 15 = 8 |
| 0x0882D6FC / 0x0882D794 | fighter 1's / 2's trail frame (`fighter/trail.c`): the place (0x09523CA0 / 0x09523C9E) on by one, 0 - 127, and the fighter recorded there |
| 0x0888D8BC | the clock's digits from BATTLE + 9 (the round's clock as shown) |
| 0x0883727C | fighter + 485 = 0 - 3, the worse of the clock (under 80 / 60 / 40) and health + 112 (under 112 / 72 / 32) |
| 0x08859238 | fighter + 508 = the distance to the near edge of the opponent's box |
| 0x0886A1E4 / 0x0886A264 | a punch and the same-strength kick changed together this frame (+ 760 \| + 762 = 0x90, 0x120, 0x240) with + 366 ≥ 72, for V-ISM (+ 386 = 1) / among the changes, with + 764 bit 1 |

**A fighter's input, script and hits** (0.65.0).

| Address | What |
|---|---|
| 0x08859650 | a fighter's input for the frame (`fighter/state.c`): + 752's left / right swapped when facing left (+ 11); + 760 = bits that came on against last frame (+ 754), + 762 = bits that went off, + 666 = bits that came on in any of the last three frames (+ 754, + 756, + 758), + 290 = the turned bits; + 764's left / right swapped by + 697 |
| 0x08877564 | the fighter's script run: handler 0x08A22498[+ 489 & 0xF](fighter, value), each given the last one's answer, until the budget 0x08B0268E is used up |
| 0x0886EDA8 | a fighter's move routine 0x08A1BFF0[+ 656, or + 670 while + 669]; then + 657 = + 656 |
| 0x088772C0 | a fighter's routine 0x08A22488[+ 484] unless BATTLE + 14 or + 788; + 506 held to 255 |
| 0x08862F28 | (fighter, opponent, push): the opponent's x less the push while the fighter is in routine 1 step 1 (conditions on + 537, + 73, + 541, + 239 and the opponent's + 713, + 73, + 596) |
| 0x08864140 | the wall jump check: characters 4, 7, 0x12, 0x1C, 32 above the floor, + 238 the wall touched (1 / 2), moving into it, holding away |
| 0x08862B84 / 0x088632C4 | the partner / both leading fighters in routine 1 step 6 |
| 0x0885911C | fighters whose frame data byte 1 is clear clear + 570 / + 571 on the fighters across |
| `fighter/hit.c` | 0x08830384 (one of 48 flags at + 400), 0x08872174, 0x088723D4, 0x08872540, 0x08872CD8, 0x08873054 (damage less for the fighter ahead on health), 0x08873448, 0x0887347C, 0x08874C90, 0x08877F44 (back to routine 1), 0x08864620, 0x08864D40 (+ 13 from 0x08A19622 by layout and side), 0x0886DC3C. The hit's working record is 0x09358410 (+ 24 the other fighter, + 44 a flag) |
| 0x0881E760 / 0x0881EB14 | the camera's span / height routine for a pair of fighters, from 0x08A09438 / 0x08A09458 by which of them has + 579 (left out) |
| 0x08854C10 / 0x08886E18 | object lists 5 and 6: 0x09512170 (8 of 0xC0, routines 0x08A16D18), 0x092E3E08 (3 of 0x30, 0x08A259F8) |
| 0x08823304 | list 3 emptied (0x1280 bytes at 0x092E2B88; free count 0x095176F9 = 32) |
| 0x0888D64C | the object at 0x092E51C0 each frame: its routine 0x08A26CB0[+ 2], then + 6 counts frames, + 8 every fourth, + 9 every third |
| 0x0888DA7C | an object's two parts (+ 12, + 32) run through 0x08A26CBC |
| 0x088A96B8 | a sequence writing GAME + 16's low byte from 0x08A2C536, timed by 0x08A2C1A8 and 0x08A2C20A (state 0x08B02840 / 44) |
| 0x0888BB24 | three lines of text: 0x089FEFE0, 0x089FEFF4, 0x089FF014 |
| `object/motion.c` | BATTLE + 44 is the fight intro's counter: 0x088560EC (at 2), 0x088562A0 / 0x08857550 (at 1), 0x08856650 (5 on), 0x08854F10 (sets it to 2); also 0x08854ED8, 0x0885661C, 0x08857D70, 0x08857E24, 0x0887E10C, 0x0887D990, 0x08881E2C, 0x08899430, 0x0889995C (an 8 × 8 grid), 0x0889BD9C, 0x088A0464, 0x08887018 |

**Pools, clears and the screens' objects** (0.66.0).

| Address | What |
|---|---|
| 0x08822E74 / 0x08822EAC / 0x08822F1C / 0x08822F54 | an object made (kind, type) from the pool of 8 (0x09512170), 32 (0x09512770), 16 (0x09514B70) or 42 (0x09515770); free counts 0x095176F2 / F3 / F5 / F6 (`object/pool.c`) |
| 0x0882319C / 0x088231C8 / 0x0882324C | an object freed to the pool of 8 / 32 / 42 (+ 8 = 3 / 4 / 6) |
| 0x08823278 / 0x088233BC / 0x08823074 | a record freed (count 0x095176F8); a list-3 object freed (0x095176F9); the first free of the 12 callbacks taken |
| 0x0880E998 | the picture cache's 512 entries (16 bytes at 0x092E6000) emptied |
| `reset.c` | clears: BATTLE (0x08822D04), fighters 3 and 4 (0x08822C4C), 0x092E51C0 on (0x08822B60), 0x092E5100 (0x08822A64), the text queue (0x0880A87C) and two more queues (0x0880A8B8, 0x0880A8F4), 0x092DCA40 filled with 0xFF (0x0880A39C), 0x08E76068, 0x08E86E20 |
| 0x0881D488 | the stage's three bytes from 0x08A09310 into + 122 of the layers 0x092E8800, 0x092E88A0, 0x092E8940 |
| 0x088192C0 | the stage's own routine, 0x08A0893C[stage], if any |
| 0x0881E64C / 0x0881EA10 / 0x0882D5D8 | by layout (BATTLE + 56): the camera's span routine 0x08A09428, its height routine 0x08A09448, the trails' routine 0x08A0F1A4; the usual ones are 0x0881E688, 0x0881EA4C and 0x0882D614 (fighters 1 and 2; trails at 0x09523CA8 and + 0x480) |
| 0x0887A10C | a script command read: the word into fighter + 488, the place after it into + 528, the budget 0x08B0268E up by one; a command whose high byte is -1 goes to 0x0887A154 |
| 0x0887B504 / 0x0887B7CC | a fighter's own routine (+ 416 → + 84); + 480 - + 484 reset and the + 484 routine run |
| 0x0895CD24 / 0x0895F2E8 / 0x0895FD00 / 0x0895FDB0 | Cammy's own routine (character 0x16: + 416 → + 84, set up by 0x0895BEEC from 0x08A858C4; reached through the CPU's script commands): by + 236 from 0x08A85984; routines 23 and 24 step by + 7 from 0x08A85C10, whose states 19 and 20 (`network/screens.c`) give 0x0895FF6C the screen's width (WIDE NATIVE: 480) once she is far enough from her opponent. In the scenarios a CPU Cammy only ran routine 4 |
| `object/screen_steps.c` | the screens' objects wait on their fighter's flags at + 424 (bits 1, 2, 4, 8: 0x08899DC8, 0x0889A194, 0x0889ADFC, 0x0889A478, 0x0889A728, 0x0889C604), start animations from 0x08A27744 (0x0889CDC8, 0x0889CDF8) and 0x08A28950 (0x0889CE34), and move to their targets (0x0889B888, 0x0889BE88, 0x08899514) |

**The fight's intro, character routines and script commands** (0.67.0).

| Address | What |
|---|---|
| 0x08854CD0, 0x08854F94, 0x08855EEC, 0x08856168, 0x08856364, 0x08856520, 0x08857364 | the intro objects' routines (`object/intro_steps.c`): the handler for + 4 from 0x08A16D40, 6D64, 6E58, 6E78, 6E94, 6EB4, 6FAC; while GAME + 38 the object goes to step 2 |
| 0x08857AC8 | an owned object's routine 0x08A17110[+ 4](object, owner) unless BATTLE + 36 |
| 0x08856058, 0x0885609C, 0x0885612C, 0x088562CC, 0x0885757C, 0x088575C8 | the intro's moves in x: until past a place or out of (into) a range, then the goal + 112 and the next step |
| 0x08854E78 / 0x08854F54 | falls to y 128 (intro stage 1) / drops by 24 to y 288 (intro stage 3) |
| 0x08855D44 / 0x08855D80 | an owned object made from the pool of 8; (object, layer, x, y) placed |
| 0x0885F838, 0x08863918, 0x0885B7D4, 0x0885C1EC | a character's own routines, from the table fighter + 416 points at: + 76; + 80 (after + 564 = 0); + 64 (after the countdowns + 638, + 643, + 542); + 88, or + 92 while + 373 |
| 0x08864494 | characters 0x11 and 0x13 swap when bit 0 of + 627 asks (the table's + 140 list is called) |
| 0x08861DE4 | the walking speed: + 96 from the character's pair in the table at + 704 (forward unless back is held), by facing |
| 0x08878A20 / 0x08879D98 / 0x08879FC4 | script commands: nested (depth + 501, handler 0x08A22598[+ 490]); its end; bits into + 544 for fighters above a level (+ 375) |
| 0x08877258 | a chance by level: table[+ 375] & (1 << random 0 - 31) |
| 0x08837238, 0x088591F4, 0x08872950 | once a frame for all four fighters: 0x0883727C, 0x08859238, 0x08872994 |
| 0x088590E4 | the hit record's first word (0x09358410) = a random one of the 32 words at 0x08A18FC0 |
| 0x088988FC | a fighter's display side: 0x08A2A116 by layout and side |
| 0x08822F8C | one of the three 0x30-byte objects made (list 6) |
| 0x0882E0D0 / 0x0882E110 | the fade back to full; BATTLE + 102 (the stage's colour number) from 0x08A0F1C4 |
| 0x08836E60 / 0x08836E90 | the s16 at 0x09524A68 changed, and held to 0 - 255 |
| `object/screen_steps.c` | objects report arriving with their bit (by type) in GAME + 20 (0x0889E6A8, 0x0889F894, 0x0889EBFC, 0x0889F050); 0x0889CF90 sets a y motion from 0x08A2A148; 0x0889610C drops the top layer (0x092E8800) to y 256 |

**Random numbers, the director and damage** (0.68.0).

| Address | What |
|---|---|
| 0x08820B7C | the random numbers seeded (`misc.c`): a Mersenne Twister, 624 words at 0x08B01564 (then its index, u16 at + 2496); word i = 1812433253 × (last ^ last >> 30) + i; GAME + 64 = the last word's low half |
| `object/director.c` | the record at 0x08B0274C that directs a sequence of screens: handlers by its + 2 (0x08A2C5A4), + 8 (0x08A2C5BC, C5E4, C644, C768) and + 16 (0x08A2C738), and by GAME + 4 (0x08A2C23C) and the high bytes of GAME + 16 (0x08A2C59C) and + 22 (0x08A2C664, C670); scroll records 0x08B027E0 and 0x08B02810 |
| 0x08872C50 | damage by the other fighter's ISM: a fifth less for ISM 1, a fifth more for the rest (not ISM 0, not for hits with byte 19 = 2; 127 at most), one less while + 308, never under 1 |
| 0x088732E0 | a value scaled by + 644: ×2 for 1, + ¼ for 2, + ½ for 3 |
| 0x088721B8 | which way the one hit falls (+ 134): the attacker's facing, away from the attacker (hit byte 11 bit 7), by the screen's half (2), or turned by the byte |
| 0x08872368 / 0x088724C0 | + 575 = 3 / 2 / 1 / 0 by + 637, + 73 bit 7, 0x088723D4; a hit by an object (+ 8): + 661, + 537, BATTLE + 28 - + 30 |
| 0x08862D88 | a fighter's ISM data: seven words from the character table's + 20 list by ISM, into + 128 (the pointer), + 208 - + 224, + 200, + 300 |
| 0x08812FD4 | the character number for display: 4 → 0x12 and 6 → 0x14 with a negative ISM; 0x15 with + 373 while BATTLE + 59 = 2 |
| 0x08879D1C | a script's block skipped: words passed over (0x7600 opens a nested block, 0x7602 closes one) to the close that ends this one |
| 0x08859AD8 | + 555 walked through its row of the table 0x08A1FD5C as + 557 counts down |
| 0x08823338 | an object made in the third list (kind + 91, type + 92) |
| 0x08822C84 / 0x08822AEC | the three layers cleared; a fighter's steps and work fields reset |
| 0x08836A3C | the s16 at 0x09524A68 grows on a timer: + 8 every 1920 frames, or + 2 every 640 while GAME + 137 |
| 0x08819020 | the save question's cursor (0x09523C69) shown by an object |
| 0x088550E8 / 0x0885515C | intro objects: settles at y 128 (intro stage 4); bounces by the table 0x08A16D88 |

**The CPU's scripts and the versus screen's objects** (0.69.0).

| Address | What |
|---|---|
| 0x0887B1B8 / 0x0887C25C | a script picked for the CPU from its character's lists; the pick at random |
| 0x0887A350 | the CPU's script restarted from where it was kept (+ 512, first word + 504); a command whose high byte is -1 picks a new script first |
| 0x088771DC / 0x0887B48C / 0x0887B648 | the CPU acts; its wait set; a script word unpacked |
| 0x08822AA8 | every fighter's steps reset |
| 0x088D55D0 | the network record (0x09523980) cleared; 0x088D5ECC, 0x088D7C4C, 0x088D7EA0, 0x088DD81C, 0x088DC080 are its small steps |
| 0x088AA6B0 / 0x088AA890 / 0x088AA950 | three more of the director's routines |
| 0x08899AC8 / 0x0889A818 / 0x0889B0A4 / 0x0889BFA4 | an object's substep handler run with its fighter, only for a side marked in GAME + 41 or + 57 (tables 0x08A2A8DC, 0x08A2A9A4, 0x08A2AA08, 0x08A2AAE4) |
| 0x08897D78 / 0x08898308 / 0x0889E5FC | arrives in y / x (at once while BATTLE + 81) and stops on its target (+ 114); the y one sets bit 2 of GAME + 20, the last its bit by type |
| 0x088993C8 | two speeds until + 32 reaches 0, then its bit (by + 10) in GAME + 22 |
| 0x0889A754 | lit (+ 12 = 0) and nudged 4 towards the middle while GAME + 20's low byte names it |
| 0x0889C6A0 / 0x0889BEC4 | a row: y from 0x08A2AB4C, animation 12 or 14 by side; animation 4 or 5 by side, or 8 |
| 0x0889EF1C | handler by substep (0x08A2B164); objects with + 10 = 2 scroll down, round from 96 to 0 |
| 0x0888D71C | 54 parts of 0x40 bytes slide while + 4 counts down: those in use up, the rest down |
| 0x088958C0 | + 22 counted down, then the second record shows fighter 1's or 2's character (bit 0 of + 57) |
| `object/screen_steps.c` | 0x0887E094, 0x08885950 (a part animated), 0x08886EE4 (a counter started), 0x08887BBC, 0x08889E80, 0x08889E00, 0x088BBF48 (a marker follows), 0x088B2C8C (the third list fades out); `intro_steps.c` 0x088551D0 waits, then intro stage 5 |

**The pools emptied and more of the screens' objects** (0.70.0).

| Address | What |
|---|---|
| 0x088222C4 / 0x08822934 / 0x088229CC / 0x08822444 / 0x0882222C | the five pools of 0xC0-byte objects emptied (`pool.c`): 8 at 0x09512170 (list 3), 32 at 0x09512770 (4), 16 at 0x09513F70 (7), 16 at 0x09514B70 (5), 42 at 0x09515770 (6); free counts 0x095176F2 - F6 |
| 0x0880A028 | the queues beside the ring (0x092DA0F0: three rows of eight words, 0x8C bytes at 0x092DA1B0) emptied, and five 0x800-byte buffers from 0x092DA240 |
| 0x088156FC | the current 216-byte record (number at 0x08E76084, records from 0x08E76088) partly cleared and 0x08808424 run with 5 |
| 0x088DF288 | a record's eight-character name (+ 0xB0), or the default at 0x08A3EB78 |
| 0x08877BC4 / 0x08879C8C | the CPU's wait set, then its script read on (or 0x08877F44 with + 506); a block left at its close or its 0x7601 |
| 0x08877148 / 0x0887BAD8 | two questions of a fighter's frame (characters 4, 7 and 28 above + 132; the opponent in routine 1.1.1.1) |
| 0x088A0248 / 0x088A0374 | follow a record while it is shown: 24 above it, or 44 to the side by the owner's side |
| 0x088AD5A8 / 0x088B2754 | thirteen frames of 12 a frame (x, or + 6 by type 4 / 5), answering 1 on the last |
| 0x088B18FC / 0x088B1978 | animations picked at random (tables 0x08A30248 / 0x08A30140; 0x08A30288 / the list at 0x08A2FFFC) |
| 0x088BB8A0 / 0x088BB9BC | scroll round 224 while sliding in and out by GAME + 18 |
| 0x088BBB14 / 0x088BBD54 / 0x088BBBEC | a row of kind 0x71 objects, each making the next when its count (+ 82) runs out |
| 0x0889A1BC / 0x0889FB04 | scroll in y / x round the length + 76 (high byte) and + 77 |
| 0x08899DF0 | the fighter's character shown: picture 0x127 + its number from 0x08ACCAE6 into the animation at 0x08A287FE |
| 0x0887D9D0 / 0x0887D684 | jittered by sixteen s16 at 0x08A242F4; and a second object made beside it |
| 0x08886160 / 0x08886314 | six parts moved by the table 0x08A25818; four parts animated, then step 3 |
| `object/screen_steps.c` | 0x0889A600, 0x0889A6C4, 0x088AD328, 0x088AD3B8, 0x088BB728, 0x08899590, 0x0889961C (table 0x08A2A8B8), 0x0889BD08 (reports bit 2 in GAME + 18), 0x0889F810, 0x088A0104; `intro_steps.c` 0x08854FEC, 0x088563BC, 0x08857A3C, 0x08855AA4 |

**The CPU's script branches and a fighter kept on the screen** (0.71.0).

| Address | What |
|---|---|
| 0x08878B30 / 0x08878C24 / 0x08878CD8 / 0x08878EEC / 0x08878A68 | script branches: into the block (0x0887A10C) or past it (0x08879C8C) one time in two, by the opponent's + 73 = 1, by its + 698, by 0x08876BE4, or once + 492 (set from the script) reaches + 508 |
| 0x08879E10 / 0x0887A188 | a script called at random (this one kept in + 504 / + 512 to come back to); a random script started (+ 528, + 488), answering the word after |
| 0x08877A38 | the CPU's first wait (0x08877BC4), then its wait set and the script read on, or + 483 = 8 and 0x08877F44 with + 506 |
| 0x0887BD60 | a script command allowed: 3 always; 4 while + 536 and + 228 are clear; 5 for ISM other than 1 while + 366 is at least 48 (ISM 0) or 144 |
| 0x08876A0C / 0x08876AA8 | 1 once (+ 498) when nothing halts the fight and the record at + 296 is shown; the question numbered by + 492 / 2 (table 0x08A22178) |
| 0x08862DF8 | the fighter kept on the screen: past the left (+ 238 = 1) or right (2) edge by its width (+ 232, 24 more with 0x08A0588A) it is pushed back and 0x08862F28 told how far |
| 0x08863168 | speeds + 96 - + 108 from the 16-byte records at 0x08A182C0, picked through 0x08A180C8 (by + 655, + 575) and 0x08A18218 (by the argument); x ones turned round unless + 134 |
| 0x088AC3A8 / 0x088AC3EC | the director empties pools 4 and 6 and the object list, or pool 5, and clears its + 78 - + 80 and + 92 |
| 0x08814B4C / 0x088DEB10 | 0x08808424 run with 6 and 7; the 0xC4-byte record at 0x09523740 reset with the default name |
| `object/screen_steps.c` | arriving in x then scrolling (0x0889A0C4, 0x0889A3A8, 0x0889B9E0, 0x0889A154, 0x0889A438, 0x0889BA70) or showing the character (0x08899CCC, 0x08899D78); the next part on a bit of the fighter's + 78 (0x08899D28, 0x0889A110, 0x0889A3F4, 0x0889BA2C, 0x0889A670); 0x0887D63C; 0x088AD1D4, 0x088B270C; 0x08889CC8 (by type from 0x08A26950) |
| `intro_steps.c` | 0x08855B3C (two parts crossing from x -384 and 384), 0x08857B14 |

**The script step, a fighter's move state and objects on the screen** (0.72.0).

| Address | What |
|---|---|
| 0x088774A8 | the CPU's next script command read (a new script for -1) and run by 0x08877564; where it left off in + 528 and, by the nesting (+ 484, 0 - 3), in + 512 - + 524 |
| 0x088647C4 | a fighter's move state cleared: speeds + 96 - + 108, many flags, + 574 of all four fighters, the opponent's + 586 / + 587 |
| 0x08863A9C | the button just pressed (+ 760 bits 4 - 9) as strength 0 - 2 and punch 0 / kick 1 |
| 0x0886BCB8 | an entry of a fighter's table of values (0x08A1FFEE with + 792's bits, else the character's from 0x08A2002C, or 0x08A20006): row by + 192 (0, 6 or 12), 3 more with + 193, column + 194 |
| 0x08821424 | an object drawn only while its x on the screen (64 to the left counted) is within 0 - 511: fixed with + 14 = 0, else by its camera (kept at 0x08B01F28), moved by the camera's + 28 × + 38 for + 14 = 2; out of it + 1 = 0 |
| 0x088081D4 | the sixteen 216-byte records from 0x08E76088 emptied, and the 0x28 bytes at 0x08E87908 |
| 0x088A9AD8 / 0x088A9D58 / 0x088AB948 | the director makes a row of objects from 0x08A2C5CC (if pool 4 has room); one of kind 0x31 every eighth frame; two of kind 7 in the third list, then its handler (0x08A2C5F4 / 0x08A2C748) |
| 0x088A9E24 / 0x088ABA4C | the s16 at 0x0951CE52 taken into GAME + 22; the word at 0x08B027F8 moved by its speed, kept above 0x2000000 |
| 0x0889C2EC / 0x0889C9FC / 0x0889CEE4 / 0x0889EE08 | scrolls while its fighter's character is shown; a row chosen (four buttons a side at 0x08A2882E); a motion entry by mode (0x08A2A3A0 / A5F8 / A148); eight entries of 0x08A2824E shown from BATTLE + 72 |
| 0x0888D5B0 | the record at 0x092E51C0 and its 54 parts set up (the ones 0x0888D71C slides) |
| `object/screen_steps.c` | 0x08889D64 (by type from 0x08A26980), 0x088B1374, 0x088B1BB4, 0x088B1D10, 0x088B2104 / 2184 / 232C / 23B4 (a record placed with a size) |
| `intro_steps.c` | 0x08855F44, 0x088561C0 (coming in by BATTLE + 63), 0x08857464 (three numbers' pictures from GAME + 20) |

**The CPU's first frame and more objects** (0.73.0).

| Address | What |
|---|---|
| 0x08877328 / 0x08877388 | the CPU's first frame: a script picked once (+ 480; by level, or at random), its first command run once from + 512 / + 516 (+ 481), then 0x0887A400 |
| 0x08863A50 | a human fighter's button just pressed kept in + 194 / + 193, its held directions in + 290 (the pair read into the guest stack) |
| 0x08879E58 / 0x088780A0 | a script called at random when 0x08877258 (list 0x08A22B20) says yes; within 2 of + 508 the script read on, else routine 1.0.2.0 with + 497 / + 78 saying whether + 490 reached + 508 |
| 0x088073E8 | the records' start-up: 0x088081B0, the reset (0x088081D4), 0x0880829C |
| 0x08881EB4 | a thrown object: falls by + 100 (growing by + 108) and swings about x + 96 by + 104 (growing by 0xC000) to alternate sides; the next step once + 75 is negative |
| 0x0888639C | frames for a record from the table 0x08A25808 while it is shown; a negative one puts it out and sets + 129 of the object at + 20 |
| 0x088894B8 | a record placed at the object at + 124 by the next entry of the list at + 120 (sizes from 0x08A262E0); -1 ends it |
| 0x0887E6EC | a jittered copy (kind 5) made beside an object (sixteen s8 at 0x08A2445A) |
| 0x08897B80 / 0x0889AB98 | arrives (y or x by + 10 bit 1) then y 120, reporting bits 0 - 1 in GAME + 20; set up by side unless the fighter's + 424 bit 1 |
| `object/screen_steps.c` | 0x0887DD04, 0x0889C1E0 / 0x0889C23C / 0x0889C290 (then 0x0889C2EC), 0x088B1314 |
| `intro_steps.c` | 0x0885647C (past x 128), 0x088564C8 (into 88 - 168), 0x08857620, 0x088573BC |

**The 2D library, the save names, the network's start and more objects** (0.74.0).

| Address | What |
|---|---|
| 0x089E0CF4 / 0x089E0EA4 | a texture list made the one in use (copied to 0x08B23334) and its descriptor 0 picked; descriptor n picked into 0x08B2333C (its number u8 at 0x08ACCD80) unless it already is (`syslib/draw2d.c`) |
| 0x089E1204 / 0x089E0778 / 0x089E1438 | a 12-byte descriptor set (+0 itself unless flag 0x40000000 gives a word, +4 flags, +8 set with 0x800000); its value: +8 itself with 0x800000 (or 0x40000000 for 0x089E0778), otherwise the word +8 points at; the same for the picked one |
| 0x089E1254 | count palette entries copied to the word at 0x09525E08 + 2 × start (2 bytes an entry for the kinds 0 - 2 at 0x08B2334C, 4 for 6, else none) |
| 0x089E055C / 0x089E0DE0 | a code's number from 17 pairs at 0x08ACCD88 (bits 4 - 19, low four kept); a 24-byte record set, +8 / +12 by the low byte of its third argument |
| 0x089E04EC | the game's zeroed allocation: (size & ~63) + 64 bytes, aligned to 64 (`libc/malloc.c`) |
| 0x089E1D3C / 0x089E26D0 / 0x089E272C | a record taken from 0x08ACCE10 (8 × 20), 0x08AD3AF8 (16 × 20) or 0x08AD3C38 (8 × 24); 0x07000101 when all are in use. 0x089E2340 / 23A0 / 25E8 / 2644 set their values (`syslib/tables.c`) |
| 0x089E3158 / 0x089E31E0 / 0x089E3248 | which of STFALPH3.SYS / .WTR / .NET / .DWN (0x08AD3D8C) a name is; a system dialogue's 0x600-byte parameters (size, language 1, button swap 1, thread priorities 0x11 / 0x13 / 0x12 / 0x10); save parameters for file n: "ULUS10062", "0000" - "0003" (0x08AD3D3C), the file's name |
| 0x088DBF88 | the network from scratch: the 64 KB block 0x09316A00 (0x08B02EA0), "heap_work_sema" (0x08B02EA4), 0x088E0A58 (the thread record 0x095236C0 and "network_sema" at 0x08A3EB88), the session 0x09523920 and step 0x09523960 cleared, 0x09523820 (0xF4 bytes), the text, 0x09523C20 |
| 0x088E3660 | the button-config remap: while the session is up (+ 48) and has not failed, the buttons go into the session's first word and 0 is answered |
| 0x088E358C / 0x088E3604 / 0x088E36B8 / 0x088E1ABC | the session has no error and has not halted (+ 16, + 49); is down (+ 48, + 37); + 38 counts 0 - 3 while up; the network thread woken (0x095236C0 + 4) while + 16 |
| 0x0886DB2C / 0x088639CC | a human fighter with + 80 and not + 262 starts routine 1 / 5 on an attack button while the opponent has health: facing + 697, 0x0886DC3C, the button kept, + 192 / + 696 whether down is held, 0x0885B7D4 |
| 0x08879DBC | a script called at random, or maybe called (0x08879E58) when + 488 has a command |
| 0x0892EA9C / 0x0892B188 / 0x0892EFB8 | x (+ 16) moved by + 96 (backwards while + 11 is 0), + 96 growing by + 104; a blink at + 828 after 23 frames, then every 15; an object copied from a fighter |
| 0x088AD538 / 0x088ADC44 / 0x088AF2D0 / 0x088B01A0 | the same routine four times: drawn on the screen while GAME + 16's high byte is 0, + 4 on while its low byte is not + 32; 0x088AFC90 counts, sinks and draws with it |
| `object/screen_steps.c` | 0x0887DAFC / 0x0887DC54, 0x0887E230 / 54, 0x08889440, 0x088F1718 (0x08A43C50 / 0x08A43C60 by + 2), 0x088BB510 / 61C / 820 / 93C, 0x088B243C / 24C0 / 2B98, 0x088EB1A4, 0x0889AE58, 0x0889B544 |

**A script restarted, the queues' records and more objects** (0.75.0).

| Address | What |
|---|---|
| 0x0887A1D8 | a CPU fighter's script from the start: picked (+ 484 = 0) or the one left off at + 504 / + 512; its first word into + 488; a command of -1 runs at once (0x0887A154). Answers the next place |
| 0x08873094 | v scaled by GAME + 116 (the record's + 19 = 2) or GAME + 46: 0 takes an eighth off, 2 adds an eighth, 3 a quarter |
| 0x0885E9A4 | the chance by level (list 0x08A22CA0) for a CPU fighter of character 6, 15 or 20, when nothing stops it (+ 702, + 568, health, ISM -1, BATTLE + 13 / 14, GAME + 38, both in step 6) |
| 0x088638A8 / 0x088638E0 | a jittered copy (+ 78 = 17 / 16) moved by the character's offset (0x08864D98) |
| 0x08809EF0 / 0x08809F48 | the record in word n of 0x092DA0F0 (from table B, 0x089E26D0) or 0x092DA130 (table C), taken the first time |
| 0x088AA6F4 | the director by + 93: GAME + 10 cleared to its high byte (negative; + 2 steps on and places scroll A), 2 (for 1) or + 93 - 2; GAME + 16's low byte 0x17 sets GAME + 20 bit 8 and scroll B |
| 0x088BBA24 / 0x088BBC54 | a line of text (18 bytes a line from 0x08A05834 + 16, the second + 0x48) in the stage's colour (0x08A381B0 / D0 by BATTLE + 102) after the step from 0x08A38198 / B8; freed while GAME + 39 or no GAME + 18 |
| 0x088BBE98 | the stage's colour (u16 at 0x08A381E0, five bits a channel) into a record's colour bytes |
| 0x088F1860 / 0x0889BC30 | an object on its fighter's picture (frame data + 10; animations 0x08A3F9E0); a substep (0x08A2AAC4) then + 96 kept within (+ 76 << 8 \| + 77) << 16 |
| 0x088957EC | six objects of kind 0x71 once pool 4 has 6 free (0x095176F3), three given the character of the fighter + 57 names |
| `object/screen_steps.c` | 0x088AFC50, 0x088B1260 (0x08A301F8), 0x088B2878, 0x088AF91C / 0x088AF658 (by the byte 0x09525DE3), 0x088AF1C8, 0x088B19F0 (switch 0x089FF778), 0x0889B71C, 0x088B59A0, 0x0889592C, 0x08897E2C |

**The fight frame's end, damage by values and more objects** (0.76.0).

| Address | What |
|---|---|
| 0x0882453C | a fight frame's last part (`task/fight.c`): unless GAME + 40 or the pause, GAME + 4 on (+ 6 / 8 / 10 cleared), BATTLE + 81 cleared, GAME + 59 set for a continuing match (+ 32 / 33 cleared) or counted, BATTLE + 52 = 1, + 23 cleared, 0x08824624 both ways; then 0x0881DF10, the second object list unless paused, 0x0880B6CC |
| 0x08872F68 | (attacker, defender, record, v): v changed by (attacker + 796 or + 798 by the record's + 19, 100 with + 792 bit 25) + + 802 - the defender's + 800 (75 with its bit 25) and + 802, each from 100, in 200ths; at least 1 |
| 0x08872080 | a gauge at + 700 filled by the hit's + 12 (+ 25 % with + 588; 20 / 30 by the hit record fighter's + 810); at + 701: + 702 set, + 700 / + 699 cleared, + 701 up by 5 to 160 (not for + 810 = 8) |
| 0x0887335C | + 126 counts to 99, the highest kept in + 357 of 0x09358410 + 24's fighter (layout 0) and of its pair |
| 0x088592E4 | 0x088593D8 for the fighters in play by layout |
| 0x08822600 / 0x0882235C | the three queues emptied and the flags at 0x092E2690 / 23C0 / 2AC0 / 29D0; the three small objects at 0x092E3E08 reset (copied from the first) and the text queue emptied |
| 0x088214E8 | 0x08821424 with a margin argument: drawn while 0 <= x + margin < 2 × margin + 0x180 |
| 0x08863850 | with + 117 = 5, a jittered copy (16 or 17 at random) |
| `object/screen_steps.c` | 0x088B2800, 0x088AE99C, 0x08889BA8, 0x088AF758 / 0x088AF574 / 0x088AF838, 0x088EB1E8, 0x088ACE00 (0x08A2CB60 by + 3), 0x0889C738 |
| `intro_steps.c` | 0x08857E98 (two parts built at + 0x78) |

**A hit's damage, speeds and script picks** (0.77.0).

| Address | What |
|---|---|
| 0x08872B54 | (attacker, defender, hit, v): v or the attacker's + 260, through 0x08872C50 and 0x08872CD8; bits 0 - 6 index the s8 table 0x08A1C0C8 (32 by CPU level, 0x09358410 + 24's + 375); ¾ for a hit + 19 of 2; at least 1; + ½ with that fighter's + 309; 0x088732E0; under 128 (`fighter/hit.c`) |
| 0x08862068 | speeds from the character's 16 s16 at + 708: rows 0 / 8 / 12 by + 497 (CPU) or the held direction; + 96 / + 100 / + 104 / + 108 (<< 8; + 104 unshifted for ISM -1 characters 17 and 19); turned for facing left |
| 0x088597F0 | the character's routine (0x08A19040), then { index, value } lists (routine table + 116 / 112 / 120) into 0x08A10040 / 0x08A10130 / 0x08A0FDC8 |
| 0x0887B308 | a CPU script by the opponent's + 307: lists at routine table + 164 (network battle with + 815) or + 56 (+ 499 added to the random pick); + 511, + 486, + 520, answers + 520 |
| 0x0886E3CC | (fighter, kind, length): 0x08805310 (empty) with the pad slot + 775 and the pair at 0x08A1BFD8 + 2 × kind, for a human (Training: the side GAME + 57 names) |
| 0x08899FD0 / 0x0889C0D4 | rows of 6-byte entries laid out by a character's width (0x08A27FFC by 0x08A2A914; 0x08A280B0 by 0x08A2AB0C) |
| `object/screen_steps.c` | 0x0889B15C, 0x0889C3D8, 0x08881FB8 (kind 15 beside, jittered by 0x08A24C38), 0x08818F1C (tables 0x08A078D0 - 0x08A07900) |

**The random numbers' twist and more** (0.78.0).

| Address | What |
|---|---|
| 0x08820A68 | the Mersenne Twister's twist over the 624 words at 0x08B01564 (mag01 at 0x08A0E06C); run by 0x08820BE0 when its place (+ 2496) is 0 |
| 0x0883021C | (n): the s8 at 0x08A0F3A8 + n; n = 41 outside a continuing match picks one of 41 at random (32 tries; not 9, 10, 23, 30, 31, BATTLE + 98 or GAME + 192's) (`task/fight.c`) |
| 0x0881E538 | a camera layer's y (+ 38) stepping by 1 towards the height 0x0881EA10 gives (on the guest stack) less 0x60, within + 66 - + 64, plus + 86; + 58 = 4 under + 46; only + 86 added while 0x08B01560 (`stage/camera.c`) |
| 0x0887B6B8 | 1 when a CPU fighter may take a chance (0x08A228A0): see `fighter/state.c` |
| 0x0889682C | nineteen kind-0x10 objects from 0x08A27328 once pool 4 has 19 free; one more of type 4 in Final Battle |
| 0x088AD60C / 0x088892C4 / 0x088ADB28 | a record bobbing by + 10 (1 on the 14th frame); flashing / freeing / fading by + 92; y wrapped within (+ 76 << 8 \| + 77), hidden for GAME + 22's low byte matching + 78 |
| `object/screen_steps.c` | 0x08897C64, 0x0889A4E8 |

**The score, the held history and more** (0.79.0).

| Address | What |
|---|---|
| 0x088211CC | (n, fighter): + 352 += 0x08A0DEC0[n] for a human, capped at 9,999,999; the partner's (+ 1168) too in the team modes (15; 7 with BATTLE + 79 = 5; 16 with BATTLE + 159 = 3); World Tour's + 920 in mode 3; not while BATTLE + 61 or World Tour + 899 (`task/fight.c`) |
| 0x08858D54 | + 754 / 756 / 758 history of + 752, masked to 0x800F by a pad setting (byte 18 of 0x08A058A0 + 20 × slot; 0x09523438 + 208 × slot in a continuing match; 0x09525D9A in Training); + 766 = + 764 |
| 0x0887B528 | a CPU state from a script word: + 497 bit 0, + 290 bits 0 - 3; bit 11 → 0x0887B648; else + 688 / + 192 / + 193 / + 194 |
| 0x0886E638 | (out, base, table, n): addresses from a table of offsets (+ 8) and counts (+ 264): one, a -1-ended list (× 10), or a count of entries (+ 12) |
| 0x08822808 / 0x088226E0 | the two pools of twelve 0x168-byte objects (0x0950FFB0 / 0x09511090; counts 0x095176F0 / F1) reset; the fight's word tables cleared (0x092E2A00 only when the argument is 0) |
| `object/screen_steps.c` | 0x08899EB0 / 0x0889A288 (falling set-ups), 0x08899258 (by GAME + 41) |

**Hold frames on a hit, button attacks and more** (0.80.0).

| Address | What |
|---|---|
| 0x0887223C | (attacker, defender, hit): + 127 of each from the pair at 0x08A21EC4 by the hit's + 16 (second half for the defender's + 117 bit 7); at least 16 with the defender's + 590; halved with + 637; + 16 (max 30) while 0x09358410 + 46 (`fighter/hit.c`) |
| 0x0886BE50 | a human's attack on + 760 bits 4 / 7: facing, + 193 kick, + 192 down, 0x0886DC3C, the opponent's + 592 / + 270, ISM -1 + 194 = 2; + 551 = 1 and answers 1 |
| 0x08878798 | a script's routine 1.0.5.0: + 482 / 483 / 498 / 235, facing, then + 490 from the script and 0x0887B528 |
| 0x0885847C | score 0x08A17270[record + 6] added as 0x088211CC does (also not while BATTLE + 16) (`task/fight.c`) |
| 0x088644F4 | + 771 by the characters met (lists 0x08A215A4, values 0x08A195D0 / E1); 1 for BATTLE + 79 of 7 or 10 |
| 0x088AB814 | the director's start: pool 5 emptied, scrolls reset, the layer's + 122 = 6, kind-0x7F objects (types 2, 1, 0, 4 × 9, 6) with 14 free in pool 7 (`director.c`) |
| `object/screen_steps.c` | 0x0889F5F8 (round pictures from 0x08A2B1F4), 0x0888A224 (lit by mode, 0x08A269B0), 0x0889E46C (nine by width, 0x08A2B0B0) |

**Sides down, starting places and more** (0.81.0).

| Address | What |
|---|---|
| 0x088262C0 | (game, battle): bit 0 second side down, bit 1 first, by layout (pairs 3 / 4 by fighter 4's + 810 = 4; layout 3 by the game's + 137) (`task/fight.c`) |
| 0x08864BC8 | a fighter's start: facing by + 121, turned in layouts 2 / 3 (GAME + 57 bit 0, network), x 0x280 less 0x60 / 0x4A / 0x40 towards its facing, y 0x28 |
| 0x0882FC2C | (game, battle, fighter, n): BATTLE + 98 / + 100 / + 17: picks at random (0x0883021C) by mode or a continuing match; rounds 8 / 9 and Final Battle from 0x08A0F341 (`task/fight.c`) |
| 0x08821080 | team score: a CPU fighter's too with a human partner (`task/fight.c`) |
| 0x08867A60 | (fighter, v, mask): an entry of the character's tables 0x08A1B3F8 / 0x08A1B4F4 by its slots + 1150 naming v; its first u16 |
| 0x0888D924 | the gauges (+ 556) placed at + 0x6FA / + 0xD3A; kind-0x1F markers at 17 or more while + 590 (flags + 27 / + 47) |
| 0x0889DE64 | an object travelling from BATTLE + 98's place to the stage's (0x08A2AFB0), speeds by a divide by 0x21000 |
| `object/screen_steps.c` | 0x0889BAE8 (0x08A2AAA4 by side), 0x0889A8C0, 0x08899B6C |

**Sprite parts, the camera's x and more** (0.82.0).

| Address | What |
|---|---|
| 0x0880D214 | (draw state, sprite, size, offset): a part's offset scaled about its centre by the sprite's + 32 / + 34 (8.8) and turned by + 36's high byte (2 × 2 matrices at 0x08A0432C), added to the draw state's + 80 / + 82 (`object/sprite.c`; ~50,000 calls in the first fight) |
| 0x0881E320 | the camera's x (+ 34) following the fighters' span (two s16 on the guest stack from 0x0881E64C): centred for a span of 192 or more, else kept within + 0x60 / + 0x120; + 82 added; + 59 = 8 / 0xC by + 42 (`stage/camera.c`) |
| 0x08873160 | damage less by the hits in a row (+ 126 into the s8 at 0x08A21F5E: ½, ¼, ⅛, 1/16, 1/32 off); Training's flags at 0x0950FF70 (`fighter/hit.c`) |
| 0x08874CDC | a hit's kinds and lengths for 0x0886E3CC (the length table 0x08A1FF60) (`fighter/hit.c`) |
| 0x088585B0 | a count's reward: a kind-3 object by + 142 (0x08A172B0) and score from 0x08A172C0 / E0 (+ 1,000 a count past 15) (`intro_steps.c`) |
| 0x08836AA8 | the s16 at 0x09524A68 limited: mode 18 at random from 0x08A0FAC8; World Tour 0xF8; else 0x08A0FA48 by GAME + 73 and BATTLE + 20 (`misc.c`) |
| 0x08895DD8 | fourteen kind-0x13 objects from 0x08A272C4, their fighters from a table on the guest stack |

**Small steps from the wider profile** (0.84.0). From 0.84.0 the functions come from a profile of ten scenarios (see the decomp log), not only the first fight.

| Address | What |
|---|---|
| `object/small_steps.c` | 25 small object and record steps: 0x08881F70, 0x088B098C / 0x088B48F0 (swaps), 0x08987D68, 0x089993E4, 0x089DFCA4, 0x088301F4 (0x08A0F396 by + 55), 0x08856D30 / 0x088815A4 / 0x088B51D8 (+ 82 counts), 0x08856E18, 0x088AA23C, 0x088AC334 / 0x088B366C (GAME + 4 unless + 93), 0x088AC37C, 0x088BDA60 / 8C, 0x089CD7D4, 0x089DEC5C (fade in), 0x088BE36C (0x08A3B01C at random), 0x088CEDA0 / 0x088CFC70, 0x08912FBC (moves by ~speed) / 0x08987474, 0x08858D04 (0x08A17E58) |
| 0x08822BD8 / 0x08838688 / 0x088392E4 / 0x088B2EFC | GAME's 0xD8 bytes, World Tour's 0x3B8 (0x09517A78), a 0x354-byte record, 0x08B0284C's 16 and the layer's + 123 cleared (`reset.c`) |
| 0x0881E84C / 0x0881E870 | a fighter's edges (x ± + 232) into two s16 (`stage/camera.c`) |
| 0x0887192C / 0x08877C30 / 0x0887BB6C / 0x08891FE8 / 0x088344B4 | + 117 by + 588; + 506 then routine 1; nesting 3 and 0x088772C0; + 340 on; + 341 on and + 412 = 10 |
| 0x088AA8E8 / 0x088E3630 / 0x08823220 / 0x08818EBC | the director's next pair; the session's + 33 from 0x08A3EB96; list 5 freed and counted; x by the save question's cursor |

**More small steps** (0.87.0), all in `object/small_steps.c`.

| Address | What |
|---|---|
| 0x088A9F54 / 0x088AAA28 / 0x088AB438 / 0x088AC020 | the director's (0x08B0274C) step from 0x08A2C608 by + 8, 0x08A2C69C by + 2 (u16), 0x08A2C720 by + 16, 0x08A2C780 by + 8 |
| 0x08812F90 / 0x088B2E2C | the step 0x08A05864[BATTLE + 56]; the step 0x08A36EF0[GAME + 4] |
| 0x089CD248 / 0x089CF574 / 0x089D9994 / 0x089DFA0C | an object's step by + 4 from 0x08AB6148 / 0x08AB6348 / 0x08AB69F0, given 0x09358BE0; by bits 8-11 of + 66 from 0x08AB79F0 |
| 0x088212F4 | a number in binary-coded decimal (a digit to each 4 bits) |
| 0x088230B4 | the first free of 8 slots of 20 bytes at 0x095177C8, marked used |
| 0x08830348 | bit k set in a record's + 400 (k < 32) or + 404 |
| 0x089DF1E0 / 0x089E04B0 | the pad word 0x08E75BA0 + 12 (+ 53 is 1) or + 56 has a bit of 0x83F0 / 0x8000 |
| 0x088BE334 | one of 36 s8 at 0x08A3AFF8 at random |
| 0x08984604 / 0x088748FC / 0x08837058 | a fighter's + 833 / + 236 when BATTLE is 0x1C; BATTLE + 28..30 once (+ 660); 1 added by 0x08836E60 unless GAME + 57 is 3 or + 373 |
| 0x08826844 / 0x08855550 / 0x088569B0 / 0x088A27CC / 0x089DF0BC | count-downs (+ 20, + 82, + 74, + 82, + 16) that move a step on |
| 0x088AD250 / 0x088B38C0 / 0x08856E44 | + 5 on after 0x088AD60C; + 3 on when GAME + 10 has bits 5-10; + 4 on once x passes 0x1C00000 |
| 0x08987404 / 0x08987440 / 0x0890A19C / 0x089CEF38 / 0x089D7808 | speeds: x and y moved, y then 0x08987474, a jump's four speeds set, four speeds times 256 |
| 0x088BCD70 / 0x08913098 / 0x088B4578 / 0x089DFD04 / 0x0892EB34 / 0x088AE200 | places and fields from an owner or the object itself; right of the camera + 0xC0; y moved 8 toward the middle |
| 0x08987D8C / 0x089880E4 / 0x089CEF04 | + 84 from 0x08A92E30 (below 345) / 0x08A930E2 (below 346); an animation from the list at 0x09358BE0 + 280 |
| 0x08817C94 / 0x088B3D94 / 0x089DE61C / 0x088CEE80 | pools reset and + 2 or 0x08B0284C + 8..10 cleared; three steps in a row; + 1 by GAME + 4 / + 6 |

**More small steps** (0.88.0), all in `object/small_steps.c`.

| Address | What |
|---|---|
| 0x088B0248 / 0x088B3DDC | 0x08B02848 / 0x08B02E08 = 1, then the step 0x08A2F21C / 0x08A3731C by + 3 |
| 0x088BDDA8 / 0x089DF82C | the step 0x08A3AFE8[GAME + 4] given (GAME, BATTLE); 0x08AB79CC[GAME + 6] given (GAME, object) |
| 0x089879F0 / 0x08987DC8 / 0x0888AAFC / 0x0883A388 | steps by + 4 from 0x08A9477C / 0x08A947C4 (+ 38 from the owner first); by + 92 from 0x08A26B3C; by + 6 from 0x08A12230, then 0x08887AFC and the frame's end |
| 0x0880A7B0 / 0x0880A7F4 | text n of 0x08A03CC0 (kind 5) / 0x08A03CD4 (kind 6) at (x, y), by 0x0880A6CC |
| 0x08857678 | BATTLE + 112, or the second fighter in layout 3 when GAME + 57's bit 0 is clear |
| 0x0886D29C | the opponent (+ 92), unless the computer, scores 0x08A1BE04[+ 126] (99 at most) through 0x08821080 |
| 0x0887763C / 0x0887D900 / 0x08834318 / 0x08827A94 / 0x08876998 | a fighter: script read (+ 696) or routine 1 again; + 9 = 2 and its animation by type; + 340 on and the side's bit out of + 42; + 452 = + 352 (and + 478 / + 451 in mode 0x12); + 208..+ 224 from + 128 |
| 0x088AC2F0 / 0x088B3628 / 0x088B3BC4 | a word (+ 84 / + 12) counts down, then the fade out and + 9 / + 3 on |
| 0x08855420 / 0x08855508 / 0x08855934 / 0x08856960 / 0x08856D58 / 0x0890CC04 / 0x0899939C | objects moving until a mark (0x700000, -0x680000, 0xF00000, y in 0x740000..0x8C0000) or by their facing |
| 0x088B0434 / 0x088B0608 / 0x088B1578 / 0x088B2838 / 0x088B2930 / 0x088B2978 / 0x088B4E74 / 0x088ABACC | objects drawn or moved on by GAME + 10 / + 16 / + 22 |
| 0x088BD2F0 / 0x088AB1D8 / 0x088A1750 / 0x08892A58 | k's place in 0x08A3AF8C's zero-ended bytes; 0x08B027B0 / 0x08B02810 from 0x08A2C6F8; 0x09517A50 / 7A28 by GAME + 53; 0x08A058A0 + 60 / 62 / 64 |
| 0x08821A64 / 0x088EAC84 / 0x08826A14 / 0x088B4878 / 0x0896A4D4 / 0x088BD0C8 | fields from a record; + 6 on and cleared; + 5 on and the swap 0x088B48F0; animate and draw; + 4 = 2 by the camera's y |
| 0x08855268 / 0x08856908 / 0x08881544 / 0x088B3348 / 0x088B376C / 0x088BC45C / 0x088BD7D8 / 0x089DEF3C / 0x089DEB88 / 0x089DFD68 | steps from tables (0.103.0, `object/batch_steps.c`): by + 4 from 0x08A16DA8 (+ 4 = 2 while GAME + 38); by + 5 from 0x08A16F14 (+ 5 = 2, y 0xF0 when BATTLE + 44 is 5); by + 5 from 0x08A24B5C (the owner's + 10 takes + 22); by + 3 from 0x08A36F24 / 0x08A36F40 after the bottom text when GAME + 20 is 4; by + 3 from 0x08A3AEF8 / 0x08A3AFC0, or freed (0x088231C8 / 0x0882324C) once GAME + 4 is 0; by + 4 from 0x08AB76D8 / 0x08AB76BC, then object lists and the frame's end; 0x08AB7A28 by bits 8 - 11 of + 66 |
| 0x088773E8 / 0x0885EACC / 0x0887A2A4 / 0x0887A0B8 / 0x08876894 / 0x08987118 / 0x088BE788 | a fighter: the CPU's first script (+ 480) and command (+ 481), then 0x0887A400; routine + 96 (human) / + 100 (CPU) of + 416, kept on screen unless + 135; a chance by level (0x08A227A0) to restart the script, else one at random; routine 1.2 from a script word (+ 490, + 192 / + 193); the frame record (+ 28: + 296, + 74 - + 77, + 204 / + 256); x nudged 4 px (CPU towards the opponent, a human by + 362); + 376 flipped for the same character (or 0x15 / 0x25) |
| 0x088B0A50 / 0x088B53F4 / 0x088CEABC / 0x088B1E18 / 0x089CEFC8 / 0x088BCAFC / 0x088BCC2C / 0x089DFE78 | places from 0x08A2F2CC (by + 3 and 0x08B02848) and 0x08A376C2 (by + 3 and 0x08B02E08); shown by the 0x354-byte record 0x09519588[+ 78] at the owner's place (+ 0x72, - 8); the other's shown flag and place (types 5 - 9); a bounce at the limit + 88; x by + 98 wrapping at + 76:+ 77 (a big-endian u16) right / left; an entry at + 224 + 10 * + 264 (kept 0 - 3) |
| 0x08999370 / 0x0899544C / 0x088AB120 / 0x08855874 / 0x088BD44C / 0x088BDFBC / 0x0884A210 / 0x088AA268 / 0x088B0BCC / 0x088378F8 / 0x088B5B2C / 0x088EACC8 / 0x08881D9C / 0x088B09B8 / 0x088B4ABC / 0x088B4424 | moves by facing then a fall, or until + 96 is negative; + 9 on and 0x088AB1D8; + 82 counting down before 0x08855934; animation 0x08A396A8[n]; a record reset with the fight's effects and 0x092E891B = 15; 0x0950FF7C / GAME + 48 = 2; count-downs (+ 13, + 280), count-ups (+ 98, + 82 / + 32 / + 34 under 14, + 38 by 4); set-ups (kind 0x1B, white colours, kind at x / y) |
| 0x0880BA94 / 0x0880DA78 / 0x08821600 / 0x08822D2C | an object drawn (0.104.0, `object/batch_large.c`): parts + 52 / + 48 before, itself through the drawer state 0x08AD9320 (place x + + 268 and 240 - y, facing + 11, colour + 160 - + 163, + 168, + 24; 0x0880E6E8 then 0x0880C4C8), parts + 56 / + 44 / + 40 after; the fading shape (shape 3 * (n / 24) + (n % 24) / 8, alpha n % 8, n = the low 12 bits at + 12's + 4), drawn twice unless + 1; an object put on 0x09517700 / 7760 (by + 121, counts 0x095177C0 / C1) and 0x092E2300 + 0x092E23C8 or their + 0x60 (by + 9, counts 0x092E22F8 / F9 copied to 0x092E23C0 / C1); made from twelve 0x168-byte slots down from 0x09510F28 or 0x09512008 (free counts 0x095176F0 / F1) |
| 0x08873728 / 0x08873BF0 / 0x088627D0 / 0x08862FF0 / 0x08863E1C / 0x088BE210 | fighters: two boxes meet (x mirrored by facing, half widths + 4 and heights + 6; + 714 - + 720 keep the places; the second version 4 wider when + 386 is -1); in range of the boxes along the frame record's chain (+ 28, + 6, + 416's + 16); pushed apart (+ 238 = 1 overlap, 2 past 0x180, margin 0x18 with 0x08A0588A); a walk step from 0x08A18D04 / 0x08A18C8C / 0x08A18EC4 (+ 584 the count); a random character and ISM, not the side's last (0x09525DEC / DEE) |
| 0x08856B38 / 0x088AF9DC / 0x088CF388 / 0x088D0354 / 0x088B2A5C / 0x088B520C / 0x088B42D0 / 0x088BD334 / 0x089DFA54 / 0x089DFED4 | set-ups: the intro's + 73 by the fight's state and 0x08A16F34 into 0x08A16750; kind 0x14 with an animation by + 3 (0x08A2ED04 / EBA0); places and animations by GAME + 4 / + 6 (0x08A3C87C); a panel shown by screen; colours by + 92; 66 particles (0x08B0285C, places 0x08A3709E, reset from 0x08B02C7C); moving to a target (+ 92 / + 94); five kind-0x23 objects; a record unpacked into three slots at 0x0951F950; objects made from a record's 32-byte list |
| 0x0880BA48 / 0x08871ED0 / 0x0885ECE8 / 0x0886731C / 0x0887DD64 / 0x0887684C / 0x0889229C / 0x08897F34 / 0x088980F4 | (0.105.0, `object/batch_large2.c`) the four fighters drawn (0x092E22F4 down to 0x092E22E8); a guard's damage cut (+669 = 4, +670 = 11, by 0x08A21EAC[+647 + kind]: half, or 1/4 - 1/32; the hit record 0x09358410); the guard choice by count +82 (CPU: chance 0x08A22AA0; human: back held with one of eight button pairs; mode 2: 0x0950FF6A / 6B); a fighter's round clear (~60 fields; the opponent's +592 / +770 settle); a kind-4 effect beside a fighter or its owner at a random spread; frame n of +352's list; name entry (+456 + cursor +463, pads 0x08E75BA0 + 4 / 12 / 48 / 56); a fighter's label set up (x 0x08A2A05C, y 0x08A2A060) and shown (0x08A287FE entries, 0x08ACCAE6 pictures) |
| 0x088B5578 / 0x088B56E4 / 0x088AED5C / 0x088AE5A8 / 0x088BDBB8 / 0x088B0960 / 0x088B48C4 / 0x088BC044 / 0x088813EC | objects: moving for +73 frames then turning round (+106 / +107); moving to a target and bouncing (+79 times, then GAME + 22's bit); set up by type (0x089FF750's switch, entries at 0x08A2EA10, animations 0x08A2E988); copying a source and making a kind-0x23 object (0x08A2E198 / E190); a name box by +92 (0x08ACCD54); swap then tick; +6 on and the five kind-0x23 objects; kind 0 at (0xB0, 0x88) |
| 0x08878ACC / 0x08877FBC / 0x0882BD04 / 0x088344E8 / 0x088636D8 / 0x08869B70 / 0x088768F0 / 0x0886B468 | (0.106.0, `object/batch_mid.c`) fighters: script commands that leave the block or read on by the level +508 (+492; +490, else the script ends); a level 1 - 2 by the other's +9 (0x08A0EF1C); new direction presses counting +341 (+347 reloads with 31); the input (+248) recorded at +378 and counted at +436 / +433 (0x08A195B0); a press counter with a wait and a need; the next frame record (+6 records of ten bytes on, flag 0x40 stays); BATTLE +136 / +140 by +121, swapped in battle states 2 and 3 |
| 0x08848050 / 0x088CF908 / 0x08847FE4 / 0x088B4514 / 0x088B4FD4 / 0x088B4EBC / 0x088B3040 / 0x088B30A8 / 0x088AA9BC / 0x089DFC40 / 0x089E044C / 0x088D42F0 / 0x088BC4B8 / 0x088BCC8C / 0x08887CBC / 0x08998214 / 0x088CE120 / 0x089D99E8 | objects: shown by the side record 0x09519588 +58 at 0x08A13408's point; shown with a parent (+124 / +88) at its place; the trail ring at +112 (sixteen points by +36; a set +72 empties the slot after); +108's top kept within 0 - 155, and split by 39 for y and the animation (0x08A37314); the timed list 0x08B0284C (0x08A36EFC / 6F0C by GAME +14); 0x08A2C68C's entries into +72 / +73 and 0x08B027F2; set-up from +66 - +76; 0x09525E00's change of +110 (switch 0x08A01D70); the fight's two side records 0x09358528 / 0x0935887C cleared; steps from 0x08A3AF28 / AF58 while the parent lives, 0x08A25B48 by +128, 0x08A98AA8 / 8A74 by +192, 0x08A3C93C, 0x08AB6A00 unless under +118 |
| 0x08822144 / 0x0887B418 / 0x0885F050 / 0x08878D30 / 0x088A377C / 0x088BBFA0 / 0x0883A6E0 / 0x089DEBE8 / 0x0880D5B4 / 0x0887BBC8 / 0x089DE978 / 0x088AB0A4 / 0x088B4498 / 0x088BC070 | (0.107.0, `object/batch_mid2.c`) the pools, small objects and fight tables reset with the four fighters at 0x092E22E8; a fighter's script +510 from its routine table's +60 (or +168 networked with +815); its routine +772 (0x08A19344) and kept on screen; a script command by the opponent's +536 / +228; the first unused colour from +376 (bits at 0x08B02718); the VS screen's step (0x08A381F0) with lists 1, 4, 3; texts from 0x08A12244 and 0x08A01CB8 - CE0; three shapes 0x80 apart; free to act (step 1.1 with +116 = 6, or standing); a small object with line n of 0x09358BE0 +304; companions of kind 7, types 9 / 10; following four back in an owner's trail; the camera 0x092E8800 scrolled by +132 / +136 |
| 0x08877B58 / 0x088781C4 / 0x08878F40 / 0x088793A8 / 0x08834558 / 0x08834430 / 0x08856C60 / 0x088572E4 / 0x08863748 / 0x088637CC / 0x088B5334 / 0x088AB150 / 0x088AC210 / 0x089CF0A4 / 0x088BD268 / 0x0887D738 / 0x0889C04C | (0.108.0, `object/batch_mid3.c`) script commands: read on while free to act (else +483 = 10, routine 1), +490 / +506 by the level, by the opponent's +73 / +78 and +73 / +100; +347 counting down (the side's bit in +53 cleared at 0) and the press step from 0x08A0F8F0; the intro's pieces placed and moving (0x08A16962, 0x08A16A90 +60 / +64); four kind-15 objects beside (0x08A195C0 / C8); the 66 particles' start (0x08A37628, 0x08A3763E); flashes by +13 into GAME +22; y held at +114; dropping to the routine entry's y + 0x30; a kind-4 object on an owner; a row by side at 1.5 |
| 0x088BE178 / 0x088BE4E0 / 0x0887B9EC / 0x08871E40 / 0x0892B31C / 0x088576EC / 0x08855CB0 / 0x088BD1D4 / 0x088AAC80 / 0x088B5AA0 / 0x088B2204 / 0x088B2298 / 0x0888AB48 / 0x0888ABDC / 0x088AA398 / 0x088BC868 | (0.109.0, `object/batch_mid4.c`) both fighters at random (P1 second colour on a match) and the stage by P1's character (10, 30, 31 become 30); a fighter's first state (+420 = 0x90, +362 the keys); close enough to throw (+508 under 48, the opponent standing, +235); x pushed by 0x08A1ECDC by +647; an effect taking a fighter's look (+228, +536); digits as pictures 0x49 + n (blank 0x4000); the intro's owner with parts 10 - 12; two kind-0x23 objects and a kind-11 one; panels (white full screen, black 0x180 x 0x40 / 0x18, 0xB0 x 0x18); GAME +22 by +2 (0x08A2C62C / C63C); falling to +132 |
| 0x08837854 / 0x0887E7AC / 0x088728B0 / 0x08869AC8 / 0x08809934 / 0x08878B88 / 0x08821384 / 0x089E3364 / 0x088230F4 / 0x088B4230 / 0x088B4DD4 / 0x088B0D08 / 0x089D9C04 / 0x0884A0B4 / 0x08846BBC | (0.110.0, `object/batch_mid5.c`) the first list's twelve 0x168-byte objects from 0x09511090 (count GAME +66, steps 0x08A10130); a kind-5 object from a description (step, kind, +78, offset mirrored by +11); a fighter's input (+248) given to its team by battle state; three press counters with a table's waits / needs; file 0x08A03150 queued and frames waited for 0x08E7606C; a script command by +366 (0x90 with +371 / +386); routine 3 past the screen's edge (depth by +38); values for 12 kinds (0x08A01DE8); an object cleared for use (free counts 0x095176F0 / F1); following a trail until GAME +22's bit; moving to +104; a panel object of kind 13; the record 0x0950FF60 reset |
| 0x08820BE0 / 0x088288F0 / 0x088782C8 / 0x088783B0 / 0x08871CA4 / 0x088734B4 / 0x08887D54 / 0x088A44B8 / 0x088BCFC0 / 0x08892438 / 0x089DE194 / 0x089DEA64 / 0x0880D3B0 / 0x08821AA8 | (0.111.0, `object/batch_mid6.c`) the random number (the twister at 0x08B01564, index 0x08B01F24, tempered; GAME +64); the four fighters' routine tables 0x0951CE68 + 0xAA8 n; script commands within 2 of the level and setting the CPU state (0x0887B528, commands 0x08A2254C); a push with a random spread (0x08A1DB88 / DBB2 / DBDC); a hit's score (0x08A21F6E / 1FAE); colours from the 15-bit palette 0x092DF980; a choice of two on the select screen (+441); a four-letter name matched in 0x08A26DF4; the next round's flags; the three camera tables 0x0951F950; mask pictures from 0x08E75AD8; an object's next frame record |
| 0x088377A8 / 0x0887BC44 / 0x088726EC / 0x0890EC3C / 0x089D32D4 / 0x088764A8 / 0x088B4618 / 0x088B47B8 / 0x088B3E84 / 0x088B4174 / 0x088B0AA8 / 0x088B0DE4 / 0x088B2544 / 0x088EAD20 / 0x0888AE14 / 0x08846B58 | (0.112.0, `object/batch_mid7.c`) the twelve objects at 0x0950FFB0 run like the first list, then the first list; the CPU's first script from 0x08A2277C by a random 0x08A2275C entry; a fighter's pad byte (+22; 8 with +80; +0x20 with 0x09358458; +0x40 while its team stands) and BATTLE +68 from 0x08A21F4E; a fighter's step by +192 / +193 (0x08A5ACDC - AD04); falling to +114 then bouncing; moved unless paused; particles cleared, placed and animated, their motion from 0x08A373C0; pictures by +3 and 0x08B02848 (0x08A2F314); panels by GAME +22 (0x08A352B8) and +92 (0x08A3F440 / F4A0); a green pulse 8 - 0x80 |
| 0x089DE8A4 / 0x08863600 / 0x089130D4 / 0x088ABD4C / 0x089CD714 / 0x0892ED04 / 0x088B03C0 / 0x088B0590 / 0x088B0840 / 0x088B4C74 / 0x088B0F28 / 0x088BC944 | (0.113.0, `object/batch_mid8.c`) a record's four timed lines at +0xE0 (start count +6, then a small object with line +2 / +4; end count +8, BATTLE +82 = 2); a fighter's input recorded for its team by battle state; an effect at a fighter's place animated by its frame's +10 (0x08A58688); counts of a screen's pieces at 0x08B0274C (+18 / +22 / +23 to 9 / 15 / 14); steps by kind or +119 (0x08AB6234); +139 counting down to frame 6; pieces set up from 0x08A2F2CC / F314; particles from 0x08A37554; set-ups from 0x08A2FBFC and by the screen record (+8, +9, +14 mod 3) |
| 0x08871D60 / 0x08857F8C / 0x08855BD8 / 0x088CF7A4 / 0x089E0178 / 0x089CD29C / 0x088B4B84 / 0x08847CF8 | (0.114.0, `object/batch_mid9.c`) damage scaled by the move's +21 row of 0x08A21E8C (x 1.25 / 1.5) and the fighter's +792 flags (0x4000 / 0x40 x 1.375, 0x40000 x 3, 0x20000 x 2.5, 0x10000 x 2); the reward count's shapes from 0x08A17208 / 7218 (60 frames apart); the intro's owner with parts from 0x08A16DF8 by GAME +114; option panels placed from 0x08A3CAC0 / CAC4 and shown by mode; camera table n restarted from 0x08AB77B0 / 7840 / 78D0; a start at +118 (steps 0x08AB6158); a particle's wait from 0x08A37548; pictures by +3 |
| 0x0886ABB4 / 0x0883436C / 0x0883311C / 0x0882659C / 0x08817950 / 0x08879EB4 / 0x08889968 / 0x08889200 / 0x088B4014 / 0x0889B90C | (0.115.0, `object/batch_mid10.c`) the held fighter put into the hold (state 1.3, +82 = 4, +135 = -1) and both throw fields cleared; continue counting (+340) by mode, character 10 or 0x08A058A0, else the step 0x08891DF0; a continuing match's pick (+157 / +158; pad 0x20 = 4, 0x40 = 3, else 0x08A0F81C); the special match for character 9 (+112 / +116, +50 / +51, +69 / +70); the demo screen (step 7.0, kind-0x99 objects, list-3 kinds 0x18 and 6); a script command by layout with +500 kept by a chance (0x08A228A0); panels from 0x08A264C0 / 68F8 and 0x08A262D0 / 62D5; a row of particles (0x08A37390); a record rising by side |
| 0x088CE18C / 0x088B3A84 / 0x088B3258 / 0x089DE52C / 0x089DDA68 / 0x088BE69C / 0x0885CC1C | (0.116.0, `object/batch_mid11.c`) an object set up by kind from 0x08A3C94C (animation index in 0x08A3C87C, x, y; kinds 6 / 7 hidden); four kind-0x7D objects from pool 4 (0x095176F3) with GAME +10 / +22; four objects of a kind chained by +88; the three camera tables (0x0951F950) moved by +160 and copied to the camera layers 0x092E8800 / 88A0 / 8940; a panel's blend and colour by type from 0x08AB6F20; a fight's two fighters paired (P1 / P2 in use and facing, P3 / P4 cleared, colour +376 from the ISM, flipped if both match); the CPU's aggression (+589) nudged by 0x08A19226 / 922A / 924A, at most 8 either way |

**The other dispatcher shapes** (0.62.0, `step.h`): 96 run the step and then one or two fixed routines on the
object (0x08821424; or 0x088219B0 then 0x088215C4, the animation and the drawing; s0 holds the object):
26 decompiled. 32 hand the handler the object's owner (the pointer at + 88) as its second argument: 11
decompiled.

## The loader

Decompiled in `src/core/loader/` (format: [PAC_FORMAT.md](PAC_FORMAT.md)).

| Address | What |
|---|---|
| 0x088075F8 | queue a load: name into the ring of 16 (names 0x08E86E20, 36 bytes each; next slot u8 0x08E76084); hangs if the slot is busy |
| 0x08807F18 / 0x08808010 | open data/<name> into a FileHandle (the first wakes the drive on 0x80210001; the second, the loader thread's, does not) |
| 0x088080D8 / 0x08808144 | read / close |
| 0x08808188 | path: sprintf "disc0:/PSP_GAME/USRDIR/usa/data/%s" (0x08A03740) |
| 0x08808424 | pac_load(n): archive n of the name table 0x08A03140, entry by entry to its destination |
| 0x0880775C / 0x0880777C | the loader thread: once a frame, one step of the first waiting request, by its state (`async.c`) |
| 0x08A03798 | the loader thread's state table: 0 open 0x08807810, 1 header done 0x08807894, 2 entry start 0x08808ED4, 3 chunk done 0x088094B8, 4 sound 0x08807988, 5 wait 0x08807A30, 6 finish 0x08807A80, 7 read header 0x08807C18, 8 retry open 0x08807DAC, 9 retry read 0x08807E7C; sound callback 0x08807A38 |
| 0x08E87060 / 0x08E76064 / 0x08E76030 | loader thread: current request / texture staging buffer / sound handle |
| 0x08E76088 | 16 LoadRequests, 216 bytes each (`loader.h`) |
| 0x08E87080 / 0x08E76E20 | archive header buffer (2 KB) / read buffer (32 KB) |
| 0x08A03788 | a file is open |
| 0x08E87940 + *0x08E87880 | the fighter heap: where fighter data goes, and how much is used |

Destination tables (by entry kind): textures 0x08A05530 (pages of 12-byte slots), palettes 0x08A0577C,
text 0x08A05834, 0x00020000 0x08A0584C, data 0x08A055DC, 0x01000000 0x08A0562C, 0x02000000
0x08A056D4, 0x08000000 0x08A05824, voice banks 0x08A0558C, fighter data 0x08E75AD8 / 0x092D9D40 /
0x092D9E00.

## Stages and the camera

See [STAGES.md](STAGES.md): stage numbers and names, the select screen's choice (0x08E759D1) and its
table (0x08A2B3E6), the stage being fought on (0x092E1228), the layers (0x092E8800, 0xA0 each; the
fighters' layer 0x092E88A0, limits 256 / 640 set at 0x0881ED74), the camera update (0x0881E1B4 and
0x0881E49C / 0x0881E4E4 clamping at +68 / +70).

### The background tile drawers (decompiled: src/core/stage/tiles.c)

A stage layer is a grid of square tiles from a tile map that wraps, one textured quad each (the quad
list 0x08B014DC: corners 0x08B014EC, texels 0x08B0150C; drawn by 0x089E1618 with the depth from
0x0880A3C4's table 0x08A03B10). A map word: bit 31 a tile to draw, bits 28-29 its flip, bit 30 the
blend, bits 8-15 the palette (0x089E0EA4), bits 0-7 its 16-texel cell.

| Address | What |
|---|---|
| 0x0881AB28 | 32-pixel tiles, 13 x 9 (WIDE NATIVE: 17 columns from 2 tiles left). a0 the layer (+3 kind: 5 a 16-wide map wrapping at 0xD00, 0xDA / 0xEE animated (+60 countdown of 6, +131 frame of 11), else 32 wide; +18 / +98 the scroll; +122 the depth column), a1 the map. Works out the grid record on its stack and draws it |
| 0x0881B268 | 16-pixel tiles, 25 columns (WIDE NATIVE: 31 from 3 tiles left) by the record's +62 rows. a0 a record in game memory, laid out as 0x0881AB28's (kind 0x85 animated by layer 0's record 0x092E8800 (+60 countdown, +131 frame) and the table 0x08A08AD8; 0x90 strips at +72 onwards) |
| 0x0881B7C8 | the third drawer (`tiles.c`, 0.99.0): 32-pixel tiles from a map 24 wide, wrapping down every 64 rows, the horizontal scroll wrapped into 0 - 767; called only by 0x0881A2C8 (from the ending task 0x089DE0E8's steps 0x089DE2C0 / 0x089DE390, when 0x0952F950 + 304 is set and the mode word at + 308 is 2); `arcade_ending_full` reaches it. WIDE NATIVE (0.120.0): no longer mirrored; an ending's scenes are cut at the 4:3 sides or drawn 1.25 times larger (`task/ending.c`, `extensions/scaled_endings.c`) |
| 0x0881A2C8 | the ending's layers (`task/ending.c`, 0.120.0): the quad list's descriptor and grey, then the fight's three layer records 0x092E8800 / 88A0 / 8940 (each turned on, +7, the first frame) drawn while their camera layer 0x0951F950 + 0 / 0x98 / 0x130 is set, by the ending's kind 0x09358BE0 + 308 (0: the picture 0x08819498, the sky 0x0881C0C8 + 0x0881A8E0, 0x0881AB28; 1: 0x08819300, 0x0881A8E0, 0x0881AB28; 2: 0x08819300, 0x0881A8E0, 0x0881B7C8), maps from 0x08A07918 (12 bytes by kind) |
| 0x089DE2C0 / 0x089DE390 | the ending task's steps (`task/ending.c`, 0.120.0): the step (frame count +6, script 0x089DF82C, its requests 0x089DF624, places 0x089DE444, timed 0x089DE7C0 / 0x089DE8A4, the object lists, camera layers 0x089DE52C, the layers 0x0881A2C8; a button moves the task on), and a draw-only step until GAME + 93 clears; the object is 0x09358BE0 |
| 0x089DE444 / 0x089DE4B4 / 0x089DE7C0 | the ending's layer places from its records +0xAA / +0xBC / +0xCE (18 bytes) into the camera layers; the timed fade-out object (+50, 0x180 x 0x30), 0x0887C708 (+54) and sound 0xB4 (+56) |

Paused (0x08E75969) or either fight flag 0x092E1237 / 0x092E124D stops their animation.

### The camera's frame (decompiled: src/core/stage/camera_frame.c)

| Address | What |
|---|---|
| 0x0881D1A8 | once a frame from 0x0881DF10: unless paused (GAME + 89, BATTLE + 15, GAME + 145) the fighters' layer 0x0881D244, the lean 0x0881D348, layer 0 0x0881D960, layer 2 0x0881DC6C, 0x0881EE84, 0x08819BBC; always the background 0x08819CFC |
| 0x0881ECA4 | places a camera layer at (x, y) by its mode +2 (0 layer 0, 1 the fighters' layer with limits 0x10 / 0x100 / 0x280, 2 layer 2); alignment masks 0x08A09468 |
| 0x0881D244 / 0x0881E1B4 | the fighters' layer: placed at (0x1C0, 0) the first frame, then follows the fighters (0x0881E320, 0x0881E538); +16 / +96 of layer 1 from +34 / +38 (BISON: +60 counts the sky's ripple) |
| 0x0881D348 | the screen's lean (0x092E5F78): +130 / +132 heights from 0x08A09268, frames 0x08A092BC by stage (0xFFFF none); each frame +28 = (layer 1's +32 less its +8) / +128, +112 = +28 × (+132 less +130) (SAGAT + 16 × +28, VEGA + 8 ×) |
| 0x0881D960 / 0x0881DC6C | layers 0 and 2: start from 0x08A08FC8, follow at rates 0x08A09118 (by stage, s16 x4) through 0x0881E058 / 0x0881E148; own drifts: ROLENTO and BATTLE1 - 4 layer 0, GUILE layer 2 (+80); ZANGIEF layer 2's 4 frames; layer 0's +155 off on VEGA / DHALSIM, layer 2's on for KEN, CHARLIE, SAGAT |
| 0x0881E0D8 | CHUN-LI's layer 2 and VEGA's layer 0: the fighters' layer's travel less the lean × rate |
| 0x0881D4D8 / 0x0881D588 / 0x0881D5F4 (0x0881D6EC, steps 0x08A093B0: 0x0881D768 ...) / 0x0881D8A8 | GEN's figure (0x08A0938E, random), VEGA's falling cage (sound 0x37), GUILE's train, KARIN 1's boat (0x08A093F8) |

### The stage's setup and animators (decompiled: src/core/stage/setup.c, animators.c)

| Address | What |
|---|---|
| 0x08819A20 | the stage's setup: 0x08815818, then 0x08A08A14[stage] (23 stages): palette animators (0x08823074: +8 script of {frames, +3, list, step}, +12 lists of {slot, source \| 0x80 last}, +16 source 0x092EEA00, +20 step) started by 0x0881F1FC, map animators (0x088230B4: +1 tile size, +8 / +10 x / y, +12 12-byte entries {+2, +3, map 0x08A08ABC[n], columns, rows, words}, +16 step) written by 0x08819A68, and palette slots 0xA3 - 0xC3 (0x089E1234) |
| 0x0881EEF4 / 0x0881EF38 / 0x0881EFB4 / 0x0881F01C / 0x0881F060 / 0x0881F0D0 / 0x0881F114 / 0x0881F148 | palette animators' steps: plain; held until 1 in 128; rotating slots 0xA3 + (0x0881F5D4, table 0x08A09480); random waits (0x0881F48C); BISON's (also the fighters' layer's frame); ZANGIEF's by layer 2's frame (0x0881F308); pairs (0x0881F3E4); HONDA's from the round's start, rewriting the map animator 0x095177C8 |
| 0x08819C2C / 0x08819C74 | map animators' steps: every +2 frames; KEN's crowd facing the nearer fighter within 48 px |
| 0x0881F6A0 / 0x0881F794 | the fighters' palettes for the layout (BATTLE + 56): 0x092D9E00[character] + colour × 160 (or × 256 for character 0x29), through 0x0880AEA8 |
| 0x0881CE4C / 0x0881CEF4 | the select / VS screens' backdrop: layers 1 / 2 set to fixed places, palette slots 0xE0 - 0xE5 and two from 0x08A08D08 |

### The stage's background and floor (decompiled: src/core/stage/background.c, floor.c)

| Address | What |
|---|---|
| 0x08819CFC | the background, once a frame (skipped while BATTLE + 23): sets the quad list's descriptor and its colour from the brightness 0x08E75C44, then draws by layer 1's +123 bits: layer 1 (2), the stage's own routine 0x088192C0 with layer 1 (8), layer 0 (1), layer 2 (4). The layer's kind (+3) picks the drawer; the first time (+7 = 0) it sets the kind from the stage (layer 0 + 0x70, 1 + 0xA0, 2 + 0xD0) instead. Layer 1: 0xAA / 0xBE 0x0881C0C8, 0xB5 / 0xC0 0x0881A8E0 (maps + 0x2400 / + 0x3400), else 0x0881BD28 (+ 0x2000). Layer 0: 0x7A / 0x8E 0x08819498, 0x90 0x0881C354, +155 set 0x0881A8E0, else 0x0881AB28. Layer 2: 0xE5 / 0xF0 0x0881AB28 (+ 0x4400 / + 0x5400), +155 0x0881A8E0, else 0x0881AB28 (+ 0x4000). Maps at 0x092E8A00 |
| 0x0881A8E0 | a 16-pixel layer 64 tiles wide: builds 0x0881B268's record (kind 0x85: 16 rows of 0x90 words; else 32 of 0x40); scrolls with the fighters' layer when +2 = 1 |
| 0x0881BD28 | layer 1's default 16-pixel drawer; kinds 0xA0 (RYU) / 0xA4 (CHUN-LI) draw the rows above a line (0x8F / 0xEF less the scroll and 0x40 / 0x70) lagging by the shake's +28 (× 8, or × the height difference + 2), then the rest with the fighters' layer: 0x0881B268 twice |
| 0x0881C354 | GUILE's layer 0: strips 16 words wide, 0xD0 a row, picked per 256 pixels of +18 by the table 0x08A08BDD; 0x0881B268's kind 0x90 |
| 0x08819498 | BISON's layer 0, a picture: one quad, layer rows 0x60 - 0xB0, x 0x200 - 0x300 less the fighters' layer's travel, palette 6, v 0 / 0x50 swapping every 8 frames (+60, +131) |
| 0x0881C0C8 | BISON's layer 1, the rippling sky: 3 columns of 256 × 160 one-pixel slices nudged by the wave table 0x08A08ADC (s8, 0x7F ends it) from +62; palette 7, then 8 after 256 rows |
| 0x0881C5A8 | the floor band (22 stages' own routine): 4 quads of 64 texels / 256 pixels from the layer's scroll, leaning with the shake (+28, +130 / +132); texture 0x08B0155C + frame |
| 0x088196DC / 0x0881C82C | CHARLIE's and the BISON stages' own routines: a band of their own (palette 1; palettes 4 + frame / 2 and the frame), then 0x0881C5A8 |

Draw depths, measured on every stage in the fight: layers 5, 9 and 13 (table 0x08A03B10 row 1), the floor 13; sprites (fighters, stage objects, HUD) take theirs from 0x08AE03B8 by their +24 (2, 4, ... 16, 17), the fighters 16. The 2D library keeps each frame's draws in submission order and sends them by depth 0 - 23 (0x089E4408), so equal depths keep their order: stage objects in sprite layer 0 (+9) at depth 16 are queued after the fighters and are the only stage art in front of them (STAGES.md lists them).

### The stage's objects (decompiled: src/core/stage/objects.c)

The pieces of a stage that move or animate but are not tile layers (Ryu's straw and rocks, Ken's crowd,
Guile's helicopter, Bison's storm and leaves...) are objects of pool 5 (0x09514B70, 16 objects of 0xC0,
`object_make_16`), run by lists.c's second list through the kind table 0x08A101F0 and drawn as sprites
(queue D, 0x0880C308). `python tools/stage_objects.py` lists every stage's records.

| Address | What |
|---|---|
| 0x0884AE40 | makes the stage's objects (from the fight's set-up and the screens that show a stage): one per 16-byte record of 0x08A143D8[stage] until the record with bit 7 of its first byte. A record: +0 kind (bits 0-6) and last (bit 7), +1 type, +2 layer (+9), +3 (+78), +4 camera (+14), +5 (+11), +6 / +8 x / y, +10 (+26), +12 (+24), +14 depth (+38). Then +164 = 0 on stages 0x06 / 0x09 / 0x18, else 1; +86 / +148 = 15 / 0 on 0x02, 06 - 09, 10, 14, 17, 18, 20, 22, 24, 29, else 4 / 7. BATTLE1 - 4 have none |
| 0x08A101F0 | u32[85]: the stage objects' routines by kind (+2); 0x0884B034 (kind 0) ... 0x0885498C (84). Their animation tables are by kind, picked by type (+3): 0x08A144A0 (kinds 0 - 2), 0x08A146A0 (3, 4) ... |
| kinds | The shapes most share (objects.c): *follow* (animated; drawn near the screen when +96 = 16 x +78 gives a margin; moves back with the fighters' layer at +100's rate in sixteenths, or fixed rates of 14 / 15 / 10, gated by +14 = 0 or +26 != 0), *plain*, *pause at the line* (step at +7: stops animating for a frame each time the fighters' layer crosses x +132), *swap at the line* (two looks in a block at +96: the record's and a fixed one, taken while the layer is past x +132), plus about thirty of their own: GUILE's helicopter (0x08852790, its steps by +5 0x0885303C), the things crossing its sky (0x08852B70, sent by 0x08852900 through 0x08852DB0), its three jumping pieces (0x08853500); BISON's storm (0x0884D3D4 / 0x088521D4) and falling leaves (0x0884D5F8 / 0x088523F8); BALROG's pacing figure (0x08850424) and the pieces it throws (0x08850084), its bobbing one (0x0885027C); FEI LONG's crowd that reacts to the fighters' +112 (0x088538A0); HONDA's figure that changes sides with the camera (0x08851004); KARIN's crossing car (0x08851EBC); and more |
| 0x08821044 | an object moved: x / y (+16 / +20) by their speeds (+96 / +100), the speeds by +104 / +108 (`object/motion.c`) |
