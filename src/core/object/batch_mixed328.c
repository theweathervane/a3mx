/* More whose callees are all done: four ways a list of opponents is made (at random, from a
   mode's table, in pairs, and in random pairs), and a result's place among a mode's ten best. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define WHO       0x09517988u /* the list's characters */
#define HOW       0x095179B0u /* and their choices */
#define PAIR      0x095179E0u
#define ENTRIES   0x095179F8u /* four bytes each; the characters at +2 and +3 */

void f_08836538(Cpu *c); void f_08821368(Cpu *c); void f_0882F024(Cpu *c); void f_0882EB74(Cpu *c);
void f_08831A78(Cpu *c); void f_0882EBB0(Cpu *c); void f_08830384(Cpu *c); void f_089F3430(Cpu *c);
void f_088934BC(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (-, y): ten random opponents (y + 72 = 9). At the very start of a run (the count at PAIR is 0)
   0x08836538 runs and y's +33, +34 and +80 are drawn, with eight times +80's low four bits kept at
   0x08B01F38. Each place gets a random choice (-1, 0 or 1) and a random one of 0x2A characters
   that 0x0882F024 lets through for the place (table 0x08A0F1EE), as 0x0882EB74 makes it for the
   choice; but the first place, when the count is a multiple of 50 and not 0, is one of the
   characters 0x24 and 0xA by chance, with the choice 0. Then 0x08831A78 (y, 0). A 0x50 frame
   keeping s0..s7 and fp; the three lists' addresses and y are kept at +16..+28 of it. */
SFA_FUNC(0882E5A0, random_ten) {
    gaddr y = ARG(1);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 24) = HOW;
    uint32_t count = *W(PAIR);
    *W(sp + 32) = sv[0];
    *W(sp + 20) = WHO;
    c->r[16] = y;
    for (uint32_t i = 1; i < 8; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 64) = sv[8]; *W(sp + 68) = sv[9];
    *W(sp + 16) = ENTRIES;
    if (count == 0) {
        ARG(0) = ENTRIES;
        call0(c, f_08836538, 0x0882E608u);
        call0(c, f_08821368, 0x0882E610u);
        *B(y + 33) = (uint8_t)c->r[2];
        call0(c, f_08821368, 0x0882E618u);
        *B(y + 34) = (uint8_t)c->r[2];
        call0(c, f_08821368, 0x0882E620u);
        *B(y + 80) = (uint8_t)c->r[2];
        *W(0x08B01F38u) = ((uint32_t)S8(y + 80) & 0xF) << 3;
    }
    *W(sp + 28) = y;
    *H(y + 72) = 9;
    c->r[19] = 0x2A; c->r[30] = 3; c->r[18] = 0x08A0F1EEu;
    for (int32_t i = 0; i < 10; i = (int16_t)(i + 1)) {
        uint32_t n = *W(PAIR);
        DIV(c, n, 0x32);
        gaddr how = *W(sp + 24) + (uint32_t)i, who = *W(sp + 20) + (uint32_t)i;
        gaddr entry = *W(sp + 16) + ((uint32_t)i << 2);
        c->r[21] = (uint32_t)i; c->r[20] = how; c->r[22] = who; c->r[23] = entry;
        if ((c->hi | (uint32_t)i) == 0 && n != 0) {
            *B(how) = 0;
            call0(c, f_08821368, 0x0882E6A0u);
            uint32_t ch = (c->r[2] & 1) != 0 ? 0x24 : 0xA;
            c->r[20] = (int32_t)c->r[2] >= 0 ? (c->r[2] & 1) : 0 - (c->r[2] & 1);
            *B(who) = (uint8_t)ch;
            *B(entry + 2) = (uint8_t)ch;
            continue;
        }
        call0(c, f_08821368, 0x0882E6E4u);
        DIV(c, c->r[2], 3);
        uint32_t place = (uint32_t)i & 0xFF;
        c->r[17] = place;
        *B(how) = (uint8_t)(c->hi - 1);
        uint32_t ch;
        do {
            call0(c, f_08821368, 0x0882E700u);
            DIV(c, c->r[2], 0x2A);
            ch = c->hi & 0xFF;
            c->r[16] = ch;
            call3(c, ch, place, 0x08A0F1EEu, f_0882F024, 0x0882E71Cu);
        } while (c->r[2] == 0);
        call2(c, ch, *B(how), f_0882EB74, 0x0882E730u);
        *B(who) = (uint8_t)c->r[2];
        *B(entry + 2) = *B(who);
    }
    call2(c, *W(sp + 28), 0, f_08831A78, 0x0882E760u);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x50;
}

/* (x, y): the mode's list of opponents (0x0882EBB0, with the ten choices at 0x08A0EE08 that y's
   +34 picks), then the first fight. In the modes 4 and 5 (y + 79) fighters 1 and 2 share their
   +400 and +404 and the opponent is fighter 3; else it is the fighter across from x's side (+57).
   y's +48 is our side. The first on the list that 0x08830384 does not hold back for us (up to the
   list's length) is kept at y + 20 and becomes the opponent's character and choice; then
   0x08831A78 with the character. A 0x30 frame keeping s0..s6. */
SFA_FUNC(0882E790, list_first_fight) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = y;
    uint32_t pick = (uint32_t)S8(y + 34) & 7;
    *W(sp + 20) = sv[1]; c->r[17] = x;
    for (uint32_t i = 2; i < 7; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 44) = sv[7];
    call3(c, x, y, 0x08A0EE08u + ((pick * 10) & 0xFF), f_0882EBB0, 0x0882E7ECu);
    int32_t mode = S8(y + 79);
    gaddr us, them;
    if (mode == 4 || mode == 5) {
        uint32_t w = *W(FIGHTER_1 + 400) | *W(FIGHTER_2 + 400);
        int32_t h1 = S16(FIGHTER_1 + 404), h2 = S16(FIGHTER_2 + 404);
        *W(FIGHTER_2 + 400) = w; *W(FIGHTER_1 + 400) = w;
        *H(FIGHTER_2 + 404) = (uint16_t)(h1 | h2); *H(FIGHTER_1 + 404) = (uint16_t)(h1 | h2);
        us = FIGHTER_1; them = FIGHTER_3;
    } else if (S8(x + 57) & 1) { us = FIGHTER_1; them = FIGHTER_2; }
    else { us = FIGHTER_2; them = FIGHTER_1; }
    c->r[17] = us; c->r[18] = them;
    *B(y + 48) = (uint8_t)S8(us + 337);
    c->r[21] = WHO;
    int32_t at = 0;
    uint32_t ch;
    for (;;) {
        ch = *B(WHO + (uint32_t)at);
        c->r[19] = (uint32_t)at; c->r[20] = (uint32_t)at; c->r[22] = ch;
        if (S16(y + 72) < at) break;
        call2(c, (uint32_t)(int32_t)(int16_t)ch, us, f_08830384, 0x0882E8B4u);
        if (c->r[2] == 0) break;
        at = (int16_t)(at + 1);
    }
    *H(y + 20) = (uint16_t)at;
    *B(them + 338) = (uint8_t)ch;
    *B(them + 386) = *B(HOW + (uint32_t)at);
    call2(c, y, (uint32_t)(int32_t)S8(them + 338), f_08831A78, 0x0882E8F8u);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x30;
}

/* (x, y): nineteen pairs of opponents (y + 72 = 0x12) from the three-byte entries at 0x08A0F1F5,
   copied to the stack first: two characters and a choice, where -2 means a random one of -1, 0
   and 1. Each pair's choice is kept at HOW and its characters, as 0x0882EB74 makes them for the
   choice, at the entry's +2 and +3. y's +48 is the side of x's fighter (+57), and fighters 3 and 4
   become the pair at y + 20; then 0x08831A78 with the first one's character. A 0x80 frame keeping
   s0..s7 and fp: the entries at +16, x at +76, HOW's address at +80. */
SFA_FUNC(0882E920, list_of_pairs) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x80;
    gaddr sp = c->r[29];
    *W(sp + 84) = sv[0]; *W(sp + 116) = sv[8];
    c->r[30] = y; c->r[16] = sp + 0x10;
    *W(sp + 76) = x;
    for (uint32_t i = 1; i < 8; i++) *W(sp + 84 + 4 * i) = sv[i];
    *W(sp + 120) = sv[9];
    call3(c, sp + 0x10, 0x08A0F1F5u, 0x39, f_089F3430, 0x0882E96Cu);
    *H(y + 72) = 0x12;
    c->r[21] = 0xFFFFFFFEu; c->r[20] = 3; c->r[23] = HOW; c->r[22] = ENTRIES;
    gaddr at = sp + 0x10;
    for (int32_t i = 0; i < 19; i = (int16_t)(i + 1)) {
        uint32_t ch1 = *B(at), ch2 = *B(at + 1);
        int32_t choice = S8(at + 2);
        at += 3;
        c->r[17] = (uint32_t)i; c->r[19] = ch1; c->r[18] = ch2; c->r[16] = at;
        if (choice == -2) {
            call0(c, f_08821368, 0x0882E9BCu);
            DIV(c, c->r[2], 3);
            choice = (int8_t)(c->hi - 1);
        }
        *W(sp + 80) = HOW;
        *B(HOW + (uint32_t)i) = (uint8_t)choice;
        c->r[23] = (uint32_t)choice & 0xFF;
        call2(c, ch1, (uint32_t)choice & 0xFF, f_0882EB74, 0x0882E9ECu);
        gaddr entry = ENTRIES + ((uint32_t)i << 2);
        c->r[19] = entry;
        *B(entry + 2) = (uint8_t)c->r[2];
        call2(c, ch2, (uint32_t)choice & 0xFF, f_0882EB74, 0x0882EA04u);
        *B(entry + 3) = (uint8_t)c->r[2];
        c->r[23] = *W(sp + 80);
    }
    gaddr mine = (S8(*W(sp + 76) + 57) & 1) ? FIGHTER_1 : FIGHTER_2;
    uint32_t place = (uint32_t)(int32_t)S16(y + 20);
    *B(y + 48) = (uint8_t)S8(mine + 337);
    gaddr entry = ENTRIES + (place << 2);
    *B(FIGHTER_3 + 338) = (uint8_t)S8(entry + 2);
    *B(FIGHTER_3 + 386) = *B(HOW + place);
    *B(FIGHTER_4 + 338) = (uint8_t)S8(entry + 3);
    *B(FIGHTER_4 + 386) = *B(HOW + place);
    c->r[7] = FIGHTER_3; c->r[8] = FIGHTER_4;
    call2(c, y, (uint32_t)(int32_t)S8(FIGHTER_3 + 338), f_08831A78, 0x0882EA9Cu);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x80;
}

/* (x): x's result among the mode's ten best (sixteen bytes each, 160 a mode from 0x08A07268; the
   modes from 7 on follow the first six). A result that does not keep its place against the last
   (0x088934BC) is out: x's +460 = 3, +474 = -1. Else the entries it beats move down one and it is
   written where it stops: x's +452, the letters 0x43, 0x41, 0x50, +448, +450 (2 when the byte
   x's +784 points at is set), the word +478 and the bytes +472 and +469..+471; the place goes to
   x's +460 and +474 is 1. A 0x30 frame keeping s0..s3. */
SFA_FUNC(088924E8, best_ten_insert) {
    gaddr x = ARG(0);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = x;
    int32_t mode = S8(BATTLE + 79);
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    mode = mode < 7 ? mode - 1 : mode - 4;
    gaddr slot = 0x08A07268u + (uint32_t)mode * 160 + 0x90;
    c->r[18] = slot;
    call2(c, x, slot, f_088934BC, 0x0889254Cu);
    if (c->r[2] == 0) { *B(x + 460) = 3; *B(x + 474) = 0xFF; goto out; }
    uint32_t place = 9;
    c->r[17] = 9; c->r[19] = slot;
    for (;;) {
        gaddr above = slot - 16;
        c->r[18] = above; c->r[19] = slot; c->r[17] = place;
        call2(c, x, above, f_088934BC, 0x0889256Cu);
        if (c->r[2] == 0) break;
        for (uint32_t i = 0; i < 16; i++) *B(slot + i) = *B(above + i);
        place -= 1;
        slot = above;
        if (place == 0) break;
    }
    *B(x + 460) = (uint8_t)place;
    *B(x + 474) = 1;
    *W(slot) = *W(x + 452);
    *B(slot + 4) = 0x43; *B(slot + 5) = 0x41; *B(slot + 6) = 0x50;
    *B(slot + 8) = (uint8_t)S8(x + 448);
    *B(slot + 9) = S8(*W(x + 784)) != 0 ? 2 : (uint8_t)S8(x + 450);
    *H(slot + 10) = *H(x + 478);
    *B(slot + 12) = (uint8_t)S8(x + 472);
    *B(slot + 13) = (uint8_t)S8(x + 469);
    *B(slot + 14) = (uint8_t)S8(x + 470);
    *B(slot + 15) = (uint8_t)S8(x + 471);
out:
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* A random character for a place on the list: one of 0x2A that 0x0882F024 lets through for the
   place with the table, as 0x0882EB74 makes it for the choice. `back`: the three calls' return
   addresses. */
static uint32_t random_for_place(Cpu *c, uint32_t place, uint32_t table, uint32_t choice_at, const uint32_t back[3]) {
    uint32_t ch;
    do {
        call0(c, f_08821368, back[0]);
        DIV(c, c->r[2], 0x2A);
        ch = c->hi & 0xFF;
        c->r[16] = ch;
        call3(c, ch, place, table, f_0882F024, back[1]);
    } while (c->r[2] == 0);
    call2(c, ch, *B(choice_at), f_0882EB74, back[2]);
    return c->r[2];
}

/* (-, y): random pairs of opponents. At the very start of a run (the count at PAIR is 0)
   0x08836538 runs and y's +33, +34 and +80 are drawn, with 32 times +80's low two bits kept at
   0x08B01F48. y's +33 (three bits) picks how many pairs (the bytes at 0x08A0F414; y + 72 is one
   less) and each entry's first byte (three a pick from 0x08A0F41C). Each pair then gets a random
   choice (-1, 0 or 1) and two random characters for its two places (the table 0x08A0F408 below a
   count of 100, 0x08A0F40F from there on), kept in WHO and at the entry's +2 and +3; the entry's
   +1 is set one time in sixteen; and 0x08831A78 (y, 0) runs after each. A 0x50 frame keeping
   s0..s7 and fp; the pair's number, how many, y and the three lists' addresses are kept at
   +16..+36 of it. */
SFA_FUNC(08830FE4, random_pairs) {
    static const uint32_t back_late[2][3] = { { 0x08831190u, 0x088311ACu, 0x088311C0u },
                                              { 0x088311D4u, 0x088311F0u, 0x08831204u } };
    static const uint32_t back_early[2][3] = { { 0x0883121Cu, 0x08831238u, 0x0883124Cu },
                                               { 0x08831260u, 0x0883127Cu, 0x08831290u } };
    gaddr y = ARG(1);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    uint32_t count = *W(PAIR);
    for (uint32_t i = 0; i < 8; i++) *W(sp + 40 + 4 * i) = sv[i];
    *W(sp + 72) = sv[8]; *W(sp + 76) = sv[9];
    c->r[16] = y;
    if (count == 0) {
        ARG(0) = 0;
        call0(c, f_08836538, 0x08831028u);
        call0(c, f_08821368, 0x08831030u);
        *B(y + 33) = (uint8_t)c->r[2];
        call0(c, f_08821368, 0x08831038u);
        *B(y + 34) = (uint8_t)c->r[2];
        call0(c, f_08821368, 0x08831040u);
        *B(y + 80) = (uint8_t)c->r[2];
        *W(0x08B01F48u) = ((uint32_t)S8(y + 80) & 3) << 5;
    }
    uint32_t pick = (uint32_t)S8(y + 33) & 7;
    int32_t pairs = S8(0x08A0F414u + pick);
    *W(sp + 20) = (uint32_t)pairs;
    for (int32_t j = 0; j < pairs; j = (int16_t)(j + 1))
        *B(ENTRIES + ((uint32_t)j << 2)) = (uint8_t)S8(0x08A0F41Cu + pick * 3 + (uint32_t)j);
    *W(sp + 24) = y;
    *H(y + 72) = (uint16_t)(pairs - 1);
    *H(sp + 16) = 0;
    if (0 < pairs) {
        *W(sp + 32) = HOW; *W(sp + 28) = WHO; *W(sp + 36) = ENTRIES;
        c->r[20] = 0x08A0F408u; c->r[21] = 0x08A0F40Fu; c->r[19] = 0x2A;
        int32_t i;
        do {
            call0(c, f_08821368, 0x08831134u);
            DIV(c, c->r[2], 3);
            i = S16(sp + 16);
            gaddr how = *W(sp + 32) + (uint32_t)i;
            gaddr entry = *W(sp + 36) + ((uint32_t)i << 2);
            gaddr who = *W(sp + 28) + (uint32_t)(i * 2);
            uint32_t place = (uint32_t)(i * 2) & 0xFF, place2 = (uint32_t)(i * 2 + 1) & 0xFF;
            c->r[23] = how; c->r[30] = entry; c->r[22] = who; c->r[17] = place; c->r[18] = place2;
            int early = (int32_t)*W(PAIR) < 100;
            *B(how) = (uint8_t)(c->hi - 1);
            uint32_t table = early ? 0x08A0F408u : 0x08A0F40Fu;
            const uint32_t (*back)[3] = early ? back_early : back_late;
            *B(who) = (uint8_t)random_for_place(c, place, table, how, back[0]);
            *B(entry + 2) = *B(who);
            *B(who + 1) = (uint8_t)random_for_place(c, place2, table, how, back[1]);
            *B(entry + 3) = *B(who + 1);
            call0(c, f_08821368, 0x088312A4u);
            *B(entry + 1) = (c->r[2] & 0xF) == 0 ? 1 : 0;
            pairs = (int32_t)*W(sp + 20);
            c->r[16] = (uint32_t)pairs;
            call2(c, *W(sp + 24), 0, f_08831A78, 0x088312CCu);
            i = (int16_t)(S16(sp + 16) + 1);
            *H(sp + 16) = (uint16_t)i;
        } while (i < pairs);
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x50;
}
