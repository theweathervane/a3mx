/* The link play's lobby: the game made ready once a match is agreed. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define F3     0x092E47D0u
#define F4     0x092E4C68u
#define NETP   0x09523980u

void f_088D5D18(Cpu *c); void f_088D5ACC(Cpu *c); void f_088E516C(Cpu *c); void f_0887C6E4(Cpu *c);
void f_0883DA0C(Cpu *c); void f_0880B57C(Cpu *c); void f_08823304(Cpu *c); void f_08822934(Cpu *c);
void f_088DD8F4(Cpu *c); void f_088DC0E0(Cpu *c); void f_08812DDC(Cpu *c); void f_08805564(Cpu *c);
void f_088054F0(Cpu *c); void f_0882134C(Cpu *c); void f_088A063C(Cpu *c); void f_088D5A54(Cpu *c);
void f_088DD114(Cpu *c); void f_08804A74(Cpu *c); void f_08805614(Cpu *c); void f_088DC42C(Cpu *c);
void f_08815374(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }

/* (game, battle, link): the fight is set from what the two machines agreed: one of two
   routines by the link's +42; the second fighter's +373 is set when the link's +43 is 2 or 4
   and its +36 is set (in kind 0xC, or with the link's +34 set); the stage comes from the
   table the routine at 0x088E516C gives, by the link's +37; the music 0x132 starts and the
   game's step becomes 1, 2. A 0x40 frame keeping s0..s5, with the two fighters at its +16
   and +20. */
SFA_FUNC(088D5F28, link_fight_set) {
    gaddr game = ARG(0), battle = ARG(1), link = ARG(2);
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 16) = F1;
    *W(sp + 36) = sv[3]; *W(sp + 44) = sv[5];
    c->r[7] = F2;
    c->r[8] = *B(link + 42);
    *W(sp + 28) = sv[1]; *W(sp + 32) = sv[2]; *W(sp + 40) = sv[4];
    *W(sp + 20) = F2;
    c->r[20] = 1;
    c->r[18] = game;
    c->r[17] = battle;
    c->r[19] = NETP;
    *W(sp + 24) = sv[0]; *W(sp + 48) = ra;
    c->r[16] = link;
    ARG(0) = game; ARG(1) = battle; ARG(2) = link;
    if (c->r[8] == 1) call0(c, f_088D5D18, 0x088D5F98u);
    else call0(c, f_088D5ACC, 0x088D5FB0u);
    c->r[21] = (uint32_t)S8(GAME + 114);
    if (c->r[21] == 0xC || *B(NETP + 34) != 0) {
        uint32_t v = *B(NETP + 43);
        if ((v == 2 || v == 4) && S8(NETP + 36) != 0) *B(*W(sp + 20) + 373) = 1;
    }
    call1(c, 0, f_088E516C, 0x088D6000u);
    uint32_t stage = *B(c->r[2] + *B(link + 37) + 14);
    *H(battle) = (uint16_t)stage;
    *H(battle + 98) = (uint16_t)stage;
    *H(battle + 100) = (uint16_t)stage;
    ARG(1) = stage;
    call1(c, 0x132, f_0887C6E4, 0x088D6024u);
    *H(game) = 1;
    *H(game + 2) = 2;
    ARG(1) = 2;
    call1(c, 1, f_0883DA0C, 0x088D6038u);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}

/* The game is made ready for a link match: the screens put away, the game's steps, counters
   and flags cleared, the four fighters numbered 0 to 3, the random numbers seeded from the
   link's +50 and +55, the game's kind (+114) taken from the routine at 0x088D5A54 (the
   battle's +155 says whether it is 6). The game's +53 and +41 become 7 when the link's +42 is
   2 or 3 and two or more players answered, else 1 when the link's +36 is set, else 3. With the
   link's +45 set and its +37 not 1 the fight is set at once. Last, the frame's routine becomes
   the one at 0x088233EC. A 0x30 frame keeping s0..s5. */
SFA_FUNC(088D821C, link_game_ready) {
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 24) = sv[2];
    c->r[20] = GAME;
    c->r[19] = BATTLE;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 36) = sv[5]; *W(sp + 40) = ra;
    c->r[18] = NETP;
    call0(c, f_0880B57C, 0x088D8258u);
    call0(c, f_08823304, 0x088D8260u);
    call0(c, f_08822934, 0x088D8268u);
    call0(c, f_088DD8F4, 0x088D8270u);
    call0(c, f_088DC0E0, 0x088D8278u);
    call1(c, 0x12, f_08812DDC, 0x088D8280u);
    call1(c, 2, f_08805564, 0x088D8288u);
    call1(c, 1, f_088054F0, 0x088D8290u);
    *B(GAME + 50) = 0; *B(GAME + 72) = 0; *B(GAME + 136) = 0;
    *B(GAME + 43) = 0; *B(GAME + 42) = 0; *B(GAME + 41) = 0;
    *H(BATTLE) = 0;
    for (int32_t at = 22; at >= 0; at -= 2) *H(GAME + (uint32_t)at) = 0;
    *B(GAME + 93) = 0; *B(GAME + 92) = 0;
    *B(GAME + 143) = 0; *B(GAME + 142) = 0; *B(GAME + 141) = 0;
    *B(BATTLE + 105) = 0;
    *B(GAME + 137) = 0;
    uint32_t v = (uint32_t)S8(GAME + 45);
    *B(GAME + 35) = 0;
    *H(GAME) = 0;
    *B(GAME + 73) = (uint8_t)v;
    *B(NETP + 48) = (uint8_t)v;
    *H(NETP + 52) = 0;
    *H(BATTLE + 56) = 0;
    *B(BATTLE + 90) = 0;
    c->r[17] = 1;
    *B(GAME + 52) = 1;
    *B(NETP + 41) = 0;
    *B(F1 + 337) = 0;
    *B(F2 + 337) = 1;
    c->r[21] = 2;
    *B(F3 + 337) = 2;
    c->r[16] = 3;
    *B(F4 + 337) = 3;
    *B(GAME + 146) = 1;
    *B(GAME + 136) = 1;
    *B(GAME + 147) = 0;
    ARG(1) = *B(NETP + 55);
    call1(c, (uint32_t)S16(NETP + 50) + *B(NETP + 55), f_0882134C, 0x088D8380u);
    call0(c, f_088A063C, 0x088D8388u);
    call1(c, NETP, f_088D5A54, 0x088D8390u);
    *B(GAME + 114) = (uint8_t)c->r[2];
    *B(BATTLE + 155) = S8(GAME + 114) == 6;
    uint32_t kind = *B(NETP + 42);
    uint32_t set = 0;
    if (kind == 3 || kind == 2) {
        call0(c, f_088DD114, 0x088D83C0u);
        if ((int32_t)c->r[2] >= 2) set = 7;
    }
    if (set == 0) set = S8(NETP + 36) != 0 ? 1 : 3;
    *B(GAME + 53) = (uint8_t)set;
    *B(GAME + 41) = (uint8_t)set;
    c->r[16] = *B(NETP + 45);
    if (c->r[16] != 0 && *B(NETP + 37) != 1) {
        ARG(0) = GAME; ARG(1) = BATTLE; ARG(2) = NETP;
        call0(c, f_088D5F28, 0x088D8420u);
    }
    call1(c, 1, f_08804A74, 0x088D8428u);
    call1(c, 0x088233ECu, f_08805614, 0x088D8434u);
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (o): the lobby's last screen: its first step puts the lobby away; then the script goes on
   (or to the one at 0x08A3EB24 when the link's +43 is 2 or 4) and the game is made ready. A
   step past 1 sets the script at 0x08A3EB14. A 0x10 frame keeping s0. */
SFA_FUNC(088D7520, lobby_leave_screen) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0;
    c->r[16] = o;
    uint32_t step = *H(o);
    *W(sp + 4) = ra;
    if (step >= 2) {
        *W(o + 20) = 0x08A3EB14u;
        *H(o + 24) = 0;
        *H(o + 2) = 0;
        *H(o) = 0;
        *B(o + 47) = 0;
    } else {
        if (step == 0) {
            call1(c, 0x08A40000u, f_088DC42C, 0x088D7568u);
            call0(c, f_088DC0E0, 0x088D7570u);
            call0(c, f_08815374, 0x088D7578u);
            *H(o + 4) = 0;
            *H(o) = (uint16_t)(*H(o) + 1);
        }
        uint32_t v = *B(NETP + 43);
        if (v == 2 || v == 4) {
            *W(o + 20) = 0x08A3EB24u;
            *H(o + 24) = 0;
            *H(o + 2) = 0;
            *H(o) = 0;
            *B(o + 47) = 0;
        } else {
            *H(o + 2) = 0;
            *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
            *H(o) = 0;
            *B(o + 47) = 0;
        }
        call0(c, f_088D821C, 0x088D75E4u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}
