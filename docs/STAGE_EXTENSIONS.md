# Stage extensions

**Status:** drawn in WIDE FORCED since 0.50.0 (`src/extensions/stagestrips.c`), one strip per layer
and side where that layer leaves a gap: 152 placeholders across the 38 stages, waiting to be painted.
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

A stage has up to 8 layers (layer 1 is the fighters' layer; on every stage layer 2 is the farthest
plane, the background, and layer 0 the nearest). Each scrolls at its own rate, so each one's art ends
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

- **Width 48:** the extra width WIDE FORCED shows on each side (480 instead of 384).
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

The committed images are placeholders: transparent except where WIDE FORCED shows black today, and
there in the colour of the layer whose strip it belongs to, with the stage number, side and layer
written on it in a darker shade (`0x0C`, `L`, `L2`). Each black pixel at a wall goes to the farthest
layer with art in that screen row near the edge (the plane the gap continues), or, in a row where no
layer has art near the edge, to the farthest layer. A strip with nothing to fill has no file.

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

| # | Menu name | Stage | Layers (depth) | Left: pixels by layer | Right: pixels by layer |
|---|---|---|---|---|---|
| 1 | RYU 1 | 0x00 | 2 (3), 1 (5), 0 (5) | 0: 1176, 1: 616, 2: 4424 | 1: 2084, 2: 5890 |
| 2 | RYU 2 | 0x17 | 2 (3), 1 (5), 0 (5) | 1: 6417, 2: 6343 | 1: 1894, 2: 4808 |
| 3 | KEN | 0x01 | 2 (3), 1 (5), 0 (5) | 1: 8272, 2: 4324 | 1: 1157, 2: 8375 |
| 4 | AKUMA 1 | 0x02 | 2 (3), 1 (5), 0 (5) | 1: 8015, 2: 7560 | 1: 3294, 2: 3883 |
| 5 | AKUMA 2 | 0x24 | 2 (3), 1 (5), 0 (5) | 1: 6384, 2: 6343 | 1: 1894, 2: 4808 |
| 6 | CHARLIE | 0x03 | 2 (3), 1 (5), 0 (5) | 0: 5542, 1: 1367, 2: 3485 | 0: 1665, 1: 1253, 2: 5151 |
| 7 | CHUNLI | 0x04 | 2 (3), 1 (5), 0 (5) | 0: 666, 1: 136, 2: 2788 | 0: 900, 1: 349, 2: 2593 |
| 8 | ADON | 0x05 | 2 (3), 1 (5), 0 (13) | 1: 10044, 2: 7217 | 1: 8000, 2: 5956 |
| 9 | SODOM | 0x06 | 2 (3), 1 (5), 0 (13) | 1: 2121, 2: 11030 | 1: 1664, 2: 10134 |
| 10 | GUY | 0x07 | 2 (3), 1 (5), 0 (9) | 1: 3520, 2: 5312 | 1: 1296, 2: 7768 |
| 11 | BIRDIE | 0x08 | 2 (3), 1 (5), 0 (9) | 1: 280, 2: 3615 | 1: 362, 2: 3419 |
| 12 | ROSE | 0x09 | 2 (3), 1 (5), 0 (13) | 0: 235, 1: 3478, 2: 8648 | 2: 7917 |
| 13 | SAGAT | 0x0B | 2 (3), 1 (5), 0 (5) | 0: 4601, 1: 3311, 2: 3955 | 0: 756, 1: 7725, 2: 4961 |
| 14 | DAN | 0x0C | 2 (3), 1 (5), 0 (9) | 0: 2132, 2: 8323 | 2: 3519 |
| 15 | SAKURA | 0x0D | 2 (3), 1 (5), 0 (13) | 1: 4292, 2: 5059 | 0: 180, 1: 2952, 2: 5688 |
| 16 | ROLENTO | 0x0E | 2 (3), 1 (5), 0 (9) | 1: 4116, 2: 5597 | 1: 9578, 2: 5530 |
| 17 | DHALSIM | 0x0F | 2 (3), 1 (5), 0 (5) | 1: 4482, 2: 5493 | 0: 84, 1: 4533, 2: 6220 |
| 18 | ZANGIEF | 0x10 | 2 (3), 1 (5), 0 (5) | 1: 1888, 2: 3123 | 0: 195, 1: 1240, 2: 3293 |
| 19 | GEN | 0x11 | 2 (3), 1 (5), 0 (9) | 2: 1247 | 1: 6303, 2: 2640 |
| 20 | BALROG | 0x15 | 2 (3), 1 (5), 0 (13) | 1: 4480, 2: 5624 | 1: 2200, 2: 5659 |
| 21 | CAMMY | 0x16 | 2 (3), 1 (5), 0 (5) | 0: 1200, 1: 240, 2: 9878 | 0: 1728, 1: 6240, 2: 6830 |
| 22 | HONDA | 0x18 | 2 (3), 1 (5), 0 (13) | 2: 9815 | 1: 336, 2: 8770 |
| 23 | BLANKA | 0x19 | 2 (3), 1 (5), 0 (5) | 1: 371, 2: 8759 | 2: 8454 |
| 24 | MIKA | 0x1A | 2 (3), 1 (5), 0 (9) | 2: 2121 | 1: 496, 2: 3513 |
| 25 | CODY | 0x1B | 2 (3), 1 (5), 0 (13) | 1: 7462, 2: 7298 | 1: 640, 2: 3853 |
| 26 | VEGA | 0x1C | 2 (3), 1 (5), 0 (13) | 1: 853, 2: 9170 | 1: 849, 2: 9204 |
| 27 | KARIN 1 | 0x1D | 2 (3), 1 (5), 0 (13) | 1: 752, 2: 3488 | 0: 5136, 1: 6066, 2: 6421 |
| 28 | KARIN 2 | 0x29 | 2 (3), 1 (5), 0 (13) | 1: 4292, 2: 6130 | 1: 2952, 2: 5868 |
| 29 | BISON1 | 0x0A | 2 (3), 1 (5) | nothing | 2: 714 |
| 30 | BISON2 | 0x1E | 2 (3), 1 (5) | nothing | 2: 714 |
| 31 | GUILE | 0x20 | 2 (3), 1 (5), 0 (9) | 1: 480, 2: 1281 | 1: 352, 2: 1072 |
| 32 | FEILONG | 0x21 | 2 (3), 1 (5), 0 (13) | 1: 728, 2: 10944 | nothing |
| 33 | DEEJAY | 0x22 | 2 (3), 1 (5), 0 (5) | 1: 574, 2: 9387 | 1: 6416, 2: 8148 |
| 34 | HAWK | 0x23 | 2 (3), 1 (5), 0 (9) | 1: 758, 2: 5795 | 1: 1256, 2: 4936 |
| 35 | BATTLE1 | 0x25 | 2 (3), 1 (5), 0 (5) | 1: 1260, 2: 864 | 1: 896, 2: 837 |
| 36 | BATTLE2 | 0x26 | 2 (3), 1 (5), 0 (5) | 1: 192, 2: 1488 | 1: 160, 2: 1253 |
| 37 | BATTLE3 | 0x27 | 2 (3), 1 (5), 0 (5) | 1: 240, 2: 1478 | 1: 192, 2: 1253 |
| 38 | BATTLE4 | 0x28 | 2 (3), 1 (5), 0 (5) | 1: 192, 2: 1488 | 1: 160, 2: 1253 |

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
   0x0881B268 with a record whose kind names the layer) is about to draw a layer, that layer's strips,
   once a frame: textured quads placed like the layer's own tiles (its scroll, the screen's scale and
   offset), at one less than the layer's depth (the game's 2D library draws a larger depth in front), or one more for a `_front` strip.
   The quads are clipped to the screen with their texels: the library clips a quad's corners without
   moving its texels, which squeezes anything that runs far off the screen.
3. **Switch:** checked every frame, so changing SCREEN SIZE takes effect at once.
4. **Checked:** on 7 stages across the select order (0x00, 0x03, 0x09, 0x15, 0x1C, 0x22, 0x28), both
   walls, every gap pixel the probe finds (105,920 of them) is filled by a strip. Parity tests run
   without `assets/` (`--no-assets`), so they draw no strips; the fighters' state is the same either
   way, because the strips change nothing but the picture and the extension's own memory.
5. **Measuring:** `A3MX_STRIP_MEASURE` (for `tools/measure_stage_layers.py`) turns the strips off and,
   in chosen frames, draws only chosen layers, or a magenta probe behind everything.
