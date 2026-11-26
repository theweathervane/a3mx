/* Steps of 65 - 69 instructions: a part character's dash by flag and strength, the round-start banner
   that follows the round number (0x08880F70), the dizzy check that ends in a get-up (0x0885E3D8), a
   leap aimed by distance (0x0896E9DC), projectile spawns with an extra one at full meter, the shared
   combo-count voice (0x088801F8, 19,106 calls) and the left / right / confirm / back menu pad
   (0x0888BBB4). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D3140(Cpu *c);
void f_0887C4CC(Cpu *c); void f_0882196C(Cpu *c); void f_088215C4(Cpu *c); void f_088632C4(Cpu *c);
void f_08863D70(Cpu *c); void f_0887C408(Cpu *c); void f_0886E3CC(Cpu *c); void f_08822D2C(Cpu *c);
void f_08963B64(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void sound(Cpu *c, uint32_t n, uint32_t back) { ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }

/* (character): the part ticked; frame flag +76 (cleared) sets x 6.0 (flag 1) or 26.0 and runs
   0x088D3140; without +77 +7 steps on, +73 = 1 and a hop by strength (x 5.0 / 7.0 / 9.0, y 2.0, fall
   -0.1875 / -0.156 / -0.141), x toward the facing. */
SFA_FUNC(089C14D0, part_flag_dash_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C14E4u);
    int32_t k = S8(o + 76);
    if (k != 0) {
        *W(o + 96) = k == 1 ? 0x60000u : 0x1A0000u;
        *B(o + 76) = 0;
        call1(c, o, f_088D3140, 0x089C1514u);
    }
    if (S8(o + 77) == 0) {
        bump(o + 7);
        *B(o + 73) = 1;
        int32_t s = S8(o + 194);
        *W(o + 104) = 0;
        if (s == 0) { *W(o + 96) = 0x50000; *W(o + 100) = 0x20000; *W(o + 108) = 0xFFFFD000u; }
        else if (s == 1) { *W(o + 96) = 0x70000; *W(o + 100) = 0x20000; *W(o + 108) = 0xFFFFD800u; }
        else if (s == 2) { *W(o + 96) = 0x90000; *W(o + 100) = 0x20000; *W(o + 108) = 0xFFFFDC00u; }
        if (*B(o + 11) == 0) *W(o + 96) = 0u - *W(o + 96);
    }
    leave(c, sv);
}

/* (banner): unless the game is idle (+69 negative or +42 clear, then its next state), it follows the
   round number (the match's fighter +348): a new one restarts it (+5 / +78, +82 = 16, voice 0x126 +
   round, its picture from 0x08A299FC +36), then the +82 slide runs (heights from 0x08A24AD4) and it is
   drawn. */
SFA_FUNC(08880F70, round_banner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr f = *W(MATCH + 92);
    c->r[17] = f;
    *W(c->r[29] + 24) = ra;
    if (S8(GAME + 69) < 0 || S8(GAME + 42) == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        if (*H(f + 348) != (uint32_t)S8(o + 78)) {
            *B(o + 5) = 0;
            *B(o + 78) = (uint8_t)*H(f + 348);
            *H(o + 82) = 0x10;
            sound(c, *H(f + 348) + 0x126, 0x08880FF4u);
            uint32_t pic = *W(0x08A299FCu + 36) + ((uint32_t)*H(f + 348) << 3);
            call2(c, o, pic, f_0882196C, 0x08881014u);
        }
        if (*B(o + 5) == 0) {
            uint32_t t = (uint32_t)S16(o + 82) - 1;
            *H(o + 82) = (uint16_t)t;
            if ((int16_t)t == 0) *B(o + 5) = 1;
            *H(o + 22) = (uint16_t)S16(0x08A24AD4u + (uint32_t)(S16(o + 82) * 2));
        }
        call1(c, o, f_088215C4, 0x08881064u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): dizzy: it gets up (state 1 / 1 / 2 / 0, +634 = 20, hit marks / +82 cleared, cue 0x2E and
   0x0886E3CC (0, 1)) once the match is over (+13), 0x088632C4 says so, or the +82 wait (shortened by
   the mashing from 0x08863D70, which ticks it once or twice and sets +669 / +670) runs out; else
   ticked. */
SFA_FUNC(0885E3D8, fighter_dizzy) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    int32_t over = S8(MATCH + 13);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int up = over != 0;
    if (!up) { call1(c, o, f_088632C4, 0x0885E404u); up = c->r[2] != 0; }
    if (!up) {
        call1(c, o, f_08863D70, 0x0885E414u);
        uint32_t n = c->r[2];
        if ((int32_t)((uint32_t)S16(o + 82) - n) < 0) up = 1;
        else {
            c->r[17] = 1;
            if (n != 0) {
                if (n != 1) call1(c, o, f_08861848, 0x0885E440u);
                call1(c, o, f_08861848, 0x0885E448u);
                *B(o + 670) = 8; *B(o + 669) = 1;
            }
            uint32_t t = (uint32_t)S16(o + 82) - 1;
            *H(o + 82) = (uint16_t)t;
            if ((int16_t)t == 0) up = 1;
            else call1(c, o, f_08861848, 0x0885E478u);
        }
    }
    if (up) {
        *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0;
        *B(o + 634) = 0x14; *B(o + 698) = 0; *B(o + 541) = 0;
        int32_t side = S8(o + 337);
        *B(o + 539) = 0; *H(o + 82) = 0;
        ARG(0) = o; ARG(1) = (uint32_t)side; ARG(2) = 0x2E; c->r[31] = 0x0885E4C0u; f_0887C408(c);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 1; c->r[31] = 0x0885E4D0u; f_0886E3CC(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ended: a leap 48 up toward the opponent: +7 on, +82 = strength + 2, facing it, y and x
   speeds 3/64 of the height to 16 above the floor and of the x distance, no brake or fall, +123
   cleared, +281 = -1, and move 0x26 + strength (+3 from 33 px of distance, +6 from 177); else ticked. */
SFA_FUNC(0896E9DC, move_leap_by_distance) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    int32_t e = S8(o + 75);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = ra;
    if (e < 0) {
        int32_t y = S16(o + 22);
        uint32_t st = *B(o + 7);
        int32_t floor = S16(o + 132);
        *H(o + 22) = (uint16_t)(y + 0x30);
        *W(sp + 16) = 0;
        int32_t s = S8(o + 194);
        *H(sp + 18) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        uint32_t tmp = *W(sp + 16);
        *H(o + 82) = (uint16_t)(s + 2);
        int32_t face = S8(o + 697);
        tmp += 0x100000u;
        uint32_t y20 = *W(o + 20);
        *B(o + 11) = (uint8_t)face;
        uint32_t d = tmp - y20;
        int32_t q = (int32_t)d >> 6;
        *W(sp + 16) = tmp;
        *W(sp + 16) = d;
        *W(o + 100) = (uint32_t)(q + q + q);
        *W(sp + 16) = (uint32_t)q;
        uint32_t dx = *W(*W(o + 92) + 16) - *W(o + 16);
        if ((int32_t)dx < 0) dx = 0u - dx;
        *W(sp + 16) = dx;
        int32_t qx = (int32_t)dx >> 6;
        *W(o + 96) = (uint32_t)(qx + qx + qx);
        *W(o + 108) = 0; *W(o + 104) = 0; *B(o + 123) = 0;
        int32_t px = S16(sp + 18);
        *B(o + 281) = 0xFF;
        uint32_t add = px < 33 ? 0 : px < 177 ? 3 : 6;
        call2(c, o, (add + (uint32_t)s + 0x26) & 0xFFFF, f_088616EC, 0x0896EAC8u);
    } else call1(c, o, f_08861848, 0x0896EADCu);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; a frame flag steps +7 on, +82 = 3 and spawns projectile 0 (set up by 0x08963B64;
   in stance 0xA its +346 by 11), kept at +228 and counted (+536); with +251 and the meter (+286) at
   +662 or more a second one, its +280 = +662. */
SFA_FUNC(08963A54, move_flag_projectiles_0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    for (int i = 0; i < 3; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x20;
    for (int i = 0; i < 3; i++) *W(c->r[29] + 16 + 4 * i) = s[i];
    *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x08963A70u);
    if (S8(o + 77) != 0) {
        bump(o + 7);
        *H(o + 82) = 3;
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1 && !(S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662)))) break;
            ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = pass ? 0x08963B08u : 0x08963AA0u; f_08822D2C(c);
            gaddr e = c->r[2];
            c->r[18] = e;
            if (e == 0) break;
            ARG(0) = o; ARG(1) = e; c->r[31] = pass ? 0x08963B1Cu : 0x08963AB4u; f_08963B64(c);
            if (pass) *B(e + 280) = (uint8_t)S16(o + 662);
            else c->r[17] = 0xA;
            if (S8(o + 236) == 0xA) *B(e + 346) = (uint8_t)(S8(e + 346) + 0xB);
            uint32_t n = *B(o + 536);
            *W(o + 228) = e;
            *B(o + 536) = (uint8_t)(n + 1);
        }
    }
    for (int i = 0; i < 3; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (hit spark): 1 when it does not count (+3 from 3, +78 set, or +139 already set / pending past state
   1); 0 while its owner's +237 is set (a pending +139 cleared then); else the combo voice 0x3E from the
   owner's side with the shared counter (0x092D9EEC, wrapping at 0x3F) in 0x092D9EED, +139 = 1 and 1. */
SFA_FUNC(088801F8, hit_combo_voice) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[3];
    for (int i = 0; i < 3; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s[0]; c->r[16] = o;
    uint32_t lvl = *B(o + 3);
    *W(c->r[29] + 20) = s[1]; *W(c->r[29] + 24) = s[2]; *W(c->r[29] + 28) = ra;
    gaddr own = *W(o + 88);
    uint32_t ret = 1;
    if (lvl < 3 && S8(o + 78) == 0) {
        int32_t k = S8(o + 139);
        if (k < 0 && *B(o + 4) < 2) {
            if (S8(own + 237) != 0) *B(o + 139) = 0;
            ret = 0;
        } else if (k == 0) {
            if (S8(own + 237) != 0) ret = 0;
            else {
                gaddr cnt = 0x092D9EECu;
                *B(cnt) = (uint8_t)(*B(cnt) - 1);
                if (*B(cnt) & 0x80) *B(cnt) = 0x3F;
                uint32_t v = *B(cnt);
                c->r[17] = 0x092E0000u;
                uint32_t old = *B(cnt + 1);
                c->r[18] = old;
                *B(cnt + 1) = (uint8_t)v;
                ARG(0) = own; ARG(1) = (uint32_t)(int32_t)S8(own + 337); ARG(2) = 0x3E;
                c->r[31] = 0x088802E4u; f_0887C408(c);
                *B(cnt + 1) = (uint8_t)old;
                *B(o + 139) = 1;
            }
        }
    }
    c->r[2] = ret;
    for (int i = 0; i < 3; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu): the pad (0x08E75BA0 +12 | +56): right / left set the cursor (0x09523C60 +9) to 1 / -1 with
   sound 0x38 when it changes; confirm (0x100) gives -1 with sound 0x2F, back (0x8080) the cursor with
   sound 0x39 (both clearing it); else 0. */
SFA_FUNC(0888BBB4, menu_pad_lr) {
    uint32_t ra = c->r[31], s[4];
    for (int i = 0; i < 4; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr pad = 0x08E75BA0u;
    *W(c->r[29] + 24) = s[2];
    uint32_t b = *W(pad + 12) | *W(pad + 56);
    c->r[18] = b;
    *W(c->r[29] + 16) = s[0]; *W(c->r[29] + 20) = s[1]; *W(c->r[29] + 28) = s[3]; *W(c->r[29] + 32) = ra;
    c->r[17] = 0;
    gaddr m = 0x09523C60u;
    if (b & 8) {
        c->r[16] = m; c->r[19] = 1;
        if (S8(m + 9) != 1) { sound(c, 0x38, 0x0888BC14u); *B(m + 9) = 1; }
    } else if (b & 4) {
        c->r[16] = m; c->r[19] = 0xFFFFFFFFu;
        if (S8(m + 9) != -1) { sound(c, 0x38, 0x0888BC50u); *B(m + 9) = 0xFF; }
    }
    uint32_t ret = 0;
    if (b & 0x100) { sound(c, 0x2F, 0x0888BC70u); *B(m + 9) = 0; ret = 0xFFFFFFFFu; }
    else if (b & 0x8080) { sound(c, 0x39, 0x0888BC98u); ret = (uint32_t)S8(m + 9); *B(m + 9) = 0; }
    c->r[17] = ret;
    c->r[2] = ret;
    for (int i = 0; i < 4; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
