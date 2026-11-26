/* Throw steps (12): the throw's step table by +7 followed by the held opponent placed and posed
   (0x0886DDCC), with a second table for a throw from the air (+192 = 2), picks between placing the
   opponent and placing the fighter, and a menu item's step table with its countdown voice
   (0x08854E00, 9,240 calls). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886DDCC(Cpu *c); void f_0886DEC8(Cpu *c); void f_088219B0(Cpu *c);
void f_08855DBC(Cpu *c); void f_088215C4(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
/* The step `tab` [index] with ARG0 = o (ARG1 holds the step's address, as the original leaves it). */
static void table_step(Cpu *c, gaddr o, gaddr tab, uint32_t index, uint32_t back) {
    uint32_t fn = *W(tab + (index << 2));
    ARG(0) = o; ARG(1) = fn;
    c->r[31] = back; rt_call_indirect(c, fn);
}

/* (fighter): the throw's step from `tab` by +7, then the opponent placed and posed (0x0886DDCC). */
static void throw_step(Cpu *c, gaddr tab, uint32_t b0, uint32_t b1) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    table_step(c, o, tab, *B(o + 7), b0);
    call1(c, o, f_0886DDCC, b1);
    leave(c, sv);
}
SFA_FUNC(088F9E7C, throw_step_a) { throw_step(c, 0x08A497F8u, 0x088F9EACu, 0x088F9EB4u); }
SFA_FUNC(0890AC58, throw_step_b) { throw_step(c, 0x08A583A0u, 0x0890AC88u, 0x0890AC90u); }
SFA_FUNC(08915AB0, throw_step_c) { throw_step(c, 0x08A5FF74u, 0x08915AE0u, 0x08915AE8u); }
SFA_FUNC(08916EF4, throw_step_d) { throw_step(c, 0x08A5FFFCu, 0x08916F24u, 0x08916F2Cu); }
SFA_FUNC(089198E0, throw_step_e) { throw_step(c, 0x08A64384u, 0x08919910u, 0x08919918u); }
SFA_FUNC(08945C44, throw_step_f) { throw_step(c, 0x08A765E4u, 0x08945C74u, 0x08945C7Cu); }
SFA_FUNC(08964F30, throw_step_g) { throw_step(c, 0x08A8A9ACu, 0x08964F60u, 0x08964F68u); }

/* (fighter): as throw_step, the air throw (+192 = 2) using its own table. */
static void throw_step_air(Cpu *c, gaddr tab, gaddr air, const uint32_t b[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    uint32_t i = *B(o + 7);
    if (S8(o + 192) == 2) table_step(c, o, air, i, b[1]);
    else table_step(c, o, tab, i, b[0]);
    call1(c, o, f_0886DDCC, b[2]);
    leave(c, sv);
}
SFA_FUNC(088F0F64, throw_step_air_a) {
    static const uint32_t b[3] = { 0x088F0FA0u, 0x088F0FC0u, 0x088F0FC8u };
    throw_step_air(c, 0x08A43BA8u, 0x08A43BBCu, b);
}
SFA_FUNC(0890C67C, throw_step_air_b) {
    static const uint32_t b[3] = { 0x0890C6B8u, 0x0890C6D8u, 0x0890C6E0u };
    throw_step_air(c, 0x08A58450u, 0x08A58460u, b);
}

/* (fighter): ticked; frame flag +76 places the fighter from the opponent (0x0886DEC8), else the
   opponent from the fighter (0x0886DDCC). */
SFA_FUNC(08978890, throw_place_by_flag) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089788A4u);
    if (S8(o + 76) != 0) call1(c, o, f_0886DEC8, 0x089788B8u);
    else call1(c, o, f_0886DDCC, 0x089788C8u);
    leave(c, sv);
}

/* (fighter): ticked; a negative frame flag places it from the opponent (0x0886DEC8); a positive one is
   cleared and +7 steps on. */
SFA_FUNC(089983E8, throw_flag_place_or_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x089983FCu);
    int32_t k = S8(o + 77);
    if (k != 0) {
        if (!(k & 0x80)) { uint32_t st = *B(o + 7); *B(o + 77) = 0; *B(o + 7) = (uint8_t)(st + 1); }
        else call1(c, o, f_0886DEC8, 0x08998418u);
    }
    leave(c, sv);
}

/* (menu item): its step from 0x08A16D50 by +5, then 0x088219B0, the countdown voice (0x08855DBC) and
   0x088215C4. */
SFA_FUNC(08854E00, menu_item_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    table_step(c, o, 0x08A16D50u, *B(o + 5), 0x08854E30u);
    call1(c, o, f_088219B0, 0x08854E38u);
    call1(c, o, f_08855DBC, 0x08854E40u);
    call1(c, o, f_088215C4, 0x08854E48u);
    leave(c, sv);
}
