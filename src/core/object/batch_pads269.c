/* Whether a pad has pressed a menu button: for a player slot (0x0882BCB0), for the game's pads by its
   flags (0x0882BB68), and the plain check (0x0882B8EC). Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D5E44(Cpu *c); void f_0882BCB0(Cpu *c); void f_0882BB68(Cpu *c);

/* (game, a1, fighter): 1 when the fighter's player (+337) is in the game's set (+57) and its pad (by +775
   from 0x08A0EF10) has pressed one of 0x83F0. */
SFA_FUNC(0882BCB0, slot_pressed) {
    gaddr g = ARG(0), f = ARG(2);
    int32_t k = S8(f + 337);
    c->r[2] = 0;
    if ((S8(g + 57) & (1 << (k & 31))) == 0) return;
    int32_t i = S8(f + 775);
    if (i < 0) return;
    gaddr pad = *W(0x08A0EF10u + ((uint32_t)i << 2));
    if (*W(pad + 12) & 0x83F0) c->r[2] = 1;
}

/* (game, a1, mask): 1 when a pad pressed one of `mask`: both pads (0x08E75BA0 +12 / +56) when 0x088D5E44
   says so and +146 is set, else the pads the game's +57 bits 0 / 1 name. (Mode 0xD has a branch for mode
   0x13 that can never be taken: it would ask 0x0882BCB0 for the four slots at 0x092E3EA0.) A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(0882BB68, pads_pressed) {
    gaddr g = ARG(0); uint32_t a1 = ARG(1), mask = ARG(2) & 0xFFFF;
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[2]; *W(sp + 20) = sv[1]; *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    c->r[18] = g; c->r[17] = mask; c->r[16] = 0; c->r[19] = a1;
    int32_t m = S8(g + 114);
    ARG(0) = (uint32_t)m;
    if (m == 0xD) { ARG(1) = 0x13; ARG(2) = 0x092E0000u; } else ARG(2) = 0xD;
    c->r[31] = 0x0882BBB0u; f_088D5E44(c);
    const gaddr pads = 0x08E75BA0u;
    uint32_t hit = 0;
    if (c->r[2] != 0 && S8(g + 146) != 0) {
        if (*W(pads + 12) & mask) hit = 1;
        if (*W(pads + 56) & mask) hit = 1;
    } else {
        int32_t fl = S8(g + 57);
        c->r[18] = (uint32_t)fl;
        if ((fl & 1) && (*W(pads + 12) & mask)) hit = 1;
        if ((fl & 2) && (*W(pads + 56) & mask)) hit = 1;
    }
    c->r[2] = hit;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (game, a1): 0 while 0x088D5E44 says so, else whether a pad pressed one of mask. */
static void pressed_mask(Cpu *c, uint32_t mask, uint32_t b1, uint32_t b2) {
    gaddr g = ARG(0); uint32_t a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[16] = a1; c->r[17] = g;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = 0;
    c->r[31] = b1; f_088D5E44(c);
    ARG(2) = mask;
    if (c->r[2] == 0) {
        ARG(0) = g; ARG(1) = a1; c->r[31] = b2; f_0882BB68(c);
        c->r[18] = c->r[2];
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
/* a menu button (0x83F0) */
SFA_FUNC(0882B8EC, menu_pressed) { pressed_mask(c, 0x83F0, 0x0882B910u, 0x0882B924u); }
SFA_FUNC(0882BB10, menu_pressed_b) { pressed_mask(c, 0x83F0, 0x0882BB34u, 0x0882BB48u); }
/* button bit 15 (0x8000) */
SFA_FUNC(0882B894, pressed_8000) { pressed_mask(c, 0x8000, 0x0882B8B8u, 0x0882B8CCu); }


void f_0882B9BC(Cpu *c); void f_08896068(Cpu *c);

/* (screen): +10 on and +20 = 0xFF00 on a menu press or when its timer (+22) runs out. */
SFA_FUNC(088960A8, screen_next_on_press) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(1) = 0x092E1228u; c->r[31] = 0x088960C4u; f_0882B8EC(c);
    int next = c->r[2] != 0;
    if (!next) {
        int32_t t = S16(o + 22) - 1;
        *H(o + 22) = (uint16_t)t;
        next = (int16_t)t == 0;
    }
    if (next) { *H(o + 10) = (uint16_t)(*H(o + 10) + 1); *H(o + 20) = 0xFF00; }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (screen, item): at item step 7 the screen moves on (+6 on, +22 / +20 / +18 / +10 / +8 cleared); before,
   a menu press takes the item to step 5. */
SFA_FUNC(08826D6C, item_skip) {
    gaddr o = ARG(0), it = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = it;
    if (S8(it + 44) == 7) {
        *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
        *H(o + 22) = 0; *H(o + 20) = 0; *H(o + 18) = 0; *H(o + 10) = 0; *H(o + 8) = 0;
    } else {
        c->r[31] = 0x08826D90u; f_0882B8EC(c);
        if (c->r[2] != 0 && S8(it + 44) < 5) *B(it + 44) = 5;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (game, a1): 0 while 0x088D5E44 says so; else the menu press for the game's pads (+57), or 0x0882B9BC
   without any. */
SFA_FUNC(0882B944, menu_pressed_any) {
    gaddr g = ARG(0); uint32_t a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = a1;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = g;
    c->r[31] = 0x0882B960u; f_088D5E44(c);
    if (c->r[2] != 0) c->r[2] = 0;
    else if (S8(g + 57) == 0) { ARG(0) = g; ARG(1) = a1; c->r[31] = 0x0882B9A8u; f_0882B9BC(c); }
    else { ARG(0) = g; ARG(1) = a1; c->r[31] = 0x0882B980u; f_0882B8EC(c); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (screen): a menu press calls 0x08896068; else when +20's low byte reaches step +10's mark (0x08A272FE)
   +20 takes the next value (0x08A2730A) and +10 moves on. */
SFA_FUNC(08895F60, screen_steps) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(1) = 0x092E1228u; c->r[31] = 0x08895F7Cu; f_0882B8EC(c);
    if (c->r[2] != 0) { ARG(0) = o; c->r[31] = 0x08895F8Cu; f_08896068(c); }
    else {
        uint32_t n = *H(o + 10);
        if ((S16(o + 20) & 0xFF) == S16(0x08A272FEu + n * 2)) {
            *H(o + 20) = (uint16_t)S16(0x08A2730Au + n * 2);
            *H(o + 10) = (uint16_t)(n + 1);
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0882B824(Cpu *c); void f_08831FF4(Cpu *c);

static void enter2(Cpu *c, gaddr o, gaddr a1, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = a1; c->r[17] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) { c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20; }
/* the screen's timer at `off` counts down: 1 once it runs out */
static int timer_out(gaddr o, uint32_t off) {
    int32_t t = S16(o + off) - 1;
    *H(o + off) = (uint16_t)t;
    return (int16_t)t < 0;
}
static void call_oa(Cpu *c, gaddr o, gaddr a1, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = a1; c->r[31] = back; fn(c);
}

/* (screen, a1): on a menu press or when +16 runs out +4 on and 0x0882B824. */
SFA_FUNC(0882B7B8, screen_next_b824) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    call_oa(c, o, a1, f_0882BB10, 0x0882B7D4u);
    if (c->r[2] != 0 || timer_out(o, 16)) {
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        call_oa(c, o, a1, f_0882B824, 0x0882B810u);
    }
    leave2(c, sv);
}

/* (screen, a1): unless +38, on a menu press (any pad) or when +18 runs out +6 on and a1's +82 = 1. */
SFA_FUNC(08827F08, screen_next_82) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    if (S8(o + 38) == 0) {
        call_oa(c, o, a1, f_0882B944, 0x08827F34u);
        if (c->r[2] != 0 || timer_out(o, 18)) { *H(o + 6) = (uint16_t)(*H(o + 6) + 1); *B(a1 + 82) = 1; }
    }
    leave2(c, sv);
}

/* (screen, a1): unless +38, button 15 makes +10 = 5 with +18 cleared and +20 = 0x18, a menu press +10 = 2;
   either sets a1's +82 = 2. */
SFA_FUNC(08895AB0, screen_pick_2_5) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    if (S8(o + 38) == 0) {
        call_oa(c, o, a1, f_0882B894, 0x08895ADCu);
        if (c->r[2] != 0) { *B(a1 + 82) = 2; *H(o + 10) = 5; *H(o + 18) = 0; *H(o + 20) = 0x18; }
        else {
            call_oa(c, o, a1, f_0882B8EC, 0x08895B0Cu);
            if (c->r[2] != 0) { *H(o + 10) = 2; *B(a1 + 82) = 2; }
        }
    }
    leave2(c, sv);
}

/* (screen, a1): button 15 calls 0x08831FF4; a menu press or +22 running out sets +10 = 2 and a1's +82 = 2. */
SFA_FUNC(08831F34, screen_skip_2) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    call_oa(c, o, a1, f_0882B894, 0x08831F50u);
    if (c->r[2] != 0) call_oa(c, o, a1, f_08831FF4, 0x08831F64u);
    else {
        call_oa(c, o, a1, f_0882B8EC, 0x08831F74u);
        if (c->r[2] != 0 || timer_out(o, 22)) { *H(o + 10) = 2; *B(a1 + 82) = 2; }
    }
    leave2(c, sv);
}

/* (screen, a1): button 15 calls 0x08831FF4; a menu press or +22 running out moves +10 on with a1's +82 = 1. */
SFA_FUNC(08831EA8, screen_skip_next) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    call_oa(c, o, a1, f_0882B894, 0x08831EC4u);
    if (c->r[2] != 0) call_oa(c, o, a1, f_08831FF4, 0x08831ED8u);
    else {
        call_oa(c, o, a1, f_0882B8EC, 0x08831EE8u);
        if (c->r[2] != 0 || timer_out(o, 22)) { *H(o + 10) = (uint16_t)(*H(o + 10) + 1); *B(a1 + 82) = 1; }
    }
    leave2(c, sv);
}


void f_08819144(Cpu *c);

/* (screen, a1): a menu press with 0x08E7606C set (a1's +81 = -1), or +20's low byte at 0x7F, moves +10 on
   with +22 = 0xC0; then 0x08819144. */
SFA_FUNC(0889641C, screen_press_or_7f) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    call_oa(c, o, a1, f_0882B8EC, 0x08896438u);
    if (c->r[2] != 0 && *B(0x08E70000u + 0x6068 + 4) != 0) {
        *B(a1 + 81) = 0xFF;
        *H(o + 10) = (uint16_t)(*H(o + 10) + 1); *H(o + 22) = 0xC0;
    } else if ((S16(o + 20) & 0xFF) == 0x7F) {
        *H(o + 10) = (uint16_t)(*H(o + 10) + 1); *H(o + 22) = 0xC0;
    }
    c->r[31] = 0x0889649Cu; f_08819144(c);
    leave2(c, sv);
}

/* (screen, a1): unless +38, button 15 makes +10 = 5 (+18 cleared, +20 = 0x18, a1's +82 = 2); a menu
   press, +22 running out or a1's +82 set moves +10 on with +82 = 1. */
SFA_FUNC(088959FC, screen_pick_5_next) {
    gaddr o = ARG(0), a1 = ARG(1); uint32_t sv[3]; enter2(c, o, a1, sv);
    if (S8(o + 38) == 0) {
        call_oa(c, o, a1, f_0882B894, 0x08895A28u);
        if (c->r[2] != 0) { *B(a1 + 82) = 2; *H(o + 10) = 5; *H(o + 18) = 0; *H(o + 20) = 0x18; }
        else {
            call_oa(c, o, a1, f_0882B8EC, 0x08895A58u);
            if (c->r[2] != 0 || timer_out(o, 22) || S8(a1 + 82) != 0) {
                *H(o + 10) = (uint16_t)(*H(o + 10) + 1); *B(a1 + 82) = 1;
            }
        }
    }
    leave2(c, sv);
}


void f_08886590(Cpu *c);

/* (screen): a menu press (with +10 set and 0x08E7606C set) calls 0x08886590; otherwise, and without a
   press, +5 on, +18 = 4 and its three entries from +32 (16 bytes each) step: +11 doubled, +10 flipped. */
SFA_FUNC(08885F2C, screen_blink) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = 2;
    *W(c->r[29] + 24) = ra;
    ARG(0) = 0x08E75910u; ARG(1) = 0x092E1228u; c->r[31] = 0x08885F58u; f_0882B8EC(c);
    int skip = c->r[2] != 0 && (*B(o + 10) == 0 || *B(0x08E70000u + 0x6068 + 4) != 0);
    if (skip) { ARG(0) = o; c->r[31] = 0x08885FD4u; f_08886590(c); }
    else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *B(o + 0x12) = 4;
        gaddr p = o + 0x20;
        for (int32_t k = 2; ; ) {
            *B(p + 11) = (uint8_t)(*B(p + 11) << 1);
            *B(p + 10) = (uint8_t)~S8(p + 10);
            int32_t old = k;
            k = (int8_t)(k - 1);
            p += 0x10;
            if (old == 0) break;
        }
        c->r[17] = 0xFFFFFFFFu; c->r[16] = p;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (screen, item): unless +38, on a menu press (any pad) or when +18 runs out the item goes to step 8;
   with +69 the screen restarts (+2 = 3, its counters cleared, the item's +56 / +81 cleared), else +6 on
   and the item's +81 cleared; +43 = 1 both ways. */
SFA_FUNC(08827F80, screen_item_done) {
    gaddr o = ARG(0), it = ARG(1); uint32_t sv[3]; enter2(c, o, it, sv);
    if (S8(o + 38) == 0) {
        call_oa(c, o, it, f_0882B944, 0x08827FACu);
        if (c->r[2] != 0 || timer_out(o, 18)) {
            *B(it + 44) = 8;
            if (S8(o + 69) != 0) {
                *H(o + 2) = 3; *H(it + 56) = 0; *B(it + 81) = 0;
                static const uint8_t z[] = { 22, 20, 18, 16, 10, 8, 6, 4 };
                for (unsigned i = 0; i < sizeof z; i++) *H(o + z[i]) = 0;
                *B(o + 43) = 1;
            } else {
                *B(o + 43) = 1;
                *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
                *B(it + 81) = 0;
            }
        }
    }
    leave2(c, sv);
}


void f_0882E0D0(Cpu *c); void f_08885910(Cpu *c);

/* (screen): on a menu press or when +18 runs out (with +10 only if 0x08E7606C is set) its phase (+6)
   runs: phase 0 sets +15 = 0xFF, +18 = 0 and counts +7 down to phase 1; phase 1 is phase 2 with +7 = 6 and
   0x0882E0D0 (2, 6); later +7 counts down and at the end the screen resets its parts (+96 / +112 cleared,
   +82 = 0x100, +84 = 0x80, phase 0, +5 on, +18 = 0x13) with 0x08885910 (+80, 0x14, +18). */
SFA_FUNC(088861E8, screen_phases) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(0) = 0x08E75910u; ARG(1) = 0x092E1228u; c->r[31] = 0x0888620Cu; f_0882B8EC(c);
    gaddr t = o + 0x12;
    if (c->r[2] == 0) {
        int32_t v = S8(t) - 1;
        *B(t) = (uint8_t)v;
        if ((int8_t)v >= 0) goto out;
    }
    if (*B(o + 10) != 0 && *B(0x08E70000u + 0x6068 + 4) == 0) goto out;
    {
        uint32_t k = *B(o + 6);
        if (k == 0) {
            *B(o + 15) = 0xFF; *B(t) = 0;
            uint32_t n = (uint32_t)(*B(o + 7) - 1) & 0xFF;
            *B(o + 7) = (uint8_t)n;
            if (n == 0) *B(o + 6) = 1;
        } else if (k == 1) {
            *B(o + 6) = 2; *B(o + 7) = 6;
            ARG(0) = 2; ARG(1) = 6; c->r[31] = 0x088862A8u; f_0882E0D0(c);
        } else {
            uint32_t v = *B(o + 7);
            *B(o + 7) = (uint8_t)(v - 1);
            if (v == 0) {
                *B(o + 96) = 0; *B(o + 112) = 0;
                *H(o + 0x50 + 2) = 0x100; *H(o + 0x50 + 4) = 0x80;
                *B(o + 6) = 0;
                *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
                *B(t) = 0x13;
                ARG(0) = o + 0x50; ARG(1) = 0x14; ARG(2) = t; c->r[31] = 0x08886304u; f_08885910(c);
            }
        }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}


void f_088E2304(Cpu *c); void f_088858D8(Cpu *c); void f_088D7C1C(Cpu *c);

/* the screen moves on once 0x088858D8 (0, 0xD) allows: 0x092E510A = 1, +10 on, +43 = 1, +72 / +40 = -1 */
static void screen_leave_d(Cpu *c, gaddr o, uint32_t back) {
    ARG(0) = 0; ARG(1) = 0xD; c->r[31] = back; f_088858D8(c);
    if (c->r[2] != 0) {
        *B(0x092E510Au) = 1;
        *H(o + 10) = (uint16_t)(*H(o + 10) + 1);
        *B(o + 43) = 1; *B(o + 72) = 0xFF; *B(o + 40) = 0xFF;
    }
}

/* (screen, a1): when 0x088D7C1C says so it leaves at once with 0x08E7606C set (0x088E2304, +157 = -1);
   otherwise, unless +38, its timer +22 runs out (cut short by a menu press with 0x08E7606C set) and it
   leaves; then 0x08819144. */
SFA_FUNC(088964B0, screen_leave_or_time) {
    gaddr o = ARG(0), a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = a1; c->r[16] = o;
    c->r[31] = 0x088964CCu; f_088D7C1C(c);
    const gaddr flag = 0x08E70000u + 0x6068 + 4;
    if (c->r[2] != 0) {
        if (*B(flag) != 0) {
            c->r[31] = 0x088964ECu; f_088E2304(c);
            c->r[17] = 0xFFFFFFFFu;
            *B(o + 157) = 0xFF;
            screen_leave_d(c, o, 0x08896500u);
        }
    } else if (S8(o + 38) == 0) {
        ARG(0) = o; ARG(1) = a1; c->r[31] = 0x08896548u; f_0882B8EC(c);
        int32_t t;
        if (c->r[2] != 0 && *B(flag) != 0) { *H(o + 22) = 0; t = S16(o + 22); }
        else {
            t = S16(o + 22);
            if (t != 0) { *H(o + 22) = (uint16_t)(t - 1); t = S16(o + 22); }
        }
        if (t == 0) screen_leave_d(c, o, 0x08896594u);
    }
    c->r[31] = 0x088965D0u; f_08819144(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
