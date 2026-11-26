/* Small functions the scenarios reach, 71 - 72 instructions: a projectile's start, a fighter's
   picture table with its partner looked up by mode, a number display built from three cells, a
   fighter's step dispatch by its flags. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define P1     0x092E3EA0u
#define P2     0x092E4338u
#define PART1  0x092E47D0u
#define PART2  0x092E4C68u

void f_0887684C(Cpu *c); void f_08876998(Cpu *c); void f_08904C94(Cpu *c); void f_08904CCC(Cpu *c);
void f_0886EB48(Cpu *c); void f_088215C4(Cpu *c); void f_08858418(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (projectile): on (+4 = 1, +9 / +73 / y step cleared), its owner's (+88) +24 / +86 / +164, pictures
   0x08A4C69C / 0x08A4C5A4 (+128 / +200, 0x08876998), its strength's (+346) speed (0x08A516C4) its way,
   0x58 ahead and 0x38 up, 0x08904C94 (it, owner), +344 from 0x08A516C0, +352 / +356 = 0x08A4C034 /
   0x08A4C054, 0x0887684C (strength) and 0x08904CCC. */
SFA_FUNC(08904B78, projectile_start_5) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = 1;
    *B(o + 9) = 0;
    *W(c->r[29] + 24) = s2;
    int32_t k = S8(o + 346);
    c->r[18] = (uint32_t)k;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 128) = 0x08A4C69Cu;
    *W(o + 200) = 0x08A4C5A4u;
    *W(c->r[29] + 28) = ra;
    call_on(c, o, 0x08904BDCu, f_08876998);
    *W(o + 100) = 0;
    *B(o + 73) = 0;
    *W(o + 96) = *W(0x08A516C4u + ((uint32_t)k << 2));
    int32_t dx = 0x58;
    if (*B(o + 11) == 0) { dx = -0x58; *W(o + 96) = (uint32_t)-(int32_t)*W(o + 96); }
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x38);
    call2(c, o, p, 0x08904C3Cu, f_08904C94);
    *B(o + 344) = (uint8_t)S8(0x08A516C0u + (uint32_t)k);
    *W(o + 352) = 0x08A4C034u;
    *W(o + 356) = 0x08A4C054u;
    call2(c, o, (uint32_t)k & 0xFFFF, 0x08904C74u, f_0887684C);
    call_on(c, o, 0x08904C7Cu, f_08904CCC);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The partner object (+600) for a fighter whose side's other fighter is character 0x1F: from the
   battle's +56 kind (1 / 2: the partners 0x092E47D0 / 0x092E4C68 by its side's low bit, sides 0 / 1
   having none; 3: the players, side 2 none); none in modes 0xF / 0x13, 7 (with BATTLE +79 = 5) and
   0x10 (with BATTLE +159 = 3), or for kind 0. */
static void partner_1f(Cpu *c, gaddr f) {
    int32_t m = S8(GAME + 114);
    c->r[17] = (uint32_t)m;
    if (m == 0xF || m == 0x13 || (m == 7 && S8(BATTLE + 79) == 5) || (m == 0x10 && S8(BATTLE + 159) == 3)) {
        *W(f + 600) = 0;
        return;
    }
    int32_t n = S16(BATTLE + 56);
    if (n < 0 || n > 3) return;
    if (n == 0) { *W(f + 600) = 0; return; }
    int32_t s = S8(f + 337);
    if ((n == 1 && s == 0) || (n == 2 && s == 1) || (n == 3 && s == 2)) { *W(f + 600) = 0; return; }
    gaddr x = n == 3 ? ((s & 1) ? P1 : P2) : ((s & 1) ? PART1 : PART2);
    *W(f + 600) = S8(x + 338) == 0x1F ? x : 0;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A96A24, 0x0886EB48
   (0x1E, 0x08A96AC0), +164 = 1 and its 0x1F partner (+600). */
SFA_FUNC(08993844, fighter_table_copy_1e) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *W(c->r[29] + 20) = s1;
    *B(f + 12) = 0xFF;
    c->r[17] = GAME;
    *W(c->r[29] + 24) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A96A24u + 4 * n);
    ARG(0) = f; ARG(1) = 0x1E; ARG(2) = 0x08A96AC0u;
    c->r[31] = 0x089938ACu; f_0886EB48(c);
    *W(f + 164) = 1;
    partner_1f(c, f);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (number, fighter): on (+7), +139 = 0x10, +140 = 3, +142 = the fighter's +769, x from 0x08A17268 by its
   kind (+3) and the round state (BATTLE +7); three 8-byte cells at +0x70 (x -0x20 / -0x10 / 0, y -8,
   picture 0x216 + tens / 0x216 + units / 0x8235, 1, 0x15) set up (0x08858418, 0xF) as its +156 with
   +148 = 5; drawn. */
SFA_FUNC(0885823C, number_three_cells) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 139) = 0x10;
    *B(o + 140) = 3;
    *B(o + 142) = (uint8_t)S8(f + 769);
    uint32_t i = 2u * *B(o + 3) + (uint32_t)S8(BATTLE + 7);
    *W(c->r[29] + 20) = s1;
    *H(o + 18) = (uint16_t)S16(0x08A17268u + 2 * i);
    gaddr cell = o + 0x70;
    c->r[17] = cell;
    *H(cell) = (uint16_t)-0x20; *H(cell + 2) = (uint16_t)-8;
    DIV(c, (uint32_t)S8(o + 142), 10);
    *B(cell + 6) = 1; *B(cell + 7) = 0x15;
    *H(cell + 4) = (uint16_t)(c->lo + 0x216);
    *H(cell + 8) = (uint16_t)-0x10; *H(cell + 10) = (uint16_t)-8;
    DIV(c, (uint32_t)S8(o + 142), 10);
    *B(cell + 14) = 1; *B(cell + 15) = 0x15;
    *H(cell + 12) = (uint16_t)(c->hi + 0x216);
    *H(cell + 16) = 0; *H(cell + 18) = (uint16_t)-8; *H(cell + 20) = 0x8235;
    *B(cell + 22) = 1;
    *W(c->r[29] + 24) = ra;
    *B(cell + 23) = 0x15;
    c->r[2] = cell + 16; c->r[3] = 0x8235;
    c->r[6] = (uint32_t)-8; c->r[7] = c->hi + 0x216; c->r[8] = 1; c->r[9] = 0x15;
    c->r[10] = cell + 8; c->r[11] = (uint32_t)S8(o + 142); c->r[12] = c->lo + 0x216;
    call2(c, o, 0xF, 0x08858334u, f_08858418);
    *W(o + 148) = 5;
    *W(o + 156) = cell;
    call_on(c, o, 0x08858348u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): its +7 step from the table picked by +192, +193 and +0x338: 0x08A924D0 / 0x08A924D8 /
   0x08A924EC / 0x08A924F8 (+696 = +192 set). */
SFA_FUNC(0897AFA4, fighter_step_by_flags_4) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t a = S8(f + 192), b = S8(f + 193);
    uint32_t i = (uint32_t)*B(f + 7) << 2;
    *W(c->r[29] + 16) = ra;
    gaddr tab; uint32_t back;
    int g = S8(f + 0x338) != 0;
    if (a == 0) {
        *B(f + 696) = 0;
        if (b != 0) { tab = 0x08A924D8u; back = 0x0897B034u; }
        else if (g) { tab = 0x08A924ECu; back = 0x0897B010u; }
        else { tab = 0x08A924D0u; back = 0x0897AFF0u; }
    } else {
        c->r[7] = 1;
        *B(f + 696) = 1;
        if (b != 0) { tab = 0x08A924F8u; back = 0x0897B0B8u; }
        else if (g) { tab = 0x08A924ECu; back = 0x0897B094u; }
        else { tab = 0x08A924F8u; back = 0x0897B074u; }
    }
    c->r[6] = tab;
    uint32_t h = *W(tab + i);
    c->r[5] = h;
    c->r[31] = back; rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
