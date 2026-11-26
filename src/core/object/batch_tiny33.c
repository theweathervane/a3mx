/* Small functions the scenarios reach, 33 - 34 instructions: effects following and copying their
   owners, effects placed by tables, a list-5 effect made. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088215C4(Cpu *c); void f_088219B0(Cpu *c); void f_0882196C(Cpu *c); void f_0887684C(Cpu *c);
void f_088D3B50(Cpu *c); void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): follows its owner (+92): step 3 with it, +5 on while it is at step 2, at its x / y plus
   +112 / +114; then 0x088219B0 and drawn. */
SFA_FUNC(08981120, effect_follow_owner_92) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    gaddr p = *W(o + 92);
    *W(c->r[29] + 20) = ra;
    if (*B(p + 4) == 3) *B(o + 4) = 3;
    if (*B(p + 4) == 2) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(o + 18) = (uint16_t)(S16(o + 112) + S16(p + 18));
    *H(o + 22) = (uint16_t)(S16(o + 114) + S16(p + 22));
    call_on(c, o, 0x0898118Cu, f_088219B0);
    call_on(c, o, 0x08981194u, f_088215C4);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect, owner): +4 on, the owner's +12 / +13 / +14 / +24 and character (+338, as +73) taken; with a
   kind (+3) its +78 = kind + 0x12, +1 = 1 and the animation 0x08A98E78 by +78. */
SFA_FUNC(0899CFD0, effect_take_owner_char) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(p + 24);
    *B(o + 73) = (uint8_t)S8(p + 338);
    *B(o + 12) = (uint8_t)S8(p + 12);
    uint32_t kind = *B(o + 3);
    *B(o + 13) = *B(p + 13);
    *B(o + 78) = 0;
    *W(c->r[29] + 16) = ra;
    *B(o + 14) = *B(p + 14);
    if (kind != 0) {
        *B(o + 78) = (uint8_t)(kind + 0x12);
        *B(o + 1) = 1;
        call2(c, o, *W(0x08A98E78u + ((uint32_t)(int32_t)S8(o + 78) << 2)), 0x0899D048u, f_0882196C);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (marker): on (+4) once its fighter (+88) has left step 8; while at 8 at its x / y with +0 = 1 (or 2
   with no box flag) and 0x0887684C (the box's +10). */
SFA_FUNC(089B2C10, marker_box_flag) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    *W(c->r[29] + 16) = ra;
    if (*B(p + 6) != 8) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *H(o + 18) = (uint16_t)S16(p + 18);
        *H(o + 22) = (uint16_t)S16(p + 22);
        *B(o) = *B(*W(p + 296) + 10) != 0 ? 1 : 2;
        call2(c, o, *B(*W(p + 296) + 10), 0x089B2C74u, f_0887684C);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +24 / +140 (one less) from its owner (+88); unless 0x088D3B50 says so its step by +4
   (0x08AA7250); from +78 = 7 it becomes the owner's partner's (+92) +56. */
SFA_FUNC(089B78FC, effect_step_7250) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *H(o + 24) = *H(p + 24);
    *W(c->r[29] + 24) = ra;
    *B(o + 140) = (uint8_t)(S8(p + 140) - 1);
    c->r[31] = 0x089B792Cu; f_088D3B50(c);
    if (c->r[2] == 0) {
        gaddr h = *W(0x08AA7250u + ((uint32_t)*B(o + 4) << 2));
        ARG(0) = o; ARG(1) = h;
        c->r[31] = 0x089B7954u; rt_call_indirect(c, h);
    }
    if (!(S8(o + 78) < 7)) *W(*W(p + 92) + 56) = o;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner, type, a, b): a list-5 effect (0x08822EAC) of the type at the owner's fighter (+92: x, its
   floor +132, direction bit 0, +14) with +9 = a, +78 = b, +12 cleared and +24 the owner's. */
SFA_FUNC(0887E664, effect_make_5) {
    gaddr o = ARG(0);
    uint32_t type = ARG(1) & 0xFF, a = ARG(2) & 0xFF, b = ARG(3) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    c->r[16] = o; c->r[18] = a; c->r[17] = b;
    *W(c->r[29] + 28) = ra;
    call2(c, 5, type, 0x0887E690u, f_08822EAC);
    gaddr n = c->r[2];
    if (n != 0) {
        gaddr f = *W(o + 92);
        *B(n + 9) = (uint8_t)a;
        *B(n + 14) = *B(f + 14);
        *B(n + 78) = (uint8_t)b;
        *H(n + 18) = (uint16_t)S16(f + 18);
        *H(n + 22) = (uint16_t)S16(f + 132);
        *B(n + 11) = (uint8_t)(*B(f + 11) & 1);
        *B(n + 12) = 0;
        *H(n + 24) = *H(o + 24);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its timer (+83) counted down to 0; placed at its owner's (+88) whole x / y plus its own
   +112 (turned with the owner's direction, which it takes) / +114 and the 8-byte row of 0x08A24698 by
   the timer. */
SFA_FUNC(0887FBE8, effect_place_by_timer) {
    gaddr o = ARG(0);
    gaddr p = *W(o + 88);
    int32_t t = S8(o + 83);
    if (t != 0) { t -= 1; *B(o + 83) = (uint8_t)t; }
    gaddr row = 0x08A24698u + ((uint32_t)t << 3);
    uint32_t dir = *B(p + 11);
    *B(o + 11) = (uint8_t)dir;
    uint32_t dx = (uint32_t)(int32_t)S16(o + 112);
    if (dir != 0) dx = 0u - dx;
    *W(o + 16) = (*W(p + 16) & 0xFFFF0000u) + (dx << 16) + *W(row);
    *W(o + 20) = (*W(p + 20) & 0xFFFF0000u) + ((uint32_t)(int32_t)S16(o + 114) << 16) + *W(row + 4);
    ARG(1) = *W(o + 20);
}

/* (effect, owner): placed at the owner's x / y plus row +82 of 0x08A24840 (x turned with the owner's
   direction) and row +78 of 0x08A248E8. */
SFA_FUNC(088804E0, effect_place_two_rows) {
    gaddr o = ARG(0), p = ARG(1);
    gaddr r1 = 0x08A24840u + ((uint32_t)(int32_t)S16(o + 82) << 2);
    int32_t y = (int16_t)(S16(p + 22) + S16(r1 + 2));
    int32_t dx = S16(r1);
    if (*B(p + 11) != 0) dx = (int16_t)-dx;
    gaddr r2 = 0x08A248E8u + ((uint32_t)(int32_t)S8(o + 78) << 2);
    *H(o + 18) = (uint16_t)((int16_t)(dx + S16(p + 18)) + S16(r2));
    *H(o + 22) = (uint16_t)(y + S16(r2 + 2));
}
