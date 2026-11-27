/* What the roots of batch_roots283 set free: scrolling-list steps, the colour-cycling object's steps,
   a bar's icon row steps, the reach checks before a block, the cpu's draw by mode, and the 2D
   library's texture copies. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_0882B8EC(Cpu *c); void f_0888639C(Cpu *c); void f_08886454(Cpu *c); void f_08886590(Cpu *c);
void f_0889E1EC(Cpu *c); void f_0889E3F8(Cpu *c); void f_0889E318(Cpu *c); void f_0889CE64(Cpu *c);
void f_0889CEA4(Cpu *c); void f_0889CFE8(Cpu *c); void f_088988FC(Cpu *c); void f_0889FB94(Cpu *c);
void f_0889F35C(Cpu *c); void f_0882196C(Cpu *c); void f_0889F31C(Cpu *c); void f_08876F78(Cpu *c);
void f_08876EA0(Cpu *c); void f_088D5E44(Cpu *c); void f_08837300(Cpu *c); void f_089E0778(Cpu *c);
void f_089E06BC(Cpu *c); void f_089E05A4(Cpu *c); void f_089E04EC(Cpu *c); void f_089F3430(Cpu *c);
void f_089E07AC(Cpu *c); void f_089E0958(Cpu *c);
void hle_sceKernelDcacheWritebackRange(Cpu *c); void hle_sceDmacMemcpy(Cpu *c);
void hle_sceKernelDcacheInvalidateRange(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
/* The frame most of these keep: 0x20 bytes with s0 at +16 and the return address at +20; s0 holds the
   object. */
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* What the scrolling list's steps start with: 0x0882B8EC (GAME, BATTLE); when it says so and the
   list's +10 is clear or 0x08E7606C is set, the step is 0x08886590 instead: 1. */
static int scroll_other(Cpu *c, gaddr o, uint32_t back, uint32_t back_other) {
    call2(c, GAME, BATTLE, f_0882B8EC, back);
    if (c->r[2] != 0 && (*B(o + 10) == 0 || *B(0x08E7606Cu) != 0)) {
        call1(c, o, f_08886590, back_other);
        return 1;
    }
    return 0;
}

/* (list): a scrolling step: 0x0888639C with the piece +0x10's +1 picks by the table 0x08A25800; when
   that is done the count +0x13 runs down and then the next wait comes from 0x08A25804 (+0x11 counts
   them) into +0x12, a negative one ending the step (+5 on, +16 and +18 cleared); then the scroll. A
   frame keeping s0, s1. */
SFA_FUNC(08885C40, scroll_step_pieces) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (!scroll_other(c, o, 0x08885C68u, 0x08885D20u)) {
        c->r[17] = o + 0x10;
        call2(c, o, o + *B(0x08A25800u + *B(o + 0x11)), f_0888639C, 0x08885C94u);
        if (c->r[2] == 0) {
            int32_t t = S8(o + 0x13) - 1;
            *B(o + 0x13) = (uint8_t)t;
            if ((int8_t)t < 0) {
                uint32_t i = *B(o + 0x11);
                *B(o + 0x11) = (uint8_t)(i + 1);
                int32_t wait = S8(0x08A25804u + i);
                *B(o + 0x12) = (uint8_t)wait;
                if (wait < 0) {
                    uint8_t step = *B(o + 5);
                    *H(o + 16) = 0; *B(o + 5) = (uint8_t)(step + 1); *H(o + 18) = 0;
                }
            }
        }
        call1(c, o, f_08886454, 0x08885CF8u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (list): a scrolling step: when the wait +0x12 runs out, the next pair from 0x08A25812 (+0x10 counts
   them): a wait, and a step size for the first three entries (+11); a negative wait ends it: +5 on,
   the wait 6, and the three entries' places (+2, +4) from 0x08A257E8; otherwise the scroll. A frame
   keeping s0, s1. */
SFA_FUNC(08885D34, scroll_step_speeds) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = 2;
    *W(c->r[29] + 24) = ra;
    if (!scroll_other(c, o, 0x08885D60u, 0x08885D8Cu)) {
        int32_t t = S8(o + 0x12) - 1;
        *B(o + 0x12) = (uint8_t)t;
        if ((int8_t)t >= 0) call1(c, o, f_08886454, 0x08885DB8u);
        else {
            int32_t i = S8(o + 0x10);
            gaddr e = 0x08A25812u + (uint32_t)(i << 1);
            int8_t wait = S8(e), size = S8(e + 1);
            *B(o + 0x10) = (uint8_t)(i + 1);
            for (uint32_t k = 0; k < 3; k++) *B(o + 0x20 + k * 0x10 + 11) = (uint8_t)size;
            c->r[17] = 0xFFFFFFFFu;
            *B(o + 0x12) = (uint8_t)wait;
            if (wait >= 0) call1(c, o, f_08886454, 0x08885E20u);
            else {
                *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
                *B(o + 0x12) = 6;
                for (uint32_t k = 0; k < 3; k++) {
                    *H(o + 0x20 + k * 0x10 + 2) = (uint16_t)S16(0x08A257E8u + 4 * k);
                    *H(o + 0x20 + k * 0x10 + 4) = (uint16_t)S16(0x08A257E8u + 4 * k + 2);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (list): a scrolling step that only waits: +5 on when the wait +0x12 runs out; the scroll. */
SFA_FUNC(08885E94, scroll_step_wait) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (!scroll_other(c, o, 0x08885EB8u, 0x08885F1Cu)) {
        int32_t t = S8(o + 0x12) - 1;
        *B(o + 0x12) = (uint8_t)t;
        if ((int8_t)t < 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        call1(c, o, f_08886454, 0x08885EF4u);
    }
    leave(c, sv);
}

/* (list): the scrolling's last step: the scroll until the wait +0x12 runs out; then +5 on, +16
   cleared, and all six entries set down unwrapped (+1) at their places from 0x08A257E8. */
SFA_FUNC(08885FE8, scroll_step_settle) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (!scroll_other(c, o, 0x0888600Cu, 0x088860C4u)) {
        int32_t t = S8(o + 0x12) - 1;
        *B(o + 0x12) = (uint8_t)t;
        if ((int8_t)t >= 0) call1(c, o, f_08886454, 0x0888609Cu);
        else {
            uint8_t step = *B(o + 5);
            *H(o + 16) = 0;
            *B(o + 5) = (uint8_t)(step + 1);
            for (uint32_t k = 0; k < 6; k++) {
                gaddr e = o + 0x20 + k * 0x10;
                *B(e + 1) = 1;
                *H(e + 2) = (uint16_t)S16(0x08A257E8u + 4 * k);
                *H(e + 4) = (uint16_t)S16(0x08A257E8u + 4 * k + 2);
            }
        }
    }
    leave(c, sv);
}

/* (object): the colour-cycling object's rest: step 3 at its home (+112, +114), then its colours
   (0x0889E318) and its drawing (0x0889E3F8). */
SFA_FUNC(0889E1EC, cycler_home) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int16_t x = S16(o + 112);
    *B(o + 5) = 3; *B(o + 6) = 0;
    int16_t y = S16(o + 114);
    *H(o + 18) = (uint16_t)x; *H(o + 22) = (uint16_t)y; *H(o + 20) = 0; *H(o + 16) = 0;
    *W(c->r[29] + 20) = ra;
    call1(c, o, f_0889E318, 0x0889E228u);
    call1(c, o, f_0889E3F8, 0x0889E230u);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): a wait (+82) before the next step (+5 on, +6 cleared), drawn; straight home while BATTLE
   + 81. A frame with only the return address. */
SFA_FUNC(0889DFF4, cycler_wait) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(BATTLE + 81) != 0) { c->r[31] = 0x0889E018u; f_0889E1EC(c); }
    else {
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) {
            uint8_t step = *B(o + 5);
            *B(o + 6) = 0; *B(o + 5) = (uint8_t)(step + 1);
        }
        c->r[31] = 0x0889E054u; f_0889E3F8(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): the move home: each axis until it arrives (+6 bits 0 and 1): across by 0x0889CE64, run
   with +114 holding +112, then set at +112; down by 0x0889CEA4, then set at +114; both there: +5 on,
   +6 cleared, the wait 16 and the colour count 1; colours and drawing. Straight home while BATTLE +
   81. A frame keeping s0, s1. */
SFA_FUNC(0889E060, cycler_move_home) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(BATTLE + 81) != 0) call1(c, o, f_0889E1EC, 0x0889E08Cu);
    else {
        uint32_t there = *B(o + 6);
        if (!(there & 1)) {
            int16_t keep = S16(o + 114);
            c->r[17] = (uint32_t)(int32_t)keep;
            *H(o + 114) = (uint16_t)S16(o + 112);
            call1(c, o, f_0889CE64, 0x0889E0B8u);
            there = *B(o + 6);
            if (c->r[2] != 0) {
                *H(o + 18) = (uint16_t)S16(o + 112); *H(o + 16) = 0;
                *B(o + 6) = (uint8_t)(there | 1);
                there = *B(o + 6);
            }
            *H(o + 114) = (uint16_t)keep;
        }
        if (!(there & 2)) {
            call1(c, o, f_0889CEA4, 0x0889E0F0u);
            there = *B(o + 6);
            if (c->r[2] != 0) {
                *H(o + 22) = (uint16_t)S16(o + 114); *H(o + 20) = 0;
                *B(o + 6) = (uint8_t)(there | 2);
                there = *B(o + 6);
            }
        }
        if (there == 3) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *B(o + 6) = 0; *H(o + 82) = 0x10; *B(o + 74) = 1;
        }
        call1(c, o, f_0889E318, 0x0889E144u);
        call1(c, o, f_0889E3F8, 0x0889E14Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): a wait (+82), then +5 on and GAME + 20 bit 0 set; colours and drawing. Straight home
   while BATTLE + 81. */
SFA_FUNC(0889E160, cycler_wait_flag) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(BATTLE + 81) != 0) call1(c, o, f_0889E1EC, 0x0889E188u);
    else {
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 1);
        }
        call1(c, o, f_0889E318, 0x0889E1D4u);
        call1(c, o, f_0889E3F8, 0x0889E1DCu);
    }
    leave(c, sv);
}

/* (object): waits for GAME + 20 bit 7: then +5 on and the wait 16; colours and drawing. */
SFA_FUNC(0889E240, cycler_await_flag) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    int16_t flags = S16(GAME + 20);
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (flags & 0x80) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0x10;
    }
    call1(c, o, f_0889E318, 0x0889E284u);
    call1(c, o, f_0889E3F8, 0x0889E28Cu);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (bar, n): a bar's icon row at its end: steps +5 and +4 = 1, the row's length (+82) n more than
   BATTLE + 72, and the icons (0x0889F35C). A frame with only the return address. */
SFA_FUNC(0889F31C, icon_row_end) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 5) = 1; *B(o + 4) = 1;
    int32_t n = (int16_t)ARG(1) + S16(BATTLE + 72);
    *W(c->r[29] + 16) = ra;
    *H(o + 82) = (uint16_t)n;
    c->r[31] = 0x0889F350u; f_0889F35C(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (bar): the icon row's first step: set up (0x0889CFE8, +148 = 2, +86 = 8, the wait +112 = 16), across
   by the owner's side (0x088988FC into 0x08A2B180), down by 0x0889FB94, the list's ten icons blanked
   (their top bit kept), the icons filled and the animation 0x08A28410 started (0x0882196C). Later:
   the row's end with two spare while BATTLE + 81; and once GAME + 20's high byte is 3, +4 on and +5
   cleared. */
SFA_FUNC(0889F0DC, icon_row_start) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 5);
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
    if (step == 0) {
        call2(c, o, 0, f_0889CFE8, 0x0889F100u);
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *W(o + 148) = 2; *H(o + 86) = 8;
        ARG(1) = 0x10; ARG(0) = *W(o + 92);
        *H(o + 112) = 0x10;
        c->r[31] = 0x0889F12Cu; f_088988FC(c);
        *H(o + 18) = (uint16_t)S16(0x08A2B180u + c->r[2] * 2);
        call1(c, o, f_0889FB94, 0x0889F14Cu);
        *H(o + 22) = (uint16_t)c->r[2];
        for (uint32_t i = 0; i < 10; i++) {
            gaddr e = 0x08A282C6u + i * 6;
            *H(e + 4) = (uint16_t)((*H(e + 4) & 0x8000) | 0x1EF);
        }
        call1(c, o, f_0889F35C, 0x0889F190u);
        call2(c, o, *W(0x08A28410u), f_0882196C, 0x0889F1A4u);
    } else if (S8(BATTLE + 81) != 0) call2(c, o, 2, f_0889F31C, 0x0889F1CCu);
    else if ((S16(GAME + 20) & 0xFF00) == 0x300) {
        uint8_t up = *B(o + 4);
        *B(o + 5) = 0; *B(o + 4) = (uint8_t)(up + 1);
    }
    leave(c, sv);
}

/* (bar): the icon row filling: every fourth frame (+112) one more shown (+82, up to BATTLE + 72 + 1),
   the icons filled; with none left over, +5 on and GAME + 20 bit 5 set. The row's end while BATTLE +
   81. */
SFA_FUNC(0889F254, icon_row_fill) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *W(c->r[29] + 20) = sv[1];
    if (S8(BATTLE + 81) != 0) call2(c, o, 0, f_0889F31C, 0x0889F284u);
    else {
        int32_t t = S16(o + 112) - 1;
        *H(o + 112) = (uint16_t)t;
        if ((int16_t)t == 0) {
            int32_t shown = S16(o + 82);
            *H(o + 112) = 4;
            *H(o + 82) = (uint16_t)(shown + 1);
            int32_t all = S16(BATTLE + 72) + 1;
            if ((int16_t)all < S16(o + 82)) *H(o + 82) = (uint16_t)all;
        }
        call1(c, o, f_0889F35C, 0x0889F2E4u);
        if (c->r[2] == 0) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 0x20);
        }
    }
    leave(c, sv);
}

/* (fighter, other): whether the other's attack is one to block: no when the fighter is in the air (+73)
   and the other on the ground; yes while the other's +502; no while its +307 bit 7 or with no attack
   out (+235); else the reach check 0x08876F78. A frame with only the return address. */
SFA_FUNC(08876EA0, attack_to_block) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(a + 73) != 0 && S8(b + 73) == 0) c->r[2] = 0;
    else if (S8(b + 502) != 0) c->r[2] = 1;
    else if (*B(b + 307) & 0x80) c->r[2] = 0;
    else {
        c->r[2] = 0;
        if (S8(b + 235) != 0) { c->r[6] = b; call2(c, a, b, f_08876F78, 0x08876F38u); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): whether there is something to block: 1 for a live projectile of the other side (+121)
   within 80 of the fighter, in either list of twelve (0x0950FFB0, 0x09511090, 0x168 bytes each, in
   use = 1, +136 not negative); else the opponent's attack (0x08876EA0, opponent +92). */
SFA_FUNC(08876C50, something_to_block) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t side = S8(o + 121), x = S16(o + 18);
    *W(c->r[29] + 16) = ra;
    static const gaddr lists[2] = { 0x0950FFB0u, 0x09511090u };
    for (int l = 0; l < 2; l++) {
        for (uint32_t i = 0; i < 12; i++) {
            gaddr e = lists[l] + i * 0x168;
            if (*B(e) != 1 || S8(e + 136) < 0 || S8(e + 121) == side) continue;
            if ((((uint32_t)S16(e + 18) - (uint32_t)x + 0x50) & 0xFFFF) < 160) {
                c->r[2] = 1;
                c->r[29] += 0x20;
                return;
            }
        }
    }
    call2(c, o, *W(o + 92), f_08876EA0, 0x08876D24u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The cpu fighters (+373) all get the draw v. */
static void draws_set(uint32_t v) {
    static const gaddr f[4] = { FIGHTER_1, FIGHTER_2, FIGHTER_3, FIGHTER_4 };
    int8_t on[4];
    for (int i = 0; i < 4; i++) on[i] = S8(f[i] + 373);
    for (int i = 0; i < 4; i++) if (on[i] != 0) *B(f[i] + 499) = (uint8_t)v;
}

/* (): the cpu fighters' draw (+499) by mode: in mode 7 the pair's two get 0x40, 0x20 from BATTLE + 79
   = 7 on; in mode 16 the cpu ones 0; over the network (0x088D5E44) the cpu ones 0x40; while GAME + 191
   the players with +432 and the pair's two 0x40; else the lots (0x08837300). A 0x10 frame keeping
   s0. */
SFA_FUNC(088374EC, cpu_draw_by_mode) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; c->r[16] = GAME;
    int32_t mode = S8(GAME + 114);
    *W(c->r[29] + 4) = ra;
    if (mode == 7) {
        uint32_t v = S8(BATTLE + 79) < 7 ? 0x40 : 0x20;
        *B(FIGHTER_3 + 499) = (uint8_t)v; *B(FIGHTER_4 + 499) = (uint8_t)v;
    } else if (mode == 0x10) draws_set(0);
    else {
        call2(c, (uint32_t)mode, 0x092E0000u, f_088D5E44, 0x088375B8u);
        if (c->r[2] != 0) draws_set(0x40);
        else if (S8(GAME + 191) != 0) {
            if (S8(FIGHTER_1 + 432) != 0) *B(FIGHTER_1 + 499) = 0x40;
            if (S8(FIGHTER_2 + 432) != 0) *B(FIGHTER_2 + 499) = 0x40;
            *B(FIGHTER_3 + 499) = 0x40; *B(FIGHTER_4 + 499) = 0x40;
        } else { c->r[31] = 0x08837678u; f_08837300(c); }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

#define TEX_SLOTS  0x08B0C334u   /* 24 bytes each: +0 the pixels' copy, +4 where they were, +8 flags, +20 its record */
#define TEX_RECS   0x08B12334u   /* 68 bytes each */

/* (entry): gives a texture entry (+0 the texture, +4 flags, +8) flagged 0x40000000 its own copy: a
   slot (0x089E0778; one in use is let go first, 0x089E06BC), memory for the pixels and 16 more
   (0x089E04EC), the pixels copied from behind the picture's header (32 bytes for GBIX, 16 for PVRT),
   the slot's record filled from the texture (+4, 40 bytes, to +0x18) with the entry's +8 first, and
   the count at 0x08ACCD84 up; always 1. A 0x30 frame keeping s0..s3, the size at +16. */
SFA_FUNC(089E07AC, texture_copy_make) {
    gaddr t = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t flags = *W(t + 4);
    *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 28) = s2; *W(sp + 32) = s3; *W(sp + 36) = ra;
    c->r[16] = t;
    if (flags & 0x40000000u) {
        call1(c, t, f_089E0778, 0x089E07E0u);
        uint32_t n = c->r[2];
        gaddr slot = TEX_SLOTS + n * 24;
        c->r[19] = n; c->r[18] = slot;
        if (*W(slot) != 0) call1(c, n, f_089E06BC, 0x089E0810u);
        gaddr tex = *W(t);
        c->r[17] = tex;
        call1(c, tex, f_089E05A4, 0x089E081Cu);
        *W(sp + 16) = c->r[2];
        call1(c, c->r[2] + 0x10, f_089E04EC, 0x089E0828u);
        *W(slot) = c->r[2];
        gaddr head = *W(tex);
        uint32_t magic = *W(head), size = *W(sp + 16);
        uint32_t skip = magic == 0x58494247u ? 0x20 : magic == 0x54525650u ? 0x10 : 0;
        call3(c, *W(slot), head + skip, size, f_089F3430, 0x089E0874u);
        gaddr rec = TEX_RECS + n * 68;
        *W(slot + 8) = 0;
        c->r[19] = rec;
        *W(slot + 20) = rec;
        *W(0x08ACCD84u) = *W(0x08ACCD84u) + 1;
        *W(slot + 16) = 0x80000000u;
        call3(c, rec + 0x18, tex + 4, 0x28, f_089F3430, 0x089E08BCu);
        *W(rec) = *W(t + 8);
    }
    c->r[2] = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (slot): moves a slot's pixels into the fast pool (0x08B23354 up to 0x08B23358): nothing for an
   empty slot (and no result); 0 when they don't fit; else copied, the slot pointing at the copy with
   the old place kept (+4) and bit 0 of +8 set, the pool on by the size in whole 64s: 1. A frame keeping
   s0..s2. */
SFA_FUNC(089E0958, texture_to_pool) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    gaddr slot = TEX_SLOTS + ARG(0) * 24;
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[18] = slot;
    if (*W(slot) != 0) {
        call1(c, *W(slot + 20) + 0x14, f_089E05A4, 0x089E0998u);
        uint32_t size = c->r[2];
        gaddr top = *W(0x08B23354u);
        c->r[16] = 0x08B20000u; c->r[17] = size;
        if (*W(0x08B23358u) < top + size) c->r[2] = 0;
        else {
            call3(c, top, *W(slot), size, f_089F3430, 0x089E09C8u);
            uint32_t old = *W(slot);
            top = *W(0x08B23354u);
            uint32_t fl = *W(slot + 8);
            *W(slot + 4) = old; *W(slot) = top; *W(slot + 8) = fl | 1;
            *W(0x08B23354u) = top + ((size >> 6) << 6);
            c->r[2] = 1;
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (slot): the same behind a frame of its own. */
SFA_FUNC(089E0A58, texture_to_pool_call) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x089E0A68u; f_089E0958(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (list): copies made for every entry of a list (+0 the entries, 12 bytes each, +4 how many); 0. A
   frame keeping s0..s2. */
SFA_FUNC(089E0D54, texture_copies_make) {
    gaddr list = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[18] = 0; c->r[16] = list;
    if (*W(list + 4) > 0) {
        uint32_t i = 0;
        do {
            c->r[17] = i * 12; c->r[18] = i;
            call1(c, *W(list) + i * 12, f_089E07AC, 0x089E0D8Cu);
            i++;
        } while (i < *W(list + 4));
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): a copy made for entry n of the list at 0x08B23334; 0. */
SFA_FUNC(089E0E70, texture_copy_make_nth) {
    uint32_t ra = c->r[31], n = ARG(0);
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    call1(c, *W(0x08B23334u) + n * 12, f_089E07AC, 0x089E0E94u);
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (slot, pixels): new pixels for a slot's copy: its size's worth written back from the cache, copied
   over the slot's pixels, and the cache dropped for them; 0. A frame keeping s0..s2. */
SFA_FUNC(089E14D8, texture_pixels_set) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    gaddr slot = TEX_SLOTS + ARG(0) * 24, from = ARG(1);
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[17] = slot; c->r[16] = from;
    call1(c, *W(slot + 20) + 0x14, f_089E05A4, 0x089E1514u);
    uint32_t size = c->r[2];
    c->r[18] = size;
    call2(c, from, size, hle_sceKernelDcacheWritebackRange, 0x089E1524u);
    call3(c, *W(slot), from, size, hle_sceDmacMemcpy, 0x089E1534u);
    call2(c, *W(slot), size, hle_sceKernelDcacheInvalidateRange, 0x089E1540u);
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

void f_0880E998(Cpu *c); void f_089E0DE0(Cpu *c); void f_089E1204(Cpu *c); void f_089E1234(Cpu *c);
void f_089E0D54(Cpu *c); void f_089F3470(Cpu *c); void f_088D7EA0(Cpu *c); void f_089E15E8(Cpu *c);
void f_088AC550(Cpu *c); void f_088139CC(Cpu *c); void f_088075F8(Cpu *c); void f_088054F0(Cpu *c);
void f_089E11C8(Cpu *c); void f_0880B57C(Cpu *c); void f_08821FEC(Cpu *c); void f_088B2EFC(Cpu *c);
void f_0881CBDC(Cpu *c);

/* (): the sprite pictures' start: the picture cache's four lists (0x08AD9378: three lists and the
   count 4, 0x0880E998); once only (0x08A04010), the 512 cell textures (0x08AD93B8, 44 bytes each, all
   over the one 64 x 64 page at 0x092E09A0, format 0x500) entered in the list 0x08ADEBB8 as numbers
   0x100 on, and copies made for the list 0x08A04008; then the 128 bytes at 0x092E11A0 set to 0xFF,
   with 0x092E1220, and 0x08E7602C cleared. A 0x40 frame keeping s0..s6. */
SFA_FUNC(0880B57C, sprite_pictures_start) {
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x40;
    *W(0x08AD9378u) = 0x08AD9388u; *W(0x08AD937Cu) = 0x08AD9398u; *W(0x08AD9380u) = 0x08AD93A8u;
    for (uint32_t i = 0; i < 7; i++) *W(c->r[29] + 32 + 4 * i) = sv[i];
    *W(c->r[29] + 60) = sv[7];
    *W(0x08AD9384u) = 4;
    c->r[6] = 0x08AD93A8u;
    call2(c, 4, 0x08AD9378u, f_0880E998, 0x0880B5D8u);
    c->r[21] = 0x08A00000u;
    if (S8(0x08A04010u) == 0) {
        *B(0x08A04010u) = 1;
        c->r[20] = 0x08AD93B8u; c->r[18] = 0x092E09A0u; c->r[16] = 0x40800000u;
        for (uint32_t i = 0; i < 512; i++) {
            gaddr tex = 0x08AD93B8u + i * 0x2C, entry = 0x08ADEBB8u + i * 0xC;
            c->r[19] = tex; c->r[17] = entry; c->r[21] = i;
            c->r[7] = 0x40; c->r[8] = 0x40;
            call3(c, tex, 0x092E09A0u, 0x500, f_089E0DE0, 0x0880B638u);
            c->r[22] = i + 0x100;
            c->r[7] = 0x40800000u;
            call3(c, entry, 0x08AD93B8u, i + 0x100, f_089E1204, 0x0880B650u);
            call2(c, i + 0x100, 0, f_089E1234, 0x0880B65Cu);
        }
        c->r[21] = 512; c->r[19] = 0x08AD93B8u + 512 * 0x2C; c->r[17] = 0x08ADEBB8u + 512 * 0xC;
        call1(c, 0x08A04008u, f_089E0D54, 0x0880B67Cu);
    }
    call3(c, 0x092E11A0u, 0xFF, 0x80, f_089F3470, 0x0880B690u);
    *B(0x092E1220u) = 0xFF;
    *W(0x08E7602Cu) = 0;
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x40;
}

/* (): the 256 x 224 picture at 0x092E22E4's address cleared to 0xFFFFFFFF and made a texture
   (0x0951F908: 512 x 256, format 0x900), entered as number 0xFF in the list 0x0951F938, which becomes
   the one-entry list at 0x09358408 and has its copy made; 0x092E22E0 cleared. A 0x30 frame keeping
   s0, s1. */
SFA_FUNC(088AC550, screen_picture_start) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr at = *W(0x092E22E4u);
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = ra;
    for (uint32_t i = 0; i < 224u * 256u; i++) *W(at + i * 4) = 0xFFFFFFFFu;
    c->r[16] = 0x0951F908u;
    c->r[7] = 0x200; c->r[8] = 0x100;
    call3(c, 0x0951F908u, *W(0x092E22E4u), 0x900, f_089E0DE0, 0x088AC5B8u);
    c->r[17] = 0x0951F938u;
    c->r[7] = 0x40800000u;
    call3(c, 0x0951F938u, 0x0951F908u, 0xFF, f_089E1204, 0x088AC5D4u);
    *W(0x09358408u) = 0x0951F938u;
    c->r[16] = 0x09358408u;
    *W(0x0935840Cu) = 1;
    call2(c, 0xFF, 0, f_089E1234, 0x088AC5F4u);
    call1(c, 0x09358408u, f_089E0D54, 0x088AC5FCu);
    *W(0x092E22E0u) = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* Copies three words of a picture list (+48, +80, +108) the way the screen start does. */
static void list_words(gaddr to, gaddr from) {
    *W(to + 48) = *W(from + 48); *W(to + 80) = *W(from + 80); *W(to + 108) = *W(from + 108);
}

/* (): the fight screen's pictures, between 0x088D7EA0 (1) and (0): the 2D library's 0x089E15E8
   (0x09351E00, 0x180), the screen picture at 0x08F58140 (0x088AC550), memory for 0x08A0577C's +160 the
   first time (0x1400 bytes), number 0x18 (0x089E1234), the three fighters' picture lists' words from
   their models, 0x088139CC (1), the current pad's record (0x08E76088, 216 bytes each) cleared at +202
   to +208, the file 0x08A03190 names read (0x088075F8) and waited for (0x08E7606C, one frame at a time
   by 0x088054F0), 0x08E75C48's three halves set to -1 and the palette 0x092E0180 from that memory
   (0x089E11C8), 0x092E0980 = 1. A 0x30 frame keeping s0..s5. */
SFA_FUNC(0881494C, fight_pictures_start) {
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    for (uint32_t i = 0; i < 6; i++) *W(c->r[29] + 16 + 4 * i) = sv[i];
    *W(c->r[29] + 40) = sv[6];
    call1(c, 1, f_088D7EA0, 0x08814974u);
    call2(c, 0x09351E00u, 0x180, f_089E15E8, 0x08814984u);
    *W(0x092E22E4u) = 0x08F58140u;
    call2(c, 0x08F58140u, 0x092E0000u, f_088AC550, 0x08814998u);
    const gaddr a = 0x08A050B0u, a_to = 0x08E75A18u, b = 0x08A0577Cu, wait = 0x08E76068u;
    c->r[19] = a; c->r[18] = a_to; c->r[17] = b; c->r[16] = wait;
    *W(a_to + 160) = *W(a + 160);
    if (*W(b + 160) == 0) {
        call1(c, 0x1400, f_089E04EC, 0x088149D0u);
        *W(b + 160) = c->r[2];
    }
    call2(c, 0x18, 0x40, f_089E1234, 0x088149E0u);
    const gaddr b_to = 0x092D9E00u, d = 0x08A0562Cu, d_to = 0x08E75AD8u;
    *W(b_to + 48) = *W(b + 48);
    uint32_t b80 = *W(b + 80), b108 = *W(b + 108);
    *W(b_to + 80) = b80;
    c->r[20] = d; c->r[21] = d_to;
    list_words(d_to, d);
    list_words(0x092D9D40u, 0x08A056D4u);
    *W(b_to + 108) = b108;
    call1(c, 1, f_088139CC, 0x08814A58u);
    uint32_t d76 = *W(d + 76), a76 = *W(a + 76);
    *W(d_to + 76) = d76;
    uint32_t pad = *B(0x08E76084u);
    *W(a_to + 76) = a76;
    gaddr rec = 0x08E76088u + pad * 216;
    *B(rec + 202) = 0; *B(rec + 203) = 0; *H(rec + 208) = 0; *B(rec + 204) = 0;
    call3(c, *W(0x08A03190u), 0, 0, f_088075F8, 0x08814AB4u);
    while (*B(wait + 4) == 0) call1(c, 1, f_088054F0, 0x08814AC8u);
    *H(0x08E75C4Cu) = 0xFFFF; *H(0x08E75C4Au) = 0xFFFF; *H(0x08E75C48u) = 0xFFFF;
    call3(c, 0x092E0180u, *W(b + 160), 0x100, f_089E11C8, 0x08814B04u);
    *W(0x092E0980u) = 1;
    call1(c, 0, f_088D7EA0, 0x08814B18u);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (): the fight task's next step (GAME + 4 on) with its counters cleared (GAME + 92, + 6 to + 22,
   BATTLE + 40, 0x08B02852), the sprite pictures started, then 0x08821FEC, 0x088B2EFC and 0x0881CBDC. A
   0x10 frame keeping s0, s1. */
SFA_FUNC(088B2E68, fight_step_reset) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; c->r[16] = GAME;
    uint16_t step = *H(GAME + 4);
    *B(GAME + 92) = 0;
    *H(GAME + 4) = (uint16_t)(step + 1);
    *W(c->r[29] + 4) = s1; c->r[17] = 0;
    *H(0x08B02852u) = 0; *H(BATTLE + 40) = 0;
    *H(GAME + 10) = 0; *H(GAME + 8) = 0; *H(GAME + 6) = 0; *H(GAME + 22) = 0; *H(GAME + 20) = 0;
    *H(GAME + 18) = 0; *H(GAME + 16) = 0;
    *W(c->r[29] + 8) = ra;
    *H(GAME + 14) = 0;
    c->r[31] = 0x088B2ED0u; f_0880B57C(c);
    c->r[31] = 0x088B2ED8u; f_08821FEC(c);
    c->r[31] = 0x088B2EE0u; f_088B2EFC(c);
    *H(GAME + 14) = (uint16_t)c->r[17];
    c->r[31] = 0x088B2EE8u; f_0881CBDC(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}
