/* Steps of 84 - 85 instructions: a pickup that its owner catches while standing still (0x0898847C,
   which swaps three of the owner's animation sets), a part projectile's start beside its owner
   (0x089CC02C) and a projectile pair by strength (0x088F9CAC). Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08821424(Cpu *c); void f_088219B0(Cpu *c); void f_08862D88(Cpu *c);
void f_0886E638(Cpu *c); void f_0887C408(Cpu *c); void f_089885CC(Cpu *c); void f_088D3D94(Cpu *c);
void f_088D3E9C(Cpu *c); void f_08822D2C(Cpu *c); void f_088F84BC(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }

/* The owner's animation set `slot` (+416 table + `off`) swapped to entry `n` (0x0886E638). */
static void swap_set(Cpu *c, gaddr own, uint32_t off, uint32_t n, uint32_t back) {
    gaddr t = *W(own + 416);
    ARG(0) = t + off; ARG(1) = *W(t + 196); ARG(2) = *W(t + 192); c->r[7] = n;
    c->r[31] = back; f_0886E638(c);
}

/* (pickup): when its owner (+88) stands still in reach (24 either side), not crouched or in the air, in
   state 1 / 0 with sub-state 0 or 2: next state, the owner's +0x33D = 1, its animation sets 0x9D4 /
   0x9D8 / 0x9E4 swapped to 6 / 7 / 10, 0x08862D88, +292 = 0x08A93396, voice 0x226 and 0x089885CC.
   Then 0x088219B0 and drawn. */
SFA_FUNC(0898847C, pickup_catch) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr own = *W(o + 88);
    c->r[17] = own;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 24) = ra;
    uint32_t b1 = 0x0898858Cu, b2 = 0x08988594u;
    if ((uint32_t)(S16(own + 18) - S16(o + 18) + 0x18) < 0x31u && S8(own + 696) == 0 && S8(own + 73) == 0
        && ((*B(own + 4) << 8) | *B(own + 5)) == 0x100) {
        uint32_t sub = *B(own + 6);
        if (sub == 0 || sub == 2) {
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            *B(own + 0x33D) = 1;
            swap_set(c, own, 0x9D4, 6, 0x08988528u);
            swap_set(c, own, 0x9D8, 7, 0x08988540u);
            swap_set(c, own, 0x9E4, 0xA, 0x08988558u);
            call1(c, own, f_08862D88, 0x08988560u);
            *W(own + 292) = 0x08A93396u;
            ARG(0) = own; ARG(1) = (uint32_t)(int32_t)S8(own + 337); ARG(2) = 0x226;
            c->r[31] = 0x0898857Cu; f_0887C408(c);
            call1(c, o, f_089885CC, 0x08988584u);
        } else { b1 = 0x089885B0u; b2 = 0x089885B8u; }
    }
    call1(c, o, f_088219B0, b1);
    call1(c, o, f_08821424, b2);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (part projectile): its next state, +123 cleared, +255 = 5, its tables (+356 0x08AB3024, +128
   0x08AB3C4C, +200 0x08AB3AC8), 0x088D3E9C (0), still, at its owner's place 88 / 168 / 248 ahead by
   strength, the picture 0x08AB2FDC from 0xE, +140 = 75, +141 cleared, the owner's palette and facing,
   +1 / +170 / +171 = 1; drawn. */
SFA_FUNC(089CC02C, part_shot_start_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr own = *W(o + 88);
    c->r[17] = own;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 123) = 0; *B(o + 255) = 5;
    *W(o + 356) = 0x08AB3024u; *W(o + 128) = 0x08AB3C4Cu; *W(o + 200) = 0x08AB3AC8u;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 0; c->r[31] = 0x089CC08Cu; f_088D3E9C(c);
    *W(o + 96) = 0; *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 108) = 0;
    *W(o + 16) = *W(own + 16); *W(o + 20) = *W(own + 20);
    int32_t s = S8(o + 194);
    int32_t off = s == 0 ? 0x58 : s == 1 ? 0xA8 : 0xF8;
    if (*B(own + 11) == 0) off = (int16_t)(-off);
    *H(o + 18) = (uint16_t)(S16(o + 18) + off);
    ARG(0) = o; ARG(1) = 0x08AB2FDCu; ARG(2) = 0xE; c->r[31] = 0x089CC128u; f_088D3D94(c);
    *B(o + 140) = 0x4B; *B(o + 141) = 0;
    *B(o + 12) = *B(own + 12); *B(o + 13) = *B(own + 13); *B(o + 11) = *B(own + 11);
    *B(o + 1) = 1; *B(o + 170) = 1; *B(o + 171) = 1;
    call1(c, o, f_08821424, 0x089CC168u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): ticked; a frame flag sets step 3 and +82 = 6 and spawns projectile 2 / 2 (set up by
   0x088F84BC; +346 by 6, +255 = 1, +112 = strength, +260 from 0x08A497F4, +311 = 2, +312 = 8); with
   +251 and the meter (+286) at +662 or more a second one, its +280 = +662. */
SFA_FUNC(088F9CAC, move_flag_projectiles_22) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[6];
    c->r[29] -= 0x30;
    for (int i = 0; i < 6; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 40) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x088F9CD4u);
    if (S8(o + 77) != 0) {
        *B(o + 7) = 3; *H(o + 82) = 6;
        gaddr tab = 0x08A497F4u;
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1 && !(S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662)))) break;
            ARG(0) = o; ARG(1) = 2; ARG(2) = 2; c->r[31] = pass ? 0x088F9D88u : 0x088F9CFCu; f_08822D2C(c);
            gaddr e = c->r[2];
            c->r[21] = e;
            if (e == 0) break;
            ARG(0) = o; ARG(1) = e; c->r[31] = pass ? 0x088F9D9Cu : 0x088F9D10u; f_088F84BC(c);
            int32_t k = S8(e + 346);
            if (pass == 0) { c->r[20] = 1; c->r[19] = tab; c->r[18] = 2; c->r[17] = 8; }
            *B(e + 346) = (uint8_t)(k + 6); *B(e + 255) = 1;
            *H(e + 112) = (uint16_t)S8(o + 194);
            if (pass) *B(e + 280) = (uint8_t)S16(o + 662);
            int32_t v = S8(tab + (uint32_t)S8(o + 194));
            *B(e + 311) = 2; *B(e + 260) = (uint8_t)v; *B(e + 312) = 8;
        }
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}
