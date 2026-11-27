# Hyper Street Fighter Alpha and S-ISM: assessment (spike)

*Street Fighter Alpha Anthology* (PS2, 2006) hides a fourth take on Alpha 3: **Hyper Street Fighter
Alpha**, a Versus and Training game where each player picks a character's version from Alpha, Alpha 2,
Alpha 2 Gold or Alpha 3, and four hidden **S-ISMs** (Shadaloo-isms). This page says what is on the
Anthology disc, how Hyper works, and what it would take to bring its modes, music and stages into this
project as an optional extension.

> **Status: experimental, in the game** (EX. SETTING > EX. ANTHOLOGY, once the launcher has imported
> your disc; every setting OFF by default): T. Hawk's walk back, ALPHA SELECT, S-ISM SELECT with the S-ISM
> gauge, and S-ISM PRESENTATION (the S-ISMs' wipes). The
> versions and S-ISMs play by Alpha 3's rules until the rules they need are decompiled. See
> [ANTHOLOGY_EXTENSION.md](ANTHOLOGY_EXTENSION.md).

Spike tool: `tools/anthology.py` reads your own disc from `imports/ps2/`. Nothing from the disc is
committed.

```
python tools/anthology.py survey   # what is on the disc -> work/anthology/survey.json
python tools/anthology.py unpack   # unpack the game overlays -> work/anthology/x/
python tools/anthology.py music    # the 588 tracks, with loop points -> assets/us/anthology/music/
```

## The short answer

- **No PS2 decompilation is needed.** The PS2 program (`SLUS_213.17`) is the collection's shell. Each
  game is a separate overlay, and each overlay is a **static recompilation**: Capcom translated the CPS2
  game's 68000 program into C, one instruction at a time, and compiled it for the PS2 (the same idea as
  the static recomp this project started from). Hyper is the overlay `zero9_usa.bin`.
- **The data tables are already in this game's format.** The recompiled games keep the arcade's tables,
  converted to little-endian 16-bit values, and this game's fighter data uses the same layout: parts of
  A3MX's hit-box and attack tables for Ryu, Ken, Chun-Li, Dhalsim and Cammy appear byte for byte in the
  Anthology's Alpha 3 and Hyper overlays. So Hyper's character versions can be lifted as data rather than
  rebuilt by hand. Finding them, and the rules that use them, means reading recompiled MIPS code.
- **The music is done (H1).** All 588 tracks extract to OGG with their loop points.
- **Rules still depend on the battle core.** Chains, custom combos, push block, parry and super cancels
  sit in the fighters' and battle code, which is still mostly translated code here. The clean way is to
  decompile the functions each rule touches first, then add the rule as an extension around them.
- **No rules ahead of the decomp (the maintainer, 2026-11-27).** S-ISM D's "no supers" was a write of 0
  to the fighter's super meter (+366) every frame; it is off until the meter code is decompiled (a TODO in
  `anthology_select.c`'s `anthology_hud_gauges`), so S-ISM D keeps its supers for now. The S-ISMs' wipes
  into the fight are EX. ANTHOLOGY's S-ISM PRESENTATION (an S-ISM chosen); the I-ISM's stay with Extra ISM
  Presentation, each without the other. EX. ANTHOLOGY's rows only look for the imported files: nothing is loaded or taken
  from the game's memory until the extension is used.

## What Hyper is (from the footage, the maintainer and community documentation)

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

**Graphics.** Every version is drawn with the Alpha 3 sprites. The new art is the version labels, the
mode boxes, the S-ISM versus text, the per-version gauges, and whatever the VS-series moves need.

**Stages.** In Versus the stage follows the version picked (reported by the maintainer; still to be
confirmed in the code): Alpha 1 versions fight on the Great Wall of China, Alpha 2 and Alpha 2 Gold
versions on Venezuela (the waterfall), Alpha 3 versions on that character's Alpha 3 stage. Training uses
an Alpha 3 stage. So Hyper brings two stages this game doesn't have, not every Alpha 1 / 2 stage.

## What is on the disc (SLUS-21317)

| File | Size | What it is |
|---|---|---|
| `SLUS_213.17` | 572 KB | The shell: menus, CRI ADX player, the overlay loader and unpackers. Also names SLES-54085 (the PAL release) |
| `X_DATA.BIN` | 200 MB | AFS archive, 553 entries, no names |
| `Y_DATA.BIN` | 1.82 GB | AFS archive, 588 CRI ADX streams (48 kHz stereo) |
| `IOPRP.IMG`, `*.IRX`, `MODULE.BIZ`, `SYSDATA.BIZ`, `HDDRC*.BIN` | small | Sony modules and HDD-install support |

**Packing.** Entries that begin with a 16-bit word before `MWo3` or `MOMO` are packed with a halfword
LZ, the routine at 0x148698 in `SLUS_213.17` (it is how `MODULE.BIZ` is unpacked at boot):
16-bit flag words, most significant bit first. A clear bit copies one halfword. A set bit reads a u16 v:
length v >> 11 halfwords (0: the next u16 is the length), distance v & 0x7FF halfwords back; distance 0
writes `length` zero halfwords, and length 0 with distance 0 ends the stream. (A byte LZ sits next to it
at 0x148778.) `anthology.py unpack` does this.

**The overlays.** Each `MWo3` unpacks to a MIPS overlay with a small header (index, load address, end
address) and its name. The program headers of `SLUS_213.17` reserve the same sizes at the same addresses.

| X entry | Name | Loads at | Size | Game |
|---|---|---|---|---|
| 203 | `sub_usa.bin` | 0x1C8000 | 80 KB | The collection's own menus and messages |
| 206 | `zero1_usa.bin` | 0x1F0000 | 3.1 MB | Alpha ("1995,2006") |
| 224 | `zero2_usa.bin` | 0x1F0000 | 5.3 MB | Alpha 2 ("1996,2006") |
| 253 | `zero3_usa.bin` | 0x1F0000 | 6.6 MB | Alpha 3 ("1998,2006") |
| 348 | `zero4_usa.bin` | 0x1F0000 | 5.6 MB | an Alpha 2 variant ("1996,2006", arrange) |
| 350 | `zero5_usa.bin` | 0x1F0000 | 7.7 MB | an Alpha 3 variant ("1998,2006", staff roll): probably Alpha 3 Upper or Arrange |
| 526 | `zero6_usa.bin` | 0x1F0000 | 5.6 MB | Alpha 2 Gold (Zero 2 Alpha) |
| 536 | `zero7_usa.bin` | 0x1F0000 | 4.4 MB | Super Gem Fighter (Pocket Fighter) |
| 543 | `zero8_usa.bin` | 0x1F0000 | 5.5 MB | an Alpha 2 variant ("1996,2006", arrange) |
| 544 | `zero9_usa.bin` | 0x1F0000 | 9.5 MB | **Hyper** ("©2006 CAPCOM U.S.A., INC." as on its title screen, no staff roll, the largest) |

The variants' exact names are to be confirmed against the collection's menu. The rest of `X_DATA.BIN`:
TIM2 pictures (the menus), MOMO containers, packed CPS2 graphics chunks (20–35 per game; X[545–548], after
Hyper's overlay, are probably a few of its own on top of the Alpha 3 family's), QSound Z80 drivers (start F3 ED 56 31) and QSound
sample sets of 4 or 8 MiB.

**How the code looks.** Every 68000 instruction became a few lines of C: the 68000's registers live in
the PS2's scratchpad (0x70001100 onwards), condition flags are worked out with bit operations, and memory
accesses go through the recompiled program's own address space. It reads like the translated code in
`build/generated/`, so the project's habits (follow data, not instructions) carry over.

**The data matches this game's.** A3MX's per-character table `characters/<nn>/fighter/data/06_data_*.bin`
(hit boxes as signed 16-bit x, y, width, height; attack properties) compared in 16-byte blocks:

| Character | Blocks found in Alpha 3 (zero3) | in Hyper (zero9) | in Alpha (zero1) |
|---|---|---|---|
| Ryu | 187 / 1001 | 179 | 2 |
| Ken | 240 / 1205 | 242 | 3 |
| Chun-Li | 249 / 924 | 251 | 1 |
| Dhalsim | 143 / 1145 | 172 | 0 |
| Cammy | 332 / 1644 | 331 | 0 |

The longest identical run for Ryu is 1,576 bytes. The blocks that differ are what one would expect to
differ (MAX's balance changes, console additions, offsets). Alpha 1 and Alpha 2's own tables are not
copied into Hyper as they are, so Hyper's older versions were rewritten in the Alpha 3 format, which is
the format this game reads.

## Mapping Hyper (H0+, first pass)

**Code and data.** In each overlay the code comes first and the data after it: Alpha 3 (zero3) has code
up to 0x583C50 and 3.0 MB of data; Hyper (zero9) has code up to 0x5AF118 and 5.8 MB of data, so Hyper
carries about 2.8 MB more. Hyper's new data is not appended: compared in 256-byte cells, 2.2 MB of its
data is new, 1.4 MB is Alpha 3's and 2.4 MB is mixed, spread through every character's block.

**How the data is linked.** The translator turned the arcade's pointer tables into real PS2 addresses:
Alpha 3's data holds about 64,000 stored pointers to data and 67,000 to code. Inside a character's
block, sections are reached by 16-bit relative offsets as in the original 68000 data, so they are
rarely pointed at directly. 32-bit values are plain little-endian too: the PSP's speed records at
0x08A182C0 are at Alpha 3's 0x5918E0 unchanged, only shifted by 8 bytes of padding the PSP adds, so
more data is shared than the run count above says.

**Hyper keeps Alpha 3's tables.** Alpha 3 Ryu's tables sit in Hyper as one block, moved by +0x2F41C.
His hurt-box records (the boxes A3MX and Alpha 3 share) appear nowhere else in Hyper, so the versions
share the boxes (one set of sprites) and differ in their move and attack data and in code.

**The version flags: object +0xD3.** Hyper's objects have a flag byte at +0xD3 that Alpha 3 fills but
never reads (Alpha 3 copies it from a data record at 0x249FB4 and has no code that tests it): Hyper's
code reads it 1,457 times in 799 functions and writes it in 10 places. The bits tested, counted by function: 0x10 (392), 0x01 (262), 0x02 (165), 0x08 (144), 0x80 (90),
0x04 (87), 0x20 (38), 0x40 (12). Every one of those functions is where Hyper's rules differ from
Alpha 3's: [tools/data/hyper_d3_sites.json](../tools/data/hyper_d3_sites.json) lists them with the bits
each tests, which makes it the work list for H4.
- The routine at 0x568E30 picks one of four 48-entry handler tables (0x5C8450, 0x5C8510, 0x5C85D0,
  0x5C8690) by these bits, 0x10, 0x01 and 0x04 with the third table as the default, then calls the
  entry for the object's byte at +2. The four tables share almost no entries: version-specific
  behaviour, one table per version.
- The byte is set at 0x4B6C18 and 0x511BB0 from a table at 0x5C2490 indexed by the object's byte at
  +0x102 (groups of 0x10, 0x01, 0x22 / 0x2A and 0x06 / 0x0E), and changed elsewhere (0x08 and 0x80 are
  OR-ed in at 0x4B6CE0 and 0x4B6D18).
- **What the bits mean: confirmed from the running game** (PCSX2 save states of the maintainer's disc,
  2026-11-15; `python tools/anthology.py state FILE.p2s` reads them). The arcade's RAM (0xFF0000) is at
  0xB80000 in the PS2's memory, so the fighters' objects (0xFF8400, 0xFF8800) are at 0xB88400 and
  0xB88800. In each fighter, **+0x102 is a character-version number** and **+0xD3 its version flags**,
  looked up in the table at 0x5C2490 by +0x102 (via +0x117):

  | Version | +0x102 | +0xD3 |
  |---|---|---|
  | Alpha 3 | 0x00–0x2F (the character's number) | 0x10 |
  | Alpha | 0x30–0x3F (0x30 + character; the 13 Alpha characters are all below 16) | 0x01 |
  | Alpha 2 | 0x40–0x5F (0x40 + character) | 0x22 |
  | Alpha 2 Gold | 0x60–0x7F (0x60 + character) | 0x06 |
  | Alpha 2 / Gold, blue variant | seven slots per group (0x15, 0x16, 0x18–0x1C), e.g. Ryu's is 0x7B | + 0x08 |

  So the bits are: **0x10 Alpha 3, 0x01 Alpha, 0x02 the Alpha 2 family, 0x20 Alpha 2 (not Gold), 0x04
  Gold, 0x08 the blue variant** (the one chosen by holding Start on ALPHA2G; seven slots fit Zero 2
  Alpha's seven Classical versions, still to confirm). 0x80 is set by the pink S-ISM (below); 0x40 is
  not seen yet. **AUTO guard is not a flag bit:** in an Alpha Ryu vs
  Alpha Ken fight on AUTO, both fighters have +0xD3 = 0x01 as on MANUAL; the difference is **+0x118 = 1**
  (the AUTO switch; Hyper reads it 31 times, Alpha 3 already has it and reads it 5 times) and **+0x32F =
  10**, the AUTO count the HUD shows ("AUTO 10"). That fight was on the Great Wall with Ryu's music from
  Alpha (the maintainer's ear). In an Alpha 2 Ryu vs Alpha 2 Gold Ken fight on AUTO the count is 255 and
  the HUD shows "AUTO" with no number: Alpha 2's auto guard has no limit, Alpha's has ten. **That fight was
  on Venezuela**, as the maintainer reported, so the stage rule holds for Alpha (Great Wall) and Alpha 2 /
  Gold (Venezuela). Gold Ken is 0x61 (0x60 + Ken). **Which game's stages are used** is decided in code
  (found 2026-11-15): the routine at 0x2949E0 looks the fighter's +0x102 up in the byte table at 0x5B6220
  (0x00–0x2F → 2, Alpha 3; 0x30–0x3F → 0, Alpha; 0x40–0x7F → 1, Alpha 2 and Gold), doubles it and stores it
  at 0xFF8174 (the old 0xFF8100 kept at 0xFF8172); 0xFF8100 then holds it. In the fight states 0xFF8100 is
  **0 on the Great Wall, 2 on Venezuela and 4 in the Alpha 3 fights**, and 0xFF430E / 0xFF430F hold the two
  fighters' version numbers. The Great Wall fight with Alpha Ryu against Alpha 3 Ken has 0, so in that
  state the first player's version decides. Alpha 3's own code also reads 0xFF8100 (43 times, as an index
  into tables), so it is probably the arcade's stage-set index reused; how Alpha 3 versions then get their
  own stage is not traced yet. In the routine at 0x568E30 the four handler
  tables are therefore Alpha 3 (0x10), Alpha (0x01), Alpha 2 Gold (0x04) and Alpha 2 (the default).
- **Alpha 3's modes and the S-ISMs (four save states, 2026-11-15).** Ryu (Alpha 3) on an S-ISM against
  Ken (Alpha 3, A-ism) on NORMAL, MAZI, SAIKYO and CLASSIC. Ken shows that Alpha 3's modes are one byte
  each in the fighter's object: **MAZI +0xEC, SAIKYO +0xEB, CLASSICAL +0xF0** (each 1 when on; copies at
  +0x15A, +0x131 and +0x15E). CLASSICAL also sets +0x123, +0x132 and +0x162 to 0xFF and clears +0x11E.
  None of the plain Alpha 3 / Alpha / Alpha 2 states sets any of them.

  | Ryu's S-ISM | +0xD3 | mode bytes | +0x122 / +0x123 | +0x132 |
  |---|---|---|---|---|
  | none (plain A-ism, NORMAL) | 0x10 | none | 0x00 / 0x00 | 0x00 |
  | green | 0x10 | none | 0x82 / 0x04 | 0x00 |
  | blue | 0x10 | none | 0x02 / 0x03 | 0xFF |
  | red | 0x10 | none | 0x80 / 0x02 | 0x00 |
  | pink | **0x98** | **+0xF0 = +0x15E = 0x81** | 0x00 / 0xFF | 0xFF |

  **The code that writes them (found 2026-11-15)** agrees with the save states:
  - **0x2891A0** picks one of eight choices from a bit mask (0x7F8, by a random number; probably for the
    computer or a random pick) and writes the bytes. The writes per choice: MAZI +0x15A = 1; SAIKYO +0x131 = 1;
    CLASSICAL +0x132 = -1, +0x15E = 1; **red** +0x132 = 0, +0x122 = 0x80; **blue** +0x132 = -1, +0x122 = 0x02;
    **green** +0x132 = 0, +0x122 = 0x82; **pink** +0x132 = -1, +0x15E = 0x81, then back to +0x15E = 1 (plain
    CLASSICAL) unless the character's bit is set in **0x13218C13**. That mask's bits are characters 0x00,
    0x01, 0x04, 0x0A, 0x0B, 0x0F, 0x10, 0x15, 0x18, 0x19 and 0x1C: Ryu, Ken, Chun-Li, M. Bison, Sagat,
    Dhalsim, Zangief, Balrog, E. Honda, Blanka and Vega in A3MX's numbering: the SF2 fighters. So only they
    can be pink, as the footage says. The test uses +0x102 & 0x1F, so Guile (0x20 in A3MX's numbering)
    would read Ryu's bit; whether Hyper has him there isn't checked.
  - **+0x132 is the ISM** (-1, 0, 1 as in A3MX's select screen; 0 is A-ism). It fits: green (A-ism moves)
    and red have 0, blue (X-ism moves) and pink have -1, and CLASSICAL forces -1.
  - **0x295F40 sets +0x123 from +0x122**: 3 if +0x122 is positive, 2 if negative, 4 if negative with bit
    0x02, which gives green 4, blue 3, red 2. Without an S-ISM it comes from a table at 0x5B62A0, and
    CLASSICAL makes it 0xFF. Alpha 3 already has the field (36 reads).
  - **0x4B6BE0 sets +0xD3** from the table at 0x5C2490 by +0x102, then, if bit 0x08 isn't set already and
    +0xF0 is negative (pink), ORs in **0x08 and 0x80**; whenever 0x08 ends up set it also sets +0xB5 = 1 (1 in
    the pink state). So **0x08 is the Classical / Champion Edition version bit**: the seven Alpha 2 "blue" slots have
    it from the table, and the pink S-ISM adds it. 0x80 marks the pink S-ISM.

  Red (VS-series moves) sets no mode byte: its marks are +0x122 = 0x80 and +0x123 = 2 only. What reads
  +0x122 / +0x123 to change the moves and the gauge is the H4 work (30 and 267 accesses in zero9 against
  5 and 36 in Alpha 3). `tools/anthology.py state` prints all of these.
- **The Alpha Ryu fight is on the Great Wall**, as the maintainer reported; Alpha Ryu's HUD has no guard
  bar and a three-level gauge.
- **Per-version tables.** Several 128-entry pointer tables are indexed by the character-version number.
  - **The move and attack data (found 2026-11-15).** Tables at **0x6A6BD4, 0x6A6DD4, 0x6A6FD4 and
    0x6A7A94**, and at **0x6AD414 and 0x6AD814 / 0x6ADA14 / 0x6ADC14 / 0x6ADE14**, point at each version's
    own blocks. For Ryu:

    | Table | Alpha 3 | Alpha | Alpha 2 | Alpha 2 Gold |
    |---|---|---|---|---|
    | 0x6A6BD4 | 0x6AF9CC | 0x8B2514 | 0x8DD2C8 | 0x8DEA6C |
    | 0x6A7A94 | 0x6B3C10 | 0x8AB9E0 | 0x8CA370 | 0x8D3380 |
    | 0x6AD814 | 0x5FB670 | 0xB159D0 | 0xB3B640 | 0xB5CF90 |
    | 0x6AD414 | 0x5FDDC4 | 0xB17AE4 | 0xB3DCDE | 0xB5F62E |

    The blocks have the same shape in every version (offset headers, then records with hit boxes such as
    -6, 83, 9, 8). Alpha 3 Ryu's 0x6A6BD4 block is A3MX's `06_data` from +0x3790 to +0x48C0 (about 220 of
    its 16-byte rows found there), with Hyper's records 0x28 bytes where the PSP file has 0x20. The Alpha 3
    entries' targets in Alpha 3 itself are pointed at from per-character tables at 0x67FAF8 / 0x67FB78 /
    0x67FBF8 / 0x67FC78 (32 entries), so Hyper widened those tables to 128 entries, one per version. Ryu's
    blue variant (0x7B) shares Gold's blocks in most of these tables but has its own in 0x6A7A94 (0x8D33E0).
    **These tables are where the converter (H5) starts.**
  - The four side by side at 0x75C478, 0x75C67C, 0x75C880 and 0x75CA84 are **not** move data, as first
    thought: each record is a short list of sprite numbers with a flip bit (0x40), symmetric end to end
    (Alpha's has 12, Alpha 3's 2). That looks like a HUD strip drawn per version, the gauge frame most
    likely (not checked). The blue variant points at an empty default (0x75C474).
  - 0x67E1E0 and 0x6A57D4 give every version of a character the same entry: data the versions share.

## Music (H1, done)

`anthology.py music` writes `assets/us/anthology/music/y000.ogg` … `y587.ogg` and `index.json`
(channels, rate, length, loop). 297 tracks loop (the music, about 347 minutes); their loop is kept as
`LOOPSTART` / `LOOPLENGTH` tags in samples, from the ADX v4 header (start at 0x28, end at 0x30). The
other 291 are short one-shots. Every file's length was checked against its ADX header. About 786 MB and
some minutes to convert; a run can be resumed.

**Which game plays which track (found 2026-11-15).** The shell's `sub_usa.bin` (X[203], loaded at
0x1C8000) has the routine 0x1CA3E0: Y = table[game][command], with nine tables by game at 0x1D4CF0.
`anthology.py music-tables` reads them from the disc into
[tools/data/anthology_music_tables.json](../tools/data/anthology_music_tables.json):

| Game | Commands | Y tracks |
|---|---|---|
| 1 (Alpha) | 13 | 2–14 |
| 2 (Alpha 2) | 21 | 16–35 |
| 3 (Alpha 3) | 32 | 36–64 |
| 4 | 29 | 66–93 |
| 5 | 38 | 94–128 |
| 6 | 31 | 129–157 |
| 7 | 12 | 158–169 |
| 8 | 24 | 170–192 |
| 9 (Hyper) | 124 | Alpha 3's commands first, then Alpha's, Alpha 2's and game 4's lists, a few of games 5 and 8 |

Game 9's list starts with Alpha 3's commands and then holds the other games' lists, which is what
Hyper needs, so it is Hyper's. Games 1–3 fit by size (Alpha's 13 characters) and order. Games 4–8 most
likely follow the overlays zero4–zero8 (their headers number them 5–9, one more than the game), but
which game is which isn't checked. **404 of the 588 tracks are in no list** (0, 1, 15, 34, 56, 65, 84,
147, 188 and 193–587): the short one-shots and whatever else the collection plays (menus, other modes),
so the extension needs only the tracks Hyper's list names. There are still no titles: the collection
draws its menus as pictures.

## Character versions: the converter (H5, started 2026-11-15)

`anthology.py versions` reads Hyper's overlay from the disc and writes, for every character version,
`assets/us/anthology/characters/<nn>_<name>/<version>/`: `palette.bin` (A3MX's palette layout),
`palette.png` (a preview), `raw/<table>.bin` (the version's own blocks, as Hyper stores them) and
`version.json`; `assets/us/anthology/versions.json` lists every version number. It takes about 25 s.

**The tables.** zero9 widened Alpha 3's per-character tables (32 entries from 0x67D678 in zero3) to 128
entries indexed by the version number. Those with a block per version:

| zero9 | zero3 (Alpha 3) | Holds |
|---|---|---|
| 0x6452E0 | 0x641270 … (Ryu's) | the colours: rows of 15 CPS2 colours (0RGB, 4 bits a channel) and a pad word |
| 0x6A6BD4 | 0x67D678 | hit boxes (x, y, w, h) and attack records: `06_data` entries 6–10 |
| 0x6A6DD4 | 0x67D6F8 | `06_data` entry 11, copied as it is |
| 0x6A6FD4 | 0x67D778 | small records: `06_data` entry 12 |
| 0x6A7A94 | 0x67DA78 | the animations (20-byte frames): `06_data`'s frame table and entries 1–5 |
| 0x6AD414, 0x6AD814, 0x6ADA14, 0x6ADC14, 0x6ADE14, 0x6AE614 | 0x67F9F8, 0x67FAF8, 0x67FB78, 0x67FBF8, 0x67FC78, 0x67FE78 | Alpha 3 data: 0x67F9F8's is `06_data` entries 15 and 16; the rest isn't in `06_data` (other files) |
| 0x75C478 … 0x75CA84 | none | HUD sprite strips |

The same area also has 128-entry tables of code addresses (0x6A59D4 …): the version-specific routines,
which are H4's.

**The versions on the disc: 69.** Unused numbers point at copies or at Alpha 3 Ryu's blocks. By the
flags table (0x5C2490) and which blocks each shares:
- **Alpha, 13:** 0x30–0x3C, Alpha 1's roster (Ryu to Dan).
- **Alpha 2, 25:** 0x40–0x51 (Ryu to Gen), and 0x52 (shares Chun-Li's moves), 0x53 (Gen's), 0x54 (its
  own), 0x55 and 0x56 (Classical, flags 0x2A: Zangief's and Dhalsim's moves), 0x57 (Ryu's moves, its own
  colours; A3MX's number 0x17 is Evil Ryu) and 0x5D (Sakura's moves).
- **Alpha 2 Gold, 31:** 0x60–0x74, 0x77 and 0x7D as in Alpha 2 (23), the Classical seven at 0x75, 0x76
  and 0x78–0x7C (Zangief, Dhalsim, Bison, Sagat, Ken, Ryu, Chun-Li: the CE fighters) and 0x7E (its own).
The converter names a slot after the character whose blocks it shares, and puts the ones that share
nothing in `characters/extra/` (0x54, 0x74, 0x7E) until the select screen code says who they are.

**Colours, checked.** Hyper's rows convert to A3MX's layout exactly: each row of 16 starts with the
transparent colour (taken from A3MX's file for the character), then the 15 colours as ABGR1555 with
each 4-bit channel doubled. Against A3MX's own palettes for the 32 Alpha 3 characters, 666 of 960 rows
match at the CPS2's precision. The rows come in sets of five (one set per button colour) and **rows 0–2
of every set match for all 32 characters**; rows 3 and 4 differ in A3MX for most (MAX's or the PSP's).
Alpha's versions have 20 rows (four sets), Alpha 2's and Gold's 30.

**What A3MX's `06_data` is (corrected 2026-11-15).** The decomp already reads this file: the fighter
set-up 0x0886EB48 fills a fighter's data pointers with `unpack_offsets` (0x0886E638,
`src/core/fighter/state.c`), whose comment gives the layout. The first attempts here sliced the file
from the wrong base and are replaced by this:
- **Header, 0x208 bytes:** word 0 = 0, word 1 = the size of the data that follows, then **64 offsets
  at +8** and **64 counts at +0x108**. The offsets count from the end of the header (file + 0x208).
  A count of 0: the entry is a table at that offset. -1: a list of indices (little-endian words, -1 at
  the end), each picking a 10-byte record from the start of the data (base + 10 × index). Over 0: that
  many words, each naming another entry.
- **Who reads which entry:** 0x0886EB48 is called by each character's own set-up (37 callers) with a
  list of 21–22 entry numbers, one per field of the fighter's data block (+0xC8, +0x308, +0x548,
  +0x788, +0x9D4 … +0x9FC, +0x10 … +0x88, +0x9C8). The lists are almost the same for every
  character: entries 1–14 always go to the same fields; only the last few (21–23) move. Entries 22–48
  are read elsewhere.
- **Where each entry comes from (32 Alpha 3 characters against zero3, by content):**

  | Entry | Count | Source in zero3 (per-character tables) | Match |
  |---|---|---|---|
  | data start | | 10-byte animation frames: Capcom's own format | none (rewritten) |
  | 1–4 | -1 | lists of frame numbers (one list per animation set) | none (rewritten) |
  | 5 | 0 | 12-byte records split out of the arcade's 20-byte frames | none (rewritten) |
  | 6–9 | 0 | 0x67D678's block: the hit-box lists 1–4 (block + 24 onwards) | exact for 26–30 of 32 |
  | 10 | 0 | 0x67D678's block: list 5, the attack records | MAX's balance values differ |
  | 11 | 0 | the whole 0x67D6F8 block | exact for 21 of 28 found |
  | 12 | 0 | the whole 0x67D778 block | exact for 30 of 32 |
  | 13 | 0 | the 0x67FCF8 block | exact for 24 |
  | 14, 20, 21 | 3 / 0 / 42 | lists naming entries; a 1,152-byte table of 15-bit colours | none |
  | 15, 16 | 0 | parts of the 0x67F9F8 block | mostly exact |
  | 17, 18, 19 | 0 | the 0x67FEF8, 0x67FD78, 0x67FDF8 blocks | exact for 25–31 |
  | 22–45 | 0 | 0x67D878's block: 33 records of 8 bytes each; x, y and flags as the arcade's, the fourth halfword the arcade's / 20 | 19,214 of 20,097 records |

- **The animation frames, the one real conversion (worked out on Ryu, 2026-11-15).** In the arcade,
  0x67DA78's block (Ryu's 0x6C4940) starts with three groups of four pointers, one group per ISM. The
  four pointers are slices of one array of 16-bit offsets (Ryu: 246 of them; the ISMs differ only in
  the fourth slice). Each offset is an animation: 20-byte frames (u16 duration, u16, u32 pointer to the
  sprite, then 12 bytes). The last frame has bit 0x8000 in its duration and is followed by a u32
  pointer to the frame it loops to (a one-frame or non-looping animation has no pointer).
  The PSP writes every animation's frames, in order, into the 10-byte table at the start of the data:
  `u16 duration, u16 (the arcade's +2), u16 sprite, s16 step, u16 tail`. The sprite is the arcade pointer
  numbered by address among all of the character's sprite pointers (Ryu: 0x6CB32A → 0, 0x6CB348 → 1,
  0x6CB368 → 2 …). The tail is an index into entry 5, which holds each distinct 12-byte arcade tail
  once, numbered in order of first use. step is the arcade's +8 on ordinary frames; on the last frame it
  is the loop as a relative jump (-5: back five frames to the start). Entries 1–4 list, for each
  animation of the four slices, the number of its first frame (-1 at the end). The sprite numbering
  still has to be checked for every character, and the sprite records themselves are in another file
  (the fighter data entries, not `06_data`).


**Hyper's Alpha 3 blocks are not Alpha 3's.** Hyper laid out even the Alpha 3 versions' tables again,
larger: Ryu's hit-box list has 64 boxes in Alpha 3 and A3MX and 114 in Hyper (room for the S-ISM and
VS additions). So the extension keeps A3MX's own data for Alpha 3 characters and takes only the 69
versions above from Hyper.

**Hyper's records are the arcade's with more in them:** each list keeps Alpha 3's entries first and adds
its own after them (Ryu's four box lists are 912, 784, 592 and 136 bytes in Hyper, 512, 512, 384 and 88
in Alpha 3), and the attack records are 8 bytes longer. So a version's `06_data` needs Hyper's records
narrowed to the arcade's size as well.

**H5b, the frame conversion checked on all 32 Alpha 3 characters (2026-11-15).**
`python tools/anthology_d06.py check` builds the frames and the copied entries from zero3 (read from
the disc) and compares them with A3MX's own `06_data` files:
- **Frame numbers:** the PSP numbers animations in order of first reference, slice by slice (the
  block's four pointers in order go to entries 1, 3, 2 and 4), each animation once; an animation that
  starts inside another one that is referenced shares its frames; a last frame is one with 0x8000
  (a loop pointer may follow) **or 0x4000** (no loop). For Ryu every animation start matches except the
  two that MAX added (the arcade has no animation there); likewise for Akuma, Adon, Sodom, Birdie, Rose,
  Bison, Cammy, Honda and Karin. Where MAX inserted animations, every later number moves, so the count
  over all 32 is 4,405 of 7,064 starts.
- **Frame contents:** 5,177 of 7,026 animations match frame for frame (duration, the second halfword,
  the advance and the 12-byte tail); the rest look like MAX's changed moves (not checked one by one). The advance is 1 on an ordinary
  frame (not the arcade's +8, which is part of the tail), 0 on a last frame, and the loop as a jump back.
- **Sprites:** a frame's sprite is the arcade sprite record's number in the character's run of records
  (u16 2, u16, u16 n, padding to 4, u32 tiles, (n + 1) × 4 bytes): exact for Ryu, Akuma and Sodom; for
  others the run has gaps or MAX's extra records, so the numbering has a few steps.
- **Copied entries, characters byte-exact:** 6: 22, 7: 20, 8: 24, 9: 27, 10: 0 (MAX's balance), 11: 18,
  12: 24, 13: 17, 17: 24, 18: 27, 19: 27 of 32.

**The sprites are the real problem for the versions.** Hyper's versions don't use Alpha 3's sprite
records: Alpha, Alpha 2, Gold and Classical Ryu's frames point at records of their own (226–364 of
them), and none is byte-identical to a record Hyper's Alpha 3 Ryu uses, though 125–184 share the tile
data pointer. Hyper's Alpha 3 Ryu records map one to one onto zero3's (335, by lining up the same
animations), but nothing lines the older versions' records up with A3MX's. Two ways on:
1. **Match by picture:** draw each Hyper sprite record from the Anthology's tiles and each of A3MX's
   sprites, and pair them by image. The versions then use A3MX's own sprites, as the footage suggests
   they look the same. Needs the Anthology's tile data found and decoded, and A3MX's sprite format
   (not decoded yet; PAC_FORMAT.md lists it as to do).
2. **Bring Hyper's sprites in:** convert Hyper's records and tiles into a sprite set of the extension's
   own. More data, and the engine has to draw from a second set.
Either way this is more than the 2–4 sessions estimated for H5; the size depends on that choice.

**Hyper's tables for entries 13 and 17–19:** zero3's 0x67FCF8, 0x67FD78, 0x67FDF8 and 0x67FEF8 are Hyper's
0x6AE014, 0x6AE214, 0x6AE414 and 0x6AE814 (the 32-entry tables became 128-entry ones at four times the
spacing). Their Alpha 3 entries match zero3's for 27–32 of 32 characters, and 24–80 version entries
have their own blocks, so `anthology.py versions` needs 0x6AE014, 0x6AE214, 0x6AE414 and 0x6AE814 added.

**Next (H5b):** write zero3 → `06_data` for the copied entries (6–13, 15–19, 22–45) and check it on the
32 Alpha 3 characters; then the frame walk (data start, 1–5) and entries 14, 20, 21; then the same from
Hyper's blocks for each version, Alpha Ryu first. The `raw/` blocks run to the next address any version
table points at, so they are upper bounds, for that work only.

## T. Hawk's walk back (the first import, 2026-11-24)

A proof of the whole way, disc to game, for one asset. The PSP's T. Hawk (`pl23`) never got his walk-back
art: his walk back (set 1, animation 3) shows pictures 51 down to 40, and those twelve are copies of the
walk forward's 28 - 39, so he walks backwards by playing his walk forward in reverse. Hyper Street
Fighter Alpha's T. Hawk (version 0x23) has a walk back of its own.

**Where it is (US disc).**
- The animation: zero9's block for version 0x23 (`0x6A7A94` + 4 x 0x23 = 0x8A20D0), its first slice
  (0x8A2100), animation 3 at 0x8A24C8: 12 frames of 6, sprite records 0x8A6A04 ... 0x8A66F0 (the walk
  forward, animation 2, is 0x8A638C ... 0x8A66A4).
- A sprite record: u16 2, u16, u16 n, padding to 4, u32 its position list, n + 1 parts of 4 bytes:
  u16 tile, u8 flags (0x20 flipped across, 0x40 down), u8 size (low nibble columns - 1, high nibble
  rows - 1; tiles are tile + column + 16 x row, CPS2 blocks).
- The position lists: X_DATA[547] (halfword LZ, loaded at 0xC84000; zero9's records point into
  0xC84000 - 0xD0B140): s16 x, s16 y for each part from the sprite's origin. Alpha 3's are X_DATA[289]
  (from 0xA127D8).
- The tiles: per-character banks. A bank: +0x14 u32 count, +0x18 0x30, +0x30 u32 offsets (bit 31:
  packed). A tile is 16 x 16 4-bit pixels, 8 bytes a row, the left pixel in the low nibble, pen 15
  transparent. Packed: a flags byte for each eight tokens, the highest bit first; 0 a literal; 1 a byte
  hl, h = 0 repeating the last byte l + 2 times, otherwise l + 2 bytes copied from h + 1 back. T. Hawk's
  bank is **X_DATA[386]** (9,248 tiles), in the run 355 - 386 of character banks after zero5; Alpha 3's
  own are 258 - 285 (Ryu 258). The 65,536-tile banks (225, 257, 286, 537) and others are stages and
  effects (not checked).
- Pen p is the PSP's colour p + 1 in palette 0: Hyper's walk forward drawn this way is the PSP's
  pictures 28 - 39, 10 of 12 identical and 32 pixels of retouches in the other two. So the PSP's
  pictures are the arcade art, cut into 32-pixel cells and re-packed.

**The pieces.**
- `tools/anthology_hawk.py` writes the twelve as the PSP's pictures 40 - 51 (160 x 160 indexed PNGs,
  origin 80, 141) to `assets/us/anthology/hawk_walkback/`, with `walkback.json`; it checks the walk
  forward against the PSP's first when `work/` is there.
- `tools/build.py --anthology-iso ISO` runs it once per disc image. The launcher's Paths tab (Images)
  has Show optional images; ticked, a PS2 ISO row checks the Anthology disc as the PSP ISO row does
  (serial and program SHA-1 from SYSTEM.CNF, then the whole image; only SLUS-21317's are known, other
  Anthology discs are named by region and not imported from) and passes it when it checks out.
- `src/extensions/anthology.c` (feature `anthology`, on by default, Extra): when T. Hawk's list (a CRC of
  its first 52 pictures) is drawn at pictures 40 - 51, the game is handed a picture of ours with the
  same columns, rows and origin, and our cells fill the picture cache (wraps 0x0880E150 and 0x0880EFB8).
  Nothing in the archive changes. EX. SETTING > EX. ANTHOLOGY (the Anthology's page, with the extension; page 6/6 before 0.167.0): T.HAWK WALKBACK, ON by default, greyed and shown
  OFF without the files (the 4P rows' grey).

What this says for H5: Hyper's sprites can be brought in as pictures and cells of our own beside a
fighter's archive (the sprites question's option 2) without touching the PSP's data, and the decoding
for any character version is the same three files: zero9's records, the position lists and that
character's bank (which bank goes with which version is still to map: for now by looking).

## For the PSP decomp

The PSP game is Capcom's C rewrite of Alpha 3, not the same translation, so the Anthology's code can't be
lifted into this project and it can't serve as a parity reference. Its data can help:

- **Shared tables.** 400 runs (28,175 bytes) of the PSP program's `.data` are byte-identical to the
  arcade data in `zero3_usa.bin`; the 73 runs of 64 bytes or more are listed with both addresses in
  [tools/data/anthology_arcade_tables.json](../tools/data/anthology_arcade_tables.json). Three of them are
  tables the decomp already reads (0x08A180C8 and 0x08A18218, the speed tables of 0x08863168, and
  0x08A1FD5C, in the largest run); the other 70 mark where arcade tables start and end before the
  decomp reaches them.
- **A second reading of a routine.** The arcade code that reads a shared table is the same logic written
  out one 68000 instruction at a time. It reaches these tables through stored offsets, not constants, so
  pairing routines needs that worked out first (1–2 sessions).
- **The fighter struct.** X at +0x12 and Y at +0x16 are where the whole-number halves of 68000 32-bit
  positions at +0x10 / +0x14 land in little-endian; if the PSP kept the arcade's layout, arcade RAM maps
  would name its fields. Not verified.

## How it would fit this project

The detailed design (feature flag, launcher, extraction, select screen, tests, order of work) is in
[ANTHOLOGY_EXTENSION.md](ANTHOLOGY_EXTENSION.md); in short:

- **Extension, optional, region data.** "Hyper" is a new mode under `src/extensions/` (off with
  `--no-extensions`), shown only when an Anthology disc has been extracted. The disc goes in
  `imports/ps2/` (already ignored); extracted files go under `assets/us/anthology/` by what they are
  (music, screens, characters/<nn>/versions/...), mapped through `tools/asset_layout.py` and `index.json`
  like the rest. Nothing extracted is committed.
- **Data, not code, for versions.** A character version is a set of tables (moves, animations, hit boxes,
  damage, gauge rules) that points at the Alpha 3 sprites this game already has. Hyper's are in the same
  format as this game's, so they become files in `assets/`, read by a loader that swaps a fighter's tables
  at select time. New art (labels, gauges, VS effects) is PNG.
- **Rules as hooks.** Chains, custom combos, auto guard, push block, parry and super cancels are switches
  read in the decompiled functions that decide what a fighter may do next (`SFA_FUNC_WRAPPED`), so with the
  extension off every frame is unchanged.
- **Parity of a new kind.** Hyper has no PSP original, so its tests compare move data and frame data with
  the Anthology version run in an emulator (startup, active and recovery frames, damage, stun, gauge
  gain), recorded once per move as reference files.
- **Timing.** CPS2 runs at about 59.64 Hz, this game at 60. Frame counts carry over.

## Work plan and rough sizes

Sizes are in working sessions like the ones so far.

| | Work | Needs | Size |
|---|---|---|---|
| H0 | Investigation: unpack the overlays, name them, find Hyper, compare the data with this game's | nothing | **done** |
| H0+ | Map Hyper's data: version flags, handler tables, AUTO guard, Alpha 3's mode bytes, the S-ISM code, the stage-set choice, each version's data tables and the music tables **found**; left: how the Alpha 3 versions' own stage is picked, and the H4 / H5 detail | H0 | about done |
| H1 | Music with loop points | nothing | **done**; names 1–2 |
| H2 | New art: CPS2 tiles to PNG (known format), cut out the labels, mode boxes, S-ISM text, per-version gauges | H0+ | 1–2 |
| H3 | Select screen: the version list, ALPHA2G, MANUAL / AUTO, the Start + left / right S-ISM input, the versus screen's version text | H2; select screen native (it largely is) | 2–3 |
| H4 | Version and S-ISM rules: Alpha 1 chains and gauge, Alpha 2 custom combos, auto guard, green chains / push block / Alpha counter, blue parry / super cancel, the single-level Shadaloo gauge | battle core and fighters native where each rule hooks | 6–12 |
| H5 | Move sets: a converter that lifts each version's tables from zero9 into `assets/`. **Started:** `anthology.py versions` writes the 69 versions' palettes (A3MX format, checked) and raw blocks; next, the `06_data` writer (format known now, from the decomp's `unpack_offsets`): the copied entries, then the animation-frame walk, then a `06_data` per version, then checks in the emulator | H0+; H4 | 2–3 for the `06_data` writer (frames checked on 32 characters; Hyper's records are wider), plus the sprites: matching Hyper's sprite records to A3MX's by picture, or bringing Hyper's sprites in (size open), then about 0.3 per version to check |
| H6 | ~~The Great Wall and Venezuela stages~~ **Out of scope** (the maintainer, 2026-11-26: the spike is the modes and the Alpha versions only). Every version fights on the character's Alpha 3 stage | | |

## Ways not taken

- **Running Hyper's overlay or the CPS2 game in an emulator inside this game.** Quickest to show, but it
  is emulation, not decompilation, and the two engines couldn't share a fight.
- **Rebuilding versions from wiki frame data alone.** No disc reading, but no way to match the original
  frame for frame; unnecessary now that the tables can be lifted.
- **Decompiling the PS2 shell.** Only its unpacker (done) and its music tables are needed.

## Legal

As with the PSP disc: you supply your own copy, it is read in place, and nothing from it is committed or
shipped. The Anthology's data is extracted at first run like the rest of `assets/`.

## Sources

- The maintainer's footage of Hyper's version select and the S-ISM selection, and their notes on stages
  (2026-11-14).
- [Hyper Street Fighter Alpha, SuperCombo Wiki](https://wiki.supercombo.gg/w/Hyper_Street_Fighter_Alpha):
  unlocking, the S-ISMs, versions per character.
- [Street Fighter Zero: Fighters Generation, StrategyWiki](https://strategywiki.org/wiki/Street_Fighter_Zero:_Fighters_Generation):
  S-ISMs use the Shadaloo gauge with one level.
- `python tools/anthology.py survey --hash`: SHA-1 `906b23e96a591d0bdc87201dacdef1ff7c875494` for
  SLUS-21317 v1.01.
