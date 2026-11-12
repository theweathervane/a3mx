/* Questions about what a fighter's player is pressing.
 *
 * 0x08864DFC turns each fighter's pad into game bits once a frame (docs/PSP_PROGRAM_MAP.md): the
 * directions it holds in fighter + 752, and its buttons in + 760. The fight's routines read them through
 * these.
 */
#include "../decomp.h"

#define F_DIRECTIONS 752     /* u16: 1 right, 2 left, 4 down, 8 up */
#define F_BUTTONS    760     /* u16: 0x3F0 the six attack buttons, 0x4000 SELECT */

#define DIR_SIDEWAYS 0x3
#define DIR_DOWN     0x4
#define DIR_VERTICAL 0xC
#define BTN_ATTACKS  0x3F0
#define BTN_SELECT   0x4000

static inline uint16_t *H(gaddr a) { return guest_ptr(a); }

/* Left or right, but only while neither up nor down is held. */
SFA_FUNC(08861D5C, fighter_sideways_only) {
    uint16_t d = *H(ARG(0) + F_DIRECTIONS);
    RETURN((d & DIR_VERTICAL) == 0 ? d & DIR_SIDEWAYS : 0);
}

SFA_FUNC(088622C0, fighter_holds_sideways) { RETURN(*H(ARG(0) + F_DIRECTIONS) & DIR_SIDEWAYS); }

SFA_FUNC(088623BC, fighter_holds_down) { RETURN(*H(ARG(0) + F_DIRECTIONS) & DIR_DOWN); }

SFA_FUNC(088639C0, fighter_attack_buttons) { RETURN(*H(ARG(0) + F_BUTTONS) & BTN_ATTACKS); }

SFA_FUNC(0886A2D8, fighter_holds_select) { RETURN((*H(ARG(0) + F_BUTTONS) & BTN_SELECT) != 0); }
