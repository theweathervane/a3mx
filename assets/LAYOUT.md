# How `assets/` is laid out

The game's data is unpacked by what it is, not by its file name on the disc: Ryu's fighter is in
`characters/00_ryu/fighter/`, not `pac/pl00_u/`. Someone painting Dan's stage, redrawing Cammy's
win screen or replacing the title should find it without knowing the disc. This page is the rule
book. Keep to it when adding anything, and when decompiling something that names what an asset is.

The map from disc archives to folders lives in one place, [tools/asset_layout.py](../tools/asset_layout.py).
The extractor writes the result to `assets/<region>/index.json`, and the game reads that file. No
folder name is written in the code, so a folder can be moved by changing its line in the index.

## The tree

```
assets/
  README.md, LAYOUT.md             committed
  extensions/                      committed: new art that isn't the game's (see below)
    stages/0c_dan/widescreen_left.png
  us/  jp/                         extracted from your own disc, never committed
    index.json                     which folder holds which archive (what the game reads)
    manifest.json                  what was extracted
    characters/<nn>_<name>/        one per character slot, 00 – 29, in the game's order
      fighter/                     pl<nn>  sprites and animation, palettes, moves, voices
      fighter_r/                   pl29r   (INGRID only: a second fighter archive)
      intro/                       op<nn>  the pre-fight intro
      win/                         win<nn> the win screen and quotes
      ending/                      end<nn> the Arcade ending
      bio/                         bd<nn>  the bio
      ranking/                     rk<nn>  per-character ranking data
    stages/<nn>_<name>/            st<nn>  one per stage, 38 of them
    screens/<name>/                title, select, mode_menu, option, save_load, world_tour ...
    effects/<name>/                shared effects
    fonts/<name>/
    sound/<name>/                  sound effects bank, the title's voices
    music/                         the 51 music tracks
    other/<disc name>/             archives not identified yet
```

The whole map (313 archives on the US disc) is printed by `python tools/asset_layout.py`.

## Inside an archive's folder

Each folder holds one archive from the disc: its `manifest.json`, and each entry's files in a
subfolder for its kind:

| Subfolder | Entries | Files |
|---|---|---|
| `textures/` | textures | `.pvr` as on the disc, `.png` to paint |
| `palettes/` | palettes | `.bin`, `.png` (16 colours a row) |
| `text/` | text | `.bin`, `.txt` |
| `sound/`, `voices/` | sound and voice banks, MIDI sequences | `.phd` / `.bin` banks, `.mid` (WAV to come) |
| `fighter_data/` | the three fighter data blocks | `.bin` |
| `data/` | every kind not decoded yet | `.bin` |

File names keep the entry's number, kind and type word (`07_texture_10000B00.png`). They're the only
thing known about an entry until its use is decoded, and they keep the folder's order the same as the
archive's. A `.pad` file beside an entry holds the stale bytes the disc had after it, so the archive
can be rebuilt exactly. Never edit those.

## Naming

- **Lower case, words joined by `_`, no punctuation**: `evil_ryu`, `shin_akuma`, `world_tour_screen`.
- **Characters and stages start with their number in hex**, two digits, as the game counts them
  (`0c_dan`, `1f_juni`). The folders then sort in the game's order, and slots that share a name stay
  apart.
- **Use the game's own names** (the US disc's: `bison`, `balrog`, `vega`), from its tables
  ([docs/STAGES.md](../docs/STAGES.md)), not ones we make up. Stages are named as FREE BATTLE's stage row
  names them (`ryu_1`, `ryu_2`, `battle_3`).
- **The same paths in every region.** `us/` and `jp/` have the same tree; the Japanese disc's
  archive names (no `_u`) map to the same folders.

Not identified yet:
- Slots 0x12 (CHUN-LI), 0x13 (GEN), 0x14 (SODOM) and 0x25 (BALROG) repeat an earlier slot's name but
  have their own fighter data. They're variants, so they keep the game's name and their own number
  until we know more.
- The archives under `other/` (`ake`, `zett`, `sign`, `vram`, `wall`).
- The screens named after their disc names (`network_d`, `network_s`, `pit`).

## When decompiling

Decompiling is how we learn what things are. When the code shows what an archive or an entry is for:

1. **Rename it, in one place.** Change its line in `tools/asset_layout.py` (move an archive out of
   `other/`, give a screen its real name). Note the evidence in a comment, such as the function that
   loads it and the screen it draws.
2. **Name entries once their use is known**, in the same file's tables, not in the extractor. A
   per-archive list of entry names (`portrait`, `select_icon`) goes beside the archive's map line, so
   one change reaches the extractor, the tests and the docs.
3. **New decoders write into the archive's own folder**, in a subfolder for what they produce: a
   fighter's sprite frames from `fighter_data/` into `characters/<nn>_<name>/fighter/sprites/`, a
   sound bank as WAV into `sound/` beside the `.bin`. Never into a new top-level folder of their own.
   Metadata (frame lists, offsets, timings) goes in a `metadata.json` there.
4. **Never write a path into C code.** The game finds folders only through `index.json`. If the game
   ever needs a file by name, add it to the index.
5. **Moving folders needs a migration.** `tools/extract_assets.py` carries a player's edits over from
   the old place (as it does from the pre-0.37 `pac/` layout), so nobody loses work. Say so in the
   CHANGELOG's For players part.
6. **Update this page and the tree above** when a top-level folder or a naming rule changes.

## New art: `extensions/`

Art made for this project, not taken from the disc, lives under `extensions/`, which is committed.
It uses the same paths as the extracted tree, so a stage's new art sits at the same relative place
as the stage: `extensions/stages/0c_dan/` beside `us/stages/0c_dan/`. It isn't per region: the same
art serves every disc.

- `extensions/stages/<nn>_<name>/widescreen_left.png`, `widescreen_right.png`: WIDE FORCED's stage
  extensions, 48 × 288, magenta placeholders until painted ([docs/STAGE_EXTENSIONS.md](../docs/STAGE_EXTENSIONS.md)).
- To come (roadmap phase 5): the extensions' own screens (EX. SETTING, PLAYER CONFIG, JOIN / WAIT, pad
  icons) under `extensions/screens/`.

Re-extracting or deleting a region's folder never touches `extensions/`.
