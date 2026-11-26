/* Small functions the scenarios reach, 7 - 69 instructions: wrappers and step pickers, the network
   match's start records, a menu screen with list-0x7D objects. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0887E884(Cpu *c); void f_08867BF0(Cpu *c); void f_0887EBA4(Cpu *c); void f_0887EF10(Cpu *c);
void f_089B3AB8(Cpu *c); void f_089B3AF8(Cpu *c); void f_089CD05C(Cpu *c); void f_089CD120(Cpu *c);
void f_088D6440(Cpu *c); void f_088D6564(Cpu *c); void f_089F3430(Cpu *c); void f_08822EAC(Cpu *c);
void f_088B3258(Cpu *c); void f_088B3C34(Cpu *c); void f_088B3D94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* The round-start object (0x0887E884) through a frame. */
SFA_FUNC(0887E84C, round_object_start_wrap) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x0887E85Cu; f_0887E884(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, step): 1 when 0x08867BF0 gives anything, else 0. */
SFA_FUNC(08867C84, fighter_buttons_step_any) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x08867C94u; f_08867BF0(c);
    c->r[2] = c->r[2] != 0;
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (stage object): 0x0887EBA4 then its palette cycle (0x0887EF10). */
SFA_FUNC(0887E9EC, stage_object_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x0887EA00u, f_0887EBA4);
    call_on(c, o, 0x0887EA08u, f_0887EF10);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its start (0x089B3AB8) while +5 is 0, then its frame (0x089B3AF8). */
SFA_FUNC(089B3A7C, effect_on_owner_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 5) == 0) { c->r[31] = 0x089B3A98u; f_089B3AB8(c); }
    else { c->r[31] = 0x089B3AACu; f_089B3AF8(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (animation): with the palette timer 0x09358BE0, its start (0x089CD05C) while +4 is 0, then its step
   (0x089CD120). */
SFA_FUNC(089CD01C, palette_anim_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0x09358BE0u;
    if (*B(o + 4) == 0) { c->r[31] = 0x089CD03Cu; f_089CD05C(c); }
    else { c->r[31] = 0x089CD050u; f_089CD120(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (match, record): the match settings (0x088D6564), three link slots reset (0x088D6440 on 0x09523438 /
   +0xD0 / +0x1A0) and the record's own (+8); its +30 = the match's +50, which moves on by +55; then the
   record takes the chosen stage's 20 bytes (0x08A058A0 by 0x09525DE4 / 2), three 0x3C-byte player blocks
   (0x0951C778 back by 0x354, into +0x9C back by 0x3C) and the 9 setting bytes 0x09523C00 (+0xD8)
   (0x089F3430 copies). */
SFA_FUNC(088D67C4, match_start_records) {
    gaddr m = ARG(0), r = ARG(1);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1];
    c->r[17] = m;
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    c->r[16] = r;
    c->r[31] = 0x088D67E8u; f_088D6564(c);
    c->r[18] = 0x09523438u;
    call_on(c, 0x09523438u, 0x088D67F8u, f_088D6440);
    call_on(c, 0x09523438u + 0xD0, 0x088D6800u, f_088D6440);
    call_on(c, 0x09523438u + 0x1A0, 0x088D6808u, f_088D6440);
    gaddr d = r + 8;
    c->r[18] = d; c->r[19] = 0x08A058A0u;
    call_on(c, d, 0x088D681Cu, f_088D6440);
    *H(d + 22) = (uint16_t)S16(m + 50);
    *H(m + 50) = (uint16_t)(S16(m + 50) + *B(m + 55));
    uint32_t k = (uint32_t)((int32_t)*B(0x09525DE4u) >> 1);
    ARG(0) = d; ARG(1) = 0x08A058A0u + 20 * k; ARG(2) = 0x14;
    c->r[31] = 0x088D685Cu; f_089F3430(c);
    gaddr to = d + 0x78 + 0x1C, from = 0x0951C778u;
    for (int32_t i = 2; i >= 0; i--) {
        c->r[18] = to; c->r[17] = from; c->r[19] = (uint32_t)i;
        ARG(0) = to; ARG(1) = from; ARG(2) = 0x3C;
        c->r[31] = 0x088D6888u; f_089F3430(c);
        to -= 0x3C; from -= 0x354;
    }
    ARG(0) = r + 0xD8; ARG(1) = 0x09523C00u; ARG(2) = 9;
    c->r[31] = 0x088D68ACu; f_089F3430(c);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (screen): counted (+2), 0x088B3D94, +3 / +8 - +10 / GAME +10 cleared; with 0x095176F3 at 20 or more
   list-0x7D objects 0 - 3 (0x088B3258) and 4 - 7 (0x08822EAC: on, +10 = 2, +78 = 0); then its eleven
   palettes (0x088B3C34). */
SFA_FUNC(088B3144, menu_screen_start_7d_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    *B(o + 2) = (uint8_t)(*B(o + 2) + 1);
    call_on(c, o, 0x088B3170u, f_088B3D94);
    *B(o + 3) = 0;
    *H(GAME + 10) = 0;
    *B(o + 10) = 0; *B(o + 9) = 0; *B(o + 8) = 0;
    if (S8(0x095176F3u) >= 20) {
        static const uint32_t b1[4] = { 0x088B31ACu, 0x088B31B8u, 0x088B31C4u, 0x088B31D0u };
        for (uint32_t i = 0; i < 4; i++) call2(c, 0x7D, i, b1[i], f_088B3258);
        static const uint32_t b2[4] = { 0x088B31DCu, 0x088B31FCu, 0x088B3214u, 0x088B322Cu };
        c->r[17] = 1; c->r[18] = 2;
        for (uint32_t i = 0; i < 4; i++) {
            call2(c, 0x7D, 4 + i, b2[i], f_08822EAC);
            gaddr n = c->r[2];
            *B(n) = 1; *B(n + 10) = 2; *B(n + 78) = 0;
        }
    }
    call_on(c, o, 0x088B3240u, f_088B3C34);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
