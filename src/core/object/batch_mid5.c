/* More middle-sized steps (0.110.0): the first object list's run, objects made from a description,
 * fighters' input written for the team, press counters for three directions, loading a file and
 * waiting for it, and screen objects' set-ups. Offsets stay numbers until something shows what they
 * are. Arguments of native callees, and what the original leaves in a0 - a3 / t0 at a call through a
 * table, are set as the original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_088054F0(Cpu *c);   /* game_task_wait_frames */
void f_088075F8(Cpu *c);   /* game_queue_load */
void f_08821330(Cpu *c);   /* object_routine_by_14 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_088B4498(Cpu *c);   /* follow_trail */
void f_088B4F70(Cpu *c);   /* move_96_wrap_108 */
void f_088378F8(Cpu *c);   /* count_280_or_routine_3 */
void f_08837950(Cpu *c);   /* count_311_then_260 */
void f_08881D9C(Cpu *c);   /* setup_kind_at */
void f_08869B70(Cpu *c);   /* fighter_press_count */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
static uint32_t screen_kind(void) { return *B(0x08ACCD54u + (uint32_t)(int32_t)S8(*W(0x09525DE8u) + 8)); }

/* ---------------------------------------------------------------- fighters */

/* (fighter, script): script command: with +371 or +386, read on only while +366 is 0x90; otherwise
   read on once +366 reaches the script's value; else the block is left. */
SFA_FUNC(08878B88, fighter_script_cmd_366) {
    gaddr f = ARG(0), p = ARG(1);
    uint32_t v = (uint32_t)(int32_t)S16(p) & 0xFFFF;
    int32_t cur = S16(f + 366);
    int read;
    if (S8(f + 371) != 0 || S8(f + 386) != 0) read = cur == 0x90;
    else read = !(cur < (int32_t)v);
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p + 2; c->r[6] = p + 2;
    if (read) f_0887A10C(c); else f_08879C8C(c);
    c->r[29] += 0x20;
}

/* (fighter, input): the input (+248) set by the battle state: state 0 itself; 1 - 2 itself and, on side
   bit 2, fighters 4 and 3; 3, unless it has one already, itself and (side bit 2 clear) fighters 2 and 1. */
SFA_FUNC(088728B0, fighter_team_input) {
    gaddr f = ARG(0);
    uint8_t v = (uint8_t)ARG(1);
    int32_t t = S16(BATTLE + 56);
    if (t == 0) { *B(f + 248) = v; return; }
    if (t == 1 || t == 2) {
        int32_t side = S8(f + 337);
        *B(f + 248) = v;
        if (side & 2) { *B(0x092E4C68u + 248) = v; *B(0x092E47D0u + 248) = v; }
    } else if (t == 3) {
        if (S8(f + 248) != 0) return;
        int32_t side = S8(f + 337);
        *B(f + 248) = v;
        if ((side & 2) == 0) { *B(0x092E4338u + 248) = v; *B(0x092E3EA0u + 248) = v; }
    }
}

/* (fighter, counters, table): three press counters (0x08869B70) with the table's mask (+0) and wait /
   need pairs at +10, +6, +2: 1 when any reaches its need, the last first. */
SFA_FUNC(08869AC8, fighter_press_three) {
    gaddr f = ARG(0), p = ARG(1), t = ARG(2), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x30;
    c->r[16] = t; c->r[17] = f; c->r[18] = p;
    uint32_t r = 1;
    c->r[4] = f; c->r[5] = p + 6; c->r[6] = *H(t); c->r[7] = *H(t + 10); c->r[8] = *H(t + 12);
    f_08869B70(c);
    if (c->r[2] == 0) {
        c->r[4] = f; c->r[5] = p + 4; c->r[6] = *H(t); c->r[7] = *H(t + 6); c->r[8] = *H(t + 8);
        f_08869B70(c);
        if (c->r[2] == 0) {
            c->r[16] = 0;
            c->r[4] = f; c->r[5] = p + 2; c->r[6] = *H(t); c->r[7] = *H(t + 2); c->r[8] = *H(t + 4);
            f_08869B70(c);
            r = c->r[2] != 0;
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
    RETURN(r);
}

/* ---------------------------------------------------------------- objects */

/* (object): at its parent's (+88) place, (0x1B, 0x50) on screens of kind 1, (-0xA0, 0xB0) otherwise;
   +4 moves on, picture 0 of +14's set, +86 = 0x17. */
SFA_FUNC(088BC6A4, setup_by_screen_kind) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *B(o + 12) = 0;
    *B(o + 14) = *B(p + 14);
    *B(o + 13) = 0;
    *H(o + 24) = 7;
    int one = screen_kind() == 1;
    *H(o + 18) = (uint16_t)(S16(p + 18) + (one ? 0x1B : -0xA0));
    *H(o + 22) = (uint16_t)(S16(p + 22) + (one ? 0x50 : 0xB0));
    *H(o + 86) = 0x17;
    *H(o + 84) = 0;
    *W(o + 148) = 3;
}

/* (n): a value for n's low four bits (the table at 0x08A01DE8): 0 for n = 0. */
SFA_FUNC(089E3364, value_by_low_4) {
    static const uint32_t V[12] = { 0xFFFF0000u, 0xFFFFFF01u, 0xFFFFFF40u, 0xFFFFFF04u, 0xFFFFFF0Bu,
                                    0xFFFFFF40u, 0xFFFFFF41u, 0xFFFFFF05u, 0xFFFFFF00u, 0xFFFF0000u,
                                    0xFFFF0000u, 0xFFFF0000u };
    uint32_t n = ARG(0), k = n & 0xF;
    c->r[4] = k;
    if (n == 0) { RETURN(0); return; }
    if (k >= 12) { RETURN(0xFFFF0000u); return; }
    c->r[4] = k << 2;
    RETURN(V[k]);
}

/* (): record 0x08E76084 of the 216-byte list at 0x08E76088 cleared (+202 - +204, +208), the file
   0x08A03150 queued, frames waited until 0x08E7606C is set, and 0x08E75C4C = -1. */
SFA_FUNC(08809934, load_and_wait) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    gaddr e = (uint32_t)*B(0x08E76084u) * 216 + 0x08E76088u;
    *B(e + 202) = 0;
    *B(e + 203) = 0;
    *H(e + 208) = 0;
    *B(e + 204) = 0;
    c->r[4] = *W(0x08A03150u); c->r[5] = 0; c->r[6] = 0;
    f_088075F8(c);
    c->r[16] = 0x08E76068u;
    while (*B(0x08E7606Cu) == 0) { c->r[4] = 1; f_088054F0(c); }
    *H(0x08E75C4Cu) = 0xFFFF;
    c->r[4] = 0xFFFFFFFFu; c->r[5] = 0x08E75C48u;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): routine 3 (+4) once it is more than 0x40 left or 0x1C0 right of its routine entry's x
   (with +14 = 2, less the depth: (+18 - +10) * +38 / 128). */
SFA_FUNC(08821384, off_screen_routine_3) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_08821330(c);
    gaddr t = c->r[2];
    int32_t ex = S16(t + 18);
    int32_t d = (int16_t)(S16(o + 18) - ex + 0x40);
    if (*B(o + 14) == 2) {
        int32_t z = S16(o + 38);
        if (z != 0) {
            int64_t m = (int64_t)(int32_t)(ex - S16(t + 10)) * z;
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            int32_t q = (int32_t)(((uint32_t)m + ((uint32_t)((int32_t)(uint32_t)m >> 7) >> 25))) >> 7;
            d = (int16_t)(d - q);
        }
    }
    if (d >= 512 || d < 0) *B(o + 4) = 3;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +84 and its place by 0x09525880 from 0x08A132E8 / 0x08A13312, unless +76: then 0x200 across,
   or picture 8 at (0xC0, 0x80) while 0x09523C69 is set. */
SFA_FUNC(08846BBC, place_by_5880) {
    gaddr o = ARG(0);
    if (S8(o + 76) == 0) {
        uint32_t i = (uint32_t)(int32_t)S8(0x09525880u);
        *H(o + 84) = (uint16_t)S16(0x08A132E8u + i * 2);
        *H(o + 18) = (uint16_t)S16(0x08A13312u + i * 4);
        *H(o + 22) = (uint16_t)S16(0x08A13312u + i * 4 + 2);
    } else if (S8(0x09523C69u) == 0) {
        *H(o + 18) = 0x200;
    } else {
        *H(o + 84) = 8;
        *H(o + 18) = 0xC0;
        *H(o + 22) = 0x80;
    }
}

/* (object, description): a kind-5 object (step from the description's +0) owned by it, its kind and
   +78 from +1 / +2, at its place + (+4 mirrored by +11, +6), with its draw set. */
SFA_FUNC(0887E7AC, make_from_description) {
    gaddr o = ARG(0), d = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = d; c->r[17] = o;
    c->r[4] = 5; c->r[5] = *B(d);
    f_08822EAC(c);
    gaddr r = c->r[2];
    if (r != 0) {
        *W(r + 88) = o;
        *B(r + 9) = *B(d + 1);
        *B(r + 78) = *B(d + 2);
        *B(r + 14) = *B(o + 14);
        *B(r + 12) = 0;
        *H(r + 24) = *H(o + 24);
        int32_t dx = S16(d + 4);
        *B(r + 11) = *B(o + 11);
        if (*B(o + 11) == 0) dx = (int16_t)(-dx);
        *H(r + 18) = (uint16_t)(S16(o + 18) + dx);
        *H(r + 22) = (uint16_t)(S16(o + 22) + S16(d + 6));
        c->r[4] = (uint32_t)(S16(o + 22) + S16(d + 6)); c->r[5] = (uint32_t)(int32_t)S16(d + 6);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +32 from GAME +16's low byte, +4 moves on and it is set up at (0xC0, 0x40) with the
   animation 0x08A2FFEC. */
SFA_FUNC(088B14D8, setup_c0_40_by_16) {
    gaddr o = ARG(0);
    *H(o + 32) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 7;
    *H(o + 86) = 0x13;
    *W(o + 164) = 1;
    *H(o + 38) = 0;
    *B(o + 15) = 0;
    *B(o + 11) = 0;
    *W(o + 100) = 0;
    *B(o + 9) = 1;
    *B(o + 14) = 3;
    *H(o + 24) = 3;
    *H(o + 26) = 3;
    *B(o + 12) = 0;
    *B(o + 13) = 0;
    *H(o + 18) = 0xC0;
    *H(o + 22) = 0x40;
    c->r[29] -= 0x20;
    c->r[6] = 0x40;
    set_anim(c, o, *W(0x08A2FFECu));
    c->r[29] += 0x20;
}

/* (object): +4 moves on once GAME +22 has its bit (+3); otherwise it follows the trail and, at GAME +20 =
   4, sets its bit in GAME +10's high byte. */
SFA_FUNC(088B4230, follow_until_22_bit) {
    gaddr o = ARG(0), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 1; c->r[18] = GAME;
    if ((S16(GAME + 22) & 0xFF) & (1u << (*B(o + 3) & 31))) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    } else {
        c->r[4] = o;
        f_088B4498(c);
        if (S16(GAME + 20) == 4) {
            uint32_t h = *H(GAME + 10), bit = (1u << (*B(o + 3) & 31)) << 8;
            c->r[16] = bit;
            if ((h & 0xFF00 & bit) == 0) *H(GAME + 10) = (uint16_t)(h | bit);
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (object): moves (0x088B4F70) until x reaches +104 from the side +96 heads; then held there, +5 moves on
   and GAME +10 gets bit +78. */
SFA_FUNC(088B4DD4, move_to_104) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088B4F70(c);
    int32_t d = (int16_t)(S16(o + 18) - S16(o + 104));
    if (d == 0 || (int16_t)((uint32_t)d ^ (uint32_t)(int32_t)S16(o + 96)) >= 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 18) = (uint16_t)S16(o + 104);
        uint32_t bit = 1u << ((uint32_t)(int32_t)(int16_t)S8(o + 78) & 31);
        *H(GAME + 10) = (uint16_t)(*H(GAME + 10) | bit);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the first object list's twelve 0x168-byte objects from 0x09511090 (count GAME +66): each in use
   counts down (0x088378F8) and, while not done, ticks 0x08837950 and runs its step (+2) from 0x08A10130. */
SFA_FUNC(08837854, run_list_09511090) {
    gaddr s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = 0x08A10130u; c->r[17] = GAME;
    *B(GAME + 66) = 12;
    gaddr o = 0x09511090u;
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
    c->r[4] = (uint32_t)(int32_t)n;
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (): the record 0x0950FF60's flags reset, and 0x09525D98 - 9A from 0x08A058A0 + 16 (GAME +53 bit 0) or + 36. */
SFA_FUNC(0884A0B4, record_ff60_reset) {
    (void)c;
    gaddr r = 0x0950FF60u;
    *B(r + 5) = 1; *B(r + 4) = 1;
    *B(r + 7) = 0; *B(r + 6) = 0; *B(r + 8) = 0; *B(r + 9) = 0;
    *B(r + 48) = 1;
    *B(r + 16) = 0; *B(r + 15) = 0; *B(r + 14) = 0; *B(r + 10) = 0; *B(r + 20) = 0;
    gaddr t = 0x08A058A0u + ((S8(GAME + 53) & 1) ? 16 : 36);
    *B(0x09525D98u) = (uint8_t)S8(t);
    *B(0x09525D99u) = (uint8_t)S8(t + 1);
    *B(0x09525D9Au) = (uint8_t)S8(t + 2);
}

/* (object): +4 moves on; set up as kind 0 at (0xC0, y), +82 = 32, with a list-3 object (kind 1, type +10
   + 2) at its place, and the animation +10 of 0x08A29A04. */
SFA_FUNC(088814A0, setup_with_list_3) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t y = S16(o + 22);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; c->r[5] = 0; c->r[6] = 0xC0; c->r[7] = (uint32_t)y;
    f_08881D9C(c);
    *H(o + 82) = 0x20;
    *W(o + 88) = 0;
    c->r[4] = 1; c->r[5] = (uint32_t)(*B(o + 10) + 2) & 0xFF;
    f_08823338(c);
    gaddr r = c->r[2];
    if (r != 0) {
        *W(r + 4) = *W(o + 16);
        *W(r + 108) = 0;
        *W(r + 8) = *W(o + 20);
        *W(o + 88) = r;
    }
    set_anim(c, o, *W(0x08A299FCu + (uint32_t)*B(o + 10) * 4 + 8));
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): shown, +4 moves on, picture 0x17 beside its parent (+88) (0x70, -10), +84 from 0x08ACCB3A by
   the screen record's entry +10 (20 bytes each). */
SFA_FUNC(088BD99C, setup_beside_parent_17) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = 1;
    *B(o + 13) = 0x17;
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0x70);
    *H(o + 22) = (uint16_t)(S16(p + 22) - 10);
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    uint32_t k = *B(p + 10);
    int32_t v = S8(*W(0x09525DE8u) + k * 20 + 8);
    *H(o + 84) = (uint16_t)(*B(0x08ACCB3Au + (uint32_t)v) + 0x34);
    *W(o + 148) = 3;
}

/* (object): +4 moves on; a list-3 object (kind 13, type +11) given its place and size (+112, +114); with
   +11 it points back (+124), without it a second (type 4) covers the screen's bottom and +4 = 2. */
SFA_FUNC(089D9C04, make_13_panel) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = 0xD; c->r[5] = *B(o + 11);
    f_08823338(c);
    gaddr r = c->r[2];
    *H(r + 6) = (uint16_t)S16(o + 18);
    *H(r + 10) = (uint16_t)S16(o + 22);
    *H(r + 14) = (uint16_t)S16(o + 112);
    *H(r + 26) = (uint16_t)S16(o + 114);
    if (*B(o + 11) != 0) {
        *W(r + 124) = o;
    } else {
        c->r[4] = 0xD; c->r[5] = 4;
        f_08823338(c);
        gaddr q = c->r[2];
        *H(q + 6) = 0;
        *H(q + 10) = 0xF0;
        *H(q + 12) = 0;
        *H(q + 24) = 0;
        *H(q + 14) = 0x180;
        *H(q + 26) = 1;
        *B(o + 4) = 2;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): its fields cleared for a new use, +8 = 2, and the free count 0x095176F0 (or F1 with +289)
   up by one. */
SFA_FUNC(088230F4, object_clear_for_use) {
    (void)c;
    gaddr o = ARG(0);
    for (uint32_t i = 0; i < 16; i += 4) *W(o + i) = 0;
    *B(o + 136) = 0; *B(o + 123) = 0; *B(o + 72) = 0;
    *H(o + 38) = 0;
    for (uint32_t i = 40; i <= 56; i += 4) *W(o + i) = 0;
    static const uint16_t BYTES[] = { 255, 271, 138, 170, 171, 260, 311, 312, 80, 280 };
    for (unsigned i = 0; i < sizeof BYTES / sizeof BYTES[0]; i++) *B(o + BYTES[i]) = 0;
    *B(o + 8) = 2;
    gaddr n = S8(o + 289) != 0 ? 0x095176F1u : 0x095176F0u;
    *B(n) = (uint8_t)(S8(n) + 1);
}

/* (object): x by +96 (x 256), +4 moving on past 0x1C8; and with +75 set, the next animation (+78 + 1,
   six back with +75's bit 7) from 0x08A2FA28. Animated. */
SFA_FUNC(088B0D08, move_and_next_anim) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *W(o + 16) += *W(o + 96) << 8;
    if ((int16_t)(S16(o + 18) + 0x48) >= 529) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    int32_t a = S8(o + 75);
    if (a != 0) {
        if (a & 0x80) *B(o + 78) = (uint8_t)(S8(o + 78) - 6);
        *B(o + 78) = (uint8_t)(S8(o + 78) + 1);
        set_anim(c, o, *W(0x08A2FA28u + (uint32_t)(int32_t)S8(o + 78) * 4));
    }
    c->r[4] = o;
    f_088219B0(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
