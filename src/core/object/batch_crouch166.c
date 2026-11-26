/* The end-of-move steps that go back to crouching (0x08864740) instead of standing, in five repeating
   shapes and six single ones. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_08864740(Cpu *c); void f_0886DB2C(Cpu *c);
void f_0890AA58(Cpu *c);

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

/* (fighter): ended: crouching; else 0x0886DB2C and ticked. */
static void crouch_or_db2c(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_08864740, b1);
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
/* (fighter): ended: crouching; else ticked. */
static void crouch_or_tick(Cpu *c, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) call1(c, o, f_08864740, b1);
    else call1(c, o, f_08861848, b2);
    frame_out(c, ra);
}
/* (fighter): ended: the opponent released (+543 = 5) and crouching; else 0x0886DB2C and ticked. */
static void crouch_release(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { *B(*W(o + 92) + 543) = 5; call1(c, o, f_08864740, b1); }
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
/* (fighter): ended: standing. Else the +83 wait (when set) runs down, clearing +262 as it runs out:
   while it lasts only ticked; out (or not set), 0x0886DB2C and ticked. */
static void wait83_recover(Cpu *c, uint32_t b_db2c, uint32_t b_tick, uint32_t b_tick_wait, uint32_t b_end) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, b_end);
    else {
        gaddr w = o + 0x53;
        int wait = 0;
        if (S8(w) != 0) {
            int8_t t = (int8_t)(S8(w) - 1);
            *B(w) = (uint8_t)t;
            if (t != 0) wait = 1; else *B(o + 262) = 0;
        }
        if (wait) call1(c, o, f_08861848, b_tick_wait);
        else { call1(c, o, f_0886DB2C, b_db2c); call1(c, o, f_08861848, b_tick); }
    }
    leave(c, sv);
}
SFA_FUNC(088FACA4, move_crouch_or_db2c_aca4) { crouch_or_db2c(c, 0x088FACC4u, 0x088FACD4u, 0x088FACDCu); }
SFA_FUNC(0892B66C, move_crouch_or_db2c_b66c) { crouch_or_db2c(c, 0x0892B68Cu, 0x0892B69Cu, 0x0892B6A4u); }
SFA_FUNC(08968C80, move_crouch_or_db2c_8c80) { crouch_or_db2c(c, 0x08968CA0u, 0x08968CB0u, 0x08968CB8u); }
SFA_FUNC(089A69E0, move_crouch_or_db2c_69e0) { crouch_or_db2c(c, 0x089A6A00u, 0x089A6A10u, 0x089A6A18u); }
SFA_FUNC(089A7278, move_crouch_or_db2c_7278) { crouch_or_db2c(c, 0x089A7298u, 0x089A72A8u, 0x089A72B0u); }
SFA_FUNC(0896905C, move_crouch_or_tick_905c) { crouch_or_tick(c, 0x08969078u, 0x0896908Cu); }
SFA_FUNC(089A2E6C, move_crouch_or_tick_2e6c) { crouch_or_tick(c, 0x089A2E88u, 0x089A2E9Cu); }
SFA_FUNC(089A7EC8, move_crouch_or_tick_7ec8) { crouch_or_tick(c, 0x089A7EE4u, 0x089A7EF8u); }
SFA_FUNC(08910B90, move_crouch80_or_db2c_0b90) { crouch_or_db2c(c, 0x08910BB8u, 0x08910BC8u, 0x08910BD0u); }
SFA_FUNC(08973FDC, move_crouch80_or_db2c_3fdc) { crouch_or_db2c(c, 0x08974004u, 0x08974014u, 0x0897401Cu); }
SFA_FUNC(089767B8, move_crouch80_or_db2c_67b8) { crouch_or_db2c(c, 0x089767E0u, 0x089767F0u, 0x089767F8u); }
SFA_FUNC(089268C0, move_crouch_release_68c0) { crouch_release(c, 0x089268F0u, 0x08926900u, 0x08926908u); }
SFA_FUNC(08927078, move_crouch_release_7078) { crouch_release(c, 0x089270A8u, 0x089270B8u, 0x089270C0u); }
SFA_FUNC(0893E2EC, move_wait83_recover_e2ec) { wait83_recover(c, 0x0893E344u, 0x0893E34Cu, 0x0893E35Cu, 0x0893E374u); }
SFA_FUNC(0893E6E4, move_wait83_recover_e6e4) { wait83_recover(c, 0x0893E73Cu, 0x0893E744u, 0x0893E754u, 0x0893E76Cu); }
/* (fighter): until ended 0x0890AA58; then crouching. */
SFA_FUNC(0890AAD8, move_bounce_then_crouch) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (!ended(o)) call1(c, o, f_0890AA58, 0x0890AAF4u);
    else call1(c, o, f_08864740, 0x0890AB08u);
    frame_out(c, ra);
}
/* (fighter): crouching back with +696, else standing. */
SFA_FUNC(0894CBCC, move_crouch_or_stand) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 696) != 0) call1(c, o, f_08864740, 0x0894CBE8u);
    else call1(c, o, f_0886469C, 0x0894CBFCu);
    frame_out(c, ra);
}
/* (fighter): ticked until ended, then crouching. */
SFA_FUNC(0896D860, move_tick_then_crouch) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (!ended(o)) call1(c, o, f_08861848, 0x0896D87Cu);
    else call1(c, o, f_08864740, 0x0896D890u);
    frame_out(c, ra);
}
/* (fighter): ended: crouching; else standing. */
SFA_FUNC(08996C08, move_crouch_else_stand) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (ended(o)) call1(c, o, f_08864740, 0x08996C24u);
    else call1(c, o, f_0886469C, 0x08996C38u);
    frame_out(c, ra);
}
/* (fighter): ended (bit 7): crouching; else ticked. */
SFA_FUNC(08998510, move_crouch80_or_tick) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) call1(c, o, f_08864740, 0x08998530u);
    else call1(c, o, f_08861848, 0x08998544u);
    frame_out(c, ra);
}
/* (fighter): ended (bit 7): the opponent released and crouching; else ticked. */
SFA_FUNC(089278AC, move_crouch_release_tick) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) { *B(*W(o + 92) + 543) = 5; call1(c, o, f_08864740, 0x089278D0u); }
    else call1(c, o, f_08861848, 0x089278E4u);
    frame_out(c, ra);
}