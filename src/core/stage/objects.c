/* The stage's objects: the animated or moving pieces of a stage that are not its tile layers (Ryu's
 * straw and rocks, Ken's crowd, Charlie's planes, Guile's trees ...).
 *
 * Each stage has a list of 16-byte records (StageObjectDef, table 0x08A143D8 by stage number), read once
 * when the stage is set up: one object of pool 5 (object_make_16, 0x09514B70, 16 objects) per record,
 * its routine picked by its kind (+2) from the table 0x08A101F0 (run by lists.c's second list). The
 * BATTLE stages (0x25 - 0x28) have none. docs/STAGES.md lists every stage's records.
 */
#include "../decomp.h"
#include "../object/step.h"

#define BATTLE        0x092E1228u   /* +0 s16: the stage being fought on */
#define STAGE_OBJECTS 0x08A143D8u   /* u32[42]: each stage's StageObjectDef list */

/* One object of a stage, as the game's data has it. */
typedef struct {
    uint8_t kind;       /* bits 0-6: the object's kind (+2, its routine); bit 7: the last record */
    uint8_t type;       /* +3: which of its kind (the routines pick its animation by it) */
    uint8_t layer;      /* +9: the sprite layer it is drawn in */
    uint8_t value_78;   /* +78: a byte the routines read (a delay or a variant) */
    uint8_t camera;     /* +14: 0 fixed to the screen, 1 moved with the fighters' camera, 2 by its depth */
    uint8_t value_11;   /* +11 */
    int16_t x, y;       /* +18 / +22: place on the stage */
    int16_t value_26;   /* +26 */
    int16_t value_24;   /* +24 */
    int16_t depth;      /* +38: how far it moves with the camera (camera 2) */
} StageObjectDef;
GUEST_STRUCT_SIZE(StageObjectDef, 16);

#define DEF_LAST 0x80u

void f_08822F1C(Cpu *c);   /* object_make_16 (pool.c): (kind, type) -> a new object of pool 5, or 0 */
void f_0882196C(Cpu *c);   /* object_set_animation (animation.c): (object, animation) */
void f_088219B0(Cpu *c);   /* object_animate (animation.c): (object) one frame on */
void f_088215C4(Cpu *c);   /* object_draw (frame.c): (object) */
void f_088214E8(Cpu *c);   /* object_draw_if_on_screen_margin (frame.c): (object, margin) */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen (frame.c): (object) */
void f_08823220(Cpu *c);   /* record_free_5 (misc.c): (object) back to pool 5 */
void f_08821368(Cpu *c);   /* game_random (fight.c) */
void f_0884D870(Cpu *c); void f_08852670(Cpu *c);   /* below */

#define ANIMATIONS    0x08A144A0u   /* u32[]: the stage objects' animations, by type (+3) */
#define LAYER_1       0x092E88A0u   /* the fighters' camera layer (+8 / +12 x / y, +32 / +36 their start) */
#define LAYER_2       0x092E8940u
#define SCROLL        0x092E5F78u   /* +112: the screen shake's x (16.16) */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }

/* Stages whose objects are drawn with +86 = 15 and +148 = 0; the others' have 4 and 7. */
static int bright_stage(int stage) {
    switch (stage) {
    case 0x02: case 0x06: case 0x07: case 0x08: case 0x09: case 0x10: case 0x14:
    case 0x17: case 0x18: case 0x20: case 0x22: case 0x24: case 0x29:
        return 1;
    }
    return 0;
}

/* Makes the stage's objects from its list, until the last record or until the pool is full. */
static void make_stage_objects(Cpu *c) {
    int stage = *(int16_t *)guest_ptr(BATTLE);
    gaddr def = *W(STAGE_OBJECTS + (uint32_t)stage * 4u);
    if (stage >= 0x25 && stage <= 0x28) return;              /* BATTLE1 - 4 */
    for (;;) {
        const StageObjectDef *d = guest_ptr(def);
        gaddr o = CALL2(f_08822F1C, d->kind & 0x7Fu, d->type);
        if (o == 0) return;
        *B(o) = 1;
        *H(o + 18) = (uint16_t)d->x;
        *H(o + 22) = (uint16_t)d->y;
        *B(o + 9) = d->layer;
        *B(o + 78) = d->value_78;
        *B(o + 11) = d->value_11;
        *B(o + 14) = d->camera;
        *H(o + 26) = (uint16_t)d->value_26;
        *H(o + 24) = (uint16_t)d->value_24;
        *H(o + 38) = (uint16_t)d->depth;
        *W(o + 164) = (stage == 0x06 || stage == 0x09 || stage == 0x18) ? 0 : 1;
        if (bright_stage(stage)) { *H(o + 86) = 15; *W(o + 148) = 0; }
        else                     { *H(o + 86) = 4;  *W(o + 148) = 7; }
        if (d->kind & DEF_LAST) return;
        def += sizeof(StageObjectDef);
    }
}

/* (): the stage's objects made (from the fight's setup and the screens that show a stage). */
SFA_FUNC(0884AE40, stage_objects_make) {
    c->r[29] -= 0x40;
    make_stage_objects(c);
    c->r[29] += 0x40;
}

/* ---- the stage objects' routines, by kind (table 0x08A101F0) ----
 * Common fields: +4 (or +7) the routine's step, +15 shown, +12 the animation's frame, +16 / +20 the
 * place (16.16; the stage's x / y at +18 / +22), +104 / +108 where it started. */

/* Neither BATTLE + 37 nor + 52 stops the fight's animations. */
static int animating(void) { return (S8(BATTLE + 37) | S8(BATTLE + 52)) == 0; }

/* How far a camera layer has come back from where it started (16.16): x with the screen shake. */
static int32_t layer_back_x(gaddr layer) {
    return (int32_t)(*W(layer + 8) - *W(layer + 32) + *W(SCROLL + 112));
}
static int32_t layer_back_y(gaddr layer) { return (int32_t)(*W(layer + 12) - *W(layer + 36)); }

/* a x b / 16 as the PSP works it: the low 32 bits of the product, divided rounding towards zero. */
static int32_t scale16(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b) / 16; }

/* The first step most kinds share: shown, its animation from the start (frame 0, or with `frame` set
   frame 0xFF and +13 = `frame`), from `table` by its type (+3). */
static void start_animation(Cpu *c, gaddr o, gaddr table, uint8_t frame) {
    if (frame) { *B(o + 12) = 0xFF; *B(o + 13) = frame; }
    else *B(o + 12) = 0;
    CALL2(f_0882196C, o, *W(table + (uint32_t)*B(o + 3) * 4u));
}

/* ---- "follow": animated, drawn only near the screen when +96 (16 x +78) gives a margin, and moving
   back with the fighters' layer at +100's rate (in sixteenths) when it follows ---- */
enum { FOLLOW_UNLESS_CAMERA, FOLLOW_IF_26, FOLLOW_ALWAYS };   /* follows while +14 is 0 / while +26 is not 0 / always */
enum { RATE_7BITS, RATE_SIGNED, RATE_7BITS_SIGNED };   /* +100: bits 0-6 the rate, y one for one with
                                                     bit 7 / the whole word, y one for one when negative /
                                                     bits 0-6, y one for one when negative */
enum { DRAW_NEAR, DRAW_NEAR_OR_ON_SCREEN, DRAW_ALWAYS };   /* margin set: near the screen; else drawn /
                                                     drawn if on the screen / always drawn */
typedef struct { gaddr table; uint8_t follow, rate, frame, draw, x_rate, y_rate; } FollowKind;   /* x_rate / y_rate: a fixed rate (sixteenths) in place of +100's, 0 = +100's */

static int follows(gaddr o, const FollowKind *k) {
    if (k->follow == FOLLOW_ALWAYS) return 1;
    return k->follow == FOLLOW_UNLESS_CAMERA ? *B(o + 14) == 0 : *H(o + 26) != 0;
}

static void follow_kind(Cpu *c, const FollowKind *k) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *W(o + 96) = (uint32_t)((int32_t)S8(o + 78) << 4);
        if (follows(o, k)) { uint8_t rate = *B(o + 11); *B(o + 11) = 0; *W(o + 100) = rate; }
        *W(o + 104) = *W(o + 16); *W(o + 108) = *W(o + 20);
        start_animation(c, o, k->table, k->frame);
    } else if (step == 1) {
        if (follows(o, k)) {
            int32_t rate = (int32_t)*W(o + 100), dx, dy = layer_back_y(LAYER_1);
            if (k->rate == RATE_7BITS) {
                dx = scale16(layer_back_x(LAYER_1), rate & 0x7F);
                if ((rate & 0x80) == 0) dy = scale16(dy, rate);
            } else if (k->rate == RATE_SIGNED) {
                dx = scale16(layer_back_x(LAYER_1), k->x_rate ? k->x_rate : rate);
                if (rate >= 0) dy = scale16(dy, k->y_rate ? k->y_rate : rate);
            } else {
                dx = scale16(layer_back_x(LAYER_1), rate & 0x7F);
                if (rate >= 0) dy = scale16(dy, rate & 0x7F);
            }
            *W(o + 16) = *W(o + 104) + (uint32_t)dx;
            *W(o + 20) = *W(o + 108) + (uint32_t)dy;
        }
        if (animating()) CALL1(f_088219B0, o);
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    uint32_t margin = *W(o + 96);
    if (k->draw == DRAW_ALWAYS) CALL1(f_088215C4, o);
    else if (margin != 0) CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)margin);
    else if (k->draw == DRAW_NEAR_OR_ON_SCREEN) CALL1(f_08821424, o);
    else CALL1(f_088215C4, o);
    c->r[29] += 0x20;
}

/* ---- "plain": animated, drawn only near the screen when +96 (16 x +78) gives a margin ---- */
static void plain_kind_ex(Cpu *c, gaddr table, uint8_t frame, int save_start) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *W(o + 96) = (uint32_t)((int32_t)S8(o + 78) << 4);
        if (save_start) { *W(o + 104) = *W(o + 16); *W(o + 108) = *W(o + 20); }
        start_animation(c, o, table, frame);
    } else if (step == 1) {
        if (animating()) CALL1(f_088219B0, o);
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    uint32_t margin = *W(o + 96);
    if (margin == 0) CALL1(f_088215C4, o);
    else CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)margin);
    c->r[29] += 0x20;
}

static void plain_kind(Cpu *c, gaddr table, uint8_t frame) { plain_kind_ex(c, table, frame, 0); }

/* Kind 60 (CODY): plain, also keeping where it started (+104 / +108). */
SFA_FUNC(08851A6C, stage_object_kind_60) { plain_kind_ex(c, 0x08A15BB0u, 0, 1); }

/* ---- "pause at the line" (step at +7): animated only while the fighters' layer stays on the same
   side of x +132; each time it crosses, the animation waits a frame ---- */
static void line_kind(Cpu *c, gaddr table) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (*B(o + 7) == 0) {
        *B(o + 15) = 1; *B(o + 7) = 1;
        start_animation(c, o, table, 0);
        *B(o + 136) = 0;
    }
    uint8_t step = *B(o + 7);
    if (step == 1) {
        int8_t side = S16(o + 132) < S16(LAYER_1 + 18) ? 1 : 0;
        if (side == S8(o + 136)) { if (animating()) CALL1(f_088219B0, o); }
        else *B(o + 136) = (uint8_t)side;
        CALL1(f_088215C4, o);
    } else if (step != 0) {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* ---- "swap at the line" (step at +7). In WIDE NATIVE each of these kinds has a twin showing the
   other look (extensions/widescreen_twins.c), since the change can happen in the 16:9 picture ---- */
/* ---- "swap at the line" (step at +7): two looks kept in a block at +96, the record's own (look 0)
   and a fixed second one (look 1); while the fighters' layer is past x +132 the object takes look 1,
   else look 0. A look: the animation (+28, +74 - +77), place (+18 / +22), layer (+9), camera (+14),
   +24 and +38 ---- */
typedef struct {
    gaddr table; uint8_t frame;
    int16_t line;                                  /* +132 */
    uint8_t anim_1, layer_1, camera_1;             /* look 1's animation number, layer, camera */
    int16_t x_1, y_1, v24_1, v38_1;
    gaddr by_type;                                 /* if set: look 1 and the line from this table of
                                                      14-byte entries by type (layer, animation, camera,
                                                      -, line, x, y, +24, +38) instead */
    uint8_t always_animate;                        /* animates even while the fight is stopped */
} SwapKind;

typedef struct {
    uint32_t anim; uint8_t a74[4];                 /* +28, +74 - +77 */
} AnimState;

enum { LOOK_0 = 0, LOOK_1 = 16, LOOK_POS = 8, LOOK_FLAGS = 32 };   /* offsets in the block at +96 */

static void save_anim(gaddr o, gaddr to) {
    *W(to) = *W(o + 28);
    for (int i = 0; i < 4; i++) *B(to + 4 + (uint32_t)i) = *B(o + 74 + (uint32_t)i);
}

static void take_look(gaddr o, int look) {
    gaddr k = o + 96;
    gaddr a = k + (look ? 16u : 0u), p = k + (look ? 24u : 8u), f = k + (look ? 34u : 32u);
    *W(o + 28) = *W(a);
    for (int i = 0; i < 4; i++) *B(o + 74 + (uint32_t)i) = *B(a + 4 + (uint32_t)i);
    *H(o + 18) = *H(p); *H(o + 22) = *H(p + 2);
    *B(o + 9) = *B(f); *B(o + 14) = *B(f + 1);
    *H(o + 24) = *H(p + 4); *H(o + 38) = *H(p + 6);
}

static void swap_kind(Cpu *c, const SwapKind *s) {
    gaddr o = ARG(0), k = o + 96;
    c->r[29] -= 0x20;
    if (*B(o + 7) == 0) {
        *B(o + 15) = 1; *B(o + 7) = 1;
        if (s->frame) { *B(o + 12) = 0xFF; *B(o + 13) = s->frame; } else *B(o + 12) = 0;
        *H(k + 8) = *H(o + 18); *H(k + 10) = *H(o + 22);           /* look 0: the record's */
        *B(k + 32) = *B(o + 9);
        *B(k + 38) = *B(o + 78);                                   /* look 0's animation number */
        *B(k + 33) = *B(o + 14);
        *H(k + 12) = *H(o + 24); *H(k + 14) = *H(o + 38);
        if (s->by_type) {
            gaddr e = s->by_type + (uint32_t)*B(o + 3) * 14u;
            *H(k + 36) = *H(e + 4);
            *H(k + 24) = *H(e + 6); *H(k + 26) = *H(e + 8);
            *B(k + 34) = *B(e); *B(k + 39) = *B(e + 1); *B(k + 35) = *B(e + 2);
            *H(k + 28) = *H(e + 10); *H(k + 30) = *H(e + 12);
        } else {
            *H(k + 36) = (uint16_t)s->line;
            *B(k + 39) = s->anim_1;
            *H(k + 24) = (uint16_t)s->x_1; *H(k + 26) = (uint16_t)s->y_1;
            *B(k + 34) = s->layer_1; *B(k + 35) = s->camera_1;
            *H(k + 28) = (uint16_t)s->v24_1; *H(k + 30) = (uint16_t)s->v38_1;
        }
        CALL2(f_0882196C, o, *W(s->table + (uint32_t)(int32_t)S8(k + 39) * 4u));
        save_anim(o, k + 16);
        CALL2(f_0882196C, o, *W(s->table + (uint32_t)(int32_t)S8(k + 38) * 4u));
        save_anim(o, k);
        *B(k + 40) = 0;
    }
    uint8_t step = *B(o + 7);
    if (step == 1) {
        int8_t side = S16(k + 36) < S16(LAYER_1 + 18) ? 1 : 0;
        if (side == S8(k + 40)) { if (s->always_animate || animating()) CALL1(f_088219B0, o); }
        else { *B(k + 40) = (uint8_t)side; take_look(o, side); }
        CALL1(f_088215C4, o);
    } else if (step != 0) {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kind 29 (SAGAT): "swap at the line" with look 1 and the line from a table by type. */
SFA_FUNC_WRAPPED(0884DB20, stage_object_kind_29, widescreen_twins) {
    swap_kind(c, &(const SwapKind){ .table = 0x08A14EC0u, .by_type = 0x08A14EE8u - 0x62u });
}

/* Kind 2: not animated (its first frame only); x moves back with half the fighters' layer, y with all
   of layer 2's. */
SFA_FUNC(0884B2AC, stage_object_kind_2) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *W(o + 104) = *W(o + 16); *W(o + 108) = *W(o + 20);
        start_animation(c, o, ANIMATIONS, 0);
        CALL1(f_088215C4, o);
    } else if (step == 1) {
        *W(o + 16) = *W(o + 104) + (uint32_t)(layer_back_x(LAYER_1) >> 1);
        *W(o + 20) = (uint32_t)layer_back_y(LAYER_2) + *W(o + 108);
        CALL1(f_088215C4, o);
    } else {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* ---- kinds of their own ---- */
#define GAME 0x08E75910u   /* +32 the frame counter, +66 the object loop's counter */
#define CALLBACKS_12 0x09517868u   /* the frame's second callback list (12 of 0x18 bytes) */

/* Kind 17 (GUY): its frame 0xFF / 41 and the animation after its type's; drawn near the screen (48). */
SFA_FUNC(0884CAB0, stage_object_kind_17) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step >= 2) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    if (step == 0) {
        *B(o + 4) = 1; *B(o + 15) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        CALL2(f_0882196C, o, *W(0x08A14B30u + (uint32_t)*B(o + 3) * 4u + 4u));
    }
    CALL2(f_088214E8, o, 0x30);
    c->r[29] += 0x20;
}

/* Kind 21 (BIRDIE): drawn near the screen (32) on every other pass (GAME + 66 + GAME + 32 odd), else
   hidden (+1 = 0). */
SFA_FUNC(0884D000, stage_object_kind_21) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        CALL2(f_0882196C, o, *W(0x08A14C30u + (uint32_t)*B(o + 3) * 4u));
    } else if (step == 1) {
        if (((uint32_t)(int32_t)S8(GAME + 66) + *B(GAME + 32)) & 1) CALL2(f_088214E8, o, 0x20);
        else *B(o + 1) = 0;
    } else {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kinds 24 / 64 (BISON 1 / 2): two places 512 apart (+104, +120); the object stands at the first while
   the fighters' layer is left of it + 64, else at the second. Drawn near the screen (63). */
static void two_places_kind(Cpu *c, gaddr table) {
    gaddr o = ARG(0), k = o + 96;
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0;
        *H(k + 8) = *H(o + 18);
        *H(k + 24) = (uint16_t)(S16(o + 18) + 0x200);
        CALL2(f_0882196C, o, *W(table + (uint32_t)*B(o + 3) * 4u));
    } else if (step == 1) {
        int16_t first = S16(k + 8);
        *H(o + 18) = S16(LAYER_1 + 18) < first + 0x40 ? (uint16_t)first : *H(k + 24);
        if (animating()) CALL1(f_088219B0, o);
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    CALL2(f_088214E8, o, 0x3F);
    c->r[29] += 0x20;
}
SFA_FUNC(0884D2D4, stage_object_kind_24) { two_places_kind(c, 0x08A14DB0u); }
SFA_FUNC(088520D4, stage_object_kind_64) { two_places_kind(c, 0x08A15DD0u); }

/* Kind 59 (MIKA): still (its animation's first frame), drawn every frame. */
SFA_FUNC(0885173C, stage_object_kind_59) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0;
        *W(o + 104) = *W(o + 16); *W(o + 108) = *W(o + 20);
        CALL2(f_0882196C, o, *W(0x08A15A98u + (uint32_t)*B(o + 3) * 4u));
        CALL1(f_088215C4, o);
    } else if (step == 1) {
        CALL1(f_088215C4, o);
    } else {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kind 68 (GUILE): the helicopter. Set up at x 0x500, y 0x58, with its speeds (+96 -8.0, +100 / +104
   / +108) and +86 = 15; then its own steps by +5 (0x0885303C); step 2 ends it. */
SFA_FUNC(0885303C, stage_object_helicopter_steps) { run_step(c, *B(ARG(0) + 5), 0x08A16024u, 5, 0x08853064u); }
SFA_FUNC(08852790, stage_object_kind_68) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = c->r[31];
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *H(o + 18) = 0x500; *H(o + 22) = 0x58;
        *W(o + 96) = 0xFFF80000u; *W(o + 104) = 0x800; *W(o + 100) = 0x3000; *W(o + 108) = 0;
        *H(o + 86) = 15;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
    } else if (step == 1) {
        c->r[5] = 1; c->r[6] = 2;
        c->r[31] = 0x08852810;
        f_0885303C(c);
    } else if (step == 2) {
        c->r[31] = 0x0885282C;
        f_08823220(c);
    }
    c->r[31] = *W(c->r[29] + 16);
    c->r[29] += 0x20;
}

/* Kind 73 (GUILE): something crossing the sky from right to left, 26 pixels a frame, back to x 0x220
   once past -1280 (not while the fight is stopped: BATTLE + 15, + 37, + 52). Drawn if on the screen,
   except while layer 0's x is 3840 - 12288 and the object is outside -143 - 479, and on World Tour's
   screen 4. */
#define LAYER_0 0x092E8800u
SFA_FUNC(08852B70, stage_object_kind_73) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = c->r[31];
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        c->r[31] = 0x08852BBC;
        CALL2(f_0882196C, o, *W(0x08A15F18u + (uint32_t)*B(o + 3) * 4u));
    } else if (step == 1) {
        int16_t lx = S16(LAYER_0 + 18), ox = S16(o + 18);
        int shown = lx < 3840 || lx >= 12289 || (ox >= -143 && ox < 480);
        if (shown) {
            if ((S8(BATTLE + 37) | S8(BATTLE + 52) | S8(BATTLE + 15)) == 0) {
                ox = S16(o + 18);
                *H(o + 18) = ox < -1280 ? 0x220 : (uint16_t)(ox - 26);
            }
            if (!(S8(GAME + 114) == 3 && *H(GAME + 4) == 4)) {
                c->r[4] = o; c->r[31] = 0x08852C78;
                f_08821424(c);
            }
        }
    } else if (step == 2) {
        c->r[4] = o; c->r[31] = 0x08852C94;
        f_08823220(c);
    }
    c->r[31] = *W(c->r[29] + 16);
    c->r[29] += 0x20;
}

/* Kind 71 (GUILE): sends things over the stage now and then, by where layer 0 is: while its x is under
   3840 or from 11521, one of 8 (a random pick) every 200 - 327 frames; at 7425 - 7519 the ninth every
   240 - 367; at 8961 - 9791 one of 8 every 0 - 63; elsewhere its count (+82) is held at 0. */
void f_08852DB0(Cpu *c);   /* (object, which): sends one of GUILE's passing things */
SFA_FUNC(08852900, stage_object_kind_71) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *H(o + 82) = 0;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
    } else if (step == 1) {
        int16_t lx = S16(LAYER_0 + 18);
        int16_t count = S16(o + 82);
        uint32_t which;
        if (lx < 3840 || lx >= 11521) {
            if (count != 0) *H(o + 82) = (uint16_t)(count - 1);
            else {
                which = CALL0(f_08821368) & 7;
                *H(o + 82) = (uint16_t)((CALL0(f_08821368) & 0x7F) + 0xC8);
                CALL2(f_08852DB0, o, which);
            }
        } else if (lx >= 7425 && lx < 7520) {
            if (count != 0) *H(o + 82) = (uint16_t)(count - 1);
            else {
                *H(o + 82) = (uint16_t)((CALL0(f_08821368) & 0x7F) + 0xF0);
                CALL2(f_08852DB0, o, 8);
            }
        } else if (lx >= 8961 && lx < 9792) {
            if (count != 0) *H(o + 82) = (uint16_t)(count - 1);
            else {
                which = CALL0(f_08821368) & 7;
                *H(o + 82) = (uint16_t)(CALL0(f_08821368) & 0x3F);
                CALL2(f_08852DB0, o, which);
            }
        } else {
            *H(o + 82) = 0;
        }
    } else if (step == 2) {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kind 70 (GUILE): three pieces (types 0 - 2) that jump between fixed places as layer 0 moves: each
   frame the object stands where +96 / +100 said (x / y), and +96 / +100 are then set for layer 0's x.
   Drawn if on the screen. */
typedef struct { int16_t from, to; uint32_t x, y; } Place;   /* while from <= layer 0's x < to */
static const struct { uint32_t x0, y0; int16_t below, above; Place p[2]; } GUILE_PIECES[3] = {
    { 0x498, 0x90, 721, 4096, { { 721, 1376, 0x498, 0x90 }, { 1376, 2560, 0x910, 0x30 } } },   /* else 0xD08, 0x20 */
    { 0x468, 0x90, 673, 4096, { { 673, 1328, 0x468, 0x90 }, { 1328, 4096, 0x8A8, 0x90 } } },
    { 0x490, 0x90, 721, 4096, { { 721, 1360, 0x490, 0x90 }, { 1360, 2336, 0x878, 0x90 } } },   /* else 0xC60, 0x76 */
};
SFA_FUNC(08853500, stage_object_kind_70) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        uint8_t type = *B(o + 3);
        int i = type == 0 ? 0 : type == 1 ? 1 : 2;
        *W(o + 96) = GUILE_PIECES[i].x0; *H(o + 18) = (uint16_t)GUILE_PIECES[i].x0;
        *W(o + 100) = GUILE_PIECES[i].y0; *H(o + 22) = (uint16_t)GUILE_PIECES[i].y0;
        *H(o + 86) = 15;
        *W(o + 148) = 0;
        CALL2(f_0882196C, o, *W(0x08A15F18u + (uint32_t)type * 4u + 24u));
        CALL1(f_08821424, o);
    } else if (step == 1) {
        *H(o + 18) = (uint16_t)*W(o + 96);
        *H(o + 22) = (uint16_t)*W(o + 100);
        uint8_t type = *B(o + 3);
        int i = type == 0 ? 0 : type == 1 ? 1 : 2;
        int16_t lx = S16(LAYER_0 + 18);
        if (lx >= GUILE_PIECES[i].below && lx < GUILE_PIECES[i].above) {
            uint32_t x, y;
            if (lx < GUILE_PIECES[i].p[0].to) { x = GUILE_PIECES[i].p[0].x; y = GUILE_PIECES[i].p[0].y; }
            else if (i == 1 || lx < GUILE_PIECES[i].p[1].to) { x = GUILE_PIECES[i].p[1].x; y = GUILE_PIECES[i].p[1].y; }
            else if (i == 0) { x = 0xD08; y = 0x20; }
            else { x = 0xC60; y = 0x76; }
            *W(o + 96) = x; *W(o + 100) = y;
        }
        CALL1(f_08821424, o);
    } else if (step == 2) {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kinds 56 / 61 (BLANKA, CODY): idle animations with waits. The record's +11 picks how it runs (now
   at +3; its type moves to +73). With +3 = 0: when the animation sets +76, it waits that many frames
   plus a random 0 - 255 (+82), then goes on. Otherwise: when the animation sets +75 negative, the next
   animation is its type's, or one 1 in 16 times the next one's. Drawn near the screen (+96). */
static void idle_kind_ex(Cpu *c, gaddr table, uint8_t frame, uint32_t wait_at) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 73) = *B(o + 3);
        *B(o + 4) = 1;
        *B(o + 3) = *B(o + 11);
        *B(o + 15) = 1;
        *H(o + 82) = 0;
        if (frame) { *B(o + 12) = 0xFF; *B(o + 13) = frame; } else *B(o + 12) = 0;
        *B(o + 11) = 0;
        *W(o + 96) = (uint32_t)((int32_t)S8(o + 78) << 4);
        CALL2(f_0882196C, o, *W(table + (uint32_t)(int32_t)S8(o + 73) * 4u));
    } else if (step == 1) {
        if (*B(o + 3) != 0) {
            if (S8(o + 75) < 0) {
                int32_t next = S8(o + 73);
                if ((CALL0(f_08821368) & 0xF) == 0) next++;
                CALL2(f_0882196C, o, *W(table + (uint32_t)next * 4u));
            } else if (animating()) CALL1(f_088219B0, o);
        } else {
            int16_t wait;
            if (S8(o + wait_at) != 0) {
                uint32_t r = CALL0(f_08821368);
                *H(o + 82) = (uint16_t)(((r + *B(GAME + 32)) & 0xFF) + (uint32_t)(int32_t)S8(o + wait_at));
                *B(o + wait_at) = 0;
            }
            wait = S16(o + 82);
            if (wait != 0) *H(o + 82) = (uint16_t)(wait - 1);
            else if (animating()) CALL1(f_088219B0, o);
        }
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
    c->r[29] += 0x20;
}
SFA_FUNC(088513F4, stage_object_kind_56) { idle_kind_ex(c, 0x08A159C0u, 0, 76); }
SFA_FUNC(08851B74, stage_object_kind_61) { idle_kind_ex(c, 0x08A15BB0u, 0, 76); }
/* Kind 18 (GUY): the same with frame 0xFF / 41 and the wait set by the animation at +77. */
SFA_FUNC(0884CB40, stage_object_kind_18) { idle_kind_ex(c, 0x08A14B30u, 41, 77); }

/* Kind 63 (KARIN): types 0, 1, 5 drawn with +160 = 0x80. Type 2 crosses the stage: it waits +116
   frames, then moves (speed +96 = 6.0) until past x 512, then starts again from -128 after a random
   10 - 73 frames. Type 3 animates, and when its animation reaches a mark (+75 = 0x40) it waits a
   random 240 - 495 frames (+82) and starts again. The other types stay on their first frame. */
void f_08821044(Cpu *c);   /* object_move (motion.c) */
SFA_FUNC(08851EBC, stage_object_kind_63) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0;
        *H(o + 86) = 4;
        uint8_t type = *B(o + 3);
        if (type == 0 || type == 1 || type == 5) *B(o + 160) = 0x80;
        if (type == 2) {
            *B(o + 5) = 0; *B(o + 116) = 1;
            *W(o + 96) = 0x600; *W(o + 108) = 0; *W(o + 100) = 0; *W(o + 104) = 0;
        } else if (type == 3) {
            *H(o + 82) = (uint16_t)((CALL0(f_08821368) & 0x3F) + 0x78);
            *B(o + 5) = 0;
        }
        CALL2(f_0882196C, o, *W(0x08A15C88u + (uint32_t)*B(o + 3) * 4u));
        c->r[29] += 0x20;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    uint8_t type = *B(o + 3);
    if (type == 2 && animating()) {
        if (*B(o + 5) == 0) {
            uint8_t wait = *B(o + 116);
            if (wait != 0) *B(o + 116) = (uint8_t)(wait - 1);
            else *B(o + 5) = 1;
        } else {
            CALL1(f_08821044, o);
            if (S16(o + 18) >= 513) {
                *B(o + 5) = 0;
                *H(o + 18) = (uint16_t)-128;
                *B(o + 116) = (uint8_t)((CALL0(f_08821368) & 0x3F) + 10);
            }
        }
    } else if (type == 3 && animating()) {
        int16_t wait = S16(o + 82);
        if (wait != 0) *H(o + 82) = (uint16_t)(wait - 1);
        else if (S8(o + 75) != 0x40) CALL1(f_088219B0, o);
        else {
            *H(o + 82) = (uint16_t)((CALL0(f_08821368) & 0xFF) + 0xF0);
            CALL2(f_0882196C, o, *W(0x08A15C88u + (uint32_t)*B(o + 3) * 4u));
        }
    }
    CALL1(f_088215C4, o);
    c->r[29] += 0x20;
}

/* Kind 76 (DEEJAY): set up and run in the same frame. Types 0 and 2 move against the fighters' layer
   (14 / 16 and 10 / 16 of how far it has come), the others stay; animated, drawn. Any step but 0 runs:
   it is never ended. */
SFA_FUNC(08853AC8, stage_object_kind_76) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (*B(o + 4) == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        *H(o + 86) = 15;
        *W(o + 96) = *W(o + 16); *W(o + 100) = *W(o + 20);
        CALL2(f_0882196C, o, *W(0x08A164B8u + (uint32_t)*B(o + 3) * 4u));
        CALL1(f_08821424, o);
    }
    if (*B(o + 4) == 0) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    uint8_t type = *B(o + 3);
    if (type == 0 || type == 2) {
        int32_t rate = type == 0 ? 14 : 10;
        int32_t tx = (int32_t)(*W(LAYER_1 + 32) - *W(LAYER_1 + 8));
        *W(o + 16) = *W(o + 96) - (uint32_t)scale16(tx, rate);
        int32_t ty = (int32_t)(*W(LAYER_1 + 36) - *W(LAYER_1 + 12));
        *W(o + 20) = *W(o + 100) - (uint32_t)scale16(ty, rate);
    }
    if (animating()) CALL1(f_088219B0, o);
    CALL1(f_088215C4, o);
    c->r[29] += 0x20;
}

/* Kind 13 (SODOM): one fixed animation (the fourth of 0x08A14A60); x moves back with 14 / 16 of the
   fighters' layer since its +16 (with the shake), y with 15 / 16 of layer 2's since its +20. Drawn if
   on the screen. */
SFA_FUNC(0884C4D8, stage_object_kind_13) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        *W(o + 104) = *W(o + 16); *W(o + 108) = *W(o + 20);
        CALL2(f_0882196C, o, *W(0x08A14A60u + 12u));
        CALL1(f_08821424, o);
    } else if (step == 1) {
        int32_t bx = (int32_t)(*W(LAYER_1 + 8) - *W(LAYER_1 + 16) + *W(SCROLL + 112));
        *W(o + 16) = *W(o + 104) + (uint32_t)scale16(bx, 14);
        int32_t by = (int32_t)(*W(LAYER_2 + 12) - *W(LAYER_2 + 20));
        *W(o + 20) = *W(o + 108) + (uint32_t)scale16(by, 15);
        CALL1(f_08821424, o);
    } else {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kind 14 (SODOM): animated; with +77 set by the animation it counts +82 down, and when the animation
   ends (+75 negative) it starts its type's again, or after 320 frames (+82 run out) the second one. */
SFA_FUNC(0884C5F8, stage_object_kind_14) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        *H(o + 82) = 0x140;
        CALL2(f_0882196C, o, *W(0x08A14A60u));
    } else if (step == 1) {
        if (animating()) {
            if (S8(o + 77) != 0) *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
            if (S8(o + 75) >= 0) { if (animating()) CALL1(f_088219B0, o); }
            else {
                uint32_t next = *B(o + 3);
                if (S16(o + 82) <= 0) { *H(o + 82) = 0x140; next = 1; }
                CALL2(f_0882196C, o, *W(0x08A14A60u + next * 4u));
            }
        }
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    CALL1(f_088215C4, o);
    c->r[29] += 0x20;
}

/* (object): a wait of 780 - 1027 frames into +82, from a random pick of eight (with the pick added).
   The original builds its table of eight on its own stack, as here. */
SFA_FUNC(0884C8A4, stage_object_random_wait) {
    gaddr o = ARG(0);
    static const uint16_t WAITS[8] = { 0x384, 0x3FC, 0x384, 0x3C0, 0x384, 0x348, 0x384, 0x30C };
    c->r[29] -= 0x20;
    for (int i = 0; i < 8; i++) *H(c->r[29] + 2u * (uint32_t)i) = WAITS[i];
    *W(c->r[29] + 16) = c->r[16];
    uint32_t pick = CALL0(f_08821368) & 7;
    *H(o + 82) = (uint16_t)(*H(c->r[29] + pick * 2u) + pick);
    c->r[29] += 0x20;
}

/* Kind 15 (SODOM): something that crosses the stage now and then: it waits (a random wait / 8 the
   first time), then flies from the nearer side (speed 8.0, gaining 1/32 a frame) until it is out
   past x 48 - 1487, then waits again. Drawn near the screen (224). */
SFA_FUNC(0884C710, stage_object_kind_15) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        *W(o + 100) = 0x80000; *W(o + 108) = 0x800;
        CALL1(f_0884C8A4, o);
        *H(o + 82) = (uint16_t)(S16(o + 82) >> 3);
        CALL2(f_0882196C, o, *W(0x08A14A60u + (uint32_t)*B(o + 3) * 4u));
        c->r[29] += 0x20;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    uint8_t phase = *B(o + 5);
    if (animating()) {
        if (phase == 0) {
            int16_t wait = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)wait;
            if (wait == 0) *B(o + 5) = 1;
        } else if (phase == 1) {
            int16_t x = S16(o + 18);
            *B(o + 5) = 2;
            uint32_t v = *W(o + 100), a = *W(o + 108);
            if (x < 641) { *W(o + 96) = v; *W(o + 104) = a; }
            else { *W(o + 96) = 0u - v; *W(o + 104) = 0u - a; }
        } else {
            uint32_t v = *W(o + 96);
            *W(o + 16) += v;
            int16_t x = S16(o + 18);
            *W(o + 96) = v + *W(o + 104);
            if (((uint32_t)(int32_t)x - 48u & 0xFFFFu) >= 1440) {
                *B(o + 5) = 0;
                CALL1(f_0884C8A4, o);
            }
        }
    }
    CALL2(f_088214E8, o, 0xE0);
    c->r[29] += 0x20;
}

/* (object): a wait of 8, 24, 40 or 56 frames into +82, by the frame counter. The original builds the
   four on its own stack, as here. */
SFA_FUNC(0884EBB4, stage_object_short_wait) {
    gaddr o = ARG(0);
    static const uint16_t WAITS[4] = { 8, 0x18, 0x28, 0x38 };
    c->r[29] -= 0x10;
    for (int i = 0; i < 4; i++) *H(c->r[29] + 2u * (uint32_t)i) = WAITS[i];
    *H(o + 82) = *H(c->r[29] + (*B(GAME + 32) & 3u) * 2u);
    c->r[29] += 0x10;
}

/* (object): a new x for it, one of the two in 0x08A15160 for where layer 0 is (a random pick of the
   two), copied to the stack first as the original does. */
void f_089F3430(Cpu *c);   /* memcpy */
SFA_FUNC(0884EC00, stage_object_pick_x) {
    gaddr o = ARG(0);
    c->r[29] -= 0x60;
    *W(c->r[29] + 80) = c->r[16]; *W(c->r[29] + 84) = c->r[17]; *W(c->r[29] + 88) = c->r[31];
    CALL3(f_089F3430, c->r[29] + 16, 0x08A15160u, 0x40);
    int32_t pair = (int16_t)((S16(LAYER_0 + 18) & 0x1E0) >> 4);
    uint32_t pick = CALL0(f_08821368) & 1;
    *H(o + 18) = *H(c->r[29] + 16 + (uint32_t)(pair + (int32_t)pick) * 2u);
    c->r[29] += 0x60;
}

/* Kind 36 (ROLENTO): appears now and then: waits (a short wait), then shows at one of the places
   picked for where layer 0 is and plays its animation (the sixth of 0x08A15148) to the end, then waits
   again. Drawn near the screen (32), not while waiting. */
SFA_FUNC(0884EAAC, stage_object_kind_36) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0;
        *H(o + 22) = 0x43;
        CALL1(f_0884EBB4, o);
    } else if (step == 1) {
        if (*B(o + 5) == 0) {
            int16_t wait = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)wait;
            if (wait == 0) {
                *B(o + 5) = 1;
                CALL1(f_0884EC00, o);
                CALL2(f_0882196C, o, *W(0x08A15148u + 20u));
                CALL2(f_088214E8, o, 0x20);
            }
        } else if (S8(o + 75) < 0) {
            *B(o + 5) = 0;
            CALL1(f_0884EBB4, o);
        } else {
            if (animating()) CALL1(f_088219B0, o);
            CALL2(f_088214E8, o, 0x20);
        }
    } else {
        CALL1(f_08823220, o);
    }
    c->r[29] += 0x20;
}

/* Kind 75 (FEI LONG): the crowd, reacting to the fighters. It remembers each fighter's +112 (+112 /
   +114) and picks its animation from type + 0, 1 (fighter 2 has moved back 8 or more since), 2 / 3
   (fighter 1 has too), until the animation reaches its mark (+75 = 0x40); when the round's result is
   in (BATTLE + 12) it plays type + that once. Drawn near the screen (+96). It never ends itself. */
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
SFA_FUNC(088538A0, stage_object_kind_75) {
    gaddr o = ARG(0);
    const gaddr table = 0x08A16460u;
    c->r[29] -= 0x30;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0;
        *W(o + 96) = (uint32_t)((int32_t)S8(o + 78) << 4);
        *H(o + 112) = *H(FIGHTER_1 + 112);
        *B(o + 116) = 0;
        *H(o + 114) = *H(FIGHTER_2 + 112);
        CALL2(f_0882196C, o, *W(table + ((uint32_t)*B(o + 3) + *B(o + 116)) * 4u));
        c->r[29] += 0x30;
        return;
    }
    if (step != 1) { c->r[29] += 0x30; return; }
    if (animating()) CALL1(f_088219B0, o);
    if (S8(BATTLE + 12) != 0) {
        if (*B(o + 5) == 0) {
            *B(o + 5) = 1;
            *B(o + 116) = (uint8_t)S8(BATTLE + 12);
            CALL2(f_0882196C, o, *W(table + ((uint32_t)*B(o + 3) + *B(o + 116)) * 4u));
        } else {
            *B(o + 116) = 0;
        }
    } else {
        int16_t p2 = S16(FIGHTER_2 + 112), was2 = S16(o + 114), was1 = S16(o + 112);
        if (p2 != was2) {
            *B(o + 116) = (was2 - p2) < 8 ? 0 : 1;
            *H(o + 114) = (uint16_t)S16(FIGHTER_2 + 112);
        } else {
            *B(o + 116) = 0;
        }
        uint8_t pick = *B(o + 116);
        int16_t p1 = S16(FIGHTER_1 + 112);
        if (p1 != was1) {
            if ((was1 - p1) >= 8) { *B(o + 116) = (uint8_t)(pick + 2); p1 = S16(FIGHTER_1 + 112); pick = *B(o + 116); }
            *H(o + 112) = (uint16_t)p1;
        }
        if (pick != 0) CALL2(f_0882196C, o, *W(table + ((uint32_t)*B(o + 3) + pick) * 4u));
        if (S8(o + 75) == 0x40) {
            *B(o + 116) = 0;
            CALL2(f_0882196C, o, *W(table + ((uint32_t)*B(o + 3) + *B(o + 116)) * 4u));
        }
    }
    CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
    c->r[29] += 0x30;
}

/* (object): +82 = a random 8, 9, 10, 10, 10, 10, 11 or 12 (the original's table is on its stack). */
SFA_FUNC(0884F0F4, stage_object_repeats) {
    gaddr o = ARG(0);
    static const uint8_t COUNTS[8] = { 8, 9, 10, 10, 10, 10, 11, 12 };
    c->r[29] -= 0x10;
    for (int i = 0; i < 8; i++) *B(c->r[29] + (uint32_t)i) = COUNTS[i];
    *H(o + 82) = *B(c->r[29] + (CALL0(f_08821368) & 7));
    c->r[29] += 0x10;
}

/* Kind 40 (ZANGIEF): plays animation +78 (from the seventh of 0x08A152D8 on) +82 times, then the next
   one once, then moves on to the next of four (+78) and a new count. Drawn. */
SFA_FUNC(0884EF98, stage_object_kind_40) {
    gaddr o = ARG(0);
    const gaddr table = 0x08A152D8u + 24u;
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 78) = 0;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        CALL1(f_0884F0F4, o);
        CALL2(f_0882196C, o, *W(table));
        c->r[29] += 0x20;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    if (S8(o + 75) >= 0) {
        if (animating()) CALL1(f_088219B0, o);
    } else {
        int32_t anim;
        if (*B(o + 5) == 0) {
            int16_t left = (int16_t)(S16(o + 82) - 1);
            anim = S8(o + 78);
            *H(o + 82) = (uint16_t)left;
            if (left == 0) { *B(o + 5) = 1; *B(o + 78) = (uint8_t)(anim + 1); anim = S8(o + 78); }
        } else {
            *B(o + 5) = 0;
            *B(o + 78) = (uint8_t)((S8(o + 78) + 1) & 3);
            CALL1(f_0884F0F4, o);
            anim = S8(o + 78);
        }
        CALL2(f_0882196C, o, *W(table + (uint32_t)anim * 4u));
    }
    CALL1(f_088215C4, o);
    c->r[29] += 0x20;
}

/* Kind 20 (BIRDIE): animated; when the animation ends it starts again, or 1 time in 2 waits (+5) until
   the callback list's +5 goes negative, then 1 time in 2 starts again. Drawn near the screen (96). */
void f_0884D0B4(Cpu *c);   /* nothing_0884D0B4 (empty.c) */
SFA_FUNC(0884CEA0, stage_object_kind_20) {
    gaddr o = ARG(0);
    const gaddr anim = 0x08A14C30u;
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 41;
        CALL2(f_0882196C, o, *W(anim));
    } else if (step == 1) {
        if (animating()) {
            if (*B(o + 5) == 0) {
                if (S8(o + 75) >= 0) CALL1(f_088219B0, o);
                else if ((CALL0(f_08821368) & 8) == 0) { *B(o + 5) = 1; CALL1(f_0884D0B4, o); }
                else CALL2(f_0882196C, o, *W(anim));
            } else if (S8(CALLBACKS_12 + 5) < 0) {
                if ((CALL0(f_08821368) & 0x20) == 0) { *B(o + 5) = 0; CALL2(f_0882196C, o, *W(anim)); }
                else CALL1(f_0884D0B4, o);
            }
        }
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x20;
        return;
    }
    CALL2(f_088214E8, o, 0x60);
    c->r[29] += 0x20;
}

/* (object): a random start for BISON's falling leaf: x near the fighters' layer, y 96 - 127, one of
   two speeds and its look (+78, +24). The original's tables are on its stack, as here. */
static void leaf_start(Cpu *c) {
    gaddr o = ARG(0);
    static const uint16_t LOOKS[2] = { 7, 3 };
    static const uint32_t SPEEDS[8] = { 0x27000, 0x12000, 0x2D000, 0x14000, 0x32000, 0x16000, 0x37000, 0x18000 };
    c->r[29] -= 0x30;
    *W(c->r[29] + 36) = c->r[16];
    *H(c->r[29]) = LOOKS[0]; *H(c->r[29] + 2) = LOOKS[1];
    for (int i = 0; i < 8; i++) *W(c->r[29] + 4 + 4u * (uint32_t)i) = SPEEDS[i];
    *W(c->r[29] + 40) = c->r[31];
    uint32_t r = CALL0(f_08821368);
    *H(o + 18) = (uint16_t)((r & 0xFF) * 2u + (uint32_t)(int32_t)S16(LAYER_1 + 18) - 64u);
    r = CALL0(f_08821368);
    *H(o + 22) = (uint16_t)((r & 0x1F) + 0x60);
    uint32_t pick = CALL0(f_08821368) & 1, look = pick;
    uint8_t other = 0;
    if ((*B(GAME + 32) & 3) == 0) { look = 0; other = 1; }
    *B(o + 78) = other;
    *H(o + 24) = *H(c->r[29] + look * 2u);
    *W(o + 96) = *W(c->r[29] + pick * 8u + 4u);
    *W(o + 100) = *W(c->r[29] + pick * 8u + 8u);
    c->r[29] += 0x30;
}
SFA_FUNC(0884D870, stage_object_leaf_start) { leaf_start(c); }
SFA_FUNC(08852670, stage_object_leaf_start_2) { leaf_start(c); }

/* Kinds 26 / 66 (BISON 1 / 2): a leaf falling from a random place, swinging (+104 between -2.25 and
   2.25 by 0.25 a frame) until below y 264. Drawn near the screen (8). */
static void leaf_kind(Cpu *c, gaddr table, GuestFunc start) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *B(o + 14) = 2; *B(o + 9) = 1;
        *W(o + 104) = 0; *H(o + 38) = 0; *B(o + 11) = 0; *B(o + 12) = 0;
        *W(o + 108) = 0xFFFFC000u;
        *H(o + 86) = 4; *W(o + 148) = 7; *W(o + 164) = 1;
        c->r[4] = o; start(c);
        CALL2(f_0882196C, o, *W(table + (uint32_t)(int32_t)S8(o + 78) * 4u + 12u));
        c->r[29] += 0x20;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x20; return; }
    if (animating()) {
        uint32_t swing = *W(o + 104), turn = *W(o + 108);
        *W(o + 16) = *W(o + 96) + swing + *W(o + 16);
        *W(o + 20) = *W(o + 20) + *W(o + 100);
        uint32_t size = (int32_t)swing < 0 ? 0u - swing : swing;
        if (size >= 0x24000u) { turn = 0u - turn; *W(o + 108) = turn; }
        *W(o + 104) = swing + turn;
        CALL1(f_088219B0, o);
        if (S16(o + 22) >= 265) *B(o + 4) = 2;
    }
    CALL2(f_088214E8, o, 8);
    c->r[29] += 0x20;
}
SFA_FUNC(0884D5F8, stage_object_kind_26) { leaf_kind(c, 0x08A14DB0u, f_0884D870); }
SFA_FUNC(088523F8, stage_object_kind_66) { leaf_kind(c, 0x08A15DD0u, f_08852670); }

/* (object): a wait into +82, a random one of eight (the second given) plus 0 - 31. The original's table
   is on its stack, as here. */
static void storm_wait(Cpu *c, uint16_t second) {
    gaddr o = ARG(0);
    const uint16_t waits[8] = { 0x80, second, 0xC0, 0xE0, 0x100, 0x120, 0x140, 0x160 };
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = c->r[16];
    for (int i = 0; i < 8; i++) *H(c->r[29] + 2u * (uint32_t)i) = waits[i];
    *W(c->r[29] + 20) = c->r[31];
    uint32_t r = CALL0(f_08821368) & 0xFF;
    *H(o + 82) = (uint16_t)(*H(c->r[29] + (r & 7) * 2u) + (r & 0x1F));
    c->r[29] += 0x20;
}
SFA_FUNC(0884D750, stage_object_storm_wait)   { storm_wait(c, 0xA0); }
SFA_FUNC(08852550, stage_object_storm_wait_2) { storm_wait(c, 0x1E0); }

/* (object): the flash's height and look (+22, +24) by the frame counter, through a pattern of 16
   (copied from `pattern` to the stack, as the original does); answers which of three it is. */
static void storm_flash(Cpu *c, gaddr pattern) {
    gaddr o = ARG(0);
    static const uint16_t LOOKS[6] = { 0x30, 7, 0x38, 7, 0x48, 5 };
    c->r[29] -= 0x40;
    *W(c->r[29] + 44) = c->r[16]; *W(c->r[29] + 48) = c->r[31];
    CALL3(f_089F3430, c->r[29] + 16, pattern, 0x10);
    for (int i = 0; i < 6; i++) *H(c->r[29] + 32 + 2u * (uint32_t)i) = LOOKS[i];
    uint32_t which = *B(c->r[29] + 16 + (*B(GAME + 32) & 0xFu));
    *H(o + 22) = *H(c->r[29] + 32 + which * 4u);
    *H(o + 24) = *H(c->r[29] + 34 + which * 4u);
    c->r[29] += 0x40;
    RETURN((int32_t)(int8_t)which);
}
SFA_FUNC(0884D7D4, stage_object_storm_flash)   { storm_flash(c, 0x08A14DD8u); }
SFA_FUNC(088525D4, stage_object_storm_flash_2) { storm_flash(c, 0x08A15DF8u); }

/* Kinds 25 / 65 (BISON 1 / 2): the storm. It waits (+82), then flashes (one of three, at a random place:
   near the fighters' layer for the first, x 512 - 767 for the others) until its animation ends, then
   tells the callback list (+3 = 0, and 1 time in 2 +5 = 1) and waits for the list's +3 to be set
   again. Every 8 - 39 frames (+112) it also drops a leaf (an object of kind `leaf`). */
void f_0884D750(Cpu *c); void f_0884D7D4(Cpu *c); void f_08852550(Cpu *c); void f_088525D4(Cpu *c);
static void storm_kind(Cpu *c, gaddr table, GuestFunc wait, GuestFunc flash, uint32_t leaf) {
    gaddr o = ARG(0);
    c->r[29] -= 0x30;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0;
        *H(o + 112) = (uint16_t)((CALL0(f_08821368) & 0x1F) + 8);
        c->r[4] = o; wait(c);
        c->r[29] += 0x30;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x30; return; }
    uint8_t phase = *B(o + 5);
    if (phase == 0) {
        if (animating()) {
            int16_t left = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)left;
            if (left == 0) {
                *B(o + 5) = 1;
                c->r[4] = o; flash(c);
                uint32_t which = c->r[2];
                gaddr anim = table + which * 4u;
                if (which != 0) *H(o + 18) = (uint16_t)((CALL0(f_08821368) & 0xFF) + 0x200);
                else *H(o + 18) = (uint16_t)((CALL0(f_08821368) & 0xFF) * 2u + (uint32_t)(int32_t)S16(LAYER_1 + 18) - 64u);
                CALL2(f_0882196C, o, *W(anim));
            }
        }
    } else if (phase == 1) {
        if (S8(o + 75) >= 0) {
            if (animating()) CALL1(f_088219B0, o);
            CALL1(f_08821424, o);
        } else {
            *B(o + 5) = 2;
            CALL0(f_08821368);
            *B(CALLBACKS_12 + 3) = 0;
            if ((CALL0(f_08821368) & 1) == 0) *B(CALLBACKS_12 + 5) = 1;
        }
    } else if (S8(CALLBACKS_12 + 3) != 0) {
        *B(o + 5) = 0;
        c->r[4] = o; wait(c);
    }
    if (animating()) {
        int16_t left = (int16_t)(S16(o + 112) - 1);
        *H(o + 112) = (uint16_t)left;
        if (left == 0) {
            *H(o + 112) = (uint16_t)((CALL0(f_08821368) & 0x1F) + 8);
            CALL2(f_08822F1C, leaf, 0);
        }
    }
    c->r[29] += 0x30;
}
SFA_FUNC(0884D3D4, stage_object_kind_25) { storm_kind(c, 0x08A14DB0u, f_0884D750, f_0884D7D4, 0x1A); }
SFA_FUNC(088521D4, stage_object_kind_65) { storm_kind(c, 0x08A15DD0u, f_08852550, f_088525D4, 0x42); }

/* Kind 45 (BALROG): "follow" (+14, the whole +100, near the screen), drawn with +160 = 0xFF for type 9
   and 0x80 for types 0 - 5. */
SFA_FUNC(0884FED8, stage_object_kind_45) {
    gaddr o = ARG(0);
    if (*B(o + 4) == 0) {
        uint8_t type = *B(o + 3);
        if (type == 9) *B(o + 160) = 0xFF;
        else if (type < 6) *B(o + 160) = 0x80;
    }
    follow_kind(c, &(const FollowKind){ 0x08A15670u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 });
}

/* Kind 48 (BALROG): bobs between y 208 and 240 (speed +100 = 0.25, turning at the ends) while the fight
   runs; the first time BATTLE + 7 is set with the clock (BATTLE + 28) at 0, it sends out four pairs of
   pool-4 objects of kind 0x87 (each pair a random one of 0, 1, 2 doubled, and that plus one).
   Drawn near the screen (+96), with +160 = 0xC0. */
void f_08822EAC(Cpu *c);   /* object_make_32 (pool.c): (kind, type) -> a new object of pool 4, or 0 */
SFA_FUNC(0885027C, stage_object_kind_48) {
    gaddr o = ARG(0);
    c->r[29] -= 0x30;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 5) = 0;
        *B(o + 160) = 0xC0; *H(o + 86) = 4; *B(o + 12) = 0;
        *W(o + 96) = (uint32_t)((int32_t)S8(o + 78) << 4);
        *W(o + 100) = 0x4000;
        CALL2(f_0882196C, o, *W(0x08A15670u + 36u));
    } else if (step == 1) {
        if (*B(o + 5) == 0 && S8(BATTLE + 7) != 0 && S8(BATTLE + 28) == 0) {
            *B(o + 5) = 1;
            for (uint32_t i = 0; i < 4; i++) {
                uint32_t pick = CALL0(f_08821368) & 3;
                if (pick == 3) pick = 0;
                CALL2(f_08822EAC, 0x87, pick * 2u);
                CALL2(f_08822EAC, 0x87, pick * 2u + 1u);
            }
        }
        if ((S8(BATTLE + 37) | S8(BATTLE + 52) | S8(BATTLE + 15)) == 0) {
            uint32_t v = *W(o + 100);
            *W(o + 20) += v;
            int16_t y = S16(o + 22);
            if ((int32_t)v >= 0 ? y >= 241 : y < 208) *W(o + 100) = 0u - v;
            CALL1(f_088219B0, o);
        }
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x30;
        return;
    }
    CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
    c->r[29] += 0x30;
}

/* Kind 46 (BALROG): goes left and right across x 568 - 672 (speed +104), turning at the ends, and now and
   then (1 time in 32 every 40 frames, with +108 clear) stops for 60 frames (+82), throwing two pieces
   (kind 47) whenever +82 & 15 is 8. Type 1 has its own
   animations (from the 19th of 0x08A15670, the others from the 7th). Drawn near the screen (+96). */
SFA_FUNC(08850424, stage_object_kind_46) {
    gaddr o = ARG(0);
    const gaddr t = 0x08A15670u;
    c->r[29] -= 0x30;
    uint8_t step = *B(o + 4);
    if (step == 0) {
        int32_t v78 = S8(o + 78);
        uint8_t type = *B(o + 3);
        *B(o + 15) = 1; *B(o + 4) = 1; *B(o + 12) = 0;
        *W(o + 100) = (uint32_t)v78;
        *W(o + 108) = 0;
        *W(o + 104) = 0xC000u - ((uint32_t)type << 10);
        *H(o + 82) = 0x10;
        CALL2(f_0882196C, o, *W(t + (uint32_t)v78 * 4u + (type == 1 ? 72u : 24u)));
        CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
        c->r[29] += 0x30;
        return;
    }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x30; return; }
    if ((S8(BATTLE + 37) | S8(BATTLE + 52) | S8(BATTLE + 15)) != 0) {
        CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
        c->r[29] += 0x30;
        return;
    }
    if (*B(o + 5) == 0) {
        int16_t x = S16(o + 18);
        if (*W(o + 100) != 0) {
            if (x < 569) {
                *W(o + 108) = 0;
                *W(o + 16) = 0x2380000;
                *W(o + 100) = 0;
                CALL2(f_0882196C, o, *W(t + (*B(o + 3) == 1 ? 72u : 24u)));
            } else {
                *W(o + 16) -= *W(o + 104);
            }
        } else if (x >= 672) {
            *W(o + 16) = 0x2A00000;
            *W(o + 100) = 1;
            CALL2(f_0882196C, o, *W(t + (*B(o + 3) == 1 ? 76u : 28u)));
        } else {
            *W(o + 16) += *W(o + 104);
            if (*W(o + 108) == 0) {
                int16_t n = S16(o + 82);
                if (n == 0) *H(o + 82) = (uint16_t)(n - 1);
                else {
                    *H(o + 82) = 0x28;
                    if ((CALL0(f_08821368) & 0x1F) == 0) {
                        *B(o + 5) = 1;
                        *W(o + 108) = 1;
                        *H(o + 82) = 0x3C;
                        CALL2(f_0882196C, o, *W(t + (*B(o + 3) == 1 ? 80u : 32u)));
                    }
                }
            }
        }
    } else {
        int16_t n = S16(o + 82);
        if (n != 0) *H(o + 82) = (uint16_t)(n - 1);
        else {
            uint8_t type = *B(o + 3);
            *B(o + 5) = 0;
            *W(o + 104) = 0xC000u - ((uint32_t)type << 9);
            CALL2(f_0882196C, o, *W(t + (type == 1 ? 72u : 24u)));
        }
        if ((S16(o + 82) & 0xF) == 8) {
            for (uint32_t i = 0; i < 2; i++) {
                gaddr p = CALL2(f_08822F1C, 0x2F, i);
                if (p == 0) continue;
                *H(p + 18) = (uint16_t)(S16(o + 18) + 0x20);
                *B(p + 9) = 1;
                *H(p + 22) = (uint16_t)(S16(o + 22) + 0x42);
                *B(p + 78) = *B(o + 78);
                *B(p + 11) = *B(o + 11);
                *B(p + 14) = *B(o + 14);
                *H(p + 26) = *H(o + 26);
                *H(p + 24) = *H(o + 24);
                *W(p + 164) = 1;
                *H(p + 38) = *H(o + 38);
                *W(p + 148) = 7;
            }
        }
    }
    CALL1(f_088219B0, o);
    CALL2(f_088214E8, o, (uint32_t)(int32_t)(int16_t)*W(o + 96));
    c->r[29] += 0x30;
}

/* Kind 47 (BALROG): a piece thrown by kind 46: flies off (type 0 to the left, the others to the right)
   with random spins from the table at 0x08A156C4 (copied to the stack each frame, as the original
   does), falling, until it is off the screen's side or below y 272. Drawn. */
SFA_FUNC(08850084, stage_object_kind_47) {
    gaddr o = ARG(0);
    const gaddr t = 0x08A15670u;
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = c->r[16]; *W(c->r[29] + 36) = c->r[17]; *W(c->r[29] + 40) = c->r[31];
    gaddr spins = c->r[29] + 16;
    CALL3(f_089F3430, spins, 0x08A156C4u, 0x10);
    uint8_t step = *B(o + 4);
    if (step == 0) {
        *B(o + 15) = 1; *B(o + 4) = 1;
        *H(o + 86) = 4;
        *W(o + 100) = 0x6000;
        *W(o + 108) = (uint32_t)((int32_t)S8(spins + 1 + (CALL0(f_08821368) & 7)) << 4);
        if (*B(o + 3) == 0) {
            *W(o + 96) = 0xFFFF6000u;
            *W(o + 104) = (uint32_t)(-(int32_t)S8(spins + (CALL0(f_08821368) & 7))) << 4;
        } else {
            *W(o + 96) = 0xA000;
            *W(o + 104) = (uint32_t)((int32_t)S8(spins + (CALL0(f_08821368) & 7)) << 4);
        }
        CALL2(f_0882196C, o, *W(t + (uint32_t)*B(o + 3) * 4u + 40u));
    } else if (step == 1) {
        if ((S8(BATTLE + 37) | S8(BATTLE + 52) | S8(BATTLE + 15)) == 0) {
            uint32_t vx = *W(o + 96), vy = *W(o + 100);
            *W(o + 16) += vx; *W(o + 96) = vx + *W(o + 104);
            *W(o + 20) += vy; *W(o + 100) = vy + *W(o + 108);
            int16_t x = S16(o + 18), lx = S16(LAYER_1 + 18);
            int gone = *B(o + 3) == 0 ? x < lx - 64 : lx + 0x1C0 < x;
            if (gone || S16(o + 22) >= 273) *B(o + 4) = (uint8_t)(step + 1);
            CALL1(f_088219B0, o);
        }
    } else {
        CALL1(f_08823220, o);
        c->r[29] += 0x30;
        return;
    }
    CALL1(f_088215C4, o);
    c->r[29] += 0x30;
}

/* Kind 54 (HONDA): someone who stands at one of two places by which half of the stage the fighters'
   layer is in (x 0x160 / 0x390, set up again each time it changes, with its own palette through
   0x0880AEA8). On the left it plays animations 4 / 5 (switching 1 time in 4 when one ends, with random
   pauses from +77), until the round ends (BATTLE + 7) and the clock (BATTLE + 28) is 0: then the
   seventh. Drawn. */
void f_0880AEA8(Cpu *c);   /* (1, palette list, 0, slot, 1): loads a palette */
void f_08850FBC(Cpu *c);   /* below */
static void honda_side(gaddr o) {
    int right = S16(LAYER_1 + 18) > 0x1C0;
    if (right ? *B(o + 5) == 0 : *B(o + 5) != 0) { *B(o + 5) = (uint8_t)right; *B(o + 6) = 0; }
}
SFA_FUNC(08850FBC, stage_object_honda_side) { honda_side(ARG(0)); }
SFA_FUNC(08851004, stage_object_kind_54) {
    gaddr o = ARG(0);
    const gaddr t = 0x08A15860u;
    c->r[29] -= 0x30;
    uint8_t step = *B(o + 4);
    if (step == 0) { *B(o + 15) = 1; *B(o + 4) = 1; c->r[29] += 0x30; return; }
    if (step != 1) { CALL1(f_08823220, o); c->r[29] += 0x30; return; }
    CALL1(f_08850FBC, o);
    uint8_t side = *B(o + 5), phase = *B(o + 6);
    int animate = 0;
    if (side != 0) {
        if (phase != 0) animate = 1;
        else {
            *B(o + 6) = 1; *H(o + 18) = 0x390; *H(o + 22) = 0x80;
            CALL5(f_0880AEA8, 1, 0x08A15880u + 0x20u, 0, 0x39, 1);
            *B(o + 12) = 0xFF; *B(o + 13) = 41;
            CALL2(f_0882196C, o, *W(t + 28u));
        }
    } else if (phase == 0) {
        *B(o + 6) = 1; *H(o + 18) = 0x160; *H(o + 22) = 0x30;
        CALL5(f_0880AEA8, 1, 0x08A15880u, 0, 0x38, 1);
        *B(o + 78) = 4; *B(o + 12) = 1; *B(o + 13) = 0x38;
        CALL2(f_0882196C, o, *W(t + (uint32_t)(int32_t)S8(o + 78) * 4u));
    } else {
        int end_check = 0;
        if (phase == 1) {
            if (S8(BATTLE + 7) != 0) { *B(o + 6) = 2; end_check = 1; }
            else {
                if (S8(o + 77) != 0) {
                    int32_t at = S8(o + 74);
                    *B(o + 74) = (uint8_t)(at + (int32_t)(CALL0(f_08821368) & 0x1F));
                    *B(o + 77) = 0;
                }
                if (S8(o + 75) >= 0 || (CALL0(f_08821368) & 3) != 0) animate = 1;
                else {
                    *B(o + 78) = (uint8_t)(S8(o + 78) ^ 1);
                    CALL2(f_0882196C, o, *W(t + (uint32_t)(int32_t)S8(o + 78) * 4u));
                }
            }
        } else if (phase == 2) end_check = 1;
        else animate = 1;
        if (end_check) {
            if (S8(BATTLE + 28) != 0) animate = 1;
            else { *B(o + 6) = 3; CALL2(f_0882196C, o, *W(t + 24u)); }
        }
    }
    if (animate && animating()) CALL1(f_088219B0, o);
    CALL1(f_088215C4, o);
    c->r[29] += 0x30;
}

/* ---- the kinds that are one of the shapes above, with their own tables and values ----
   (generated from the program; their stages' scenarios stage_objects_* check them) */
/* GENERATED-KINDS-BEGIN */
SFA_FUNC(0884B034, stage_object_kind_0) { follow_kind(c, &(const FollowKind){ 0x08A144A0u, FOLLOW_UNLESS_CAMERA, RATE_7BITS, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884B1C4, stage_object_kind_1) { line_kind(c, 0x08A144A0u); }
SFA_FUNC(0884B398, stage_object_kind_3) { follow_kind(c, &(const FollowKind){ 0x08A146A0u, FOLLOW_IF_26, RATE_SIGNED, 0, DRAW_NEAR, 15, 15 }); }
SFA_FUNC_WRAPPED(0884B520, stage_object_kind_4, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A146A0u, 0, 560, 3, 1, 2, 944, 112, 7, 14, 0, 0 }); }
SFA_FUNC(0884B7B4, stage_object_kind_5) { follow_kind(c, &(const FollowKind){ 0x08A146D8u, FOLLOW_IF_26, RATE_SIGNED, 41, DRAW_NEAR, 15, 15 }); }
SFA_FUNC_WRAPPED(0884B948, stage_object_kind_6, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A146D8u, 41, 352, 1, 1, 2, 912, 256, 7, 8, 0, 0 }); }
SFA_FUNC(0884BBE8, stage_object_kind_7) { plain_kind(c, 0x08A147F0u, 0); }
SFA_FUNC(0884BCBC, stage_object_kind_8) { follow_kind(c, &(const FollowKind){ 0x08A148E0u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 15 }); }
SFA_FUNC(0884BE44, stage_object_kind_9) { line_kind(c, 0x08A148E0u); }
SFA_FUNC(0884BF2C, stage_object_kind_10) { follow_kind(c, &(const FollowKind){ 0x08A14918u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC_WRAPPED(0884C0B4, stage_object_kind_11, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A14918u, 0, 448, 2, 0, 2, 944, 32, 7, 0, 0, 0 }); }
SFA_FUNC(0884C344, stage_object_kind_12) { follow_kind(c, &(const FollowKind){ 0x08A14A60u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884C91C, stage_object_kind_16) { follow_kind(c, &(const FollowKind){ 0x08A14B30u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884CD0C, stage_object_kind_19) { follow_kind(c, &(const FollowKind){ 0x08A14C30u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884D0BC, stage_object_kind_22) { follow_kind(c, &(const FollowKind){ 0x08A14C70u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884D990, stage_object_kind_28) { follow_kind(c, &(const FollowKind){ 0x08A14EC0u, FOLLOW_UNLESS_CAMERA, RATE_7BITS, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884DDD8, stage_object_kind_30) { follow_kind(c, &(const FollowKind){ 0x08A14FC8u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC_WRAPPED(0884DF60, stage_object_kind_31, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A14FC8u, 0, 448, 5, 1, 1, 856, 152, 5, 0, 0, 0 }); }
SFA_FUNC(0884E1E8, stage_object_kind_32) { follow_kind(c, &(const FollowKind){ 0x08A150C8u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC_WRAPPED(0884E690, stage_object_kind_33, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A150C8u, 0, 496, 4, 1, 2, 912, 208, 7, 8, 0, 0 }); }
SFA_FUNC(0884E370, stage_object_kind_34) { run_step(c, *B(ARG(0) + 4), 0x08A150ECu, 5, 0x0884E398u); }
SFA_FUNC(0884E924, stage_object_kind_35) { follow_kind(c, &(const FollowKind){ 0x08A15148u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884EC74, stage_object_kind_37) { follow_kind(c, &(const FollowKind){ 0x08A151E0u, FOLLOW_UNLESS_CAMERA, RATE_7BITS, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884EE04, stage_object_kind_38) { follow_kind(c, &(const FollowKind){ 0x08A152D8u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC_WRAPPED(0884F160, stage_object_kind_39, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A152D8u, 41, 448, 5, 1, 2, 928, 56, 7, 20, 0, 0 }); }
SFA_FUNC(0884F400, stage_object_kind_41) { follow_kind(c, &(const FollowKind){ 0x08A15388u, FOLLOW_UNLESS_CAMERA, RATE_7BITS_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0884F7C8, stage_object_kind_43) { run_step(c, *B(ARG(0) + 4), 0x08A15448u, 5, 0x0884F7F0u); }
SFA_FUNC(0884FC68, stage_object_kind_44) { run_step(c, *B(ARG(0) + 4), 0x08A154B0u, 5, 0x0884FC90u); }
SFA_FUNC(08850784, stage_object_kind_49) { follow_kind(c, &(const FollowKind){ 0x08A15738u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0885090C, stage_object_kind_50) { line_kind(c, 0x08A15738u); }
SFA_FUNC(088509F4, stage_object_kind_51) { follow_kind(c, &(const FollowKind){ 0x08A15770u, FOLLOW_IF_26, RATE_SIGNED, 40, DRAW_NEAR, 15, 15 }); }
SFA_FUNC_WRAPPED(08850B88, stage_object_kind_52, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A15770u, 41, 352, 1, 1, 2, 912, 256, 7, 8, 0, 0 }); }
SFA_FUNC(08850E28, stage_object_kind_53) { follow_kind(c, &(const FollowKind){ 0x08A15860u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(0885125C, stage_object_kind_55) { follow_kind(c, &(const FollowKind){ 0x08A159C0u, FOLLOW_ALWAYS, RATE_7BITS_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(088515B4, stage_object_kind_57) { follow_kind(c, &(const FollowKind){ 0x08A15A98u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC_WRAPPED(088517D8, stage_object_kind_58, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A15A98u, 0, 384, 1, 1, 2, 896, 80, 7, 8, 0, 0 }); }
SFA_FUNC(08851D34, stage_object_kind_62) { follow_kind(c, &(const FollowKind){ 0x08A15BF8u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR_OR_ON_SCREEN, 0, 0 }); }
SFA_FUNC(08853718, stage_object_kind_74) { follow_kind(c, &(const FollowKind){ 0x08A16460u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 0, DRAW_NEAR_OR_ON_SCREEN, 0, 0 }); }
SFA_FUNC(08853C8C, stage_object_kind_77) { run_step(c, *B(ARG(0) + 4), 0x08A165D0u, 5, 0x08853CB4u); }
SFA_FUNC(08853E00, stage_object_kind_78) { run_step(c, *B(ARG(0) + 4), 0x08A165E0u, 5, 0x08853E28u); }
SFA_FUNC(08853EB0, stage_object_kind_79) { run_step(c, *B(ARG(0) + 4), 0x08A165F0u, 5, 0x08853ED8u); }
SFA_FUNC(08853FC0, stage_object_kind_80) { follow_kind(c, &(const FollowKind){ 0x08A165A8u, FOLLOW_UNLESS_CAMERA, RATE_7BITS, 0, DRAW_NEAR, 0, 0 }); }
SFA_FUNC(088543E8, stage_object_kind_81) { follow_kind(c, &(const FollowKind){ 0x08A16618u, FOLLOW_IF_26, RATE_SIGNED, 41, DRAW_NEAR, 15, 15 }); }
SFA_FUNC_WRAPPED(0885457C, stage_object_kind_82, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A16618u, 41, 352, 1, 1, 2, 912, 256, 7, 8, 0, 0 }); }
SFA_FUNC(0885481C, stage_object_kind_83) { follow_kind(c, &(const FollowKind){ 0x08A166B8u, FOLLOW_UNLESS_CAMERA, RATE_SIGNED, 41, DRAW_ALWAYS, 0, 0 }); }
SFA_FUNC_WRAPPED(0885498C, stage_object_kind_84, widescreen_twins) { swap_kind(c, &(const SwapKind){ 0x08A166B8u, 41, 496, 4, 1, 2, 912, 208, 7, 8, 0, 1 }); }
/* GENERATED-KINDS-END */
