/* More steps (0.123.0): the fight resets in their three sizes, the ending task, a fighter's round part
   or guard choice and push, the entries' panel, a fighter that may throw, score points for a hit, a
   panel by type, the score panel's digits and whether a human or the computer gets up early. Offsets
   stay numbers until something shows what they are. Arguments of callees, and what the original leaves
   in a0 - a3 at a call (step dispatchers hand them on), are set as the original sets them; s0 - s2 as
   the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define SCORE_MAX 9999999u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_08822040(Cpu *c);   /* fight_reset */
void f_08821F58(Cpu *c);   /* fight_reset_short */
void f_08823304(Cpu *c);   /* game_objects_3_clear */
void f_08822C84(Cpu *c);   /* layers_clear */
void f_0880A8B8(Cpu *c);   /* queue_4_clear */
void f_0880A8F4(Cpu *c);   /* queue_10_clear */
void f_08822A64(Cpu *c);   /* clear_092E5100 */
void f_08822C00(Cpu *c);   /* fighters_clear */
void f_08822D04(Cpu *c);   /* battle_clear */
void f_088054F0(Cpu *c);   /* game_task_wait_frames */
void f_0885EB74(Cpu *c);   /* fighter_round_part_reset */
void f_0885ECE8(Cpu *c);   /* fighter_guard_choice */
void f_08862FF0(Cpu *c);   /* fighters_push_apart */
void f_088B4F2C(Cpu *c);   /* move_then_wrap */
void f_088CE33C(Cpu *c);   /* entry_names */
void f_088625C0(Cpu *c);   /* near_other_side */
void f_08822E74(Cpu *c);   /* object_make_8 */
void f_088988FC(Cpu *c);   /* fighter_display_side */
void f_08887D20(Cpu *c);   /* step_08887D20 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08857678(Cpu *c);   /* fighter_by_layout_3 */
void f_088576EC(Cpu *c);   /* digits_as_pictures */
void f_08821368(Cpu *c);   /* game_random */
void f_08877258(Cpu *c);   /* fighter_chance_by_level */

/* ---------------------------------------------------------------- resets and the ending task */

/* (): the fight reset and the game's objects cleared. */
SFA_FUNC(08821FC8, fight_reset_objects) {
    c->r[29] -= 0x10;
    f_08822040(c);
    f_08823304(c);
    c->r[29] += 0x10;
}

/* (): the layers too. */
SFA_FUNC(08821F9C, fight_reset_layers) {
    c->r[29] -= 0x10;
    f_08822C84(c);
    f_08822040(c);
    f_08823304(c);
    c->r[29] += 0x10;
}

/* (): everything: the text queues, the fighters, the layers, the battle record, 0x092E5100, then the
   fight reset and the game's objects. */
SFA_FUNC(08821FEC, fight_reset_all) {
    c->r[29] -= 0x10;
    f_0880A8B8(c);
    f_0880A8F4(c);
    f_08822C00(c);
    f_08822C84(c);
    f_08822D04(c);
    f_08822A64(c);
    f_08822040(c);
    f_08823304(c);
    c->r[29] += 0x10;
}

/* The ending task: the short fight reset, 0x09525E00 and GAME + 0 - + 22's steps cleared; then every
   frame GAME + 32 counts and the step by GAME + 0 (0x08AB7680) runs with (GAME, BATTLE, 0x09358BE0). */
SFA_FUNC(089DE0E8, ending_task) {
    c->r[29] -= 0x20;
    c->r[18] = GAME; c->r[17] = BATTLE; c->r[16] = 0x09358BE0u;
    f_08821F58(c);
    *B(0x09525E00u) = 0;
    static const uint32_t CLEAR[10] = { 22, 20, 18, 16, 10, 8, 6, 4, 2, 0 };
    for (int i = 0; i < 10; i++) *H(GAME + CLEAR[i]) = 0;
    for (;;) {
        *B(GAME + 32) = (uint8_t)(*B(GAME + 32) + 1);
        gaddr fn = *W(0x08AB7680u + (uint32_t)*H(GAME) * 4);
        c->r[7] = fn;
        c->r[4] = GAME; c->r[5] = BATTLE; c->r[6] = 0x09358BE0u;
        rt_call_indirect(c, fn);
        c->r[4] = 1;
        f_088054F0(c);
    }
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): the round's part reset at its start (+7 = 0), the guard choice after; then the fighters
   pushed apart. */
SFA_FUNC(0885EB2C, fighter_round_step) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    c->r[5] = *B(f + 7);
    c->r[4] = f;
    if (c->r[5] == 0) f_0885EB74(c);
    else f_0885ECE8(c);
    c->r[4] = f;
    f_08862FF0(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): whether it may throw now: not while +251, +816, +790 = 8; the buttons for it (+764 bit 0,
   or bit 1 not pressed with +792's 0x2000; +792's 0x00800020 alone needs none; in Training a side
   marked in 0x0950FF71 needs none either); then +694 = 1, and with ISM -1 (+386) a +73 fighter needs
   +792's 0x100; and another fighter near (0x088625C0). 1 or 0. */
SFA_FUNC(08862484, fighter_may_throw) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    uint32_t r = 0;
    if (S8(f + 251) != 0 || *W(f + 816) != 0 || S8(f + 790) == 8) goto out;
    if (S8(GAME + 114) == 2) {
        int side = S8(f + 337);
        uint32_t marks = *B(0x0950FF60u + 17);
        if ((marks & (7u << ((side << 2) & 31))) == 0 && (*H(f + 764) & 1) == 0) goto out;
    } else {
        uint32_t w = *W(f + 792);
        if ((w & 0x00800020u) != 0 || (w & 0x2000) != 0) {
            if ((w & 0x2000) != 0 && (*H(f + 764) & 2) != 0) goto out;
        } else if ((*H(f + 764) & 1) == 0) goto out;
    }
    c->r[16] = 1;
    *B(f + 694) = 1;
    if (S8(f + 386) == -1 && S8(f + 73) != 0 && (*W(f + 792) & 0x100) == 0) goto out;
    c->r[4] = f;
    f_088625C0(c);
    if (c->r[2] != 0) r = 1;
out:
    RETURN(r);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): whether it gets up early. A human (+373 = 0) not +313: once +604 is set (in Training only
   for a side not in GAME + 53 with 0x0950FF68 not 0 / 4, otherwise by two or three punches or kicks
   held / pressed together, +760 / +762), and nothing holding it (GAME + 38, +702, +587, BATTLE + 13 /
   14, +586, +592 with +773): when it is higher than +132 by more than 0x10 / 0x20 / 0x30 (its held
   direction, +752; in Training by 0x0950FF68: 1 0x20, 2 0x30, 3 0x10, 5 a random 0 - 0x30). The
   computer: nothing holding it, by its level's chance (0x08A22AA0), and 48 higher. 1 or 0. */
SFA_FUNC(08866A84, fighter_gets_up_early) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    uint32_t r = 0;
    uint32_t need;
    if (S16(f + 112) < 0) goto out;
    if (S8(f + 373) != 0) {
        int32_t held = S8(f + 586) | S8(f + 773) | S8(BATTLE + 14) | S8(BATTLE + 13) | S8(GAME + 38) |
                       S8(f + 702) | S8(f + 587);
        if (held != 0) goto out;
        if (S8(f + 592) != 0) goto out;
        c->r[4] = f; c->r[5] = 0x08A22AA0u;
        f_08877258(c);
        if (c->r[2] == 0) goto out;
        if (S16(f + 22) - S16(f + 132) < 48) goto out;
        r = 1;
        goto out;
    }
    if (S8(f + 313) != 0) goto out;
    if (S8(f + 604) == 0) {
        int keys = 1;
        if (S8(GAME + 114) == 2 && (S8(GAME + 53) & (1 << (S8(f + 337) & 31))) == 0) {
            uint32_t opt = *B(0x0950FF60u + 8);
            if (opt != 0 && opt != 4) keys = 0;
        }
        if (keys) {
            uint32_t v = (uint32_t)(*H(f + 760) | *H(f + 762)) & 0x70;
            if (v != 0x70 && v != 0x30 && v != 0x50 && v != 0x60) goto out;
        }
        *B(f + 604) = 1;
    }
    if ((S8(GAME + 38) | S8(f + 702) | S8(f + 587) | S8(BATTLE + 14) | S8(BATTLE + 13) | S8(f + 586)) != 0)
        goto out;
    if (S8(f + 592) != 0 && S8(f + 773) != 0) goto out;
    if (S8(GAME + 114) == 2 && (S8(GAME + 53) & (1 << (S8(f + 337) & 31))) == 0) {
        uint32_t opt = *B(0x0950FF60u + 8);
        if (opt == 5) {
            c->r[4] = 1; c->r[5] = 2; c->r[6] = 5; c->r[7] = 3;
            f_08821368(c);
            int32_t v = (int32_t)c->r[2];
            need = ((uint32_t)((v % 4) << 4)) & 0xFFFF;
            if (need == 0) goto out;
        } else if (opt == 3) need = 0x10;
        else if (opt == 2) need = 0x30;
        else if (opt == 1) need = 0x20;
        else need = 0x10;
    } else {
        uint32_t d = *H(f + 752) & 3;
        need = d == 0 ? 0x30 : (d & 2) ? 0x20 : 0x10;
    }
    if ((int32_t)need < (int32_t)((uint32_t)(S16(f + 22) - S16(f + 132)) & 0xFFFF)) r = 1;
out:
    RETURN(r);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (hit object): its follower (kind 4 of the 8-pool, type the fighter's +248) made; then, outside a set
   match, BATTLE + 61 and a computer fighter, the hit's points (by +248's low four bits, 0x08A17300, or
   0x08A17320 with its bit 5) added to the fighter's score (+352, at most 9,999,999) and, in Variable
   Battle, Survival kind 5 and Free Battle kind 3, its partner's (+1168) too. In World Tour (mode 3) only
   while 0x09517E03 is 0 and 0x09517E11 or 0x09517E02 under 46 (0x08A17340 / 0x08A17320), into the
   score and 0x09517E10. */
SFA_FUNC(088586C8, hit_score_points) {
    gaddr o = ARG(0);
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = o;
    gaddr f = *W(o + 88);
    c->r[18] = f;
    c->r[17] = 0;
    c->r[4] = 4; c->r[5] = (uint32_t)S8(f + 248) & 0xFF;
    f_08822E74(c);
    gaddr p = c->r[2];
    if (p != 0) {
        *B(p + 78) = *B(o + 3);
        *B(p + 9) = (uint8_t)(*B(o + 9) + 2);
        *W(p + 88) = o;
        *W(p + 96) = *W(o + 96);
        *W(p + 92) = f;
    }
    if ((S8(BATTLE + 16) | S8(BATTLE + 61) | S8(f + 373)) != 0) goto out;
    int32_t v = S8(f + 248);
    uint32_t k = (uint32_t)(v & 0xF);
    int m = S8(GAME + 114);
    if (v & 0x20) c->r[17] = 1;
    if (m == 3) {
        gaddr t = 0x09517A78u;
        if (S8(t + 899) != 0) goto out;
        if (S8(t + 913) == 0 && !(S8(t + 906) < 46)) goto out;
        uint32_t pts = *H((c->r[17] ? 0x08A17320u : 0x08A17340u) + k * 2);
        if (pts == 0) goto out;
        uint32_t sc = *W(f + 352) + pts;
        *W(f + 352) = sc;
        if ((int32_t)SCORE_MAX < (int32_t)sc) *W(f + 352) = SCORE_MAX;
        uint32_t tot = *W(t + 920) + pts;
        *W(t + 920) = tot;
        if ((int32_t)SCORE_MAX < (int32_t)tot) *W(t + 920) = SCORE_MAX;
        goto out;
    }
    uint32_t pts = *H((c->r[17] ? 0x08A17320u : 0x08A17300u) + k * 2);
    if (pts == 0) goto out;
    uint32_t sc = *W(f + 352) + pts;
    int both = m == 15 || (m == 7 && S8(BATTLE + 79) == 5) || (m == 0x10 && S8(BATTLE + 159) == 3);
    *W(f + 352) = sc;
    if ((int32_t)SCORE_MAX < (int32_t)sc) *W(f + 352) = SCORE_MAX;
    if (both) {
        gaddr q = *W(f + 1168);
        c->r[18] = q;
        uint32_t qs = *W(q + 352) + pts;
        *W(q + 352) = qs;
        if ((int32_t)SCORE_MAX < (int32_t)qs) *W(q + 352) = SCORE_MAX;
    }
out:
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* ---------------------------------------------------------------- objects */

/* (object): moved and wrapped; +82 counts down, at 0 its next step (+5). */
SFA_FUNC(088B4D88, move_then_count_82) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088B4F2C(c);
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): the entries' panel by type (+3): 5 writes the entries' names; 6 and 7 are shown (+1) on the
   screen's step 2 once GAME + 4 is set, unless the owner's +1 is 2, blinking on +82's bit 5 unless a
   pad holds up or down (0x08E75BA4 / 0x08E75BD0 bits 2 - 3). */
SFA_FUNC(088CE274, entries_panel) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t t = *B(o + 3);
    gaddr p = *W(o + 88);
    c->r[5] = p;
    if (t == 5) {
        c->r[6] = 5; c->r[7] = 5;
        f_088CE33C(c);
    } else if (t == 6 || t == 7) {
        *B(o + 1) = 0;
        if (*H(GAME + 6) == 2 && *H(GAME + 4) != 0) {
            *B(o + 1) = 1;
            if (S8(p + 1) != 2) {
                int16_t n = (int16_t)(S16(o + 82) + 1);
                *H(o + 82) = (uint16_t)n;
                if (n & 0x20) *B(o + 1) = 0;
                if ((*W(0x08E75BA0u + 4) | *W(0x08E75BA0u + 48)) & 0xC) *B(o + 1) = 1;
            }
        }
    }
    c->r[29] += 0x20;
}

/* (panel): its next step (+128), shown (+93), +55 = 7, +3 = 0, +140 = 0xF, then by type (+92): 0 and 1
   picture 0x1C / 0x1B and their sizes; 2 / 3 picture 0x1E with the sizes at 0x08A25B6C (+140 = 0xB); 4
   picture 0x1B on the owner's side of the screen (x 0x240 moving -0x18 a frame, or -0xC0 moving 0x18;
   y 0x40 / 0xC0 by the owner being the computer), its twelve colour bytes 1; then its step by type. */
SFA_FUNC(08887EE8, panel_by_type) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    c->r[17] = 1;
    *B(o + 93) = 1;
    *B(o + 55) = 7;
    *B(o + 3) = 0;
    uint32_t t = *B(o + 92);
    *B(o + 140) = 0xF;
    if (t == 0) {
        *B(o + 56) = 0;
        *B(o + 2) = 0x1C;
        *W(o + 4) = 0x00C00000u;
        *H(o + 12) = 0xFF40;
        *H(o + 24) = 0x20;
        *H(o + 14) = 0x180;
        c->r[5] = 0x180;
    } else if (t == 1) {
        *B(o + 136) = 0x80;
        *B(o + 56) = 6;
        *B(o + 2) = 0x1B;
        *W(o + 4) = 0x00C00000u;
        *H(o + 12) = 0xFF60;
        *H(o + 24) = 0x28;
        *H(o + 14) = 0x140;
        *H(o + 26) = 0x38;
        c->r[5] = 0x140; c->r[6] = 2;
    } else if (t == 2 || t == 3) {
        *B(o + 56) = 6;
        gaddr e = 0x08A25B6Cu + (t & 1) * 8;
        *B(o + 2) = 0x1E;
        *H(o + 12) = (uint16_t)S16(e);
        *H(o + 14) = (uint16_t)S16(e + 2);
        *H(o + 24) = (uint16_t)S16(e + 4);
        *H(o + 26) = (uint16_t)S16(e + 6);
        *B(o + 140) = 0xB;
        c->r[5] = 0xB; c->r[6] = 0x1E;
    } else if (t == 4) {
        *B(o + 136) = 0x80;
        *B(o + 56) = 1;
        *B(o + 1) = 0;
        gaddr p = *W(o + 124);
        *B(o + 2) = 0x1B;
        *H(o + 98) = 0xF;
        c->r[4] = p; c->r[5] = 0x1B; c->r[6] = p;
        f_088988FC(c);
        if (c->r[2] == 0) { *H(o + 6) = 0xFF40; *W(o + 104) = 0x18; }
        else { *H(o + 6) = 0x240; *W(o + 104) = 0xFFFFFFE8u; c->r[7] = 0xFFFFFFE8u; }
        if (S8(*W(o + 124) + 373) == 0) { *H(o + 10) = 0xC0; *W(o + 108) = 8; c->r[6] = 8; }
        else { *H(o + 10) = 0x40; *W(o + 108) = 0xFFFFFFF8u; c->r[6] = 0xFFFFFFF8u; }
        *H(o + 12) = 0xFF40;
        *H(o + 24) = 0x20;
        *H(o + 14) = 0x180;
        *H(o + 26) = 0x30;
        for (int i = 0; i < 4; i++) { *B(o + 36 + i) = 1; *B(o + 42 + i) = 1; *B(o + 48 + i) = 1; }
        c->r[5] = 0x180;
    } else {
        c->r[5] = 0x80; c->r[6] = 4;
    }
    c->r[4] = o;
    f_08887D20(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (panel): +4 on, +82 = 0x17; set up as a 6-cell picture (+86) at (0xF6, 0x78) with the animation
   0x08A16B0C + 20 by its kind +73: 0 in a set match (BATTLE + 16), else 1; 2 with BATTLE + 5, 3 when
   then BATTLE + 32 is clear or GAME + 137 set; then the digits into its frame's pictures (0x08A166D8, six bytes each, by its frame's +4)
   for the fighter (0x08857678): kind 0 the clear time and wins (0x09523C94, +426), 1 the score parts
   (0x09523C8C / 90) and +392, 2 and 3 as 1 led by 0x09523C88 (2) and the time; the last picture's
   bit 15 set. */
SFA_FUNC(08857810, score_panel_digits) {
    gaddr o = ARG(0);
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[17] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = 0x17;
    c->r[18] = BATTLE;
    f_08857678(c);
    gaddr f = c->r[2];
    *B(o + 9) = 4;
    *B(o + 14) = 0;
    *H(o + 86) = 6;
    *W(o + 164) = 1;
    *W(o + 148) = 1;
    *B(o + 13) = 0;
    *B(o + 12) = 0;
    *H(o + 24) = 7;
    *B(o + 73) = 0;
    c->r[16] = f;
    if (S8(BATTLE + 16) == 0) {
        *B(o + 73) = 1;
        if (S8(BATTLE + 5) != 0) {
            *B(o + 73) = 2;
            if (S8(BATTLE + 32) == 0 || S8(GAME + 137) != 0) *B(o + 73) = 3;
        }
    }
    int32_t k = S8(o + 73);
    *H(o + 18) = 0xF6;
    *H(o + 22) = 0x78;
    c->r[4] = o; c->r[5] = *W(0x08A16B0Cu + (uint32_t)k * 4 + 20); c->r[6] = 0x78;
    f_0882196C(c);
    gaddr pic = 0x08A166D8u + (uint32_t)*H(*W(o + 28) + 4) * 6;
    c->r[17] = (uint32_t)k;
    const gaddr T = 0x09523C88u;
    gaddr at = pic;
#define DIGITS(v, d) do { c->r[4] = at; c->r[5] = (v); c->r[6] = (d); f_088576EC(c); at = c->r[2]; } while (0)
    if (k >= 0 && k < 2) {
        c->r[17] = T;
        if (k == 0) {
            DIGITS(*W(T + 12), 10);
            DIGITS((uint32_t)(int32_t)S16(f + 426), 10);
        } else {
            DIGITS(*W(T + 4), 10000);
            DIGITS(*W(T + 8), 10);
            DIGITS(*W(f + 392), 1000000);
        }
    } else if (k == 2 || k == 3) {
        if (k == 2) DIGITS(*W(T), 100000);
        c->r[17] = T;
        DIGITS(*W(T + 12), 10);
        DIGITS(*W(T + 4), 10000);
        DIGITS(*W(T + 8), 10);
        DIGITS(*W(f + 392), 1000000);
    }
#undef DIGITS
    *H(at - 2) = (uint16_t)(*H(at - 2) | 0x8000);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}
