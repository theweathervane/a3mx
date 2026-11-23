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
 *     frame); in the endings' own scenes its pictures (sprites of kinds 73, 77 and 78: the close-ups,
 *     skulls and statues, the bursts and speed lines, built of pieces side by side) with it, and what is
 *     linked to them; over a stage's layers a picture is a character's art and keeps its size;
 *   - the profile pictures at the 4:3 sides (kind 74, at x 64 or 320) move out to the 16:9 edges;
 *   - everything else (characters, props) is drawn across the 16:9 screen, except a sprite waiting past
 *     a 4:3 side to come in, which is cut there (core/object/sprite.c).
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

/* a picture of a scaled scene (core/object/sprite.c): 1 drawn 1.25 times larger with the sprites linked
   to it: in the endings' own scenes (ending kind 2: the 24-tile-wide map), whose pictures are parts of
   the scene (a statue's face on its body, a burst behind a fighter). 2 over a stage's layers (kinds 0
   and 1: the sunset behind Sagat, the stage behind the last portrait), where a picture is a character's
   art standing on its own: the layers are scaled and it keeps its size, unless it reaches a 4:3 side
   (art cut straight there, sprite_drawers.c measures it), when it grows with them as before. 0 not one. */
#define ENDING_KIND (0x09358BE0u + 308)   /* u32: 0 / 1 a stage's layers, 2 the endings' map (core/task/ending.c) */
int scaled_endings_oversize(uint32_t sp) {
    if (!scaled_endings_on() || !scene_scaled) return 0;
    if (RB(sp + 2) != KIND_PICTURE && RB(sp + 2) != KIND_PICTURE_2 && RB(sp + 2) != KIND_PICTURE_3) return 0;
    return RW(ENDING_KIND) == 2 ? 1 : 2;
}

/* a profile picture at a 4:3 side: moved out by the margin, to the edge it stands at (0: not one) */
int scaled_endings_profile_shift(uint32_t sp) {
    if (!scaled_endings_on() || RB(sp + 2) != KIND_PROFILE) return 0;
    return (int16_t)RH(sp + 18) < 192 ? -MARGIN : MARGIN;
}
