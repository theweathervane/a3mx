# The .pac archives

Almost all of the game's data is in 313 `.pac` archives in `PSP_GAME/USRDIR/usa/data/` (151 MB on the
US disc): characters, stages, screens, fonts, sound banks, voices, text. `tools/extract_assets.py`
unpacks each into its own folder under `assets/us/`, placed by what it holds ([assets/LAYOUT.md](../assets/LAYOUT.md)); `tools/pac.py` reads, unpacks and packs them; the game's
own reader is decompiled in `src/core/loader/pac.c`.

## Layout

| Offset | Size | What |
|---|---|---|
| 0x000 | 4 | entry count |
| 0x004 | 4 | data size: the file is 0x800 + this many bytes |
| 0x008 | 8 × count | per entry: `u32 type`, `u32 size` |
| ... | | zero up to 0x800 |
| 0x800 | | the entries, in header order, each starting on a 2 KB (0x800) boundary |

All little-endian. Every archive on the disc follows this exactly (checked on all 313). The bytes
between an entry's end and the next 2 KB boundary are **not** always zero: Capcom's packer left stale
buffer contents there. The game never uses them, but `tools/pac.py` keeps them (`.pad` files) so an
unchanged archive packs back byte for byte (`tests/pac_roundtrip.py`, a ctest).

## The type word

`type & 0xFFFF0000` is the **kind**: one flag saying what the entry is and where the loader puts it.
The low 16 bits are slot / page numbers for that kind.

| Kind | Entries | What | Where the loader puts it (`pac.c`) |
|---|---|---|---|
| 0x10000000 | 845 | textures: PowerVR header (`GBIX` + `PVRT`), pixels linear | 130 KB staging buffer; texture page `(type >> 8) & 0xFF`, slot `type & 0xFF`; uploaded to the GPU's texture list. A `GBIX` index picks the VRAM address |
| 0x04000000 | 148 | palettes (16-bit colours) for the 4 / 8-bit textures | palette table `[type & 0xFF]` |
| 0x20000000 | 41 | sound: SAS bank (`PPHD` header + samples) and MIDI sequences (`MThd`) | a buffer from the sound library (bit 7 set: a sequence), then registered with it |
| 0x40000000 | 176 | voices: SAS banks (`PPHD` + samples) | as sound, bank from a table by `type & 0xFF` |
| 0x00010000 | 209 | text: bios, win quotes, ending scripts (ASCII with control bytes) | text table `[type & 0xF]` |
| 0x00000000 | 344 | misc data: stage and screen layouts, tables | data table `[slot + (type & 0xFF)]` |
| 0x80000000 | 2 | data | data table `[(type >> 8) & 0xFF]` |
| 0x01000000 | 197 | data (indexes / layouts for the entries that follow) | table `[type & 0xFF]` |
| 0x02000000 | 170 | data (sprite / animation sets) | table `[type & 0xFF]` |
| 0x00020000 | 252 | small tables (endings) | table `[type & 0xF]` |
| 0x00100000 / 0x00200000 / 0x00400000 | 43 each | fighter data in `pl*` (0x00200000 is the big one: 386 KB - 1.4 MB, sprites / animation) | the fighter heap at 0x08E87940, one pointer per fighter slot |
| 0x00800000, 0x08000000 | 0 | (handled by the loader, in no archive) | texture flagged 0x20000000 / table `[(type >> 8) & 0xFF]` |

## Textures

Each texture entry is a Dreamcast-style PowerVR file: an optional `GBIX` block (global index), then
`PVRT`: `u32 length, u8 pixel_format, u8 data_format, u16 0, u16 width, u16 height`, then the pixels.
On the PSP the pixels are **linear** (row by row), not twiddled as on the Dreamcast.

| Data format | Pixels | Entries |
|---|---|---|
| 0x01 | 16-bit: pixel format 0 ABGR1555, 1 BGR565 (the PSP's 5551 and 565: red in the low bits) | 489 |
| 0x05 | 4-bit indexed | 343 |
| 0x07 | 8-bit indexed | 13 |

### The indexed textures' palettes

An indexed texture holds colour indices; its colours are chosen when it is drawn, and nothing in the
archive says from where. `tools/pair_palettes.py` found out by watching the game draw: the renderer
records each indexed texture with the colours it is drawn with (`--palette-census`), across the test
scenarios, a Free Battle on every stage and Arcade with every character slot to its ending; the
colours are then found in the disc's files. The result is `tools/data/palette_pairs.json`
(references only: archive, entry, byte offset, or a program address). What it showed:

- Palettes are in three kinds of place: palette entries (kind 0x04000000, often in another archive),
  **the stage layout** (each stage's data entry 0 ends with its palettes: 16 colours a row), and **the
  program itself** (the menus' palettes, in its data).
- A texture uses one palette throughout (no sheet mixes rows by region), but many are drawn with
  several in turn: **palette animation** (lights, water, neon). The pairing keeps the colours drawn
  most often.
- Index 0 is always drawn transparent, whatever the palette holds there.
- Screens that fade keep their fade steps as palette data (down to plain black), so those match too;
  the pairing passes over the darker, flatter candidates and keeps the full colours.

297 of the 356 indexed textures are paired and extracted in colour (an 8-bit palette PNG with the
texture's own palette); the rest, never drawn in the census, stay grey (the grey level is the index):
the screens the census does not reach: `wall` (9, a character collage), `ake` (11, effects), the title variants (`title_u` 4-8), `open_u` 24-25 and `netd_u` 0 (ad-hoc play); and the endings of the characters whose Arcade run does not reach them (Sodom, Rose, Dhalsim, Balrog, Cammy, Evil Ryu, Blanka, R. Mika, Juli, Guile, Fei Long, Dee Jay, T. Hawk, and slots 0x14 and 0x25: 44 textures, some of these runs end in a fight the health hold does not cover).

## Who loads what

Two loaders, both decompiled (`src/core/loader/`):

- `pac_load` (0x08808424, `pac.c`) loads an archive by number from the name table at 0x08A03140, all
  at once. In the test scenarios: `efct`, `zett`, `ake`, `sign`, `se_mid`, `ascii`, `vram`, `pit_u`,
  `opt_u`, `memory_u` and the character intros `op*_u`.
- The loader thread (`async.c`) loads queued archives (and plain files) in the background, one step
  a frame: characters (`pl*`), stages (`st*`), the title, MODE MENU, World Tour. It routes entries
  as `pac_load` does, except that it does not handle kind 0x00020000, hands every sound and voice
  bank to the sound library through a callback, puts fighter data in a second table per fighter too,
  and lets a plain file (queued with a kind) land where it was asked to.

Kinds the scenarios have not taken through `pac_load` yet: 0x00000000 and 0x00020000 (both in the
ending archives, reached only at the end of Arcade mode). The fighter, 0x00800000, 0x08000000 and
table-voice kinds are handled by `pac_load` but occur in none of the archives it loads.

## Still to work out

- The few indexed textures the census never sees drawn (see above).
- The data kinds' structures (0x00000000, 0x01000000, 0x02000000, 0x00020000, fighter data).
- The text encoding's control bytes.
- `PPHD` banks to WAV (VAG samples), MIDI sequences with them.
