# Changelog

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
- **Bring your own device (BYOD):** The Controllers tab maps keyboard keys and a controller to each of 4 players outside the game, with a Test button. Player 1 has the keyboard by default; until you change something the game still assigns devices automatically, see [docs/CONTROLLERS.md](docs/CONTROLLERS.md).
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