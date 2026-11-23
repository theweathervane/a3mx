/* Scaled endings (the scaled_endings feature, WIDE NATIVE): the endings fill the 16:9 screen.
 *
 * An ending's scene is a layer of the 24-tile-wide map (core/stage/tiles.c, 0x0881B7C8: two 12-tile
 * scenes side by side, 384 pixels each) with pictures drawn as sprites over it (core/object/sprite.c).
 * Other endings' scenes are a stage's layers (16- and 32-pixel tiles, pictures). In WIDE NATIVE
 * (widescreen.c tells an ending's frame, widescreen_in_ending):
 *   - a scene with art of its own past the 4:3 sides (a layer that scrolls across its map, a stage
 *     wider than the screen) shows it: the picture carried on;
 *   - otherwise (the map standing still on one half, the other half being another scene; layers whose
 *     columns past the sides are bare) the scene is drawn 1.25 times larger about the middle of its top,
 *     as the super finish's backdrops are, so its 384 fill the 480 (core/task/ending.c
 *     decides); in the endings' own scenes its pictures (sprites of kinds 73, 77 and 78: the close-ups,
 *     skulls and statues, the bursts and speed lines, built of pieces side by side) with it, and what is
 *     linked to them; over a stage's layers a backdrop reaching a 4:3 side grows with it, a character's
 *     art and the last portrait keep their size (scaled_endings_oversize);
 *   - a scene is scaled about the middle of its top: its top row stays, its bottom goes under the text
 *     band; whether it is scaled is decided when it is cut to and kept while its camera pans;
 *   - over a stage drawn as it is, a backdrop picture covering the whole 4:3 picture is scaled on its own;
 *   - a narrow backdrop standing alone in the middle (Dee Jay's speed lines) is mirrored out to the sides,
 *     and the black sides that narrow a scene to a panel go on to the 16:9 edges (widescreen.c);
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
static int scene_scaled, scene_bare;
void scaled_endings_scene(int scaled) { scene_scaled = scaled; }
/* no layer of this frame's scene is shown (core/task/ending.c): its pictures stand on black */
void scaled_endings_bare(int bare) { scene_bare = bare; }
int scaled_endings_scene_bare(void) { return scene_bare; }

/* an ending's picture (core/object/sprite.c), what is done with it and the sprites linked to it:
     1  drawn 1.25 times larger with its scene: in the endings' own scenes (ending kind 2: the 24-tile-wide
        map), whose pictures are parts of the scene (a statue's face on its body, a burst behind a fighter);
     2  over a scaled stage's layers (kinds 0 and 1): measured (core/object/sprite_drawers.c), it grows
        with the layers when it covers the whole 4:3 picture, or is a backdrop's piece at a side; else
        keeps its size, standing on the stage (a character sliding in, Eagle's and Balrog's last pictures);
     4  over a stage drawn as it is (not scaled), a backdrop (kind 73 type 0): grows if it covers the
        whole 4:3 picture, hiding the stage (Juni before Chun-Li's portrait), else is drawn as any sprite;
     0  not one of these. */
#define ENDING_KIND (0x09358BE0u + 308)   /* u32: 0 / 1 a stage's layers, 2 the endings' map (core/task/ending.c) */
int scaled_endings_oversize(uint32_t sp) {
    if (!scaled_endings_on()) return 0;
    int kind = RB(sp + 2);
    if (kind != KIND_PICTURE && kind != KIND_PICTURE_2 && kind != KIND_PICTURE_3) return 0;
    int backdrop = kind == KIND_PICTURE && RB(sp + 3) == 0;
    if (!scene_scaled) return backdrop ? 4 : 0;
    return RW(ENDING_KIND) == 2 ? 1 : 2;
}

/* a profile picture at a 4:3 side: moved out by the margin, to the edge it stands at (0: not one) */
int scaled_endings_profile_shift(uint32_t sp) {
    if (!scaled_endings_on() || RB(sp + 2) != KIND_PROFILE) return 0;
    return (int16_t)RH(sp + 18) < 192 ? -MARGIN : MARGIN;
}
