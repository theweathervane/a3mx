# Controllers

## Bring your own device (0.60.0)

The launcher's **Controllers** tab sets, outside the game, which keys and which controller play each
of the four players. The game reads it when it starts.

- Pick a player (1 to 4) and its **Control Type**:

  | Control Type | The player plays with |
  |---|---|
  | **Automatic** (the default) | a controller no other player names, given out in the order they're plugged in (lowest automatic player first) with the usual layout; automatic player 1 also has the usual keys, unless another player has keys |
  | **Keyboard** | only the keys set here |
  | **Controller** | only the controller chosen here, with the buttons set here |
  | **Keyboard/Controller** | both |
  | **None** | nothing: the player can't play |

- **Controller:** choose one of the controllers plugged in. A controller belongs to one player;
  choosing it for another takes it from the first. The first time, its buttons get the usual layout,
  which you can change.
- Click a cell and press the key or button for it (Esc cancels). **x** unsets it. A stick or trigger
  can be a button too. The left stick always works as the d-pad as well. **Default keys** /
  **Default buttons** put the usual layout back.
- **Pad icon** shows the player's numbered bubble, as on the MODE MENU's bar; every type but None
  shows it.
- **Test** shows the pad icon beside each button while it is held, by a key or by the controller.
  Nothing is sent to the game.

All four players Automatic is the arrangement the game always had: the keyboard and the first
controller are player 1, the next controllers players 2 to 4. Mixing works too: player 1 on the
keyboard, player 3 on a chosen controller, 2 and 4 Automatic, and the next controller plugged in is
player 2, then player 4. Two players can't share a key.

The tab writes `configs/controllers.cfg` in your a3mx folder ([SAVES.md](SAVES.md)), one `type=` per
player; with no file every player is Automatic. A file from before the control types is read as
Keyboard, Controller or Keyboard/Controller from what it sets. In the game a player counts as plugged
in (its bubble, joining in as a human) while it can play: it has keys, or its controller is plugged
in. The game's log lists what it read (`controls:` lines).

This is separate from the in-game BUTTON CONFIG and PLAYER CONFIG ([PLAYER_CONFIG.md](PLAYER_CONFIG.md)),
which choose what each PSP button does in a fight; this tab chooses which key or controller button
*is* each PSP button.

The game reads the keyboard and up to four gamepads through SDL3. Any controller SDL3 knows as a
gamepad should work: Xbox / XInput pads, DualSense and DualShock 4, Switch Pro, and the many others in
SDL's controller database. So far it has been tested with four DualSense pads on Windows 11; reports
for other controllers are welcome.

## Who is which player

| Device | Player |
|---|---|
| Keyboard | 1 |
| Gamepad 1 | 1 (together with the keyboard) |
| Gamepad 2 | 2 |
| Gamepad 3 | 3 |
| Gamepad 4 | 4 (see [below](#player-4)) |

Gamepads are numbered by the player number each controller reports (its XInput slot, or the lit
player LED), then by when it was connected, with no gaps. Unplugging a pad moves the ones after it up:
take out gamepad 1 and gamepad 2 becomes player 1.

Pads can be plugged in and out at any time. The MODE MENU's bar shows a numbered bubble on the left
for each gamepad that is plugged in, with the player it is: **1** red, **2** blue, **3** green,
**4** yellow. Each keeps its place when another comes or goes. The right end of the bar shows the
frame timing you're getting, **ORIGINAL** (red) or **SMOOTH** (blue): [DISPLAY.md](DISPLAY.md).

## Buttons

| PSP | Keyboard | Gamepad | Xbox | PlayStation |
|---|---|---|---|---|
| D-pad | Arrow keys | D-pad or left stick | D-pad / left stick | D-pad / left stick |
| Cross | Z | South | A | Cross |
| Circle | X | East | B | Circle |
| Square | A | West | X | Square |
| Triangle | S | North | Y | Triangle |
| L / R | Q / W | Shoulders | LB / RB | L1 / R1 |
| L2 / R2 (the project's own) | E / R | Triggers | LT / RT | L2 / R2 |
| Start | Enter | Start | Menu | Options |
| Select | Backspace | Back | View | Create |

The left stick and the triggers count as pressed past about half way. What each button does in a
fight is set per player in OPTION MODE > PLAYER CONFIG ([PLAYER_CONFIG.md](PLAYER_CONFIG.md)), and from
the pause menu's BUTTON CONFIG.

Keyboard only: **F2** goes back to the MODE MENU at once without saving, **F11** or **Alt+Enter**
switches full screen. No gamepad button does these. To quit, close the game the way your system closes
any program: **Alt+F4** on Windows, **Cmd+Q** on macOS, **Ctrl+Q** on Linux, or the window's close
button. (Esc used to quit, and was too easy to press by mistake.)

## What a plugged-in pad does

- **Offline modes:** whoever starts a mode plays it. Gamepads 2 and 3 come in by pressing Start where a
  select screen shows JOIN, or through OPTION MODE > EX. SETTING > JOIN-IN (HUMAN). Mode by mode:
  [PLAYERS.md](PLAYERS.md).
- **Network modes on one PC** (COUCH CO-OP): gamepads 2 and 3 are PLAYER2 and PLAYER3; a pad that is
  not plugged in is played by the computer (CPU2 / CPU3).
- **Pause:** a player who plays a fighter pauses with their own Start, and the pause menu's BUTTON
  CONFIG then edits that player's buttons.

## Player 4

The game has three player slots, so gamepad 4 does not play yet: it shows its 4 bubble, and its buttons
can be set in OPTION MODE > PLAYER CONFIG > P4 CONFIG, ready for a 2v2 / X.Dramatic mode. It cannot
pause a fight ([KNOWN_ISSUES.md](KNOWN_ISSUES.md), E7).

## Tests

`--p2` / `--p3 human|cpu|auto` treat gamepad 2 / 3 as plugged in or not, and `--pads 1234` sets which
bubbles the MODE MENU shows, for headless runs without controllers.
