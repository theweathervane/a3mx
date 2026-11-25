/* Small functions the scenarios reach, 8 - 29 instructions: effect makers and followers, fighter
   objects' starts, a slide. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089CC718(Cpu *c); void f_0898B0F0(Cpu *c); void f_089CB1E8(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_08821424(Cpu *c); void f_0887684C(Cpu *c);
void f_088768F0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (owner): +820 / +824 cleared and its three effects made (0x089CC718). */
SFA_FUNC(089C5DD8, effects_reset_make) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(o + 820) = 0;
    *W(c->r[29] + 16) = ra;
    *W(o + 824) = 0;
    c->r[31] = 0x089C5DECu; f_089CC718(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): its slowing slide (0x0898B0F0) while +836 is set. */
SFA_FUNC(0898B0C8, object_slide_if) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*W(o + 836) != 0) { c->r[31] = 0x0898B0E4u; f_0898B0F0(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, kind): a list-0x35 effect of kind 0 or 1 (0x089CB1E8); other kinds none. */
SFA_FUNC(089CAC7C, effect_make_35_kind) {
    gaddr o = ARG(0);
    uint32_t k = ARG(1) & 0xFF;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (k == 0) call2(c, o, 0, 0x089CACA0u, f_089CB1E8);
    else if (k == 1) call2(c, o, 1, 0x089CACBCu, f_089CB1E8);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = `k236`; +5 / +7 cleared,
   +11 (and with `also78` +78) = +697) and `then`; 1 then, else 0. */
static void start_235(Cpu *c, uint32_t k236, int also78, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = (uint8_t)k236;
        *B(o + 235) = 1;
        uint8_t v = (uint8_t)S8(o + 697);
        *B(o + 11) = v;
        c->r[17] = 1;
        if (also78) *B(o + 78) = v;
        call_on(c, o, back2, then);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088F7DC4, object_start_235_78) { start_235(c, 0, 1, f_0886DB94, 0x088F7DE0u, 0x088F7E20u); }
SFA_FUNC(08909924, object_start_235_2)  { start_235(c, 2, 0, f_0886DB94, 0x08909940u, 0x08909980u); }
SFA_FUNC(08909B68, object_start_235_5)  { start_235(c, 5, 0, f_0886DBE8, 0x08909B84u, 0x08909BC4u); }

/* (effect): first frame its owner's (+88) +228 let go and 0x0887684C (3); then on when +75 goes
   negative, 0x088768F0; drawn. */
SFA_FUNC(08904DFC, effect_play_once_owner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(*W(o + 88) + 228) = 0;
        call2(c, o, 3, 0x08904E30u, f_0887684C);
    } else {
        if (S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        call_on(c, o, 0x08904E58u, f_088768F0);
    }
    call_on(c, o, 0x08904E60u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): placed at its owner's (+88, or with +126 the owner's +608) x / y plus row +83 of
   0x08A645A8 (x turned with +11). */
SFA_FUNC(0891CD58, effect_follow_row) {
    gaddr o = ARG(0);
    gaddr p = *W(o + 88);
    if (S8(o + 126) != 0) p = *W(p + 608);
    gaddr row = 0x08A645A8u + ((uint32_t)*B(o + 83) << 2);
    int32_t dx = S16(row);
    if (*B(o + 11) == 0) *H(o + 18) = (uint16_t)(S16(p + 18) - dx);
    else *H(o + 18) = (uint16_t)(S16(p + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(p + 22) + S16(row + 2));
}
