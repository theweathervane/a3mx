/* The fight's camera: small steps the camera's routines share.
 *
 * The fighters' layer (0x092E88A0) holds the camera: its x at +34 and the stage's limits at +68 (left)
 * and +70 (right).
 */
#include "../decomp.h"

#define L_CAMERA_X   34     /* s16 */
#define L_LIMIT_LEFT 68     /* s16 */
#define L_LIMIT_RIGHT 70    /* s16 */
#define F_Y          22     /* s16: a fighter's height (40 on the ground) */
#define CAMERA_STEP  5      /* the most the camera moves in a frame */

static inline int16_t *S(gaddr a) { return guest_ptr(a); }

/* Moves the camera right by `by` (5 at most) and answers where it is, the right limit if it has
   passed it. The stored x is left past the limit: the caller stores the answer. */
SFA_FUNC(0881E49C, camera_step_right) {
    gaddr layer = ARG(0);
    int16_t by = (int16_t)ARG(1), limit = *S(layer + L_LIMIT_RIGHT);
    if (by >= 6) by = CAMERA_STEP;
    *S(layer + L_CAMERA_X) = (int16_t)(*S(layer + L_CAMERA_X) + by);
    int16_t x = *S(layer + L_CAMERA_X);
    RETURN((int32_t)(x < limit ? x : limit));
}

/* The same to the left: `by` is negative as it comes in. */
SFA_FUNC(0881E4E4, camera_step_left) {
    gaddr layer = ARG(0);
    int16_t by = (int16_t)-(int16_t)ARG(1), limit = *S(layer + L_LIMIT_LEFT);
    if (by >= 6) by = CAMERA_STEP;
    *S(layer + L_CAMERA_X) = (int16_t)(*S(layer + L_CAMERA_X) - by);
    int16_t x = *S(layer + L_CAMERA_X);
    RETURN((int32_t)(limit < x ? x : limit));
}

/* The higher of two fighters' heights, into `out`. */
SFA_FUNC(0881EB78, camera_higher_fighter) {
    int16_t a = *S(ARG(0) + F_Y), b = *S(ARG(1) + F_Y);
    *S(ARG(2)) = b < a ? a : b;
}

/* ---- the background layers follow the camera ---- */

#define CAMERA   0x092E88A0u     /* the fighters' layer: +8 / +12 where it starts, +32 / +36 where it is */
#define SHAKE    0x092E5F78u     /* +112: an x offset taken off before the layers move */

static inline int32_t *L(gaddr a) { return guest_ptr(a); }

/* A layer's x from the camera's: the camera's travel times the layer's rate in sixteenths, from the
   layer's own start (+8) and offset (+80). +59 notes which side of its limit (+40) it is. */
SFA_FUNC(0881E058, layer_follow_x) {
    gaddr layer = ARG(0);
    int32_t travel = (int32_t)((uint32_t)*L(CAMERA + 32) - (uint32_t)*L(CAMERA + 8) - (uint32_t)*L(SHAKE + 112));
    int32_t scaled = (int32_t)((uint32_t)travel * (uint32_t)(int32_t)(int16_t)ARG(1)) / 16;
    int32_t x = (int32_t)((uint32_t)scaled + (uint32_t)*L(layer + 8) + (uint32_t)*L(layer + 80));
    *(uint8_t *)guest_ptr(layer + 59) = x < *L(layer + 40) ? 8 : 12;
    *L(layer + 32) = x;
}

/* The same for y (start +12, offset +84, limit +44, side in +58). */
SFA_FUNC(0881E148, layer_follow_y) {
    gaddr layer = ARG(0);
    int32_t travel = (int32_t)((uint32_t)*L(CAMERA + 36) - (uint32_t)*L(CAMERA + 12));
    int32_t scaled = (int32_t)((uint32_t)travel * (uint32_t)(int32_t)(int16_t)ARG(1)) / 16;
    int32_t y = (int32_t)((uint32_t)scaled + (uint32_t)*L(layer + 12) + (uint32_t)*L(layer + 84));
    *(uint8_t *)guest_ptr(layer + 58) = y < *L(layer + 44) ? 4 : 0;
    *L(layer + 36) = y;
}

/* The span two fighters cover: the leftmost left edge into `lo` and the rightmost right edge into `hi`,
   each fighter reaching its half-width (+232) either side of its x. */
SFA_FUNC(0881E7DC, camera_fighters_span) {
    gaddr a = ARG(0), b = ARG(1);
    int16_t al = (int16_t)(*S(a + 18) - *S(a + 232)), bl = (int16_t)(*S(b + 18) - *S(b + 232));
    int16_t ar = (int16_t)(*S(a + 18) + *S(a + 232)), br = (int16_t)(*S(b + 18) + *S(b + 232));
    *S(ARG(2)) = bl < al ? bl : al;
    *S(ARG(3)) = ar < br ? br : ar;
}
