/* Small functions the scenarios reach, 14 - 27 instructions: the main menu's frame, the game's reset to
   the menus, some characters' effect step dispatchers, two menu pieces. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0880B6CC(Cpu *c); void f_08822600(Cpu *c); void f_08837690(Cpu *c); void f_0883771C(Cpu *c);
void f_0889682C(Cpu *c); void f_088D5E44(Cpu *c); void f_088E2304(Cpu *c); void f_088E358C(Cpu *c);
void f_088E36B8(Cpu *c); void f_088E36EC(Cpu *c); void f_08821424(Cpu *c); void f_08804A74(Cpu *c);
void f_08813744(Cpu *c); void f_088AC618(Cpu *c); void f_088AC488(Cpu *c); void f_088BD8B0(Cpu *c);
void f_088BD918(Cpu *c); void f_088BDA8C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

static void step_from(Cpu *c, gaddr o, gaddr table, uint32_t by, uint32_t back) {
    gaddr h = *W(table + ((uint32_t)*B(o + by) << 2));
    ARG(0) = o; ARG(1) = h;
    c->r[31] = back;
    rt_call_indirect(c, h);
}

/* (menu): the main menu's frame: unless 0x088E358C says no, by +8: 0 waits (0x088E36B8, then +8 on)
   unless GAME +93; 1 until 0x088E36EC says so, then +8 / +6 cleared, +4 two on, with 0x088D5E44 the
   pads and counters reset (0x088E2304) and +157 = 1, +32 / +33 / +59 cleared, then 0x08822600 and
   0x0889682C; then 0x08837690 and 0x0883771C. Always 0x0880B6CC. */
SFA_FUNC(08893E40, main_menu_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    call0(c, 0x08893E54u, f_088E358C);
    if (c->r[2] != 0) {
        uint32_t st = *H(o + 8);
        if (st == 0) {
            if (S8(GAME + 93) == 0) {
                ARG(0) = 0;
                call0(c, 0x08893EA4u, f_088E36B8);
                *H(o + 8) = (uint16_t)(*H(o + 8) + 1);
            }
        } else if (st == 1) {
            ARG(0) = 1;
            call0(c, 0x08893EBCu, f_088E36EC);
            if (c->r[2] != 0) {
                *H(o + 8) = 0;
                uint32_t n4 = *H(o + 4) + 2u;
                *H(o + 4) = (uint16_t)n4;
                *H(o + 6) = 0;
                ARG(0) = n4;
                call0(c, 0x08893EDCu, f_088D5E44);
                if (c->r[2] != 0) {
                    call0(c, 0x08893EECu, f_088E2304);
                    *B(o + 157) = 1;
                    *B(o + 33) = 0; *B(o + 32) = 0; *B(o + 59) = 0;
                }
                call0(c, 0x08893F08u, f_08822600);
                call0(c, 0x08893F10u, f_0889682C);
            }
        }
        call0(c, 0x08893F18u, f_08837690);
        call0(c, 0x08893F20u, f_0883771C);
    }
    call0(c, 0x08893F28u, f_0880B6CC);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}

/* (effect): its step by +7, from 0x08AB14B4 for a fighter (+92) of character 0x21, else 0x08AB14A8. */
SFA_FUNC(089BEF64, effect_step_char21) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(*W(o + 92) + 338) == 0x21) step_from(c, o, 0x08AB14B4u, 7, 0x089BEF9Cu);
    else step_from(c, o, 0x08AB14A8u, 7, 0x089BEFC0u);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +7, from 0x08AB5CD4 for a fighter (+92) of character 0x19, else 0x08AB5CC4. */
SFA_FUNC(089C6084, effect_step_char19) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(*W(o + 92) + 338) == 0x19) step_from(c, o, 0x08AB5CD4u, 7, 0x089C60BCu);
    else step_from(c, o, 0x08AB5CC4u, 7, 0x089C60E0u);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +5 from `table` unless the fight is halted (BATTLE +37 / +78), then drawn. */
static void step_unless_halted(Cpu *c, gaddr table, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) step_from(c, o, table, 5, back1);
    call_on(c, o, back2, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089CBCD8, effect_step_6070) { step_unless_halted(c, 0x08AB6070u, 0x089CBD28u, 0x089CBD30u); }
SFA_FUNC(089CC254, effect_step_6078) { step_unless_halted(c, 0x08AB6078u, 0x089CC2A4u, 0x089CC2ACu); }

/* Back to the menus: frame pacing 0 (0x08804A74), GAME +2 - +22's counters cleared, GAME +0 on,
   0x088AC618 and 0x08813744. */
SFA_FUNC(088A9440, game_to_menus) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = 0;
    call_on(c, 0, 0x088A9458u, f_08804A74);
    *H(GAME + 22) = 0; *H(GAME + 20) = 0; *H(GAME + 16) = 0; *H(GAME + 14) = 0; *H(GAME + 10) = 0;
    uint32_t n = *H(GAME) + 1u;
    *H(GAME + 8) = 0; *H(GAME + 6) = 0; *H(GAME + 4) = 0; *H(GAME + 2) = 0;
    *H(GAME) = (uint16_t)n;
    ARG(0) = 0x08E70000u; ARG(1) = n;
    call0(c, 0x088A9494u, f_088AC618);
    call0(c, 0x088A949Cu, f_08813744);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): +1 cleared; while GAME +16's high byte is set 0x088AC488 (0, 4, 0, 1), drawn and
   0x092E22E0 = 1. */
SFA_FUNC(088B118C, menu_overlay) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 1;
    if ((((int32_t)S16(GAME + 16) & 0xFF00) >> 8) != 0) {
        ARG(0) = 0; ARG(1) = 4; ARG(2) = 0; ARG(3) = 1;
        call0(c, 0x088B11D4u, f_088AC488);
        call_on(c, o, 0x088B11DCu, f_08821424);
        *W(0x092E22E0u) = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): while its owner (+88) is live and points back at it (+104): 0x088BD8B0 on its first step
   (+4 = 0), else 0x088BD918; otherwise 0x088BDA8C. */
SFA_FUNC(088BD844, owned_object_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr p = *W(o + 88);
    if (*B(p) != 0 && *W(p + 104) == o) {
        if (*B(o + 4) == 0) call0(c, 0x088BD87Cu, f_088BD8B0);
        else call0(c, 0x088BD890u, f_088BD918);
    } else call0(c, 0x088BD8A4u, f_088BDA8C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
