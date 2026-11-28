/* Small ones whose callees are all done: the link play's wait for every peer's answer and two of its
   wrappers, and moves' starts and steps that pass on to one of two others. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LINK      0x09523740u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_088D36E0(Cpu *c); void f_088D36EC(Cpu *c); void f_088DE4BC(Cpu *c); void f_088DE924(Cpu *c);
void f_088DFC34(Cpu *c); void f_088E114C(Cpu *c); void f_088E0C6C(Cpu *c); void f_088E2694(Cpu *c);
void f_0886BF84(Cpu *c); void f_088F78F0(Cpu *c); void f_08861674(Cpu *c); void f_0890EE10(Cpu *c);
void f_08910BE0(Cpu *c); void f_08861848(Cpu *c); void f_089110B0(Cpu *c); void f_08910D94(Cpu *c);
void f_0886C380(Cpu *c); void f_0891523C(Cpu *c); void f_089151D0(Cpu *c); void f_0899EDE0(Cpu *c);
void f_0899EE54(Cpu *c); void f_089A3678(Cpu *c); void f_089A36EC(Cpu *c); void f_08981640(Cpu *c);
void f_0898DAAC(Cpu *c); void f_0886163C(Cpu *c); void f_0899302C(Cpu *c); void f_08999338(Cpu *c);
void f_088D3514(Cpu *c); void f_088616EC(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
/* The same keeping s0 and s1, ra at +24. */
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (x): 0x088D36E0, then 0x088D36EC. */
SFA_FUNC(088D36F8, both_d36e0_d36ec) {
    gaddr x = ARG(0);
    uint32_t sv[2];
    enter(c, x, sv);
    call0(c, f_088D36E0, 0x088D370Cu);
    call1(c, x, f_088D36EC, 0x088D3714u);
    leave(c, sv);
}

/* The message send by kind (0x088DE4BC) behind a 0x30 frame of its own. */
SFA_FUNC(088DE69C, link_send_kind_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = ra;
    call0(c, f_088DE4BC, 0x088DE6ACu);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (a, b): the link's thread entry: marked as running (LINK + 188), 0x088DE924, then the thread's
   body with the two arguments. */
SFA_FUNC(088DEAC4, link_up_entry) {
    uint32_t a = ARG(0), b = ARG(1);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = b; c->r[17] = a;
    *B(LINK + 188) = 1;
    call2(c, 1, LINK, f_088DE924, 0x088DEAF0u);
    call2(c, a, b, f_088DFC34, 0x088DEAFCu);
    leave2(c, sv);
}

/* One more wait: 0x088E0C6C (0) and the count at x + 28. */
static void wait_more(Cpu *c, gaddr x, uint32_t back) {
    call1(c, 0, f_088E0C6C, back);
    *H(x + 28) = (uint16_t)(*H(x + 28) + 1);
    c->r[2] = 0;
}

/* (x, y): the wait for the peers' answers, on how many 0x088E114C says have come. Not started (+32)
   and fewer than the byte at 0x08A3EB96: reported, +46 cleared. While the delay +33 runs: with
   fewer than that less one, y's +8 counts and it waits; else the delay comes down, and until it is
   out the same. Then (y's +8 cleared when there was no delay): with y's +6 set, fewer than the byte
   at 0x08A3EB97 waits, else +6 is cleared and the check 0x088E2694 runs; with it clear, any at all
   runs the check, and none sets it and waits. */
SFA_FUNC(088E27DC, link_wait_answers) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = y; c->r[17] = x;
    call0(c, f_088E114C, 0x088E27F8u);
    int32_t n = (int32_t)c->r[2];
    if (S8(x + 32) == 0 && n < S8(0x08A3EB96u)) {
        call1(c, 0, f_088E0C6C, 0x088E2834u);
        *B(x + 46) = 0;
        c->r[2] = 0;
        goto out;
    }
    {
        int32_t delay = S8(x + 33);
        if (delay == 0) *H(y + 8) = 0;
        else {
            delay -= 1;
            if (n < S8(0x08A3EB96u) - 1) {
                *H(y + 8) = (uint16_t)(S16(y + 8) + 1);
                *B(x + 46) = 0;
                wait_more(c, x, 0x088E28E0u);
                goto out;
            }
            *B(x + 33) = (uint8_t)delay;
            if ((int8_t)delay != 0) {
                *H(y + 8) = (uint16_t)(S16(y + 8) + 1);
                *B(x + 46) = 0;
                wait_more(c, x, 0x088E28A4u);
                goto out;
            }
        }
    }
    if (*H(y + 6) != 0) {
        if (n < S8(0x08A3EB97u)) { wait_more(c, x, 0x088E2940u); goto out; }
        *H(y + 6) = 0;
    } else if (!(n > 0)) {
        *H(y + 6) = 1;
        wait_more(c, x, 0x088E297Cu);
        goto out;
    }
    call2(c, x, y, f_088E2694, 0x088E2924u);
out:
    leave2(c, sv);
}

/* (fighter): when 0x0886BF84 allows, the move 8 from state 1 / 0 / 7 / 0, facing as +697 says,
   with +78 the other way (or the same way), and 0x088F78F0. Returns whether. */
static void move8(Cpu *c, int same, uint32_t back0, uint32_t back1) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = o; c->r[17] = 0;
    call0(c, f_0886BF84, back0);
    if (c->r[2] != 0) {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 4) = 1; *B(o + 5) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 8;
        *B(o + 78) = same ? (uint8_t)facing : (uint8_t)(*B(o + 11) ^ 1);
        c->r[17] = 1;
        call1(c, o, f_088F78F0, back1);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}
SFA_FUNC(088F7878, move8_away) { move8(c, 0, 0x088F7894u, 0x088F78D8u); }
SFA_FUNC(088F7950, move8_toward) { move8(c, 1, 0x088F796Cu, 0x088F79A8u); }

/* (fighter): strength 1 takes the animation 3 (0x08861674), the others 0x0890EE10. */
SFA_FUNC(0890EDD0, by_strength_edd0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t strength = S8(o + 194);
    c->r[5] = (uint32_t)strength; c->r[6] = 1;
    *W(c->r[29] + 16) = ra;
    if (strength == 1) call2(c, o, 3, f_08861674, 0x0890EDF0u);
    else call0(c, f_0890EE10, 0x0890EE04u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the time +82 counts down: ticked until it is out, then 0x08910BE0. */
SFA_FUNC(089107E8, timed_then_10be0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t = (uint32_t)(S16(o + 82) - 1);
    c->r[6] = t; c->r[7] = t << 16; c->r[5] = (uint32_t)(int32_t)(int16_t)t;
    *W(c->r[29] + 16) = ra;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t != 0) call0(c, f_08861848, 0x08910824u);
    else call0(c, f_08910BE0, 0x08910810u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): further away (+508) than 0x30, or 0x60 for strengths past the first, 0x08910D94; else
   0x089110B0. */
SFA_FUNC(08910D44, by_distance_10d44) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t reach = S8(o + 194) != 0 ? 0x60 : 0x30;
    *W(c->r[29] + 16) = ra;
    int32_t dist = S16(o + 508);
    c->r[6] = (uint32_t)dist; c->r[5] = (uint32_t)(reach < dist);
    if (reach < dist) call0(c, f_08910D94, 0x08910D88u);
    else call0(c, f_089110B0, 0x08910D74u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): when 0x0886C380 allows, one of two starts: `plain` for the choice (+386) 0, else
   `other`. Returns 0 when not allowed; else 1, or for the two later ones what the start returned. */
static void start_by_choice(Cpu *c, void (*plain)(Cpu *), void (*other)(Cpu *), uint32_t back0,
                            uint32_t back_plain, uint32_t back_other, int one) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_0886C380, back0);
    if (c->r[2] == 0) c->r[2] = 0;
    else {
        int32_t choice = S8(o + 386);
        c->r[4] = (uint32_t)choice;
        if (choice != 0) call1(c, o, other, back_other);
        else call1(c, o, plain, back_plain);
        if (one) c->r[2] = 1;
    }
    leave(c, sv);
}
SFA_FUNC(08915168, start_by_choice_15168) {
    start_by_choice(c, f_0891523C, f_089151D0, 0x0891517Cu, 0x08915198u, 0x089151BCu, 1);
}
SFA_FUNC(0899ED7C, start_by_choice_9ed7c) {
    start_by_choice(c, f_0899EDE0, f_0899EE54, 0x0899ED90u, 0x0899EDACu, 0x0899EDD0u, 0);
}
SFA_FUNC(089A3614, start_by_choice_a3614) {
    start_by_choice(c, f_089A3678, f_089A36EC, 0x089A3628u, 0x089A3644u, 0x089A3668u, 0);
}

/* (x): 0x08981640 with x and each of the four fighters. A 0x30 frame keeping s0..s3. */
SFA_FUNC(089815C0, each_fighter_81640) {
    gaddr x = ARG(0);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    c->r[6] = 0x092E0000u;
    for (uint32_t i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 32) = sv[4];
    c->r[16] = x; c->r[17] = FIGHTER_2; c->r[18] = FIGHTER_3; c->r[19] = FIGHTER_4;
    call2(c, x, FIGHTER_1, f_08981640, 0x08981600u);
    call2(c, x, FIGHTER_2, f_08981640, 0x0898160Cu);
    call2(c, x, FIGHTER_3, f_08981640, 0x08981618u);
    call2(c, x, FIGHTER_4, f_08981640, 0x08981624u);
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (object): once 0x0898DAAC says so: step 2, hidden, +10 cleared, the cue 0x40. */
SFA_FUNC(0898958C, wait_then_cue_40) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_0898DAAC, 0x089895A0u);
    if (c->r[2] != 0) {
        *B(o + 7) = 2; *B(o + 1) = 0; *B(o + 10) = 0;
        call2(c, o, 0x40, f_0886163C, 0x089895C0u);
    }
    leave(c, sv);
}

/* (object): its fighter (+600) is thrown into state 1 / 0 / 8 / 7 with the move 0x11 / 1, facing as
   +697 says, +196..+199 = 0, -1, 0, 0; then `fn` with 0x500. */
static void throw_owner(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 600);
    *B(p + 4) = 1; *B(p + 5) = 0; *B(p + 6) = 8; *B(p + 7) = 7;
    *B(p + 236) = 0x11;
    int8_t facing = (int8_t)S8(p + 697);
    *B(p + 235) = 1;
    *B(p + 11) = (uint8_t)facing;
    *B(p + 196) = 0; *B(p + 197) = 0xFF; *B(p + 198) = 0; *B(p + 199) = 0;
    *W(c->r[29] + 16) = ra;
    c->r[6] = 0xFFFFFFFFu; c->r[7] = 1;
    call2(c, o, 0x500, fn, back);
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(0898EFC4, throw_owner_8efc4) { throw_owner(c, f_0899302C, 0x0898F020u); }
SFA_FUNC(08994834, throw_owner_94834) { throw_owner(c, f_08999338, 0x08994890u); }

/* (fighter): a move's start: the move and 1 at +236 / +235, facing as +697 says, the strength,
   +777 set, 0x088D3514 and the animation. */
static void move_start(Cpu *c, uint8_t move, uint8_t strength, uint32_t anim, uint32_t back0, uint32_t back1) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int8_t facing = (int8_t)S8(o + 697);
    *B(o + 236) = move;
    *B(o + 11) = (uint8_t)facing;
    *B(o + 235) = 1;
    *B(o + 194) = strength;
    *B(o + 777) = 1;
    c->r[5] = strength ? strength : (uint32_t)(int32_t)facing;
    call1(c, o, f_088D3514, back0);
    call2(c, o, anim, f_088616EC, back1);
    leave(c, sv);
}
SFA_FUNC(089B760C, move_start_b760c) { move_start(c, 0xA, 0, 0x2C, 0x089B7644u, 0x089B7650u); }
SFA_FUNC(089BDE08, move_start_bde08) { move_start(c, 0xD, 2, 0x44, 0x089BDE44u, 0x089BDE50u); }
SFA_FUNC(089C338C, move_start_c338c) { move_start(c, 0xC, 2, 0x28, 0x089C33C8u, 0x089C33D4u); }
SFA_FUNC(089CA13C, move_start_ca13c) { move_start(c, 0xD, 2, 0x3F, 0x089CA178u, 0x089CA184u); }
