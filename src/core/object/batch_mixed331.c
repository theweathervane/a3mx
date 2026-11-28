/* More whose callees are all done: the partners' frame in a two-on-one fight, the list of
   opponents by mode, both sides' fighters out of their lists, a result's ranking, the lists sent
   over the link, and the link's peers looked over. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define PAIR      0x095179E0u
#define LIST_1    0x09517A28u
#define LIST_2    0x09517A50u
#define NETP      0x09523980u

void f_08825F50(Cpu *c); void f_08825D84(Cpu *c); void f_0882E920(Cpu *c); void f_08830FE4(Cpu *c);
void f_0882E5A0(Cpu *c); void f_0882E790(Cpu *c); void f_08831924(Cpu *c); void f_088303B8(Cpu *c);
void f_08824164(Cpu *c); void f_088932A4(Cpu *c); void f_08892D6C(Cpu *c); void f_08891FE8(Cpu *c);
void f_08892020(Cpu *c); void f_088924E8(Cpu *c); void f_088D61F8(Cpu *c); void f_088E51DC(Cpu *c);
void f_0882FB0C(Cpu *c); void f_088DCF0C(Cpu *c); void f_088DC48C(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (-, y): the partners' frame in a fight with partners (y + 56) in the modes 7 and 10: the wait
   to come back (0x08825F50 with PAIR), and the check for one going down (0x08825D84) for fighter
   3 and, outside mode 10, fighter 4. A frame keeping s0, s1. */
SFA_FUNC(08825C08, partners_frame) {
    gaddr y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t kind = S16(y + 56);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = y;
    if (kind != 0) {
        int32_t mode = S8(y + 79);
        c->r[17] = 0xA;
        if (mode == 7 || mode == 0xA) {
            call2(c, y, PAIR, f_08825F50, 0x08825C4Cu);
            call2(c, y, FIGHTER_3, f_08825D84, 0x08825C5Cu);
            if (S8(y + 79) != 0xA) call2(c, y, FIGHTER_4, f_08825D84, 0x08825C74u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): the list of opponents by mode (y + 79): 6 the pairs, 7 the random pairs, 8 the random
   ten, below 6 the mode's own table; others none. */
SFA_FUNC(0882EACC, opponents_by_mode) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t mode = S8(y + 79);
    *W(c->r[29] + 16) = ra;
    c->r[6] = y; c->r[7] = y;
    if (mode == 6) call2(c, x, y, f_0882E920, 0x0882EAFCu);
    else if (mode == 7) call2(c, x, y, f_08830FE4, 0x0882EB24u);
    else if (mode == 8) call2(c, x, y, f_0882E5A0, 0x0882EB48u);
    else if (mode < 7) call2(c, x, y, f_0882E790, 0x0882EB68u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The first entry of a side's list becomes its fighter: the character, the choice, +376 and +439
   from the list's +3, +6, +9 and +12; the list's place and rounds are cleared. */
static void first_of_list(gaddr f, gaddr list) {
    *B(f + 338) = (uint8_t)S8(list + 3);
    *B(f + 386) = (uint8_t)S8(list + 6);
    *B(f + 376) = (uint8_t)S8(list + 9);
    *B(f + 439) = (uint8_t)S8(list + 12);
    *B(list + 2) = 0;
    *B(list + 1) = 0;
}

/* (x, y): a fight between two lists starts: both sides' first fighters, their stored blocks
   (0x08831924), x's +77 the longer list's length, both marked (+413). In mode 6 (x + 114), and in
   mode 0x10 with y's +159 = 2, both start on full vitality (+420 = 0x90); else the fighter across
   from x's side (+53) starts on the byte at 0x09517A78 + 912. Then 0x088303B8 (x, y) and
   0x08824164 with the two fighters. A 0x30 frame keeping s0..s5. */
SFA_FUNC(088317B4, lists_fight_start) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4]; *W(sp + 28) = sv[3];
    c->r[20] = LIST_1; c->r[19] = FIGHTER_1;
    *W(sp + 16) = sv[0]; c->r[16] = y;
    *W(sp + 20) = sv[1]; c->r[17] = x;
    first_of_list(FIGHTER_1, LIST_1);
    *W(sp + 36) = sv[5]; c->r[21] = LIST_2;
    *W(sp + 24) = sv[2]; c->r[18] = FIGHTER_2;
    first_of_list(FIGHTER_2, LIST_2);
    *W(sp + 40) = sv[6];
    call2(c, FIGHTER_1, LIST_1, f_08831924, 0x08831858u);
    call2(c, FIGHTER_2, LIST_2, f_08831924, 0x08831864u);
    {
        int32_t n2 = S8(LIST_2), n1 = S8(LIST_1);
        *B(x + 77) = (uint8_t)(n2 < n1 ? n1 : n2);
    }
    *B(FIGHTER_1 + 413) = 1; *B(FIGHTER_2 + 413) = 1;
    int32_t mode = S8(x + 114);
    if (mode == 6 || (mode == 0x10 && S8(y + 159) == 2)) {
        *H(FIGHTER_1 + 420) = 0x90; *H(FIGHTER_2 + 420) = 0x90;
    } else {
        int32_t side = S8(x + 53);
        uint8_t v = *B(0x09517A78u + 912);
        *H(((side & 1) ? FIGHTER_2 : FIGHTER_1) + 420) = v;
    }
    call2(c, x, y, f_088303B8, 0x088318F4u);
    call2(c, FIGHTER_1, FIGHTER_2, f_08824164, 0x08831900u);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (x): a finished run's ranking. x's +341 moves on, the entry's name starts as 0x30, 0x30, 0x2D
   (+457..+459), +346 = 0x258, +348 = 0xF000, +350 = 0x960, and the characters 0x25 and 0x13 count
   as 0x15 and 0x11 (+448). In the first mode (BATTLE + 79) the result goes into one of the two
   38-entry rankings (0x088932A4 in mode 0x12, else 0x08892D6C; the other's place is 0x27): with a
   place in neither 0x08891FE8, else the better place is kept at +462 and 0x08892020 (1) runs. In
   the other modes it goes among the mode's ten best, and 0x08891FE8 or 0x08892020 (1) by whether
   it got in. A frame keeping s0, s1. */
SFA_FUNC(08891E24, run_ranking) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    int32_t n = S8(x + 341);
    *B(x + 463) = 0;
    *B(x + 341) = (uint8_t)(n + 1);
    *B(x + 456) = 0;
    *B(x + 457) = 0x30; *B(x + 458) = 0x30; *B(x + 459) = 0x2D;
    *H(x + 350) = 0x960;
    *H(x + 346) = 0x258;
    *H(x + 348) = 0xF000;
    int32_t ch = S8(x + 448);
    *B(x + 474) = 0;
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = x;
    if (ch == 0x25) *B(x + 448) = 0x15;
    if (S8(x + 448) == 0x13) *B(x + 448) = 0x11;
    if (S8(BATTLE + 79) == 0) {
        c->r[17] = 0x27;
        if (S8(GAME + 114) == 0x12) {
            call1(c, x, f_088932A4, 0x08891EF0u);
            *B(x + 460) = 0x27;
        } else {
            call1(c, x, f_08892D6C, 0x08891ED8u);
            *B(x + 461) = 0x27;
        }
        int32_t b = S8(x + 461), a = S8(x + 460);
        c->r[17] = (uint32_t)a;
        if (!(a < 39) && !(b < 39)) call1(c, x, f_08891FE8, 0x08891F34u);
        else {
            *B(x + 462) = (uint8_t)(b < a ? b : a);
            call2(c, x, 1, f_08892020, 0x08891F58u);
        }
    } else {
        call1(c, x, f_088924E8, 0x08891F68u);
        if (S8(x + 474) < 0) call1(c, x, f_08891FE8, 0x08891F7Cu);
        else call2(c, x, 1, f_08892020, 0x08891F8Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* Both sides' lists go out over the link: for each, a buffer on the stack takes the fighter's +438
   (first byte) and +441 (byte 23) and what 0x088D61F8 packs of the list with its length, and
   0x088E51DC takes it by number. Between the two, in mode 0xC or with NETP + 34 set, for the link
   kinds 2 and 4 with NETP + 36 set, the second side's three are drawn at random first. A 0x50
   frame keeping s0, s1; the fighters and lists are kept at +40..+52 of it. */
SFA_FUNC(088D6258, link_send_lists) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 40) = FIGHTER_1; *W(sp + 44) = FIGHTER_2;
    *W(sp + 48) = LIST_1; *W(sp + 52) = LIST_2;
    gaddr f = *W(sp + 40), list = *W(sp + 48);
    *B(sp + 16) = (uint8_t)S8(f + 438);
    *B(sp + 39) = (uint8_t)S8(f + 441);
    *W(sp + 56) = s0; *W(sp + 60) = s1;
    c->r[16] = sp + 0x10; c->r[17] = 0;
    *W(sp + 64) = ra;
    call3(c, sp + 0x10, list, (uint32_t)(int32_t)S8(list), f_088D61F8, 0x088D62C8u);
    call2(c, 0, sp + 0x10, f_088E51DC, 0x088D62D4u);
    c->r[17] = 1;
    uint32_t kind = *B(NETP + 43);
    if ((S8(GAME + 114) == 0xC || *B(NETP + 34) != 0) && (kind == 2 || kind == 4) && S8(NETP + 36) != 0) {
        ARG(0) = (uint32_t)(int32_t)S8(NETP + 36); ARG(1) = NETP;
        call0(c, f_0882FB0C, 0x088D632Cu);
    }
    f = *W(sp + 44); list = *W(sp + 52);
    *B(sp + 16) = (uint8_t)S8(f + 438);
    *B(sp + 39) = (uint8_t)S8(f + 441);
    c->r[7] = list;
    call3(c, sp + 0x10, list, (uint32_t)(int32_t)S8(list), f_088D61F8, 0x088D6358u);
    call2(c, 1, sp + 0x10, f_088E51DC, 0x088D6364u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x50;
}

/* The link's peers are looked over, under the list's lock (0x09523720 + 8). A peer marked 2
   (+154) is removed by its address and freed, unless its state (+144) is 5. Another one's wait
   (+152) counts down; while it still runs, the first peer in state 3 that is not marked becomes
   the pick (marked 1) if there is none yet; a peer whose wait is over and that is not marked is
   freed. Returns the pick, or 0. A 0x40 frame keeping s0..s7. */
SFA_FUNC(088DCB00, link_peers_sweep) {
    const gaddr list = 0x09523720u;
    uint32_t sv[9];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = list;
    uint32_t sema = *W(list + 8);
    *W(sp + 20) = sv[1]; c->r[17] = 0;
    for (uint32_t i = 2; i < 8; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 48) = sv[8];
    c->r[18] = 0;
    if (sema != 0) call3(c, sema, 1, 0, hle_sceKernelWaitSema, 0x088DCB4Cu);
    gaddr node = *W(list), before = 0, pick = 0;
    c->r[20] = node; c->r[21] = 2;
    if (node != 0) { c->r[22] = 5; c->r[23] = 3; c->r[19] = 1; }
    while (node != 0) {
        int32_t mark = S8(node + 154);
        c->r[17] = before; c->r[18] = pick; c->r[20] = node;
        if (mark == 2) {
            if (*W(node + 144) != 5) {
                call1(c, node + 4, f_088DCF0C, 0x088DCB84u);
                *B(node + 154) = 0;
                call3(c, list, node, before, f_088DC48C, 0x088DCB98u);
                node = c->r[2];
                continue;
            }
        } else {
            int32_t wait = S16(node + 152);
            if (wait > 0) {
                *H(node + 152) = (uint16_t)(wait - 1);
                if (S16(node + 152) > 0 && *W(node + 144) == 3 && ((uint32_t)S8(node + 154) | pick) == 0) {
                    pick = node;
                    *B(node + 154) = 1;
                }
            } else if (mark == 0) {
                call3(c, list, node, before, f_088DC48C, 0x088DCBF8u);
                node = c->r[2];
                continue;
            }
        }
        before = node;
        node = *W(before);
    }
    c->r[17] = before; c->r[18] = pick; c->r[20] = 0;
    sema = *W(list + 8);
    c->r[16] = sema;
    if (sema != 0) call2(c, sema, 1, hle_sceKernelSignalSema, 0x088DCC24u);
    c->r[2] = pick;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[8]; c->r[29] += 0x40;
}
