# Who plays which fighter

How a second or third pad gets into a game, mode by mode. Code: `src/extensions/hooks.c`
(`apply_local_players`, the select-screen join), `src/extensions/selectscreen.c` (JOIN / WAIT),
`src/extensions/playercfg.c` (whose buttons a fighter reads), `src/extensions/netplay.c` (network
modes).

## In short

There are two separate ways a pad gets a fighter, and they don't depend on each other:

1. **Joining in (pressing Start).** Wherever a mode lets another player join, a plugged-in pad
   that presses Start becomes a **human player**, on its own side, exactly as a second player would
   on the PSP. This works **whatever EX. SETTING says**. Pads that don't press Start stay out, and
   the computer plays the fighters it would have played on a PSP.
2. **JOIN-IN HUMAN (OPTION MODE > EX. SETTING).** This is only a way to **take over a fighter the
   computer would otherwise play**:
   - in modes where a pad *could* have joined but didn't (for example, it didn't press Start in
     Arcade, so the opponent is still the computer);
   - in modes where a pad *can't* join at all (World Tour, R.Dramatic Battle from the MODE MENU).

With **1P / 2P / 3P JOIN-IN all on CPU** (the default), every mode plays as on a PSP, except that
pads 2 and 3 can still join wherever the mode allows it.

## The EX. SETTING rows

| Row | Values (default first) | What it does |
|---|---|---|
| 1P JOIN-IN | CPU / HUMAN | Pad 1 (and the keyboard) takes over a computer fighter when it **didn't** start the mode (player 2 started it) |
| 2P JOIN-IN | CPU / HUMAN | Pad 2 takes over a computer fighter when it didn't start or join |
| 3P JOIN-IN | CPU / HUMAN | Pad 3 takes over a computer fighter when it didn't join |
| COUCH CO-OP | ON / OFF | The network modes: ON plays them on this PC with pads 2 and 3; OFF leaves the network stubbed, as if no wireless network were found |

A pad set to HUMAN that isn't plugged in changes nothing: the computer keeps the fighter. DEFAULT puts
the three JOIN-IN rows back to CPU and COUCH CO-OP to ON.

## Mode by mode

"Starter" is whoever chose the mode on the MODE MENU. "First free pad" and "second free pad" are the
pads that neither started nor joined, in order (P1 started: pads 2 then 3; P2 started: pads 1 then 3).

Rows marked **(not checked)** come from reading the code, not from a scripted run. Phase 10 of the
roadmap checks them.

| PSP mode | What the original PSP mode does | Join in with Start (pad plugged in) | With JOIN-IN HUMAN |
|---|---|---|---|
| **Arcade** | One player against the Arcade ladder; every opponent is the computer | **Yes**, in a fight: HERE COMES A NEW CHALLENGER. Both humans keep the character they chose (the PSP's challenger path had been cut and took the ladder's) | The opponent goes to the first free pad, for a player who didn't press Start |
| **World Tour** | One player travels the map, fighting computer opponents and levelling up their ISM | **No** | The opponent goes to the first free pad |
| **Survival:** Arcade, Original, Boss, Infinity | One player against a run of computer opponents on one health bar | Not offered by the project (not checked whether the game's own challenger path is reachable) | The opponent goes to the first free pad |
| **Survival:** Dramatic | You and a partner against computer opponents; the starter picks every box, the partner is the computer | **Yes**, on the select screen: the partner's box says JOIN; the first other pad to press Start picks the partner and plays it (the box says WAIT until its turn) | Partner to the first free pad, opponent to the second |
| **Survival:** Variable | A tag team (you and a partner) against computer opponents; the starter picks every box | **Yes**, on the select screen: JOIN / WAIT on the 2P box; the first other pad to press Start picks and plays the partner | Partner to the first free pad, next opponent to the second |
| **Survival:** R.Dramatic | One player against two computer opponents at once | **No** | The two opponents go to the free pads, in order |
| **Dramatic Battle** | You and a computer partner against a boss | **Yes**, player 2, on the select screen (an empty box says JOIN). Its box says WAIT until the starter has chosen, then player 2 picks the partner on pad 2 and plays it; the cursors take turns as on the PSP | Partner to the first free pad, boss to the second |
| **R.Dramatic Battle** | One player against two computer opponents at once | **No**. Another pad's Start pauses the fight instead (the game would otherwise have made it the opponent) | The two opponents go to the free pads, in order |
| **Variable Battle** | A tag team (you and a computer partner) against computer opponents | **Yes**, player 2, on the select screen, as in Dramatic Battle (JOIN, then WAIT, then picks the partner on pad 2) | Partner to the first free pad, next opponent to the second |
| **VS 100 Kumite** | One player against 100 computer opponents | Not offered by the project (not checked) | The opponent goes to the first free pad |
| **Final Battle** | One player against the final boss | Not offered by the project (not checked) | The opponent goes to the first free pad |
| **Free Battle:** Arcade | A single match; the starter picks both fighters, the opponent is the computer | **Yes**, on the select screen: the opponent's box says JOIN; the first other pad to press Start picks and plays the opponent | The opponent goes to the first free pad |
| **Free Battle:** Team | Team against team; the starter picks both teams, the opponent team is the computer | **Yes**, on the select screen: the opponent's team-size icons say JOIN; the first other pad to press Start picks the team size and characters and plays them | The opponent team goes to the first free pad |
| **Free Battle:** Variable | Tag team against an opponent; the starter picks every box | **Yes**, on the select screen: the partner's box (pad 1 or 2) and the ENEMY box (pad 3) say JOIN | Partner to the first free pad, opponent to the second |
| **Free Battle:** Dramatic | You and a partner against an opponent; the starter picks every box | **Yes**, on the select screen: the partner's box (pad 1 or 2) and the ENEMY box (pad 3) say JOIN | Partner to the first free pad, opponent to the second |
| **Free Battle:** R.Dramatic | One player against two opponents; the starter picks every box | **Yes**, on the select screen: the opponents' boxes say JOIN; pad 3 takes the 3P box, pad 1 or 2 the other | The two opponents go to the free pads, in order |
| **Training** | One player and a training dummy, which TRAINING's own options control | **No** | **No change**: the dummy stays TRAINING's |
| **Edit, Entry** | Menus (Edit has a test fight against the computer) | No (not checked) | Edit's test fight: the opponent goes to the first free pad (not checked) |
| **Option, Score Ranking** | Menus | n/a | n/a |
| **Network:** VS / TEAM / VARIABLE BATTLE | Two PSPs over the wireless link | COUCH CO-OP ON: played on this PC; pad 2 is PLAYER2, or the computer (CPU2) if no pad 2 is plugged in. OFF: the game finds no wireless network | Not used |
| **Network:** DRAMATIC BATTLE | Three PSPs: two players against one | COUCH CO-OP ON: pads 2 and 3 are PLAYER2 and PLAYER3, CPU2 / CPU3 when not plugged in; player 1 is always the host. OFF: no network | Not used |
| **Network:** LEAGUE, TOURNAMENT | Several PSPs in a league or a bracket | COUCH CO-OP ON: as DRAMATIC BATTLE (players 2 and 3). OFF: no network | Not used |

Network modes are emulated on one PC for now: the other PSPs are virtual players in the same program
(`vpeer.c`, `hle_net.c`). After a match the CONTINUE / SELECT / EXIT menu belongs to player 1 (the
host), as it did on the PSP. Real online play is phase 12 of the roadmap.

### Examples

- P1 starts Arcade, pad 2 is plugged in, JOIN-IN all CPU: the opponents are the computer until pad 2
  presses Start, then pad 2 is the challenger.
- P1 starts Arcade, 2P JOIN-IN HUMAN: pad 2 plays every opponent without pressing Start.
- P2 starts Arcade, 1P HUMAN: pad 1 plays the opponent.
- P1 starts R.Dramatic Battle, 2P and 3P HUMAN: pads 2 and 3 play the two opponents.
- P2 starts R.Dramatic Battle, 1P CPU, 3P HUMAN: of the two opponents, the first is the computer and
  the second is pad 3.
- P1 starts Dramatic Battle, pad 2 presses Start on the select screen, 3P HUMAN: pad 2 picks and plays
  the partner, pad 3 plays the boss.
- P1 starts Dramatic Battle, 2P CPU, 3P HUMAN, nobody presses Start: the partner is the computer, the
  boss is pad 3.

## The rule, exactly

`apply_local_players` (hooks.c) runs every frame, offline only:

1. A player who started the mode or joined with Start plays their own fighter, on the side the game
   gives them. The game records players present at 0x08E75910 + 0x35; the select screens of Survival
   Dramatic / Variable and Free Battle keep no record of a second player, so hooks.c keeps its own
   (`join_to`), cleared whenever a mode is chosen on the MODE MENU.
2. Every other fighter, in the game's order, goes to the next free pad.
3. That pad plays it if its JOIN-IN is HUMAN and the pad is plugged in (pad 1 always counts as plugged in:
   it has the keyboard). Otherwise the fighter stays the computer's.

Start joins only on a select screen that allows it, or through the game's own joins (Arcade's
new challenger, Dramatic and Variable Battle's 0x0883465C). Start during a fight is the pause.

A side can have more fighters than players (R.Dramatic Battle's two opponents and Dramatic Battle's
boss all say player 1). A fighter given to another pad gets that pad's player number at +2, so it reads
that pad's slot and button config. It gets its own number back when the pad lets it go.

## What else the project changes (whatever JOIN-IN says)

- Pads 2 and 3 feed the game's player 2 and 3 controller slots (the PSP had one pad), so a second pad
  can press Start to join wherever the game lets player 2 join.
- A player who joins late on a select screen can move their cursor (the PSP never had a second player
  there), except in Dramatic and Variable Battle, where the cursors take turns.
- Survival, Free Battle and Dramatic Battle: a human partner or Free Battle opponent picks their own
  character, with JOIN / WAIT boxes (on the PSP the starter picked every box).
- L2 / R2 are extra buttons ([PLAYER_CONFIG.md](PLAYER_CONFIG.md)); Start keeps pausing, from the pad of
  whoever plays the fighter.

## The check: unchanged with JOIN-IN on CPU

23 scripted runs, one per mode and sub-mode (pad 1 boots, chooses the mode, then presses Cross every 100
frames), give every fighter the same player number and computer flag as the game without the player
rules, and the same screen. With all three JOIN-IN on HUMAN, the same runs give the non-starting fighters
to pads 2 and 3 in order, as the table says, and leave Training's dummy alone.

Not covered yet:
- what happens after a match where a pad played an opponent (results, continue, the next fight);
- modes reached through a menu choice other than the first;
- which modes the game's own in-fight challenger reaches (the "not checked" rows above);
- plugging a pad in or pulling it out mid-mode.
