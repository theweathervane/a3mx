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
