/* Steps (0.154.0): the 2 functions of 9 - 200 instructions made ready by the batch before: the first
   ranking row's start, and the fighters pushed apart in pairs by layout. Offsets stay numbers until
   something shows what they are. What the original leaves in a0 - a1 at a call is set as the original
   sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u

void f_088BCDD8(Cpu *c);   /* ranking_row_start */
void f_08875FB8(Cpu *c);   /* fighters_push_apart */

/* (row): GAME +10 = 0xFF78, then the row's start. */
SFA_FUNC(088BD158, ranking_first_row_start) {
    c->r[29] -= 0x20;
    *H(GAME + 10) = 0xFF78;
    f_088BCDD8(c);
    c->r[29] += 0x20;
}

/* A pair pushed apart, both counts (+648) cleared first. */
static void push(Cpu *c, gaddr a, gaddr b) {
    *B(b + 648) = 0; *B(a + 648) = 0;
    c->r[4] = a; c->r[5] = b; f_08875FB8(c);
}

/* (): the pairs pushed apart by layout (BATTLE +56): 0 fighters 1 / 2 (2 / 1 when fighter 1's +653,
   which is cleared on both), 1 fighter 1 with 3 and 4, 2 fighter 2 with 3 and 4, 3 fighters 1 and 2
   with 3. */
SFA_FUNC(08873D5C, fighters_push_by_layout) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    int32_t l = S16(BATTLE + 56);
    if (l >= 0) c->r[16] = FIGHTER0;
    if (l == 0) {
        *B(FIGHTER1 + 648) = 0;
        int8_t first = S8(FIGHTER0 + 653);
        *B(FIGHTER0 + 648) = 0;
        if (first != 0) {
            *B(FIGHTER1 + 653) = 0; *B(FIGHTER0 + 653) = 0;
            c->r[4] = FIGHTER1; c->r[5] = FIGHTER0; f_08875FB8(c);
        } else {
            c->r[4] = FIGHTER0; c->r[5] = FIGHTER1; f_08875FB8(c);
        }
    } else if (l == 1) {
        push(c, FIGHTER0, FIGHTER2);
        push(c, FIGHTER0, FIGHTER3);
    } else if (l == 2) {
        c->r[16] = FIGHTER1;
        push(c, FIGHTER1, FIGHTER2);
        push(c, FIGHTER1, FIGHTER3);
    } else if (l == 3) {
        c->r[16] = FIGHTER2;
        push(c, FIGHTER0, FIGHTER2);
        push(c, FIGHTER1, FIGHTER2);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
