# Display: VSYNC and FRAME TIMING

Two settings in **OPTION MODE > SCREEN SETTING**, below SCREEN SIZE and WALLPAPER, decide how each frame
reaches your screen. Neither changes how the game plays: moves, timings and the CPU behave the same
frame for frame whatever you choose. They only change when you *see* each frame.

| Setting | Values | Default | In `configs/a3mx.cfg` (`[screen setting]`) |
|---|---|---|---|
| VSYNC | ON, OFF | ON | `vsync=ON` |
| FRAME TIMING | AUTO, SMOOTH, ORIGINAL | AUTO | `frame_timing=AUTO` |
| FILTER | OFF and eleven shaders ([below](#filter)) | OFF | `filter=OFF` |

Both apply as soon as you change them, and are saved when you leave SCREEN SETTING. On a display whose
driver can't sync to the monitor at all, VSYNC is greyed out (the log says so); your saved setting is
kept. Both work the same with the software renderer as with the GPU one. DEFAULT sets them back to ON
and AUTO along with the game's own settings. For one run only, `--frame-timing auto`, `smooth` or
`original` on the command line overrides FRAME TIMING.

**Which one you're getting** is shown at the right end of the MODE MENU's bar: **ORIGINAL** in a red
box, **SMOOTH** in a blue one. If you chose SMOOTH or ORIGINAL, that is always what it says. With AUTO
it shows what AUTO picked.

**The speed never changes on its own.** SMOOTH is always 60.00 Hz and ORIGINAL always 59.94 Hz, on any
screen. AUTO picks one of the two when the MODE MENU first opens (it is ORIGINAL until then), and looks
again only when you come back to the MODE MENU or leave SCREEN SETTING; if nothing it chose from has
changed (VSYNC, the monitor's rate), it keeps its choice. It never changes while a menu is on screen
or during a fight.

## FILTER

SCREEN SETTING > FILTER (0.60.0) draws the finished picture through one of RetroArch's shaders. It is
part of the **Shaders** extension (the launcher's Extensions tab), on by default; with it off the row
isn't there.

| FILTER | What it is |
|---|---|
| OFF | the picture as it is drawn (the default) |
| CRT CURVED | crt-geom: a curved tube with scanlines and a shadow mask |
| CRT LOTTES | crt-lottes: bloom, a shadow mask, a slight curve |
| CRT ROYALE | crt-royale: the most detailed tube; heavy, wants a real GPU |
| CRT GUEST | crt-guest-advanced: a sharp arcade-monitor look |
| CRT LIGHT | zfast-crt: a light CRT for integrated GPUs and handhelds |
| SCAN LIGHT, SCAN STRONG, SCAN DARK | scanlines only, at three strengths |
| SCAN OSSC | scanlines as the OSSC line doubler draws them |
| XBRZ | xBRZ: smooth pixel edges |
| SCALEFX | ScaleFX: smooth pixel edges, truer to the pixel art |

It applies as you change it and is kept in `a3mx.cfg` (`[screen setting] filter=CRT GUEST`); DEFAULT
sets it back to OFF. `--filter crt-guest` chooses one for a single run without saving it.

**It needs two things that are not part of this project**, and without either the picture is simply
shown plain (the log and `last_run.txt`'s `filter=` line say which is missing and where it looked):

1. **The shaders:** libretro's slang-shaders. The launcher's **Dependencies** tab has a **Download the
   FILTER shaders** button (or `python tools/shaders.py`): the commit pinned in `tools/data/pins.json`,
   about 85 MB, into `.tools/slang-shaders/`, kept only if the presets match their SHA-256.
2. **The library:** [librashader](https://github.com/SnowflakePowered/librashader) (MPL-2.0),
   `librashader.dll` on Windows, `librashader.dylib` on macOS, `librashader.so` on Linux, in
   `.tools/librashader/` (or beside `a3mx.exe`). **A release's download has it already**: the
   pipeline builds it from librashader's source at the commit pinned in `pins.json`. From a clone,
   the launcher's **Dependencies > Build librashader...** builds it the same way, once (or
   `build.cmd --librashader`, or `python tools/librashader.py`). It needs [Rust](https://rustup.rs)
   1.88 or later; the launcher offers to install it (rustup, for your user only), and on Windows
   Microsoft's C++ build tools ("Build Tools for Visual Studio", "Desktop development with C++"),
   which it checks for and links to but doesn't install. A source folder you unpacked into `.tools/`
   (`librashader-*`) is used if it is the pinned one. It takes ten minutes or more; `--clean` keeps
   the result. You can also put a library you have there yourself. It is never downloaded
   ready-built.

FILTER needs the OpenGL window (the GPU renderer, or the software renderer shown through OpenGL).
The shader sees the picture at the game's own 480 x 272, so the scanlines are the game's rows
whatever Resolution is set to. Implementation: `src/platform/shader_gl.c`, the list in `shader.h`.
Tried by the maintainer on Windows with a library built locally (2026-11); not yet on Linux or macOS.

## VSYNC

- **ON** (recommended): each frame is handed to the monitor between refreshes, so the picture never
  tears.
- **OFF**: frames go out the moment they're ready. Input reaches the screen a few milliseconds
  sooner, but you'll see tearing: a line across the picture where the top and bottom come from different
  frames. On most screens it's very noticeable in a fighting game.

## FRAME TIMING

The PSP runs the game at **59.94** frames a second. Most PC monitors refresh at **60** Hz (or 120, 144,
...). The two don't divide evenly, and FRAME TIMING decides what to do about it.

### SMOOTH

The game runs at **60.00** frames a second. On a 60 Hz screen that's one frame per refresh, so no
frame is ever shown twice. On 120 Hz each frame is shown for exactly two refreshes, on 180 Hz three,
on 240 Hz four, and so on for any multiple of 60 up to 480 Hz.

- That's **0.1 % faster** than the PSP: one extra frame every 16.7 seconds, about 11 seconds over a
  three-hour session. You won't feel it, and the game's own timings (frames of startup, recovery, hit
  stun) don't change, because they're counted in frames, not seconds.
- The sound is played 0.1 % faster to match (1.7 cents: far below what anyone can hear), so it never
  drifts out of step or skips.
- With VSYNC ON and a screen at a multiple of 60 Hz, the monitor sets the pace: at 60 Hz each frame
  waits for the refresh; at 120 Hz and up the game times each frame to exactly two (three, four ...)
  of the monitor's refreshes (0.50.1). Anywhere else (VSYNC off, VRR, 75 / 144 / 165 Hz) a 60.00 Hz
  timer does, so you still get SMOOTH's speed, but the monitor may show some frames a refresh longer
  than others.

### ORIGINAL

Exactly **59.94** frames a second, as on the PSP.

- On a 60 Hz monitor, 0.06 refreshes a second are left over, so **one frame is shown twice about every
  16.7 seconds**: a small hitch. That isn't a fault, it's the arithmetic of 59.94 on 60.
- To get ORIGINAL timing without hitches, change the display instead:
  - **VRR** (G-Sync, FreeSync, "Adaptive Sync"): the monitor refreshes whenever a frame arrives, so
    59.94 is shown perfectly. Turn it on in your graphics driver and monitor; keep VSYNC ON in the game
    and play full screen.
  - A **59.94 Hz display mode**, if your monitor or TV offers one (Windows: Display settings > Advanced
    display > Choose a refresh rate, where it may be listed as 59.94 or 59 Hz). There the monitor sets
    the pace.

### AUTO (default)

Picks for your screen when the MODE MENU opens and when you leave SCREEN SETTING:

| Your screen | AUTO gives |
|---|---|
| 60, 120, 180, 240 Hz ... (a multiple of 60), VSYNC ON | **SMOOTH**, paced by the monitor |
| A 59.94 Hz mode (or 119.88 ...) | **ORIGINAL**, paced by the monitor |
| VRR at 60 Hz, or a swap that doesn't wait for the monitor | **ORIGINAL** (59.94 Hz timer: perfect with VRR) |
| VRR at 120 Hz and up | **SMOOTH** (since 0.50.1: smooth with VRR too; choose ORIGINAL for the PSP's speed) |
| 75, 144, 165 Hz ..., VSYNC OFF | **ORIGINAL** (59.94 Hz timer) |

If AUTO picked SMOOTH and the swap then turns out not to wait for the monitor, the game stays SMOOTH
on a 60.00 Hz timer until the next time AUTO looks, and AUTO is ORIGINAL from then on for that run.

Choose SMOOTH or ORIGINAL yourself to have that speed everywhere.

Settings saved by 0.48 and earlier said SMOOTH when nobody had chosen it (it was the default, and it
already fell back to ORIGINAL's timing where it couldn't keep in step). They're read as AUTO, which
does the same. A SMOOTH you choose from 0.49.0 on stays SMOOTH.

## Checking what you get

Play for a minute (a fight is best), quit, and open `logs/last_run.txt` in your a3mx folder ([SAVES.md](SAVES.md)):

| Line | Meaning |
|---|---|
| `frame_timing=` | the setting: AUTO, SMOOTH or ORIGINAL |
| `frame_timing_given=` | what you got, as the MODE MENU shows it: SMOOTH or ORIGINAL |
| `pacing=` | what was used: `the monitor, 60.00 Hz`; at 120 Hz and up `the monitor's rate, 120.00 Hz, each frame shown twice, timed by the clock`; or `timer, 59.94 Hz (...)` / `timer, 60.00 Hz (...)` with the reason |
| `vsync=`, `refresh_hz=` | VSYNC, and your monitor's rate as Windows (or macOS, Linux) reports it |
| `hitches=` | frames shown longer than they should be. SMOOTH at 60 or 120 Hz: none apart from loads (the boot, the screen before a fight, round starts). ORIGINAL at 60 Hz: about one per 17 s |
| `hitch_N=` | the 64 longest hitches, in time order: `seconds,frame,ms_on_screen,screen,fight_screen,held,disc_kb`. `screen` / `fight_screen` are the game's (GAME + 0 / + 2; in a fight `screen` is 2), `held` 1 when the display was being held through a load, `disc_kb` what was read from the disc since the frame before. A hitch with `disc_kb` above 0 or `held` 1 is a load; one in a fight with neither is the one to report |
| `shown_p99_ms=` | how long 99 % of frames stayed on screen: about 16.7 (17.0) at 60 and 120 Hz |
| `audio_ratio=` | how much faster sound is played (SMOOTH: about 1.0010 at 60 Hz; ORIGINAL: 1.0000) |

The launcher's bug report copies these into the issue for you
([REPORTING_ISSUES.md](REPORTING_ISSUES.md)).

## How it works

- The game's own clock never changes. The kernel (`src/platform/kernel.c`) still steps it by one PSP
  vblank, 1/59.94 s, per frame, so the per-frame hashes and every parity test are the same either way.
  The tests run with `--fast`, which doesn't wait for the wall clock at all.
- **ORIGINAL:** the kernel waits until 1/59.94 s after the previous frame. On Windows that wait uses a
  high-resolution timer and spins the last ~0.7 ms (0.35.3), so frames start on time.
- **SMOOTH's timer** (where the monitor can't set the pace): each frame is anchored to a 60.00 Hz grid
  (`rt_pace_anchor`, which takes a signed offset now) and the frame's timeline is scaled by 1000/1001.
- **AUTO** chooses SMOOTH or ORIGINAL at a decision point (`pacing_decide`: `padicons.c` when the MODE
  MENU opens, `screensetting.c` when SCREEN SETTING closes) and is then exactly that setting. SMOOTH is
  paced by the monitor only at a multiple of 60.00, ORIGINAL only at a multiple of 59.94; anywhere else
  each runs on its own timer. `pacing_given()` (the setting, or AUTO's choice; never the measured rate)
  feeds the MODE MENU's badge and `frame_timing_given=`. Before 0.60.0 the badge followed the measured
  rate, so SMOOTH on a 60 Hz monitor measuring about 59.97 Hz showed SMOOTH and ORIGINAL in turn.
  `ctest -R pacing_rules` (`tests/pacing_test.c`) checks these rules against simulated monitors.
- **Paced by the monitor** (`src/platform/pacing.c`):
  - The kernel squeezes each frame's timeline (every wait in it, not only the vblank) so that it ends
    about 4 ms before the monitor's next refresh is due (`pacing_time_scale`).
  - The swap then waits for the refresh, and the time it came back starts the next frame's clock
    (`rt_pace_anchor`).
  - Sound is sped up by the measured frame rate over 59.94 (`host_audio_ratio`,
    `SDL_SetAudioStreamFrequencyRatio`), checked every 120 frames.
  - The monitor's rate is checked every 60 frames. This is how it runs at **one refresh a frame**
    (60 Hz, 59.94 Hz), swap interval 1.
  - **At two or more refreshes a frame** (120, 180, 240 Hz ..., up to 8) it is "clocked" instead (0.50.1):
    a timer at exactly n of the monitor's reported refreshes (16.667 ms at 120.00 Hz, kept to the
    fraction of a microsecond so it doesn't drift), swap interval 1, so each frame goes out at the next
    refresh. Nothing is measured from the swap and nothing below this applies; `pacing=` says "the
    monitor's rate, 120.00 Hz, each frame shown twice, timed by the clock". 0.35.3 - 0.50.0 used a swap
    interval of n instead, which in full screen on a 120 Hz NVIDIA laptop made frames miss their refresh
    from the QSound logo until the fallbacks below gave up, up to 13 s.
  - The frame is ready at most 0.45 of a frame before its refresh (kept under a refresh with the
    desktop compositor's wait, which waits for the next refresh only).
  - A swap that doesn't wait (frames shown faster than the monitor refreshes, as when a driver forces
    vsync off) turns SMOOTH off until the window changes, with a warning in the log.
  - Frames that keep missing their refresh (more than 6 of 120, two checks in a row) turn SMOOTH off
    until the window changes (0.48.2).
- Held frames (loads and saves kept off screen) are always timed as ORIGINAL, since nothing is
  presented. So are the frames shown while the window settles (the first 120, and after each change
  between full screen and a window), since none of them is anchored to a refresh.
- Frame times and hitches: `src/platform/perf.c` ([BUILDING.md](BUILDING.md), `--perf-log`).
- Trying it without a real monitor: `A3MX_TEST_VBLANK_HZ=60` makes every swap wait for a simulated
  60 Hz vblank, and makes the monitor's rate read 60 Hz. With `xvfb-run -a` and `SDL_AUDIODRIVER=dummy`
  that runs anywhere. `A3MX_TEST_VBLANK_HZ=0` reports 60 Hz but doesn't wait, which tries the fallback.
  The simulated swap honours the swap interval; `A3MX_TEST_SWAP_INTERVAL_MAX=1` makes it ignore
  anything above 1, as some drivers do, and `A3MX_TEST_SWAP_FROM_CALL=1` counts the interval from the
  swap instead of from the last frame shown (as that laptop's driver did in full screen). Since 0.50.1 a
  swap interval above 1 is never asked for, so neither changes anything at 120 Hz: 1 hitch in 1500
  frames either way (0.50.0, from the call: 6, and every frame missed its refresh for 6 s). Run it `--windowed --renderer none`: the software renderer
  takes ~17 ms a frame on a small cloud machine, which hides the pacing.
  Compare `hitches` and `audio_ratio` in `logs/last_run.txt`, or the `shown_interval_us` column of
  `--perf-log`.
