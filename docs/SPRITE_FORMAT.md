# Sprites: the fighters' pictures, tiles, palettes and animations

How a fighter's sprites are stored in its `pl*_u.pac` archive and drawn. Everything here follows the
decompiled drawing code: `src/core/object/sprite_cache.c` (cells, tile sets, the LZ unpacker, the
picture cache), `src/core/object/sprite_grid.c` (placing cells), `src/core/object/sprite_drawers.c`
(pictures by frame) and `src/core/loader/async.c` (where each entry goes). `tools/sprites.py` reads it;
`sprites.py verify` rebuilds the pages the game caches and compares them byte for byte with a RAM
snapshot (52 cached pictures of Ryu and Dan checked, all identical).

Other sprite sets (effects, screens, stages' objects) use the same pictures, cells and tiles; only
where their lists come from differs.

## The archive

Ryu's `pl00_u.pac`, as an example (sizes differ by character):

| Entry | Type | Size | What | Where the game puts it |
|---|---|---|---|---|
| 0 | `0x00100000` | 18,905 | the picture list | `0x08E75AD8[slot]` |
| 1 | `0x00200000` | 611,681 | the tile set | `0x092D9D40[slot]` |
| 2 | `0x00400000` | 960 | the palettes | `0x092D9E00[slot]` |
| 3, 4 | `0x4000xxxx` | | voices | the sound library |
| 5, 6, 7 | data | | `06_data` is entry 6: animation frames and the fighter's tables | the fighter's set-up (`unpack_offsets`) |
| 8 | `0x10000E00` | | a texture | a texture page |

The slot is the sprite's +86: 0 and 1 for the first two fighters (3 and 4 in team modes), other numbers
for effects and screens (list 5 is a fighter's extra picture, `0x0880D68C`). `0x088158A4` copies each
fighter's three pointers into its slot from the loader's per-heap rows (`0x092D9EC0` / `0x092D9ED0` /
`0x092D9EE0`).

## Pictures (entry 0)

The list starts with 12-byte pictures; the first picture's cell offset says how many there are (Ryu:
5,652 / 12 = 471). Then the cells.

| Offset | Type | What |
|---|---|---|
| +0 | u32 | where its cells start, from the start of the list |
| +4 | u8 | columns |
| +5 | u8 | rows |
| +6 | s16 | origin x: the sprite's place is this far right of the picture's left edge |
| +8 | s16 | origin y: and this far below its top |
| +10 | s16 | nonzero: the next picture is drawn too, at the same place (and so on, to one with 0) |

**Cells**, row by row, left to right: one byte `0xFF` for an empty cell, otherwise three bytes:

| Byte | What |
|---|---|
| 0, 1 | the tile number, big-endian |
| 2 | bits 0 - 4 the palette (added to the sprite's, below); bit 5 blended; bit 6 flipped left to right; bit 7 flipped top to bottom |

A cell is 32 x 32 pixels when the sprite's +164 is set (every fighter) and 16 x 16 otherwise. The
picture is drawn from its origin: the first column at `x - origin x`, the first row at
`y - origin y`; flipped across, the picture turns about the same point (columns from
`x + origin x - columns × size`, right to left).

Sprites drawn with the cells' **own palettes** (drawers 7 - 10, `0x088117D0` / `0x08811BCC`) read the
same three bytes differently: byte 0 picks a texture of the list in use, byte 1 the 16 x 16 texel cell
in it (low nibble across, high nibble down), byte 2 as above.

## Tiles (entry 1)

The set starts with u32 offsets, one per 16 x 16 tile, each from the start of the set (Ryu: 50,736 / 4
= 12,684 tiles). A 16-pixel cell's tile number n is tile n; a 32-pixel cell n is tiles 4n (top left),
4n + 1 (bottom left), 4n + 2 (top right) and 4n + 3 (bottom right).

A tile is **LZ-compressed** to 128 bytes of 4-bit pixels (two to a byte, the left one in the low
nibble), written into a 64 x 64 page whose rows are 32 bytes (`0x088D2B64`):

- Tokens come eight to a flags byte, bit 0 first.
- Flag 0: one literal byte.
- Flag 1: a byte `hl`. With h = 0, the next byte written l + 1 times. Otherwise l bytes copied from
  earlier output, starting `0x08A3E600[(dst & 7) × 16 + h]` bytes back: h 1 - 7 reach back along the
  row, h 8 - 15 into the rows above.
- After each byte the write moves on by `0x08A3E680[dst & 7]`: 1, and 25 after a row's eighth byte,
  so the tile's rows land 32 bytes apart.
- It stops once 128 bytes are written; a run or copy can write a few bytes more than that (past the
  tile, and at a page's last tile past the page).

Colour 0 is transparent.

## Palettes (entry 2)

Six colours (the buttons' choices) of 160 bytes (256 for character `0x29`, INGRID: eight palettes):
five palettes of 16 colours, each colour a u16
`ABGR1555` (bits 0 - 4 red, 5 - 9 green, 10 - 14 blue, 15 opaque). `0x0881F6A0` / `0x0881F794` load the
fighter's colour into palette slots from the sprite's +13 (0 for the first fighter, 64 for the second)
through `0x0880AEA8`; the game draws with them as they are.

A cell's palette is its own (bits 0 - 4) plus a base, by the sprite's +12 (`0x0880E150`):

| +12 | Palette |
|---|---|
| 0 | the first non-empty cell's |
| 1, -2 | the sprite's +13 |
| -1 (fighters) | the cell's own + the sprite's +13 |
| 2 and over, under -2 | as the draw state already had it |

## Animations (entry 6, `06_data`)

`06_data` starts with a 0x208-byte header: word 0 is 0, word 1 the size of what follows, then 64
offsets (from the end of the header) at +8 and 64 counts at +0x108 (`unpack_offsets`, `0x0886E638`).
A count of -1 makes the entry a list of frame numbers (u32, -1 at the end); 0 a table.

The data starts with the **frames**, 10 bytes each (Ryu: 931):

| Offset | Type | What |
|---|---|---|
| +0 | u16 | how many game frames it shows (low byte); 0x8000 the last frame of a looping animation; 0x4000 the last frame of one that holds |
| +2 | u16 | |
| +4 | u16 | the picture (a number in entry 0); bit 15 (0x8000): nothing drawn this frame |
| +6 | s16 | the step to the next frame, in frames: 1 the next; on a looping last frame the jump back (-5: back five frames); 0 on a frame that holds |
| +8 | u16 | a record in the fighter's 12-byte table (+416's +16; entry 5 of `06_data`): boxes and the extra picture (`0x0880D68C`) |

The entries with a count of -1 list, for each set of animations, each animation's first frame; the
frames end where the first list starts. Most fighters have four sets (entries 1 - 4; Ryu: 64, 50, 48
and 37 animations), `pl18` five, `pl11` / `pl13` ten. **`pl28` (YUN) has none**: its `06_data` starts
with hit boxes, so its frames are elsewhere; finding them needs its set-up (the callers of
`0x0886EB48`). `sprites.py` writes YUN's pictures and palettes and leaves its frames out. A fighter's +28 points at its frame; the game moves it by the step when the frame's time is
up, and points it at another animation's first frame when the fighter starts a new move. Checked by
following Ryu's +28 through arcade_fight frame by frame.

## Pictures in pieces

A fighter can be drawn from several pictures at one place: a picture with +10 set goes on into the
next. Most fighters' pictures are whole; INGRID's (`pl29`, `pl29r`) are pieces (legs, body, arms ...),
so her pictures on their own show parts. The metadata's `continues` says which go together.

## Extracted files

`tools/extract_assets.py` (US disc; `--no-sprites` to skip, a few minutes) runs `tools/sprites.py
extract` for every fighter archive into `characters/<nn>_<name>/fighter/sprites/`:

| File | What |
|---|---|
| `pictures/NNNN.png` | picture NNNN as an 8-bit indexed PNG: index = palette × 16 + colour, 0 transparent; the PNG's own palette is colour 0's, for viewing |
| `colours/colour_N.png` | every picture in colour N on a grid, for looking (a picture larger than 128 × 128 is cut) |
| `metadata.json` | each picture's cells, origin and `continues`; the six colours' palettes (hex `ABGR1555`); the frames (picture, duration, `end`: loop / hold, step, record) and the animation sets (first frames) |

## Drawing past the 4:3 edge

The pictures are whole: nothing in the data is cut. NORMAL and WIDE skip a cell that starts left of
minus its width, so a fighter at the left wall loses part of itself (Blanka's shoulder); WIDE NATIVE
draws it whole. A game screenshot compared with these pictures should be taken in WIDE NATIVE, or away
from the screen's sides.
