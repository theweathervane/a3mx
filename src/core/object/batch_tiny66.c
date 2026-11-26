/* Small functions the scenarios reach, 68 - 70 instructions: an effect riding its owner's move, a
   fighter's three gauge pieces set up, a random-launch object's start, a sparkle effect thrown off
   a fighter, two boxes' overlap. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D3B64(Cpu *c); void f_088D3B88(Cpu *c); void f_0887CAD0(Cpu *c); void f_0887CB3C(Cpu *c);
void f_0887CC00(Cpu *c); void f_08821330(Cpu *c); void f_08821368(Cpu *c); void f_0882196C(Cpu *c);
void f_089133E8(Cpu *c); void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): while its owner (+88) is in step 1/0/7 it takes the owner's place; first (+5 = 0) its
   animation is set (0x088D3B64: 0x08AA75E8, 0x1E) unless the owner's level (+296's +10) is 21 - 26
   (then 0x088D3B88), later 0x088D3B88 until +75 goes negative (then on, +4). The owner gone on,
   so is it. */
SFA_FUNC(089BEAFC, effect_ride_owner_move) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 20) = ra;
    if (*B(p + 4) != 1 || *B(p + 5) != 0 || *B(p + 6) != 7) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    } else {
        *W(o + 16) = *W(p + 16);
        *W(o + 20) = *W(p + 20);
        uint32_t st = *B(o + 5);
        if (st == 0) {
            int32_t lv = (int32_t)*B(*W(p + 296) + 10);
            if (lv >= 21 && lv < 27) call_on(c, o, 0x089BEBB0u, f_088D3B88);
            else {
                *B(o + 5) = (uint8_t)(st + 1);
                ARG(0) = o; ARG(1) = 0x08AA75E8u; ARG(2) = 0x1E;
                c->r[31] = 0x089BEBD0u; f_088D3B64(c);
            }
        } else {
            call_on(c, o, 0x089BEBE0u, f_088D3B88);
            if (S8(o + 75) < 0) {
                *B(o + 5) = 0;
                *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): with 0x0887CB3C, +656 = 4 and its three gauge pieces (0x08B02698 / 94 / 90) given its +14,
   0x0887CAD0's value for its side (+96), +82 = 2 / 4 / 6, 0x0887CC00 (fighter, piece, 0 / 1 / 2),
   +100 = 0x01000800 and +13 moved on by 3 / 4 / 4. */
SFA_FUNC(0887CC68, fighter_gauge_pieces) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = f;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    ARG(0) = (uint32_t)S8(f + 337);
    c->r[31] = 0x0887CC90u; f_0887CAD0(c);
    uint32_t v = c->r[2];
    c->r[17] = v;
    c->r[31] = 0x0887CC98u; f_0887CB3C(c);
    c->r[18] = 4;
    if (c->r[2] != 0) {
        *B(f + 656) = 4;
        static const uint32_t at[3] = { 0x08B02698u, 0x08B02694u, 0x08B02690u };
        static const uint32_t back[3] = { 0x0887CCD0u, 0x0887CD10u, 0x0887CD4Cu };
        static const uint8_t more[3] = { 3, 4, 4 };
        for (uint32_t k = 0; k < 3; k++) {
            gaddr g = *W(at[k]);
            if (k < 2) c->r[19] = g; else c->r[18] = g;
            *B(g + 14) = *B(f + 14);
            *W(g + 96) = v;
            *H(g + 82) = (uint16_t)(2 + 2 * k);
            ARG(0) = f; ARG(1) = g; ARG(2) = k;
            c->r[31] = back[k]; f_0887CC00(c);
            c->r[20] = 0x01000800u;
            *W(g + 100) = 0x01000800u;
            *B(g + 13) = (uint8_t)(*B(g + 13) + more[k]);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object): on (+4, +9 = 2, +73 cleared), its owner's (+88) side (as +11), +12 - +14 / +24 / +38;
   placed at a random spot (0x08821368) round the screen edge (0x08821330's x - 0x30, or + 0x1B0 for
   +11) and 0x48 - 0x87 up, a random launch (0x089133E8) and the animation 0x08A58688's +20 / +24 word
   (+3 set). */
SFA_FUNC(0891324C, object_random_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 2;
    *B(o + 11) = (uint8_t)S8(p + 337);
    *B(o + 14) = *B(p + 14);
    *H(o + 38) = (uint16_t)S16(p + 38);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 73) = 0;
    *B(o + 13) = *B(p + 13);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    *H(o + 24) = *H(p + 24);
    call_on(c, p, 0x089132B8u, f_08821330);
    gaddr q = c->r[2];
    c->r[18] = q;
    c->r[31] = 0x089132C0u; f_08821368(c);
    int16_t dx = (int16_t)((c->r[2] & 0x1F) - 0x10);
    c->r[17] = 0x08A58688u;
    int32_t base = *B(o + 11) != 0 ? 0x1B0 : -0x30;
    uint32_t x = (uint32_t)((int32_t)(int16_t)(S16(q + 18) + base) + dx) << 16;
    *W(o + 16) = x;
    call_on(c, x, 0x08913308u, f_08821368);
    *W(o + 20) = ((c->r[2] & 0x3F) + 0x48) << 16;
    call_on(c, o, 0x08913320u, f_089133E8);
    if (*B(o + 3) != 0) call2(c, o, *W(0x08A58688u + 24), 0x08913348u, f_0882196C);
    else call2(c, o, *W(0x08A58688u + 20), 0x08913338u, f_0882196C);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, fighter): one time in 16 (0x08821368) unless the fighter's +0x343 is set, a list-1 effect
   (0x08822EAC, 0x1C) copied from the owner (its +88 / +11 - +14 / +24 / place / +86 / +164, kept at the
   owner's +96), moved by the fighter's +77 offset pair (0x08A947D4, x turned for its +11), +139 =
   the fighter's +373; the fighter's +0x33C cleared, the owner's +0x66 = 0x20. */
SFA_FUNC(08987FA4, fighter_sparkle) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[17] = o;
    c->r[31] = 0x08987FC4u; f_08821368(c);
    gaddr g = f + 0x33C;
    c->r[18] = g;
    if ((c->r[2] & 0xF) == 0 && S8(g + 7) == 0) {
        call2(c, 1, 0x1C, 0x08987FE4u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) {
            *W(n + 88) = *W(o + 88);
            *B(n + 12) = (uint8_t)S8(o + 12);
            *B(n + 13) = *B(o + 13);
            *B(n + 14) = *B(o + 14);
            *B(n + 11) = *B(o + 11);
            *H(n + 24) = *H(o + 24);
            *W(n + 16) = *W(o + 16);
            *W(n + 20) = *W(o + 20);
            *H(n + 86) = *H(o + 86);
            *W(n + 164) = *W(o + 164);
            *W(o + 0x60) = n;
            gaddr e = 0x08A947D4u + 4 * (uint32_t)S8(f + 77);
            int32_t dx = S16(e), dy = S16(e + 2);
            if (*B(f + 11) != 0) dx = -dx;
            c->r[17] = (uint32_t)dx;
            *H(n + 18) = (uint16_t)(S16(n + 18) + dx);
            *H(n + 22) = (uint16_t)(S16(n + 22) + dy);
            *B(n + 139) = (uint8_t)S8(f + 373);
            *B(g) = 0;
            *B(o + 0x66) = 0x20;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (a, b, box a, box b, x offset << 16): 1 when the two boxes (x / y centres +0 / +2, half sizes
   +4 / +6) overlap, a's at its place with the offset (both turned for their +11), else 0. */
SFA_FUNC(08873AD8, boxes_overlap) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    int32_t off = (int32_t)c->r[8] >> 16;
    int16_t xa = *B(a + 11) == 0 ? (int16_t)(S16(ba) + off) : (int16_t)(off - S16(ba));
    int16_t xb = *B(b + 11) != 0 ? (int16_t)(S16(b + 18) - S16(bb)) : (int16_t)(S16(bb) + S16(b + 18));
    int16_t d = (int16_t)(xb - xa);
    if (d < 0) d = (int16_t)-d;
    uint32_t r = 0;
    if (!((uint16_t)(int16_t)(S16(ba + 4) + S16(bb + 4)) < (uint16_t)d)) {
        int16_t ya = (int16_t)(S16(ba + 2) + S16(a + 22));
        int16_t yb = (int16_t)(S16(bb + 2) + S16(b + 22));
        d = (int16_t)(yb - ya);
        if (d < 0) d = (int16_t)-d;
        if (!((uint16_t)(int16_t)(S16(ba + 6) + S16(bb + 6)) < (uint16_t)d)) r = 1;
    }
    c->r[2] = r;
}
