# Loading from `assets/`

Status: **built (0.5.0; folders by content since 0.37.0)**: the game loads each archive that has a
folder under `assets/<region>/` (the one `index.json` names; [assets/LAYOUT.md](../assets/LAYOUT.md))
from that folder: raw files as they are, and textures and palettes from their PNGs when those have
been edited.

## What it does

Edit a texture's or palette's PNG, or replace any raw file, in an archive's folder under `assets/us/`, and the
game uses it the next time it loads that archive, with no repacking. Archives with no folder still come from the disc, and an unedited folder
gives the game exactly the disc's bytes, so an unmodded install plays the same, frame for frame (tested).

```
python tools/extract_assets.py              # once: unpacks every archive into its folder under assets/us/
# paint over e.g. assets/us/stages/0c_dan/textures/07_texture_10000B00.png (Dan's stage) in any image editor
build/a3mx --work work                   # assets/us beside the work folder is used by default
```

## How

Every archive byte the game reads goes through two loaders, both decompiled (`src/core/loader/`):
`pac_load` (`pac.c`) and the loader thread (`async.c`). Each opens the archive by name
(`disc0:/PSP_GAME/USRDIR/usa/data/st0c.pac`), reads the 2 KB header, then each entry in 32 KB chunks
into a destination chosen by the entry's type. So the archive is the unit to override, and the
override sits at the bottom, in the PSP's file functions (`src/platform/hle_io.c`):

1. `sceIoOpen` of a `.pac` in the disc's `data/` folder, read-only, checks for
   the folder `<assets>/index.json` gives for `<name>` (`st0c`: `stages/0c_dan`; with no index, the
   pre-0.37.0 `<assets>/pac/<name>/`), and its `manifest.json`. If it is there, the archive is **built in memory** from the
   folder (`src/platform/asset_pac.c`): the header from the manifest and the raw files' sizes, then
   each entry on a 2 KB boundary, with the stale bytes the disc had after an entry (`.pad` files) where
   the entry is unchanged and zeros where it is not, exactly as `tools/pac.py build` lays it out.
2. The descriptor is then served from that image: `sceIoRead`, `sceIoReadAsync` / `sceIoPollAsync`,
   `sceIoLseek`, `sceIoClose`, and `sceIoGetstat` (the image's size).
3. The image is compared with the disc's archive. What differs is logged once per change, e.g.
   `assets: st0c.pac from assets/us/stages/0c_dan: entry 7 (10000B00) changed`. The end-of-run report
   counts the archive opens served from assets.

Nothing changes in the game's memory or in the loaders: they make the same calls, one step a frame, in
the same chunk sizes, whether the bytes come from the disc or from assets. That is why the hook is
here and not in `file_open` as first designed: no per-request table is needed, the decompiled loaders
stay exactly as the originals, and plain files could be overridden the same way later.

If a folder cannot be used (manifest missing or broken, a raw file missing, an entry the loaders
cannot take), the reason is logged and the archive comes from the disc.

## Edited PNGs

Each texture and palette entry has a PNG beside its raw file (`manifest.json` names it). When the PNG
is **newer than the raw file**, it is converted back into the raw file's format as the archive is
built, so the game still only sees its own formats:

| Entry | The PNG | Converted to |
|---|---|---|
| 16-bit texture (489) | RGBA, the texture's size | its own pixel format (ABGR1555 or BGR565), header kept |
| indexed texture, 4 or 8-bit (356) | in its own palette's colours (index 0 transparent) where the palette is known ([PAC_FORMAT.md](PAC_FORMAT.md)); otherwise grey levels = colour indices (0, 17 ... 255 for 4-bit; 0 - 255 for 8-bit) | the indices: a palette PNG with its palette unchanged keeps them exactly; otherwise each pixel takes the nearest of the palette's colours (transparent: index 0) |
| palette (148) | 16 colours a row | ABGR1555 |

- Any PNG an editor saves works: every colour type and bit depth, any row filter (tested; not
  interlaced). Colours narrow to the format with rounding, so painting in 8 bits a channel is fine;
  alpha below 128 is transparent in 1555.
- Indexed textures stay indexed: paint with the colours of their palette (an editor in palette /
  indexed mode keeps to them; any other colour becomes the nearest one). The few whose palette is not
  known are painted in the greys of their PNG.
- A raw file newer than its PNG wins, so replacing raw files keeps working.
- A PNG that cannot be used (another size, non-grey pixels in a grey indexed texture, a damaged file)
  leaves the raw file in use, and the log says why.
- An unedited PNG converts back to exactly its raw file (all 993 are tested), so an old timestamp
  never changes anything.

The log line for an archive names the entries taken from PNGs:
`assets: st0c.pac from assets/us/stages/0c_dan: entry 7 (10000B00) changed; entry 7 from textures/07_texture_10000B00.png`.

**Colours:** the PSP keeps red in the low bits of its 16-bit colours (ABGR1555, BGR565). The PNGs
extracted before 0.5.0 had red and blue swapped; extract again (`python tools/extract_assets.py`).

## What can be changed

- **Replace an entry** with a file of the same size: always safe.
- **A larger or smaller entry**: the header and everything after it move; the loaders read the sizes
  from the header. Where the bytes go decides whether a larger entry fits:

| Kinds | Destination | Larger entry |
|---|---|---|
| fighter data (0x00100000, 0x00200000, 0x00400000) | the fighter heap, which advances by the entry's size | fine (tested: 8 KB longer, nothing else moves) |
| sound, voice (0x20000000, 0x40000000) | a buffer the sound library allocates for the entry's size | fine |
| textures (0x10000000, 0x00800000) | the loaders' 130 KB staging buffer (0x20800 bytes) | up to 0x20800 bytes; a larger one is refused (the archive comes from the disc) |
| everything else (data, text, palettes, tables) | a buffer that already exists, from a pointer table | logged with a **WARNING**: it may overrun that buffer; the buffers' sizes are not known yet |

- **Add entries** to `manifest.json` (up to 254 in an archive): the loaders route them by type like any
  other, with the same rule for their size.

## Switches

- `--assets DIR`: the folder holding `index.json` and the archives' folders (default: `assets/<region>` beside the work folder, the
  region from `work/disc.json`, used when it exists).
- `--no-assets`: everything from the disc.
- The parity tests run with `--no-assets`: they test the code, not the mods.

## Tests

- `asset_pac` (`tests/asset_pac_test.c`):
  - the PNG decoder on 20 files `tests/png_cases.py` writes the way editors do (every colour type,
    bit depth and row filter; stored, fixed and dynamic deflate), checked against the pixels they hold;
  - all 313 archives built from their folders by the game's own builder are byte-identical to the
    disc's, and again with every texture and palette taken from its PNG;
  - hand-made folders: a changed entry, a larger one (with the warning), an added one, a texture too
    large (refused), a broken manifest, a missing raw file;
  - PNG edits: a texture, an indexed texture and a palette repainted; a newer raw file winning; a PNG
    of the wrong size, a non-grey indexed one and a damaged one refused.
- `assets_in_game` (`tests/assets_test.py`), on `arcade_fight` up to the fight (Ryu against Dan on
  Dan's stage):
  1. **Identity:** every archive from an unedited `assets/us`: the same game as from the disc, memory
     and picture, on every frame, with every open served from assets.
  2. **A real change:** a texture of `st0c` repainted red in its PNG: converted to ABGR1555, its new
     pixels are in both places the texture lives in memory, the stage looks different once it is on
     screen, and nothing else in memory differs but the loaders' read buffer.
  3. **A larger entry:** Ryu's last fighter-data entry 8 KB longer: it arrives whole, nothing else in
     memory moves but the loader's file position, and the picture is the same on every frame.

  A deliberate fault (every entry read one byte off) fails it.

## Next (BELOW SHOULD GO IN ROAD MAP)

1. ~~Pair indexed textures with their palettes~~ (0.6.0: [PAC_FORMAT.md](PAC_FORMAT.md)).
2. WAV for sound banks, and text.
3. `tools/pac.py pack` reads raw files only; teach it the PNG rule too if packing by hand is still wanted.
4. Find the sizes of the fixed buffers (text, palettes, data tables) so a larger entry can be checked
   instead of warned about.
5. Plain files queued by name (not archives) go through the same functions; they can be overridden from
   `assets/<region>/files/` the same way if any turn out to matter.
6. The extensions' own screens (EX. SETTING, PLAYER CONFIG, JOIN / WAIT, pad icons) draw from code
   today; giving them asset files is a separate step (ROADMAP phase 5) that can reuse the same layout.
