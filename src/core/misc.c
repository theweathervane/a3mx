/* Small functions not yet tied to a subsystem. */
#include "decomp.h"

#define GAME         0x08E75910u
#define G_MODE       114
#define CAMERA_LAYER 0x092E88A0u     /* the fighters' layer */
#define RING         0x092D9EF0u     /* 64 entries of four u16 */
#define RING_NEXT    0x092D9EEDu     /* u8: the entry the next call fills */

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* Four values into the next entry of a 64-entry ring. */
SFA_FUNC(08809A08, ring_put) {
    uint16_t *e = guest_ptr(RING + *B(RING_NEXT) * 8u);
    e[0] = (uint16_t)ARG(0); e[1] = (uint16_t)ARG(1); e[2] = (uint16_t)ARG(2); e[3] = (uint16_t)ARG(3);
    *B(RING_NEXT) = (uint8_t)((*B(RING_NEXT) + 1) & 0x3F);
}

/* A 0xAC-byte record cleared for use, with its kind (+8) set. */
SFA_FUNC(088232B0, record_reset) {
    gaddr r = ARG(0);
    W(r)[0] = 0; W(r)[1] = 0; W(r)[2] = 0; W(r)[3] = 0;
    *B(r + 72) = 0;
    *H(r + 38) = 0;
    for (int i = 0; i < 5; i++) W(r + 40)[i] = 0;
    *B(r + 8) = (uint8_t)ARG(1);
    *B(r + 138) = 0; *B(r + 170) = 0; *B(r + 171) = 0;
}

/* The mode is neither Dramatic Battle (8) nor Variable Battle (15). */
SFA_FUNC(088939AC, mode_is_not_team) {
    int8_t mode = (int8_t)*B(GAME + G_MODE);
    RETURN(mode != 8 && mode != 15);
}

/* The low byte of GAME + 16 has reached 7: the object's +17 steps on and 0x08B02800 is set to -0x180. */
SFA_FUNC(088ABA0C, wait_game_16) {
    if ((*H(GAME + 16) & 0xFF) < 7) return;
    *B(ARG(0) + 17) = (uint8_t)(*B(ARG(0) + 17) + 1);
    *H(0x08B02800u) = 0xFE80;
}

/* Three of an object's switches (+78, +79, +80) become bits 0 to 2 of the fighters' layer + 123. */
SFA_FUNC(088AC4D8, layer_flags_from_object) {
    gaddr o = ARG(0);
    *B(CAMERA_LAYER + 123) = (uint8_t)((*B(o + 78) != 0) | (*B(o + 79) != 0) << 1 | (*B(o + 80) != 0) << 2);
}
