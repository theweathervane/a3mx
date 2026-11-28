/* A player's next opponent by mode, a fight's start and its frame through the table of steps, and
   a small script reader's three parts. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_0882F194(Cpu *c); void f_0882FFE4(Cpu *c); void f_08831A78(Cpu *c); void f_0882FEB8(Cpu *c);
void f_08821FC8(Cpu *c); void f_088317B4(Cpu *c); void f_088967C4(Cpu *c); void f_0881CEF4(Cpu *c);
void f_08819144(Cpu *c); void f_088969BC(Cpu *c); void f_08812DDC(Cpu *c); void f_08815D60(Cpu *c);
void f_088131E0(Cpu *c); void f_08814D94(Cpu *c); void f_08819984(Cpu *c); void f_088D7C1C(Cpu *c);
void f_08837690(Cpu *c); void f_0883771C(Cpu *c); void f_08887AFC(Cpu *c); void f_0880B6CC(Cpu *c);
void f_088961DC(Cpu *c); void f_089DF87C(Cpu *c); void f_089DF91C(Cpu *c); void f_089DF97C(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (a, b, fighter, g): a player's next opponent (nothing for the cpu): its side's place for the
   next fight (0x0882F194), then by the mode (b + 79): 7, 8 and 10 the list's next entry
   (0x0882FFE4) and 0x08831A78 (b, 0); 6 the same with fighter 3's character; the others the first
   the list allows (0x0882FEB8). A 0x30 frame keeping s0..s3. */
SFA_FUNC(0882FD70, next_opponent_by_mode) {
    gaddr a = ARG(0), b = ARG(1), f = ARG(2);
    uint32_t g = ARG(3);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    int32_t cpu = S8(f + 373);
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3];
    c->r[19] = a; c->r[18] = b; c->r[17] = f;
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4];
    c->r[16] = g;
    if (cpu == 0) {
        ARG(3) = g;
        call3(c, a, b, f, f_0882F194, 0x0882FDB4u);
        int32_t mode = S8(b + 79);
        if (mode == 7 || mode == 8 || mode == 0xA) {
            call2(c, a, b, f_0882FFE4, 0x0882FDE0u);
            call2(c, b, 0, f_08831A78, 0x0882FDECu);
        } else if (mode == 6) {
            call2(c, a, b, f_0882FFE4, 0x0882FE08u);
            call2(c, b, (uint32_t)(int32_t)S8(FIGHTER_3 + 338), f_08831A78, 0x0882FE1Cu);
        } else {
            ARG(3) = g;
            call3(c, a, b, f, f_0882FEB8, 0x0882FE34u);
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* The three parts of the screen every frame of a fight. */
static void screen_parts(Cpu *c, uint32_t back) {
    call0(c, f_08837690, back);
    call0(c, f_0883771C, back + 8);
    call0(c, f_08887AFC, back + 16);
}

/* (x, y): a fight. The first time (x + 8): the fight's counters are cleared (x + 10, + 20, + 22,
   + 43, + 72, y + 81), 0x08821FC8 runs, the fights through lists (y + 155) take their first
   fighters, the stage is set up (0x088967C4, 0x0881CEF4 with y's +102, 0x08819144, 0x088969BC with
   eleven times the stage plus 0x101), and unless x's +38 is set 0x08812DDC (0x13) runs, the four
   fighters get their own blocks (+416) and the fighters of the fight's kind (y + 56) are started:
   0 by 0x08815D60, the others by 0x088131E0 with the three taking part; then 0x08814D94 (0) and
   0x08819984 (0). Afterwards the step from the table 0x08A27318 (by x + 10), and the screen's
   parts unless 0x088D7C1C says so while x's +145 is set. 0x0880B6CC ends both. A frame keeping
   s0, s1. */
SFA_FUNC(088961DC, fight_frame) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = x;
    uint32_t started = *H(x + 8);
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = y;
    if (started != 0) {
        uint32_t fn = *W(0x08A27318u + ((uint32_t)*H(x + 10) << 2));
        ARG(0) = x; ARG(1) = y; c->r[6] = fn; c->r[31] = 0x088963ACu;
        rt_call_indirect(c, fn);
        call0(c, f_088D7C1C, 0x088963B4u);
        if (c->r[2] == 0) screen_parts(c, 0x088963F0u);
        else if (S8(x + 145) == 0) screen_parts(c, 0x088963D0u);
    } else {
        *H(x + 8) = 1;
        *H(x + 20) = 0;
        *B(y + 81) = 0;
        *B(x + 72) = 0;
        *B(x + 43) = 0;
        *H(x + 22) = 0; *H(x + 20) = 0; *H(x + 10) = 0;
        call1(c, 0, f_08821FC8, 0x0889622Cu);
        if (S8(y + 155) != 0) call2(c, x, y, f_088317B4, 0x08896240u);
        else ARG(0) = x;
        call0(c, f_088967C4, 0x08896248u);
        call2(c, (uint32_t)(int32_t)S16(y + 102), 1, f_0881CEF4, 0x08896254u);
        call0(c, f_08819144, 0x0889625Cu);
        uint32_t stage = (uint32_t)(int32_t)S16(BATTLE + 102);
        call3(c, stage * 11 + 0x101, 0x14, 0xA, f_088969BC, 0x08896288u);
        if (S8(x + 38) == 0) {
            call1(c, 0x13, f_08812DDC, 0x0889629Cu);
            *W(FIGHTER_1 + 416) = 0x0951CE68u;
            *W(FIGHTER_2 + 416) = 0x0951D910u;
            *W(FIGHTER_3 + 416) = 0x0951E3B8u;
            *W(FIGHTER_4 + 416) = 0x0951EE60u;
            int32_t kind = S16(y + 56);
            c->r[16] = (uint32_t)kind;
            if (kind == 0) {
                ARG(0) = FIGHTER_4; ARG(1) = FIGHTER_3; ARG(2) = FIGHTER_4; ARG(3) = FIGHTER_1;
                call0(c, f_08815D60, 0x08896314u);
            } else if (kind == 1) call3(c, FIGHTER_1, FIGHTER_3, FIGHTER_4, f_088131E0, 0x08896344u);
            else if (kind == 2) call3(c, FIGHTER_2, FIGHTER_3, FIGHTER_4, f_088131E0, 0x08896358u);
            else if (kind == 3) call3(c, FIGHTER_1, FIGHTER_2, FIGHTER_3, f_088131E0, 0x08896370u);
            call1(c, 0, f_08814D94, 0x08896378u);
            call1(c, 0, f_08819984, 0x08896380u);
        }
    }
    call0(c, f_0880B6CC, 0x08896408u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The same behind a frame of its own. */
SFA_FUNC(088243E8, fight_frame_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    call0(c, f_088961DC, 0x088243F8u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, script): the script's next entry is read from its place (+60): a wait (+64), a word (+66)
   whose low four bits say how many more words follow, and those words (to +68 on); a's +6 moves on
   and the wait starts (0x089DF91C). */
SFA_FUNC(089DF87C, script_read) {
    gaddr a = ARG(0), s = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(s + 60);
    *H(s + 64) = (uint16_t)S16(p);
    *H(s + 66) = (uint16_t)S16(p + 2);
    p += 4;
    *W(c->r[29] + 16) = ra;
    gaddr to = s + 0x44;
    for (int32_t i = 0; i < (S16(s + 66) & 0xF); i++) {
        *H(to) = (uint16_t)S16(p);
        p += 2; to += 2;
    }
    *W(s + 60) = p;
    *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
    c->r[6] = s;
    call2(c, a, s, f_089DF91C, 0x089DF910u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, script): unless a's +93 is set the wait counts down, and when it is over a's +6 moves on
   and the entry runs (0x089DF97C). */
SFA_FUNC(089DF91C, script_wait) {
    gaddr a = ARG(0), s = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t held = S8(a + 93);
    *W(c->r[29] + 16) = ra;
    if (held == 0) {
        int32_t t = S16(s + 64) - 1;
        *H(s + 64) = (uint16_t)t;
        if (!((int16_t)t > 0)) {
            *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
            c->r[6] = s;
            call2(c, a, s, f_089DF97C, 0x089DF970u);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, script): the entry's own routine, from the table 0x08AB79E0 by two bits of its word; then,
   unless the script's +101 is set, a's +6 goes back to 1 and the next entry is read; with it set
   a's +6 moves on. A frame keeping s0, s1. */
SFA_FUNC(089DF97C, script_run) {
    gaddr a = ARG(0), s = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = s;
    *W(sp + 20) = s1; c->r[17] = a;
    uint32_t fn = *W(0x08AB79E0u + (((uint32_t)(S16(s + 66) >> 4) & 3) << 2));
    *W(sp + 24) = ra;
    ARG(0) = a; ARG(1) = s; c->r[6] = fn; c->r[31] = 0x089DF9C0u;
    rt_call_indirect(c, fn);
    if (S8(s + 101) == 0) {
        *H(a + 6) = 1;
        call2(c, a, s, f_089DF87C, 0x089DF9DCu);
    } else *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
