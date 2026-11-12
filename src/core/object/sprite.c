/* Drawing one queued sprite (0x0880C308): a small tree of sprites, each set up in the shared draw state
 * and handed to a drawer chosen by its kind.
 */
#include "../decomp.h"

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_089E0C4C(Cpu *c);   /* switches a GE drawing mode on (1) or off (0) around a sprite that asks for it */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

#define DRAW_STATE     0x08AD9320u   /* where a sprite is set up for its drawer */
#define DRAWERS        0x08A04B48u   /* u32[]: drawer by sprite kind (+148) */
#define ANCHORS        0x08A04014u   /* u32[]: anchor records by anchor kind (+14); +18 x, +22 y */
#define DRAW_OFF       0x08E7602Cu   /* u32: nonzero = no sprites drawn */
#define MODE_SWITCH_OFF 0x08E759F8u  /* u16: nonzero = leave the GE mode alone for flagged sprites */
#define SCROLL         0x092E5F78u   /* +28: a scroll value (16.16) for anchor kind 2 */
#define BATTLE         0x092E1228u   /* +74: the screen shake */

/* A sprite (offsets used here) */
enum {
    SP_ACTIVE = 0, SP_SHOWN = 1, SP_GROUP = 8, SP_FLIP = 11, SP_ANCHOR = 14, SP_SHAKES = 15,
    SP_X = 18, SP_Y = 22, SP_ATTR = 24, SP_SCROLL_X = 38, SP_NEXT = 40, SP_AFTER = 44,
    SP_BEFORE_2 = 48, SP_BEFORE_1 = 52, SP_STATE = 86, SP_DRAWER = 148, SP_DONE = 164,
    SP_COLOUR = 160, SP_BLEND = 168, SP_GE_MODE = 169,
};
#define GROUP_ALONE 3   /* a sprite that does not draw the sprites linked to it */

/* The draw state (offsets used here) */
enum { DS_X = 0, DS_Y = 2, DS_ATTR = 44, DS_COLOUR = 48, DS_FLIP_X = 56, DS_FLIP_Y = 60,
       DS_POS_X = 80, DS_POS_Y = 82, DS_BLEND = 85 };

static inline int16_t S16(gaddr a) { return (int16_t)*H(a); }

/* Moves the sprite's position by its anchor: a scrolling anchor (kind 2) first adds its share of the
   scroll; then the anchor's own offset; then the screen shake if the sprite shakes. */
static void sprite_anchor(gaddr ds, gaddr sp) {
    uint8_t anchor = *B(sp + SP_ANCHOR);
    if (anchor != 0) {
        if (anchor == 2) {
            int16_t share = S16(sp + SP_SCROLL_X);
            if (share != 0) {
                int32_t scrolled = (int32_t)((uint32_t)*W(SCROLL + 28) * (uint32_t)(int32_t)share) >> 16;
                *H(ds + DS_POS_X) = (uint16_t)(S16(ds + DS_POS_X) + scrolled);
                anchor = *B(sp + SP_ANCHOR);
            }
        }
        gaddr rec = *W(ANCHORS + anchor * 4u);
        *H(ds + DS_POS_X) = (uint16_t)(S16(ds + DS_POS_X) - S16(rec + 18));
        *H(ds + DS_POS_Y) = (uint16_t)(S16(ds + DS_POS_Y) + S16(rec + 22));
    }
    if (*B(sp + SP_SHAKES) != 0)
        *H(ds + DS_POS_Y) = (uint16_t)(S16(ds + DS_POS_Y) - S16(BATTLE + 74));
}

static void switch_ge_mode(Cpu *c, gaddr sp, uint32_t on) {
    if ((int8_t)*B(sp + SP_GE_MODE) != 0 && *H(MODE_SWITCH_OFF) == 0) CALL1(f_089E0C4C, on);
}

void f_0880C308(Cpu *c);   /* this function: the linked sprites go through it, so --reference-only applies */
void f_0880E6E8(Cpu *c);   /* sprite_anchor */

/* Draws the sprite at `sp` if it is active and shown: the two sprites linked before it, itself, then
   (unless it stands alone) the one linked after it and the next in its chain. */
static void draw_sprite(Cpu *c, gaddr sp) {
    if (*B(sp + SP_ACTIVE) == 0 || *B(sp + SP_SHOWN) == 0 || *W(DRAW_OFF) != 0) return;
    gaddr ds = DRAW_STATE;
    if (*B(sp + SP_GROUP) != GROUP_ALONE) {
        if (*W(sp + SP_BEFORE_1) != 0) CALL1(f_0880C308, *W(sp + SP_BEFORE_1));
        if (*W(sp + SP_BEFORE_2) != 0) CALL1(f_0880C308, *W(sp + SP_BEFORE_2));
    }
    *H(ds + DS_POS_X) = (uint16_t)S16(sp + SP_X);
    *H(ds + DS_POS_Y) = (uint16_t)(0xF0 - S16(sp + SP_Y));
    CALL2(f_0880E6E8, ds, sp);
    *H(ds + DS_X) = *H(ds + DS_POS_X);
    *H(ds + DS_Y) = *H(ds + DS_POS_Y);
    uint8_t flip = *B(sp + SP_FLIP);
    *W(ds + DS_FLIP_X) = flip & 1u;
    *W(ds + DS_FLIP_Y) = flip >> 1;
    const uint8_t *colour = B(sp + SP_COLOUR);
    *W(ds + DS_COLOUR) = (uint32_t)colour[0] << 24 | (uint32_t)colour[1] << 16 | (uint32_t)colour[2] << 8 | colour[3];
    *B(ds + DS_BLEND) = *B(sp + SP_BLEND);
    *W(ds + DS_ATTR) = *H(sp + SP_ATTR);
    if (*H(sp + SP_STATE) == 5) *W(sp + SP_DONE) = 1;

    switch_ge_mode(c, sp, 1);
    c->r[4] = ds; c->r[5] = sp;
    rt_call_indirect(c, *W(DRAWERS + *W(sp + SP_DRAWER) * 4u));
    switch_ge_mode(c, sp, 0);

    if (*B(sp + SP_GROUP) == GROUP_ALONE) return;
    if (*W(sp + SP_AFTER) != 0) CALL1(f_0880C308, *W(sp + SP_AFTER));
    if (*W(sp + SP_NEXT) != 0) CALL1(f_0880C308, *W(sp + SP_NEXT));
}

SFA_FUNC(0880C308, game_draw_sprite) { c->r[29] -= 0x20; draw_sprite(c, ARG(0)); c->r[29] += 0x20; }
SFA_FUNC(0880E6E8, game_sprite_anchor) { sprite_anchor(ARG(0), ARG(1)); }
