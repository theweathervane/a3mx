# Hyper Street Fighter Alpha and S-ISM: assessment (spike)

*Street Fighter Alpha Anthology* (PS2, 2006) hides a fourth take on Alpha 3: **Hyper Street Fighter
Alpha**, a Versus and Training game where each player picks a character's version from Alpha, Alpha 2,
Alpha 2 Gold or Alpha 3, and four hidden **S-ISMs** (Shadaloo-isms). This page says what is on the
Anthology disc, how Hyper works, and what it would take to bring its modes, music and stages into this
project as an optional extension. It is a spike: nothing here is wired into the game.

Spike tool: `python tools/anthology.py survey` (and `music`) reads your own disc from `imports/ps2/`.
Nothing from the disc is committed.

## The short answer

- **No PS2 decompilation is needed for the game logic.** The PS2 program (`SLUS_213.17`, 572 KB) is
  Capcom's CPS2 emulator and menus. Hyper itself is a CPS2 arcade program (Motorola 68000 code plus
  graphics and QSound data) run by that emulator. What we would read is 68000 code, and Hyper is built
  on the Alpha 3 arcade program, which is also on the disc. Diffing the two finds what Hyper added.
- **The music is easy.** The 588 tracks are CRI ADX, which ffmpeg already decodes.
- **The fighters are the big job.** The Alpha 1 / Alpha 2 / Alpha 2 Gold versions, the CE versions (pink
  S-ISM) and the VS-series moves (red S-ISM) are new move data per character, written for the CPS2
  program's formats. They have to be translated into this game's formats, character by character.
- **It depends on the battle core.** Rules such as chains, custom combos, push block, parry and super
  cancels sit in the fighters' and battle code, which is still mostly translated code here. The clean way
  is to decompile the functions each rule touches first, then add the rule as an extension around them.
- **Recommendation:** do the investigation (H0) and the music (H1) now; they don't touch the game. Plan the
  rest as a roadmap phase 14 track after the battle core and fighters are native (phase 3).

## What Hyper is (from the footage and community documentation)

Unlocked in the US release by beating Alpha 3, then every game in the collection; then hold Select on
Street Fighter Alpha in the collection's menu. Versus and Training only, no Arcade.

**Select screen (per player).** After the character, a list: ALPHA, ALPHA2, ALPHA2G (shown in blue),
ALPHA3.
- ALPHA, ALPHA2, ALPHA2G go on to MANUAL / AUTO (guard), as in Alpha 1 and 2.
- ALPHA3 goes on to the four mode boxes, NORMAL (green), MAZI (blue), SAIKYO (pink), CLASSIC (red), then
  the ism choice (A / X / V) as in Alpha 3.
- **S-ISM:** on a mode box, hold Start and press left or right until the highlight disappears, then any
  button. The versus screen then reads "S-ISM" under the name. All four S-ISMs use the Shadaloo gauge with
  one super level (the boss gauge this game already has).

| Version | What changes |
|---|---|
| Alpha | Alpha 1 move set; Alpha 1 gauge and rules (chain combos, auto guard choice) |
| Alpha 2 / 2 Gold | Alpha 2 move set and custom combos; Gold is the later revision |
| Alpha 3 | As Alpha 3, with NORMAL / MAZI / SAIKYO / CLASSIC (this game already has these) |
| S-ISM green (NORMAL) | A-ism move set; ground and air chains, push block (F + PPP in block stun), Alpha counter with a DP motion + PPP / KKK |
| S-ISM blue (MAZI) | X-ism move set; super cancels (special into super) and parry (forward / down at impact) |
| S-ISM pink (SAIKYO) | The 12 original SF2 fighters as their Champion Edition selves: high damage, no supers. Others can't pick it |
| S-ISM red (CLASSIC) | VS-series moves for some characters (Shin Bison, Guy's teleport, Ken / Akuma / E.Ryu air fireballs and VS animations, Sakura's teleport and Shun Goku Satsu, Ryu's Shin Shoryuken, Charlie's air Flash Kick, Dhalsim's air fireball, Dan) |

Versions per character: Alpha 1 for 13 characters (Adon, Akuma, Birdie, Charlie, Chun-Li, Dan, Guy, Ken,
M. Bison, Rose, Ryu, Sagat, Sodom); Alpha 2 / Gold for those plus Dhalsim, Evil Ryu, Gen, Rolento, Sakura
and Zangief (19); Alpha 3 for all 33. Hyper has no Eagle, Ingrid, Maki or Yun.

**Graphics.** In the footage every version is drawn with the Alpha 3 sprites, and the stages are Alpha 3's.
The new art is the version labels, the mode boxes, the S-ISM versus text, the per-version gauges, and
whatever the VS-series moves need (larger fireballs and so on).

## What is on the disc (SLUS-21317, `python tools/anthology.py survey`)

| File | Size | What it is |
|---|---|---|
| `SLUS_213.17` | 572 KB | The PS2 program: CPS2 emulator, the collection's menus, CRI ADX player. Also names SLES-54085 (the PAL release) |
| `X_DATA.BIN` | 200 MB | AFS archive, 553 entries, no names |
| `Y_DATA.BIN` | 1.82 GB | AFS archive, 588 CRI ADX music streams (48 kHz stereo) |
| `IOPRP.IMG`, `*.IRX`, `MODULE.BIZ`, `SYSDATA.BIZ`, `HDDRC*.BIN` | small | Sony modules and HDD-install support |

`X_DATA.BIN` by kind:

| Kind | Entries | Notes |
|---|---|---|
| TIM2 pictures | 206 | The collection's menus and gallery |
| MOMO containers | 41 | Collection data; 314–346 look like per-character art |
| MWo3 containers | 10 | The 68000 programs, packed: `b_usa.bin` and `ro1_usa.bin` … `ro9_usa.bin` (each file's name is readable near the start of its packed data) |
| QSound Z80 drivers | 6 | Start F3 ED 56 31, with the driver's version string |
| QSound samples | 6 | 4 MiB (Alpha 1, 2, 2 Gold) or 8 MiB (Alpha 3 and its variants, Pocket Fighter) |
| other | 284 | Packed CPS2 graphics chunks (about 20–35 per game), small tables |

The entries come in one block per game: program, graphics chunks, Z80 driver, samples.

| Block | Program | Z80 driver | Samples | Probably |
|---|---|---|---|---|
| X[206–222] | ro1 | 1.05A | 4 MiB | Alpha |
| X[224–251] | ro2 | 1.05C | 4 MiB | Alpha 2 |
| X[253–288] | ro3 | 1.71 | 8 MiB | Alpha 3 |
| X[348–389] | ro4, ro5 | 1.71 | 8 MiB | Alpha 3 variants: Alpha 3 Upper / Arrange and **Hyper** |
| X[526–533] | ro6 | 1.06b | 4 MiB | Alpha 2 Gold (Zero 2 Alpha) |
| X[536–541] | ro7 | 1.30 | 8 MiB | Super Gem Fighter (Pocket Fighter) |
| X[543–544] | ro8, ro9 | (shared) | (shared) | Arrange variants, using another block's data |

The ro4 / ro5 block uses the same Z80 driver version as Alpha 3, and 10 of its graphics chunks are
byte-identical to Alpha 3's, which fits Hyper being Alpha 3 with additions. Which of ro4, ro5 and ro9 is
Hyper is the first thing H0 confirms (ro5 is the likeliest: the largest Alpha 3-family program).

## How it would fit this project

- **Extension, optional, region data.** "Hyper" is a new mode under `src/extensions/` (off with
  `--no-extensions`), shown only when an Anthology disc has been extracted. The disc goes in
  `imports/ps2/` (already ignored); extracted files go under `assets/us/anthology/` by what they are
  (music, screens, characters/<nn>/versions/...), mapped through `tools/asset_layout.py` and `index.json`
  like the rest. Nothing extracted is committed.
- **Data, not code, for versions.** A character version is a move set (commands, animations, hit boxes,
  damage, gauge rules) that points at the Alpha 3 sprites this game already has. Hyper's versions become
  files in `assets/`, read by a loader that swaps a fighter's move tables at select time. New art (labels,
  gauges, VS fireballs) is PNG.
- **Rules as hooks.** Chains, custom combos, auto guard, push block, parry and super cancels are switches
  read in the decompiled functions that decide what a fighter may do next (`SFA_FUNC_WRAPPED`), so with the
  extension off every frame is unchanged.
- **Parity of a new kind.** The existing tests compare against the PSP game. Hyper has no PSP original, so
  its tests compare move data and frame data against the Anthology version run in an emulator (startup,
  active and recovery frames, damage, stun, gauge gain), recorded once per move as reference files.
- **Timing.** CPS2 runs at about 59.64 Hz, this game at 60. Frame counts carry over; only the music's
  speed against the action differs, which nobody will notice.

## Work plan and rough sizes

Sizes are rough and in working sessions like the ones so far; the unknown that matters most is answered
by H0.

| | Work | Needs | Size |
|---|---|---|---|
| H0 | Investigation: unpack MWo3 (find the unpacker in `SLUS_213.17`, a small MIPS routine), name ro1–ro9, unpack the graphics chunks, diff Hyper's 68000 program against Alpha 3's, and compare one character's move data in the CPS2 program with `pl00`'s in this game | nothing | 2–4 |
| H1 | Music: `anthology.py music` with loop points (ADX headers carry them), name the 588 tracks (game, stage or screen, original or arranged) from the PS2 program's tables, extract to `assets/us/anthology/music/`, and an option to play Anthology tracks for this game's stages | nothing | 1–3 |
| H2 | New art: CPS2 tiles to PNG (known format), cut out the labels, mode boxes, S-ISM text, per-version gauges | H0 | 1–2 |
| H3 | Select screen: the version list, ALPHA2G, MANUAL / AUTO, the Start + left / right S-ISM input, the versus screen's version text | H2; select screen native (it largely is) | 2–3 |
| H4 | Version and S-ISM rules: Alpha 1 chains and gauge, Alpha 2 custom combos, auto guard, green chains / push block / Alpha counter, blue parry / super cancel, the single-level Shadaloo gauge | battle core and fighters native where each rule hooks | 6–12 |
| H5 | Move sets: a converter from Hyper's data to this game's, then per character version: 13 Alpha 1 + 19 Alpha 2 + 19 Alpha 2 Gold + 12 CE + about 10 VS changes, each checked against the emulator | H0's format answer; H4 | 1 per character version, about 70 in all |
| H6 | Stages from Alpha 1 and 2 (not part of Hyper): render their CPS2 background layers to PNG and play them through a data-driven stage renderer | H0, H2; stage drawing native (phase 6) | 4–8 |

H5 is most of the work. If H0 finds that this game's move data is close to the CPS2 program's (Capcom's
ports usually kept the arcade tables), the converter does most of it and the per-character cost drops a
lot; if not, each version is rebuilt by hand from the emulator's frame data.

## Ways not taken

- **Running Hyper's 68000 program in an emulator inside this game.** Quickest to show, but it is an
  emulator, not a decompilation, the two engines couldn't share a fight, and it would ship nothing the
  project wants to keep.
- **Rebuilding versions from wiki frame data alone.** No disc parsing, but no way to match the original
  frame for frame.
- **Decompiling the PS2 program.** It is the emulator and the collection's menus; none of Hyper's rules
  are in it. Only its MWo3 unpacker and its music tables are needed.

## Legal

As with the PSP disc: you supply your own copy, it is read in place, and nothing from it is committed or
shipped. The Anthology's data is extracted at first run like the rest of `assets/`.

## Sources

- Your footage of Hyper's version select and the S-ISM selection (2026-11-14).
- [Hyper Street Fighter Alpha, SuperCombo Wiki](https://wiki.supercombo.gg/w/Hyper_Street_Fighter_Alpha):
  unlocking, the S-ISMs, versions per character.
- [Street Fighter Zero: Fighters Generation, StrategyWiki](https://strategywiki.org/wiki/Street_Fighter_Zero:_Fighters_Generation):
  S-ISMs use the Shadaloo gauge with one level.
- `python tools/anthology.py survey --hash`: SHA-1 `906b23e96a591d0bdc87201dacdef1ff7c875494` for
  SLUS-21317 v1.01.
