# assets/

The game's data, unpacked into standard formats you can edit (PNG today; WAV / OGG to come), laid
out by what it is: characters, stages, screens, sound. The rules for that layout, and for keeping to
it while decompiling, are in [LAYOUT.md](LAYOUT.md).

| Path | What | Committed |
|---|---|---|
| `us/`, `jp/` | extracted from your own disc by `python tools/extract_assets.py` | never |
| `us/index.json` | which folder holds which of the disc's archives: what the game reads | never |
| `us/characters/00_ryu/fighter/` ... | each archive unpacked into one folder: a `manifest.json`, and each entry in a subfolder for its kind (`textures/`, `palettes/`, `text/`, `sound/`, `voices/`, `fighter_data/`, `data/`) | never |
| `us/music/*.at3` | the 51 music tracks (ATRAC3+), as on the disc; WAV / OGG decoding to come | never |
| `extensions/` | new art that isn't the game's, such as WIDE FORCED's stage edges ([docs/STAGE_EXTENSIONS.md](../docs/STAGE_EXTENSIONS.md)) | yes |

The game loads every archive whose folder `index.json` names from that folder, raw files and PNGs
included. Archives with no folder still come from the disc. `--no-assets` plays from the disc only.

**To change something,** paint over a texture's or palette's PNG, or replace a raw file, in its
folder. For example, Dan's stage is `us/stages/0c_dan/textures/`. The game builds the archive from
the folder each time it loads it, so the change shows up the next time that archive loads, with no
repacking. The log says which entries differ from the disc and which came from PNGs.
- A PNG is used when it is newer than its raw file.
- Indexed textures come in their own palette's colours. Paint with those: any other colour becomes the
  nearest one. The few whose palette isn't known are grey, and the grey level is the colour index.
- Same size is always safe. What a larger entry can be is in
  [docs/ASSET_LOADING.md](../docs/ASSET_LOADING.md).
- `python tools/pac.py pack assets/us/stages/0c_dan st0c.pac` still makes a `.pac` file from the raw
  files.

**Going back to the disc:** delete an archive's folder, or its line in `index.json`. Running
`tools/extract_assets.py` again rewrites every folder, so your edits there are overwritten: keep a
copy.

**Extracted before 0.37.0?** The old layout was `us/pac/<archive name>/`. Run
`tools/extract_assets.py` again: it unpacks into the new folders, carries over every file you had
changed, and renames the old folder `pac.before-0.37/` for you to delete. Until then, the game still
reads the old layout. Before 0.6.0, indexed textures were all grey, and before 0.5.0 the 16-bit PNGs
had red and blue swapped. If yours are that old, extract again.
