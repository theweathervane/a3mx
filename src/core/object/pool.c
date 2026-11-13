/* Making objects, and two helpers of the sprite drawing. */
#include "../decomp.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

#define OBJECT_SIZE 0xC0u

/* Takes a free object from a pool, looking down from `top` through `count` of them, and sets it up with
   its kind (+2) and type (+3); the pool's count of free objects (an s8 the fifth argument points at)
   goes down by one. Answers the object, or 0 when the pool is full. */
SFA_FUNC(08822FE4, object_make) {
    gaddr o = ARG(0), free_count = ARG(4);
    uint8_t kind = (uint8_t)ARG(1), type = (uint8_t)ARG(2), count = (uint8_t)ARG(3);
    while (*B(o) != 0) {
        if (--count == 0) { RETURN(0); return; }
        o -= OBJECT_SIZE;
    }
    *B(free_count) = (uint8_t)((int8_t)*B(free_count) - 1);
    *B(o) = 1; *B(o + 2) = kind; *B(o + 3) = type;
    *W(o + 148) = 0; *W(o + 164) = 0;
    *B(o + 163) = 0xFF; *B(o + 162) = 0xFF; *B(o + 161) = 0xFF; *B(o + 160) = 0xFF;
    *B(o + 168) = 0; *B(o + 169) = 0;
    *W(o + 108) = 0; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 96) = 0;
    *W(o + 20) = 0; *W(o + 16) = 0;
    RETURN(o);
}

#define FADE_BY_SLOT 0x092E11A0u   /* u8 per slot: how bright what is drawn for it is, 0xFF full */
#define FADE_ALL     0x092E1220u   /* u8: the same for everything else */

/* A colour dimmed by the screen's fade: red, green and blue each less the fade (whatever the level is
   short of 0xFF, by the owner's slot at +13 if there is an owner), wrapping as bytes; alpha kept.
   Objects with +85 set are not dimmed. */
SFA_FUNC(0880E388, colour_dimmed) {
    gaddr obj = ARG(0), owner = ARG(2);
    uint32_t colour = ARG(1);
    if ((int8_t)*B(obj + 85) != 0) { RETURN(colour); return; }
    uint8_t level = owner != 0 ? *B(FADE_BY_SLOT + *B(owner + 13)) : *B(FADE_ALL);
    uint8_t less = (uint8_t)(0xFF - level);
    if (less == 0) { RETURN(colour); return; }
    uint8_t r = (uint8_t)(colour - less), g = (uint8_t)((colour >> 8) - less), b = (uint8_t)((colour >> 16) - less);
    RETURN((colour & 0xFF000000u) | (uint32_t)b << 16 | (uint32_t)g << 8 | r);
}

#define CACHE       0x092E8000u    /* 512 pointers to cached pictures */
#define CACHE_SIZE  512

/* Finds a cached picture: one in use (+14) made from the same data (+4), for the same +12 and +13 (the
   third argument's +86 and the first's +84). Marks it wanted for 8 more frames (+15) and answers it;
   0 if there is none. */
SFA_FUNC(0880EB84, cache_find) {
    uint32_t data = *W(ARG(1));
    uint16_t a = *H(ARG(2) + 86);
    uint8_t b = *B(ARG(0) + 84);
    const uint32_t *slots = guest_ptr(CACHE);
    for (int i = 0; i < CACHE_SIZE; i++) {
        gaddr p = slots[i];
        if (p == 0 || *B(p + 14) == 0 || *W(p + 4) != data || *B(p + 12) != a || *B(p + 13) != b) continue;
        *B(p + 15) = 8;
        RETURN(p);
        return;
    }
    RETURN(0);
}

/* ---- the pools ---- */

void f_08822FE4(Cpu *c);   /* object_make, above */
void f_088232B0(Cpu *c);   /* record_reset (misc.c): clears an object for reuse and sets +8 */

/* Each pool of 0xC0-byte objects is searched from its last object down; its count of free objects is a
   byte of its own. (first object, how many, free count, the +8 an object is given when it is freed) */
typedef struct { gaddr first; uint32_t count; gaddr free_count; uint8_t freed_kind; } Pool;
static const Pool POOL_8  = { 0x09512170u, 8,    0x095176F2u, 3 };
static const Pool POOL_32 = { 0x09512770u, 0x20, 0x095176F3u, 4 };
static const Pool POOL_16 = { 0x09514B70u, 0x10, 0x095176F5u, 0 };
static const Pool POOL_42 = { 0x09515770u, 0x2A, 0x095176F6u, 6 };

/* (kind, type): a new object from the pool, or 0. */
static void pool_make(Cpu *c, const Pool *p) {
    uint32_t kind = ARG(0) & 0xFF, type = ARG(1) & 0xFF;
    c->r[29] -= 0x30;
    CALL5(f_08822FE4, p->first + (p->count - 1) * OBJECT_SIZE, kind, type, p->count, p->free_count);
    c->r[29] += 0x30;
}
SFA_FUNC(08822E74, object_make_8)  { pool_make(c, &POOL_8); }
SFA_FUNC(08822EAC, object_make_32) { pool_make(c, &POOL_32); }
SFA_FUNC(08822F1C, object_make_16) { pool_make(c, &POOL_16); }
SFA_FUNC(08822F54, object_make_42) { pool_make(c, &POOL_42); }

/* (object): back to its pool. */
static void pool_free(Cpu *c, const Pool *p) {
    c->r[29] -= 0x20;
    c->r[5] = p->freed_kind;
    f_088232B0(c);
    *B(p->free_count) = (uint8_t)((int8_t)*B(p->free_count) + 1);
    c->r[29] += 0x20;
}
SFA_FUNC(0882319C, object_free_8)  { pool_free(c, &POOL_8); }
SFA_FUNC(088231C8, object_free_32) { pool_free(c, &POOL_32); }
SFA_FUNC(0882324C, object_free_42) { pool_free(c, &POOL_42); }

/* Frees a record of the pool counted at 0x095176F8: its first 16 bytes cleared, +8 = 6. */
SFA_FUNC(08823278, record_free_f8) {
    gaddr r = ARG(0);
    W(r)[0] = 0; W(r)[1] = 0; W(r)[2] = 0; W(r)[3] = 0;
    *B(r + 8) = 6;
    *B(0x095176F8u) = (uint8_t)((int8_t)*B(0x095176F8u) + 1);
}

/* Frees an object of the third list (0x94 bytes, all cleared; free count 0x095176F9). */
SFA_FUNC(088233BC, object_free_list_3) {
    for (uint32_t i = 0; i < 0x94u; i++) *B(ARG(0) + i) = 0;
    *B(0x095176F9u) = (uint8_t)((int8_t)*B(0x095176F9u) + 1);
}

/* Takes the first free entry of the 12-entry callback table (0x09517868, lists.c): marks it used and
   answers it, or 0. */
SFA_FUNC(08823074, callback_take) {
    gaddr e = 0x09517868u;
    for (int i = 0; i < 12; i++, e += 0x18) {
        if ((int8_t)*B(e) != 0) continue;
        *B(e) = 1;
        RETURN(e);
        return;
    }
    RETURN(0);
}

/* Empties the picture cache's 512 entries (16 bytes each at 0x092E6000; the pointers to them are at
   CACHE): each numbered from 0x100 at +8. */
SFA_FUNC(0880E998, cache_clear) {
    gaddr e = 0x092E6000u;
    for (uint32_t i = 0; i < CACHE_SIZE; i++, e += 16) {
        W(e)[0] = 0;
        W(e)[2] = i + 0x100;
        *B(e + 15) = 0; *B(e + 14) = 0; *B(e + 13) = 0; *B(e + 12) = 0;
        W(e)[1] = 0;
    }
}

/* (kind, type): one of the three 0x30-byte objects at 0x092E3E08 (list 6), looking from the last down;
   their free count is 0x095176F8. Answers it, or 0. */
SFA_FUNC(08822F8C, object_make_small) {
    uint8_t kind = (uint8_t)ARG(0), type = (uint8_t)ARG(1);
    gaddr o = 0x092E3E08u + 0x60;
    for (int left = 3; *B(o) != 0; o -= 0x30) if (--left == 0) { RETURN(0); return; }
    *B(0x095176F8u) = (uint8_t)((int8_t)*B(0x095176F8u) - 1);
    *B(o) = 1; *B(o + 2) = kind; *B(o + 3) = type;
    RETURN(o);
}

/* (kind, type): one of the third list's 32 objects (0x94 bytes, in use at +90), looking from the last
   down; set up with its kind (+91) and type (+92). Answers it, or 0. */
SFA_FUNC(08823338, object_make_list_3) {
    uint8_t kind = (uint8_t)ARG(0), type = (uint8_t)ARG(1);
    gaddr o = 0x092E2B88u + 0x11EC;
    for (int left = 0x20; *B(o + 90) != 0; o -= 0x94) if (--left == 0) { RETURN(0); return; }
    *B(0x095176F9u) = (uint8_t)((int8_t)*B(0x095176F9u) - 1);
    *B(o + 90) = 1; *B(o + 91) = kind; *B(o + 92) = type;
    *B(o + 137) = 0; *B(o + 136) = 0xFF;
    *B(o + 131) = 0; *B(o + 130) = 0; *B(o + 129) = 0; *B(o + 128) = 0;
    *B(o + 54) = 0xFF; *B(o + 89) = 0; *B(o + 88) = 0; *B(o + 138) = 0;
    RETURN(o);
}

/* ---- the pools emptied (0.70.0) ---- */

/* A pool of `count` objects made empty: the first cleared and given its list's number (+8), the rest
   copies of it; all of them free. */
static void pool_reset(gaddr pool, uint32_t count, gaddr free_count, uint8_t list) {
    *B(free_count) = (uint8_t)count;
    uint8_t *first = guest_ptr(pool);
    for (uint32_t i = 0; i < OBJECT_SIZE; i++) first[i] = 0;
    first[8] = list;
    for (uint32_t n = 1; n < count; n++) {
        uint8_t *to = first + n * OBJECT_SIZE;
        for (uint32_t i = 0; i < OBJECT_SIZE; i++) to[i] = first[i];
    }
}
SFA_FUNC(088222C4, pool_3_reset) { pool_reset(0x09512170u, 8,  0x095176F2u, 3); }
SFA_FUNC(08822934, pool_4_reset) { pool_reset(0x09512770u, 32, 0x095176F3u, 4); }
SFA_FUNC(088229CC, pool_7_reset) { pool_reset(0x09513F70u, 16, 0x095176F4u, 7); }
SFA_FUNC(08822444, pool_5_reset) { pool_reset(0x09514B70u, 16, 0x095176F5u, 5); }
SFA_FUNC(0882222C, pool_6_reset) { pool_reset(0x09515770u, 42, 0x095176F6u, 6); }
