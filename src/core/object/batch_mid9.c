/* More steps whose callees became ready with 0.106.0 - 0.113.0 (0.114.0): the ending's pieces stepped
 * on, the intro's owner by stage, option panels by mode, camera lines started, the reward count, and
 * a fighter's damage scaled by its state. Offsets stay numbers until something shows what they are.
 * Arguments of native callees, and what the original leaves in a0 - a3 / t0 at a call through a table,
 * are set as the original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_089CEF38(Cpu *c);   /* speeds_times_256_a */
void f_089CD714(Cpu *c);   /* step_by_kind_or_119 */
void f_088B54FC(Cpu *c);   /* clear_5c_7f */
void f_088B4C74(Cpu *c);   /* particle_start_by_78 */
void f_08822E74(Cpu *c);   /* object_make_8 */
void f_08855D44(Cpu *c);   /* intro_make_owned */
void f_088BAC18(Cpu *c);   /* game_option_pulse */
void f_088585B0(Cpu *c);   /* count_reward */
void f_088215C4(Cpu *c);   /* object_draw */

/* ---------------------------------------------------------------- objects */

/* (object, entry): once the entry's +2 reaches +118, +4 moves on, its speeds scaled (0x089CEF38), its
   start step (+3) from 0x08AB6158 and the step by kind (0x089CD714). */
SFA_FUNC(089CD29C, start_at_118) {
    gaddr o = ARG(0), e = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = e; c->r[17] = o;
    if (!(S16(e + 2) < (int32_t)*B(o + 118))) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *B(o + 14) = 0;
        *H(o + 38) = 0;
        c->r[4] = o; c->r[5] = 0;
        f_089CEF38(c);
        gaddr fn = *W(0x08AB6158u + (uint32_t)*B(o + 3) * 4);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
        c->r[4] = o; c->r[5] = e;
        f_089CD714(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (particle): cleared, +5 moves on, set up (0x092E3E98 = 0x08A36F60), +73 its wait from 0x08A37548 by
   +78; started at once (0x088B4C74) when there is none. */
SFA_FUNC(088B4B84, particle_wait_by_78) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 0;
    f_088B54FC(c);
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *W(o + 148) = 1;
    *H(o + 86) = 0xA;
    *W(0x092E3E98u) = 0x08A36F60u;
    *B(o + 9) = 1;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 11) = 0;
    *H(o + 24) = 7;
    *H(o + 26) = 3;
    *H(o + 34) = 0;
    *B(o + 79) = 0;
    *B(o + 73) = (uint8_t)S8(0x08A37548u + (uint32_t)(int32_t)S8(o + 78));
    if (S8(o + 73) == 0) { c->r[4] = o; f_088B4C74(c); }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +4 moves on and it is set up as one of four pictures by +3 (0 - 1, 2, 3, 4). */
SFA_FUNC(08847CF8, setup_picture_by_kind) {
    (void)c;
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *H(o + 24) = 7;
    *B(o + 14) = 0;
    *B(o + 11) = 0;
    *B(o + 12) = 1;
    uint32_t k = *B(o + 3);
    if ((int32_t)k < 2 || k == 4) { *B(o + 13) = 0xD; *H(o + 86) = 0x12; *H(o + 84) = 0x14; }
    else if (k == 2) { *B(o + 13) = 0xF; *H(o + 86) = 0x21; *H(o + 84) = 0x58; }
    else if (k == 3) { *B(o + 13) = 0x11; *H(o + 86) = 0x1C; *H(o + 84) = 8; }
    *W(o + 148) = 3;
}

/* (): the intro's owner (kind 0, type 2, pool of 8) with its two parts from 0x08A16DF8 (or the pair after
   for GAME +114 = 6 / 7, or 0x10 with BATTLE +159 = 2); in a set match (BATTLE +16) only when BATTLE +5
   is set or GAME +114 is 6. */
SFA_FUNC(08855BD8, intro_owner_by_stage) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[4] = 0; c->r[5] = 2;
    f_08822E74(c);
    gaddr r = c->r[2];
    c->r[16] = r;
    if (r != 0) {
        *B(r) = 1;
        int32_t m = S8(GAME + 114);
        if (S8(BATTLE + 16) == 0 || S8(BATTLE + 5) != 0 || m == 6) {
            gaddr t = 0x08A16DF8u;
            if (m == 7 || m == 6 || (m == 0x10 && S8(BATTLE + 159) == 2)) t += 2;
            c->r[17] = t + 1;
            c->r[4] = r; c->r[5] = (uint32_t)S8(t) & 0xFF; c->r[6] = 0;
            f_08855D44(c);
            c->r[4] = r; c->r[5] = (uint32_t)S8(t + 1) & 0xFF; c->r[6] = 0;
            f_08855D44(c);
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): the option pulse, its place from 0x08A3CAC0 / CAC4 by its parent's (+124) +7 / +8, shown
   (+93) in modes 1 and 3 while GAME +4 is set and in mode 2 when the parent's +1 is 2. */
SFA_FUNC(088CF7A4, option_place_by_mode) {
    gaddr o = ARG(0), p = *W(o + 124), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = p;
    f_088BAC18(c);
    *H(o + 6) = *H(0x08A3CAC0u + (uint32_t)(int32_t)S8(p + 7) * 2);
    *H(o + 10) = *H(0x08A3CAC4u + (uint32_t)(int32_t)S8(p + 8) * 2);
    uint32_t m = *H(GAME + 6);
    if (m < 5) {
        if (m == 1 || m == 3) *B(o + 93) = *H(GAME + 4) != 0 ? 1 : 0;
        else if (m == 2) *B(o + 93) = S8(p + 1) == 2 ? 1 : 0;
        else *B(o + 93) = 0;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (n, k): camera table n (0x0951F950 + 152 n) restarted: +28 from 0x08AB7960 by 0x09358BE0 +134, +2
   counted, speeds cleared, and its place from entry k of 0x08AB77B0 / 7840 / 78D0 (y 16 up). */
SFA_FUNC(089E0178, camera_line_start) {
    (void)c;
    uint32_t n = ARG(0), k = ARG(1);
    int32_t s = S16(0x09358BE0u + 134);
    uint8_t b = *B(0x08AB7960u + (uint32_t)(s * 3) + n);
    gaddr e = 0x0951F950u + n * 152;
    *B(e + 28) = b;
    *B(e + 2) = (uint8_t)(*B(e + 2) + 1);
    *W(e + 16) = 0;
    *W(e + 20) = 0;
    gaddr t = n == 0 ? 0x08AB77B0u : n == 1 ? 0x08AB7840u : 0x08AB78D0u;
    gaddr q = t + k * 4;
    *H(e + 10) = (uint16_t)S16(q);
    *H(e + 14) = (uint16_t)(S16(q + 2) - 16);
}

/* (object, fighter): +139 counts down through the shapes of 0x08A17208 / 7218 (into +0x70 - +0x7A); at
   0 the reward count (0x088585B0) and +7 moves on with 60 more, unless the fighter's +769; with BATTLE +7
   it goes to step 3 instead. Drawn. */
SFA_FUNC(08857F8C, reward_count_shapes) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 139) != 0) {
        *B(o + 139) = (uint8_t)(S8(o + 139) - 1);
        gaddr q = 0x08A17218u + (uint32_t)(int32_t)S8(0x08A17208u + (uint32_t)(int32_t)S8(o + 139)) * 2;
        gaddr v = o + 0x70;
        *H(v) = (uint16_t)S16(q);
        *H(v + 2) = (uint16_t)S16(q + 2);
        *H(v + 8) = (uint16_t)S16(q + 4);
        *H(v + 10) = (uint16_t)S16(q + 6);
    }
    if (S8(BATTLE + 7) != 0) {
        *B(o + 7) = 3;
        *B(o + 127) = 0x15;
        *B(o + 119) = 0x15;
    } else if (S8(f + 769) == 0) {
        c->r[4] = o;
        f_088585B0(c);
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 139) = 0x3C;
    }
    c->r[4] = o;
    f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, hit, move, amount): the amount scaled by the move's (+21) row of 0x08A21E8C for the hit frame's
   +5 (1: x 1.25, else x 1.5), then by the fighter's +792 flags (0x4000 x 1.375, 0x40000 x 3, 0x20000
   x 2.5, 0x10000 x 2, 0x40 x 1.375). */
SFA_FUNC(08871D60, damage_scale) {
    gaddr a = ARG(0), b = ARG(1), m = ARG(2);
    uint32_t v = ARG(3);
    uint32_t idx = (uint32_t)*B(m + 21) * 4 + *B(*W(b + 296) + 5);
    int32_t t = S8(0x08A21E8Cu + idx);
    uint32_t fl = *W(a + 792);
    if (t != 0) v = t == 1 ? v + (v >> 2) : v + (v >> 1);
    if (fl & 0x4000) v = (v >> 2) + (v >> 3) + v;
    else if (fl & 0x40000) v = v + (v << 1);
    else if (fl & 0x20000) v = v + (v >> 1) + v;
    else if (fl & 0x10000) v = v + v;
    else if (fl & 0x40) v = (v >> 2) + (v >> 3) + v;
    RETURN(v);
}
