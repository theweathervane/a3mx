/* More steps whose callees became ready with 0.106.0 - 0.112.0 (0.113.0): particles and pieces set up
 * from tables, counters of a screen's pieces, timed lines of a record, a fighter's input recorded for
 * its team, and an effect that follows a fighter's frame. Offsets stay numbers until something shows
 * what they are. Arguments of native callees, and what the original leaves in a0 - a3 / t0 at a call
 * through a table, are set as the original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u
#define P3      0x092E47D0u
#define P4      0x092E4C68u

#include "../guest_mem.h"

void f_088B0A50(Cpu *c);   /* place_from_f2cc */
void f_088B0AA8(Cpu *c);   /* anim_by_3_and_2848 */
void f_088764A8(Cpu *c);   /* move_frame_unless_paused */
void f_0892EE70(Cpu *c);   /* frame_6_then_next */
void f_088B4EBC(Cpu *c);   /* anim_by_108_over_39 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_089DE978(Cpu *c);   /* small_object_line_n */
void f_088636D8(Cpu *c);   /* fighter_record_input */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }

/* ---------------------------------------------------------------- objects */

/* (piece): +4 moves on, set up (kind +9), placed (0x088B0A50) and given its picture (0x088B0AA8). */
static void piece_start(Cpu *c, int kind_from_78, int clear_15, int tail) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0xC;
    *W(o + 164) = 0;
    *B(o + 9) = kind_from_78 ? (uint8_t)S8(o + 78) : 3;
    *H(o + 38) = 0;
    if (clear_15) *B(o + 15) = 0;
    *B(o + 14) = 0; *B(o + 13) = 0; *B(o + 12) = 0; *B(o + 11) = 0;
    *H(o + 24) = 5;
    c->r[4] = o; f_088B0A50(c);
    c->r[4] = o; f_088B0AA8(c);
    if (tail) { *H(o + 82) = 0; *H(o + 34) = 0; *H(o + 32) = 0; }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(088B03C0, piece_start_by_78) { piece_start(c, 1, 0, 0); }
SFA_FUNC(088B0590, piece_start_3)     { piece_start(c, 0, 1, 0); }
SFA_FUNC(088B0840, piece_start_3_b)   { piece_start(c, 0, 1, 1); }

/* (object): moved with its frames (0x088764A8) until +139 runs out while the fight runs; then step 2 and
   frame 6 (0x0892EE70). */
SFA_FUNC(0892ED04, count_139_then_frame_6) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int restart = 0;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        int8_t n = (int8_t)(S8(o + 139) - 1);
        *B(o + 139) = (uint8_t)n;
        c->r[5] = 2;
        restart = n == 0;
    }
    if (restart) {
        *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        f_0892EE70(c);
    } else {
        f_088764A8(c);
    }
    c->r[29] += 0x20;
}

/* (object, entry): kinds 0x17, 0x23 and 0x2F (+3) always run their step from 0x08AB6234 with (object,
   entry); the others only while the entry's +2 is under +119, else step 2. */
SFA_FUNC(089CD714, step_by_kind_or_119) {
    gaddr o = ARG(0), e = ARG(1);
    uint32_t k = *B(o + 3);
    int32_t lim = (int16_t)*B(o + 119);
    c->r[29] -= 0x20;
    if (k == 0x17 || k == 0x23 || k == 0x2F || S16(e + 2) < lim) {
        gaddr fn = *W(0x08AB6234u + k * 4);
        c->r[4] = o; c->r[5] = e; c->r[6] = fn; c->r[7] = e;
        rt_call_indirect(c, fn);
    } else {
        *B(o + 4) = 2;
        c->r[4] = 2; c->r[7] = e;
    }
    c->r[29] += 0x20;
}

/* (particle): +4 moves on; its start from 0x08A37554's 16 bytes by +78 (place +32 / +34, speeds +0x60 -
   +0x68, +82), x 0 and +108's top from +34; the picture by +108 (0x088B4EBC). */
SFA_FUNC(088B4C74, particle_start_by_78) {
    gaddr o = ARG(0), v = o + 0x60;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 5) = 0;
    *H(o + 16) = 0;
    *W(o + 108) &= 0xFFFF0000u;
    gaddr e = 0x08A37554u + (uint32_t)(int32_t)(int16_t)(S8(o + 78) << 3) * 2;
    *H(o + 32) = (uint16_t)S16(e);
    *H(o + 34) = (uint16_t)S16(e + 2);
    *H(v) = (uint16_t)S16(e + 4);
    *H(v + 4) = (uint16_t)S16(e + 6);
    *H(v + 2) = (uint16_t)S16(e + 8);
    *H(v + 6) = (uint16_t)S16(e + 10);
    *H(v + 8) = (uint16_t)S16(e + 12);
    uint32_t w = *W(o + 108);
    *H(o + 82) = (uint16_t)S16(e + 14);
    *H(o + 18) = (uint16_t)S16(o + 32);
    *W(o + 108) = w & 0xFFFF;
    *W(o + 108) = (w & 0xFFFF) | ((uint32_t)(int32_t)S16(o + 34) << 16);
    c->r[29] -= 0x20;
    f_088B4EBC(c);
    c->r[29] += 0x20;
}

/* (object): +4 moves on; set up by +10 from 0x08A2FBFC (place and +104, six bytes each) with the
   animation +10 of 0x08A2FB80. */
SFA_FUNC(088B0F28, setup_by_10) {
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
    *B(o + 15) = 0;
    uint32_t k = *B(o + 10);
    *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *H(o + 24) = 3;
    *H(o + 26) = 1;
    gaddr e = 0x08A2FBFCu + k * 6;
    *H(o + 18) = *H(e);
    *H(o + 22) = *H(e + 2);
    *W(o + 104) = *H(e + 4);
    c->r[29] -= 0x20;
    c->r[6] = k * 4;
    set_anim(c, o, *W(0x08A2FB78u + k * 4 + 8));
    c->r[29] += 0x20;
}

/* (object): +4 moves on; a copy of its parent (+88) 0xA0 right and 0x14 down, picture 0xD; +73 / +78 from
   the screen record's +8 / +9, +10 = (+14 mod 3) - 1, the animation +73 of 0x08A3ADA8. */
SFA_FUNC(088BC944, copy_parent_by_record) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *B(o + 12) = 0xFF;
    *B(o + 13) = 0xD;
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0xA0);
    *H(o + 22) = (uint16_t)(S16(p + 22) + 0x14);
    *H(o + 86) = 0x17;
    *W(o + 148) = 0;
    gaddr s = *W(0x09525DE8u);
    *B(o + 73) = (uint8_t)S8(s + 8);
    *B(o + 78) = (uint8_t)S8(*W(0x09525DE8u) + 9);
    DIV(c, (uint32_t)(int32_t)S8(*W(0x09525DE8u) + 14), 3);
    *B(o + 10) = (uint8_t)(c->hi - 1);
    c->r[29] -= 0x20;
    c->r[6] = c->hi - 1; c->r[7] = 3;
    set_anim(c, o, *W(0x08A3ADA8u + (uint32_t)(int32_t)S8(o + 73) * 4));
    c->r[29] += 0x20;
}

/* (object, ?, kind): x by +10 (x 256), +10 by +14; then the count of its kind at 0x08B0274C (+18 for 0,
   +22 for 1, +23 for 2) up by one: 1 once it reaches 9, 15 and 14 (0 for other kinds). */
SFA_FUNC(088ABD4C, move_and_count_kind) {
    gaddr o = ARG(0);
    uint32_t k = ARG(2) & 0xFF;
    int32_t y = S16(o + 10);
    *W(o + 4) += (uint32_t)y << 8;
    *H(o + 10) = (uint16_t)(y + S16(o + 14));
    gaddr l = 0x08B0274Cu;
    uint32_t at, need;
    if (k == 0) { at = 18; need = 9; }
    else if (k == 1) { at = 22; need = 15; }
    else if (k == 2) { at = 23; need = 14; }
    else { RETURN(0); return; }
    uint8_t n = (uint8_t)(*B(l + at) + 1);
    *B(l + at) = n;
    RETURN(!((int32_t)n < (int32_t)need));
}

/* (record): its four timed lines (+0xE0, ten bytes each): +6 counts down to the line's start (a small
   object with line +2, or +4 with BATTLE +69), then +8 to its end (cleared, BATTLE +82 = 2). */
SFA_FUNC(089DE8A4, timed_lines) {
    gaddr r = ARG(0), s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    c->r[16] = 2; c->r[17] = BATTLE;
    gaddr e = r + 0xE0;
    for (uint32_t i = 0; i < 4; i++, e += 10) {
        c->r[18] = e; c->r[19] = i;
        if (S16(e) == 0) continue;
        int32_t t = S16(e + 6);
        if (t >= 0) {
            *H(e + 6) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) >= 0) continue;
            c->r[5] = (uint32_t)(int32_t)S8(BATTLE + 69);
            c->r[4] = (uint32_t)(int32_t)(S8(BATTLE + 69) != 0 ? S16(e + 4) : S16(e + 2));
            f_089DE978(c);
            continue;
        }
        int32_t u = S16(e + 8);
        if (u >= 0) {
            *H(e + 8) = (uint16_t)(u - 1);
            if ((int16_t)(u - 1) >= 0) continue;
        }
        *H(e) = 0;
        *B(BATTLE + 82) = 2;
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (effect, fighter): shown at the fighter's place while it is the fighter's character (+3) and its frame
   (+296) has +10, animated by that +10 (0x08A58688, on change); otherwise +127 cleared, or +4 moves on
   for another character. */
SFA_FUNC(089130D4, effect_follow_frame) {
    gaddr o = ARG(0), f = ARG(1);
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    if ((uint32_t)(int32_t)S8(f + 338) != *B(o + 3)) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    } else if (*B(*W(f + 296) + 10) == 0) {
        *B(o + 127) = 0;
    } else {
        *H(o + 18) = (uint16_t)S16(f + 18);
        *H(o + 22) = (uint16_t)S16(f + 22);
        *B(o + 1) = 1;
        *B(o + 11) = *B(f + 11);
        uint32_t n = *B(*W(f + 296) + 10);
        if (n == ((uint32_t)(int32_t)S8(o + 127) & 0xFF)) {
            c->r[4] = o;
            f_088219B0(c);
        } else {
            *B(o + 127) = (uint8_t)n;
            set_anim(c, o, *W(0x08A58688u + n * 4));
        }
    }
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): its input recorded (0x088636D8) for itself, or by battle state for its team: states 1 - 2
   fighters 3 and 4 on side bit 2, state 3 fighters 1 and 2 without it. */
SFA_FUNC(08863600, fighter_record_team_input) {
    gaddr f = ARG(0);
    int32_t t = S16(BATTLE + 56);
    c->r[29] -= 0x20;
    if (t == 0) { c->r[4] = f; f_088636D8(c); }
    else if (t == 1 || t == 2) {
        if (S8(f + 337) & 2) { c->r[4] = P3; f_088636D8(c); c->r[4] = P4; f_088636D8(c); }
        else { c->r[4] = f; f_088636D8(c); }
    } else if (t == 3) {
        if (S8(f + 337) & 2) { c->r[4] = f; f_088636D8(c); }
        else { c->r[4] = P1; f_088636D8(c); c->r[4] = P2; f_088636D8(c); }
    }
    c->r[29] += 0x20;
}
