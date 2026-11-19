/* The single object at 0x092E5100 (taken by 0x088858D8, screen_steps.c) drawn in the text pass
 * (0x0880D848, frame.c): its six slots (+0x20 ... +0x70, 16 bytes each: +0 in use, +2 x, +4 y, +12 a
 * record whose +4 gives the slot's kind in its top four bits and its picture in the low twelve). It
 * carries the logo between the VS screen and the fight (STREET FIGHTER ALPHA3, its words sliding in)
 * and that wipe's fading shapes.
 *
 *   kinds 4 and 0xB: a picture (0x0880D988) 64 left and 16 above the slot's place, from the picture
 *     list at 0x08E75AD8 + 28 (12 bytes a picture), flipped by the object's +11
 *   kind 0xA: the fading shape 0x0880DA78 (still translated), flipped the same way
 *
 * WIDE NATIVE: a word waiting at the 4:3 picture's right edge (x 384 or more) to slide in showed in the
 * right margin (a stray "F" of FIGHTER over the VS screen); such a picture is not drawn until it moves
 * in. Only the drawing changes.
 */
#include "../decomp.h"

extern int rt_widescreen;   /* WIDE NATIVE: extensions/widescreen.c (extensions_off.c: 0) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_089E0CF4(Cpu *c);   /* selects a texture page (a0 its descriptor) */
void f_089E1464(Cpu *c);   /* a 2D library setting: a0 which, a1 its value */
void f_0880E150(Cpu *c);   /* a sprite's picture drawn as a grid of cells (a0 draw state, a1, a2 picture, a3, t0 flag) */
void f_0880DA78(Cpu *c);   /* a fading shape (a0 draw state, a1 the slot) */
void f_0880D988(Cpu *c);   /* this file: a slot's picture */
void f_0880D8C8(Cpu *c);   /* this file: a slot */

#define SINGLE      0x092E5100u
#define DRAW_STATE  0x08AD9320u
#define PICTURES    0x08E75AD8u   /* +28: the picture list */
#define TEXTURES    0x08E75A18u   /* +28: its texture page */
enum { DS_X = 0, DS_Y = 2, DS_DEPTH = 44, DS_COLOUR = 48, DS_FLIP_X = 56, DS_FLIP_Y = 60,
       DS_BLEND = 85, DS_EFFECT = 86, DS_SCALED = 87 };

static uint32_t slot_record(gaddr slot) { return *H(*W(slot + 12) + 4); }

/* (draw state, object, slot) */
static void slot_picture(Cpu *c, gaddr ds, gaddr obj, gaddr slot) {
    int16_t x = (int16_t)(*H(slot + 2) - 64);
    *H(ds + DS_X) = (uint16_t)x;
    *H(ds + DS_Y) = (uint16_t)(*H(slot + 4) - 16);
    if (rt_widescreen && x >= 384) return;          /* waiting past the 4:3 edge: not yet */
    uint32_t n = slot_record(slot) & 0xFFF;
    gaddr list = *W(PICTURES + 28), picture = list + n * 12u;
    CALL1(f_089E0CF4, *W(TEXTURES + 28));
    CALL2(f_089E1464, 0, 0x1E);
    CALL2(f_089E1464, 1, 0x1E);
    *W(ds + DS_FLIP_X) = *B(obj + 11) & 1u;
    *W(ds + DS_FLIP_Y) = (uint32_t)(*B(obj + 11) >> 1);
    *W(ds + DS_COLOUR) = 0xFFFFFFFFu;
    *B(ds + DS_BLEND) = 0;
    *W(ds + DS_DEPTH) = 7;
    *B(ds + DS_EFFECT) = 0;
    *B(ds + DS_SCALED) = 0;
    c->r[4] = ds; c->r[5] = 0; c->r[6] = picture; c->r[7] = list; c->r[8] = 1;
    f_0880E150(c);
}

/* (object, slot): drawn by its kind */
static void draw_slot(Cpu *c, gaddr obj, gaddr slot) {
    if (*B(slot) == 0) return;
    uint32_t rec = slot_record(slot);
    if (rec == 0xFFFF) return;
    uint32_t kind = (rec & 0xF000) >> 12;
    if (kind == 4 || kind == 0xB) { CALL3(f_0880D988, DRAW_STATE, obj, slot); return; }
    if (kind == 0xA) {
        *W(DRAW_STATE + DS_FLIP_X) = *B(obj + 11) & 1u;
        *W(DRAW_STATE + DS_FLIP_Y) = (uint32_t)(*B(obj + 11) >> 1);
        CALL2(f_0880DA78, DRAW_STATE, slot);
    }
}

/* The object in use (+0) and shown (+1): its six slots in order */
static void draw_single(Cpu *c) {
    gaddr obj = SINGLE;
    if (*B(obj) == 0 || *B(obj + 1) == 0) return;
    for (gaddr slot = obj + 0x20; slot <= obj + 0x70; slot += 0x10) CALL2(f_0880D8C8, obj, slot);
}

SFA_FUNC(0880D848, single_object_draw) { c->r[29] -= 0x20; draw_single(c); c->r[29] += 0x20; }
SFA_FUNC(0880D8C8, single_object_draw_slot) { gaddr o = ARG(0), s = ARG(1); c->r[29] -= 0x20; draw_slot(c, o, s); c->r[29] += 0x20; }
SFA_FUNC(0880D988, single_object_slot_picture) {
    gaddr ds = ARG(0), o = ARG(1), s = ARG(2);
    c->r[29] -= 0x40; slot_picture(c, ds, o, s); c->r[29] += 0x40;
}
