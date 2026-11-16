# Stage extensions

**Status:** drawn in WIDE FORCED since 0.50.0 (`src/extensions/stagestrips.c`), one strip per layer
and side. Since 0.90.0 every layer whose art stops short of a wall has a strip there (227 placeholders across
the 38 stages, waiting to be painted), each continuing its own layer and reaching 8 columns under that
layer's art and 8 past the screen's edge, so no gap shows at any camera position; BISON's two picture layers have them too.
See [How the game draws them](#how-the-game-draws-them).

WIDE FORCED shows 48 more pixels on each side of the screen than the stage's art was made for. At the
camera limits that leaves a black strip of up to 48 px at the edge ([STAGES.md](STAGES.md#widescreen)).
A **stage extension** is new art for that strip: one image per layer and side, joined to that layer's
edge, drawn just behind that layer.

- **Only in WIDE FORCED.** NORMAL and WIDE draw exactly what they draw today.
- **Drawn behind its layer.** Where a layer's own art reaches the edge, the art stays on top; nearer
  layers stay in front of farther layers' strips.
- **Picture only.** The camera limits (256 / 640) and the walls stay as they are. Pulling the limits in
  would make every stage narrower and move the corners, which changes how fights play. Parity runs with
  WIDE FORCED on must still match it off, frame for frame (the rule in [WIDESCREEN.md](WIDESCREEN.md)).
- **Not from the disc.** These images are new work, not the game's, so they are committed to the repo
  in `assets/extensions/`. The rest of `assets/` comes from your own disc and is never committed.

## The images

A stage has up to 8 layers (layer 1 is the fighters' layer). Which plane each one is depends on the
stage: the depth the game draws it at decides (the table below lists them farthest first). On RYU 1,
layer 0 is the sky and sun, layer 2 the castle and trees, layer 1 the floor in front; on ADON, layer 2
is the farthest and layer 0 the nearest. Before 0.88.2 the measured depths were read on the screens
before the fight, which use the same layers for their own backdrops, so this page said layer 2 was
always the farthest and layer 0 the nearest. Each scrolls at its own rate, so each one's art ends
at its own place, and each can have a strip at each side, joined to that layer. They go in the stage's
folder under `assets/extensions/`, for example `stages/0c_dan/`: the same name as the stage's extracted
folder (`assets/us/stages/0c_dan/`, which holds `st0c.pac`; [assets/LAYOUT.md](../assets/LAYOUT.md)).
The four stage numbers you can't pick (0x12, 0x13, 0x14, 0x1F) load another stage's archive, so they
use that stage's files. US and JP have the same stages, so the same files serve both.

| File | What |
|---|---|
| `widescreen_left.png`, `widescreen_right.png` | layer 1, the fighters' layer |
| `widescreen_layer<n>_left.png`, `widescreen_layer<n>_right.png` | layer n (0, 2 ...) |
| `widescreen.json` | where each strip goes: `"layer<n>_left"` / `"layer<n>_right"`, the layer x of its left column |
| | `"layer<n>_<side>_top"`: for a layer whose top rows scroll on their own (RYU 1 and CHUN-LI: layer 1), the x those rows' part of the strip goes at (0.91.0) |
| | `"layer<n>_<side>_front": 1`: that strip is drawn just in front of its layer instead of behind it, so it covers the layer's own art at the edge (0.50.1) |

`tools/measure_stage_layers.py` writes `widescreen.json` and the placeholders (see below). Without a
layer's entry in it, layer 1's strips go at 208 and 1024 and other layers draw none.

**Seeing them:** turn on **Stage Extensions** on the launcher's Extensions tab (or run the game with
`--features default,stage_extensions`; it needs Widescreen), set SCREEN SETTING > SCREEN SIZE to
WIDE FORCED, and fight. Off (the default), no strip is loaded. Before 0.60.0 this was **Custom
Assets** (`--custom-assets`, still accepted).

**Strips asked for by hand** (the maintainer, 2026-11-06; `EXTRAS` in the tool, so a re-measure
keeps them). The probe only finds black, so these two needed saying:

| Stage | Strip | Why |
|---|---|---|
| CHARLIE (03) | layer 0, both sides, **in front** of the layer | the stage's own art ends in bright green filler columns (screen x 34 - 45 at the left wall, 435 - 446 at the right): the placeholder covers them as well as the black |
| FEILONG (21) | layer 1 (the fighters' layer), right | the tree and the floor stop at the right wall with sky behind them, so nothing is black: the placeholder covers every row the layer has art in near the edge, for the tree and floor to be painted on (leave the sky transparent) |

**Every image is 48 × 288 pixels**, RGBA: alpha below 128 is transparent (the layers behind show
through), and colour is rounded to 15 bits.

- **Width 48 - 64:** the extra width WIDE FORCED shows on each side (480 instead of 384) is 48; a strip
  can be up to 64 wide, the extra columns reaching under its layer's own art (inwards: the left strip's
  right-hand side, the right strip's left-hand side) or past the screen's edge. The placeholders are
  64: 8 columns under the art, the 48, and 8 past the edge, so neither rounding at the join nor a
  farther layer's scroll wavering by a few pixels at the wall leaves a gap. `widescreen.json` gives the
  strip's leftmost column: a left placeholder starts 56 left of where the layer's view starts at the
  left limit, a right one 8 left of where it ends at the right limit.
- **Height 288:** row r is the layer's y r. With the camera at rest (the layer's y scroll 16) the
  screen's top row shows row 16; the camera rises up to 16 px when a fighter is in the air (measured;
  it never goes higher), so rows 0 - 15 only show then. The game's 2D drawing has 224 rows, stretched
  to the screen's 272, so rows 0 - 239 are the ones ever seen; 240 - 287 never show.
- **Where it sits:** joined to its layer. The left strip's right-hand column touches where that layer's
  view starts with the camera at its left limit (for the fighters' layer: layer x 208 - 255, touching
  the stage's left edge at 256); the right strip's left-hand column touches where the view ends at the
  right limit (fighters' layer: 1024 - 1071). With the camera at a limit, each fills the outer 48
  columns of the screen; as the camera moves away, it slides off at its own layer's speed.
- **In front of what:** each strip is drawn just behind its own layer, so the layer's own art stays on
  top of it, and a nearer layer's art and strips stay in front of a farther layer's.

### The placeholders

The committed images are placeholders, 64 × 288, in the colour of the layer they belong to, with the
stage number, side and layer written on them in a darker shade (`0x0C`, `L`, `L2`). Since 0.90.0 each is
filled in whole rows: every row where its layer's art runs into the edge but stops short of the screen's
end (the layer draws in any of the 8 columns next to the strip but not in all of the outermost 4, at
either camera height), so each strip carries its own layer on, and a layer whose art already reaches
the edge gets none there and a
nearer layer's strip covers a farther one's only where the nearer layer has art; and every row with a
gap no layer's art explains, given to the farthest layer with art near the edge in that row (or the
farthest layer); filled rows grow by 2 rows up and down. A strip with nothing to fill has no file. Before 0.90.0 they filled only the pixels
that were black at the two measured camera positions, exactly, so any other position or a rounding
step could show a gap.

| Layer | Colour |
|---|---|
| 0 | orange `#FF8000` |
| 1 (fighters') | magenta `#FF00FF` |
| 2 | blue `#00A0FF` |
| 3 | green `#00D200` |
| 4 - 7 | yellow, purple, red, teal |

If a coloured patch shows in a fight, nobody has painted that strip yet.

### Painting one

1. Take a WIDE FORCED screenshot at the wall you're painting: walk both fighters into that corner.
2. Paint over the coloured area so it continues that layer's art outwards from its edge. You can paint
   more of the strip than the coloured area (another layer's art covers it where it's in front), and
   leave transparent what that layer would leave empty (sky behind a fence, for one).
3. Keep to 15-bit colour (5 bits per channel, as the stage's own textures have), so the strip doesn't
   look smoother than the art next to it. The loader rounds to that anyway.
4. Keep the file name, size and format. A file that isn't a placeholder any more is never overwritten
   by the measuring tool, and its entry in `widescreen.json` is kept.

## What each strip shows today

How many pixels each layer's strip fills at each wall (both camera heights together), from
`python tools/measure_stage_layers.py --exe build-headless/a3mx` (about 5 minutes; `--dry-run`
writes nothing, `--stages 1,6` measures some). The layers are listed farthest first, with their depth.
Re-measured in 0.90.0 with the whole-row, overlapping placeholders (none had been painted); every
layer whose art stops short of a wall has a strip there.

| # | Menu name | Stage | Layers (depth) | Left: pixels by layer | Right: pixels by layer |
|---|---|---|---|---|---|
| 1 | RYU 1 | 0x00 | 0 (5), 2 (9), 1 (13) | 0: 13568, 1: 9536, 2: 12032 | 0: 13568, 1: 7040, 2: 9472 |
| 2 | RYU 2 | 0x17 | 0 (5), 2 (9), 1 (13) | 0: 13952, 1: 13504, 2: 5696 | 0: 11648, 1: 13440, 2: 10496 |
| 3 | KEN | 0x01 | 0 (5), 1 (9), 2 (13) | 0: 10688, 1: 13632, 2: 3904 | 0: 14016, 1: 5376, 2: 12736 |
| 4 | AKUMA 1 | 0x02 | 0 (5), 2 (9), 1 (13) | 0: 14208, 1: 13568, 2: 9408 | 0: 11008, 1: 14016, 2: 7232 |
| 5 | AKUMA 2 | 0x24 | 0 (5), 2 (9), 1 (13) | 0: 13952, 1: 13568, 2: 5696 | 0: 11648, 1: 13440, 2: 10496 |
| 6 | CHARLIE | 0x03 | 0 (5), 2 (9), 1 (13) | 0: 14592, 1: 6848, 2: 3904 | 0: 14656, 1: 6464, 2: 5248 |
| 7 | CHUNLI | 0x04 | 0 (5), 1 (9), 2 (13) | 0: 14080, 1: 6400, 2: 11392 | 0: 14208, 1: 11136, 2: 11904 |
| 8 | ADON | 0x05 | 2 (5), 1 (9), 0 (13) | 0: 5248, 1: 15360, 2: 10176 | 0: 5312, 1: 14400, 2: 11008 |
| 9 | SODOM | 0x06 | 1 (5), 2 (9), 0 (13) | 0: 8448, 1: 14592, 2: 14016 | 0: 7936, 1: 15488, 2: 10624 |
| 10 | GUY | 0x07 | 2 (5), 0 (9), 1 (13) | 0: 8640, 1: 14336, 2: 12096 | 0: 4992, 1: 6976, 2: 15168 |
| 11 | BIRDIE | 0x08 | 2 (5), 0 (9), 1 (13) | 0: 6848, 1: 7936, 2: 14784 | 0: 6848, 1: 7552, 2: 14784 |
| 12 | ROSE | 0x09 | 2 (5), 1 (9), 0 (13) | 0: 13184, 1: 12608, 2: 12352 | 0: 5376, 1: 5440, 2: 13760 |
| 13 | SAGAT | 0x0B | 0 (5), 1 (9), 2 (13) | 0: 14464, 1: 5888, 2: 3904 | 0: 14208, 1: 11008, 2: 5376 |
| 14 | DAN | 0x0C | 2 (5), 0 (9), 1 (13) | 0: 13376, 1: 6848, 2: 13312 | 0: 5504, 1: 5504, 2: 13824 |
| 15 | SAKURA | 0x0D | 2 (5), 1 (9), 0 (13) | 0: 5568, 1: 9728, 2: 12736 | 0: 6720, 1: 14144, 2: 11520 |
| 16 | ROLENTO | 0x0E | 2 (5), 0 (9), 1 (13) | 0: 6400, 1: 14912, 2: 10560 | 0: 6400, 1: 14976, 2: 10432 |
| 17 | DHALSIM | 0x0F | 0 (5), 2 (9), 1 (13) | 0: 14208, 1: 9280, 2: 10816 | 0: 14208, 1: 5504, 2: 7296 |
| 18 | ZANGIEF | 0x10 | 0 (5), 2 (9), 1 (13) | 0: 9088, 1: 8256, 2: 10816 | 0: 11392, 1: 12864, 2: 10944 |
| 19 | GEN | 0x11 | 2 (5), 0 (9), 1 (13) | 0: 9728, 1: 9216, 2: 4480 | 0: 4288, 1: 13760, 2: 7360 |
| 20 | BALROG | 0x15 | 2 (5), 1 (9), 0 (13) | 0: 9792, 1: 13248, 2: 12608 | 0: 10048, 1: 11136, 2: 13632 |
| 21 | CAMMY | 0x16 | 0 (5), 2 (9), 1 (13) | 0: 13760, 1: 6848, 2: 11584 | 0: 13568, 1: 12480, 2: 6656 |
| 22 | HONDA | 0x18 | 2 (5), 1 (9), 0 (13) | 0: 5888, 1: 7488, 2: 14144 | 0: 5376, 1: 14016, 2: 14016 |
| 23 | BLANKA | 0x19 | 0 (5), 2 (9), 1 (13) | 0: 13504, 1: 9216, 2: 7872 | 0: 13568, 1: 11840, 2: 12928 |
| 24 | MIKA | 0x1A | 2 (5), 0 (9), 1 (13) | 0: 6016, 1: 14016, 2: 13888 | 0: 13952, 1: 9536, 2: 14144 |
| 25 | CODY | 0x1B | 2 (5), 1 (9), 0 (13) | 0: 8192, 1: 13568, 2: 11968 | 0: 5440, 1: 9408, 2: 14144 |
| 26 | VEGA | 0x1C | 2 (5), 1 (9), 0 (13) | 0: 10240, 1: 12352, 2: 13440 | 0: 10368, 1: 13440, 2: 14016 |
| 27 | KARIN 1 | 0x1D | 2 (5), 1 (9), 0 (13) | 0: 4096, 1: 7296, 2: 14272 | 0: 12416, 1: 10560, 2: 9344 |
| 28 | KARIN 2 | 0x29 | 2 (5), 1 (9), 0 (13) | 0: 8192, 1: 12032, 2: 13248 | 0: 5376, 1: 14016, 2: 11840 |
| 29 | BISON1 | 0x0A | 1 (5), 0 (9), 2 (13) | 0: 3392, 1: 3648, 2: 6592 | 0: 8128, 1: 3520, 2: 12416 |
| 30 | BISON2 | 0x1E | 1 (5), 0 (9), 2 (13) | 0: 5824, 1: 3648, 2: 4864 | 0: 5376, 1: 4096, 2: 12416 |
| 31 | GUILE | 0x20 | 2 (5), 0 (9), 1 (13) | 0: 2880, 1: 9792, 2: 2112 | 0: 2496, 1: 6464, 2: 2048 |
| 32 | FEILONG | 0x21 | 2 (5), 1 (9), 0 (13) | 0: 5504, 1: 15424, 2: 14976 | 0: 5376, 1: 14832 |
| 33 | DEEJAY | 0x22 | 0 (5), 2 (9), 1 (13) | 0: 14784, 1: 9408, 2: 14080 | 0: 14656, 1: 13440, 2: 10368 |
| 34 | HAWK | 0x23 | 2 (5), 0 (9), 1 (13) | 0: 5248, 1: 10432, 2: 14144 | 0: 5376, 1: 13440, 2: 14208 |
| 35 | BATTLE1 | 0x25 | 0 (5), 2 (9), 1 (13) | 0: 2048, 1: 7872, 2: 4864 | 0: 2048, 1: 9024, 2: 5760 |
| 36 | BATTLE2 | 0x26 | 0 (5), 2 (9), 1 (13) | 0: 1920, 1: 9600, 2: 10816 | 0: 1920, 1: 8960, 2: 6528 |
| 37 | BATTLE3 | 0x27 | 0 (5), 2 (9), 1 (13) | 0: 1216, 1: 6976, 2: 5824 | 0: 960, 1: 9024, 2: 6656 |
| 38 | BATTLE4 | 0x28 | 0 (5), 2 (9), 1 (13) | 0: 1920, 1: 9600, 2: 10816 | 0: 1920, 1: 8960, 2: 6528 |

Measured at the camera's limits with both fighters pinned at the wall, on the ground and high in the
air. A gap is a pixel where a magenta probe drawn behind everything shows through (so black that a
layer or the fight's overlay draws itself is not one). Shares under 64 pixels are dropped as noise
from the fighters' animation.

## How the game draws them

`src/extensions/stagestrips.c`, an extension (so `--no-extensions` and a build without the extensions
draw no strips):

1. **Load:** the first time a stage is drawn in WIDE FORCED, its strips and `widescreen.json` from
   `assets/extensions/<the stage archive's folder in index.json>/` (`stages/0c_dan` for `st0c`; the
   four numbers you can't pick use the stage they load), up to 8 strips a stage. A missing or
   unreadable file is skipped (the log says why).
2. **Draw:** while WIDE FORCED is on and a fight is running (the fight's per-frame step 0x08823674 has
   run in the last two frames), each time one of the stage tile drawers (0x0881AB28 with the layer,
   0x0881B268 with a record whose kind names the layer) or picture drawers (0x08819498 / 0x0881C0C8,
   BISON's mountains and sky, with the layer; since 0.90.0) is about to draw a layer, that layer's strips,
   once a frame: textured quads placed like the layer's own tiles (the screen's scale and offset, its
   +98, and the horizontal scroll that layer's drawer actually uses: the layer's +18 for the 32-pixel
   drawer and GUILE's strips, the column and part-tile in 0x0881B268's record otherwise (the fighters'
   layer and most 16-pixel layers follow the fighters' layer's travel, not +18), BISON's mountains the
   travel and its sky half of +8 less +16; since 0.91.0, before which a strip slid by up to ~10 px
   against its layer as the camera left the wall). RYU's and CHUN-LI's layer 1 move their top rows on
   their own: 0x0881B268 draws them in a first call, and their strip rows there are placed by that
   call's scroll and `"layer1_<side>_top"` in `widescreen.json`, the rest by the second's, at one less than the layer's depth (the game's 2D library draws a larger depth in front), or one more for a `_front` strip.
   The quads are clipped to the screen with their texels: the library clips a quad's corners without
   moving its texels, which squeezes anything that runs far off the screen.
3. **Switch:** checked every frame, so changing SCREEN SIZE takes effect at once.
4. **Checked:** in 0.91.0 on all 38 stages with the camera 0, 16, 32 and 52 pixels from each wall: no black
   pixel the strips leave that isn't there with them off (the stages' own black art). In 0.90.0 on all 38 stages, both walls, on the ground and high: with the placeholders
   no black edge pixel is left that the probe calls a gap (up to 13,000 per wall without them);
   the few black pixels left (DAN's right wall, CODY's left, the portraits' frames, a line on BISON 1)
   are the stage's or the overlay's own black art. (0.50.0 checked 7 stages.) Parity tests run
   without `assets/` (`--no-assets`), so they draw no strips; the fighters' state is the same either
   way, because the strips change nothing but the picture and the extension's own memory.
5. **Measuring:** `A3MX_STRIP_MEASURE` (for `tools/measure_stage_layers.py`) turns the strips off and,
   in chosen frames, draws only chosen layers, or a magenta probe behind everything.
