/* Animations of the 0xC0-byte objects (0x09512770 on: docs/PSP_PROGRAM_MAP.md). */
#include "../decomp.h"

#define O_ANIMATION 28     /* u32: the animation's current entry */
#define O_FRAME     74     /* 4 bytes copied from the entry: u8, then three s8 */
#define O_TIMER     152    /* u32: cleared when an animation starts */

/* Starts an animation: the object points at its first entry and takes the entry's first four bytes. */
SFA_FUNC(0882196C, object_set_animation) {
    gaddr obj = ARG(0), entry = ARG(1);
    const uint8_t *e = guest_ptr(entry);
    uint8_t *o = guest_ptr(obj);
    *(uint32_t *)(o + O_ANIMATION) = entry;
    o[O_FRAME] = e[0]; o[O_FRAME + 1] = e[1]; o[O_FRAME + 2] = e[2]; o[O_FRAME + 3] = e[3];
    *(uint32_t *)(o + O_TIMER) = 0;
}
