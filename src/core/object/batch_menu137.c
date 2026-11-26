/* Menu cursor steps and two animation helpers, 22 - 28 instructions. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0889CE64(Cpu *c); void f_0889C640(Cpu *c); void f_088215C4(Cpu *c); void f_0889CCD4(Cpu *c);
void f_0889C954(Cpu *c); void f_0889D01C(Cpu *c); void f_08855B3C(Cpu *c); void f_0882581C(Cpu *c);
void f_0886175C(Cpu *c);

static void enter(Cpu *c, gaddr m, gaddr x, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = sv[1];
    c->r[17] = m;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = x;
}
static void leave(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2];
    c->r[29] += 0x20;
}
static void call2(Cpu *c, gaddr m, gaddr x, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = m; ARG(1) = x; c->r[31] = back; fn(c);
}

/* (menu, x): when 0x0889CE64 says so the step (+6) moves on (and with `keep`, +16 = +114 << 16); then
   the choice (0x0889C640) and 0x088215C4, or the game cursor (0x0889CCD4). */
static void step_when(Cpu *c, int keep, uint32_t back1, void (*fn)(Cpu *), uint32_t back2, uint32_t back3) {
    gaddr m = ARG(0), x = ARG(1);
    uint32_t sv[3];
    enter(c, m, x, sv);
    c->r[31] = back1; f_0889CE64(c);
    if (c->r[2] != 0) {
        uint32_t k = *H(m + 114);
        *B(m + 6) = (uint8_t)(*B(m + 6) + 1);
        if (keep) *W(m + 16) = (uint32_t)(int32_t)(int16_t)k << 16;
    }
    call2(c, m, x, fn, back2);
    if (back3) { ARG(0) = m; c->r[31] = back3; f_088215C4(c); }
    leave(c, sv);
}
SFA_FUNC(0889C5AC, menu_choice_step) { step_when(c, 0, 0x0889C5C8u, f_0889C640, 0x0889C5E8u, 0x0889C5F0u); }
SFA_FUNC(0889C4D4, menu_choice_step_keep) { step_when(c, 1, 0x0889C4F0u, f_0889C640, 0x0889C51Cu, 0x0889C524u); }
SFA_FUNC(0889CBC4, menu_cursor_game_keep) { step_when(c, 1, 0x0889CBE0u, f_0889CCD4, 0x0889CC0Cu, 0); }

/* (menu, item): with the item's +78 bit `bit` the step (+6) moves on, +114 = +112, +96 cleared and the
   cursor cue (0x0889D01C); then the cursor (`fn`). */
static void confirm(Cpu *c, uint32_t bit, uint32_t back1, void (*fn)(Cpu *), uint32_t back2) {
    gaddr m = ARG(0), it = ARG(1);
    uint32_t sv[3];
    int32_t f = S8(it + 78);
    enter(c, m, it, sv);
    if (f & bit) {
        int32_t k = S16(m + 112);
        *B(m + 6) = (uint8_t)(*B(m + 6) + 1);
        *H(m + 114) = (uint16_t)k;
        *W(m + 96) = 0;
        ARG(0) = m; c->r[31] = back1; f_0889D01C(c);
    }
    call2(c, m, it, fn, back2);
    leave(c, sv);
}
SFA_FUNC(0889C8A0, menu_item_confirm) { confirm(c, 4, 0x0889C8E4u, f_0889C954, 0x0889C8F0u); }
SFA_FUNC(0889CC20, menu_game_confirm) { confirm(c, 8, 0x0889CC64u, f_0889CCD4, 0x0889CC70u); }

/* (attacker, hit): for a hit of kind 3 (+44) once the counter at 0x095176F2 reaches 8, the attacker's +8
   counts and 0x08855B3C; then the gauges fill (0x0882581C). */
SFA_FUNC(08825520, hit_gauges) {
    gaddr a = ARG(0), h = ARG(1);
    uint32_t sv[3];
    int32_t kind = S8(h + 44);
    enter(c, a, h, sv);
    if (kind == 3 && S8(0x095176F2u) >= 8) {
        uint32_t n = (uint32_t)*H(a + 8) + 1;
        *H(a + 8) = (uint16_t)n;
        ARG(0) = n; c->r[31] = 0x08825568u; f_08855B3C(c);
    }
    call2(c, a, h, f_0882581C, 0x08825574u);
    leave(c, sv);
}

/* (fighter): a step with +1 bit 6 ends the animation (+75 = -1); else the animation jumps by +6 steps
   (10 bytes each) and is applied. */
SFA_FUNC(08904248, fighter_anim_jump) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr st = *W(o + 28);
    int32_t f = S8(st + 1);
    *W(c->r[29] + 16) = ra;
    if (f & 0x40) *B(o + 75) = 0xFF;
    else {
        st += (uint32_t)(S16(st + 6) * 10);
        *W(o + 28) = st;
        c->r[31] = (f & 0x80) ? 0x08904298u : 0x089042ACu;
        f_0886175C(c);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
