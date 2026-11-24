/* Middle-sized steps of objects, fighters and records (0.106.0): counters that reload from tables,
 * places taken from a parent, script commands that test a level, steps through tables, and the
 * fighters' input counters. Offsets stay numbers until something shows what they are. Arguments of
 * native callees, and what the original leaves in a0 - a3 / t0 at a call through a table, are set as
 * the original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_08823970(Cpu *c);   /* game_fight_start */
void f_088392E4(Cpu *c);   /* record_354_clear */
void f_088D7C4C(Cpu *c);   /* net_battle_and_35 */
void f_088B098C(Cpu *c);   /* swap_96_74_100_28_a */
void f_088B09B0(Cpu *c);   /* nothing_088B09B0 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_088BD44C(Cpu *c);   /* start_anim_96a8 */
void f_088BAC18(Cpu *c);   /* game_option_pulse */
void f_088233BC(Cpu *c);   /* object_free_list_3 */
void f_08876894(Cpu *c);   /* fighter_frame_record */
void f_088A2808(Cpu *c);   /* game_bselect_state_1 */
void f_088347B4(Cpu *c);   /* fighter_set_412 */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0881CFD0(Cpu *c);   /* screen_backdrop_picture */
void f_0882196C(Cpu *c);   /* object_set_animation */

/* A call through a table of steps indexed by a byte: a0 the object, a1 the step, a2 the table. */
static void table_step(Cpu *c, gaddr o, gaddr table, uint32_t i) {
    gaddr fn = *W(table + i * 4);
    c->r[4] = o; c->r[5] = fn; c->r[6] = table;
    rt_call_indirect(c, fn);
}

/* ---------------------------------------------------------------- objects */

/* (object): shown (+1) only while its fighter's record (+78) has +58 set; then placed at the
   fighter's point in 0x08A13408, four right and four up. */
SFA_FUNC(08848050, place_at_fighter_point) {
    gaddr o = ARG(0);
    int32_t i = S8(o + 78);
    int64_t m = (int64_t)i * 0x354;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    *B(o + 1) = 0;
    if (S8(0x09519588u + (uint32_t)m + 58) == 0) return;
    *B(o + 1) = 1;
    gaddr e = 0x08A13408u + (uint32_t)i * 4;
    *H(o + 18) = (uint16_t)(*H(e) + 4);
    *H(o + 22) = (uint16_t)(*H(e + 2) - 4);
}

/* (object): from battle state 7 on, +4 moves on. Before it, +96 is added to +16 and grows by 0x10000;
   +4 moves on once +16 reaches 0x1800000. */
SFA_FUNC(08856AA0, fall_faster_until_1800000) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 44) >= 7) { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    uint32_t v = *W(o + 96), y = v + *W(o + 16);
    *W(o + 16) = y;
    *W(o + 96) = v + 0x10000;
    if ((int32_t)y >= 0x1800000) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object): kinds 1 and 4 take step 2 (+128) when GAME +40 is set; then its step from 0x08A25B48. */
SFA_FUNC(08887CBC, step_by_128) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 92);
    if ((k == 1 || k == 4) && S8(GAME + 40) != 0) *B(o + 128) = 2;
    c->r[29] -= 0x20;
    table_step(c, o, 0x08A25B48u, *B(o + 128));
    c->r[29] += 0x20;
}

/* (object): the trail. +36 moves on by two (in sixteen); unless +72 is negative, the point (+18, +22)
   goes into the ring at +112, and a set +72 marks the slot after it empty (0xFFFF) and becomes -1. */
SFA_FUNC(088B4514, trail_record_point) {
    gaddr o = ARG(0), ring = o + 0x70;
    uint32_t n = ((uint32_t)*H(o + 36) + 2) & 0xF;
    if (S8(o + 72) >= 0) {
        gaddr s = ring + n * 2;
        *H(s) = (uint16_t)S16(o + 18);
        *H(s + 2) = (uint16_t)S16(o + 22);
        if (S8(o + 72) != 0) {
            *H(ring + ((n + 2) & 0xF) * 2) = 0xFFFF;
            *B(o + 72) = 0xFF;
        }
    }
    *H(o + 36) = (uint16_t)n;
}

/* (object): shown (+93) while its parent (+124) has +1; then white, no added colour, at the parent's
   place (+6, +10 from +18, +22). */
SFA_FUNC(088CF908, show_with_parent_124) {
    gaddr o = ARG(0), p = *W(o + 124);
    *B(o + 93) = 0;
    if (*B(p + 1) == 0) return;
    *B(o + 93) = 1;
    for (int k = 36; k <= 39; k++) *B(o + k) = 0xFF;
    for (int k = 42; k <= 45; k++) *B(o + k) = 0xFF;
    for (int k = 48; k <= 51; k++) *B(o + k) = 0;
    *H(o + 6) = (uint16_t)S16(p + 18);
    *H(o + 10) = (uint16_t)S16(p + 22);
}

/* (a, b): the fight started (0x08823970), and the two 0x354 records at 0x09358528 and 0x0935887C
   cleared and given to the two sides (0x092E3EA0 / 0x092E4338 +784). */
SFA_FUNC(088D42F0, fight_start_two_records) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    f_08823970(c);
    c->r[16] = 0x09358410u;
    c->r[17] = 0x09358528u;
    c->r[4] = c->r[17]; f_088392E4(c);
    c->r[16] = 0x0935887Cu;
    c->r[4] = c->r[16]; f_088392E4(c);
    *W(0x092E3EA0u + 784) = 0x09358528u;
    *W(0x092E4338u + 784) = 0x0935887Cu;
    c->r[4] = 0x092E4338u;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): its step (+7) from 0x08A98AA8 when +192 is 2, otherwise from 0x08A98A74. */
SFA_FUNC(08998214, step_by_192) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    c->r[7] = 2;
    table_step(c, o, S8(o + 192) == 2 ? 0x08A98AA8u : 0x08A98A74u, *B(o + 7));
    c->r[29] += 0x20;
}

/* (object, entry): unless the entry's +2 is under the object's +118, its step (+3) from 0x08AB6A00
   with (object, entry). */
SFA_FUNC(089D99E8, step_unless_under_118) {
    gaddr o = ARG(0), e = ARG(1);
    c->r[29] -= 0x20;
    if (S16(e + 2) < (int32_t)*B(o + 118)) {
        c->r[4] = e; c->r[5] = o;
    } else {
        gaddr fn = *W(0x08AB6A00u + (uint32_t)*B(o + 3) * 4);
        c->r[4] = o; c->r[5] = e; c->r[6] = o; c->r[7] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
}

/* (object): set up from its numbers: +68 to +136 and +4; the top four bits of +66 to +103 (unless 15)
   with +70 - +76 to +126 - +132. +104 ends 0xFF. */
SFA_FUNC(089DFC40, setup_from_66) {
    gaddr o = ARG(0);
    uint16_t v = *H(o + 68);
    uint32_t k = ((uint32_t)(int32_t)S16(o + 66) & 0xF000) >> 12;
    *H(o + 136) = v;
    *H(o + 4) = v;
    if (k != 15) {
        *B(o + 103) = (uint8_t)k;
        *H(o + 126) = *H(o + 70);
        *H(o + 128) = *H(o + 72);
        *H(o + 130) = *H(o + 74);
        *H(o + 132) = *H(o + 76);
    }
    *B(o + 104) = 0xFF;
}

/* (object): when 0x09525E00 is set it is cleared, +98 set, and +110 changed: 0 and 15 to 15, 2 and 10
   to 10 (the table at 0x08A01D70). */
SFA_FUNC(089E044C, flag_5E00_change_110) {
    gaddr o = ARG(0);
    if (*B(0x09525E00u) == 0) return;
    *B(0x09525E00u) = 0;
    *B(o + 98) = 1;
    int32_t v = S16(o + 110);
    if (v < 0 || v >= 16) return;
    if (v == 0 || v == 15) *H(o + 110) = 15;
    else if (v == 2 || v == 10) *H(o + 110) = 10;
}

/* (fighter, object): the object's +72 from the fighter's +159 in a network battle when set, otherwise 9. */
SFA_FUNC(088237DC, set_72_from_159) {
    gaddr f = ARG(0), o = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = o; c->r[17] = f;
    f_088D7C4C(c);
    int32_t v;
    if (c->r[2] != 0 && (v = S8(f + 159)) != 0) *H(o + 72) = (uint16_t)v;
    else *H(o + 72) = 9;
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): while +0 is 1 and +260 is set, +311 counts down; at 0 it reloads from +312 and +260 counts
   down, stopping at 1. */
SFA_FUNC(08837950, count_311_then_260) {
    gaddr o = ARG(0);
    if (*B(o) != 1) return;
    int32_t a = S8(o + 260);
    if (a == 0) return;
    int32_t b = S8(o + 311);
    if (b != 0) {
        *B(o + 311) = (uint8_t)(b - 1);
        if ((int8_t)(b - 1) != 0) return;
        *B(o + 311) = (uint8_t)S8(o + 312);
    }
    *B(o + 260) = (uint8_t)(a - 1);
    if ((int8_t)(a - 1) == 0) *B(o + 260) = 1;
}

/* (object): once GAME +10 has 0x200, +5 moves on, +96 / +100 from +74 / +28, and the swap 0x088B098C. */
SFA_FUNC(088B08F8, on_200_swap) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*H(GAME + 10) & 0x200) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        int32_t v = S8(o + 74);
        *W(o + 96) = (uint32_t)v;
        *W(o + 100) = *W(o + 28);
        c->r[4] = o; c->r[5] = (uint32_t)v; f_088B09B0(c);
        c->r[4] = o; f_088B098C(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the timed list at 0x08B0284C: once GAME +14 reaches the time of entry +6 in its table, +0 takes
   the entry's value, +2 - +5 are cleared and +6 moves on. */
static void timed_list(gaddr table) {
    gaddr l = 0x08B0284Cu;
    uint16_t n = *H(l + 6);
    gaddr e = table + (uint32_t)(int16_t)n * 8;
    if (S16(GAME + 14) < S16(e)) return;
    *H(l) = (uint16_t)S16(e + 4);
    *B(l + 2) = 0; *B(l + 3) = 0; *B(l + 4) = 0; *B(l + 5) = 0;
    *H(l + 6) = (uint16_t)(n + 1);
}
SFA_FUNC(088B3040, timed_list_6EFC) { (void)c; timed_list(0x08A36EFCu); }
SFA_FUNC(088B30A8, timed_list_6F0C) { (void)c; timed_list(0x08A36F0Cu); }

/* (object): rises by two until +22 is -16; then +4 moves on and, for +78 = 4, a kind-0x71 object
   (step 5) with +78 cleared and +10 copied. */
SFA_FUNC(088BBDFC, rise_then_make_71) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t y = S16(o + 22);
    if (y != -16) {
        *H(o + 22) = (uint16_t)(y - 2);
    } else {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        if (S8(o + 78) == 4) {
            c->r[4] = 0x71; c->r[5] = 5;
            f_08822EAC(c);
            gaddr n = c->r[2];
            *B(n + 78) = 0;
            *B(n + 10) = *B(o + 10);
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): while its parent (+88) lives, its step (+4) from the table and drawn if on screen;
   otherwise freed. */
static void step_while_parent(Cpu *c, gaddr table, int clear_92) {
    gaddr o = ARG(0), s16 = c->r[16], p = *W(o + 88);
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(p) == 0) {
        if (clear_92) *W(o + 92) = 0;
        c->r[4] = o; c->r[5] = p;
        f_088231C8(c);
    } else {
        gaddr fn = *W(table + (uint32_t)*B(o + 4) * 4);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
        c->r[4] = o;
        f_08821424(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(088BC4B8, step_while_parent_a) { step_while_parent(c, 0x08A3AF28u, 1); }
SFA_FUNC(088BCC8C, step_while_parent_b) { step_while_parent(c, 0x08A3AF58u, 0); }

/* (object): shown, +4 moves on, set up as its parent's (+88) copy (0x15 picture), and the animation
   0x088BD44C with the parent's +10. */
SFA_FUNC(088BD8B0, copy_parent_start_anim) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 12) = 1;
    *B(o + 13) = 0x15;
    *H(o + 18) = (uint16_t)S16(p + 18);
    *H(o + 22) = (uint16_t)S16(p + 22);
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = *B(p + 10); c->r[6] = p; c->r[7] = 1;
    f_088BD44C(c);
    c->r[29] += 0x20;
}

/* (object): the option pulse, +6 from 0x08A3CA60 by GAME +6, and +136 = 0xFF while GAME +4 is set
   (not in mode 4). */
SFA_FUNC(088CF73C, option_pulse_set_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088BAC18(c);
    *H(o + 6) = *H(0x08A3CA60u + (uint32_t)*H(GAME + 6) * 2);
    if (*H(GAME + 6) != 4 && *H(GAME + 4) != 0) *B(o + 136) = 0xFF;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): kind 4 turns +28 round and moves +10 by it; then, if its parent (+124) is at step 2, it is
   hidden (+93) and freed. */
SFA_FUNC(089DDB68, turn_then_free_with_parent) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 92);
    if (k == 0) return;
    gaddr p = *W(o + 124);
    if (k == 4) {
        *H(o + 28) = (uint16_t)(-S16(o + 28));
        *H(o + 10) = (uint16_t)(S16(o + 10) - S16(o + 28));
    }
    if (p == 0 || *B(p + 4) != 2) return;
    *B(o + 93) = 0;
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = 2; c->r[6] = 2;
    f_088233BC(c);
    c->r[29] += 0x20;
}

/* (object): shown at (0xC0, 0x3C) in mode 2 when its parent's +2 is 1, or in mode 3 when its +1 is 1. */
SFA_FUNC(08847FE4, show_by_mode_2_3) {
    gaddr o = ARG(0), p = *W(o + 88);
    uint32_t m = *H(GAME + 6);
    *B(o + 1) = 0;
    if ((m == 2 && S8(p + 2) == 1) || (m == 3 && S8(p + 1) == 1)) {
        *B(o + 1) = 1;
        *H(o + 18) = 0xC0;
        *H(o + 22) = 0x3C;
    }
}

/* (fighter): one of BATTLE +136 / +140, chosen by the fighter's +121 and swapped in battle state 2
   (or 3 without GAME +57 bit 0). */
SFA_FUNC(0886B468, battle_136_or_140) {
    int32_t t = S16(BATTLE + 56), v = S8(ARG(0) + 121);
    int swap = t == 2 || (t == 3 && (S8(GAME + 57) & 1) == 0);
    int first = swap ? v != 0 : v == 0;
    RETURN(*W(BATTLE + (first ? 136 : 140)));
}

/* (fighter): +74 counts the frame down; at 0 the next record (+28) is +6 records on (ten bytes each),
   or with flag 0x40 +75 = 0xFF and it stays. */
SFA_FUNC(088768F0, fighter_next_frame) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    int32_t n = S8(f + 74) - 1;
    *B(f + 74) = (uint8_t)n;
    if ((int8_t)n == 0) {
        gaddr r = *W(f + 28);
        int32_t fl = S8(r + 1);
        if (fl & 0x40) {
            *B(f + 75) = 0xFF;
        } else {
            int32_t s = S16(r + 6);
            *W(f + 28) = r + (uint32_t)(s * 10);
            c->r[4] = f; c->r[5] = (uint32_t)(fl & 0x80); c->r[6] = (uint32_t)(s * 10); c->r[7] = (uint32_t)(s * 9);
            f_08876894(c);
            *W(f + 152) = 1;
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (me, a, list): with +424 bit 0x20 +4 moves on and state 1's step (0x088A2808); otherwise +5 moves on,
   the list's first byte is cleared and +82 = 600. */
SFA_FUNC(088A2660, bselect_on_20) {
    gaddr me = ARG(0), q = ARG(2);
    c->r[29] -= 0x20;
    if (S16(me + 424) & 0x20) {
        *B(me + 4) = (uint8_t)(*B(me + 4) + 1);
        c->r[4] = me; c->r[6] = q; c->r[7] = q;
        f_088A2808(c);
    } else {
        *B(me + 5) = (uint8_t)(*B(me + 5) + 1);
        *B(q) = 0;
        *H(me + 82) = 0x258;
        c->r[4] = q;
    }
    c->r[29] += 0x20;
}

/* (object): +72 counts down; at 0 the next entry of 0x08A2C68C (+73) gives +72 (high byte), +73 (low)
   and 0x08B027F2. */
SFA_FUNC(088AA9BC, count_72_next_entry) {
    gaddr o = ARG(0);
    uint8_t n = (uint8_t)(*B(o + 72) - 1);
    *B(o + 72) = n;
    if (n != 0) return;
    gaddr e = 0x08A2C68Cu + (uint32_t)*B(o + 73) * 2;
    uint16_t h = *H(e);
    *B(o + 72) = (uint8_t)(h >> 8);
    *B(o + 73) = (uint8_t)h;
    *H(0x08B027F2u) = *H(e + 2);
}

/* (object): the top half of +108 kept within 0 - 155. */
SFA_FUNC(088B4FD4, wrap_108_156) {
    gaddr o = ARG(0);
    uint32_t w = *W(o + 108);
    int32_t v = (int16_t)(w >> 16);
    while (v < 0) v = (int16_t)(v + 156);
    while (v >= 156) v = (int16_t)(v - 156);
    *W(o + 108) = (w & 0xFFFF) | ((uint32_t)v << 16);
}

/* (object): +4 moves on and it is set up as picture 8 + its +3 (+84 is 0 for +3 = 3, otherwise 1). */
SFA_FUNC(088BC7FC, setup_picture_by_3) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 3);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 3;
    *B(o + 14) = 0;
    *H(o + 24) = 7;
    *B(o + 12) = 1;
    *B(o + 13) = (uint8_t)(k + 8);
    *B(o + 11) = 0;
    *H(o + 82) = 0;
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    *H(o + 84) = k == 3 ? 0 : 1;
    *W(o + 148) = 3;
}

/* (object): its step (+4) from 0x08A3C93C; kinds 6 and 7 (+3) hidden while 0x09525881 is set. */
SFA_FUNC(088CE120, step_hide_6_7) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    table_step(c, o, 0x08A3C93Cu, *B(o + 4));
    uint32_t k = *B(o + 3);
    if ((k == 6 || k == 7) && S8(0x09525881u) != 0) *B(o + 1) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, other): a level 1 - 2: 1 for +47 = 3; with +137, 2 from the other's +9 = 70; without,
   from 0x08A0EF1C by the other's +9 over 60. */
SFA_FUNC(0882BD04, fighter_level_by_9) {
    gaddr f = ARG(0), g = ARG(1);
    int32_t a = S8(f + 137), b = S8(f + 47);
    if (b == 3) { RETURN(1); return; }
    int32_t v = S8(g + 9);
    if (a != 0) { RETURN(v < 70 ? 1 : 2); return; }
    v -= 60;
    RETURN(v <= 0 ? 1 : (uint32_t)(int32_t)S8(0x08A0EF1Cu + (uint32_t)v));
}

/* (a, b, fighter): a new direction press (+362 not in +364) adds one to +341 and reloads +347 with 31;
   otherwise +347 counts down, at 0 also adding to +341, and in between +412 = 10 (0x088347B4). */
SFA_FUNC(088344E8, fighter_count_presses) {
    gaddr f = ARG(2);
    c->r[29] -= 0x20;
    uint32_t m = ~(uint32_t)*H(f + 364) & *H(f + 362) & 0x3F0;
    c->r[4] = f;
    int again = m != 0;
    if (!again) {
        uint8_t n = (uint8_t)(*B(f + 347) - 1);
        *B(f + 347) = n;
        if (n != 0) {
            c->r[5] = 10;
            f_088347B4(c);
        } else again = 1;
    }
    if (again) {
        *B(f + 341) = (uint8_t)(S8(f + 341) + 1);
        *B(f + 347) = 0x1F;
        c->r[5] = (uint32_t)(S8(f + 341));
        c->r[6] = 0x1F;
    }
    c->r[29] += 0x20;
}

/* (fighter): the input (+248) recorded at +378 + (+374) - 1; 0x40 counts +436 and the direction kind
   (0x08A195B0) counts its own +433 slot, both up to 99. */
SFA_FUNC(088636D8, fighter_record_input) {
    gaddr f = ARG(0);
    int32_t i = S8(f + 374), v = S8(f + 248);
    *B(f + 0x17A + (uint32_t)(i - 1)) = (uint8_t)v;
    if (v & 0x40) {
        int32_t n = S8(f + 436);
        if (n < 99) *B(f + 436) = (uint8_t)(n + 1);
    }
    int32_t t = S8(0x08A195B0u + (uint32_t)(v & 0xF));
    if (t < 0) return;
    gaddr q = f + (uint32_t)t;
    int32_t n = S8(q + 433);
    if (n < 99) *B(q + 433) = (uint8_t)(n + 1);
}

/* (fighter, counter, mask, wait, need): a press in the mask (+760 bits 0x3F0) counts the counter's +1
   and reloads its wait; reaching `need` gives 1. Without a press the wait runs down and clears the count. */
SFA_FUNC(08869B70, fighter_press_count) {
    gaddr f = ARG(0), p = ARG(1);
    uint32_t mask = ARG(2) & 0xFFFF, wait = ARG(3) & 0xFFFF, need = c->r[8] & 0xFFFF;
    c->r[4] = need; c->r[6] = wait;
    if ((*H(f + 760) & 0x3F0 & mask) != 0) {
        *B(p) = (uint8_t)wait;
        *B(p + 1) = (uint8_t)(*B(p + 1) + 1);
        if ((int32_t)*B(p + 1) >= (int32_t)need) { *B(p + 1) = (uint8_t)need; RETURN(1); return; }
    } else {
        uint8_t n = (uint8_t)(*B(p) - 1);
        *B(p) = n;
        if (n == 0) { *B(p) = (uint8_t)wait; *B(p + 1) = 0; }
    }
    RETURN(0);
}

/* (fighter, script): script command: +492 from the script; past the level (+508) the block is left
   (0x08879C8C), otherwise read on (0x0887A10C). */
SFA_FUNC(08878ACC, fighter_script_cmd_492) {
    gaddr f = ARG(0), p = ARG(1);
    *H(f + 492) = (uint16_t)S16(p);
    int leave = S16(f + 508) < S16(f + 492);
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p + 2; c->r[6] = p + 2;
    if (leave) f_08879C8C(c); else f_0887A10C(c);
    c->r[29] += 0x20;
}

/* (fighter, script): script command: +490 from the script; at or past the level (+508) read on,
   otherwise the fighter's script ends (state 1, step 2). */
SFA_FUNC(08877FBC, fighter_script_cmd_490) {
    gaddr f = ARG(0), p = ARG(1);
    int32_t v = S16(p);
    *H(f + 490) = (uint16_t)v;
    int32_t d = (int16_t)(v - S16(f + 508));
    c->r[2] = p + 2;
    c->r[29] -= 0x20;
    if (d >= 0) {
        c->r[5] = p + 2; c->r[6] = (uint32_t)d; c->r[7] = (uint32_t)(v - S16(f + 508));
        f_0887A10C(c);
    } else {
        *B(f + 482) = 1; *B(f + 483) = 0;
        *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
        *B(f + 78) = 0; *B(f + 497) = 0;
        c->r[5] = 1; c->r[6] = 2;
    }
    c->r[29] += 0x20;
}

/* (object): +20 moves by +100; on reaching 0x800000 +5 moves on, +22 = 0x80 and +82 = 16. Drawn. */
SFA_FUNC(088815D4, move_20_to_800000) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 100), y = *W(o + 20) + v;
    *W(o + 20) = y;
    uint32_t d = y - 0x800000u;
    c->r[29] -= 0x20;
    if (d == 0 || (int32_t)(v ^ d) >= 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 22) = 0x80;
        *H(o + 82) = 0x10;
        c->r[5] = 0x10; c->r[6] = 0x80;
    } else {
        c->r[5] = v ^ d; c->r[6] = v; c->r[7] = 0x800000u;
    }
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): drawn; kind 2 (+3) hides and moves on once GAME +2 reaches 5, kind 0 also draws the
   backdrop picture at (0x80, 0xB8). */
SFA_FUNC(088A904C, draw_kind_0_backdrop) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088215C4(c);
    uint32_t k = *B(o + 3);
    if (k == 2) {
        if (*H(GAME + 2) >= 5) {
            *B(o + 1) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        }
    } else if (k == 0) {
        c->r[4] = 0x80; c->r[5] = 0xB8;
        f_0881CFD0(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): hidden; drawn if on screen while GAME +16's high byte is 0; +4 moves on once its low byte
   is no longer +32. */
SFA_FUNC(088AE240, draw_until_16_changes) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = GAME;
    *B(o + 1) = 0;
    if ((S16(GAME + 16) & 0xFF00) == 0) { c->r[4] = o; c->r[5] = (uint32_t)(int32_t)S16(GAME + 16); f_08821424(c); }
    uint32_t lo = (uint32_t)(int32_t)S16(GAME + 16) & 0xFF;
    int32_t want = S16(o + 32);
    if ((int32_t)lo != want) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): kind 11 (+92) moves +128 on while GAME +16's high byte is set, kind 13 while it is clear. */
SFA_FUNC(088B29C0, step_128_by_16_high) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 92), high = *H(GAME + 16) & 0xFF00;
    if ((k == 11 && high != 0) || (k == 13 && high == 0)) *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
}

/* (object): the top half of +108 split by 39: the remainder plus 0x100 to +22, the quotient picks the
   animation from the list at 0x08A37314. */
SFA_FUNC(088B4EBC, anim_by_108_over_39) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 108) >> 16, q = v / 39, r = v % 39;
    c->lo = q; c->hi = r;
    *H(o + 22) = (uint16_t)((int16_t)r + 0x100);
    uint32_t off = (uint32_t)(int32_t)(int16_t)q << 3;
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = *W(0x08A37314u) + off; c->r[6] = off; c->r[7] = 0x08A37310u; c->r[8] = r;
    f_0882196C(c);
    c->r[29] += 0x20;
}
