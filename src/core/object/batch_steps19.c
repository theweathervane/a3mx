/* Small steps (0.148.0): the 1 function of 9 - 100 instructions made ready by the batch before and the
   next 2 of 101 - 200. A text scroller's start, the fade slots for flagged fighters by layout, and the
   network menu's status line. Offsets stay numbers until something shows what they are. What the
   original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define NET       0x09523980u

void f_089DDEEC(Cpu *c);   /* text_scroller */
void f_0882DE00(Cpu *c);   /* fade_slots */
void f_0882E0D0(Cpu *c);   /* fade_reset */
void f_0880A6CC(Cpu *c);   /* game_text */

/* (scroller): started (+4 = 1, +40 / +1 cleared) and run once. */
SFA_FUNC(089DDEC4, text_scroller_start) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *B(o + 4) = 1; *H(o + 40) = 0; *B(o + 1) = 0;
    c->r[5] = 1;
    f_089DDEEC(c);
    c->r[29] += 0x20;
}

static void fade(Cpu *c, uint32_t first) { c->r[4] = first; c->r[5] = 8; c->r[6] = 0xFF; f_0882DE00(c); }

/* (): with BATTLE +37 (a fighter's +237 flag), slot 0x19 faded full and the flagged fighters' runs
   (8 slots from 0, 0x40 or 0x48) by the layout BATTLE +56; else the fade reset. */
SFA_FUNC(0882DB4C, fade_flagged_fighters) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = BATTLE; c->r[17] = BATTLE;
    if (S8(BATTLE + 37) == 0) f_0882E0D0(c);
    else {
        c->r[16] = 1;
        c->r[4] = 0x19; c->r[5] = 1; c->r[6] = 0xFF; f_0882DE00(c);
        int32_t l = S16(BATTLE + 56);
        int f0 = S8(FIGHTER0 + 237) != 0, f1 = S8(FIGHTER1 + 237) != 0;
        int f2 = S8(FIGHTER2 + 237) != 0, f3 = S8(FIGHTER3 + 237) != 0;
        if (l == 0) {
            if (f0) fade(c, 0);
            if (S8(FIGHTER1 + 237) != 0) fade(c, 0x40);
        } else if (l == 1 || l == 2) {
            if (f2 || f3) { fade(c, 0x40); fade(c, 0x48); }
            else if (l == 1 ? f0 : f1) fade(c, 0);
        } else if (l == 3) {
            if (f2) fade(c, 0x48);
            else if (f0 || f1) { fade(c, 0); fade(c, 0x40); }
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): the network menu's status line at 0xC0 by the state byte 0x08B03F8D: 0 - 7 through the table
   0x08A016D0 (texts 0x08A015DC on, 8 bytes apart), 129 0x08A015CC, 130 0x08A015D4; y from the row's
   byte +4 in 0x08A3EA34 (11 bytes a row by NET +47). */
SFA_FUNC(088ECD14, net_status_text) {
    c->r[29] -= 0x30;
    int32_t n = S8(NET + 47);
    uint32_t y = *B(0x08A3EA34u + (uint32_t)n * 11 + 4);
    uint32_t v = *B(0x08B03F8Du);
    uint32_t txt = 0;
    if (v < 129) {
        if (v < 8) {
            uint32_t to = *W(0x08A016D0u + v * 4);
            switch (to) {
            case 0x088ECDE4u: txt = 0x08A015DCu; break;
            case 0x088ECE0Cu: txt = 0x08A015E4u; break;
            case 0x088ECE34u: txt = 0x08A015ECu; break;
            case 0x088ECE5Cu: txt = 0x08A015F4u; break;
            case 0x088ECE84u: txt = 0x08A015FCu; break;
            case 0x088ECEACu: txt = 0x08A01604u; break;
            case 0x088ECED4u: txt = 0x08A0160Cu; break;
            case 0x088ECEFCu: txt = 0x08A01614u; break;
            default: rt_bad_jump(c, to, 0x088ECD80u); return;
            }
        }
    } else if (v < 130) txt = 0x08A015CCu;
    else if (v < 131) txt = 0x08A015D4u;
    if (txt != 0) {
        c->r[4] = txt; c->r[5] = 1; c->r[6] = 0xC0; c->r[7] = y; c->r[8] = 0xB;
        f_0880A6CC(c);
    }
    c->r[29] += 0x30;
}
