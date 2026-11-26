/* End-of-move steps with a twist, 11 - 21 instructions: the opponent released (+543 = 5) or the fighter
   turned round as it stands again, and a hold that drifts with the held direction (0x08944018).
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c);
void f_0894410C(Cpu *c); void f_08921224(Cpu *c); void f_088646CC(Cpu *c); void f_0898B0C8(Cpu *c);
void f_08931A94(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static int ended(gaddr o) { return S8(o + 75) < 0; }
static void release_opponent(gaddr o) { *B(*W(o + 92) + 543) = 5; }

/* (fighter): a hold. A frame flag ends it (step 3, +262 cleared, 0x0894410C). Otherwise the opponent's
   +592 hand-off is taken (+270 = 1), and the fighter drifts by +96: unlocked (+373 clear) with the held
   direction (+362 bit 1 forward, bit 0 back); locked, after the +832 wait, for +836 frames, away from the
   facing and against +828's sign. Ticked. */
SFA_FUNC(08944018, move_hold_drift) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) != 0) {
        *B(o + 7) = 3; *B(o + 262) = 0;
        call1(c, o, f_0894410C, 0x08944054u);
        frame_out(c, ra);
        return;
    }
    gaddr opp = *W(o + 92);
    if (S8(opp + 592) != 0) { *B(opp + 270) = 1; *B(opp + 592) = 0; }
    if (S8(o + 373) == 0) {
        uint32_t held = *H(o + 362), v = *W(o + 96);
        uint32_t d = (held & 2) ? v : (held & 1) ? 0u - v : 0;
        *W(o + 16) = *W(o + 16) + d;
    } else {
        uint32_t dir = *W(o + 828);
        if (dir != 0) {
            uint32_t wait = *W(o + 832);
            if (wait != 0) *W(o + 832) = wait - 1;
            else {
                uint32_t n = *W(o + 836);
                if (n != 0) {
                    *W(o + 836) = n - 1;
                    uint32_t v = *W(o + 96);
                    if ((int32_t)dir < 0) v = 0u - v;
                    if (*B(o + 11) != 0) v = 0u - v;
                    *W(o + 16) = *W(o + 16) + v;
                }
            }
        }
    }
    call1(c, o, f_08861848, 0x08944100u);
    frame_out(c, ra);
}

/* (fighter): frame flag 1 clears +262; then 0x08921224. */
SFA_FUNC(08921B88, move_flag1_then) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 1) { *B(o + 77) = 0; *B(o + 262) = 0; }
    call1(c, o, f_08921224, 0x08921BB0u);
    frame_out(c, ra);
}

/* (fighter): standing again, releasing the opponent if the animation has ended. */
SFA_FUNC(0895EFFC, move_round_release) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) { release_opponent(o); call1(c, o, f_0886469C, 0x0895F01Cu); }
    else call1(c, o, f_0886469C, 0x0895F030u);
    frame_out(c, ra);
}

/* (fighter): ended: turned round and standing; else ticked. */
SFA_FUNC(0892E678, move_end_turn_round) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) { *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1); call1(c, o, f_0886469C, 0x0892E69Cu); }
    else call1(c, o, f_08861848, 0x0892E6B0u);
    frame_out(c, ra);
}

/* (fighter): ended: the opponent released and standing; else ticked. */
static void end_release(Cpu *c, uint32_t b_end, uint32_t b_tick) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) { release_opponent(o); call1(c, o, f_0886469C, b_end); }
    else call1(c, o, f_08861848, b_tick);
    frame_out(c, ra);
}
SFA_FUNC(08931840, move_end_release) { end_release(c, 0x08931864u, 0x08931878u); }
SFA_FUNC(089916F0, move_end_release_b) { end_release(c, 0x08991714u, 0x08991728u); }

/* (fighter): not ended: ticked; else 0x0886A530 and standing. */
SFA_FUNC(0890C634, move_tick_or_a530) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!ended(o)) call1(c, o, f_08861848, 0x0890C654u);
    else { call1(c, o, f_0886A530, 0x0890C664u); call1(c, o, f_0886469C, 0x0890C66Cu); }
    leave(c, sv);
}

/* (fighter): unlocked in state 10, move 0x30 started fresh (0x088646CC); else standing. */
SFA_FUNC(08864650, move_round_or_30) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t cond = (uint32_t)S8(o + 373) | (uint32_t)(S8(o + 338) != 0xA);
    frame_in(c, &ra);
    if (cond != 0) call1(c, o, f_0886469C, 0x0886467Cu);
    else { ARG(0) = o; ARG(1) = 0x30; c->r[31] = 0x08864690u; f_088646CC(c); }
    frame_out(c, ra);
}

/* (fighter): ended: +626 cleared and standing; else 0x0886DB2C and ticked. */
SFA_FUNC(08936384, move_end_626) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { *B(o + 626) = 0; call1(c, o, f_0886469C, 0x089363A8u); }
    else { call1(c, o, f_0886DB2C, 0x089363B8u); call1(c, o, f_08861848, 0x089363C0u); }
    leave(c, sv);
}

/* (fighter): 0x0898B0C8; ended: standing; else ticked. */
SFA_FUNC(0898B6E8, move_helper_end_round) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0898B0C8, 0x0898B6FCu);
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0898B714u);
    else call1(c, o, f_08861848, 0x0898B724u);
    leave(c, sv);
}

/* (fighter): 0x0886DB2C; not ended: ticked; else 0x0886A530 and standing. */
static void db2c_then(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0886DB2C, b1);
    if (!ended(o)) call1(c, o, f_08861848, b2);
    else { call1(c, o, f_0886A530, b3); call1(c, o, f_0886469C, b4); }
    leave(c, sv);
}
SFA_FUNC(088FDDB0, move_db2c_then_end) { db2c_then(c, 0x088FDDC4u, 0x088FDDD8u, 0x088FDDE8u, 0x088FDDF0u); }
SFA_FUNC(0899BC3C, move_db2c_then_end_b) { db2c_then(c, 0x0899BC50u, 0x0899BC64u, 0x0899BC74u, 0x0899BC7Cu); }

/* (fighter): ended: standing; else +543 = 5, 0x0886DB2C and ticked. */
SFA_FUNC(0892B118, move_end_or_543) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, 0x0892B138u);
    else { *B(o + 543) = 5; call1(c, o, f_0886DB2C, 0x0892B150u); call1(c, o, f_08861848, 0x0892B158u); }
    leave(c, sv);
}

/* (fighter): ticked; once ended the opponent released, turned round and standing. */
SFA_FUNC(08931F4C, move_tick_release_turn) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931F60u);
    if (ended(o)) {
        release_opponent(o);
        *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
        call1(c, o, f_0886469C, 0x08931F8Cu);
    }
    leave(c, sv);
}

/* (fighter): ended: the opponent released and standing; else 0x0886DB2C and ticked. */
static void end_release_db2c(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { release_opponent(o); call1(c, o, f_0886469C, b1); }
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
SFA_FUNC(08930FE4, move_end_release_db2c) { end_release_db2c(c, 0x08931010u, 0x08931020u, 0x08931028u); }
SFA_FUNC(0895D324, move_end_release_db2c_b) { end_release_db2c(c, 0x0895D350u, 0x0895D360u, 0x0895D368u); }

/* (fighter): ticked; ended: the opponent released and standing; else 0x08931A94. */
SFA_FUNC(08931A40, move_tick_release_or_1a94) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08931A54u);
    if (ended(o)) { release_opponent(o); call1(c, o, f_0886469C, 0x08931A74u); }
    else call1(c, o, f_08931A94, 0x08931A84u);
    leave(c, sv);
}

/* (fighter): ended: standing; else a negative frame flag clears +262, 0x0886DB2C and ticked. */
SFA_FUNC(0895DA58, move_end_or_flag_262) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, 0x0895DA78u);
    else {
        if (S8(o + 77) < 0) *B(o + 262) = 0;
        call1(c, o, f_0886DB2C, 0x0895DA94u);
        call1(c, o, f_08861848, 0x0895DA9Cu);
    }
    leave(c, sv);
}
