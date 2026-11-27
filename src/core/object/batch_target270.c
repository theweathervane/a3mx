/* Picking the opponent (+92) in a two-on-one fight: of two candidates the one still standing, the one on
   the ground (by the pick mode), else the nearer on the side the fighter faces (0x0886A3FC), for the
   team's pair chosen by the round mode and team size (0x0886A2F4). Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886A3FC(Cpu *c);

/* (fighter, a, b, mode): +92 = a or b: one with no vitality left (+112 below zero) loses; mode 0 takes the
   one on the ground (+73 clear) when the other is in the air, mode 1 the one in the air; otherwise the
   nearer of the two in front (by +18 against the facing +697), else the other. */
SFA_FUNC(0886A3FC, pick_opponent) {
    gaddr o = ARG(0), a = ARG(1), b = ARG(2); uint32_t mode = ARG(3);
    int32_t ha = S16(a + 112);
    if (ha < 0) { *W(o + 92) = b; return; }
    if (S16(b + 112) < 0) { *W(o + 92) = a; return; }
    if (mode == 0) {
        if (S8(a + 73) != 0) { *W(o + 92) = b; return; }
        if (S8(b + 73) != 0) { *W(o + 92) = a; return; }
    } else if (mode == 1) {
        if (S8(a + 73) == 0) { *W(o + 92) = b; return; }
        if (S8(b + 73) == 0) { *W(o + 92) = a; return; }
    }
    int32_t x = S16(o + 18);
    int32_t da = (int16_t)(x - S16(a + 18)), db = (int16_t)(x - S16(b + 18));
    if (S8(o + 697) == 0) {
        if (da < 0) *W(o + 92) = b;
        else if (db < 0) *W(o + 92) = a;
        else *W(o + 92) = da < db ? a : b;
    } else {
        if (da > 0) *W(o + 92) = b;
        else if (db > 0) *W(o + 92) = a;
        else *W(o + 92) = db < da ? a : b;
    }
}

/* (fighter, mode): outside round modes 0xF, 0x13, 7 with 0x092E1277 at 5 and 0x10 with 0x092E12C7 at 3,
   by the team size (0x092E1260): with 1 or 2 the fighters without +337 bit 1 pick from 0x092E47D0 /
   0x092E4C68, with 3 those with it from 0x092E3EA0 / 0x092E4338. */
SFA_FUNC(0886A2F4, pick_opponent_team) {
    gaddr o = ARG(0); uint32_t mode = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t m = S8(0x08E75910u + 114);
    const gaddr g = 0x092E1228u;
    int skip = m == 0xF || (m == 7 && S8(g + 79) == 5) || m == 0x13 || (m == 0x10 && S8(g + 159) == 3);
    if (!skip) {
        int32_t n = S16(g + 56);
        int bit = (S8(o + 337) & 2) != 0;
        if (n > 0 && n < 3 && !bit) {
            ARG(0) = o; ARG(1) = 0x092E47D0u; ARG(2) = 0x092E4C68u; ARG(3) = mode;
            c->r[31] = 0x0886A3BCu; f_0886A3FC(c);
        } else if (n == 3 && bit) {
            ARG(0) = o; ARG(1) = 0x092E3EA0u; ARG(2) = 0x092E4338u; ARG(3) = mode;
            c->r[31] = 0x0886A3F0u; f_0886A3FC(c);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886A2F4(Cpu *c); void f_08873BF0(Cpu *c); void f_0886AAE8(Cpu *c); void f_0886A540(Cpu *c);
void f_0886A9FC(Cpu *c);

/* (fighter, throw n): an air throw: in the air (+80), with +251 or the game not paused (0x092E1236), the
   opponent picked (mode 2) still standing and not held (+573), its box (+256) open and it in the air but
   neither held (+135) nor with box +3 clear; then 0x08873BF0 on the throw's entry (+224 + 32 n) and the
   box's (+220 + 8 x box +3) decides and 0x0886AAE8 throws. 1 when it does. */
SFA_FUNC(0886A540, air_throw) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = o;
    *W(sp + 16) = s0; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = n;
    uint32_t ok = 0;
    if (S8(o + 80) == 0) goto out;
    if (S8(o + 251) == 0 && S8(0x092E1228u + 14) != 0) goto out;
    ARG(0) = o; ARG(1) = 2; c->r[31] = 0x0886A590u; f_0886A2F4(c);
    {
        gaddr p = *W(o + 92);
        c->r[18] = p;
        if (S16(p + 112) < 0 || S8(p + 573) != 0) goto out;
        gaddr q = *W(p + 256);
        if (*B(q) == 0 && *B(q + 1) == 0 && *B(q + 2) == 0) goto out;
        uint32_t k = *B(q + 3);
        if ((S8(p + 73) | S8(p + 135)) != 0 || k == 0) goto out;
        gaddr e = *W(o + 224) + (n << 5);
        c->r[16] = e;
        ARG(0) = o; ARG(1) = p; ARG(2) = e; ARG(3) = *W(p + 220) + (k << 3);
        c->r[31] = 0x0886A620u; f_08873BF0(c);
        if (c->r[2] == 0) goto out;
        ARG(0) = o; ARG(1) = p; ARG(2) = e; c->r[31] = 0x0886A634u; f_0886AAE8(c);
        ok = 1;
    }
out:
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, throw n): a throw: in the air only from a special state (1 / 0 / 7) as an air throw; on the
   ground the opponent picked (mode 0), not thrown (+543) or held (+573), in state 1 with +5 0 or 2, or held
   by +816, or state 1 / 1 in step 6, is thrown by 0x0886A9FC (its result); else 0. */
SFA_FUNC(0886A6D4, throw_check) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = o;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = n;
    if (S8(o + 80) != 0) {
        if (*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7) { ARG(0) = o; ARG(1) = n; c->r[31] = 0x0886A734u; f_0886A540(c); }
        else c->r[2] = 0;
        goto out;
    }
    ARG(0) = o; ARG(1) = 0; c->r[31] = 0x0886A754u; f_0886A2F4(c);
    {
        gaddr p = *W(o + 92);
        if ((S8(p + 543) | S8(p + 573)) != 0) { c->r[2] = 0; goto out; }
        uint32_t a4 = *B(p + 4), a5 = *B(p + 5);
        uint32_t back;
        if (a4 == 1 && a5 == 2) back = 0x0886A840u;
        else if (a4 == 1 && a5 == 0) back = 0x0886A81Cu;
        else if (*W(p + 816) != 0) back = 0x0886A7FCu;
        else if (a4 != 1 || a5 != 1) { c->r[2] = 0; goto out; }
        else if (*B(p + 116) == 6) back = 0x0886A87Cu;
        else { c->r[2] = 0; goto out; }
        ARG(0) = o; ARG(1) = p; ARG(2) = n; c->r[31] = back; f_0886A9FC(c);
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
