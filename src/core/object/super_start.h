/* The super-combo start shared by the batch_supers files: when the choice (+386) and the distance (+366)
   allow it and 0x0886C6EC says the gauge is there, the fighter enters the super state (+4..+7 = 1 / 0 /
   8 / 0), takes its super number (+236), faces the opponent and starts through 0x0886D2EC or 0x0886D3E0;
   the result (v0) is whether it started. Each original keeps its own frame and order of calls, so the
   table says which.
 */
#ifndef A3MX_SUPER_START_H
#define A3MX_SUPER_START_H
#include "step.h"
#include "../guest_mem.h"

void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c); void f_0886D3E0(Cpu *c);

/* the frame: s0 / ra (FR_S0), or s0 / s1 / ra with s1 the result: zeroed first (FR_S1Z), set to 1 first
   (FR_S1ONE) or left until the start (FR_S1) */
enum { FR_S0, FR_S1Z, FR_S1, FR_S1ONE };
/* the choice test: none, or what stops it: not 0, above 0, exactly 1 */
enum { T_NONE, T_NE0, T_GT0, T_EQ1, T_NOT_M1 };   /* T_NOT_M1: anything but -1 (no choice made) stops it */
/* the distance test: none, closer than 48 stops it (signed), or the same on its low 16 bits (unsigned) */
enum { N_ANY, N_48, N_48U };
/* the result of a start: 1 (s1 in the s1 frames), or whatever the start left in v0 */
enum { R_ONE, R_LEFT };
typedef struct {
    uint8_t frame, test, near, ret, busy536;   /* busy536: +536 set stops it */
    uint8_t lim;               /* the distance limit when not 48 */
    uint16_t busy_b, busy_w;   /* a byte / word field whose being set stops it (0: none) */
    uint8_t set823;            /* +823 = +81 just before the start */
    uint8_t need338;           /* the character (+338) it belongs to (0: any) */
    uint16_t clr_b;            /* one more byte field cleared with the state (0: none) */
    int16_t b236;              /* -1 leaves +236 (and the facing) */
    int8_t b235;               /* -1 leaves +235 */
    uint8_t clr696, clr196, clr836;   /* clr196: how many of +196.. to clear */
    uint8_t d3e0;              /* start through 0x0886D3E0, else 0x0886D2EC */
    void (*pre)(Cpu *);        /* after the gauge check, before the state */
    void (*mid)(Cpu *);        /* after the state, with a1 = mid_a1 */
    uint32_t mid_a1;
    uint32_t b_gauge, b_pre, b_start;
} Super;

static inline void super_try(Cpu *c, const Super *k) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    if (k->frame == FR_S0) *W(sp + 20) = ra;
    else { *W(sp + 20) = s1; *W(sp + 24) = ra; }
    if (k->frame == FR_S1Z) c->r[17] = 0;
    if (k->frame == FR_S1ONE) c->r[17] = 1;
    c->r[16] = o;
    int32_t ch = S8(o + 386);
    int ok = 0;
    if (k->need338 && S8(o + 338) != k->need338) goto done;
    if (k->test == T_NE0 && ch != 0) goto done;
    if (k->test == T_GT0 && ch > 0) goto done;
    if (k->test == T_EQ1 && ch == 1) goto done;
    if (k->test == T_NOT_M1 && ch != -1) goto done;
    int32_t lim = k->lim ? k->lim : 48;
    if (k->near == N_48 && S16(o + 366) < lim) goto done;
    if (k->near == N_48U && *H(o + 366) < lim) goto done;
    if (k->busy536 && *B(o + 536) != 0) goto done;
    if (k->busy_b && *B(o + k->busy_b) != 0) goto done;
    if (k->busy_w && *W(o + k->busy_w) != 0) goto done;
    ARG(0) = o; c->r[31] = k->b_gauge; f_0886C6EC(c);
    if (c->r[2] == 0) goto done;
    if (k->pre) {
        if (k->frame != FR_S0) c->r[17] = 1;
        ARG(0) = o; c->r[31] = k->b_pre; k->pre(c);
    }
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
    if (k->mid) { ARG(0) = o; ARG(1) = k->mid_a1; c->r[31] = k->b_pre; k->mid(c); }
    if (k->b236 >= 0) *B(o + 236) = (uint8_t)k->b236;
    if (k->b235 >= 0) *B(o + 235) = (uint8_t)k->b235;
    if (k->clr696) *B(o + 696) = 0;
    for (int i = 0; i < k->clr196; i++) *B(o + 196 + i) = 0;
    if (k->clr836) *W(o + 836) = 0;
    if (k->b236 >= 0) *B(o + 11) = *B(o + 697);
    if (k->set823) *B(o + 0x334 + 3) = *B(o + 81);
    if (k->clr_b) *B(o + k->clr_b) = 0;
    if (k->frame != FR_S0) c->r[17] = 1;
    ARG(0) = o; c->r[31] = k->b_start;
    if (k->d3e0) f_0886D3E0(c); else f_0886D2EC(c);
    ok = 1;
done:
    if (k->frame == FR_S0) {
        if (!ok) c->r[2] = 0;
        else if (k->ret == R_ONE) c->r[2] = 1;
        c->r[16] = s0; c->r[31] = ra;
    } else {
        if (!ok) c->r[2] = 0;
        else if (k->ret == R_ONE) c->r[2] = c->r[17];
        c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    }
    c->r[29] += 0x20;
}
#endif
