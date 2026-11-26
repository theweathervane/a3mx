/* Small functions the scenarios reach, 109 - 113 instructions: a three-part effect cycling its parts,
   a fighter's command-input step matching its held buttons. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088215C4(Cpu *c); void f_088231C8(Cpu *c); void f_0891DB50(Cpu *c); void f_0891DBD8(Cpu *c);
void f_0891DD1C(Cpu *c); void f_088681F8(Cpu *c); void f_08868208(Cpu *c); void f_08868214(Cpu *c);
void f_08869FEC(Cpu *c); void f_0886A048(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): step 0 sets up its three 16-byte parts at +0x60 / +0x70 / +0x80 (0x0891DB50 then 0x0891DBD8
   each; +12 = -1, +13 = 2, +9 / +72 = 1, +82 = 3); step 1 takes the next part in turn (+82 round 0 - 2),
   0x0891DD1C, and while its +5 isn't negative moves it by its steps (+8 / +10 into +12 / +14), shows
   at it (+18 / +22, picture +28 from its +0) and draws; a part done ends it (+4 = 2); steps 2 / 3 free
   it (0x088231C8). */
SFA_FUNC(0891D99C, effect_three_parts) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 4);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (st == 0) {
        *B(o + 4) = 1;
        *B(o + 12) = 0xFF; *B(o + 13) = 2; *B(o + 9) = 1; *B(o + 72) = 1;
        *H(o + 82) = 3;
        static const uint32_t back[6] = { 0x0891D9F8u, 0x0891DA04u, 0x0891DA14u, 0x0891DA20u, 0x0891DA30u, 0x0891DA3Cu };
        for (int k = 0; k < 3; k++) {
            gaddr part = o + 0x60 + 0x10 * (uint32_t)k;
            c->r[17] = part;
            call2(c, o, part, back[2 * k], f_0891DB50);
            call2(c, o, part, back[2 * k + 1], f_0891DBD8);
        }
    } else if (st == 1) {
        int16_t h = (int16_t)(S16(o + 82) + 1);
        *H(o + 82) = (uint16_t)h;
        if (!(h < 3)) *H(o + 82) = 0;
        int32_t i = S16(o + 82);
        if (i >= 0 && i <= 2) c->r[17] = o + 0x60 + 0x10 * (uint32_t)i;
        gaddr part = c->r[17];
        call2(c, o, part, 0x0891DAA8u, f_0891DD1C);
        if (S8(part + 5) < 0) *B(o + 4) = 2;
        else {
            *H(part + 12) = (uint16_t)(S16(part + 12) + S16(part + 8));
            *H(part + 14) = (uint16_t)(S16(part + 14) + S16(part + 10));
            *H(o + 18) = (uint16_t)S16(part + 12);
            *H(o + 22) = (uint16_t)S16(part + 14);
            *W(o + 28) = *W(part);
            call_on(c, o, 0x0891DAF4u, f_088215C4);
        }
    } else if (st == 2) call_on(c, o, 0x0891DB24u, f_088231C8);
    else if (st == 3) call_on(c, o, 0x0891DB3Cu, f_088231C8);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, step, table): counts the step down (+4; at 0 it lapses, 0x088681F8); otherwise the table's
   entry for its position (+1) is matched against the buttons held (+760 | +762): 0x1800 always moves on
   (0x08868208); 0x1000 needs one of its buttons (0x3F0) held, with 0x4000 / 0x8000 accepting all three
   punches (0x70) / kicks (0x380) or a press (0x08869FEC / 0x0886A048), to succeed (0x08868214), else it
   moves on; without 0x1000 any of its buttons held succeeds, none moves on. */
SFA_FUNC(08867EA8, fighter_command_step) {
    gaddr f = ARG(0), t = ARG(1), tab = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t cnt = (uint32_t)*B(t + 4) - 1;
    *W(sp + 20) = sv[1];
    c->r[17] = f;
    *B(t + 4) = (uint8_t)cnt;
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    c->r[16] = t;
    if ((cnt & 0xFF) == 0) call_on(c, t, 0x08867F4Cu, f_088681F8);
    else {
        uint32_t e = *H(tab + 2u * *B(t + 1));
        c->r[19] = e;
        uint32_t held = (*H(f + 760) | *H(f + 762)) & 0xFFFF;
        uint32_t dirs = e & 0x3F0;
        c->r[18] = held;
        if ((e & 0x1800) == 0x1800) call_on(c, t, 0x08867F28u, f_08868208);
        else if (e & 0x1000) {
            held &= dirs;
            c->r[18] = held;
            int done = 0;
            if (e & 0x4000) {
                if ((held & 0x70) == 0x70) { call_on(c, t, 0x08867FE0u, f_08868214); done = 1; }
                else {
                    call_on(c, f, 0x08867F8Cu, f_08869FEC);
                    if (c->r[2] != 0) { call_on(c, t, 0x08867FE0u, f_08868214); done = 1; }
                }
            }
            if (!done) {
                int ok = 0;
                if (e & 0x8000) {
                    if ((held & 0x380) == 0x380) ok = 1;
                    else { call_on(c, f, 0x08868004u, f_0886A048); ok = c->r[2] != 0; }
                }
                if (ok) call_on(c, t, 0x08868014u, f_08868214);
                else call_on(c, t, 0x08867FBCu, f_08868208);
            }
        } else {
            if (dirs & held) call_on(c, t, 0x08868040u, f_08868214);
            else call_on(c, t, 0x08868050u, f_08868208);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
