/* Animations of the 0xC0-byte objects (0x09512770 on: docs/PSP_PROGRAM_MAP.md). */
#include "../decomp.h"

#define O_ANIMATION 28     /* u32: the animation's current entry */
#define O_FRAME     74     /* 4 bytes copied from the entry: u8, then three s8 */
#define O_TIMER     152    /* u32: 1 when the picture has just changed; cleared when an animation starts */

/* Starts an animation: the object points at its first entry and takes the entry's first four bytes. */
SFA_FUNC(0882196C, object_set_animation) {
    gaddr obj = ARG(0), entry = ARG(1);
    const uint8_t *e = guest_ptr(entry);
    uint8_t *o = guest_ptr(obj);
    *(uint32_t *)(o + O_ANIMATION) = entry;
    o[O_FRAME] = e[0]; o[O_FRAME + 1] = e[1]; o[O_FRAME + 2] = e[2]; o[O_FRAME + 3] = e[3];
    *(uint32_t *)(o + O_TIMER) = 0;
}

/* An animation entry is 8 bytes: +0 how many frames it shows, +1 flags (0x40: the animation ends here),
   +2, +3 two more bytes the object copies, +4 u16 the picture, +6 s16 how many entries on the next is. */
#define E_FLAGS 1
#define E_END   0x40
#define E_PICTURE 4
#define E_NEXT  6

/* One frame of an object's animation. When the entry's time is up it moves to the next one (+152 says
   whether the picture changed); an entry that ends the animation sets +75 to -1 on its last frame and
   stays. */
SFA_FUNC(088219B0, object_animate) {
    gaddr obj = ARG(0), entry = *(uint32_t *)guest_ptr(obj + O_ANIMATION);
    int8_t *o = guest_ptr(obj);
    const int8_t *e = guest_ptr(entry);
    int8_t left = (int8_t)(o[O_FRAME] - 1);
    o[O_FRAME] = left;
    if (left != 0) {
        if (left == 1 && (e[E_FLAGS] & E_END)) o[O_FRAME + 1] = -1;
        return;
    }
    if (e[E_FLAGS] & E_END) { o[O_FRAME + 1] = -1; return; }
    gaddr next = entry + (uint32_t)((int32_t)*(int16_t *)guest_ptr(entry + E_NEXT) << 3);
    const uint8_t *n = guest_ptr(next);
    *(uint32_t *)guest_ptr(obj + O_TIMER) = *(uint16_t *)guest_ptr(entry + E_PICTURE) != *(uint16_t *)guest_ptr(next + E_PICTURE);
    *(uint32_t *)guest_ptr(obj + O_ANIMATION) = next;
    o[O_FRAME] = (int8_t)n[0]; o[O_FRAME + 1] = (int8_t)n[1]; o[O_FRAME + 2] = (int8_t)n[2]; o[O_FRAME + 3] = (int8_t)n[3];
}

/* The routine table 0x08A0E05C's entry for the object's +14. */
SFA_FUNC(08821330, object_routine_by_14) {
    RETURN(*(uint32_t *)guest_ptr(0x08A0E05Cu + *(uint8_t *)guest_ptr(ARG(0) + 14) * 4u));
}
