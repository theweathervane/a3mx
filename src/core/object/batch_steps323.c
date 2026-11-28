/* The last two the small ones set free: a round's end for the winner (one on one, and the fights
   with partners), and the link play's own frame for a peer. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define NETP      0x09523980u

void f_08826184(Cpu *c); void f_088261A0(Cpu *c); void f_08825FF8(Cpu *c); void f_08830334(Cpu *c);
void f_08830348(Cpu *c); void f_088E0C6C(Cpu *c); void f_088D59A0(Cpu *c); void f_088E0E64(Cpu *c);
void f_088E29A0(Cpu *c); void f_088E27DC(Cpu *c); void f_08804F50(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* A count up by one, 99 at most: the wins at +426 (kept at +451 too). */
static void one_more_win(gaddr f) {
    int32_t w = S16(f + 426) + 1;
    *H(f + 426) = (uint16_t)w;
    if (!((int16_t)w < 100)) *H(f + 426) = 0x63;
    *B(f + 451) = (uint8_t)S16(f + 426);
}

/* The winner's result goes to the records: 0x08830348 with what 0x08830334 makes of the loser's
   character. */
static void record_win(Cpu *c, gaddr winner, gaddr loser, uint32_t back0, uint32_t back1) {
    call1(c, (uint32_t)(int32_t)S8(loser + 338), f_08830334, back0);
    call2(c, winner, c->r[2], f_08830348, back1);
}

/* The list of opponents is through (b's +20 has reached +72, unless b's +85 is set). */
static void list_done(gaddr a, gaddr b) {
    if (S8(b + 85) == 0 && S16(b + 20) == S16(b + 72)) { *B(b + 70) = 0xFF; *B(a + 72) = 0xFF; }
}

/* (a, b, fighter, fighter): a round's end. The winner is the one b's +12 says (odd: the first);
   0x08826184 gives a third fighter, the winner's partner. Winner and loser are kept (b + 112,
   + 116, their characters at + 50, + 51) and marked 1 and -1 at +1172.

   One on one (b + 56 = 0): b's +83 says whether the winner kept full vitality (0x90), a player
   who lost is marked (+407), and the winner's rounds (+374) go up. With the rounds needed (b + 8)
   the fight is over (b + 5 = -1): in the first mode (b + 79) on the last stage (b + 2) b's +32 is
   set and, outside mode 0x12, b's +55 counted. Then with b's +61 the winner's wins and its +437 go
   up (99 at most) and the loser's are cleared; without it: mode 0x12 counts a win and ends the
   list when it is through; else for a player who won, mode 7 has its own end (0x08825FF8) and the
   others record the win and end the list when it is through.

   With partners: BATTLE + 64 says who is still standing: for the kinds 1 and 2 with the winner on
   the far side, 2 when fighter 4 is down and 3 up, 3 the other way round; for the kind 3 with the
   winner on the near side, 0 or 1 for fighter 1 or 2 down while the other stands (in mode 0xF, and
   0x10 with BATTLE + 159 = 3, the cpu's one counts as down). b's +83 is cleared, and for the kind
   3 the character at b + 50 is the partner's when b's +64 is odd (other kinds: when it is 3). Then
   0x088261A0 says how it ended: bit 0 clear, the partner lost with the loser (both marked); set,
   the partner won as well (its rounds go up too). The rounds, the fight's end, the wins and the
   records follow as above, the records only for the right kind (not 3 when the partner lost, 3
   when it won, where the partner also takes the winner's +400 and +404) and with bit 1 clear.
   A 0x50 frame keeping s0..s7 and fp. */
SFA_FUNC(08829D70, round_end) {
    gaddr a = ARG(0), b = ARG(1), winner = ARG(2), loser = ARG(3);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    uint32_t first_won = (uint32_t)S8(b + 12) & 1;
    *W(sp + 32) = sv[0]; *W(sp + 36) = sv[1]; *W(sp + 40) = sv[2];
    *W(sp + 52) = sv[5]; *W(sp + 56) = sv[6]; *W(sp + 60) = sv[7];
    *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4]; *W(sp + 64) = sv[8]; *W(sp + 68) = sv[9];
    c->r[21] = 1; c->r[22] = 0xFFFFFFFFu; c->r[23] = 0x90;
    c->r[16] = a; c->r[17] = b;
    c->r[8] = first_won;
    if (!first_won) { gaddr t = winner; winner = loser; loser = t; }
    c->r[18] = winner; c->r[19] = loser;
    call1(c, b, f_08826184, 0x08829DD8u);
    gaddr partner = c->r[2];
    *W(b + 112) = winner; *W(b + 116) = loser;
    *B(b + 50) = (uint8_t)S8(winner + 338);
    *B(b + 51) = (uint8_t)S8(loser + 338);
    *B(winner + 1172) = 1;
    *B(loser + 1172) = 0xFF;
    int32_t kind = S16(b + 56);
    c->r[20] = partner;
    if (kind == 0) {
        *B(b + 83) = S16(winner + 112) == 0x90 ? 0xFF : 0;
        if (S8(loser + 373) == 0) *B(loser + 407) = 0xFF;
        int32_t rounds = S8(winner + 374) + 1;
        int32_t need = S8(b + 8);
        *B(winner + 374) = (uint8_t)rounds;
        if ((int8_t)rounds != need) goto out;
        *B(b + 5) = 0xFF;
        int32_t mode1 = S8(b + 79);
        *B(b + 32) = 0;
        int32_t counted = S8(b + 61);
        if (mode1 == 0 && need - 1 == S16(b + 2)) {
            *B(b + 32) = 0xFF;
            if (S8(a + 114) != 0x12) *B(b + 55) = (uint8_t)(S8(b + 55) + 1);
        }
        if (counted != 0) {
            int32_t w = S16(winner + 426) + 1;
            *H(winner + 426) = (uint16_t)w;
            if (!((int16_t)w < 100)) *H(winner + 426) = 0x63;
            int32_t v = S8(winner + 437) + 1;
            *B(winner + 437) = (uint8_t)v;
            if (!((int8_t)v < 100)) *B(winner + 437) = 0x63;
            *B(winner + 451) = (uint8_t)S16(winner + 426);
            *H(loser + 426) = 0;
            goto out;
        }
        int32_t mode = S8(a + 114);
        if (mode == 0x12) {
            if (S16(b + 20) == S16(b + 72)) { *B(b + 70) = 0xFF; *B(a + 72) = 0xFF; }
            *H(winner + 426) = (uint16_t)(S16(winner + 426) + 1);
        } else if (S8(winner + 373) == 0) {
            if (mode == 7) {
                ARG(0) = a; ARG(1) = b; ARG(2) = winner; ARG(3) = loser; c->r[8] = partner;
                c->r[31] = 0x08829F44u; f_08825FF8(c);
            } else {
                record_win(c, winner, loser, 0x08829F54u, 0x08829F60u);
                list_done(a, b);
            }
        }
        goto out;
    }
    c->r[30] = 3;
    if (kind == 1 || kind == 2) {
        if (S8(winner + 337) & 2) {
            int32_t hp3 = S16(FIGHTER_3 + 112), hp4 = S16(FIGHTER_4 + 112);
            if (hp4 < 0 && !(hp3 < 0)) *B(BATTLE + 64) = 2;
            if (hp3 < 0 && !(hp4 < 0)) *B(BATTLE + 64) = 3;
        }
    } else if (kind == 3) {
        if (!(S8(winner + 337) & 2)) {
            int32_t mode = S8(GAME + 114);
            int32_t hp1 = S16(FIGHTER_1 + 112), hp2 = S16(FIGHTER_2 + 112);
            if (mode == 0xF || (mode == 0x10 && S8(BATTLE + 159) == 3)) {
                if ((S8(FIGHTER_2 + 373) != 0 || hp2 < 0) && !(hp1 < 0)) *B(BATTLE + 64) = 0;
                if ((S8(FIGHTER_1 + 373) != 0 || S16(FIGHTER_1 + 112) < 0) && !(S16(FIGHTER_2 + 112) < 0))
                    *B(BATTLE + 64) = 1;
            } else {
                if (hp2 < 0 && !(hp1 < 0)) *B(BATTLE + 64) = 0;
                if (hp1 < 0 && !(hp2 < 0)) *B(BATTLE + 64) = 1;
            }
        }
    }
    kind = S16(b + 56);
    *B(b + 83) = 0;
    {
        int32_t alive = S8(b + 64);
        if (kind == 3 ? (alive & 1) != 0 : alive == 3) *B(b + 50) = (uint8_t)S8(partner + 338);
    }
    call1(c, b, f_088261A0, 0x0882A188u);
    uint32_t res = c->r[2];
    int both = (res & 1) != 0;
    if (!both) {
        *B(loser + 407) = 0xFF;
        *B(partner + 407) = 0xFF;
        *B(partner + 1172) = 0xFF;
        if (S16(winner + 112) == 0x90) *B(b + 83) = 0xFF;
        *B(winner + 374) = (uint8_t)(S8(winner + 374) + 1);
    } else {
        if (S16(winner + 112) == 0x90 && S16(partner + 112) == 0x90) *B(b + 83) = 0xFF;
        *B(winner + 374) = (uint8_t)(S8(winner + 374) + 1);
        int32_t theirs = S8(partner + 374);
        *B(partner + 1172) = 1;
        *B(partner + 374) = (uint8_t)(theirs + 1);
    }
    int32_t rounds = S8(winner + 374);
    c->r[21] = (uint32_t)rounds;
    int32_t need = S8(b + 8);
    if (need != rounds) goto out;
    *B(b + 5) = 0xFF;
    *B(b + 32) = 0;
    {
        int32_t counted = S8(b + 61);
        if (need - 1 == S16(b + 2)) {
            int32_t n = S8(b + 55);
            *B(b + 32) = 0xFF;
            *B(b + 55) = (uint8_t)(n + 1);
        }
        if (counted != 0) {
            one_more_win(winner);
            if (!both) { *H(loser + 426) = 0; *H(partner + 426) = 0; }
            else { one_more_win(partner); *H(loser + 426) = 0; }
            goto out;
        }
    }
    if ((S16(b + 56) == 3) != both || (res & 2)) goto out;
    if (S8(a + 114) == 7) {
        ARG(0) = a; ARG(1) = b; ARG(2) = winner; ARG(3) = loser; c->r[8] = partner;
        c->r[31] = both ? 0x0882A3E0u : 0x0882A350u; f_08825FF8(c);
    } else {
        if (!both) record_win(c, winner, loser, 0x0882A300u, 0x0882A30Cu);
        else {
            record_win(c, winner, loser, 0x0882A380u, 0x0882A38Cu);
            *W(partner + 400) = *W(winner + 400);
            *H(partner + 404) = (uint16_t)S16(winner + 404);
        }
        list_done(a, b);
    }
out:
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0x50;
}

/* One pad's place in the input buffer: a word of two halves and a clear word, passed to
   0x08804F50 with the pad. */
static void pad_reset(Cpu *c, gaddr pad, gaddr slot, uint16_t n, uint32_t back) {
    *H(slot) = n; *H(slot + 2) = 1;
    *W(slot + 4) = 0;
    call2(c, pad, slot, f_08804F50, back);
}

/* (x, peer): the link play's frame for a peer. Nothing (0) while x's +44 is set; 1 for a peer
   other than the one we play against (NETP + 12). Else 0x088E0C6C (1), and while x's +49 is clear
   or the lead +32 is above 0: our input goes out, unless 0x088D59A0 says to hold it on this count
   (+34; every count the byte at 0x08A3EBA8 divides, while the lead is not above 0): the word at x
   with x's +38 (two bits) and a check byte made of the frame, GAME + 64 and the count, through
   0x088E0E64. A send that fails reports (0x088E0C6C 0), counts a wait at +28 and returns 0. A sent
   one clears the word and counts; with a lead it is used up by one and the three pads' buffers
   (0x08E75C28) are reset, with x's +4..+12 cleared (1). Otherwise 0x088E29A0, and when that says
   0 the wait for the peers' answers (0x088E27DC with peer + 0xD4); else 1. A 0x30 frame keeping
   s0..s3; the word goes out from +16 of it. */
SFA_FUNC(088E2AE4, link_frame) {
    gaddr x = ARG(0), peer = ARG(1);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[3];
    int32_t off = S8(x + 44);
    *W(sp + 24) = sv[1];
    c->r[17] = peer + 0xD4; c->r[19] = GAME;
    *W(sp + 20) = sv[0]; *W(sp + 28) = sv[2]; *W(sp + 36) = sv[4];
    c->r[16] = x;
    if (off != 0) { c->r[2] = 0; goto out; }
    c->r[18] = NETP;
    if (*W(peer + 228) != *W(NETP + 12)) { c->r[2] = 1; goto out; }
    call1(c, 1, f_088E0C6C, 0x088E2B34u);
    if (S8(x + 49) == 0 || S8(x + 32) > 0) {
        uint32_t count = *B(x + 34);
        c->r[18] = count;
        call2(c, NETP, count, f_088D59A0, 0x088E2BA0u);
        uint32_t now = *B(x + 34);
        uint32_t hold = 0;
        if (c->r[2] != 0) {
            uint32_t every = *B(0x08A3EBA8u);
            if (every != 0 && !(S8(x + 32) > 0)) {
                DIV(c, count, every);
                hold = c->hi < 1;
            }
        }
        c->r[18] = hold;
        if (hold) *B(x + 34) = (uint8_t)(now + 1);
        else {
            uint32_t word = *W(x) & 0xFFFF;
            word += ((uint32_t)S8(x + 38) & 3) << 16;
            word += (((uint32_t)*B(GAME + 32) + (uint32_t)S16(GAME + 64) + now) & 0xFF) << 24;
            *W(sp + 16) = word;
            call2(c, sp + 0x10, 1, f_088E0E64, 0x088E2C34u);
            if ((int32_t)c->r[2] <= 0) {
                call1(c, 0, f_088E0C6C, 0x088E2D28u);
                *H(x + 28) = (uint16_t)(*H(x + 28) + 1);
                c->r[2] = 0;
                goto out;
            }
            uint8_t n = *B(x + 34);
            *W(x) = 0;
            *B(x + 34) = (uint8_t)(n + 1);
            int32_t lead = S8(x + 32);
            if (lead > 0) {
                const gaddr slots = 0x08E75C28u, pads = 0x08E75BA0u;
                *B(x + 32) = (uint8_t)(lead - 1);
                call1(c, 0, f_088E0C6C, 0x088E2C94u);
                c->r[17] = slots; c->r[18] = 1; c->r[19] = pads;
                pad_reset(c, pads, slots, 0, 0x088E2CC0u);
                pad_reset(c, pads + 0x2C, slots + 8, 1, 0x088E2CD8u);
                pad_reset(c, pads + 0x58, slots + 0x10, 2, 0x088E2CF4u);
                *W(x + 4) = 0; *W(x + 8) = 0; *W(x + 12) = 0;
                c->r[2] = 1;
                goto out;
            }
        }
    }
    call1(c, x, f_088E29A0, 0x088E2C5Cu);
    if (c->r[2] == 0) call2(c, x, peer + 0xD4, f_088E27DC, 0x088E2D5Cu);
    else c->r[2] = 1;
out:
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}
