/* Two more steps that start music: the start of a pair's game, and a fight's win. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define F3     0x092E47D0u
#define F4     0x092E4C68u
#define PAIR   0x095179E0u

void f_0887C6E4(Cpu *c); void f_08823338(Cpu *c); void f_088D2B54(Cpu *c); void f_08821F58(Cpu *c);
void f_0888D5B0(Cpu *c); void f_088D7CE0(Cpu *c); void f_0882E110(Cpu *c); void f_08821368(Cpu *c);
void f_0887C680(Cpu *c); void f_088969BC(Cpu *c); void f_0882DB10(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): the music 0x100 starts and a's step (+4) goes on; the player's fighter (the first
   when a's +53 is odd, else the second) has +413 set; the pair's record is cleared; fifteen
   pieces are asked for (six of kind 6, nine of kind 0xE); b's +90 and +20 are cleared, a's +53
   kept at 0x08B01F36, b's +79 made 1 if it was 0, and the byte at 0x08B01F34 set to where b's
   +79 stands in the list of twelve at 0x08A0E778; then the routine at 0x088D2B54. A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(0882BED4, pair_game_start) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 24) = s2;
    c->r[18] = a; c->r[16] = b;
    *W(sp + 20) = s1; *W(sp + 28) = s3; *W(sp + 32) = ra;
    call1(c, 0x100, f_0887C6E4, 0x0882BEFCu);
    *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
    c->r[19] = 1;
    c->r[17] = 0x08B00000u;
    *B(((S8(a + 53) & 1) != 0 ? F1 : F2) + 413) = 1;
    *W(PAIR) = 0;
    *H(PAIR + 10) = 0;
    *W(PAIR + 4) = 0;
    *H(PAIR + 8) = 0;
    *B(PAIR + 14) = 0; *B(PAIR + 13) = 0; *B(PAIR + 12) = 0; *B(PAIR + 15) = 0;
    static const uint8_t pieces[15][2] = {
        { 6, 0x10 }, { 6, 0x11 }, { 6, 0x12 }, { 6, 0x13 }, { 6, 0x14 }, { 6, 0x19 },
        { 0xE, 0 }, { 0xE, 1 }, { 0xE, 2 }, { 0xE, 3 }, { 0xE, 4 }, { 0xE, 5 }, { 0xE, 6 }, { 0xE, 7 }, { 0xE, 8 },
    };
    for (uint32_t i = 0; i < 15; i++) call2(c, pieces[i][0], pieces[i][1], f_08823338, 0x0882BF74u + 0xC * i);
    *B(0x08B01F34u) = 0;
    *B(b + 90) = 0;
    *H(b + 20) = 0;
    *B(0x08B01F36u) = *B(a + 53);
    if (S8(b + 79) == 0) *B(b + 79) = 1;
    *B(0x08B01F34u) = 0;
    int32_t at = S8(0x08B01F34u);
    while (at < 12) {
        if ((uint32_t)S8(b + 79) == *B(0x08A0E778u + (uint32_t)at)) break;
        *B(0x08B01F34u) = (uint8_t)(at + 1);
        at = S8(0x08B01F34u);
    }
    c->r[31] = 0x0882C088u; f_088D2B54(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, b): a fight is won: a's step (+6) goes on with a wait of 0x3C frames (+18), the four
   fighters' +1 cleared, the screen made ready. When the routine at 0x088D7CE0 says so the step
   becomes 4 with no wait, and that is all. Else the winner (b's +112) has its routine by
   character run and, unless its +373 is set, a random pose (0 to 3 at b's +102) when its +426
   asks for one; the sixteen objects of kind 7 have +9 set; the music 0x132 starts (and, with
   a's +69 clear, a's +130 is cleared and the routine at 0x0887C680 runs). Then, in the cases
   below, the step becomes 4 with no wait: b's +79 set; game mode 0x12, when a's +69 is set or
   b's +85 is not below 0; game mode 0x10 with b's +76 set, which also asks for the voice
   0x68E plus four times the battle's +102; and b's +85 below 0 with a's +69 clear. A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(08827C30, fight_won) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = a;
    *W(sp + 16) = s0;
    c->r[16] = b;
    *W(sp + 28) = s3;
    gaddr winner = *W(b + 112);
    c->r[19] = winner;
    *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
    *H(a + 18) = 0x3C;
    *B(F4 + 1) = 0; *B(F3 + 1) = 0; *B(F2 + 1) = 0; *B(F1 + 1) = 0;
    *B(b + 81) = 0;
    *B(b + 15) = 0;
    ARG(0) = 1; ARG(1) = 0xFFFFFFFFu;
    *B(a + 43) = 1;
    *B(a + 72) = 0xFF;
    *W(sp + 24) = s2; *W(sp + 32) = ra;
    *B(b + 37) = 1;
    c->r[31] = 0x08827CB8u; f_08821F58(c);
    c->r[31] = 0x08827CC0u; f_0888D5B0(c);
    c->r[31] = 0x08827CC8u; f_088D7CE0(c);
    if (c->r[2] != 0) {
        *H(a + 6) = 4;
        *H(a + 18) = 0;
    } else {
        call1(c, (uint32_t)S8(winner + 338), f_0882E110, 0x08827CD8u);
        c->r[18] = 0x10;
        if (S8(winner + 373) == 0 && (S16(winner + 426) & 3) == 3) {
            c->r[31] = 0x08827D30u; f_08821368(c);
            *H(b + 102) = (uint16_t)(c->r[2] & 3);
        }
        gaddr rec = 0x09514B70u;
        for (int i = 0; i < 16; i++, rec += 0xC0)
            if (*B(rec) != 0 && *H(rec + 24) == 7 && *B(rec + 9) == 0) *B(rec + 9) = 1;
        call1(c, 0x132, f_0887C6E4, 0x08827D8Cu);
        if (S8(a + 69) == 0) {
            *H(a + 130) = 0;
            c->r[31] = 0x08827DA0u; f_0887C680(c);
        }
        int32_t mode = S8(a + 114);
        int at_once;
        if (S8(b + 79) != 0) at_once = 1;
        else if (S8(a + 69) == 0 && S8(b + 85) < 0) at_once = 1;
        else if (mode == 0x12) at_once = 1;
        else at_once = mode == 0x10 && S8(b + 76) != 0;
        if (at_once) {
            *H(a + 6) = 4;
            *H(a + 18) = 0;
            if (mode == 0x10) {
                ARG(0) = ((uint32_t)S16(BATTLE + 102) << 2) + 0x68E;
                ARG(1) = 0x1C; ARG(2) = 1;
                c->r[31] = 0x08827E14u; f_088969BC(c);
            }
        }
        c->r[31] = 0x08827E1Cu; f_0882DB10(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}
