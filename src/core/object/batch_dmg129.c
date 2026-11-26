/* The current hit's damage taken off a fighter's vitality (0x088729C4, a leaf): gated by the attacker's
   damage rule, raised by an eighth for a guard-crushed / weakened fighter, and added to the combo
   damage. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME       0x08E75910u
#define G_MODE     114
#define HIT_DAMAGE 0x09358430u   /* s16: the damage of the hit being applied */

/* (attacker, fighter, hit): the attacker's rule (+790): 0 always; 1 only once the fighter's +126 is 3 or
   more; 2 only with the attacker's +80; 3 once the fighter's +791 is set (set here the first time +816
   is), or with +816, or (resetting +791) not below +126 = 2; 4 only for hits of kind 2 (+19); 5 only with
   the attacker's +135; others always. Never while the fighter's +810 is 4. The damage (an eighth more
   with +588) comes off +112 (held at 0 in mode 2, or for hits with +8 bit 7), adds to the combo damage
   +778 (its best +780), and is the last hit +782. */
SFA_FUNC(088729C4, fighter_damage_apply) {
    gaddr a = ARG(0), f = ARG(1), hit = ARG(2);
    int32_t dmg = S16(HIT_DAMAGE);
    int32_t rule = S8(a + 790);
    c->r[7] = (uint32_t)dmg;
    int apply;
    switch (rule) {
    case 0: apply = 1; break;
    case 1: apply = S8(f + 126) >= 3; break;
    case 2: apply = S8(a + 80) != 0; break;
    case 3:
        if (S8(f + 791) == 0) {
            if (*W(f + 816) == 0) return;
            *B(f + 791) = 1;
            apply = 1;
        } else if (*W(f + 816) != 0) apply = 1;
        else if (S8(f + 126) < 2) { *B(f + 791) = 0; return; }
        else apply = 1;
        break;
    case 4: apply = *B(hit + 19) == 2; break;
    case 5: apply = S8(a + 135) != 0; break;
    default: apply = 1; break;
    }
    if (!apply || S8(f + 810) == 4) return;
    if (S8(f + 588) != 0) dmg = (int16_t)(dmg + (dmg >> 3));
    int32_t combo = S16(f + 778);
    *H(f + 112) = (uint16_t)(S16(f + 112) - dmg);
    int hold;
    if (S8(GAME + G_MODE) == 2) hold = 1;
    else hold = (*B(hit + 8) & 0x80) != 0;
    if (hold && S16(f + 112) < 0) *H(f + 112) = 0;
    combo += dmg;
    *H(f + 778) = (uint16_t)combo;
    *H(f + 782) = (uint16_t)dmg;
    if (S16(f + 780) < S16(f + 778)) *H(f + 780) = *H(f + 778);
}
