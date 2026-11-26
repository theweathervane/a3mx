/* Small functions the scenarios reach, 46 - 47 instructions: an effect's two-stage end, a list-0x43
   effect scattered round its owner, a gauge-like object following its owner's level, a blinking
   countdown, a list-7 helper, a mode's frame, a fighter object's start. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08821424(Cpu *c); void f_088219B0(Cpu *c); void f_0882196C(Cpu *c); void f_08822EAC(Cpu *c);
void f_08821368(Cpu *c); void f_088D3B64(Cpu *c); void f_088D3B88(Cpu *c); void f_088215C4(Cpu *c);
void f_088585B0(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);
void f_088B3040(Cpu *c); void f_088B30A8(Cpu *c); void f_088B3110(Cpu *c); void f_088B3698(Cpu *c);
void f_08887AFC(Cpu *c); void f_08837690(Cpu *c); void f_0880B6CC(Cpu *c); void f_088B3DCC(Cpu *c);
void f_0881EE84(Cpu *c); void f_0881A040(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): when +75 goes negative: on its first time (+7 = 1) with +73 set 0x10 lower and the
   animation 0x08A9B2B0's +24 word, else (and the second time) on (+4); until then 0x088219B0 and drawn. */
SFA_FUNC(089A2434, effect_two_stage_end_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t st = *B(o + 7);
    int neg = S8(o + 75) < 0;
    if (!neg) {
        call_on(c, o, st != 0 ? 0x089A24D4u : 0x089A24A4u, f_088219B0);
        call_on(c, o, st != 0 ? 0x089A24DCu : 0x089A24ACu, f_08821424);
    } else if (st != 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *B(o + 7) = 1;
        if (S8(o + 73) != 0) {
            *H(o + 22) = (uint16_t)(S16(o + 22) - 0x10);
            call2(c, o, *W(0x08A9B2B0u + 24), 0x089A2494u, f_0882196C);
        } else *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): a list-0x43 effect (0x08822EAC) at its place, side and pointing back at it (+88), moved by
   two random entries (0x08821368, & 15) of the offsets at 0x08A9C19C, 0x48 higher. */
SFA_FUNC(089A2620, effect_scattered_43) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    call2(c, 0x43, 2, 0x089A2648u, f_08822EAC);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n != 0) {
        *B(n + 11) = *B(o + 11);
        *H(n + 18) = (uint16_t)S16(o + 18);
        *W(n + 88) = o;
        *H(n + 22) = (uint16_t)S16(o + 22);
        c->r[18] = (uint32_t)S16(o + 18);
        call_on(c, (uint32_t)S16(o + 22), 0x089A2678u, f_08821368);
        c->r[19] = 0x08A9C19Cu;
        uint32_t x = c->r[18] + (uint32_t)S16(0x08A9C19Cu + 2 * (c->r[2] & 15));
        *H(n + 18) = (uint16_t)x;
        c->r[16] = (uint32_t)S16(o + 22);
        call_on(c, x, 0x089A26A0u, f_08821368);
        *H(n + 22) = (uint16_t)(c->r[16] + (uint32_t)S16(0x08A9C19Cu + 2 * (c->r[2] & 15)) + 0x48);
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): follows its owner's (+88) level (+296's +10): with none, off (+1, +78 cleared, the owner's
   +92 +56 cleared); with one, on (+1); a new level is taken (+78) and set up (0x088D3B64 with
   0x08AA4010), +170 = it under 7; an unchanged one 0x088D3B88 unless the owner's +141. */
SFA_FUNC(089B7B14, object_follow_level) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 20) = ra;
    uint32_t lv = *B(*W(p + 296) + 10);
    if (lv == 0) {
        *B(o + 1) = 0; *B(o + 78) = 0;
        *W(*W(p + 92) + 56) = 0;
    } else {
        *B(o + 1) = 1;
        lv = *B(*W(p + 296) + 10);
        if ((uint32_t)S8(o + 78) != lv) {
            *B(o + 78) = (uint8_t)lv;
            c->r[7] = 0x08AA4010u;
            ARG(0) = o; ARG(1) = 0x08AA4010u; ARG(2) = *B(*W(p + 296) + 10);
            c->r[31] = 0x089B7B8Cu; f_088D3B64(c);
            *B(o + 170) = S8(o + 78) < 7 ? 1 : 0;
        } else if (S8(p + 141) == 0) call_on(c, o, 0x089B7B6Cu, f_088D3B88);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object, fighter): every 4th frame (+140 countdown) +119 / +127 flip (^ 3); with the round over
   (BATTLE +7) on to step 3 with both 0x15; else unless the fighter's +769, both 0x15,
   0x088585B0, on (+7) and +139 = 0x3C; then 0x088215C4. */
SFA_FUNC(0885835C, object_blink_countdown) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    int8_t t = (int8_t)(S8(o + 140) - 1);
    *B(o + 140) = (uint8_t)t;
    *W(c->r[29] + 20) = ra;
    if (t < 0) {
        *B(o + 140) = 3;
        *B(o + 119) = (uint8_t)(*B(o + 119) ^ 3);
        *B(o + 127) = (uint8_t)(S8(o + 127) ^ 3);
    }
    if (S8(BATTLE + 7) != 0) {
        *B(o + 7) = 3;
        *B(o + 127) = 0x15; *B(o + 119) = 0x15;
    } else if (S8(f + 769) == 0) {
        *B(o + 127) = 0x15; *B(o + 119) = 0x15;
        call_on(c, o, 0x088583D8u, f_088585B0);
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 139) = 0x3C;
    }
    call_on(c, o, 0x08858408u, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, a, b): a list-7 helper (0x08822EAC) with its +13 / +14 / +24 / side, +9 = 1, +86 = 5, a / b
   at +112 / +114, pointing back at it (+88); 1 with one made, else 0. */
SFA_FUNC(0887FC70, helper_make_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[16] = o;
    c->r[18] = (uint32_t)(int32_t)(int16_t)ARG(1);
    c->r[17] = (uint32_t)(int32_t)(int16_t)ARG(2);
    *W(sp + 28) = ra;
    call2(c, 7, 0, 0x0887FCA4u, f_08822EAC);
    gaddr n = c->r[2];
    ARG(0) = n;
    if (n != 0) {
        *B(n + 9) = 1; *B(n + 12) = 0; *B(n + 73) = 0;
        *B(n + 13) = *B(o + 13);
        *B(n + 78) = 0;
        *B(n + 14) = *B(o + 14);
        *H(n + 86) = 5;
        *H(n + 24) = *H(o + 24);
        *W(n + 88) = o;
        *H(n + 112) = (uint16_t)c->r[18];
        *H(n + 114) = (uint16_t)c->r[17];
        *B(n + 11) = *B(o + 11);
        c->r[2] = 1;
    } else { ARG(1) = 1; c->r[2] = 0; }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* A mode's frame: GAME +14 counted, its own (0x088B30A8, with 0x09525DE3 set) or other (0x088B3040)
   step, then while 0x08B0284C is not negative its 0x088B3698 / 0x088B3110 with it; then the
   frame's common work (0x08887AFC, 0x08837690, 0x0880B6CC, 0x088B3DCC with it, 0x0881EE84,
   0x0881A040). */
SFA_FUNC(088B2F2C, mode_frame_284c) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *H(GAME + 14) = (uint16_t)(S16(GAME + 14) + 1);
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0x08B00000u;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    gaddr m = 0x08B0284Cu;
    c->r[16] = m;
    int32_t sel = S8(0x09525DE3u);
    ARG(0) = (uint32_t)sel;
    if (sel != 0) {
        c->r[31] = 0x088B2F90u; f_088B30A8(c);
        if (S16(m) >= 0) call_on(c, m, 0x088B2FA4u, f_088B3698);
    } else {
        c->r[31] = 0x088B2F6Cu; f_088B3040(c);
        if (S16(m) >= 0) call_on(c, m, 0x088B2F80u, f_088B3110);
    }
    c->r[31] = 0x088B2FACu; f_08887AFC(c);
    c->r[31] = 0x088B2FB4u; f_08837690(c);
    c->r[31] = 0x088B2FBCu; f_0880B6CC(c);
    call_on(c, m, 0x088B2FC4u, f_088B3DCC);
    c->r[31] = 0x088B2FCCu; f_0881EE84(c);
    c->r[31] = 0x088B2FD4u; f_0881A040(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with few effects (none, or under two with +251) and 0x0886BF84 allowing it, on (+4 = 1, +6 =
   7, +235 = 1, +236 = 0xA; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(088EF01C, object_start_few_a) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int ok = 0;
    if (S8(o + 251) != 0) {
        if ((int32_t)n < 2) { c->r[31] = 0x088EF0B8u; f_0886BF84(c); ok = c->r[2] != 0; }
    } else if (n == 0) { c->r[31] = 0x088EF04Cu; f_0886BF84(c); ok = c->r[2] != 0; }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 0xA;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[17] = 1;
        *B(o + 235) = 1;
        call_on(c, o, 0x088EF08Cu, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
