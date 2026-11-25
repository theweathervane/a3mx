/* Small functions the scenarios reach, 18 - 27 instructions: a menu overlay's moves, two owned
   objects' step choices, a fighter's lookups (whether its feet are above a line, its partner, the last
   live entry of a list) and an object's start. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_088B118C(Cpu *c); void f_088BD99C(Cpu *c); void f_088BDA40(Cpu *c); void f_088BDA60(Cpu *c);
void f_088DD114(Cpu *c); void f_088DD2A8(Cpu *c); void f_0886BF84(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

/* (overlay): moved by `dy` (+18), +100 cleared (set first while GAME +16's high byte is 0), then
   0x088B118C. */
static void overlay_step(Cpu *c, int dy, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    if (dy != 0) *H(o + 18) = (uint16_t)(S16(o + 18) + dy);
    *W(c->r[29] + 16) = ra;
    if ((((int32_t)S16(GAME + 16) & 0xFF00) >> 8) == 0) *W(o + 100) = 1;
    uint32_t was = *W(o + 100);
    if (was != 0) *W(o + 100) = 0;
    ARG(1) = was;
    call0(c, back, f_088B118C);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088B110C, menu_overlay_still) { overlay_step(c, 0, 0x088B1148u); }
SFA_FUNC(088B0EA0, menu_overlay_up)    { overlay_step(c, -1, 0x088B0EE8u); }
SFA_FUNC(088B0FF0, menu_overlay_down)  { overlay_step(c, 1, 0x088B1038u); }

/* (object): while its owner (+88) is live and points back at it (+108): 0x088BD99C on its first step
   (+4 = 0), else 0x088BDA40; otherwise 0x088BDA60. */
SFA_FUNC(088BD930, owned_object_step_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr p = *W(o + 88);
    if (*B(p) != 0 && *W(p + 108) == o) {
        if (*B(o + 4) == 0) call0(c, 0x088BD968u, f_088BD99C);
        else call0(c, 0x088BD97Cu, f_088BDA40);
    } else call0(c, 0x088BD990u, f_088BDA60);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 1 when y (+22) plus its box's +3 (the box at +296; none in its step 1 / 0 / 5) is under
   +132. */
SFA_FUNC(088D35D0, fighter_under_line) {
    gaddr f = ARG(0);
    int32_t d;
    if (*B(f + 4) == 1 && *B(f + 5) == 0 && *B(f + 6) == 5) d = 0;
    else d = (int16_t)*B(*W(f + 296) + 3);
    int32_t y = S16(f + 22);
    int32_t sum = y + d;
    int lt = sum < S16(f + 132);
    ARG(1) = (uint32_t)sum; ARG(2) = (uint32_t)y; ARG(0) = (uint32_t)lt;
    c->r[2] = lt ? 1 : 0;
}

/* (fighter): of the four fighters flagged in its +594 (0x08A17F88's list) the last one not hidden
   (+117 bit 7), or its own (+92) when that comes first; 0 with none. */
SFA_FUNC(088D4144, fighter_flagged_other) {
    gaddr f = ARG(0);
    int32_t flags = S8(f + 594);
    gaddr own = *W(f + 92);
    c->r[2] = 0;
    for (uint32_t n = 0; (int32_t)n < 4; n = (n + 1) & 0xFF) {
        if (!(flags & (1 << n))) continue;
        gaddr p = *W(0x08A17F88u + (n << 2));
        if (*B(p + 117) & 0x80) continue;
        c->r[2] = p;
        if (p == own) break;
    }
    ARG(0) = own;
}

/* The last list entry (0x088DD114's count, 0x088DD2A8 (n) down to 0) whose +4 is set: that +4, or 0. */
SFA_FUNC(088DF7BC, last_live_entry) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    call0(c, 0x088DF7D0u, f_088DD114);
    uint32_t found = 0;
    for (c->r[16] = c->r[2]; (int32_t)c->r[16] >= 0; c->r[16]--) {
        call_on(c, c->r[16], 0x088DF7E4u, f_088DD2A8);
        if (c->r[2] != 0 && *W(c->r[2] + 4) != 0) { found = *W(c->r[2] + 4); break; }
    }
    c->r[2] = found;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 0xD; +5 / +7 cleared, +11
   = +697); 1 then, else 0. */
SFA_FUNC(08909CBC, object_start_bf84) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    c->r[31] = 0x08909CD8u; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 236) = 0xD;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[17] = 1;
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
