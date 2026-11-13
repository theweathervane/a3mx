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
