/* The game's engine: what comes after a fight is over, by the kind of game. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define F3     0x092E47D0u
#define F4     0x092E4C68u
#define PAIR   0x095179E0u

void f_08821F58(Cpu *c); void f_08821F9C(Cpu *c); void f_08822AEC(Cpu *c); void f_0881CE4C(Cpu *c);
void f_088261A0(Cpu *c); void f_0887C6E4(Cpu *c); void f_08828890(Cpu *c); void f_0882781C(Cpu *c);
void f_088D5E44(Cpu *c); void f_0882FE94(Cpu *c); void f_08814384(Cpu *c); void f_08839130(Cpu *c);
void f_088279C0(Cpu *c); void f_0882FE50(Cpu *c); void f_0882FD70(Cpu *c); void f_0882744C(Cpu *c);
void f_08827A94(Cpu *c); void f_0882AE20(Cpu *c); void f_088E348C(Cpu *c); void f_088E3630(Cpu *c);
void f_0881F684(Cpu *c); void f_0881DF10(Cpu *c); void f_08822F8C(Cpu *c); void f_0888D5B0(Cpu *c);
void f_08819A20(Cpu *c); void f_0884AE40(Cpu *c); void f_0880B6CC(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call4(Cpu *c, uint32_t a, uint32_t b, uint32_t p, uint32_t q, void (*fn)(Cpu *), uint32_t back) {
    ARG(2) = p; ARG(3) = q; call2(c, a, b, fn, back);
}

/* (fighter): the fighter's record of the fight just over is kept for the results (unless the
   fighter is the machine's, +373, outside the battle's kind 5): its +352 to +452, the pair's
   three bytes from +12 (the third as five thirds of itself) to +469.., the battle's +70 to
   +472, and at +478 a count of at most 999: the pair's +10 in the battle's kind 7, else the
   fighter's own +426. */
SFA_FUNC(08827AD8, fight_record_keep) {
    gaddr f = ARG(0);
    c->r[6] = (uint32_t)S8(f + 373);
    ARG(1) = BATTLE;
    if (c->r[6] != 0) {
        c->r[6] = (uint32_t)S8(BATTLE + 79);
        c->r[7] = 5;
        if (c->r[6] != 5) return;
    }
    *W(f + 452) = *W(f + 352);
    *B(f + 469) = (uint8_t)S8(PAIR + 12);
    *B(f + 470) = (uint8_t)S8(PAIR + 13);
    c->r[6] = (uint32_t)S8(PAIR + 14);
    c->r[9] = c->r[6] << 2;
    c->r[6] += c->r[9];
    c->r[8] = 3;
    DIV(c, c->r[6], c->r[8]);
    c->r[6] = 7;
    c->r[8] = c->lo;
    *B(f + 471) = (uint8_t)c->r[8];
    c->r[8] = (uint32_t)S8(BATTLE + 70);
    *B(f + 472) = (uint8_t)c->r[8];
    c->r[7] = (uint32_t)S8(BATTLE + 79);
    uint32_t count = c->r[7] == 7 ? *H(PAIR + 10) : *H(f + 426);
    if ((int32_t)count >= 999) count = 999;
    ARG(1) = count;
    *H(f + 478) = (uint16_t)count;
    *B(f + 341) = 0;
    *B(f + 340) = 0;
}

/* (a, b): the game goes to its step 3 (a's +2) with its counters cleared; both first fighters
   become the machine's. A 0x20 frame keeping s0..s3. */
SFA_FUNC(08827910, game_to_step3) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = a;
    *W(sp + 8) = s2;
    c->r[18] = F1;
    *W(sp) = s0;
    c->r[16] = b;
    *W(sp + 12) = s3; *W(sp + 16) = ra;
    c->r[19] = F2;
    call1(c, 0x092E0000u, f_08821F58, 0x08827944u);
    *H(a + 2) = 3;
    *H(a + 22) = 0; *H(a + 20) = 0; *H(a + 18) = 0; *H(a + 16) = 0;
    *H(a + 10) = 0; *H(a + 8) = 0; *H(a + 6) = 0; *H(a + 4) = 0;
    *B(b + 15) = 1;
    *B(b + 55) = 0;
    *H(b + 56) = 0;
    *B(F1 + 373) = 1;
    *B(F2 + 373) = 1;
    *B(F4 + 1) = 0; *B(F3 + 1) = 0; *B(F2 + 1) = 0; *B(F1 + 1) = 0;
    ARG(0) = F3;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a): the game goes to its step 1 with its counters cleared. In kind 1 (a's +114) its +41
   and +53 become 3 and the two first fighters' +424 are cleared and their routine at
   0x08822AEC run; in kinds 0xD and 0x13 the same is done for all four with +41 taken from
   +53. A 0x20 frame keeping s0, s1. */
SFA_FUNC(08827720, game_to_step1) {
    gaddr a = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = a;
    call0(c, f_08821F9C, 0x08827738u);
    *H(a + 2) = 1;
    *H(a + 22) = 0; *H(a + 20) = 0; *H(a + 18) = 0; *H(a + 16) = 0; *H(a + 14) = 0;
    *H(a + 10) = 0; *H(a + 8) = 0; *H(a + 6) = 0; *H(a + 4) = 0;
    int32_t kind = S8(a + 114);
    *B(a + 57) = 0;
    if (kind == 1) {
        *B(a + 41) = 3;
        *B(a + 53) = 3;
        c->r[17] = F2;
        *H(F2 + 424) = 0;
        *H(F1 + 424) = 0;
        call1(c, F1, f_08822AEC, 0x0882779Cu);
        call1(c, F2, f_08822AEC, 0x088277A4u);
        kind = S8(a + 114);
    }
    if (kind == 0xD || kind == 0x13) {
        *B(a + 41) = (uint8_t)S8(a + 53);
        c->r[16] = F4;
        c->r[17] = F3;
        *H(F4 + 424) = 0; *H(F3 + 424) = 0; *H(F2 + 424) = 0; *H(F1 + 424) = 0;
        call1(c, F1, f_08822AEC, 0x088277F8u);
        call1(c, F3, f_08822AEC, 0x08827800u);
        call1(c, F4, f_08822AEC, 0x08827808u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): after a fight, where the game goes next. First the stage is loaded again and the
   music 0x132 may start (in kind 7 or with b's +105 set; the music unless the first side is
   the machine's, or, in the battle's kinds 4 and 5, when the routine at 0x088261A0 says 1);
   otherwise a's step (+4) just goes on. Then, in order: with a's +50 set, or the link saying
   so, the routine at 0x0882781C; kinds 1, 0xD and 0x13 go to step 1; kind 0x10 (or the link
   again) the routine at 0x0882781C with +69 set; kind 0x12 its own; the link's game step 3 in
   kind 6, else by whether the first side is the machine's. The rest go by b's +56 (3 the
   game over), a's +137 and +146, and kind 0x11; and at the end the second side becomes the
   machine's when the first is not, else step 3 or the routine at 0x0882781C. A 0x30 frame
   keeping s0..s5. */
SFA_FUNC(0882A874, after_fight_next) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t sv[6], ra = c->r[31];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[5];
    gaddr first = *W(b + 112);
    c->r[21] = first;
    *W(sp + 32) = sv[4];
    gaddr second = *W(b + 116);
    c->r[20] = second;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3];
    c->r[19] = 7;
    c->r[18] = 1;
    c->r[17] = a;
    *W(sp + 16) = sv[0]; *W(sp + 40) = ra;
    c->r[16] = b;
    if (S8(a + 114) == 7 || S8(b + 105) != 0) {
        *H(a + 4) = 0;
        call2(c, (uint32_t)S16(b + 102), 0, f_0881CE4C, 0x0882A8D0u);
        int32_t bk = S8(b + 79);
        if (bk == 4 || bk == 5) {
            call1(c, b, f_088261A0, 0x0882A90Cu);
            if (c->r[2] == 1) call1(c, 0x132, f_0887C6E4, 0x0882A91Cu);
        } else if (S8(first + 373) == 0) {
            call1(c, 0x132, f_0887C6E4, 0x0882A8FCu);
        }
        *B(b + 15) = 1;
    } else {
        *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
        *B(b + 15) = 1;
    }
    ARG(2) = first;
    call2(c, a, b, f_08828890, 0x0882A944u);
    int32_t kind;
    if (S8(a + 50) != 0) { call2(c, a, b, f_0882781C, 0x0882A958u); goto done; }
    call1(c, a, f_088D5E44, 0x0882A968u);
    if (c->r[2] != 0) {
        *B(a + 69) = 1;
        call2(c, a, b, f_0882781C, 0x0882A980u);
        goto done;
    }
    kind = S8(a + 114);
    if (kind == 1 || kind == 0xD || kind == 0x13) { call2(c, a, b, f_08827720, 0x0882A9B4u); goto done; }
    if (kind == 0x10) {
        *B(a + 69) = 1;
        call2(c, a, b, f_0882781C, 0x0882A9E4u);
        goto done;
    }
    ARG(1) = (uint32_t)kind;
    call1(c, 0x10, f_088D5E44, 0x0882A9CCu);
    if (c->r[2] != 0) {
        *B(a + 69) = 1;
        call2(c, a, b, f_0882781C, 0x0882A9E4u);
        goto done;
    }
    kind = S8(a + 114);
    if (kind == 0x12) {
        *B(a + 69) = 0;
        call4(c, a, b, first, second, f_0882FE94, 0x0882AA10u);
        call0(c, f_08814384, 0x0882AA18u);
        goto done;
    }
    if (S8(b + 155) != 0) {
        if (kind == 6) call2(c, a, b, f_08827910, 0x0882AA38u);
        else if (S8(first + 373) == 0) call4(c, a, b, first, second, f_08839130, 0x0882AA60u);
        else call2(c, a, b, f_0882781C, 0x0882AA70u);
        goto done;
    }
    if (S16(b + 56) == 3) {
        if (S8(a + 137) != 0) {
            if ((S8(first + 337) & 2) != 0) call2(c, a, b, f_0882781C, 0x0882AAA8u);
            else if ((S8(a + 53) & 1) != 0) call4(c, a, b, F1, second, f_08839130, 0x0882AAD8u);
            else call4(c, a, b, F2, second, f_08839130, 0x0882AAF4u);
        } else {
            call2(c, a, b, f_088279C0, 0x0882AB04u);
            if (S8(a + 114) != 7) call0(c, f_08814384, 0x0882AB18u);
        }
        goto done;
    }
    if (S8(a + 137) != 0) {
        call4(c, a, b, first, second, f_08839130, 0x0882AB38u);
    } else {
        if (S8(a + 146) != 0) {
            *H(a + 4) = 4;
            *H(a + 22) = 0; *H(a + 20) = 0; *H(a + 18) = 0;
            *H(a + 10) = 0; *H(a + 8) = 0; *H(a + 6) = 0;
            goto done;
        }
        if (kind == 0x11) { ARG(2) = first; call2(c, a, b, f_0882FE50, 0x0882AB68u); }
        else call4(c, a, b, first, second, f_0882FD70, 0x0882ABC0u);
        if (S8(a + 114) != 7) call0(c, f_08814384, 0x0882ABD4u);
    }
    if (S8(first + 373) == 0) *B(second + 373) = 1;
    else if (S8(b + 105) != 0 || S8(b + 79) != 0) call2(c, a, b, f_08827910, 0x0882AC14u);
    else call2(c, a, b, f_0882781C, 0x0882AC04u);
done:
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, b): a fight is over: a's counters are cleared, the four fighters' +1 cleared, and the
   time for the next fight set from the table at 0x08A0E098 (by a's +47). The fight is noted:
   by the link's own routine in a link game; in kind 7 the pair's count (+10) goes up (by one
   in the battle's kind 8, else by b's +108) and both fighters' records are kept; otherwise by
   the routine at 0x08827A94 for both. Then: with b's +70 set the routine at 0x0882AE20; with
   b's +5 set, or b's +63 set in the kinds that go on (a's +146, or kinds 1, 0xD, 0x10 and
   0x13), where the game goes next; with b's +79 or +63 set, step 3; else the next round is
   made: b's round (+2) goes up and the screen is set up again. A 0x30 frame keeping s0..s3. */
SFA_FUNC(088271DC, fight_over) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = a;
    *H(a + 22) = 0; *H(a + 20) = 0; *H(a + 18) = 0; *H(a + 10) = 0; *H(a + 8) = 0;
    *W(sp + 24) = s2;
    *H(a + 6) = 0;
    c->r[18] = 1;
    *B(b + 15) = 1;
    *B(a + 125) = 1;
    *B(a + 43) = 1;
    *B(b + 14) = 0;
    *B(b + 11) = 0;
    *W(sp + 28) = s3;
    *B(b + 78) = 0;
    *B(F1 + 1) = 0;
    c->r[19] = F2;
    *B(F2 + 1) = 0; *B(F3 + 1) = 0; *B(F4 + 1) = 0;
    *B(b + 9) = (uint8_t)S8(0x08A0E098u + (uint32_t)S8(a + 47));
    *B(b + 10) = 0x3B;
    *W(sp + 20) = s1; *W(sp + 32) = ra;
    c->r[17] = b;
    if (S8(b + 155) != 0) {
        call2(c, a, b, f_0882744C, 0x08827294u);
    } else if (S8(a + 114) == 7) {
        int32_t count = S16(PAIR + 10);
        if (S8(b + 79) == 8) count += 1;
        else count += S8(b + 108);
        *H(PAIR + 10) = (uint16_t)count;
        *B(b + 108) = 0;
        call2(c, F1, PAIR, f_08827AD8, 0x088272E4u);
        call1(c, F2, f_08827AD8, 0x088272ECu);
    } else {
        ARG(2) = F1;
        call2(c, a, b, f_08827A94, 0x08827300u);
        ARG(2) = F2;
        call2(c, a, b, f_08827A94, 0x08827310u);
    }
    if (S8(b + 70) != 0) {
        call2(c, a, b, f_0882AE20, 0x08827324u);
    } else {
        int go_on = S8(b + 5) != 0;
        int32_t more = 0;
        if (!go_on) {
            more = S8(b + 63);
            int32_t kind = S8(a + 114);
            int kinds = S8(a + 146) != 0 || kind == 1 || kind == 0x13 || kind == 0x10 || kind == 0xD;
            go_on = kinds && more != 0;
        }
        if (go_on) {
            call2(c, a, 1, f_088E348C, 0x0882737Cu);
            call2(c, a, b, f_0882A874, 0x08827388u);
        } else if ((S8(b + 79) | more) != 0) {
            call2(c, a, b, f_08827910, 0x088273A4u);
        } else {
            call1(c, a, f_088E3630, 0x088273B4u);
            call0(c, f_08821F9C, 0x088273BCu);
            *H(b + 2) = (uint16_t)(S16(b + 2) + 1);
            call0(c, f_0881F684, 0x088273CCu);
            *B(b + 15) = 0;
            call0(c, f_0881DF10, 0x088273D4u);
            *B(b + 15) = 1;
            if (S8(a + 114) != 2) {
                if (S8(b + 79) != 0) call2(c, 3, 0, f_08822F8C, 0x08827410u);
                else call2(c, 2, 0, f_08822F8C, 0x08827400u);
            }
            call0(c, f_0888D5B0, 0x08827418u);
            call0(c, f_08819A20, 0x08827420u);
            call0(c, f_0884AE40, 0x08827428u);
            call0(c, f_0880B6CC, 0x08827430u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}
