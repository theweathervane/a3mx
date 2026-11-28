/* Five more: which of fourteen teams is fighting, a fighter beaten with a partner left, the
   attract mode's script a frame at a time, and a partner called in and whether that may happen.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_088A05B4(Cpu *c); void f_08834318(Cpu *c); void f_0883402C(Cpu *c); void f_0888BB24(Cpu *c);
void f_088A9350(Cpu *c); void f_088A9368(Cpu *c); void f_08822934(Cpu *c); void f_08823304(Cpu *c);
void f_0882222C(Cpu *c); void f_08822444(Cpu *c); void f_088A9380(Cpu *c); void f_088A9398(Cpu *c);
void f_0887C708(Cpu *c); void f_088A96B8(Cpu *c); void f_088679BC(Cpu *c); void f_08859650(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* Which of the fourteen teams of 0x08A0E024 (four bytes each) is the cpu's: BATTLE's +154 while
   bit 4 of the game's three bytes is set; otherwise the three (+775) of the fight's kind
   (BATTLE + 56) are looked up: for the kind 1 fighter 1, then 3 and 4 with a 2 moved last; for
   the kind 2 fighters 3 and 4 the same way, then fighter 2. Returns the team's number, 0 when
   none fits. A 0x10 frame with the four bytes at its start. */
SFA_FUNC(08820EB4, cpu_team_number) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *B(sp) = 0xFF; *B(sp + 1) = 0xFF; *B(sp + 2) = 0xFF;
    *W(sp + 4) = ra;
    *B(sp + 3) = 0xFF;
    ARG(0) = 0xFFFFFFFFu;
    call0(c, f_088A05B4, 0x08820ED4u);
    if (c->r[2] != 0) c->r[2] = (uint32_t)(int32_t)S8(BATTLE + 154);
    else {
        int32_t kind = S16(BATTLE + 56);
        int32_t a = S8(FIGHTER_3 + 775), b = S8(FIGHTER_4 + 775);
        if (kind == 1) {
            *B(sp) = (uint8_t)S8(FIGHTER_1 + 775);
            *B(sp + 1) = (uint8_t)(a == 2 ? b : a);
            *B(sp + 2) = (uint8_t)(b == 2 ? a : b);
        } else if (kind == 2) {
            *B(sp) = (uint8_t)(a == 2 ? b : a);
            *B(sp + 1) = (uint8_t)(b == 2 ? a : b);
            *B(sp + 2) = (uint8_t)S8(FIGHTER_2 + 775);
        }
        uint32_t found = 0;
        for (uint32_t i = 0; i < 0xE && !found; i++) {
            int same = 1;
            for (uint32_t j = 0; j < 4 && same; j++)
                if (S8(0x08A0E024u + i * 4 + j) != S8(sp + j)) same = 0;
            if (same) { c->r[2] = i; found = 1; }
        }
        if (!found) c->r[2] = 0;
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (x, y, fighter): a fighter is beaten: its side's bit of x's +42 is set. With y's +70 or +6 set
   or without y's +31, 0x08834318 takes it; otherwise the first frame starts its wait (+341 at 1,
   0x7F at +346, 9 at +348, 1 at +350, +412 cleared) and the later ones are its coming back. */
SFA_FUNC(08833F74, fighter_beaten) {
    gaddr x = ARG(0), y = ARG(1), f = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(x + 42) = (uint8_t)((uint32_t)(int32_t)S8(x + 42) | (1u << ((uint32_t)S8(f + 337) & 31)));
    *W(c->r[29] + 16) = ra;
    c->r[7] = f;
    if (S8(y + 70) != 0 || S8(y + 6) != 0 || S8(y + 31) == 0) call3(c, x, y, f, f_08834318, 0x08833FD0u);
    else if (S8(f + 341) != 0) call3(c, x, y, f, f_0883402C, 0x08833FF4u);
    else {
        *B(f + 341) = 1;
        *H(f + 346) = 0x7F;
        *H(f + 348) = 9;
        *H(f + 350) = 1;
        *B(f + 412) = 0;
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The attract mode's script, a frame at a time: 0x0888BB24 (0) past the frame 0x61E of GAME's
   +14. Its entries of 14 bytes at 0x08A2C26C are taken in turn (the count at 0x08B02764) once the
   low byte of GAME's +16 reaches an entry's first word: the entry's +2 becomes that word's high
   byte; a picture (+4, when not negative) is set up with its +6 halved, +8 as the word at
   0x0951CE52 and +9 and +12 as two bytes, through 0x088A9350, 0x088A9368, 0x08822934, 0x08823304
   and 0x0882222C; a sound (+10, when not negative) with +12, through 0x08822444, 0x088A9380 and
   0x088A9398. At the frame 0x1F the sound thread restarts and GAME's +16 goes to 1; from then on
   0x088A96B8 runs. A frame keeping s0..s2. */
SFA_FUNC(088A9520, attract_script_frame) {
    const gaddr st = 0x08B0274Cu;
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = GAME;
    *W(sp + 20) = s1;
    int32_t t = S16(GAME + 14);
    *W(sp + 24) = s2;
    c->r[17] = st;
    *W(sp + 28) = ra;
    c->r[18] = 0x08A2C26Cu;
    if (t != 0x61E && !(t < 1567)) call1(c, 0, f_0888BB24, 0x088A956Cu);
    uint32_t n = *H(st + 24);
    gaddr e = 0x08A2C26Cu + (uint32_t)((int32_t)(int16_t)n * 14);
    c->r[18] = e;
    if (!((S16(GAME + 16) & 0xFF) < S16(e))) {
        *H(st + 24) = (uint16_t)(n + 1);
        *H(GAME + 16) = (uint16_t)(S16(GAME + 16) & 0xFF);
        *H(GAME + 16) = (uint16_t)(S16(GAME + 16) | (S8(e + 2) << 8));
        if (S16(e + 4) >= 0) {
            *H(st) = (uint16_t)S16(e + 4);
            *H(st + 2) = (uint16_t)(S16(e + 6) >> 1);
            *H(0x0951CE52u) = (uint16_t)(S8(e + 8) << 8);
            *B(st + 93) = (uint8_t)S16(e + 12);
            *B(st + 94) = (uint8_t)S8(e + 9);
            call1(c, st, f_088A9350, 0x088A960Cu);
            call1(c, st, f_088A9368, 0x088A9614u);
            call0(c, f_08822934, 0x088A961Cu);
            call0(c, f_08823304, 0x088A9624u);
            call0(c, f_0882222C, 0x088A962Cu);
        }
        if (S16(e + 10) >= 0) {
            *H(st + 4) = (uint16_t)S16(e + 10);
            *H(st + 6) = (uint16_t)S16(e + 12);
            if (*H(st + 6) != 0) call1(c, *H(st + 6), f_08822444, 0x088A9658u);
            call1(c, st, f_088A9380, 0x088A9660u);
            call1(c, st, f_088A9398, 0x088A9668u);
        }
    }
    if (S16(GAME + 14) == 0x1F) {
        call0(c, f_0887C708, 0x088A9680u);
        *H(GAME + 16) = 1;
    }
    if (!(S16(GAME + 14) < 31)) call0(c, f_088A96B8, 0x088A96A0u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): its partner (+1168) is called in: the fighter's +270 is set and its step is 1, 9,
   0, 0 (+4 to +7); the partner takes its facing (+11) and the step 1, 9, 3 with a last byte 0
   when the fighter is in the state 0x110 (+760) or its new press is the first button, and 1
   otherwise. A 0x30 frame keeping s0..s4. */
SFA_FUNC(08862164, partner_call_in) {
    gaddr f = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 5; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 36) = sv[5];
    gaddr p = *W(f + 1168);
    c->r[20] = p; c->r[19] = 1; c->r[18] = 1; c->r[17] = 9; c->r[16] = 3;
    *B(f + 270) = 1;
    *B(f + 4) = 1;
    *B(f + 5) = 9;
    *B(f + 6) = 0;
    uint32_t state = (uint32_t)*H(f + 760) & 0x3F0;
    *B(f + 7) = 0;
    *B(p + 11) = *B(f + 11);
    uint32_t last = 0;
    if (state != 0x110) {
        c->r[5] = state; c->r[6] = 0x110;
        call0(c, f_088679BC, 0x088621C8u);
        if (c->r[2] != 1) last = 1;
    }
    *B(p + 4) = 1;
    *B(p + 5) = 9;
    *B(p + 6) = 3;
    *B(p + 7) = (uint8_t)last;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (fighter, calling): 1 when the partner may be called in (calling set) or is standing by
   (clear). Only in the modes 0xF and 0x13, the mode 7 with BATTLE's +79 at 5 and the mode 0x10
   with BATTLE's +159 at 3; with nothing holding the fight (BATTLE + 13, + 14, + 36, GAME + 38); a
   partner there with its +112 not -1; and, unless the fighter is in the state 0x110 or 0xA0, one
   of the two buttons newly pressed. Calling (after 0x08859650): the partner's +5 at 0 and its +6
   and +7 one of seven pairs. Standing by: the partner's +5 at 9 and +6 at 2. A frame keeping
   s0..s2. */
SFA_FUNC(08866824, partner_may_come) {
    gaddr f = ARG(0);
    int32_t calling = (int8_t)ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = f;
    *W(sp + 16) = s0;
    int32_t mode = S8(GAME + 114);
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = (uint32_t)calling;
    uint32_t answer = 0;
    int ok = mode == 0xF || mode == 0x13 || (mode == 7 && S8(BATTLE + 79) == 5)
             || (mode == 0x10 && S8(BATTLE + 159) == 3);
    if (ok && S8(BATTLE + 36) == 0 && S8(BATTLE + 13) == 0 && S8(BATTLE + 14) == 0 && S8(GAME + 38) == 0) {
        gaddr p = *W(f + 1168);
        c->r[18] = p;
        if (p != 0 && S16(p + 112) != -1) {
            if (calling != 0) call1(c, f, f_08859650, 0x08866908u);
            uint32_t state = (uint32_t)*H(f + 760) & 0x3F0;
            int go = 1;
            if (state != 0x110 && state != 0xA0) {
                call1(c, f, f_088679BC, 0x0886692Cu);
                if (c->r[2] == 0) go = 0;
            }
            if (go) {
                uint32_t a = *B(p + 6), b = *B(p + 7);
                if (calling == 0) answer = *B(p + 5) == 9 && a == 2;
                else if (*B(p + 5) == 0)
                    answer = (a == 0 && (b == 1 || b == 2)) || (a == 1 && (b == 1 || b == 3))
                             || (a == 2 && b == 1) || (a == 4 && (b == 0 || b == 1));
            }
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
