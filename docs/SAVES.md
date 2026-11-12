# Saves and settings

The project does not use a PSP memory stick. Everything the game saves, and every setting, is kept in
**your a3mx folder**, the system's own place for a program's files, so a new download or a fresh clone
finds them again:

| System | Your a3mx folder |
|---|---|
| Windows | `%APPDATA%\a3mx` (`C:\Users\<you>\AppData\Roaming\a3mx`: paste `%APPDATA%\a3mx` into Explorer's address bar) |
| macOS | `~/Library/Application Support/a3mx` |
| Linux | `$XDG_DATA_HOME/a3mx`, else `~/.local/share/a3mx` |

| Folder in it | What |
|---|---|
| `saves/` | `a3mx.dat`, its `.bak`, `import/`, `export/` (and `ms0/`, the memory stick of `--no-extensions`) |
| `configs/` | `a3mx.cfg` and its `.bak` |
| `logs/` | `a3mx.log` and `last_run.txt` from the last run, and the launcher's `launcher.log` (builds and downloads) |
| `launcher/` | the launcher's own settings, `launcher.ini` |

**If that folder can't be written** (a locked-down account), the same four folders are made beside the
game instead, next to `imports/`, and the log's first lines say so. To choose the folder yourself (a
portable copy on a stick), start the game with `--user-dir DIR` or set `A3MX_USER_DIR`.

The two files that matter:

| File | What |
|---|---|
| `a3mx.cfg` | every setting, by name: EX. SETTING, GAME OPTION, SCREEN SETTING (with VSYNC and FRAME TIMING: [DISPLAY.md](DISPLAY.md)), each player's buttons (PLAYER CONFIG, with L2 / R2 and player 4), unlocks |
| `a3mx.dat` | everything else the game saves (rankings, records, Entry and Edit characters, World Tour), as blocks of hex |

The settings in `a3mx.cfg` win over the same bytes in `a3mx.dat`, so `a3mx.cfg` can be edited by
hand (with the program closed). Button settings take the names shown on the P1 - P4 CONFIG screens
(see [PLAYER_CONFIG.md](PLAYER_CONFIG.md)) or their numbers.

**Without the extensions** (`--no-extensions`, or a build without them) none of this applies: the game
saves as on a PSP, to the emulated memory stick (`saves/ms0/PSP/SAVEDATA/ULUS10062*/` in your a3mx
folder, or under `--save-dir`), shows its own save messages and questions, and leaves `a3mx.cfg` / `a3mx.dat` alone
([BUILDING.md](BUILDING.md#without-the-extensions)).

Both files are written whenever the game saves (AUTO SAVE is always on: leaving a mode or OPTION
MODE saves). `a3mx.cfg` is also written when you leave EX. SETTING or a P1 - P4 CONFIG screen, and
both are written when the program closes, so a setting is never lost.

`work/` holds only what can be made again from your disc, so it can be deleted at any time
(`--clean` empties it); your a3mx folder is never deleted by any tool. Back it up, or copy it to
another PC, to take your saves with you. `--save-dir DIR` keeps saves and settings together in DIR
and the log in `work/`, as before 0.60.0 (the tests use an empty one).

**From before 0.60.0**, saves and settings were in `saves/` beside `work/` (and before 0.44.0 in `work/`
itself). The first time 0.60.0 or later runs, it moves `a3mx.dat`, its `.bak`, `ms0/`, `import/` and
`export/` into your a3mx folder's `saves/`, and `a3mx.cfg` into its `configs/`, once (the log says
`saves: moved ...`). Anything already there is never replaced: a file in both stays where it was and
the log says so.

## First run

- With `a3mx.dat` missing, a save on the memory stick of an earlier version
  (`saves/ms0/PSP/SAVEDATA`) is imported once.
- With nothing at all, the game's defaults are used (with MODE SELECT on and everything unlocked).
- A `a3mx.cfg` on its own (no `a3mx.dat`) is applied on top of the defaults.

## OPTION MODE > SAVE LOAD

| Row | What it does |
|---|---|
| SAVE / LOAD | save or load at once (no questions, no screens): SYSTEM DATA SAVED / LOADED shows at the bottom |
| DELETE | puts the system data (options, button configs, rankings, Entry characters ...) and the project's own settings back to the defaults; the World Tour data is kept. The old `a3mx.dat` / `a3mx.cfg` are copied to `.bak` first. SYSTEM DATA DELETED shows at the bottom |
| IMPORT | reads the PSP saves in `saves/import` and uses them from now on. The current `a3mx.dat` and `a3mx.cfg` are copied to `a3mx.dat.bak` / `a3mx.cfg.bak` first |
| EXPORT | writes the saves to `saves/export`, as a PSP keeps them |

A PSP save is a folder: `ULUS100620000` (system data: options, rankings, Entry characters ...) and
`ULUS100620001` (World Tour and Edit Mode characters). To import, copy one or both folders from a
PSP's or PPSSPP's `PSP/SAVEDATA` into `saves/import` (putting the whole `SAVEDATA` or `PSP` folder there
works too). To use an export on a PSP or in PPSSPP, copy the folders from `saves/export` into its
`PSP/SAVEDATA`.

Exports are encrypted with the game's own key and carry a PARAM.SFO and the game's save icon, as the
PSP makes them (the method is PPSSPP's; neither PPSSPP nor a real PSP has been tried with them yet:
known issues E5 / E6).
Imports read encrypted saves (from a PSP or PPSSPP) and the unencrypted ones of earlier versions.

the project's own settings (EX. SETTING, L2 / R2, player 4) are not part of a PSP save: IMPORT keeps them as
they are, EXPORT leaves them out.

## Save messages kept off screen

Saves are silent: nothing to read, nothing to answer. The list below is every save message or question
the game has been seen to show, with where it comes from, how to see it, and whether it is silent yet.
Add to it as more turn up.

| # | When | What showed | Code | Silent? | How to see it |
|---|---|---|---|---|---|
| S1 | Back from a mode to the MODE MENU, the auto-save fails | "Do you wish to continue the game without auto-saving?" Yes / No | 0x0888D2B0 (`src/core/task/mode_menu.c`, `screen_from_mode`), the cursor 0x0888CC78 | Yes (0.24.0): goes on as Yes would, the failure in the log (`sysdata.c`) | `arcade_ending` scenario: `--save-fails`, and `--save-questions` to bring the question back |
| S2 | S1 answered No: the save is tried again and fails again | "Auto-save failed.", then S1 again | 0x0888D2B0 (`SAVE_ASK_RETRY`) | Yes, with S1 (never asked) | `arcade_ending` with `--save-questions`, frames 7500 - 7700 |
| S3 | Boot: the system data auto-loads | "Auto-loading SYSTEM DATA... Please do not turn off the PSP...", "Auto-load completed." | 0x0888BFBC (`src/core/task/title.c`), steps 1, 3, 4, 6 | Yes. Since 0.47.0 the screen shows, with the project's version (for example `V0.47.0`, or a nightly's `V0.47.0-NIGHTLY.20261106`) in the message's place, centred, instead of the message: `bootmsg.c` switches off the message's two pictures (objects of kind 0x88) and draws the version around the first object list (0x08837690), every frame, the load included. Before: kept off screen since the static recomp (0.24.0: `bootmsg.c` wraps the decompiled screen; 0.32.1: held through the load too) | any boot, frames 150 - 240 |
| S4 | Back from a mode, the save works | "Auto-saving SYSTEM DATA...", "Auto-save completed." | 0x0888D2B0, states 0 - 2 | Yes (since the static recomp, `sysdata.c`) | any mode's end |

Not save messages: TRY AGAIN and GAME OVER after an Arcade ending are the game's own end of a run.

A save can fail for real when the disk is full or `saves/` can't be written. `--save-fails`
(tests) makes every save fail, so S1 and S2 can be reached on purpose.
