/* Steps (0.156.0): the next ready function of 201 - 400 instructions: the fighters matched to a list of
   three characters, each first match given its row of values. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u

void f_08812FD4(Cpu *c);   /* fighter_character_for_display */

/* (list): the fighters of the layout (BATTLE +56: 0 fighters 1 / 2; 1 1 / 3 / 4; 2 2 / 3 / 4; 3 1 / 2 / 3)
   checked against the three characters of the list (s16 each): for each, every fighter not yet taken
   that shows that character (fighter_character_for_display) gets, by its side (+337), the values of
   that row of 0x092D9EC0 / 0x092D9ED0 / 0x092D9EE0 in 0x08E75AD8 / 0x092D9D40 / 0x092D9E00, the row's
   number in 0x092E89E8 and 0x09358BD0 + 4 i. Returns the fighters taken (bit i) with 0x10 / 0x100 /
   0x1000 for the rows that matched. */
SFA_FUNC(088158A4, fighters_by_character_list) {
    gaddr list = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23], s30 = c->r[30];
    c->r[29] -= 0x70;
    gaddr arr = c->r[29] + 16;
    int32_t l = S16(BATTLE + 56);
    int32_t n = 3;
    if (l == 0) { *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER1; n = 2; }
    else if (l == 1) { *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER2; *W(arr + 8) = FIGHTER3; }
    else if (l == 2) { *W(arr) = FIGHTER1; *W(arr + 4) = FIGHTER2; *W(arr + 8) = FIGHTER3; }
    else if (l == 3) { *W(arr) = FIGHTER0; *W(arr + 4) = FIGHTER1; *W(arr + 8) = FIGHTER2; }
    uint32_t taken = 0;
    static const uint32_t mark[3] = { 0x10, 0x100, 0x1000 };
    for (uint32_t row = 0; row < 3; row++) {
        int32_t want = S16(list + row * 2);
        for (int32_t i = 0; i < n; i++) {
            c->r[4] = *W(arr + (uint32_t)i * 4); f_08812FD4(c);
            if ((uint32_t)want != c->r[2]) continue;
            uint32_t bit = 1u << (i & 31);
            if (taken & bit) continue;
            gaddr f = *W(arr + (uint32_t)i * 4);
            uint32_t s = (uint32_t)(int32_t)S8(f + 337) * 4;
            *W(0x08E75AD8u + s) = *W(0x092D9EC0u + row * 4);
            *W(0x092D9D40u + s) = *W(0x092D9ED0u + row * 4);
            *W(0x092D9E00u + s) = *W(0x092D9EE0u + row * 4);
            *W(0x092E89E8u + s) = row;
            taken = (((taken | bit) & 0xFFFF) | mark[row]) & 0xFFFF;
            *W(0x09358BD0u + (uint32_t)i * 4) = row;
        }
    }
    RETURN(taken);
    c->r[29] += 0x70;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23; c->r[30] = s30;
}
