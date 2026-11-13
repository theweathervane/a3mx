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
