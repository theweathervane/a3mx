/* Steps of 91 - 93 instructions: the throw target search over the fighters in +263 (0x0886A890), a
   hop-and-land sequence with a voice (0x08984774) and a held opponent's puppets with an extra one at
   full meter (0x0893E178). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0886469C(Cpu *c); void f_0886A9FC(Cpu *c);
void f_08837098(Cpu *c); void f_0887C408(Cpu *c); void f_08987440(Cpu *c); void f_0886DB2C(Cpu *c);
void f_08822D2C(Cpu *c); void f_089426E4(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter, kind): unless +80, each fighter in +263's bits (from 0x08A17F88) that is standing and not
   blocking (+543 / +573) is tried when it is in state 1 / 2 or 1 / 0, has +816 set, or is in 1 / 1 hit
   with +116 = 6; the first 0x0886A9FC accepts becomes the opponent (+92) and gives 1, else 0. */
SFA_FUNC(0886A890, throw_find_target) {
    gaddr o = ARG(0);
    uint32_t kind = ARG(1);
    uint32_t ra = c->r[31], s[7];
    c->r[29] -= 0x30;
    for (int i = 0; i < 7; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 44) = ra;
    c->r[16] = o; c->r[17] = kind;
    uint32_t ret = 0;
    if (S8(o + 80) == 0) {
        gaddr tab = 0x08A17F88u;
        c->r[20] = tab; c->r[19] = 1; c->r[22] = 6;
        for (int32_t i = 0; i < 4; i = (int16_t)(i + 1)) {
            c->r[18] = (uint32_t)i;
            if (!(S8(o + 263) & (1 << i))) continue;
            gaddr t = *W(tab + ((uint32_t)i << 2));
            c->r[21] = t;
            if (S8(t + 73) != 0 || (S8(t + 543) | S8(t + 573)) != 0) continue;
            uint32_t a = *B(t + 4), b = *B(t + 5);
            int try_it = (a == 1 && b == 2) || (a == 1 && b == 0) || *W(t + 816) != 0
                         || (a == 1 && b == 1 && *B(t + 116) == 6);
            if (!try_it) continue;
            ARG(0) = o; ARG(1) = t; ARG(2) = kind; c->r[31] = 0x0886A984u; f_0886A9FC(c);
            if (c->r[2] != 0) { *W(o + 92) = t; ret = 1; break; }
        }
    }
    c->r[2] = ret;
    for (int i = 0; i < 7; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): step 0: a hop back (+262 = 1, +235 / +696 cleared, 0x08837098, voice 0x209, x -1.75, y
   8.75 / -0.625) into move 0xC; 1: into the air (+73 = 1) at a frame flag-less frame; 2: 0x08987440
   and landing (+262 / +73 / +20 cleared, at the floor, +74 = 1); 3: standing once ended, else
   0x0886DB2C; ticked through. */
SFA_FUNC(08984774, move_hop_back_c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t st = *B(o + 7);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 1;
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1; *B(o + 235) = 0; *B(o + 696) = 0;
        call1(c, o, f_08837098, 0x089847B0u);
        ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x209; c->r[31] = 0x089847C0u; f_0887C408(c);
        *W(o + 96) = 0xFFFE4000u; *W(o + 100) = 0x8C000; *W(o + 104) = 0; *W(o + 108) = 0xFFFF6000u;
        call2(c, o, 0xC, f_088616EC, 0x089847F4u);
    } else if (st == 1) {
        if (S8(o + 77) == 0) { *B(o + 7) = 2; *B(o + 73) = 1; }
        call1(c, o, f_08861848, 0x08984820u);
    } else if (st == 2) {
        if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08984844u);
        call1(c, o, f_08987440, 0x0898484Cu);
        int32_t floor = S16(o + 132);
        if (!(floor < S16(o + 22))) {
            uint32_t n = *B(o + 7);
            *B(o + 262) = 0; *B(o + 7) = (uint8_t)(n + 1);
            *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0; *H(o + 20) = 0; *B(o + 74) = 1;
            call1(c, o, f_08861848, 0x08984890u);
        }
    } else if (st == 3) {
        if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089848B4u);
        else { call1(c, o, f_0886DB2C, 0x089848C4u); call1(c, o, f_08861848, 0x089848CCu); }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; a frame flag steps +7 on (+83 = 5) and spawns the held opponent's puppet
   (0x08822D2C 0xF, set up by 0x089426E4; with no choice made its +2 / +3 = 0x1C / 0): +255 / +347 /
   +112 cleared, +260 = 0xC, +311 = 2, +312 = 8, kept at +228 and counted; with +251 and the meter at
   +662 or more a second one, its +280 = +662. */
SFA_FUNC(0893E178, move_flag_puppets_f) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[5];
    c->r[29] -= 0x30;
    for (int i = 0; i < 5; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x0893E19Cu);
    if (S8(o + 77) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 0x53) = 5;
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1 && !(S8(o + 251) != 0 && !((S16(o + 286) & 0xFFFF) < (S16(o + 662) & 0xFFFF)))) break;
            ARG(0) = o; ARG(1) = 0xF; ARG(2) = 0; c->r[31] = pass ? 0x0893E268u : 0x0893E1D4u; f_08822D2C(c);
            gaddr e = c->r[2];
            c->r[20] = e;
            if (e == 0) break;
            if (pass == 0) { c->r[19] = 0xC; c->r[18] = 2; c->r[17] = 8; }
            if (S8(o + 386) < 0) { *B(e + 2) = 0x1C; *B(e + 3) = 0; }
            ARG(0) = e; ARG(1) = o; c->r[31] = pass ? 0x0893E298u : 0x0893E20Cu; f_089426E4(c);
            *B(e + 255) = 0; *B(e + 347) = 0; *H(e + 112) = 0;
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            if (pass) {
                uint32_t m = (uint32_t)S16(o + 662);
                *B(o + 536) = (uint8_t)(n + 1);
                *B(e + 280) = (uint8_t)(m & 0xFF);
            } else *B(o + 536) = (uint8_t)(n + 1);
            *B(e + 260) = 0xC; *B(e + 311) = 2; *B(e + 312) = 8;
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
