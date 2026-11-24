/* Small steps (0.145.0): the last 5 ready functions of 61 - 100 instructions. Each fighter's pause
   check, the fighters' flag +237 fading the fight, the edges of three fighters for the camera, a
   network menu item placed, and four flying objects by row. Offsets stay numbers until something shows
   what they are. What the original leaves in a0 - t1 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define NET       0x09523980u
#define CAM_0     0x092E8800u

void f_08828AF4(Cpu *c);   /* game_pause_check */
void f_08822B88(Cpu *c);   /* clear_0950FF40 */
void f_0886E3CC(Cpu *c);   /* fighter_call_08805310_by_kind */
void f_0881E760(Cpu *c);   /* camera_span_by_pair */
void f_0882DE3C(Cpu *c);   /* fade_layout_60 */
void f_0882E0D0(Cpu *c);   /* fade_reset */
void f_089F3430(Cpu *c);   /* game_memcpy */
void f_08821368(Cpu *c);   /* game_random */
void f_08822F1C(Cpu *c);   /* object_make_16 */

static const gaddr FIGHTERS[4] = { FIGHTER0, FIGHTER1, FIGHTER2, FIGHTER3 };

/* Each fighter's call by kind (kind, value). */
static void fighters_call(Cpu *c, uint32_t kind, uint32_t value, int only_237) {
    for (int i = 0; i < 4; i++) {
        if (only_237 && S8(FIGHTERS[i] + 237) == 0) continue;
        c->r[4] = FIGHTERS[i]; c->r[5] = kind; c->r[6] = value; f_0886E3CC(c);
    }
}

/* (game, battle): unless the game's +89 / +93 or the battle's +14, on screen 3 each fighter's pause
   check; if that set the game's +89, 0x0950FF40 cleared, the fighters' kind 0 call with 1, and in mode 2
   the bytes 0x0950FF60 +0 - +3, +21, +22 cleared. +135 cleared. */
SFA_FUNC(088289A4, fighters_pause_check) {
    gaddr g = ARG(0), bt = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    c->r[29] -= 0x30;
    c->r[16] = g;
    if (S8(g + 89) == 0 && S8(g + 93) == 0 && S8(bt + 14) == 0 && *H(g + 6) == 3) {
        c->r[20] = FIGHTER0; c->r[19] = FIGHTER1; c->r[18] = FIGHTER2; c->r[17] = FIGHTER3;
        for (int i = 0; i < 4; i++) { c->r[4] = g; c->r[5] = FIGHTERS[i]; c->r[6] = (uint32_t)i; f_08828AF4(c); }
        if (S8(g + 89) != 0) {
            f_08822B88(c);
            fighters_call(c, 0, 1, 0);
            if (S8(g + 114) == 2) {
                static const uint8_t at[] = { 22, 21, 3, 2, 1, 0 };
                for (unsigned i = 0; i < sizeof at; i++) *B(0x0950FF60u + at[i]) = 0;
            }
        }
    }
    *B(g + 135) = 0;
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
}

/* (_, record): +37 = any fighter's +237. Without +7: when that is set +38 = -1 (a negative one leaves
   +38, and +38 at 0 then fades the layout, +38 = -1, and each fighter with +237 gets kind 9 with
   0x12C); with +7, or with none set, a +38 left over is cleared, the fade reset and the fighters' kind 0
   with 1. */
SFA_FUNC(0882D998, fighters_flag_237) {
    gaddr rec = ARG(1);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x30;
    c->r[19] = FIGHTER2; c->r[18] = FIGHTER3; c->r[17] = FIGHTER0; c->r[16] = FIGHTER1;
    int8_t any = (int8_t)(S8(FIGHTER2 + 237) | S8(FIGHTER3 + 237) | S8(FIGHTER0 + 237) | S8(FIGHTER1 + 237));
    *B(rec + 37) = (uint8_t)any;
    c->r[4] = rec;
    if (S8(rec + 7) == 0 && S8(rec + 37) != 0) {
        if (S8(rec + 37) >= 0) *H(rec + 38) = 0xFFFF;
        if (S16(rec + 38) == 0) {
            *H(rec + 38) = 0xFFFF;
            c->r[4] = rec; f_0882DE3C(c);
            fighters_call(c, 9, 0x12C, 1);
        }
    } else if (S16(rec + 38) != 0) {
        *H(rec + 38) = 0;
        c->r[4] = rec; f_0882E0D0(c);
        fighters_call(c, 0, 1, 0);
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* (a, b, d, left, right): with a fighter's +579 set the camera span of the other two (a skips first,
   then b, then d); with none, *left = the lowest x - half width (+232) and *right the highest x + it. */
SFA_FUNC(0881E8B0, edges_of_three) {
    gaddr a = ARG(0), b = ARG(1), d = ARG(2), left = ARG(3), right = c->r[8];
    c->r[29] -= 0x20;
    if (S8(a + 579) != 0) {
        c->r[4] = d; c->r[5] = b; c->r[6] = left; c->r[7] = right; c->r[8] = left; c->r[9] = right; f_0881E760(c);
    } else if (S8(b + 579) != 0) {
        c->r[4] = a; c->r[5] = d; c->r[6] = left; c->r[7] = right; c->r[8] = a; c->r[9] = right; f_0881E760(c);
    } else if (S8(d + 579) != 0) {
        c->r[4] = a; c->r[5] = b; c->r[6] = left; c->r[7] = right; c->r[8] = a; c->r[9] = (uint32_t)(int32_t)S8(d + 579); f_0881E760(c);
    } else {
        int16_t la = (int16_t)(S16(a + 18) - S16(a + 232));
        int16_t lb = (int16_t)(S16(b + 18) - S16(b + 232));
        int16_t ld = (int16_t)(S16(d + 18) - S16(d + 232));
        int16_t lo = ld < lb ? ld : lb;
        if (lo < la) la = lo;
        *H(left) = (uint16_t)la;
        int16_t ra = (int16_t)(S16(a + 18) + S16(a + 232));
        int16_t rb = (int16_t)(S16(b + 18) + S16(b + 232));
        int16_t rd = (int16_t)(S16(d + 18) + S16(d + 232));
        int16_t hi = rb < rd ? rd : rb;
        if (ra < hi) ra = hi;
        *H(right) = (uint16_t)ra;
    }
    c->r[29] += 0x20;
}

/* (item): a network menu item's start: running, +148 7, +86 0x18, +164 1, +6 0xFF, +10 = NET +47; placed
   by type (+3) through the table 0x08A01630: x 0xC0, 0x110 or type * 16 - 0x90, y 0xF0 less the row's
   byte +3 in 0x08A3EA34 (11 bytes a row by NET +47); or left where it is. */
SFA_FUNC(088EBEF8, net_menu_item_place) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 0;
    *W(o + 148) = 7; *H(o + 86) = 0x18; *W(o + 164) = 1;
    *B(o + 11) = 0; *B(o + 14) = 0; *H(o + 38) = 0; *B(o + 12) = 0;
    *H(o + 24) = 7; *B(o + 9) = 0; *H(o + 18) = 0; *H(o + 22) = 0;
    *B(o + 6) = 0xFF;
    int32_t n = S8(NET + 47);
    uint32_t t = *B(o + 3);
    *B(o + 10) = (uint8_t)n;
    if (t >= 0x27u) return;
    uint32_t to = *W(0x08A01630u + t * 4);
    int32_t x;
    switch (to) {
    case 0x088EBF84u: return;
    case 0x088EBF8Cu: x = 0xC0; break;
    case 0x088EBFC8u: x = 0x110; break;
    case 0x088EC004u: x = (int32_t)(t << 4) - 0x90; break;
    case 0x088EC040u: return;
    default: rt_bad_jump(c, to, 0x088EBF7Cu); return;
    }
    *H(o + 18) = (uint16_t)x;
    *H(o + 22) = (uint16_t)(0xF0 - *B(0x08A3EA34u + (uint32_t)n * 11 + 3));
}

/* (_, row): up to four objects of kind 0x48 from the row's x list (4 halfwords from the table copied
   from 0x08A15FBC; 0xFFFF ends it): x + random 0 - 31, high (0x10) one time in four else 0x20, less
   random 0 - 31; flying left at -0x18 between camera x 4864 and 12288, else -0xA. */
SFA_FUNC(08852DB0, flock_by_row) {
    uint32_t row = ARG(1) & 0xFF;
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21];
    gaddr s22 = c->r[22], s23 = c->r[23], s30 = c->r[30];
    c->r[29] -= 0x90;
    gaddr sp = c->r[29], list = sp + row * 8;
    c->r[16] = row;
    c->r[4] = sp + 0x10; c->r[5] = 0x08A15FBCu; c->r[6] = 0x48; f_089F3430(c);
    *W(sp + 92) = list;
    *W(sp + 88) = CAM_0;
    c->r[22] = 1; c->r[21] = 3; c->r[20] = 1; c->r[19] = 0xF; c->r[30] = 0xFFF60000u; c->r[23] = 0xFFE80000u;
    for (uint32_t i = 0; i < 4; i = (i + 1) & 0xFFFF) {
        c->r[18] = i;
        uint32_t x = *H(list + i * 2 + 16);
        c->r[17] = x;
        if (x == 0xFFFF) break;
        c->r[4] = 0x48; c->r[5] = 0; f_08822F1C(c);
        gaddr o = c->r[2];
        c->r[16] = o;
        if (o == 0) break;
        f_08821368(c);
        *H(o + 18) = (uint16_t)x;
        *B(o + 3) = (c->r[2] & 3) == 0;
        int32_t v = S16(o + 18);
        f_08821368(c);
        *H(o + 18) = (uint16_t)(v + (int32_t)(c->r[2] & 0x1F));
        int32_t y = *B(o + 3) == 0 ? 0x20 : 0x10;
        *H(o + 22) = (uint16_t)y;
        v = S16(o + 22);
        f_08821368(c);
        *H(o + 22) = (uint16_t)(v - (int32_t)(c->r[2] & 0x1F));
        int32_t cx = S16(CAM_0 + 18) + S16(o + 18);
        c->r[17] = (uint32_t)cx;
        *W(o + 96) = (cx >= 4864 && cx < 12289) ? 0xFFE80000u : 0xFFF60000u;
        *B(o + 9) = 0; *B(o + 78) = 1; *B(o + 11) = 0; *B(o + 14) = 0;
        *H(o + 26) = 0; *H(o + 24) = 3; *H(o + 38) = 0;
        *W(o + 164) = 1;
        *H(o + 86) = 0xF;
        *W(o + 148) = 0;
    }
    c->r[29] += 0x90;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21;
    c->r[22] = s22; c->r[23] = s23; c->r[30] = s30;
}
