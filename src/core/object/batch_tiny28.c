/* Small functions the scenarios reach, 27 - 32 instructions: fighter objects' starts and a screen's
   timed exit. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089DF164(Cpu *c); void f_089DF1E0(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c);
void f_0886DBE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (screen, b): 0x089DF1E0 (screen, b) or +16 counted below 0 moves it on (+4 on, 0x089DF164). */
SFA_FUNC(089DF0F8, screen_timed_quit) {
    gaddr o = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = o;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = b;
    c->r[31] = 0x089DF114u; f_089DF1E0(c);
    int go = c->r[2] != 0;
    if (!go) {
        int16_t t = (int16_t)(S16(o + 16) - 1);
        *H(o + 16) = (uint16_t)t;
        go = t < 0;
    }
    if (go) {
        *H(o + 4) = (uint16_t)(*H(o + 4) + 1);
        call2(c, o, b, 0x089DF150u, f_089DF164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

enum { NEED_ANY, NEED_386_SET, NEED_386_CLEAR };
enum { EXTRA_NONE = 0, EXTRA_123 = 1, EXTRA_196 = 2 };

/* (object): when allowed (+386's sign as `need`, then 0x0886BF84), on (+4 = 1, +6 = 7, +235 = 1, +236
   = k236; +5 / +7 cleared, +11 = +697; +123 or +196 - +199 cleared as `extra`) and `then`; 1 then,
   else 0. `zero_first`: s1 is 0 from the start (the answer is s1). */
static void start_obj(Cpu *c, int need, uint32_t k236, int extra, int zero_first, void (*then)(Cpu *),
                      uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (zero_first) c->r[17] = 0;
    int ok = 0;
    int allowed = need == NEED_ANY || (need == NEED_386_SET ? S8(o + 386) >= 0 : S8(o + 386) < 0);
    if (allowed) {
        c->r[31] = back1; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            *B(o + 236) = (uint8_t)k236;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            if (extra == EXTRA_123) *B(o + 123) = 0;
            if (extra == EXTRA_196) { *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0; *B(o + 196) = 0; }
            c->r[17] = 1;
            call_on(c, o, back2, then);
            ok = 1;
        }
    }
    c->r[2] = zero_first ? c->r[17] : (uint32_t)ok;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(08901B7C, object_start_386_set_1) { start_obj(c, NEED_386_SET, 1, EXTRA_123, 1, f_0886DBE8, 0x08901BA4u, 0x08901BE4u); }
SFA_FUNC(08901CC4, object_start_386_neg_3) { start_obj(c, NEED_386_CLEAR, 3, EXTRA_NONE, 1, f_0886DB94, 0x08901CECu, 0x08901D2Cu); }
SFA_FUNC(08906350, object_start_1_dbe8)    { start_obj(c, NEED_ANY, 1, EXTRA_NONE, 0, f_0886DBE8, 0x08906368u, 0x089063A0u); }
SFA_FUNC(08910154, object_start_1_dbe8b)   { start_obj(c, NEED_ANY, 1, EXTRA_NONE, 0, f_0886DBE8, 0x0891016Cu, 0x089101A4u); }
SFA_FUNC(08914CB8, object_start_0_db94)    { start_obj(c, NEED_ANY, 0, EXTRA_NONE, 0, f_0886DB94, 0x08914CD0u, 0x08914D08u); }
SFA_FUNC(08918D20, object_start_0_196)     { start_obj(c, NEED_ANY, 0, EXTRA_196, 1, f_0886DB94, 0x08918D3Cu, 0x08918D88u); }
