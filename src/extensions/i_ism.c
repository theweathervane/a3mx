/* I-ISM extras (the i_ism feature): what the game left out for the entry characters (EDIT / ENTRY,
 * shown as I-ISM).
 *
 * 1. The wipe into the fight. Between the VS screen and the fight three columns roll like a slot
 *    machine and settle into the ISM's letter, X, A or V, in its colour, and the STREET FIGHTER ALPHA3
 *    logo slides in with that ISM's gradient (task/object: screen_steps.c's logo_start_step). An entry
 *    plays as its ISM underneath (an A-ISM entry got A's wipe), so I-ISM had none of its own. Now an
 *    entry gets an I: the same rolling columns (A's animation) with I shapes in yellow, and a gold logo.
 *    The I was measured from Dan's WIDE NATIVE mock-up: a serifed I across the 4:3 picture (its serifs
 *    from x 16 to 368, 32 high; its stem 168 - 216), the yellow around it; the margins of WIDE NATIVE
 *    are filled by shapes.c's rule as for the other letters. NORMAL and WIDE draw the same shapes.
 *
 * The wipe is for one fighter, as the game picks it (logo_ism in screen_steps.c): fighter 3 when BATTLE
 * + 56 is 3, fighter 2 when BATTLE + 48 is set, else fighter 1. It plays an entry when its player (+337,
 * 0 or 1) has one in use (0x09358528 + 0x354 n: the entry a player plays as), as the VS screen's ISM
 * label decides (0x08898F5C shows I-ISM, label 0x147, from the same record).
 */
#include "rt.h"
#include <string.h>

void f_0880E434(Cpu *c);
void shapes_draw(Cpu *c, uint32_t place, const uint16_t *quads, uint32_t rgb, uint32_t alpha);   /* object/shapes.c */

#define BATTLE        0x092E1228u
#define FIGHTER(k)    (0x092E3EA0u + 0x498u * (uint32_t)(k))
#define ENTRIES       0x09358528u       /* the entry each player plays as (+0 in use) */
#define ENTRY_SIZE    0x354u
#define SINGLE        0x092E5100u       /* the logo's object: +0 in use, +3 its type */
#define LOGO_TYPE     0x0D
#define PALETTES      0x092DF980u       /* 16 colours (u16) a palette */
#define PALETTES_DIRTY 0x092E0980u
#define LOGO_PALETTE  0x1Eu

/* ---- the colours ---- */

/* The wipe's yellow (0xRRGGBB, as the game's own green 0x00D000), from the mock-ups: #F3E40C */
#define I_YELLOW      0xF3E40Cu

/* The logo's gold, for A's green palette entry by entry. X's, A's and V's gradients run from nearly
   black (STREET's fill: #210000, #002000, #000031) to the wipe's own colour in the middle (the H of
   ALPHA), so the gold one runs from a dark brown #311800 through #C87B01 (Dan's mock-up) to the wipe's
   yellow #F3E40C (5 - 15). 4 is the letters' outline, a bright contrasting hue as theirs are (orange,
   yellow-green, cyan): magenta #B53184, from the World Tour map's NEXT STAGE bar; 2 and 3 are its
   corner pixels, in the outline's own hue as theirs are (X's are a darker orange, V's a darker
   cyan): purple #943184 and pink #C64273, Dan's picks from the same bar; 1 is the yellow; 0 clear. */
static const uint8_t GOLD[16][3] = {
    { 0x00, 0x00, 0x00 }, { 0xF3, 0xE4, 0x0C }, { 0x94, 0x31, 0x84 }, { 0xC6, 0x42, 0x73 },
    { 0xB5, 0x31, 0x84 }, { 0x31, 0x18, 0x00 }, { 0x4A, 0x28, 0x00 }, { 0x63, 0x39, 0x00 },
    { 0x7C, 0x4A, 0x00 }, { 0x96, 0x5A, 0x01 }, { 0xAF, 0x6A, 0x01 }, { 0xC8, 0x7B, 0x01 },
    { 0xD3, 0x95, 0x04 }, { 0xDE, 0xB0, 0x06 }, { 0xE8, 0xCA, 0x09 }, { 0xF3, 0xE4, 0x0C },
};

/* ---- the I ---- */

/* Quads (4 corners: top left, top right, bottom right, bottom left) of each of the three columns, 128
   wide and 256 high like the other letters' (object/shapes.c), in yellow; what they leave open is the
   I. At rest a column's y 16 is the screen's top. */
#define BAND_TOP    0, 0, 128, 0, 128, 16, 0, 16
#define BAND_BOTTOM 0, 232, 128, 232, 128, 256, 0, 256
#define MIDDLE      0, 48, 128, 48, 128, 200, 0, 200
static const uint16_t I_LEFT[] = {
    BAND_TOP, 0, 16, 16, 16, 64, 48, 0, 48, MIDDLE, 0, 200, 64, 200, 16, 232, 0, 232, BAND_BOTTOM, 0xFFFF };
static const uint16_t I_MIDDLE[] = {
    BAND_TOP, 0, 48, 40, 48, 40, 200, 0, 200, 88, 48, 128, 48, 128, 200, 88, 200, BAND_BOTTOM, 0xFFFF };
static const uint16_t I_RIGHT[] = {
    BAND_TOP, 112, 16, 128, 16, 128, 48, 64, 48, MIDDLE, 64, 200, 128, 200, 128, 232, 112, 232, BAND_BOTTOM, 0xFFFF };
static const uint16_t *const I_SHAPES[3] = { I_LEFT, I_MIDDLE, I_RIGHT };

/* ---- which fighter ---- */

static int wipe_is_entry(void) {
    uint32_t f = (int16_t)RH(BATTLE + 56) == 3 ? FIGHTER(2) : (int8_t)RB(BATTLE + 48) != 0 ? FIGHTER(1) : FIGHTER(0);
    int player = (int8_t)RB(f + 337);
    return player >= 0 && player < 2 && RB(ENTRIES + ENTRY_SIZE * (uint32_t)player) != 0;
}

static uint16_t rgb555(const uint8_t c[3]) {
    return (uint16_t)(0x8000u | (uint32_t)(c[0] >> 3) | (uint32_t)(c[1] >> 3) << 5 | (uint32_t)(c[2] >> 3) << 10);
}

/* The logo starts (screen_steps.c, 0x08885A44): after the game has loaded the ISM's palette, an entry's
   is gold instead. */
int i_ism_logo_start(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    if (!after || !wipe_is_entry()) return 0;
    for (uint32_t i = 1; i < 16; i++) WH(PALETTES + LOGO_PALETTE * 32u + i * 2u, rgb555(GOLD[i]));
    WW(PALETTES_DIRTY, 1);
    rt_log("i_ism: an entry's wipe: the I and the gold logo");
    return 0;
}

/* A fading shape of the logo's wipe (batch_large.c, 0x0880DA78): while one of an entry's is drawn,
   A's shapes (3 - 5) are drawn as the I by i_ism_shape. */
static int drawing_entry_wipe;
int i_ism_fading_shape(Cpu *c, uint32_t a0, int after) {
    (void)c; (void)a0;
    drawing_entry_wipe = !after && RB(SINGLE) && RB(SINGLE + 3) == LOGO_TYPE && wipe_is_entry();
    return 0;
}

/* A shape drawn (shapes.c, 0x0880E434: a0 its place, a1 the shape, a2 the alpha). */
int i_ism_shape(Cpu *c, uint32_t a0, int after) {
    uint32_t shape = c->r[5], alpha = c->r[6];
    if (after || !drawing_entry_wipe || shape < 3 || shape > 5) return 0;
    c->r[29] -= 0x40;
    shapes_draw(c, a0, I_SHAPES[shape - 3], I_YELLOW, alpha);
    c->r[29] += 0x40;
    return 1;
}
