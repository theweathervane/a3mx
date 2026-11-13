/* Questions about what a fighter's player is pressing.
 *
 * 0x08864DFC turns each fighter's pad into game bits once a frame (docs/PSP_PROGRAM_MAP.md), all of
 * them in fighter + 752; 0x08859650 (state.c) then works out which came on this frame (+ 760) and which
 * went off (+ 762). The fight's routines read them through these.
 */
#include "../decomp.h"

#define F_HELD       752     /* u16: 1 right, 2 left, 4 down, 8 up; 0x3F0 the six attack buttons */
#define F_PRESSED    760     /* u16: the bits of + 752 that came on this frame; 0x4000 SELECT */

#define DIR_SIDEWAYS 0x3
#define DIR_DOWN     0x4
#define DIR_VERTICAL 0xC
#define BTN_ATTACKS  0x3F0
#define BTN_SELECT   0x4000

static inline uint16_t *H(gaddr a) { return guest_ptr(a); }

/* Left or right, but only while neither up nor down is held. */
SFA_FUNC(08861D5C, fighter_sideways_only) {
    uint16_t d = *H(ARG(0) + F_HELD);
    RETURN((d & DIR_VERTICAL) == 0 ? d & DIR_SIDEWAYS : 0);
}

SFA_FUNC(088622C0, fighter_holds_sideways) { RETURN(*H(ARG(0) + F_HELD) & DIR_SIDEWAYS); }

SFA_FUNC(088623BC, fighter_holds_down) { RETURN(*H(ARG(0) + F_HELD) & DIR_DOWN); }

/* The attack buttons pressed this frame. */
SFA_FUNC(088639C0, fighter_pressed_attacks) { RETURN(*H(ARG(0) + F_PRESSED) & BTN_ATTACKS); }

SFA_FUNC(0886A2D8, fighter_pressed_select) { RETURN((*H(ARG(0) + F_PRESSED) & BTN_SELECT) != 0); }
