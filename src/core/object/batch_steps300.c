/* More of what the new leaves set free: a helper fighter's start beside its owner, a super's start
   from the ground or the air, the taunt that throws something, and a dive kick's step with its throw.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08876998(Cpu *c); void f_0892EF20(Cpu *c); void f_0887684C(Cpu *c); void f_088D34D8(Cpu *c);
void f_088D3218(Cpu *c); void f_0886C8F4(Cpu *c); void f_088D3278(Cpu *c); void f_0886D2EC(Cpu *c);
void f_0886C6EC(Cpu *c); void f_08869FEC(Cpu *c); void f_0886163C(Cpu *c); void f_0887E5A4(Cpu *c);
void f_08961448(Cpu *c); void f_0886CA2C(Cpu *c); void f_088616EC(Cpu *c); void f_0895DE28(Cpu *c);
void f_08861848(Cpu *c); void f_0886B100(Cpu *c); void f_0895DD9C(Cpu *c); void f_08867C30(Cpu *c);
void f_0895DD5C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (helper): a helper fighter's start: state 1, its owner's (+88) +24, +86 and +164, its own tables
   (+352, +356, +128, +200), 0x08876998, at rest on the ground; kind 2 (+3) takes its script by
   vitality (0x0892EF20); placed 0x5A in front of where it is and 0x44 up, moving 3.0 that way; +344
   and +139 from the tables 0x08A6BC28 / 0x08A6BC2E by its +346, and 0x0887684C with that. A frame
   keeping s0, s1. */
SFA_FUNC(0892EBA0, helper_start) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = x;
    *B(x + 4) = 1;
    *W(c->r[29] + 20) = s1; c->r[17] = 0;
    gaddr own = *W(x + 88);
    *B(x + 9) = 0;
    *H(x + 24) = *H(own + 24);
    *H(x + 86) = *H(own + 86);
    *W(x + 164) = *W(own + 164);
    *W(x + 352) = 0x08A6AA48u; *W(x + 356) = 0x08A6AA74u; *W(x + 128) = 0x08A6ABF8u; *W(x + 200) = 0x08A6AB00u;
    *W(c->r[29] + 24) = ra;
    call1(c, x, f_08876998, 0x0892EC18u);
    *W(x + 100) = 0;
    uint32_t kind = *B(x + 3);
    *B(x + 73) = 0;
    if (kind == 2) call1(c, x, f_0892EF20, 0x0892EC34u);
    int right = *B(x + 11) != 0;
    int32_t px = S16(x + 18), py = S16(x + 22);
    *W(x + 96) = right ? 0x30000u : 0xFFFD0000u;
    *H(x + 18) = (uint16_t)(px + (right ? 0x5A : -0x5A));
    int32_t k = S8(x + 346);
    *H(x + 22) = (uint16_t)(py + 0x44);
    *B(x + 344) = *B(0x08A6BC28u + (uint32_t)k);
    *B(x + 139) = *B(0x08A6BC2Eu + (uint32_t)k);
    call2(c, x, (uint32_t)k & 0xFFFF, f_0887684C, 0x0892ECA0u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a super's start for the choice 0, on its command (0x088D34D8 with the fighter's +0x380):
   in the air when the meter holds its cost (0x088D3218) and 0x0886C8F4 passes; on the ground when
   0x0886C6EC passes, facing the opponent; then 0x088D3278, move 6 with +235 set and 0x0886D2EC: 1;
   else 0. A frame keeping s0..s2. */
SFA_FUNC(089B9500, super_6_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 24) = s2; c->r[18] = o + 0x380;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    call2(c, o, o + 0x380, f_088D34D8, 0x089B9524u);
    c->r[17] = 1;
    uint32_t done = 0;
    if (c->r[2] == 1 && S8(o + 386) == 0) {
        if (S8(o + 73) == 1) {
            call1(c, o, f_088D3218, 0x089B9550u);
            if (c->r[2] == 1) {
                call1(c, o, f_0886C8F4, 0x089B9560u);
                if (c->r[2] == 1) {
                    call1(c, o, f_088D3278, 0x089B9570u);
                    *B(o + 236) = 6; *B(o + 235) = 1;
                    call2(c, o, o + 0x380, f_0886D2EC, 0x089B9588u);
                    done = 1;
                }
            }
        } else {
            call1(c, o, f_0886C6EC, 0x089B95ACu);
            if (c->r[2] == 1) {
                call1(c, o, f_088D3278, 0x089B95BCu);
                int8_t facing = S8(o + 697);
                *B(o + 236) = 6; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 1;
                call2(c, o, o + 0x380, f_0886D2EC, 0x089B95DCu);
                done = 1;
            }
        }
    }
    c->r[2] = done;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the throwing taunt of character 31, and of character 12 with the choice 1: a player with
   nothing stored (+816), the opponent's +690 clear and forward held (+752 bit 1), on all three
   punches or when 0x08869FEC passes: +691 set, the opponent's +690 = 31, the cue 0x32 (on the ground
   0x30 standing, 0x31 crouching) and the request 0x4F with 0x3A / 0x43 (0x2F on the ground). A 0x30
   frame keeping s0..s5. */
SFA_FUNC(0885CF5C, taunt_throw) {
    gaddr o = ARG(0);
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = o;
    *W(sp + 36) = sv[5];
    gaddr other = *W(o + 92);
    c->r[21] = other;
    int32_t who = S8(o + 338);
    *W(sp + 20) = sv[1]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    c->r[20] = 0x3A; c->r[19] = 0x43; c->r[17] = 0x1F;
    *W(sp + 24) = sv[2]; *W(sp + 40) = sv[6];
    c->r[18] = 0x32;
    if ((who == 0x1F || (who == 0xC && S8(o + 386) == 1)) && *W(o + 816) == 0 && S8(other + 690) == 0 &&
        S8(o + 373) == 0 && (*H(o + 752) & 2)) {
        int ok = (*H(o + 760) & 0x70) == 0x70;
        if (!ok) {
            call1(c, o, f_08869FEC, 0x0885D008u);
            ok = c->r[2] != 0;
        }
        if (ok) {
            *B(o + 691) = 1;
            *B(other + 690) = 0x1F;
            if (S8(o + 73) == 0) {
                c->r[18] = S8(o + 696) == 0 ? 0x30 : 0x31;
                c->r[20] = 0x3A; c->r[19] = 0x2F;
            }
            call2(c, o, c->r[18], f_0886163C, 0x0885D04Cu);
            c->r[6] = c->r[19];
            call2(c, o, c->r[20], f_0887E5A4, 0x0885D05Cu);
        }
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (fighter): a dive kick's step: after 0x08961448, when that is negative and the ground is reached:
   step 6 on the ground, the meter by 3 and the animation 0x1A. Else, once 0x0895DE28 says so: the cpu
   throws when the check 0x0886B100 (0x59) passes (0x0895DD9C) or is ticked; a player with no kick down
   needs 0x08867C30 (14) first, then with a direction held tries the same throw, and else kicks
   (0x0895DD5C). Otherwise ticked. */
SFA_FUNC(0895DC2C, dive_kick_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x0895DC40u; f_08961448(c);
    if ((int32_t)c->r[2] < 0 && !(S16(o + 132) < S16(o + 22))) {
        int16_t ground = S16(o + 132);
        *B(o + 7) = 6;
        *H(o + 22) = (uint16_t)ground; *H(o + 20) = 0;
        *B(o + 73) = 0;
        call2(c, o, 3, f_0886CA2C, 0x0895DC78u);
        call2(c, o, 0x1A, f_088616EC, 0x0895DC84u);
        goto out;
    }
    call1(c, o, f_0895DE28, 0x0895DC94u);
    if (c->r[2] == 0) { call1(c, o, f_08861848, 0x0895DCA4u); goto out; }
    if (S8(o + 373) != 0) {
        call2(c, o, 0x59, f_0886B100, 0x0895DCBCu);
        if (c->r[2] != 0) call1(c, o, f_0895DD9C, 0x0895DCCCu);
        else call1(c, o, f_08861848, 0x0895DCDCu);
        goto out;
    }
    if (!(*H(o + 760) & 0x380)) {
        call2(c, o, 0xE, f_08867C30, 0x0895DCFCu);
        if (c->r[2] == 0) { call1(c, o, f_08861848, 0x0895DD0Cu); goto out; }
    }
    if (*H(o + 752) & 3) {
        call2(c, o, 0x59, f_0886B100, 0x0895DD2Cu);
        if (c->r[2] != 0) { call1(c, o, f_0895DD9C, 0x0895DD3Cu); goto out; }
    }
    call1(c, o, f_0895DD5C, 0x0895DD4Cu);
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
