/* Super-combo starts: when the choice (+386) and the distance allow it and 0x0886C6EC says the gauge is
   there, the fighter enters the super state (+4..+7 = 1 / 0 / 8 / 0), takes its super number (+236),
   faces the opponent and starts through 0x0886D2EC or 0x0886D3E0; the result is whether it started.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c); void f_0886D3E0(Cpu *c); void f_089439DC(Cpu *c);
void f_08902314(Cpu *c); void f_089A536C(Cpu *c);

/* the frame: s0 / ra (FR_S0), or s0 / s1 / ra with s1 the result, zeroed first (FR_S1Z) or not (FR_S1) */
enum { FR_S0, FR_S1Z, FR_S1 };
/* the choice test: none, not 0 stops it, above 0 stops it */
enum { T_NONE, T_NE0, T_GT0 };
/* the result of a FR_S0 start: 1, or whatever the start left in v0 */
enum { R_ONE, R_LEFT };
typedef struct {
    uint8_t frame, test, near48, ret;
    int16_t b236;              /* -1 leaves +236 */
    int8_t b235;               /* -1 leaves +235 */
    uint8_t clr696;
    uint8_t d3e0;              /* 1: start through 0x0886D3E0, 0: 0x0886D2EC */
    void (*pre)(Cpu *);        /* after the gauge check, before the state */
    void (*mid)(Cpu *);        /* after the state, with a1 = mid_a1 */
    uint32_t mid_a1;
    uint32_t b_gauge, b_pre, b_start;
} Super;

static void super_try(Cpu *c, const Super *k) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    if (k->frame == FR_S0) *W(sp + 20) = ra;
    else { *W(sp + 20) = s1; *W(sp + 24) = ra; }
    if (k->frame == FR_S1Z) c->r[17] = 0;
    c->r[16] = o;
    int32_t ch = S8(o + 386);
    int ok = 0;
    if (k->test == T_NE0 && ch != 0) goto done;
    if (k->test == T_GT0 && ch > 0) goto done;
    if (k->near48 && S16(o + 366) < 48) goto done;
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
    if (k->b236 >= 0) *B(o + 11) = *B(o + 697);
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
        c->r[2] = ok ? c->r[17] : 0;
        c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    }
    c->r[29] += 0x20;
}

SFA_FUNC(08983EFC, super_13) {
    static const Super k = { FR_S1Z, T_NE0, 0, 0, 0xD, -1, 0, 1, 0, 0, 0, 0x08983F24u, 0, 0x08983F5Cu };
    super_try(c, &k);
}
SFA_FUNC(08901EB0, super_6) {
    static const Super k = { FR_S1Z, T_GT0, 0, 0, 6, 1, 0, 1, 0, 0, 0, 0x08901ED8u, 0, 0x08901F18u };
    super_try(c, &k);
}
SFA_FUNC(08901FBC, super_8) {
    static const Super k = { FR_S1Z, T_NE0, 0, 0, 8, 1, 0, 0, 0, 0, 0, 0x08901FE4u, 0, 0x08902024u };
    super_try(c, &k);
}
SFA_FUNC(089739F8, super_14) {
    static const Super k = { FR_S0, T_NE0, 0, R_ONE, 0xE, -1, 1, 0, 0, 0, 0, 0x08973A18u, 0, 0x08973A50u };
    super_try(c, &k);
}
SFA_FUNC(08973A78, super_15) {
    static const Super k = { FR_S0, T_NE0, 0, R_ONE, 0xF, -1, 1, 0, 0, 0, 0, 0x08973A98u, 0, 0x08973AD0u };
    super_try(c, &k);
}
SFA_FUNC(08909998, super_3_near) {
    static const Super k = { FR_S1Z, T_NE0, 1, 0, 3, -1, 0, 0, 0, 0, 0, 0x089099D0u, 0, 0x08909A08u };
    super_try(c, &k);
}
SFA_FUNC(08943550, super_39dc_near) {
    static const Super k = { FR_S0, T_NE0, 1, R_ONE, -1, -1, 0, 1, 0, f_089439DC, 0xA, 0x08943580u, 0x089435A8u,
                             0x089435B0u };
    super_try(c, &k);
}
SFA_FUNC(08901F30, super_7_2314) {
    static const Super k = { FR_S1Z, T_NE0, 0, 0, 7, 1, 0, 1, f_08902314, 0, 0, 0x08901F58u, 0x08901F6Cu,
                             0x08901FA4u };
    super_try(c, &k);
}
SFA_FUNC(089A34FC, super_9_536c) {
    static const Super k = { FR_S0, T_NE0, 0, R_LEFT, 9, 1, 0, 1, f_089A536C, 0, 0, 0x089A351Cu, 0x089A352Cu,
                             0x089A3564u };
    super_try(c, &k);
}
SFA_FUNC(089A3588, super_a_536c) {
    static const Super k = { FR_S0, T_NE0, 0, R_LEFT, 0xA, 1, 0, 0, f_089A536C, 0, 0, 0x089A35A8u, 0x089A35B8u,
                             0x089A35F0u };
    super_try(c, &k);
}
SFA_FUNC(088EF1F8, super_5) {
    static const Super k = { FR_S1, T_NE0, 0, 0, 5, 1, 0, 1, 0, 0, 0, 0x088EF21Cu, 0, 0x088EF258u };
    super_try(c, &k);
}
SFA_FUNC(088F2F60, super_4_near) {
    static const Super k = { FR_S1Z, T_NE0, 1, 0, 4, 1, 0, 1, 0, 0, 0, 0x088F2F98u, 0, 0x088F2FD8u };
    super_try(c, &k);
}
