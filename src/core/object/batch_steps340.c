/* Twelve that the last ones set free: a round's end by its screen, the cpu's two choices put to
   use, a pause's start and end, the attract mode's start, the link's start, and a fighter set up
   again for the link. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define NETP      0x09523980u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_088249A0(Cpu *c); void f_08826EC4(Cpu *c); void f_088D5E44(Cpu *c); void f_088284DC(Cpu *c);
void f_0882872C(Cpu *c); void f_08809BC8(Cpu *c); void f_08823304(Cpu *c); void f_0880A39C(Cpu *c);
void f_08812DDC(Cpu *c); void f_08821F9C(Cpu *c); void f_08809FC8(Cpu *c); void f_088325DC(Cpu *c);
void f_08823338(Cpu *c); void f_0882DD5C(Cpu *c); void f_08809BE4(Cpu *c); void f_08809FE8(Cpu *c);
void f_088099D4(Cpu *c); void f_088D2B54(Cpu *c); void f_08822B88(Cpu *c); void f_0882DB4C(Cpu *c);
void f_0880A004(Cpu *c); void f_088D1564(Cpu *c); void f_08804A74(Cpu *c); void f_0880B57C(Cpu *c);
void f_08821FEC(Cpu *c); void f_088A9328(Cpu *c); void f_088157E4(Cpu *c); void f_0882E0D0(Cpu *c);
void f_089E0B64(Cpu *c); void f_0880AA74(Cpu *c); void f_08822600(Cpu *c); void f_08805564(Cpu *c);
void f_088054F0(Cpu *c); void f_08805614(Cpu *c); void f_088D7DB8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}
static void enter3(Cpu *c, uint32_t sv[4]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[18]; sv[3] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    *W(c->r[29] + 28) = sv[3];
}
static void leave3(Cpu *c, const uint32_t sv[4]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = sv[3]; c->r[29] += 0x20;
}
/* The screen goes black with nothing let through (0xFF000000 three times). */
static void black(Cpu *c, uint32_t back) {
    ARG(0) = 0xFF000000u; ARG(1) = 0xFF000000u; ARG(2) = 0xFF000000u;
    c->r[31] = back; f_089E0B64(c);
}

/* (x, y): a round's end by its screen: 0x088249A0 with x's +38 at 1; nothing with +39 set; the
   screen's start (0x08826EC4) before the fight is over (y + 5), with x's +69 set, in the mode 7,
   and for a player's fights (0x088D5E44) below the eighth (y + 59); the others by the table
   0x08A0E298. A frame keeping s0, s1. */
SFA_FUNC(08826DE0, round_end_by_screen) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    int32_t first = S8(x + 38);
    enter2(c, sv);
    c->r[17] = x; c->r[16] = y;
    if (first == 1) call2(c, x, y, f_088249A0, 0x08826E10u);
    else if (S8(x + 39) == 0) {
        if (S8(y + 5) == 0 || S8(x + 69) != 0 || S8(x + 114) == 7)
            call2(c, x, y, f_08826EC4, 0x08826E58u);
        else {
            call2(c, (uint32_t)(int32_t)S8(x + 114), 7, f_088D5E44, 0x08826E68u);
            int32_t n = S8(y + 59);
            if (c->r[2] != 0 && n < 8) call2(c, x, y, f_08826EC4, 0x08826E88u);
            else {
                uint32_t fn = *W(0x08A0E298u + ((uint32_t)n << 2));
                c->r[6] = fn;
                ARG(0) = x; ARG(1) = y; c->r[31] = 0x08826EB0u;
                rt_call_indirect(c, fn);
            }
        }
    }
    leave2(c, sv);
}

/* One fighter's +420 takes its +112 and v, 0x90 at most. */
static int32_t gauge_sum(gaddr f, uint32_t v) {
    int32_t t = (int16_t)((uint32_t)(int32_t)S16(f + 112) + v);
    return t < 144 ? t : 0x90;
}

/* (a, y): once the fight is over without a draw (y + 5, + 76), the winner (y + 112) gets the
   cpu's first choice (0x088284DC) added to its +420: a player always, the cpu in the modes 4 and
   5 unless its +337 has bit 2. In those modes its partner (fighter 1 or 2, by its +337) gets the
   same, with -1 kept as 0. A 0x30 frame keeping s0..s2 with the six bytes at sp + 16. */
SFA_FUNC(088283B0, winner_gauge_a) {
    gaddr y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    int32_t over = S8(y + 5);
    *W(sp + 24) = s0; *W(sp + 28) = s1; *W(sp + 32) = s2; *W(sp + 36) = ra;
    c->r[16] = y;
    if (over != 0 && S8(y + 76) == 0) {
        gaddr winner = *W(y + 112);
        c->r[18] = winner;
        int go = 1;
        c->r[17] = 4;
        if (S8(winner + 373) != 0) {
            int32_t mode = S8(y + 79);
            if (mode != 4 && mode != 5) go = 0;
            else if (S8(winner + 337) & 2) go = 0;
        }
        if (go) {
            ARG(1) = y; ARG(2) = sp + 0x10; ARG(3) = winner;
            c->r[31] = 0x08828420u; f_088284DC(c);
            uint32_t v = ((((uint32_t)*B(sp + 19) << 8) & 0xFFFF) | *B(sp + 20)) & 0xFFFF;
            *H(winner + 420) = (uint16_t)gauge_sum(winner, v);
            int32_t mode = S8(y + 79);
            if (mode == 4 || mode == 5) {
                gaddr other = S8(winner + 337) != 0 ? FIGHTER_1 : FIGHTER_2;
                int32_t t = (int16_t)((uint32_t)(int32_t)S16(other + 112) + v);
                if (!(t < 144)) t = 0x90;
                else if (t == -1) t = 0;
                *H(other + 420) = (uint16_t)t;
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, y): without a draw, the winner gets the cpu's second choice (0x0882872C, its six bytes at
   0x08B01F2C) added to its +420, read as two decimal digits and 0x90 at most; unless its +441 is
   set its +422 takes its +366. A frame keeping s0, s1. */
SFA_FUNC(08828674, winner_gauge_b) {
    uint32_t a = ARG(0);
    gaddr y = ARG(1);
    const gaddr out = 0x08B01F2Cu;
    uint32_t sv[3];
    int32_t draw = S8(y + 76);
    enter2(c, sv);
    if (draw == 0) {
        gaddr winner = *W(y + 112);
        c->r[17] = out; c->r[16] = winner;
        ARG(0) = a; ARG(1) = y; ARG(2) = out; ARG(3) = winner;
        c->r[31] = 0x088286BCu; f_0882872C(c);
        uint32_t v = ((((uint32_t)*B(out + 3) << 8) & 0xFFFF) | *B(out + 4)) & 0xFFFF;
        uint32_t t = ((v >> 4) * 10 + (v & 0xF) + (uint32_t)(int32_t)S16(winner + 112)) & 0xFFFF;
        if (!((int32_t)t < 144)) t = 0x90;
        *H(winner + 420) = (uint16_t)t;
        if (S8(winner + 441) == 0) *H(winner + 422) = (uint16_t)S16(winner + 366);
    }
    leave2(c, sv);
}

/* (.., y): y's +36 counts down, and at 0 the tempo's other way goes to 0x1E. */
SFA_FUNC(088288B8, count_36_then_tempo) {
    gaddr y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t t = S8(y + 36) - 1;
    *B(y + 36) = (uint8_t)t;
    *W(c->r[29] + 16) = ra;
    if ((int8_t)t == 0) call1(c, 0x1E, f_08809BC8, 0x088288E4u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): 0x08823304 and 0x0880A39C; then with y's +159 set the fight is left for the next
   screen: 0x08812DDC (0x12), x's +2 moves on with +4 cleared, y's +105 takes the byte at
   0x08A058E1, the fight's flags and the four fighters' +1 are cleared, and 0x08821F9C runs.
   Without it the sound thread's bit 8 and 0x088325DC. A frame keeping s0, s1. */
SFA_FUNC(088307A0, fight_to_next_screen) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = y; c->r[17] = x;
    call0(c, f_08823304, 0x088307BCu);
    call0(c, f_0880A39C, 0x088307C4u);
    if (S8(y + 159) != 0) {
        call1(c, 0x12, f_08812DDC, 0x088307D8u);
        *H(x + 4) = 0;
        *H(x + 2) = (uint16_t)(*H(x + 2) + 1);
        *B(y + 105) = (uint8_t)S8(0x08A058A0u + 65);
        *B(x + 56) = 1;
        *B(y + 59) = 0;
        *B(y + 45) = 0;
        *B(y + 36) = 0;
        *B(y + 15) = 0;
        *B(x + 125) = 0;
        *B(x + 190) = 1;
        *B(FIGHTER_4 + 1) = 0;
        *B(FIGHTER_3 + 1) = 0;
        *B(FIGHTER_2 + 1) = 0;
        *B(FIGHTER_1 + 1) = 0;
        call2(c, FIGHTER_1, 1, f_08821F9C, 0x0883084Cu);
    } else {
        call0(c, f_08809FC8, 0x0883085Cu);
        call2(c, x, y, f_088325DC, 0x08830868u);
    }
    leave2(c, sv);
}

/* (a, .., p): a pause starts: p's first byte moves on, its +1 to +3 and +8 are cleared and +10
   set. With both of 0x08823338 (3, 0) and (3, 1) agreeing: 0x0882DD5C, the music's tempo 0x1E,
   the sound thread's bit 0x10, the callback tables' reset and 0x088D2B54. Otherwise it is called
   off: p's first byte and +10, a's +89 and the word at 0x08B01F4E are cleared. */
SFA_FUNC(08834A04, pause_start) {
    gaddr a = ARG(0), p = ARG(2);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = p;
    c->r[17] = a;
    *B(p) = (uint8_t)(*B(p) + 1);
    *B(p + 3) = 0; *B(p + 2) = 0; *B(p + 1) = 0;
    *B(p + 8) = 0;
    *B(p + 10) = 1;
    int ok = 0;
    call2(c, 3, 0, f_08823338, 0x08834A4Cu);
    if (c->r[2] != 0) {
        call2(c, 3, 1, f_08823338, 0x08834A5Cu);
        if (c->r[2] != 0) ok = 1;
    }
    if (ok) {
        call0(c, f_0882DD5C, 0x08834A6Cu);
        call1(c, 0x1E, f_08809BE4, 0x08834A74u);
        call0(c, f_08809FE8, 0x08834A7Cu);
        call0(c, f_088099D4, 0x08834A84u);
        call0(c, f_088D2B54, 0x08834A8Cu);
    } else {
        *B(p) = 0;
        *B(p + 10) = 0;
        *B(a + 89) = 0;
        *H(0x08B01F4Eu) = 0;
    }
    leave2(c, sv);
}

/* (a, .., p): a pause ends: 0x08822B88, p's +10 and a's +89 cleared, 0x0882DB4C, the sound
   thread's bit 0x10 cleared and the tempo's other way at 0x1E. */
SFA_FUNC(08834FA4, pause_end) {
    gaddr a = ARG(0), p = ARG(2);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = p;
    c->r[17] = a;
    call0(c, f_08822B88, 0x08834FC0u);
    *B(p + 10) = 0;
    *B(a + 89) = 0;
    call0(c, f_0882DB4C, 0x08834FCCu);
    call0(c, f_0880A004, 0x08834FD4u);
    call1(c, 0x1E, f_08809BC8, 0x08834FDCu);
    leave2(c, sv);
}

/* The step the byte at 0x0950FFA0 names in the table 0x08A0F9A8, then the step by +128 of each of
   the 32 objects of 0x94 bytes from 0x092E2B88 that is in use (+90) with the kind 0x1A (+91). A
   frame keeping s0..s2. */
SFA_FUNC(0883584C, step_then_objects_1a) {
    uint32_t sv[4];
    uint32_t fn = *W(0x08A0F9A8u + ((uint32_t)*B(0x0950FFA0u) << 2));
    c->r[6] = 0x0950FFA0u; c->r[7] = fn; c->r[8] = 0x08A0F9A8u;
    enter3(c, sv);
    c->r[31] = 0x08835888u; rt_call_indirect(c, fn);
    c->r[16] = 0x1A;
    gaddr o = 0x092E2B88u;
    for (uint32_t i = 0; i < 32; i++, o += 0x94) {
        c->r[17] = i; c->r[18] = o;
        if (*B(o + 90) != 0 && *B(o + 91) == 0x1A) call1(c, o, f_088D1564, 0x088358B8u);
    }
    leave3(c, sv);
}

/* The attract mode's start: 0x08804A74 (1), GAME's +4 moves on with its counters cleared, the
   word at BATTLE + 40 and the one at 0x08B02764, 0x0880B57C, 0x08821FEC and 0x088A9328, the bytes
   at 0x092E889B and 0x092E893B set, seven times the word at 0x08A2C1A8 to 0x08B02840 (0 at
   0x08B02844), and the lists 0x09358408 and 0x08A054C0. A frame keeping s0..s2. */
SFA_FUNC(088A9240, attract_start) {
    uint32_t sv[4];
    enter3(c, sv);
    c->r[16] = 0; c->r[17] = 1;
    call1(c, 1, f_08804A74, 0x088A9264u);
    c->r[18] = GAME;
    *B(GAME + 92) = 0;
    *H(GAME + 4) = (uint16_t)(*H(GAME + 4) + 1);
    *H(0x08B0274Cu + 24) = 0;
    *H(BATTLE + 40) = 0;
    *H(GAME + 10) = 0; *H(GAME + 8) = 0; *H(GAME + 6) = 0; *H(GAME + 22) = 0;
    *H(GAME + 20) = 0; *H(GAME + 16) = 0; *H(GAME + 14) = 0;
    call1(c, BATTLE, f_0880B57C, 0x088A92B4u);
    call0(c, f_08821FEC, 0x088A92BCu);
    call0(c, f_088A9328, 0x088A92C4u);
    *H(GAME + 14) = 0;
    *W(0x08B00000u + 10308) = 0;
    *B(0x092E8800u + 155) = 1;
    *B(0x092E88A0u + 155) = 1;
    uint32_t v = (uint32_t)(int32_t)S16(0x08A2C1A8u);
    *W(0x08B00000u + 10304) = v * 7;
    ARG(0) = v * 7; ARG(1) = 0x08B00000u; ARG(2) = 0x092E8800u;
    c->r[31] = 0x088A9310u; f_088157E4(c);
    leave3(c, sv);
}

/* (x): x's +4 moves on, the sound thread's bit 8, the callback tables' reset, 0x0882E0D0, the
   screen black and 0x0880AA74. */
SFA_FUNC(088BE0CC, step_on_and_black) {
    gaddr x = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    *H(x + 4) = (uint16_t)(*H(x + 4) + 1);
    c->r[5] = *H(x + 4);
    call0(c, f_08809FC8, 0x088BE0E4u);
    call0(c, f_088099D4, 0x088BE0ECu);
    call0(c, f_0882E0D0, 0x088BE0F4u);
    black(c, 0x088BE104u);
    call0(c, f_0880AA74, 0x088BE10Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (game): the link's start: the sound thread's bit 8, the callback tables' reset, 0x08821F9C,
   0x08822600, 0x08805564 (2), the screen black, the mode 0xC, 0x08804A74 (0); then frames pass
   (0x088054F0) until NETP's +264 is clear and the byte at 0x08E7606C set, and 0x08805614 takes
   0x088D8128. A frame keeping s0, s1. */
SFA_FUNC(088D7F48, link_start) {
    gaddr g = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = g;
    call0(c, f_08809FC8, 0x088D7F60u);
    call0(c, f_088099D4, 0x088D7F68u);
    call0(c, f_08821F9C, 0x088D7F70u);
    call0(c, f_08822600, 0x088D7F78u);
    call1(c, 2, f_08805564, 0x088D7F80u);
    black(c, 0x088D7F90u);
    *B(g + 114) = 0xC;
    call1(c, 0, f_08804A74, 0x088D7FA0u);
    c->r[17] = NETP;
    c->r[16] = 0x08E76068u;
    while (S8(NETP + 264) != 0 || *B(0x08E76068u + 4) == 0) call1(c, 1, f_088054F0, 0x088D7FD0u);
    call1(c, 0x088D8128u, f_08805614, 0x088D7FE0u);
    leave2(c, sv);
}

/* (fighter, flag): a fighter set up again for the link: its counters cleared, its character
   (+338) and +386 from +448 and +450, +420 at 0x90, +340 at 2; the cpu's mark (+373) with the
   flag and its +775 at 2; and with the link at 2 or 3 and GAME's +22 at 1, its +374, +426 and
   +451 cleared. A 0x10 frame keeping s0, s1. */
SFA_FUNC(088E938C, link_fighter_again) {
    gaddr f = ARG(0);
    uint32_t flag = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0;
    c->r[16] = f;
    *B(f + 432) = 0;
    *B(f + 430) = 0;
    *W(sp + 4) = s1;
    c->r[17] = 0;
    *B(f + 356) = 0;
    *B(f + 373) = 0;
    *H(f + 404) = 0;
    *W(f + 400) = 0;
    *W(f + 352) = 0;
    *B(f + 338) = (uint8_t)S8(f + 448);
    *B(f + 386) = (uint8_t)S8(f + 450);
    *H(f + 420) = 0x90;
    *H(f + 422) = 0;
    *H(f + 424) = 0;
    *B(f + 412) = 0;
    *B(f + 336) = 0;
    *B(f + 340) = 2;
    *W(sp + 8) = ra;
    *B(f + 341) = 0;
    if (flag != 0 && S8(f + 775) == 2) *B(f + 373) = 1;
    c->r[31] = 0x088E941Cu; f_088D7DB8(c);
    if (c->r[2] != 0 && S16(GAME + 22) == 1) {
        *B(f + 451) = 0;
        *H(f + 426) = 0;
        *B(f + 374) = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}
