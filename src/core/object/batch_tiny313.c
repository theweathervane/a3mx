/* More small ones whose callees are all done: the two partners' downs in a two-on-one fight, a
   random opponent, a time written out as text, three trail objects, and more moves' starts and
   steps. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define PAIR      0x095179E0u /* the two-on-one fight's own state */

void f_08821368(Cpu *c); void f_08823ED4(Cpu *c); void f_0882F024(Cpu *c); void f_0887CAD0(Cpu *c);
void f_0887CB3C(Cpu *c); void f_0887CC00(Cpu *c); void f_0886D678(Cpu *c); void f_0886469C(Cpu *c);
void f_08861848(Cpu *c); void f_0892B188(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c);
void f_08861674(Cpu *c); void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DD68(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
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

/* (x, fighter): a partner that has just gone down for good (state 1 / 6 / . / 0xFF) while x's +109
   is above 0 is marked in x's +107 by its side's bit, once. With both marked its wait (PAIR + 16 by
   side) is 0xB4 and the other's 0; else the wait is 1 in mode 10 (x + 79) and otherwise a random
   one of the sixteen words at 0x08A0E224. A 0x10 frame keeping s0. */
SFA_FUNC(08825D84, partner_down) {
    gaddr x = ARG(0), f = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    if (*B(f + 4) == 1 && *B(f + 5) == 6 && *B(f + 7) == 0xFF && S8(x + 109) > 0) {
        uint32_t side = (uint32_t)S8(f + 337) & 1;
        int32_t marks = S8(x + 107);
        uint32_t bit = 1u << side;
        c->r[16] = bit;
        if (!((uint32_t)marks & bit)) {
            *B(x + 107) = (uint8_t)((uint32_t)marks | bit);
            gaddr wait = PAIR + side * 2;
            c->r[16] = wait;
            if (S8(x + 107) == 3) {
                *H(wait + 16) = 0xB4;
                *H(PAIR + (side ^ 1) * 2 + 16) = 0;
            } else if (S8(x + 79) == 0xA) *H(wait + 16) = 1;
            else {
                call1(c, (uint32_t)(int32_t)S8(x + 79), f_08821368, 0x08825E3Cu);
                *H(wait + 16) = (uint16_t)S16(0x08A0E224u + (c->r[2] & 0xF) * 2);
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (x, side): a partner comes back in: x's +109 counts down (at 0 the marks +107 are cleared too),
   the side's mark is cleared, and fighter 3 or 4 goes to state 1 / 7 / 0 / 0 with +771 = 0xC and
   +573 = 1; 0x08823ED4 (GAME, x, the fighter), PAIR + 8 goes up, the side's wait is cleared, and
   the fighter's +702. A frame keeping s0, s1. */
SFA_FUNC(08825E6C, partner_back) {
    gaddr x = ARG(0);
    int32_t side = (int8_t)ARG(1);
    uint32_t sv[3];
    gaddr f = side != 0 ? FIGHTER_4 : FIGHTER_3;
    enter2(c, f, sv);
    c->r[17] = (uint32_t)side;
    *B(x + 109) = (uint8_t)(S8(x + 109) - 1);
    if (!(S8(x + 109) > 0)) {
        *B(x + 109) = 0; *B(x + 107) = 0;
        if (S8(x + 109) != 0) { leave2(c, sv); return; }
    }
    *B(x + 107) = (uint8_t)((uint32_t)S8(x + 107) & ~(1u << (side & 31)));
    *B(f + 4) = 1; *B(f + 5) = 7; *B(f + 6) = 0; *B(f + 7) = 0;
    *B(f + 771) = 0xC;
    *B(f + 573) = 1;
    call3(c, GAME, x, f, f_08823ED4, 0x08825F18u);
    *H(PAIR + 8) = (uint16_t)(S16(PAIR + 8) + 1);
    *H(PAIR + (uint32_t)side * 2 + 16) = 0;
    *B(f + 702) = 0;
    leave2(c, sv);
}

/* (who, kind, d, e): a random opponent: the kind is a random -1, 0 or 1, and the character a
   random one of 0x2A that 0x0882F024 (with the table 0x08A0F33C) lets through, with 4 and 6 turned
   into 0x12 and 0x14 for the kind -1; d is cleared and e is -1. A 0x40 frame keeping s0..s7. */
SFA_FUNC(0882F9D0, random_opponent) {
    gaddr who = ARG(0), kind = ARG(1), d = ARG(2), e = ARG(3);
    uint32_t sv[9];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 8; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 48) = sv[8];
    c->r[16] = e; c->r[17] = d; c->r[18] = kind; c->r[19] = who;
    call0(c, f_08821368, 0x0882FA0Cu);
    DIV(c, c->r[2], 3);
    c->r[21] = 0x08A0F33Cu; c->r[23] = 0x2A; c->r[20] = 0xFFFFFFFFu;
    *B(kind) = (uint8_t)(c->hi - 1);
    int32_t ch;
    do {
        call0(c, f_08821368, 0x0882FA38u);
        DIV(c, c->r[2], 0x2A);
        ch = (int8_t)c->hi;
        c->r[22] = (uint32_t)ch;
        call3(c, (uint32_t)ch & 0xFF, 0, 0x08A0F33Cu, f_0882F024, 0x0882FA58u);
    } while (c->r[2] == 0);
    if (S8(kind) == -1) {
        if (ch == 4) ch = 0x12;
        if (ch == 6) ch = 0x14;
    }
    *B(who) = (uint8_t)ch;
    *B(d) = 0;
    *B(e) = 0xFF;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[8]; c->r[29] += 0x40;
}

/* (text, minutes, seconds, frames): a time as nine bytes: two digits of minutes, 0x27, two of
   seconds, 0x22, and two of hundredths (the frames times five over three), ended by 0. It leaves
   text + 4 and text + 5 in v0 and v1. */
SFA_FUNC(0883296C, time_text) {
    gaddr text = ARG(0);
    int32_t minutes = (int8_t)ARG(1), seconds = (int8_t)ARG(2), frames = (int8_t)ARG(3);
    int32_t hundredths = (int16_t)(frames * 5 / 3);
    *B(text) = (uint8_t)(minutes / 10 + 0x30);
    *B(text + 1) = (uint8_t)(minutes % 10 + 0x30);
    *B(text + 2) = 0x27;
    *B(text + 3) = (uint8_t)(seconds / 10 + 0x30);
    *B(text + 4) = (uint8_t)(seconds % 10 + 0x30);
    *B(text + 5) = 0x22;
    DIV(c, (uint32_t)hundredths, 10);
    *B(text + 6) = (uint8_t)(c->lo + 0x30);
    *B(text + 7) = (uint8_t)(c->hi + 0x30);
    *B(text + 8) = 0;
    c->r[2] = text + 4; c->r[3] = text + 5;
}

/* (owner, w): unless its +80 is set, and when 0x0887CB3C says so: the three objects listed at
   0x08A22DFC are started on the owner (0x0887CC00 with 2) as kind 0xB, with what 0x0887CAD0 returns
   for the owner's side at +96, the owner's +14, a time from the words at 0x08A22E08, a colour
   moved on by the words at 0x08A22E0E, and w at +100. A 0x40 frame keeping s0..s7 and fp. */
SFA_FUNC(0887CF24, three_trails) {
    gaddr o = ARG(0);
    uint32_t w = ARG(1);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    int32_t off = S8(o + 80);
    c->r[6] = (uint32_t)off;
    *W(sp + 20) = sv[1]; c->r[17] = o;
    *W(sp + 16) = sv[0];
    for (uint32_t i = 2; i < 8; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 48) = sv[8]; *W(sp + 52) = sv[9];
    c->r[16] = w;
    if (off == 0) {
        call1(c, (uint32_t)(int32_t)S8(o + 337), f_0887CAD0, 0x0887CF68u);
        uint32_t side_word = c->r[2];
        c->r[23] = side_word;
        call0(c, f_0887CB3C, 0x0887CF70u);
        c->r[21] = 0x08A20000u;
        if (c->r[2] != 0) {
            c->r[20] = 0xB;
            for (uint32_t i = 0; i < 3; i++) {
                c->r[22] = i; c->r[21] = 0x08A22DFCu + i * 4;
                c->r[19] = 0x08A22E08u + i * 2; c->r[18] = 0x08A22E0Eu + i * 2;
                gaddr p = *W(*W(0x08A22DFCu + i * 4));
                c->r[30] = p;
                call3(c, o, p, 2, f_0887CC00, 0x0887CFACu);
                *B(p + 2) = 0xB;
                *W(p + 96) = side_word;
                *B(p + 14) = *B(o + 14);
                uint8_t colour = *B(p + 13);
                *H(p + 82) = (uint16_t)S16(0x08A22E08u + i * 2);
                *B(p + 13) = (uint8_t)(colour + S16(0x08A22E0Eu + i * 2));
                *W(p + 100) = w;
            }
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x40;
}

/* (fighter): a move's steps. 0: facing as +697 says, +235 set, 0x0892B188, 0x08863114, 0x0886CA2C
   with the strength, the animation 6 + twice the strength; the heavy one goes to step 2, the
   others to 1. 1: 0x0886D678. 2: ticked to the animation's end (then 0x0886469C), moving 8 forward
   on each cue (+77). A frame keeping s0, s1. */
SFA_FUNC(0892A15C, move_steps_2a15c) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    uint32_t k = *B(o + 7);
    if (k == 1) call1(c, o, f_0886D678, 0x0892A188u);
    else if (k == 0) {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        call1(c, o, f_0892B188, 0x0892A204u);
        call1(c, o, f_08863114, 0x0892A20Cu);
        int32_t strength = S8(o + 194);
        c->r[17] = (uint32_t)strength;
        call2(c, o, (uint32_t)strength, f_0886CA2C, 0x0892A21Cu);
        uint32_t anim = ((uint32_t)strength << 1) + 6;
        if (S8(o + 194) == 2) *B(o + 7) = 2;
        call2(c, o, anim & 0xFFFF, f_08861674, 0x0892A244u);
    } else if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x0892A1ACu);
    else {
        if (S8(o + 77) != 0) {
            *B(o + 77) = 0;
            *W(o + 16) = *W(o + 16) + (*B(o + 11) != 0 ? 0x80000u : 0xFFF80000u);
        }
        call1(c, o, f_08861848, 0x0892A1E4u);
    }
    leave2(c, sv);
}

/* (fighter): a super move's start: never with +634 set; on the ground both 0x0886C9F0 and
   0x0886C7E4 must allow, and it faces as +697 says; in the air +251 must be clear and 0x0886C2B4
   allow. Then the move from state 1 / 0 / 8 / 0 and 0x0886DD68. Returns whether (one of them the
   other way round). `back`: the three checks' return addresses, then 0x0886DD68's on the ground
   and in the air. */
static void super_start(Cpu *c, uint8_t move, int inverted, const uint32_t back[5]) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int air = 0;
    int ok = S8(o + 634) == 0;
    if (ok) {
        air = S8(o + 73) != 0;
        if (!air) {
            call1(c, o, f_0886C9F0, back[0]);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, back[1]);
                ok = c->r[2] != 0;
            }
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, back[2]);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 236) = move;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        c->r[5] = 1;
        if (!air) {
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 11) = (uint8_t)facing;
            c->r[5] = (uint32_t)(int32_t)facing;
        }
        call1(c, o, f_0886DD68, back[air ? 4 : 3]);
    }
    c->r[2] = (uint32_t)(inverted ? !ok : ok);
    leave(c, sv);
}
SFA_FUNC(08935818, super_start_35818) {
    static const uint32_t back[5] = { 0x08935844u, 0x08935854u, 0x089358ACu, 0x08935888u, 0x089358D8u };
    super_start(c, 0xB, 1, back);
}
SFA_FUNC(0893D22C, super_start_3d22c) {
    static const uint32_t back[5] = { 0x0893D258u, 0x0893D268u, 0x0893D2C0u, 0x0893D29Cu, 0x0893D2ECu };
    super_start(c, 4, 0, back);
}
SFA_FUNC(08949B5C, super_start_49b5c) {
    static const uint32_t back[5] = { 0x08949B88u, 0x08949B98u, 0x08949BF0u, 0x08949BCCu, 0x08949C1Cu };
    super_start(c, 0xC, 0, back);
}
SFA_FUNC(0896C870, super_start_6c870) {
    static const uint32_t back[5] = { 0x0896C89Cu, 0x0896C8ACu, 0x0896C904u, 0x0896C8E0u, 0x0896C930u };
    super_start(c, 2, 0, back);
}

/* (fighter): the same for the move 0, needing half a meter (0x48) as well. A frame keeping s0,
   s1. */
SFA_FUNC(0899B058, super_start_9b058) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int ok = S8(o + 634) == 0 && !(S16(o + 366) < 72);
    if (ok) {
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, 0x0899B098u);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, 0x0899B0A8u);
                ok = c->r[2] != 0;
            }
            if (ok) *B(o + 11) = (uint8_t)S8(o + 697);
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, 0x0899B108u);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        c->r[5] = 1; c->r[17] = 1;
        *B(o + 4) = 1; *B(o + 6) = 8; *B(o + 7) = 0; *B(o + 5) = 0;
        *B(o + 236) = 0;
        call1(c, o, f_0886DD68, 0x0899B0E0u);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}

/* (fighter): step 2 of a move: +0x3BB = -1 and 0x0886CA2C with the strength. Further away than the
   word from 0x08A74390 by strength, +0x3BB is cleared and the animation is 0x3D + strength (3 +
   strength for the choice -1); nearer, 0x46 + strength (0x1B + strength). A frame keeping
   s0..s2. */
SFA_FUNC(0893C254, move_far_or_near_3c254) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    *B(o + 7) = 2;
    c->r[18] = 0xFFFFFFFFu; c->r[17] = o + 0x3B8;
    *B(o + 0x3B8 + 3) = 0xFF;
    *W(sp + 28) = ra;
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0893C28Cu);
    int32_t strength = S8(o + 194);
    uint32_t anim, back;
    if ((int32_t)*H(0x08A74390u + (uint32_t)(strength * 2)) < (int32_t)*H(o + 508)) {
        *B(o + 0x3B8 + 3) = 0;
        int32_t choice = S8(o + 386);
        strength = S8(o + 194);
        if (choice == -1) { anim = (uint32_t)(strength + 3) & 0xFFFF; back = 0x0893C2F0u; }
        else { anim = (uint32_t)(strength + 0x3D) & 0xFFFF; back = 0x0893C2D8u; }
    } else if (S8(o + 386) == -1) { anim = (uint32_t)(strength + 0x1B) & 0xFFFF; back = 0x0893C324u; }
    else { anim = (uint32_t)(strength + 0x46) & 0xFFFF; back = 0x0893C310u; }
    call2(c, o, anim, f_08861674, back);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
