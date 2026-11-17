/* WIDE FORCED: twins for the stage objects that change props at a line.
 *
 * To save sprites, some stage objects play two props ("swap at the line", stage/objects.c swap_kind:
 * kinds 4, 6, 11, 29, 31, 33, 39, 52, 58, 82, 84): while the fighters' layer is left of the object's
 * line it shows look 0 at one place, past it look 1 at another, and the arcade's line puts the change
 * just off the 4:3 screen (DAN: a prop at x 408, and the crate post at x 856, 24 pixels past the
 * picture's right edge when it changes). The 16:9 picture shows 48 pixels more on each side, so the
 * change happens in view: the prop appears from nowhere.
 *
 * Here, in WIDE FORCED, each such object gets a twin that always shows the look the object isn't
 * showing, so both props are there while they are in view. A twin is a copy of the object in memory of
 * its own (never in an object list, so the game never runs it): when the object swaps, the twin takes
 * the other look, as the object does (its animation from that look's start); otherwise its animation
 * steps when the object's would; and it is queued for drawing after the object. It goes when the
 * object is set up again or ends.
 *
 * This only adds drawing, in WIDE FORCED: the objects, the fight and NORMAL are untouched. It runs
 * around the decompiled kinds (SFA_FUNC_WRAPPED), so the parity tests' reference run does the same.
 */
#include "rt.h"
#include <string.h>

void f_088215C4(Cpu *c);   /* object_draw (object/frame.c): queues an object for drawing */
void f_088219B0(Cpu *c);   /* object_animate (object/animation.c): one frame on */
extern int rt_widescreen;

#define POOL       0x09514B70u   /* pool 5: the stage's objects (object_make_16), 16 of 0xC0 bytes */
#define POOL_N     16
#define OBJ_SIZE   0xC0u
#define BATTLE     0x092E1228u   /* +37, +52: set while the fight's animation is stopped */
#define LOOKS      96            /* the object's two looks (stage/objects.c take_look) */
#define SIDE       (LOOKS + 40)  /* s8: the look the object shows (0 left of the line, 1 past it) */
#define KIND       2
#define STEP       7             /* 0 being set up, 1 running */
#define ALWAYS_ANIMATED_KIND 84  /* its animation runs even while the fight's is stopped */

static uint32_t twins;            /* POOL_N twins of OBJ_SIZE (game memory, allocated once) */
static int8_t twin_look[POOL_N];  /* the look each twin shows; -1 none */

static uint8_t  *P(uint32_t a) { return HOST(a); }
static void      W16(uint32_t a, uint32_t v) { uint16_t h = (uint16_t)v; memcpy(HOST(a), &h, 2); }

/* stage/objects.c's take_look, on the twin: the look's animation, place, layer, camera, +24 and +38 */
static void take_look(uint32_t o, int look) {
    uint32_t k = o + LOOKS;
    uint32_t a = k + (look ? 16u : 0u), p = k + (look ? 24u : 8u), f = k + (look ? 34u : 32u);
    memcpy(P(o + 28), P(a), 4);
    memcpy(P(o + 74), P(a + 4), 4);
    W16(o + 18, RH(p)); W16(o + 22, RH(p + 2));
    *P(o + 9) = *P(f); *P(o + 14) = *P(f + 1);
    W16(o + 24, RH(p + 4)); W16(o + 38, RH(p + 6));
}

static void call1(Cpu *c, void (*f)(Cpu *), uint32_t a0) {
    uint32_t keep[32], lo = c->lo, hi = c->hi;
    memcpy(keep, c->r, sizeof keep);        /* the caller's registers, as the kind routine left them */
    c->r[4] = a0; c->r[31] = 0x0884DF60u;
    f(c);
    memcpy(c->r, keep, sizeof keep); c->lo = lo; c->hi = hi;
}

/* After a swapping kind's routine (a0 the object) */
int widescreen_twins(Cpu *c, uint32_t o, int after) {
    if (o < POOL || o >= POOL + POOL_N * OBJ_SIZE || (o - POOL) % OBJ_SIZE) return 0;
    int i = (int)((o - POOL) / OBJ_SIZE);
    if (!twins) {                           /* once, whether or not WIDE FORCED is on */
        twins = rt_alloc(POOL_N * OBJ_SIZE, 1, 64, "widescreen twins");
        memset(twin_look, -1, sizeof twin_look);
        if (!twins) { rt_warn("widescreen twins: no memory, the swapping stage objects have none"); return 0; }
    }
    if (!after) {                           /* being set up again: a new stage, or a new round's */
        if (*P(o + STEP) == 0) twin_look[i] = -1;
        return 0;
    }
    if (!rt_widescreen || *P(o) == 0 || *P(o + STEP) != 1) { twin_look[i] = -1; return 0; }

    uint32_t t = twins + (uint32_t)i * OBJ_SIZE;
    int other = (int8_t)*P(o + SIDE) ? 0 : 1;
    if (twin_look[i] < 0) {                 /* the first frame: a copy of the object, the other look */
        memcpy(P(t), P(o), OBJ_SIZE);
        memset(P(t + 44), 0, 12);           /* no sprites linked to it: those are the object's */
        take_look(t, other);
        twin_look[i] = (int8_t)other;
    } else if (twin_look[i] != other) {     /* the object swapped: the twin takes the other look */
        take_look(t, other);
        twin_look[i] = (int8_t)other;
    } else if (*P(o + KIND) == ALWAYS_ANIMATED_KIND || ((int8_t)*P(BATTLE + 37) | (int8_t)*P(BATTLE + 52)) == 0) {
        call1(c, f_088219B0, t);            /* stepped when the object's would be */
    }
    call1(c, f_088215C4, t);
    return 0;
}
