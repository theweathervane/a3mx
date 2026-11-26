/* Small callers of the animation and menu-cursor functions, 12 - 20 instructions: a fighter's animation
   chosen from its three lists, a part's animation set, a fighter's one-off cue, menu cursors that step
   on when 0x0889CE64 says so. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C4F8(Cpu *c); void f_0886175C(Cpu *c); void f_088D3CD8(Cpu *c); void f_08861AE0(Cpu *c);
void f_0889CE64(Cpu *c); void f_0889C954(Cpu *c); void f_0889CCD4(Cpu *c);

/* (fighter): once (+774 set to -1), the 0x200 cue set (0x0887C4F8) from its side. */
SFA_FUNC(0885D134, fighter_cues_once) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(1) = 0xFFFFFFFFu;
    if (S8(o + 774) == 0) {
        *B(o + 774) = 0xFF;
        ARG(1) = (uint32_t)(int32_t)S8(o + 337);
        c->r[31] = 0x0885D158u; f_0887C4F8(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, n): animation n (+84) of the character's list `which` (+416 +0 / +4 / +8) made current
   (+28) and applied. */
static void anim_pick(Cpu *c, uint32_t which, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr list = *W(*W(o + 416) + which);
    *H(o + 84) = (uint16_t)n;
    *W(c->r[29] + 16) = ra;
    *W(o + 28) = *W(list + ((n & 0xFFFF) << 2));
    c->r[31] = back; f_0886175C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0886163C, fighter_anim_pick0) { anim_pick(c, 0, 0x08861668u); }
SFA_FUNC(088616EC, fighter_anim_pick1) { anim_pick(c, 4, 0x08861718u); }
SFA_FUNC(08861724, fighter_anim_pick2) { anim_pick(c, 8, 0x08861750u); }

/* (part, list, n): the part's animation list (+352) set, step n made current (+28) and applied, +152 = 1. */
SFA_FUNC(088D3D94, part_anim_set) {
    gaddr o = ARG(0), list = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(o + 352) = list;
    *W(c->r[29] + 20) = ra;
    *W(o + 28) = *W(list + (ARG(2) << 2));
    ARG(0) = o; c->r[31] = 0x088D3DC0u; f_088D3CD8(c);
    *W(o + 152) = 1;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, n): step n (+84) of the third list's table (10 bytes each) set and applied unless current. */
SFA_FUNC(0886E238, fighter_anim_step_set) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr st = *W(*W(*W(o + 416) + 8)) + (n & 0xFFFF) * 10;
    *H(o + 84) = (uint16_t)n;
    *W(c->r[29] + 16) = ra;
    if (*W(o + 28) != st) { ARG(1) = st; c->r[31] = 0x0886E278u; f_08861AE0(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (menu, x): the menu's step (+6) moves on when 0x0889CE64 says so; then the cursor (`fn`). */
static void cursor_step(Cpu *c, uint32_t back1, void (*fn)(Cpu *), uint32_t back2) {
    gaddr m = ARG(0), x = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = m;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = x;
    c->r[31] = back1; f_0889CE64(c);
    if (c->r[2] != 0) *B(m + 6) = (uint8_t)(*B(m + 6) + 1);
    ARG(0) = m; ARG(1) = x;
    c->r[31] = back2; fn(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0889C904, menu_cursor_item_step) { cursor_step(c, 0x0889C920u, f_0889C954, 0x0889C940u); }
SFA_FUNC(0889CC84, menu_cursor_game_step) { cursor_step(c, 0x0889CCA0u, f_0889CCD4, 0x0889CCC0u); }
