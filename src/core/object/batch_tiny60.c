/* Small functions the scenarios reach, 56 - 57 instructions: a rising effect waiting on its owner,
   an effect beside its owner during its move, a list-0x32 helper of a fighter, a thrown effect's
   start, bouncing effects and their start. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_089885CC(Cpu *c); void f_088215C4(Cpu *c);
void f_08822F54(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_0893A4E0(Cpu *c);
void f_08971D6C(Cpu *c); void f_08821368(Cpu *c); void f_08971DA8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): step 0 rises by its y step (+100) until 0x80 above its ground (+132), then step 1 with
   the animation 0x08A92D68's second word; step 1 waits for its owner (+88) to be back in step
   1/0/0/0, then on (+4) via 0x089885CC. 0x088219B0 while waiting. */
SFA_FUNC(08988A50, effect_rise_wait_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t st = *B(o + 5);
    *W(c->r[29] + 16) = ra;
    c->r[6] = 1;
    if (st == 0) {
        *W(o + 20) += *W(o + 100);
        if (S16(o + 132) + 0x80 < S16(o + 22)) {
            ARG(1) = 1;
            c->r[31] = 0x08988AB4u; f_088219B0(c);
        } else {
            *B(o + 5) = 1;
            ARG(1) = *W(0x08A92D68u + 4);
            c->r[31] = 0x08988AA0u; f_0882196C(c);
        }
    } else if (st == 1) {
        gaddr p = *W(o + 88);
        uint32_t step = ((uint32_t)*B(p + 4) << 24) | ((uint32_t)*B(p + 5) << 16) | ((uint32_t)*B(p + 6) << 8) | *B(p + 7);
        ARG(1) = step;
        if (step == 0x01000000u) {
            c->r[6] = (uint32_t)*B(o + 4) + 1;
            *B(o + 4) = (uint8_t)c->r[6];
            c->r[31] = 0x08988B10u; f_089885CC(c);
        } else { c->r[31] = 0x08988B24u; f_088219B0(c); }
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): halted it is only drawn; while its owner (+88) is in step 1/0/8 with its x step (+96)
   not negative, 0x50 ahead of it, 0x088219B0 and drawn every other frame (GAME +59 + +66);
   otherwise gone (+4 on). */
SFA_FUNC(089B38F0, effect_ahead_of_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    gaddr p = *W(o + 88);
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) != 0) call_on(c, o, 0x089B39C0u, f_088215C4);
    else if ((int32_t)*W(p + 96) < 0 || *B(p + 4) != 1 || *B(p + 5) != 0 || *B(p + 6) != 8)
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *H(o + 18) = (uint16_t)(S16(p + 18) + (*B(p + 11) != 0 ? -0x50 : 0x50));
        call_on(c, o, 0x089B397Cu, f_088219B0);
        if ((S8(GAME + 59) + S8(GAME + 66)) & 1) call_on(c, o, 0x089B39A4u, f_088215C4);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, kind, b): a list-0x32 helper (0x08822F54) on (+0), its kind (+3) and b (+141), pointing
   back at it (+88) with its +12 - +14 / +24 / +139 / place, +140 one below its; kept as the
   fighter's +40 (kind 3), else +48 or (taken) +52. */
SFA_FUNC(089C3B0C, fighter_helper_32) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[16] = f;
    int32_t kind = (int8_t)ARG(1), b = (int8_t)ARG(2);
    c->r[18] = (uint32_t)kind; c->r[17] = (uint32_t)b;
    *W(sp + 28) = ra;
    call2(c, 0x32, 0, 0x089C3B40u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n) = 1;
        *B(n + 3) = (uint8_t)kind;
        *B(n + 141) = (uint8_t)b;
        *W(n + 88) = f;
        *B(n + 139) = (uint8_t)S8(f + 139);
        *B(n + 14) = *B(f + 14);
        if (kind == 3) *W(f + 40) = n;
        else if (*W(f + 48) != 0) *W(f + 52) = n;
        else *W(f + 48) = n;
        *B(n + 13) = *B(f + 13);
        *B(n + 12) = (uint8_t)S8(f + 12);
        *H(n + 24) = *H(f + 24);
        *B(n + 140) = (uint8_t)(S8(f + 140) - 1);
        *W(n + 16) = *W(f + 16);
        *W(n + 20) = *W(f + 20);
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4), +344 = -1, its owner's (+88) +24, its pictures (0x08A6E404 / 0x08A6E478 at +352
   / +356, 0x08A6E01C / 0x08A6DF84 at +128 / +200), 0x08876998; thrown (x step 0x12000 its way, y
   step 0x100000 falling by 0x18000), +112 = 0xFFF, a pixel lower, 0x0887684C (4) and 0x0893A4E0. */
SFA_FUNC(0893A3FC, effect_thrown_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(*W(o + 88) + 24);
    *W(o + 352) = 0x08A6E404u;
    *W(o + 356) = 0x08A6E478u;
    *W(o + 128) = 0x08A6E01Cu;
    *W(o + 200) = 0x08A6DF84u;
    *W(c->r[29] + 20) = ra;
    c->r[5] = 0x08A70000u;
    call_on(c, o, 0x0893A464u, f_08876998);
    *B(o + 73) = 0; *B(o + 9) = 0;
    *H(o + 112) = 0xFFF;
    *W(o + 100) = 0x100000u;
    *W(o + 108) = 0xFFFE8000u;
    *W(o + 104) = 0;
    *W(o + 96) = *B(o + 11) != 0 ? 0x12000u : 0xFFFEE000u;
    *H(o + 22) = (uint16_t)(S16(o + 22) + 1);
    call2(c, o, 4, 0x0893A4C8u, f_0887684C);
    call_on(c, o, 0x0893A4D0u, f_0893A4E0);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): for +82 frames it moves (0x08971D6C) and, on reaching its ground (+132), bounces back
   up at half speed (+100) until +83 bounces are used (then off, +4 = 2); 0x088219B0 and drawn
   every other frame (GAME +59 + +66). Out of frames, off. */
SFA_FUNC(08970C08, effect_bounce) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr t = o + 0x52;
    c->r[17] = t;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    int8_t left = (int8_t)(S8(t) - 1);
    *B(t) = (uint8_t)left;
    *W(c->r[29] + 24) = ra;
    if (left < 0) *B(o + 4) = 2;
    else {
        call_on(c, o, 0x08970C44u, f_08971D6C);
        uint16_t ground = *H(o + 132);
        if (!(ground < *H(o + 22))) {
            *H(o + 22) = ground;
            int8_t b = (int8_t)(S8(t + 1) - 1);
            *B(t + 1) = (uint8_t)b;
            if (b < 0) *B(o + 4) = 2;
            else *W(o + 100) = (uint32_t)-((int32_t)*W(o + 100) >> 1);
        }
        call_on(c, o, 0x08970CA4u, f_088219B0);
        if ((S8(GAME + 59) + S8(GAME + 66)) & 1) call_on(c, o, 0x08970CCCu, f_088215C4);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): on (+4), 0x08971DA8, +9 = 1, a random 0 - 31 frames (+82) and x offset from its owner
   (+88), on the owner's ground (+132) and 0xE8 above it, 2 bounces (+83), the owner's side and
   +164, and its +139's animation from 0x08A8C90C. */
SFA_FUNC(08971638, effect_bounce_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 20) = sv[1];
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    call_on(c, o, 0x0897166Cu, f_08971DA8);
    *B(o + 9) = 1;
    ARG(0) = 1; c->r[31] = 0x08971678u; f_08821368(c);
    uint32_t r = c->r[2] & 0x1F;
    c->r[18] = o + 0x52;
    *B(o + 0x52) = (uint8_t)r;
    c->r[19] = (uint32_t)S16(p + 18);
    ARG(0) = r; c->r[31] = 0x0897168Cu; f_08821368(c);
    *H(o + 18) = (uint16_t)(c->r[19] + (uint32_t)(int32_t)(int8_t)c->r[2]);
    *H(o + 132) = (uint16_t)S16(p + 132);
    *H(o + 22) = (uint16_t)(S16(o + 132) + 0xE8);
    *B(o + 0x53) = 2;
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(o + 164) = *W(p + 164);
    uint32_t k = (uint32_t)S8(o + 139) & 0xFFFF;
    call2(c, o, *W(0x08A8C90Cu + (k << 2)), 0x08971700u, f_0882196C);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
