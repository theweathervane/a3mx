/* Small functions the scenarios reach, 11 - 26 instructions: a select page's textures freed around
   0x088D7EA0, sparks at a place, a fighter's input step, effects set up from their owners before
   their level step, a gauge flash. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D7EA0(Cpu *c); void f_08815374(Cpu *c); void f_088D3724(Cpu *c); void f_088678B0(Cpu *c);
void f_08867A60(Cpu *c); void f_089B3AF8(Cpu *c); void f_08905070(Cpu *c); void f_0887CB3C(Cpu *c);
void f_0887D348(Cpu *c); void f_089A20D8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* The select screen's textures freed (0x08815374) between 0x088D7EA0 (1) and (0). */
SFA_FUNC(08815348, select_textures_free_wrapped) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    call_on(c, 1, 0x08815358u, f_088D7EA0);
    c->r[31] = 0x08815360u; f_08815374(c);
    call_on(c, 0, 0x08815368u, f_088D7EA0);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object, x, y): a kind-0xB spark at that offset from it (0x088D3724, b = 0xFF). */
SFA_FUNC(088D3968, spark_b_at) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    ARG(1) = (uint32_t)(int32_t)(int16_t)ARG(1);
    ARG(2) = (uint32_t)(int32_t)(int16_t)ARG(2);
    ARG(3) = 0xB; c->r[8] = 0xFF;
    *W(c->r[29] + 32) = ra;
    c->r[9] = 0;
    c->r[31] = 0x088D3990u; f_088D3724(c);
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter, step): its buttons as a byte (0x088678B0) handed to 0x08867A60 (fighter, step, buttons). */
SFA_FUNC(08867BF0, fighter_buttons_step) {
    gaddr f = ARG(0), t = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = t;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = f;
    c->r[31] = 0x08867C0Cu; f_088678B0(c);
    ARG(0) = f; ARG(1) = t; ARG(2) = c->r[2];
    c->r[31] = 0x08867C1Cu; f_08867A60(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): with its owner's (+88) +76 set, on (+5) for 0x78 frames (+82) via 0x089B3AF8. */
SFA_FUNC(089B3AB8, effect_on_owner_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(*W(o + 88) + 76) != 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *B(o + 0x52) = 0x78;
        c->r[31] = 0x089B3AECu; f_089B3AF8(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): +4 = 1, the owner's +12 - +14 / +24 / character (as +7), +78 cleared, then its level
   step (0x08905070). */
SFA_FUNC(08905024, effect_level_start_6) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = 1;
    *H(o + 24) = *H(p + 24);
    *B(o + 7) = (uint8_t)S8(p + 338);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 78) = 0;
    *W(c->r[29] + 16) = ra;
    *B(o + 14) = *B(p + 14);
    c->r[31] = 0x08905064u; f_08905070(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): its fighter (+92) gets +572 = 1 and, with 0x0887CB3C, +656 = 4 and its gauge pieces set
   (0x0887D348 with 0x01030000). */
SFA_FUNC(0887CD7C, gauge_flash) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr f = *W(o + 92);
    c->r[17] = f;
    *W(c->r[29] + 24) = ra;
    *B(f + 572) = 1;
    ARG(0) = 1; c->r[31] = 0x0887CDA0u; f_0887CB3C(c);
    if (c->r[2] != 0) {
        *B(f + 656) = 4;
        ARG(0) = o; ARG(1) = f; ARG(2) = 0x01030000u;
        c->r[31] = 0x0887CDBCu; f_0887D348(c);
    } else ARG(0) = 4;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object, -, dy): a kind-6 spark (0x088D3724, b = 0xFF, absolute) at 0x092E88A0's x (+ 0x180 for its
   +11) and its own y + dy. */
SFA_FUNC(088D39D0, spark_6_at_edge) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    int32_t dy = (int16_t)ARG(2);
    *W(c->r[29] + 32) = ra;
    int32_t x = S16(0x092E88A0u + 18);
    if (*B(o + 11) != 0) x = (int16_t)(x + 0x180);
    ARG(1) = (uint32_t)x;
    ARG(2) = (uint32_t)(int32_t)(int16_t)(dy + S16(o + 22));
    ARG(3) = 6; c->r[8] = 0xFF; c->r[9] = 1;
    c->r[31] = 0x088D3A24u; f_088D3724(c);
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (effect, owner): on (+4), the owner's +12 - +14 / +24 / character (as +3) / side / +164, +78 cleared,
   then its level step (0x089A20D8). */
SFA_FUNC(089A2070, effect_level_start_4) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(p + 24);
    *B(o + 3) = (uint8_t)S8(p + 338);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *B(o + 13) = *B(p + 13);
    *B(o + 78) = 0;
    *B(o + 14) = *B(p + 14);
    *H(o + 86) = (uint16_t)(int32_t)S8(p + 337);
    *W(c->r[29] + 16) = ra;
    *W(o + 164) = *W(p + 164);
    c->r[31] = 0x089A20CCu; f_089A20D8(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
