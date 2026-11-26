/* More end-of-move steps back to crouching (0x08864740), 18 - 23 instructions, after 0x0886A530, a
   release of the opponent, a turn or a short wait. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_08864740(Cpu *c); void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c);
void f_0894CBCC(Cpu *c); void f_08987118(Cpu *c); void f_08987404(Cpu *c); void f_08984DF4(Cpu *c);

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

/* (fighter): ticked until ended; then 0x0886A530 and crouching. */
SFA_FUNC(088FF590, move_tick_then_a530_crouch) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (!ended(o)) call1(c, o, f_08861848, 0x088FF5B0u);
    else { call1(c, o, f_0886A530, 0x088FF5C0u); call1(c, o, f_08864740, 0x088FF5C8u); }
    leave(c, sv);
}

/* (fighter): ended: crouching; else +262 cleared, 0x0886DB2C and ticked. */
SFA_FUNC(08919E0C, move_crouch_or_262) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_08864740, 0x08919E2Cu);
    else { *B(o + 262) = 0; call1(c, o, f_0886DB2C, 0x08919E40u); call1(c, o, f_08861848, 0x08919E48u); }
    leave(c, sv);
}

/* (fighter): with any animation end byte (+75) 0x0886A530 and crouching; else 0x0886DB2C and ticked. */
SFA_FUNC(0890711C, move_any75_crouch) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) != 0) { call1(c, o, f_0886A530, 0x0890713Cu); call1(c, o, f_08864740, 0x08907144u); }
    else { call1(c, o, f_0886DB2C, 0x08907154u); call1(c, o, f_08861848, 0x0890715Cu); }
    leave(c, sv);
}

/* (fighter): ended: the opponent released, turned round and crouching; else ticked. */
SFA_FUNC(08946314, move_release_turn_crouch) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) {
        release_opponent(o);
        *B(o + 11) = (uint8_t)(*B(o + 11) ^ 1);
        call1(c, o, f_08864740, 0x08946348u);
    } else call1(c, o, f_08861848, 0x0894635Cu);
    frame_out(c, ra);
}

/* (fighter): ended: the opponent released and crouching or standing (0x0894CBCC); else 0x0886DB2C and
   ticked. */
SFA_FUNC(0894A308, move_release_crouch_or_stand) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { release_opponent(o); call1(c, o, f_0894CBCC, 0x0894A334u); }
    else { call1(c, o, f_0886DB2C, 0x0894A344u); call1(c, o, f_08861848, 0x0894A34Cu); }
    leave(c, sv);
}

/* (fighter): 0x0886DB2C; ended: the opponent released and crouching; else ticked. */
static void db2c_then_release(Cpu *c, uint32_t b1, uint32_t b_crouch, uint32_t b_tick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr opp = *W(o + 92);
    c->r[17] = opp;
    call1(c, o, f_0886DB2C, b1);
    if (ended(o)) { *B(opp + 543) = 5; call1(c, o, f_08864740, b_crouch); }
    else call1(c, o, f_08861848, b_tick);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0895D6E8, move_db2c_release_crouch) { db2c_then_release(c, 0x0895D704u, 0x0895D71Cu, 0x0895D72Cu); }
SFA_FUNC(08995494, move_db2c_release_crouch_b) { db2c_then_release(c, 0x089954B0u, 0x089954D8u, 0x089954C4u); }

/* (fighter): ended (bit 7): crouching; else without a frame flag it moves on by +96; ticked. */
SFA_FUNC(08976C6C, move_crouch_or_walk) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) call1(c, o, f_08864740, 0x08976C8Cu);
    else {
        if (S8(o + 77) == 0) *W(o + 16) = *W(o + 16) + *W(o + 96);
        call1(c, o, f_08861848, 0x08976CB8u);
    }
    frame_out(c, ra);
}

/* (fighter): once ended the +83 wait runs down, then crouching; until then ticked. */
SFA_FUNC(0896DE40, move_end_wait_crouch) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    int crouch = 0;
    if (ended(o)) {
        int8_t t = (int8_t)(S8(o + 0x53) - 1);
        *B(o + 0x53) = (uint8_t)t;
        crouch = t < 0;
    }
    if (crouch) call1(c, o, f_08864740, 0x0896DE7Cu);
    else call1(c, o, f_08861848, 0x0896DE90u);
    frame_out(c, ra);
}

/* (fighter): 0x08987118, 0x08987404; more than 32 up ticked, else the jump back (0x08984DF4). */
SFA_FUNC(08984D98, move_air_or_jump_back) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08987118, 0x08984DACu);
    call1(c, o, f_08987404, 0x08984DB4u);
    if (S16(o + 132) - 0x20 < S16(o + 22)) call1(c, o, f_08861848, 0x08984DD4u);
    else call1(c, o, f_08984DF4, 0x08984DE4u);
    leave(c, sv);
}
