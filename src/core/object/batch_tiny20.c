/* Small functions the scenarios reach, 27 - 28 instructions: effects copying their owner's place and
   looks, an effect's step or removal, objects' starts, a homing step. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0882324C(Cpu *c); void f_08822F54(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);
void f_0886DBE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): gone (0x0882324C) once its +139 no longer matches its owner's (+88); else, unless the
   fight is halted (BATTLE +37), its step by kind +3 (0x08AB1778). */
SFA_FUNC(089C3BEC, effect_step_owner_139) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 139) != S8(*W(o + 88) + 139)) { c->r[31] = 0x089C3C10u; f_0882324C(c); }
    else if (S8(BATTLE + 37) == 0) {
        gaddr h = *W(0x08AB1778u + ((uint32_t)*B(o + 3) << 2));
        ARG(1) = h;
        c->r[31] = 0x089C3C4Cu; rt_call_indirect(c, h);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): a list-0x37 effect (0x08822F54) for the owner (+88, +139 and +14 its, +0 = +3 = 1),
   chained at the owner's +48 (or the first one's +48). */
SFA_FUNC(089CC7A4, effect_make_37) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    call2(c, 0x37, 0, 0x089CC7C0u, f_08822F54);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = o;
        *B(n + 139) = (uint8_t)S8(o + 139);
        *B(n) = 1; *B(n + 3) = 1;
        gaddr first = *W(o + 48);
        if (first == 0) *W(o + 48) = n;
        else *W(first + 48) = n;
        c->r[16] = *B(o + 14);
        *B(n + 14) = (uint8_t)c->r[16];
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1; +5 / +7 / +236 cleared, +11 =
   +697) and `then`; 1 then, else 0. */
static void start_235(Cpu *c, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0;
        *B(o + 235) = 1;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, back2, then);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088F2FF0, object_start_235_db94) { start_235(c, f_0886DB94, 0x088F300Cu, 0x088F3048u); }
SFA_FUNC(08901B0C, object_start_235_dbe8) { start_235(c, f_0886DBE8, 0x08901B28u, 0x08901B64u); }

/* (effect, owner, n): placed at the owner's x / y plus row n of 0x08A6463E (x turned with the owner's
   direction +11, which it takes) and the owner's +268. */
SFA_FUNC(0891D610, effect_place_by_row) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t n = ARG(2);
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    gaddr e = 0x08A6463Eu + (n << 2);
    int32_t dx = S16(e), dy = S16(e + 2);
    *B(o + 11) = *B(p + 11);
    if (*B(o + 11) == 0) dx = (int16_t)-dx;
    *H(o + 18) = (uint16_t)(dx + S16(p + 268) + S16(o + 18));
    *H(o + 22) = (uint16_t)(S16(o + 22) + dy);
}

/* (owner, effect): the effect takes the owner's +121 / +122 / +80 / +11 - +14 / +38 / x / y and +194
   (as its +346), points back at it (+88); the owner's +228 = it and +536 on. */
SFA_FUNC(089247DC, effect_take_owner) {
    gaddr a = ARG(0), b = ARG(1);
    *B(b + 122) = (uint8_t)S8(a + 122);
    *B(b + 121) = (uint8_t)S8(a + 121);
    *B(b + 346) = (uint8_t)S8(a + 194);
    *B(b + 80) = (uint8_t)S8(a + 80);
    *B(b + 14) = *B(a + 14);
    *B(b + 11) = *B(a + 11);
    *B(b + 12) = (uint8_t)S8(a + 12);
    *B(b + 13) = *B(a + 13);
    *H(b + 38) = (uint16_t)S16(a + 38);
    *H(b + 18) = (uint16_t)S16(a + 18);
    *W(b + 88) = a;
    *H(b + 22) = (uint16_t)S16(a + 22);
    uint32_t n = *B(a + 536);
    *W(a + 228) = b;
    *B(a + 536) = (uint8_t)(n + 1);
    ARG(1) = n + 1; ARG(2) = n;
}

/* (effect, owner): the effect takes the owner's +121 / +122 / +80 / +11 - +14 / +38 / x / y / +164 and
   side (+337, as +86), and points back at it (+88). */
SFA_FUNC(0893B604, effect_take_owner_2) {
    gaddr o = ARG(0), p = ARG(1);
    *B(o + 122) = (uint8_t)S8(p + 122);
    *B(o + 121) = (uint8_t)S8(p + 121);
    *B(o + 80) = (uint8_t)S8(p + 80);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *W(o + 16) = *W(p + 16);
    *W(o + 88) = p;
    *W(o + 20) = *W(p + 20);
    uint32_t side = (uint32_t)(int32_t)S8(p + 337);
    *H(o + 86) = (uint16_t)side;
    *W(o + 164) = *W(p + 164);
    ARG(2) = side; ARG(1) = *W(p + 164);
}

/* (object): its x step (+96) to reach its target's x (+0x3C0's high half, plus row +0x3BC of
   0x08A7447C turned with +11) from its x, in 1/16 of the distance a frame (x 0x1000); +16 and the
   target flag (+0x3BC) cleared. */
SFA_FUNC(0893E040, object_aim_x) {
    gaddr o = ARG(0);
    gaddr q = o + 0x3B8;
    int32_t dx = S16(0x08A7447Cu + 2u * (uint32_t)(int32_t)S8(q + 4));
    if (*B(o + 11) != 0) dx = (int16_t)-dx;
    int32_t to = (int16_t)(dx + (int32_t)(*W(q + 8) >> 16));
    int32_t d = (int16_t)(to - S16(o + 18));
    *W(o + 96) = ((uint32_t)d << 12);
    *H(o + 16) = 0;
    *B(q + 4) = 0;
    ARG(1) = ((uint32_t)d << 12); ARG(2) = q;
}
