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

Applies cleanly to this `BOOT.BIN`. Changes 35 words in `.text` (viewport 384 -> 480 and the camera
-192 -> -240 as immediates; 0x0880F390, the pillarbox, returns at once; six stores in the background
tile drawers 0x0881AB28 / 0x0881B268 / 0x0881B7C8 become jumps into caves) and adds code in caves in
unused `.data` (0x08A02ED0, 0x08A02F38, 0x08A031B4; zero in the original, though 0x08A02ED0 is also
an address the program computes). `prepare.py` records the overlay (`work/boot.widescreen.json`) and the
translation switches it with `rt_widescreen` (SCREEN SIZE > WIDE FORCED). The game's own display option
must be **Normal** for it, which WIDE FORCED sees to.

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
| 0x0880DB78 | the fight's overlay object at 0x092E51C0 (+0 active, +2 shown), skipped while 0x08E7602C or BATTLE + 36: layer 0 0x0880DC88, layer 1 0x0880DC08, 0x0880DE2C, 0x0880DF98, 0x0880DFC8 |
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
| `object/screen_steps.c` | the screens' objects wait on their fighter's flags at + 424 (bits 1, 2, 4, 8: 0x08899DC8, 0x0889A194, 0x0889ADFC, 0x0889A478, 0x0889A728, 0x0889C604), start animations from 0x08A27744 (0x0889CDC8, 0x0889CDF8) and 0x08A28950 (0x0889CE34), and move to their targets (0x0889B888, 0x0889BE88, 0x08899514) |

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
| 0x0881AB28 | 32-pixel tiles, 13 x 9 (WIDE FORCED: 17 columns from 2 tiles left). a0 the layer (+3 kind: 5 a 16-wide map wrapping at 0xD00, 0xDA / 0xEE animated (+60 countdown of 6, +131 frame of 11), else 32 wide; +18 / +98 the scroll; +122 the depth column), a1 the map. Works out the grid record on its stack and draws it |
| 0x0881B268 | 16-pixel tiles, 25 columns (WIDE FORCED: 31 from 3 tiles left) by the record's +62 rows. a0 a record in game memory, laid out as 0x0881AB28's (kind 0x85 animated by 0x092F8800 and the table 0x08A08AD8; 0x90 strips at +72 onwards) |
| 0x0881B7C8 | the third drawer, still translated: called only by 0x0881A2C8 (from 0x089DE2C0 / 0x089DE390, when 0x0952F950 + 304 is set and the mode word at + 308 is 2), which no scenario reaches yet |
| 0x0881A2C8 | sets up the quad list's descriptor and draws a screen's layers through the three drawers |

Paused (0x08E75969) or either fight flag 0x092E1237 / 0x092E124D stops their animation.
