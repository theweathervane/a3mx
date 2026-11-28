/* What the fade colour set free: four steps of two characters' flashes, a screen's start, the
   link's fight set up again, the flash every frame of a fight, and two of the cpu's choices of a
   move. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define NETP      0x09523980u
#define PAIR      0x095179E0u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_0882E138(Cpu *c); void f_08861848(Cpu *c); void f_0886E3CC(Cpu *c); void f_0886B68C(Cpu *c);
void f_0882E1A0(Cpu *c); void f_0886B164(Cpu *c); void f_08821368(Cpu *c); void f_08821F58(Cpu *c);
void f_0882E264(Cpu *c); void f_08836538(Cpu *c); void f_0883680C(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* The white flash: the step (+7) moves on, the fight goes off in white and 0x08861848 runs. */
static void flash_white(Cpu *c, gaddr o, uint32_t back) {
    call1(c, o, f_0882E138, back);
    call1(c, o, f_08861848, back + 8);
}

/* The flash's end: both sides shake (0x0886E3CC with 9, 4), the step moves on, the move `move`
   starts, the fight comes back in black, 0x08861848 runs, the other's +698 is set and the sound
   `sound` plays. */
static void flash_end(Cpu *c, gaddr o, uint32_t move, uint32_t sound, uint32_t back) {
    call3(c, *W(o + 92), 9, 4, f_0886E3CC, back);
    call3(c, o, 9, 4, f_0886E3CC, back + 0x10);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, move, f_0886B68C, back + 0x28);
    call1(c, o, f_0882E1A0, back + 0x30);
    call1(c, o, f_08861848, back + 0x38);
    *B(*W(o + 92) + 698) = 1;
    call2(c, o, sound, f_0886B164, back + 0x50);
}

/* One hit of the flash: one of two moves by a random bit (s1), and both sides shake. */
static void flash_hit(Cpu *c, gaddr o, uint32_t even, uint32_t odd, uint32_t back) {
    c->r[17] = even;
    c->r[31] = back; f_08821368(c);
    if (c->r[2] & 1) c->r[17] = odd;
    call2(c, o, c->r[17], f_0886B68C, back + 0x18);
    call3(c, *W(o + 92), 9, 4, f_0886E3CC, back + 0x28);
    call3(c, o, 9, 4, f_0886E3CC, back + 0x38);
}

/* (object): the count at +832 runs down; at 0 it goes to 0x3C and the white flash starts. */
SFA_FUNC(08965204, flash_wait_832) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    uint32_t t = *W(o + 832) - 1;
    enter(c, o, sv);
    *W(o + 832) = t;
    if (t == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 832) = 0x3C;
        flash_white(c, o, 0x08965240u);
    }
    leave(c, sv);
}

/* (object): the same with the count in the byte at +0x339. */
SFA_FUNC(088FA150, flash_wait_339) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t t = S8(o + 0x339) - 1;
    *B(o + 0x339) = (uint8_t)t;
    if ((int8_t)t == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 0x339) = 0x3C;
        flash_white(c, o, 0x088FA19Cu);
    }
    leave(c, sv);
}

/* (object): the flash itself: while the count at +832 runs, every fifth frame (+82) one hit (the
   moves 0x5F or 0x5E); at 0 its end with the move 0x5C and the sound 0x5B. */
SFA_FUNC(08965258, flash_run_832) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    uint32_t t = *W(o + 832) - 1;
    enter2(c, o, sv);
    *W(o + 832) = t;
    if (t == 0) flash_end(c, o, 0x5C, 0x5B, 0x0896528Cu);
    else {
        *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
        if (S16(o + 82) == 5) {
            *H(o + 82) = 0;
            ARG(0) = 5; ARG(1) = 5;
            flash_hit(c, o, 0x5F, 0x5E, 0x0896530Cu);
        }
    }
    leave2(c, sv);
}

/* (object): the same with the count in the byte at +0x339, a hit every fourth frame (the moves
   0x6A or 0x69), and the end with the move 0x66 and the sound 0x5D. */
SFA_FUNC(088FA1B4, flash_run_339) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    int32_t t = S8(o + 0x339) - 1;
    enter2(c, o, sv);
    *B(o + 0x339) = (uint8_t)t;
    if ((int8_t)t == 0) flash_end(c, o, 0x66, 0x5D, 0x088FA1F8u);
    else {
        *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
        *H(o + 82) = (uint16_t)(S16(o + 82) & 3);
        if (S16(o + 82) == 0) {
            ARG(0) = 0; ARG(1) = o + 0x334;
            flash_hit(c, o, 0x6A, 0x69, 0x088FA278u);
        }
    }
    leave2(c, sv);
}

/* (x, y): a screen starts: x's +6 moves on, its counters (+8, +10, +18, +20, +22) are cleared,
   y's +15 and +42 and x's +125 are set, and x's +47 takes the byte at 0x09523C98 in the mode 7,
   or with x's +137 and y's +5 both set. Then 0x08821F58 and the colour black. */
SFA_FUNC(08826EC4, screen_start_black) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *H(x + 22) = 0;
    *H(x + 6) = (uint16_t)(*H(x + 6) + 1);
    *H(x + 20) = 0; *H(x + 18) = 0; *H(x + 10) = 0; *H(x + 8) = 0;
    *B(y + 42) = 1;
    *B(y + 15) = 1;
    int32_t mode = S8(x + 114);
    *B(x + 125) = 1;
    *W(c->r[29] + 16) = ra;
    if (mode == 7) {
        c->r[6] = 7;
        c->r[5] = (uint32_t)(int32_t)S8(0x09523C98u);
        *B(x + 47) = (uint8_t)c->r[5];
    } else {
        c->r[6] = (uint32_t)(int32_t)S8(x + 137);
        if (c->r[6] != 0) {
            int32_t over = S8(y + 5);
            c->r[5] = 0x09520000u;
            if (over != 0) {
                c->r[5] = (uint32_t)(int32_t)S8(0x09523C98u);
                *B(x + 47) = (uint8_t)c->r[5];
            }
        }
    }
    c->r[31] = 0x08826F40u; f_08821F58(c);
    call1(c, 0, f_0882E264, 0x08826F48u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): the link's fight set up again: x's counters (+2 to +22) are cleared, y's +15 and +81
   and x's +69 and +72, x's +41 and +57 take its +53, +43 is set, the fight's kind (y + 56) comes
   from NETP + 52 and x's +45 and +73 from NETP + 48. Then 0x08836538, and 0x0883680C for the
   kinds past 0. A 0x10 frame keeping s0. */
SFA_FUNC(088E95DC, link_fight_again) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *H(x + 22) = 0; *H(x + 20) = 0; *H(x + 18) = 0; *H(x + 16) = 0; *H(x + 14) = 0;
    *H(x + 10) = 0; *H(x + 8) = 0; *H(x + 6) = 0; *H(x + 4) = 0;
    *W(c->r[29]) = s0;
    c->r[16] = y;
    *H(x + 2) = 0;
    *B(y + 15) = 0;
    *B(y + 81) = 0;
    *B(x + 72) = 0;
    *B(x + 69) = 0;
    int32_t v = S8(x + 53);
    *B(x + 43) = 1;
    *B(x + 41) = (uint8_t)v;
    *B(x + 57) = (uint8_t)v;
    int32_t kind = S16(NETP + 52);
    *H(y + 56) = (uint16_t)kind;
    int32_t w = S8(NETP + 48);
    *W(c->r[29] + 4) = ra;
    *B(x + 45) = (uint8_t)w;
    *B(x + 73) = (uint8_t)w;
    c->r[5] = (uint32_t)w; c->r[6] = (uint32_t)kind;
    c->r[31] = 0x088E9658u; f_08836538(c);
    if (S16(y + 56) != 0) { ARG(0) = (uint32_t)(int32_t)S16(y + 56); c->r[31] = 0x088E966Cu; f_0883680C(c); }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (.., y): the flash a fighter asks for (its +590): it starts when none runs (y + 96) and y's +36
   is clear: +36 goes to -1, +43 to 0xB, the screen white for 0xF frames (+97); when they are over
   +36, +96 and the four fighters' asks are cleared and the screen is black again. A 0x30 frame
   keeping s0..s4. */
SFA_FUNC(0882E28C, fight_flash) {
    gaddr y = ARG(1);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4]; *W(sp + 28) = sv[3]; *W(sp + 24) = sv[2]; *W(sp + 20) = sv[1];
    c->r[20] = FIGHTER_1; c->r[19] = FIGHTER_2; c->r[18] = FIGHTER_3; c->r[17] = FIGHTER_4;
    uint32_t asked = (uint32_t)((int32_t)S8(FIGHTER_3 + 590) | S8(FIGHTER_4 + 590)
                                | S8(FIGHTER_1 + 590) | S8(FIGHTER_2 + 590));
    *W(sp + 16) = sv[0]; *W(sp + 36) = sv[5];
    c->r[16] = y;
    if (asked != 0) {
        int run = 1;
        if (S8(y + 96) == 0) {
            if (S8(y + 36) != 0) run = 0;
            else {
                *B(y + 36) = 0xFF;
                *B(y + 43) = 0xB;
                *B(y + 96) = (uint8_t)asked;
                *B(y + 97) = 0xF;
                call1(c, 0xFF, f_0882E264, 0x0882E324u);
            }
        }
        if (run) {
            int32_t t = S8(y + 97) - 1;
            *B(y + 97) = (uint8_t)t;
            if ((int8_t)t < 0) {
                *B(y + 36) = 0;
                *B(y + 96) = 0;
                *B(FIGHTER_1 + 590) = 0;
                *B(FIGHTER_2 + 590) = 0;
                *B(FIGHTER_3 + 590) = 0;
                *B(FIGHTER_4 + 590) = 0;
                call1(c, 0, f_0882E264, 0x0882E360u);
            }
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* The frame both choices share: a 0x20 frame keeping s0..s4 from the stack's start. */
static void choice_enter(Cpu *c, uint32_t sv[6]) {
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x20;
    for (int i = 0; i < 5; i++) *W(c->r[29] + 4 * i) = sv[i];
    *W(c->r[29] + 20) = sv[5];
}
static void choice_leave(Cpu *c, const uint32_t sv[6]) {
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x20;
}

/* The end both share: out's +2 and the record's by the fighter's +357 (15 at most) from `table`,
   then the three bytes' sum to the record's +4 and, high byte first, out's +3 and +4. */
static void choice_sum(Cpu *c, gaddr out, gaddr f, gaddr table) {
    const gaddr rec = 0x095179D8u;
    int32_t level = S8(f + 357);
    if (!(level < 15)) level = 0xF;
    c->r[16] = (uint32_t)level;
    int32_t v = S8(table + (uint32_t)level);
    *B(rec + 2) = (uint8_t)v;
    *B(out + 2) = (uint8_t)v;
    uint32_t sum = (uint32_t)*B(out) + *B(out + 1) + *B(out + 2);
    *H(rec + 4) = (uint16_t)sum;
    *B(out + 3) = (uint8_t)((sum & 0xFFFF) >> 8);
    *B(out + 4) = (uint8_t)sum;
}

/* (.., y, out, fighter): the cpu's choice, six bytes at out and a record at 0x095179D8. A random
   one of 32 from the row the stage (y + 20) names in 0x08A0E324; with the entry of 0x08A0E405 by
   the stage's byte in 0x08A0E3E4 and y's +9 added it is out's first byte. The fifth is 3 with y's
   +13 set and the fighter's +248 otherwise, and the second comes from 0x08A0E509 by its low four
   bits. In the mode 7 the second grows by six times PAIR's +4 and the first is halved. */
SFA_FUNC(088284DC, cpu_choice_a) {
    gaddr y = ARG(1), out = ARG(2), f = ARG(3);
    const gaddr rec = 0x095179D8u;
    uint32_t sv[6];
    choice_enter(c, sv);
    c->r[19] = y; c->r[16] = f; c->r[17] = out; c->r[18] = rec;
    int32_t row = S8(0x08A0E2FCu + (uint32_t)(int32_t)S16(y + 20));
    c->r[20] = (uint32_t)row << 5;
    ARG(0) = (uint32_t)row; ARG(1) = 0x08A0E2FCu;
    c->r[31] = 0x08828528u; f_08821368(c);
    *B(out + 1) = (uint8_t)S8(0x08A0E324u + c->r[20] + (c->r[2] & 0x1F));
    uint32_t a = (uint32_t)*B(0x08A0E3E4u + (uint32_t)(int32_t)S16(y + 20)) + (uint32_t)(int32_t)S8(y + 9);
    *B(out) = (uint8_t)(*B(out + 1) + S8(0x08A0E405u + a));
    uint32_t fifth = S8(y + 13) != 0 ? 3 : *B(f + 248);
    *B(out + 5) = (uint8_t)fifth;
    *B(rec + 3) = (uint8_t)fifth;
    *B(out + 1) = (uint8_t)S8(0x08A0E509u + (*B(out + 5) & 0xF));
    int32_t mode = S8(y + 79);
    c->r[19] = *B(out);
    if (mode == 7) {
        *B(out + 1) = (uint8_t)(*B(out + 1) + *W(PAIR + 4) * 6);
        *B(out) = (uint8_t)(c->r[19] >> 1);
        c->r[19] = *B(out);
    }
    *B(rec) = (uint8_t)c->r[19];
    *B(rec + 1) = *B(out + 1);
    choice_sum(c, out, f, 0x08A0E519u);
    choice_leave(c, sv);
}

/* (.., y, out, fighter): the same from the tables at 0x08A0E529 on, with half the added entry and
   no mode 7. */
SFA_FUNC(0882872C, cpu_choice_b) {
    gaddr y = ARG(1), out = ARG(2), f = ARG(3);
    const gaddr rec = 0x095179D8u;
    uint32_t sv[6];
    choice_enter(c, sv);
    c->r[19] = y; c->r[16] = f; c->r[17] = out; c->r[18] = rec;
    int32_t row = S8(0x08A0E529u + (uint32_t)(int32_t)S16(y + 20));
    c->r[20] = (uint32_t)row << 5;
    ARG(0) = (uint32_t)row; ARG(1) = 0x08A0E529u;
    c->r[31] = 0x08828778u; f_08821368(c);
    *B(out + 1) = (uint8_t)S8(0x08A0E549u + c->r[20] + (c->r[2] & 0x1F));
    uint32_t a = (uint32_t)*B(0x08A0E609u + (uint32_t)(int32_t)S16(y + 20)) + (uint32_t)(int32_t)S8(y + 9);
    int32_t add = S8(0x08A0E624u + a);
    uint32_t first = (uint32_t)*B(out + 1) + (uint32_t)(add / 2);
    *B(out) = (uint8_t)first;
    *B(rec) = (uint8_t)first;
    uint32_t fifth = S8(y + 13) != 0 ? 3 : *B(f + 248);
    *B(out + 5) = (uint8_t)fifth;
    *B(rec + 3) = (uint8_t)fifth;
    int32_t second = S8(0x08A0E728u + (*B(out + 5) & 0xF));
    *B(out + 1) = (uint8_t)second;
    *B(rec + 1) = (uint8_t)second;
    choice_sum(c, out, f, 0x08A0E738u);
    choice_leave(c, sv);
}
