/* Scaled endings (the scaled_endings feature, WIDE NATIVE): the endings fill the 16:9 screen.
 *
 * An ending's scene is a layer of the 24-tile-wide map (core/stage/tiles.c, 0x0881B7C8: two 12-tile
 * scenes side by side, 384 pixels each) with pictures drawn as sprites over it (core/object/sprite.c).
 * Other endings' scenes are a stage's layers (16- and 32-pixel tiles, pictures). In WIDE NATIVE
 * (widescreen.c tells an ending's frame, widescreen_in_ending):
 *   - a scene with art of its own past the 4:3 sides (a layer that scrolls across its map, a stage
 *     wider than the screen) shows it: the picture carried on;
 *   - otherwise (the map standing still on one half, the other half being another scene; layers whose
 *     columns past the sides are bare) the scene is drawn 1.25 times larger about the picture's middle,
 *     as the super finish's backdrops are, so its 384 fill the 480 (core/task/ending.c decides, each
 *     frame); its pictures (sprites of kinds 73, 77 and 78: the close-ups, skulls and statues, the bursts
 *     and speed lines, built of pieces side by side) with it, and what is linked to them;
 *   - the profile pictures at the 4:3 sides (kind 74, at x 64 or 320) move out to the 16:9 edges;
 *   - everything else (characters, props) is cut at the 4:3 sides, so what the 4:3 screen kept off it
 *     stays hidden.
 * With this feature off the ending keeps black sides: the map's columns and every sprite are cut at the
 * 4:3 sides and nothing moves (widescreen.c, tiles.c, sprite.c). Only the drawing changes.
 */
#include "rt.h"
#include "ext_features.h"

int widescreen_in_ending(void);   /* widescreen.c */

/* the ending's frame is being drawn and its scenes are scaled */
int scaled_endings_on(void) { return FEATURE(SCALED_ENDINGS) && widescreen_in_ending(); }

#define KIND_PICTURE  73
#define KIND_PICTURE_2 77
#define KIND_PICTURE_3 78
#define KIND_PROFILE  74
#define MARGIN        48

/* this frame's scene is drawn 1.25 times larger (core/task/ending.c decides, as its layers are drawn) */
static int scene_scaled;
void scaled_endings_scene(int scaled) { scene_scaled = scaled; }

/* a picture of a scaled scene, drawn 1.25 times larger with the sprites linked to it (core/object/sprite.c) */
int scaled_endings_oversize(uint32_t sp) {
    return scaled_endings_on() && scene_scaled && (RB(sp + 2) == KIND_PICTURE || RB(sp + 2) == KIND_PICTURE_2 || RB(sp + 2) == KIND_PICTURE_3);
}

/* a profile picture at a 4:3 side: moved out by the margin, to the edge it stands at (0: not one) */
int scaled_endings_profile_shift(uint32_t sp) {
    if (!scaled_endings_on() || RB(sp + 2) != KIND_PROFILE) return 0;
    return (int16_t)RH(sp + 18) < 192 ? -MARGIN : MARGIN;
}
