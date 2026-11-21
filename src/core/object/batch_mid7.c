/* More steps whose callees became ready with 0.106.0 - 0.111.0 (0.112.0): the list of twelve objects at
 * 0x0950FFB0 run before the first list, the CPU's first script picked at random, a fighter's pad byte
 * for its team, particles and panels set up, and objects stepped by table. Offsets stay numbers until
 * something shows what they are. Arguments of native callees, and what the original leaves in a0 - a3 /
 * t0 at a call through a table, are set as the original sets them; s0 - s4 as the original sets them
 * around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08846BBC(Cpu *c);   /* place_by_5880 */
void f_089CF0A4(Cpu *c);   /* stop_at_114 */
void f_08821AA8(Cpu *c);   /* object_next_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_08821384(Cpu *c);   /* off_screen_routine_3 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_088B54FC(Cpu *c);   /* clear_5c_7f */
void f_088B53F4(Cpu *c);   /* place_from_76c2 */
void f_088B544C(Cpu *c);   /* anim_by_3_and_2e08 */
void f_088728B0(Cpu *c);   /* fighter_team_input */
void f_088378F8(Cpu *c);   /* count_280_or_routine_3 */
void f_08837950(Cpu *c);   /* count_311_then_260 */
void f_08837854(Cpu *c);   /* run_list_09511090 */
void f_08821368(Cpu *c);   /* game_random */
void f_0887B528(Cpu *c);   /* fighter_cpu_state_from_word */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088EAC84(Cpu *c);   /* set_from_16_bytes */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
static void colours(gaddr o, uint8_t a, uint8_t b, uint8_t d) {
    for (int k = 36; k <= 39; k++) *B(o + k) = a;
    for (int k = 42; k <= 45; k++) *B(o + k) = b;
    for (int k = 48; k <= 51; k++) *B(o + k) = d;
}

/* ---------------------------------------------------------------- objects */

/* (object): shown, +4 moves on, set up as picture 0x11 and placed by 0x09525880 (0x08846BBC). */
SFA_FUNC(08846B58, setup_11_place_by_5880) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *B(o + 14) = 0;
    *B(o + 9) = 0;
    *B(o + 12) = 1;
    *B(o + 13) = 0x11;
    *B(o + 11) = 0;
    *H(o + 24) = 7;
    *H(o + 86) = 0x1C;
    *W(o + 148) = 3;
    *W(o + 164) = 0;
    c->r[29] -= 0x20;
    c->r[5] = 3; c->r[6] = 0x1C;
    f_08846BBC(c);
    c->r[29] += 0x20;
}

/* (object): y by +100 until it stops at +114 (0x089CF0A4); then +5 moves on, +100 from +96 and +88 its
   negative. Its frame steps on. */
SFA_FUNC(089D32D4, fall_to_114_then_bounce) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t v = *W(o + 100);
    *W(o + 20) += v;
    c->r[5] = *W(o + 20); c->r[6] = v;
    f_089CF0A4(c);
    if (S8(o + 78) == 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        uint32_t s = *W(o + 96);
        *W(o + 100) = s;
        *W(o + 88) = 0u - s;
    }
    c->r[4] = o;
    f_08821AA8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): unless the fight is paused (BATTLE +37 / +78), moved by +96 / +100, its next frame, routine
   3 off screen; drawn if on screen. */
SFA_FUNC(088764A8, move_frame_unless_paused) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        *W(o + 16) += *W(o + 96);
        *W(o + 20) += *W(o + 100);
        c->r[4] = o; f_088768F0(c);
        c->r[4] = o; f_08821384(c);
    }
    c->r[4] = o;
    f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (particle): cleared (0x088B54FC), +4 moves on, set up (kind +9), placed (0x088B53F4) and animated
   (0x088B544C). */
static void particle_setup(Cpu *c, uint8_t kind, int clear_15) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0xA;
    *B(o + 9) = kind;
    *H(o + 38) = 0;
    if (clear_15) *B(o + 15) = 0;
    *B(o + 14) = 0; *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 11) = 0;
    *H(o + 24) = 7;
}
static void particle_start(Cpu *c, uint8_t kind, int clear_15, int tail) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 0;
    f_088B54FC(c);
    particle_setup(c, kind, clear_15);
    if (tail == 2 && ((S16(GAME + 22) & 0xFF) & 1) == 0) *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 1);
    c->r[4] = o; f_088B53F4(c);
    c->r[4] = o; f_088B544C(c);
    if (tail == 1) { *H(o + 82) = 0; *H(o + 34) = 0; *H(o + 32) = 0; }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(088B4618, particle_start_a) { particle_start(c, 3, 1, 0); }
SFA_FUNC(088B47B8, particle_start_b) { particle_start(c, 3, 1, 1); }
SFA_FUNC(088B3E84, particle_start_c) { particle_start(c, 1, 0, 2); }

/* (object, fighter, pad record): the fighter's pad byte for its team (0x088728B0): the record's +22 (8
   with +80; 7 becomes +194 + 10, or 9 with ISM bit 7, which also caps it at 9), +0x20 with 0x09358458,
   +0x40 while the fighters of its team stand (+112 = 0x90); BATTLE +68 from 0x08A21F4E. */
SFA_FUNC(088726EC, fighter_pad_byte) {
    gaddr a = ARG(0), f = ARG(1), r = ARG(2), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = BATTLE;
    uint32_t k;
    if (S8(a + 80) != 0) k = 8;
    else {
        k = *B(r + 22);
        if (S8(f + 386) & 0x80) { if (k == 7 || !((int32_t)k < 9)) k = 9; }
        else if (k == 7) k = (uint32_t)(S8(a + 194) + 10) & 0xFF;
    }
    if (*B(0x09358410u + 72) != 0) k = (k | 0x20) & 0xFF;
    int32_t t = S16(BATTLE + 56);
    int stand = 0;
    if (t == 0) stand = S16(f + 112) == 0x90;
    else if (t == 1 || t == 2) {
        if (S8(f + 337) & 2) stand = S16(P3 + 112) == 0x90 && S16(P4 + 112) == 0x90;
        else stand = S16(f + 112) == 0x90;
    } else if (t == 3) {
        if (S8(f + 337) & 2) stand = S16(f + 112) == 0x90;
        else stand = S16(P1 + 112) == 0x90 && S16(P2 + 112) == 0x90;
    }
    if (stand) k = (k | 0x40) & 0xFF;
    c->r[17] = k;
    c->r[4] = f; c->r[5] = k;
    f_088728B0(c);
    *B(BATTLE + 68) = (uint8_t)(*B(0x08A21F4Eu + (k & 0xF)) >> 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): the twelve 0x168-byte objects at 0x0950FFB0 run like the first list (count GAME +66, steps
   0x08A10130), then the first list itself (0x08837854). */
SFA_FUNC(088377A8, run_lists_0950ffb0) {
    gaddr s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = 0x08A10130u; c->r[17] = GAME;
    *B(GAME + 66) = 12;
    gaddr o = 0x0950FFB0u;
    int8_t n;
    do {
        c->r[18] = o;
        if (*B(o) != 0) {
            c->r[4] = o;
            f_088378F8(c);
            if (c->r[2] == 0) {
                c->r[4] = o;
                f_08837950(c);
                gaddr fn = *W(0x08A10130u + (uint32_t)*B(o + 2) * 4);
                c->r[4] = o; c->r[5] = fn;
                rt_call_indirect(c, fn);
            }
        }
        n = (int8_t)(S8(GAME + 66) - 1);
        *B(GAME + 66) = (uint8_t)n;
        o += 0x168;
    } while (n != 0);
    c->r[18] = o;
    c->r[4] = (uint32_t)(int32_t)n;
    f_08837854(c);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (object): the picture for its +3 and 0x08B02848 from 0x08A2F314's pairs (into 0x08A2F168, or 0x08A2F210
   with +26 = 1 when the pair's first is set). */
SFA_FUNC(088B0AA8, anim_by_3_and_2848) {
    gaddr o = ARG(0);
    int32_t i = (int16_t)((int16_t)((int32_t)(int16_t)*B(o + 3) << 2) + *B(0x08B02848u) * 2);
    gaddr e = 0x08A2F314u + (uint32_t)i * 4;
    int32_t a = (int16_t)*W(e), b = (int16_t)*W(e + 4);
    gaddr t = a != 0 ? 0x08A2F210u : 0x08A2F168u;
    *H(o + 26) = a != 0 ? 1 : 3;
    c->r[29] -= 0x20;
    c->r[6] = t;
    set_anim(c, o, *W((uint32_t)b * 4 + t));
    c->r[29] += 0x20;
}

/* (object): +4 moves on; set up at x 0xF0, y and +104 from 0x08A2FBE0 by +78, animation 0x08A2FB7C. */
SFA_FUNC(088B0DE4, setup_f0_by_78) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0xE;
    *H(o + 86) = 0x1B;
    *W(o + 164) = 0;
    *W(o + 156) = 0x08A2FA88u;
    *B(o + 12) = 1;
    *W(o + 96) = 0;
    *B(o + 13) = 0;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *H(o + 24) = 3;
    *H(o + 26) = 1;
    *H(o + 18) = 0xF0;
    gaddr e = 0x08A2FBE0u + (uint32_t)(int32_t)S8(o + 78) * 4;
    *H(o + 22) = (uint16_t)S16(e);
    *W(o + 104) = (uint32_t)(int32_t)S16(e + 2);
    c->r[29] -= 0x20;
    c->r[6] = 0x08A30000u;
    set_anim(c, o, *W(0x08A2FB7Cu));
    c->r[29] += 0x20;
}

/* (panel): +94 from GAME +16's low byte, +128 moves on; a 0x80 x 0xF0 panel (0x50, 0x50, 0x60) with x from
   0x08A352B8 by GAME +22's high byte. */
SFA_FUNC(088B2544, panel_80_by_22) {
    (void)c;
    gaddr o = ARG(0);
    *H(o + 94) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *H(o + 98) = 0;
    *B(o + 55) = 7;
    *B(o + 56) = 1;
    *B(o + 1) = 1;
    *B(o + 3) = 0;
    *W(o + 4) = *W(0x08A352B8u + ((uint32_t)(S16(GAME + 22) & 0xFF00) >> 8) * 4);
    *W(o + 8) = 0xF00000;
    colours(o, 0x50, 0x50, 0x60);
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x80;
    *H(o + 26) = 0xF0;
}

/* (particle): +73 counts down; at 0 +5 moves on and its motion (+0x5C - +0x66) comes from 0x08A373C0 by
   +3 and +38, +104 / +105 set while its speeds are. */
SFA_FUNC(088B4174, particle_motion) {
    (void)c;
    gaddr o = ARG(0);
    int8_t n = (int8_t)(S8(o + 73) - 1);
    *B(o + 73) = (uint8_t)n;
    if (n != 0) return;
    gaddr v = o + 0x60;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(v) = 0;
    *H(v + 2) = 0;
    gaddr e = 0x08A373C0u + ((uint32_t)*B(o + 3) * 16 + (uint32_t)(int32_t)S16(o + 38)) * 4;
    *H(o + 0x5C) = (uint16_t)*W(e);
    *H(o + 0x5E) = (uint16_t)*W(e + 4);
    *H(v + 4) = (uint16_t)*W(e + 8);
    *H(v + 6) = (uint16_t)*W(e + 12);
    *B(v + 8) = S16(v + 4) != 0 ? 0xFF : 0;
    *B(v + 9) = S16(v + 6) != 0 ? 0xFF : 0;
}

/* (panel): set up from 0x08A3F440's 16 bytes for +92 (0x088EAC84), placed from 0x08A3F4A0, grey (0x80). */
SFA_FUNC(088EAD20, panel_by_92) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 54) = 0x12;
    *B(o + 88) = 1;
    *B(o + 55) = 7;
    c->r[4] = o; c->r[5] = 0x08A3F440u + (uint32_t)*B(o + 92) * 16; c->r[6] = 7;
    f_088EAC84(c);
    gaddr e = 0x08A3F4A0u + (uint32_t)*B(o + 92) * 4;
    *H(o + 6) = *H(e);
    *H(o + 10) = *H(e + 2);
    *B(o + 56) = 2;
    *B(o + 136) = 0xC0;
    colours(o, 0x80, 0x80, 0x80);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): y from 0x08A26B6A by GAME +10; its green (+42 - +45) pulses between 8 and 0x80 by 4, the
   direction in +131. */
SFA_FUNC(0888AE14, pulse_green) {
    (void)c;
    gaddr o = ARG(0);
    *H(o + 10) = (uint16_t)S16(0x08A26B6Au + (uint32_t)*H(GAME + 10) * 2);
    uint32_t d = *B(o + 131), v = *B(o + 42);
    if (d != 0) {
        v -= 4;
        if (!((int32_t)v < 8)) *B(o + 42) = (uint8_t)v;
        else { *B(o + 131) = (uint8_t)(d ^ 1); *B(o + 42) = 8; }
    } else {
        v += 4;
        if ((int32_t)v < 129) *B(o + 42) = (uint8_t)v;
        else { *B(o + 131) = (uint8_t)(d ^ 1); *B(o + 42) = 0x80; }
    }
    uint8_t g = *B(o + 42);
    *B(o + 45) = g; *B(o + 44) = g; *B(o + 43) = g;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): the CPU's first script: +480 / +481 = 1, a random entry of 0x08A2277C (by 0x08A2275C) into
   +488 - +494, the state from +490 and +496 / +497 from +492; command 2 keeps +494 in +506. */
SFA_FUNC(0887BC44, fighter_cpu_random_start) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 481) = 1;
    *B(f + 480) = 1;
    *B(f + 482) = 2;
    *B(f + 483) = 0;
    f_08821368(c);
    gaddr e = 0x08A2277Cu + (uint32_t)*B(0x08A2275Cu + (c->r[2] & 0x1F)) * 8;
    *H(f + 488) = (uint16_t)S16(e);
    *H(f + 490) = (uint16_t)S16(e + 2);
    *H(f + 492) = (uint16_t)S16(e + 4);
    *H(f + 494) = (uint16_t)S16(e + 6);
    c->r[4] = f; c->r[5] = (uint32_t)S16(f + 490) & 0xFFFF; c->r[6] = (uint32_t)(int32_t)S16(f + 490);
    f_0887B528(c);
    int32_t a = S8(f + 493), b = S8(f + 492);
    *B(f + 497) = (uint8_t)a;
    *B(f + 496) = (uint8_t)b;
    if ((b >> 1) == 2) *H(f + 506) = (uint16_t)S16(f + 494);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): its step (+7) from one of four tables by +192 (0x08A5ACDC / ACF0 without, 0x08A5ACFC / AD04
   with) and +193, +696 = (+192 set). */
SFA_FUNC(0890EC3C, fighter_step_by_192_193) {
    gaddr f = ARG(0);
    int32_t a = S8(f + 192), b = S8(f + 193);
    uint32_t k = *B(f + 7);
    gaddr t;
    if (a == 0) { *B(f + 696) = 0; t = b != 0 ? 0x08A5ACF0u : 0x08A5ACDCu; c->r[7] = (uint32_t)a; }
    else { *B(f + 696) = 1; t = b != 0 ? 0x08A5AD04u : 0x08A5ACFCu; c->r[7] = 1; }
    c->r[29] -= 0x20;
    gaddr fn = *W(t + k * 4);
    c->r[5] = fn; c->r[6] = t;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}
