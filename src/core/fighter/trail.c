/* The trail a fighter leaves (the after-images): where it was, frame by frame.
 *
 * A trail buffer holds four arrays indexed by frame: the animation entry (u32 at +0), the position
 * (s16 x at +512, y at +768) and the facing (u8 at +1024).
 */
#include "../decomp.h"

#define F_FACING    11
#define F_X         18
#define F_Y         22
#define F_ANIMATION 28

/* Records the fighter as it is now in the trail's entry `index`. */
SFA_FUNC(0882D95C, fighter_record_trail) {
    gaddr f = ARG(0), trail = ARG(1);
    uint32_t index = ARG(2) & 0xFFFF;
    *(uint32_t *)guest_ptr(trail + index * 4u) = *(uint32_t *)guest_ptr(f + F_ANIMATION);
    *(uint8_t *)guest_ptr(trail + index + 1024) = *(uint8_t *)guest_ptr(f + F_FACING);
    *(int16_t *)guest_ptr(trail + index * 2u + 512) = *(int16_t *)guest_ptr(f + F_X);
    *(int16_t *)guest_ptr(trail + index * 2u + 768) = *(int16_t *)guest_ptr(f + F_Y);
}

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_SIZE 0x498u
#define F_TRAIL_OFF 237          /* s8 */
#define F_FLAG_549  549          /* s8 */
#define TRAIL_FRAME_1 0x09523CA0u   /* u16: fighter 1's place in its trail, 0 - 127 */
#define TRAIL_FRAME_2 0x09523C9Eu   /* u16: fighter 2's */

void f_0882D95C(Cpu *c);   /* fighter_record_trail, above */

/* One frame of fighter `n`'s trail: the next of its 128 entries is recorded, unless another fighter has
   +237 set while this one has not, and its +549 is set. */
static void trail_step(Cpu *c, int n, gaddr place, gaddr trail) {
    gaddr f = FIGHTER_1 + (uint32_t)n * FIGHTER_SIZE;
    const int8_t *off = guest_ptr(FIGHTER_1 + F_TRAIL_OFF);
    if (off[n * (int)FIGHTER_SIZE] == 0) {
        int others = 0;
        for (int i = 0; i < 4; i++) if (i != n) others |= off[i * (int)FIGHTER_SIZE];
        if (others != 0 && *(int8_t *)guest_ptr(f + F_FLAG_549) != 0) return;
    }
    uint16_t *frame = guest_ptr(place);
    *frame = (uint16_t)((*frame + 1) & 0x7F);
    c->r[4] = f; c->r[5] = trail; c->r[6] = *frame; c->r[7] = *frame;
    f_0882D95C(c);
}
SFA_FUNC(0882D6FC, fighter_1_trail_step) { c->r[29] -= 0x20; trail_step(c, 0, TRAIL_FRAME_1, ARG(0)); c->r[29] += 0x20; }
SFA_FUNC(0882D794, fighter_2_trail_step) { c->r[29] -= 0x20; trail_step(c, 1, TRAIL_FRAME_2, ARG(0)); c->r[29] += 0x20; }
