/* The game's object lists and the text pass that ends a screen's frame.
 *
 * Three fixed arrays of objects are run once a frame by the screens that use them: each active object's
 * routine, picked by its type from a table, is called with the object. The loop's counter is kept in
 * GAME + 66 (and read back after every call, as the original does, since a routine could change it).
 *
 *   list        objects at     count  size  active  type  routines
 *   0x08837690  0x09512770     0x20   0xC0  +0      +2    0x08A0FDC8
 *   0x088379B8  0x09514B70     0x10   0xC0  +0      +2    0x08A101F0
 *   0x08887AFC  0x092E2B88     0x20   0x94  +90     +91   0x08A25AA0
 *   0x0883771C  0x09515770     0x2A   0xC0  +0      +2    0x08A10040
 *   0x08854C10  0x09512170     8      0xC0  +0      +2    0x08A16D18
 *   0x08886E18  0x092E3E08     3      0x30  +0      +2    0x08A259F8
 */
#include "../decomp.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define G_COUNTER 66     /* s8: the object loop's counter */

static inline uint8_t *B(gaddr a) { return guest_ptr(a); }
static inline int8_t S8(gaddr a) { return (int8_t)*B(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_0880B7BC(Cpu *c); void f_0880EBF8(Cpu *c);   /* the text pass's two parts */
void f_088224DC(Cpu *c);   /* sprites: finish the frame (0) */
void f_088225EC(Cpu *c);   /* sprites: finish a halted frame */
void rt_call_indirect(Cpu *c, uint32_t addr);

typedef struct { gaddr first; uint8_t count; uint16_t size; uint8_t active, type; gaddr routines; } ObjectList;

static void run_list(Cpu *c, const ObjectList *l) {
    gaddr obj = l->first;
    *B(GAME + G_COUNTER) = l->count;
    for (;;) {
        if (*B(obj + l->active) != 0) {
            gaddr fn = W(l->routines)[*B(obj + l->type)];
            c->r[4] = obj; c->r[5] = fn;
            rt_call_indirect(c, fn);
        }
        int8_t left = (int8_t)(S8(GAME + G_COUNTER) - 1);
        *B(GAME + G_COUNTER) = (uint8_t)left;
        if (left == 0) break;
        obj += l->size;
    }
}

static const ObjectList LIST_1 = { 0x09512770u, 0x20, 0xC0, 0, 2, 0x08A0FDC8u };
static const ObjectList LIST_2 = { 0x09514B70u, 0x10, 0xC0, 0, 2, 0x08A101F0u };
static const ObjectList LIST_3 = { 0x092E2B88u, 0x20, 0x94, 90, 91, 0x08A25AA0u };
static const ObjectList LIST_4 = { 0x09515770u, 0x2A, 0xC0, 0, 2, 0x08A10040u };
static const ObjectList LIST_5 = { 0x09512170u, 8, 0xC0, 0, 2, 0x08A16D18u };
static const ObjectList LIST_6 = { 0x092E3E08u, 3, 0x30, 0, 2, 0x08A259F8u };

/* The text pass at a frame's end (unless the frame is skipped), then the sprites finished: the halted
   way while GAME + 0x59 halts the fight (and + 0x92 is clear). */
static void end_of_frame(Cpu *c) {
    if (S8(BATTLE + 23) == 0) {
        c->r[4] = 0;
        f_0880B7BC(c);
        f_0880EBF8(c);
    }
    if (S8(GAME + 146) != 0 || S8(GAME + 89) == 0) CALL1(f_088224DC, 0);
    else f_088225EC(c);
}

#define FRAME(size, call) do { c->r[29] -= (size); call; c->r[29] += (size); } while (0)
/* wrapped: on the boot screen's message steps the project's version is drawn (src/extensions/bootmsg.c) */
SFA_FUNC_WRAPPED(08837690, game_objects_1, bootmsg_objects) { FRAME(0x20, run_list(c, &LIST_1)); }
SFA_FUNC(088379B8, game_objects_2)    { FRAME(0x20, run_list(c, &LIST_2)); }
SFA_FUNC(08887AFC, game_objects_3)    { FRAME(0x20, run_list(c, &LIST_3)); }
SFA_FUNC(0883771C, game_objects_4)    { FRAME(0x20, run_list(c, &LIST_4)); }
SFA_FUNC(08854C10, game_objects_5)    { FRAME(0x20, run_list(c, &LIST_5)); }
SFA_FUNC(08886E18, game_objects_6)    { FRAME(0x20, run_list(c, &LIST_6)); }

/* Two small tables of callbacks, run once a frame unless BATTLE + 37 or + 52 stops them: each used
   entry's routine (kept in the entry itself) is called with the entry. */
typedef struct { gaddr first; int count; uint32_t size, routine; } CallbackTable;
static void run_callbacks(Cpu *c, const CallbackTable *t) {
    if ((S8(BATTLE + 37) | S8(BATTLE + 52)) != 0) return;
    gaddr e = t->first;
    for (int i = 0; i < t->count; i++, e += t->size) {
        if (S8(e) == 0) continue;
        gaddr fn = *W(e + t->routine);
        c->r[4] = e; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
}
static const CallbackTable CALLBACKS_8  = { 0x095177C8u, 8, 0x14, 16 };
static const CallbackTable CALLBACKS_12 = { 0x09517868u, 12, 0x18, 20 };
SFA_FUNC(08819BBC, game_callbacks_8)  { FRAME(0x20, run_callbacks(c, &CALLBACKS_8)); }
SFA_FUNC(0881EE84, game_callbacks_12) { FRAME(0x20, run_callbacks(c, &CALLBACKS_12)); }
SFA_FUNC(0880B6CC, game_end_of_frame) { FRAME(0x20, end_of_frame(c)); }

/* Empties the third list: every byte of its 32 objects, and its count of free ones (0x095176F9) back
   to 32. */
SFA_FUNC(08823304, game_objects_3_clear) {
    *B(0x095176F9u) = 0x20;
    for (uint32_t i = 0; i < 0x1280u; i++) *B(LIST_3.first + i) = 0;
}
