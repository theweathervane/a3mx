/* Small functions the scenarios reach, 7 - 41 instructions: effects' timed restarts, a fighter's picture
   table and its list-0x83 effect, an effect timer's steps, fighter objects' starts by count. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08876520(Cpu *c); void f_088FC444(Cpu *c); void f_0886EB48(Cpu *c); void f_089A8A18(Cpu *c);
void f_088219B0(Cpu *c); void f_0887DF88(Cpu *c); void f_0887FBE8(Cpu *c); void f_0886BF84(Cpu *c);
void f_0886DB94(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (effect): its timed restart (0x08876520). */
SFA_FUNC(0899D520, effect_timed_restart_call) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x0899D530u; f_08876520(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): 0x088FC444 for kinds after 0, then its timed restart (0x08876520). */
SFA_FUNC(088FC120, effect_timed_restart_kind) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (*B(o + 3) != 0) call_on(c, o, 0x088FC140u, f_088FC444);
    call_on(c, o, 0x088FC148u, f_08876520);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): +171 cleared, +12 = -1, its 39-word picture table (+416) from 0x08A9E62C, 0x0886EB48 (0x23,
   0x08A9E6C8), +86 = its side, +164 = 1 and its list-0x83 effect (0x089A8A18 (0)). */
SFA_FUNC(089A57B4, fighter_table_copy_23) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *B(f + 171) = 0;
    gaddr dst = *W(f + 416);
    *B(f + 12) = 0xFF;
    *W(c->r[29] + 20) = ra;
    for (uint32_t n = 0; n < 0x27; n++) *W(dst + 4 * n) = *W(0x08A9E62Cu + 4 * n);
    ARG(0) = f; ARG(1) = 0x23; ARG(2) = 0x08A9E6C8u;
    c->r[31] = 0x089A5814u; f_0886EB48(c);
    *H(f + 86) = (uint16_t)(int32_t)S8(f + 337);
    *W(f + 164) = 1;
    call2(c, f, 0, 0x089A5838u, f_089A8A18);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): +0x52 counted down; when it goes negative, on (+4, +5 cleared) with +73 set, else +5 on
   and 0x0887DF88 (0x14); while it lasts placed by its timer (0x0887FBE8) with +83 or +78 set, else
   0x088219B0. */
SFA_FUNC(0887F7F0, effect_timer_steps) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t t = (uint32_t)(int32_t)S8(o + 0x52) - 1u;
    *B(o + 0x52) = (uint8_t)t;
    *W(c->r[29] + 16) = ra;
    if (t & 0x80) {
        if (S8(o + 73) != 0) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
        else {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            call2(c, o, 0x14, 0x0887F84Cu, f_0887DF88);
        }
    } else if (S8(o + 0x53) != 0 || S8(o + 78) != 0) { c->r[31] = 0x0887F874u; f_0887FBE8(c); }
    else { c->r[31] = 0x0887F888u; f_088219B0(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when it has no effects (+536), or under two while +251 is set, and 0x0886BF84 allows it,
   on (+4 = 1, +6 = 7, +235 = 1, +236 = 1; +5 / +7 cleared, +11 = +697) and 0x0886DB94; 1 then, else 0. */
static void start_few(Cpu *c, uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t n = *B(o + 536);
    int few = S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0;
    c->r[2] = 0;
    if (few) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            c->r[17] = 1;
            *B(o + 236) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1;
            call_on(c, o, back2, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(088F3060, object_start_few_1) { start_few(c, 0x088F3090u, 0x088F30C8u); }
SFA_FUNC(088FD4D4, object_start_few_2) { start_few(c, 0x088FD504u, 0x088FD53Cu); }
