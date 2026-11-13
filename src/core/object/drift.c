/* Things that fall across the screen (0x94-byte objects): each frame they move by their speed and wrap
   round when they leave. Positions and speeds are 16.16 fixed point. */
#include "../decomp.h"

#define D_X      4       /* s32 */
#define D_Y      8       /* s32 */
#define D_TYPE   92      /* u8: type 0x11 comes back in twice as far to the right */
#define D_SPEED_X 104    /* s32 */
#define D_SPEED_Y 108    /* s32 */

static inline int32_t *L(gaddr a) { return guest_ptr(a); }

/* Sideways: off the left edge it comes back at x 128 (256 for type 0x11). */
SFA_FUNC(0888A1B0, drift_sideways) {
    gaddr o = ARG(0);
    int32_t x = (int32_t)((uint32_t)*L(o + D_X) + (uint32_t)*L(o + D_SPEED_X));
    *L(o + D_X) = x;
    if (x <= 0) *L(o + D_X) = *(uint8_t *)guest_ptr(o + D_TYPE) == 0x11 ? 0x1000000 : 0x800000;
}

/* Down: at y 240 it starts again from the top. */
SFA_FUNC(0888A1EC, drift_down) {
    gaddr o = ARG(0);
    int32_t y = (int32_t)((uint32_t)*L(o + D_Y) + (uint32_t)*L(o + D_SPEED_Y));
    *L(o + D_Y) = y;
    if (y >= 0xF00000) *L(o + D_Y) = 0;
}
