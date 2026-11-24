/* More middle-sized steps (0.107.0): positions from tables and parents, counters, a frame's object
 * lists, the reset before a fight, text screens and fighters' script commands. Offsets stay numbers
 * until something shows what they are. Arguments of native callees, and what the original leaves in
 * a0 - a3 / t0 at a call through a table, are set as the original sets them; s0 - s4 as the original
 * sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_088B4FD4(Cpu *c);   /* wrap_108_156 */
void f_08837690(Cpu *c);   /* game_objects_1 */
void f_0883771C(Cpu *c);   /* game_objects_4 */
void f_08887AFC(Cpu *c);   /* game_objects_3 */
void f_0880B6CC(Cpu *c);   /* game_end_of_frame */
void f_08819144(Cpu *c);   /* screen_vs_backdrop_draw */
void f_0882222C(Cpu *c);   /* pool_6_reset */
void f_088222C4(Cpu *c);   /* pool_3_reset */
void f_0882235C(Cpu *c);   /* small_objects_reset */
void f_088224DC(Cpu *c);   /* game_sprites_reset */
void f_088226E0(Cpu *c);   /* fight_tables_clear */
void f_08821368(Cpu *c);   /* game_random */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08855934(Cpu *c);   /* move_x_unless_battle_78 */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08858418(Cpu *c);   /* owned_set_animation */
void f_0885847C(Cpu *c);   /* game_add_score_by_record */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_088D5E44(Cpu *c);   /* game_network_battle */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088BAC18(Cpu *c);   /* game_option_pulse */
void f_0881CFD0(Cpu *c);   /* screen_backdrop_picture */
void f_0880E434(Cpu *c);   /* screen_draw_shape */
void f_08862DF8(Cpu *c);   /* fighter_keep_on_screen */
void f_0887D990(Cpu *c);   /* object_set_animation_by_type */
void f_0887D9D0(Cpu *c);   /* jitter_once */
void f_08822F8C(Cpu *c);   /* object_make_small */
void f_0887684C(Cpu *c);   /* fighter_set_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */

static void text(Cpu *c, gaddr s, uint32_t font, uint32_t x, uint32_t y, uint32_t colour) {
    c->r[4] = s; c->r[5] = font; c->r[6] = x; c->r[7] = y; c->r[8] = colour;
    f_0880A6CC(c);
}
static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
/* The 0x354-byte record of a fighter's side (0x09519588 + n * 0x354), mult's lo / hi as the original. */
static gaddr side_record(Cpu *c, int32_t n) {
    int64_t m = (int64_t)n * 0x354;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    return 0x09519588u + (uint32_t)m;
}
/* 0x08ACCD54's byte for the screen record 0x09525DE8's +8. */
static uint32_t screen_kind(void) { return *B(0x08ACCD54u + (uint32_t)(int32_t)S8(*W(0x09525DE8u) + 8)); }

/* ---------------------------------------------------------------- objects */

/* (object): +16 and +108 move by the speeds at +96 / +98 (x 256), the speeds by +100 / +102; then the
   top of +108 is kept within 0 - 155. */
SFA_FUNC(088B4F70, move_96_wrap_108) {
    gaddr o = ARG(0), v = o + 0x60;
    *W(o + 16) += (uint32_t)(int32_t)S16(v) << 8;
    *H(v) = (uint16_t)(S16(v) + S16(v + 4));
    *W(o + 108) += (uint32_t)(int32_t)S16(v + 2) << 8;
    *H(v + 2) = (uint16_t)(S16(v + 2) + S16(v + 6));
    c->r[29] -= 0x20;
    c->r[5] = v;
    f_088B4FD4(c);
    c->r[29] += 0x20;
}

/* (screen): its step (+4) from 0x08A381F0, the object lists 1, 4 and 3, the frame's end, and the VS
   backdrop while GAME +4 is set. */
SFA_FUNC(088BBFA0, screen_step_lists_vs) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    gaddr fn = *W(0x08A381F0u + (uint32_t)*H(o + 4) * 4);
    c->r[5] = fn; c->r[6] = 0x08A381F0u;
    rt_call_indirect(c, fn);
    f_08837690(c);
    f_0883771C(c);
    f_08887AFC(c);
    f_0880B6CC(c);
    if (*H(GAME + 4) != 0) f_08819144(c);
    c->r[29] += 0x20;
}

/* (object): at its parent's (+124) place; type 0 moves 0xBB right and 0x60 up on screens whose kind
   (0x08ACCD54) is not 1. */
SFA_FUNC(088BDB2C, place_at_parent_124) {
    gaddr o = ARG(0), p = *W(o + 124);
    int32_t x = S16(p + 18);
    if (*B(o + 92) == 0 && screen_kind() != 1) {
        *H(o + 6) = (uint16_t)(x + 0xBB);
        *H(o + 10) = (uint16_t)(S16(p + 22) - 0x60);
    } else {
        *H(o + 6) = (uint16_t)x;
        *H(o + 10) = (uint16_t)S16(p + 22);
    }
}

/* (): the pools and small objects reset, the four fighters' places at 0x092E22E8, the fight's tables
   cleared and the sprites reset. */
SFA_FUNC(08822144, fight_objects_reset) {
    c->r[29] -= 0x20;
    f_0882222C(c);
    f_088222C4(c);
    f_0882235C(c);
    gaddr l = 0x092E22E8u;
    *W(l) = 0x092E3EA0u;
    *W(l + 4) = 0x092E4338u;
    *W(l + 8) = 0x092E47D0u;
    *W(l + 12) = 0x092E4C68u;
    c->r[5] = 0x092E4C68u;
    c->r[4] = 0; f_088226E0(c);
    c->r[4] = 0; f_088224DC(c);
    c->r[29] += 0x20;
}

/* (): 0x09524A68 lowered by 0x08A0FA20[GAME +73] and then by a random 0x08A0FA28[0 - 31]. */
SFA_FUNC(088369C8, lower_4A68_random) {
    gaddr s16 = c->r[16], s17 = c->r[17], v = 0x09524A68u;
    c->r[29] -= 0x10;
    *H(v) = (uint16_t)(S16(v) - S8(0x08A0FA20u + (uint32_t)(int32_t)S8(GAME + 73)));
    c->r[16] = 0x09520000u;
    c->r[17] = (uint32_t)(int32_t)S16(v);
    f_08821368(c);
    *H(v) = (uint16_t)((int32_t)c->r[17] - S8(0x08A0FA28u + (c->r[2] & 0x1F)));
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): three texts from the list at 0x08A12244 (colour 9) and 0x089FE5B0 at (0x70, 0x14). */
SFA_FUNC(0883A6E0, texts_2244) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x30;
    gaddr e = 0x08A12244u;
    for (uint32_t i = 0; i < 3; i++, e += 12) {
        c->r[16] = i; c->r[17] = e;
        text(c, *W(e), *B(e + 4), (uint32_t)(int32_t)S16(e + 6), (uint32_t)(int32_t)S16(e + 8), 9);
    }
    c->r[16] = 3; c->r[17] = e;
    text(c, 0x089FE5B0u, 2, 0x70, 0x14, 3);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): moves (0x08855934) until battle state 6; then +4 moves on, x 0xC0, and a list-3 object
   (kind 5, type 11) owned by it. */
SFA_FUNC(088558C0, move_until_battle_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 44) != 6) {
        c->r[4] = o; c->r[5] = 6; c->r[6] = (uint32_t)(int32_t)S8(BATTLE + 44);
        f_08855934(c);
    } else {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *H(o + 18) = 0xC0;
        c->r[4] = 5; c->r[5] = 0xB;
        f_08823338(c);
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
        c->r[4] = c->r[2];
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +82 counts down (+5 moves on at 0); y is 0x8C (kind 10) or 0x74 plus a quarter of +82 + 3. */
SFA_FUNC(08856DA4, count_82_set_y) {
    gaddr o = ARG(0);
    int32_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    int32_t q = (int16_t)(((int32_t)(int16_t)(n + 3)) >> 2);
    *H(o + 22) = (uint16_t)(int16_t)(q + (*B(o + 3) == 10 ? 0x8C : 0x74));
}

/* (object, fighter): +7 moves on, the score for the record (0x0885847C), x -0x40 (kind 0) or 0x1C0,
   animation +3 + 16, drawn. */
SFA_FUNC(08857CFC, score_then_animate) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 139) = 7;
    *W(o + 148) = 6;
    c->r[4] = o; c->r[6] = 7;
    f_0885847C(c);
    uint32_t k = *B(o + 3);
    *H(o + 18) = k != 0 ? 0x1C0 : 0xFFC0;
    c->r[4] = o; c->r[5] = (uint32_t)(int32_t)(int16_t)(k + 0x10);
    f_08858418(c);
    c->r[4] = o;
    f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +82 counts down (+5 moves on at 0); y is 0x80 plus (or, with +10, minus) 0x08A24B6C[+82]. */
SFA_FUNC(08881644, count_82_wave_y) {
    gaddr o = ARG(0);
    int32_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    int32_t v = S8(0x08A24B6Cu + (uint32_t)(int32_t)S16(o + 82));
    *H(o + 22) = (uint16_t)(*B(o + 10) == 0 ? v + 0x80 : 0x80 - v);
    c->r[29] -= 0x20;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +5 moves on; place and speed (+18, +22, +96) from 0x08A2FBC0 by +3, the animation from
   0x08A2FA28 by +78. */
SFA_FUNC(088B0C94, place_by_3_anim_by_78) {
    gaddr o = ARG(0);
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    gaddr e = 0x08A2FBC0u + (uint32_t)(int32_t)(int16_t)(*B(o + 3) << 2) * 2;
    *H(o + 18) = (uint16_t)S16(e);
    *H(o + 22) = (uint16_t)S16(e + 2);
    *W(o + 96) = (uint32_t)(int32_t)S16(e + 4);
    uint32_t k4 = (uint32_t)(int32_t)S8(o + 78) << 2;
    c->r[29] -= 0x20;
    c->r[6] = 0x08A30000u; c->r[7] = k4;
    set_anim(c, o, *W(0x08A2FA28u + k4));
    c->r[29] += 0x20;
}

/* (object): +6 counts the frames while +10 is positive; when +10's low byte runs out (or +10 is 0)
   the camera layer 0x092E8800 moves by its speeds (+132, +136). */
SFA_FUNC(088BC070, count_then_scroll) {
    gaddr o = ARG(0);
    uint16_t h = *H(o + 10);
    if ((int16_t)h > 0) *H(o + 6) = (uint16_t)(*H(o + 6) + 1);
    if (h != 0) {
        uint8_t n = (uint8_t)(*B(o + 10) - 1);
        *B(o + 10) = n;
        if (n != 0) return;
        *H(o + 10) = 0;
    }
    gaddr l = 0x092E8800u;
    *W(l + 16) += *W(l + 132);
    *W(l + 20) += *W(l + 136);
}

/* (object): at its parent's (+88) place, (0x1B, 0x50) on screens of kind 1, (-0xA0, 0xB0) otherwise;
   drawn. */
SFA_FUNC(088BC740, place_by_screen_kind) {
    gaddr o = ARG(0), p = *W(o + 88);
    int one = screen_kind() == 1;
    *H(o + 18) = (uint16_t)(S16(p + 18) + (one ? 0x1B : -0xA0));
    *H(o + 22) = (uint16_t)(S16(p + 22) + (one ? 0x50 : 0xB0));
    c->r[29] -= 0x20;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +4 moves on and it is set up at (0xC0, 0x70) with the animation 0x08A3C87C. */
SFA_FUNC(088CED2C, setup_c0_70) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *B(o + 13) = 0; *B(o + 12) = 0;
    *H(o + 38) = 0;
    *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *H(o + 24) = 6;
    *H(o + 18) = 0xC0;
    *H(o + 22) = 0x70;
    c->r[29] -= 0x20;
    c->r[6] = 0x70;
    set_anim(c, o, *W(0x08A3C87Cu));
    c->r[29] += 0x20;
}

/* (object): y from 0x08A3CAF6 by 0x08A05898; shown (+93) with the option pulse while GAME +4 is set
   and +6 is 0. */
SFA_FUNC(088CFBFC, option_y_pulse) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 93) = 0;
    *H(o + 10) = (uint16_t)S16(0x08A3CAF6u + *W(0x08A05898u) * 2);
    if (*H(GAME + 4) != 0 && *H(GAME + 6) == 0) {
        c->r[4] = o;
        f_088BAC18(c);
        *B(o + 93) = 1;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): while its parent (+88) is shown, at its place (+0x4C, -16), with +84 from 0x08ACCB10 by the
   side's record when it is set. */
SFA_FUNC(088D0738, place_by_side_record) {
    gaddr o = ARG(0), p = *W(o + 88);
    if (*B(p + 1) == 0) return;
    gaddr r = side_record(c, S8(p + 78));
    if (S8(r) != 0) *H(o + 84) = (uint16_t)(*B(0x08ACCB10u + (uint32_t)(int32_t)S8(r + 1)) + 0x51);
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0x4C);
    *H(o + 22) = (uint16_t)(S16(p + 22) - 16);
}

/* (): the backdrop picture at (0x80, 0xA8) and three texts (0x08A01CB8, 0x08A01CD0, 0x08A01CE0). */
SFA_FUNC(089DEBE8, backdrop_three_texts) {
    c->r[29] -= 0x30;
    c->r[4] = 0x80; c->r[5] = 0xA8;
    f_0881CFD0(c);
    text(c, 0x08A01CB8u, 2, 0x18, 0x30, 3);
    text(c, 0x08A01CD0u, 3, 0x7C, 0x8A, 3);
    text(c, 0x08A01CE0u, 1, 0x68, 0xD0, 4);
    c->r[29] += 0x30;
}

/* (place, record): three shapes from 3 * the record's +3, 0x80 apart from (0, -16), alpha 7. */
SFA_FUNC(0880D5B4, three_shapes) {
    gaddr p = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    uint32_t shape = (uint32_t)*B(ARG(1) + 3) * 3;
    *H(p) = 0;
    *H(p + 2) = 0xFFF0;
    c->r[29] -= 0x20;
    c->r[16] = p;
    for (uint32_t i = 0; i < 3; i++, shape++) {
        c->r[17] = i; c->r[18] = shape;
        c->r[4] = p; c->r[5] = shape; c->r[6] = 7;
        f_0880E434(c);
        *H(p) = (uint16_t)(S16(p) + 0x80);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (object): +82 counts down; at 0 +6 moves on, battle state 2, and +82 starts again from 60 (or 120,
   with +7 = 15 and +79 = 5, when BATTLE +83 is set). Drawn. */
SFA_FUNC(088557FC, count_82_battle_2) {
    gaddr o = ARG(0);
    int32_t n = (int16_t)(S16(o + 82) - 1);
    if (n == 0) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(BATTLE + 44) = 2;
        if (S8(BATTLE + 83) != 0) {
            *B(o + 7) = 15;
            *B(o + 79) = 5;
            n = 0x3C;
        }
        n = (int16_t)(n + 0x3C);
    }
    *H(o + 82) = (uint16_t)n;
    c->r[29] -= 0x20;
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, script): script command: read on (0x0887A10C) when the fighter's +92 has +536 and its +228
   is at step 1; otherwise the block is left (0x08879C8C). */
SFA_FUNC(08878D30, fighter_script_cmd_536) {
    gaddr f = ARG(0), p = ARG(1), q = *W(f + 92);
    int read = 0;
    c->r[7] = p;
    if (*B(q + 536) != 0) {
        gaddr e = *W(q + 228);
        if (e != 0) { c->r[7] = 1; read = *B(e + 4) == 1; }
    }
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (read) f_0887A10C(c); else f_08879C8C(c);
    c->r[29] += 0x20;
}

/* (fighter): the script +510 from the routine table's +60 list (or +168 in a network battle with +815),
   +486 its first word, +524 the rest. */
SFA_FUNC(0887B418, fighter_script_start_510) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = f;
    *B(f + 550) = 0;
    f_088D5E44(c);
    gaddr t = *W(f + 416);
    gaddr list = *W(t + (c->r[2] != 0 && S8(f + 815) != 0 ? 168 : 60));
    int32_t i = S8(f + 510);
    *B(f + 511) = (uint8_t)i;
    gaddr s = *W(list + (uint32_t)i * 4);
    *H(f + 486) = (uint16_t)S16(s);
    *W(f + 524) = s + 2;
    *B(f + 565) = 0;
    *B(f + 501) = 0;
    c->r[4] = (uint32_t)(int32_t)S16(s); c->r[5] = (uint32_t)i; c->r[6] = (uint32_t)i * 4;
    c->r[29] += 0x10;
    c->r[16] = s16;
    RETURN(s + 2);
}

/* (fighter): its side's bit (+337) set in BATTLE +11, its routine +772 from 0x08A19344, and kept on the
   screen unless +772 is 9. */
SFA_FUNC(0885F050, fighter_routine_772) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    int32_t old = S8(BATTLE + 11);
    *B(BATTLE + 11) = (uint8_t)(old | (1 << (S8(f + 337) & 31)));
    gaddr fn = *W(0x08A19344u + (uint32_t)(int32_t)S8(f + 772) * 4);
    c->r[4] = f; c->r[5] = fn; c->r[6] = BATTLE; c->r[7] = (uint32_t)old;
    rt_call_indirect(c, fn);
    if (S8(f + 772) != 9) { c->r[4] = f; c->r[5] = 9; f_08862DF8(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): jittered (0x0887D9D0); picture 2 (colour -1) without +121, picture 1 while its fighter's
   (+88) +556 - +557 is under 17; then the animation by type. */
SFA_FUNC(0887D7F8, jitter_picture_by_556) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], f = *W(o + 88);
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = f;
    *B(o + 9) = 0;
    f_0887D9D0(c);
    if (S8(o + 121) == 0) { *B(o + 12) = 0xFF; *B(o + 13) = 2; }
    if (S8(f + 556) - S8(f + 557) < 17) { *B(o + 12) = 0xFF; *B(o + 13) = 1; }
    c->r[4] = o;
    f_0887D990(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter-like object): first time, frame 6 (0x0887684C) and its owner's +228 cleared; after, the next
   frame (0x088768F0), going to step 3 once +75 is negative. Drawn if on screen. */
SFA_FUNC(0892EE70, frame_6_then_next) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(*W(o + 88) + 228) = 0;
        c->r[4] = o; c->r[5] = 6;
        f_0887684C(c);
    } else {
        if (S8(o + 75) < 0) {
            *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        }
        c->r[4] = o;
        f_088768F0(c);
    }
    c->r[4] = o;
    f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (n): BATTLE +82 cleared and a small object (kind 1, type 1) set up with line n of 0x09358BE0 +304's
   list (128 bytes each). */
SFA_FUNC(089DE978, small_object_line_n) {
    gaddr s16 = c->r[16];
    uint32_t n = ARG(0);
    c->r[29] -= 0x20;
    c->r[16] = n;
    *B(BATTLE + 82) = 0;
    c->r[4] = 1; c->r[5] = 1;
    f_08822F8C(c);
    gaddr r = c->r[2];
    c->r[4] = r; c->r[5] = 0x98;
    if (r != 0) {
        *H(r + 26) = 0;
        *H(r + 30) = 0x98;
        *H(r + 32) = 1;
        *B(r + 42) = 0x1F;
        *B(r + 14) = 0;
        *W(r + 44) = *W(0x09358BE0u + 304) + (n << 7);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): is its fighter (+92) at a moment to act? Step 1.1 with +116 = 6, or step 1.0 standing (+73,
   +573 clear and the frame's first byte set). */
SFA_FUNC(0887BBC8, fighter_free_to_act) {
    gaddr q = *W(ARG(0) + 92);
    uint32_t k = *B(q + 4);
    if (k != 1) { RETURN(0); return; }
    if (*B(q + 5) == 1) { RETURN(*B(q + 116) == 6); return; }
    RETURN(*B(q + 5) == 0 && S8(q + 73) == 0 && S8(q + 573) == 0 && *B(*W(q + 296)) != 0);
}

/* (fighter, choice): the character (+338) put in the choice's list (+3 + count +1), and the first colour
   from +376 on that the character has not used yet (bits at 0x08B02718 + character, six in all)
   taken; it and +439 go to +9 / +12 of the list. */
SFA_FUNC(088A377C, take_free_colour) {
    gaddr f = ARG(0), p = ARG(1);
    int32_t ch = S8(f + 338);
    *B(p + (uint32_t)(int32_t)S8(p + 1) + 3) = (uint8_t)ch;
    gaddr m = 0x08B02718u + (uint32_t)ch;
    uint32_t bits = *B(m);
    int32_t i = S8(f + 376);
    uint32_t bit = 1u << (i & 31);
    for (int j = 0; j < 6 && (bits & bit) != 0; j++) {
        i++;
        if (i >= 6) i -= 6;
        bit = 1u << (i & 31);
    }
    *B(m) = (uint8_t)(bits | bit);
    gaddr q = p + (uint32_t)(int32_t)S8(p + 1);
    *B(q + 9) = (uint8_t)i;
    *B(q + 12) = (uint8_t)S8(f + 439);
    RETURN(0);
}

/* (object): first time, two list-3 objects (kind 7, types 9 and 10) or nothing more this frame; then its
   step (+9) from 0x08A2C6EC. */
SFA_FUNC(088AB0A4, make_9_10_then_step) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int ok = 1;
    if (*B(o + 92) == 0) {
        for (c->r[17] = 9; c->r[17] < 11; ) {
            c->r[4] = 7; c->r[5] = c->r[17] & 0xFF;
            f_08823338(c);
            c->r[17]++;
            if (c->r[2] == 0) { ok = 0; break; }
        }
        if (ok) *B(o + 92) = 1;
    }
    if (ok) {
        gaddr fn = *W(0x08A2C6ECu + (uint32_t)*B(o + 9) * 4);
        c->r[4] = o; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): kind 2 (+92) moves up by 8 to 240, kind 3 down by 8 to 80; +128 moves on once GAME +16's
   low byte is no longer +94. */
SFA_FUNC(088B265C, slide_until_16_changes) {
    gaddr o = ARG(0);
    uint32_t k = *B(o + 92);
    int32_t y = S16(o + 10);
    if (k == 2 && y >= 241) *H(o + 10) = (uint16_t)(y - 8);
    else if (k == 3 && y < 80) *H(o + 10) = (uint16_t)(y + 8);
    if ((S16(GAME + 16) & 0xFF) != S16(o + 94)) *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
}

/* (object): unless +72 is negative, at the point four back in its owner's (+88) trail ring; +72 = 1
   once the slot two after that is empty (0xFFFF). */
SFA_FUNC(088B4498, follow_trail) {
    gaddr o = ARG(0);
    if (S8(o + 72) < 0) return;
    gaddr p = *W(o + 88), ring = p + 0x70;
    uint32_t i = (uint32_t)(int32_t)(int16_t)(S16(p + 36) - 4) & 0xF;
    *H(o + 18) = (uint16_t)S16(ring + i * 2);
    *H(o + 22) = (uint16_t)S16(ring + i * 2 + 2);
    uint32_t j = (uint32_t)(int32_t)(int16_t)(i + 2) & 0xF;
    if (S16(ring + j * 2) == -1) *B(o + 72) = 1;
}

/* (object): +0x70 - +0x7F, +0x5C - +0x5F and +0x60 - +0x6B cleared. */
SFA_FUNC(088B54FC, clear_5c_7f) {
    gaddr o = ARG(0);
    for (uint32_t i = 0; i < 16; i++) *B(o + 0x70 + i) = 0;
    for (uint32_t i = 0; i < 4; i++) *B(o + 0x5C + i) = 0;
    for (uint32_t i = 0; i < 12; i++) *B(o + 0x60 + i) = 0;
}

/* (object): +4 moves on; a copy of its parent (+88) 0xB0 left and 12 down, picture - 2, +84 = 0x27. */
SFA_FUNC(088BCCF4, copy_parent_left) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1;
    *H(o + 24) = *H(p + 24);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *H(o + 18) = (uint16_t)(S16(p + 18) - 0xB0);
    *H(o + 22) = (uint16_t)(S16(p + 22) + 0xC);
    *B(o + 12) = (uint8_t)S8(p + 12);
    *H(o + 86) = 0x16;
    *B(o + 13) = (uint8_t)(*B(p + 13) - 2);
    *W(o + 164) = 1;
    *H(o + 84) = 0x27;
    *W(o + 148) = 3;
}

/* (object): +4 moves on and it is set up at (0xC0, 0x48), picture 0xD, with the animation 0x08A3C8C8. */
SFA_FUNC(088CEE04, setup_c0_48) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *H(o + 38) = 0;
    *B(o + 14) = 0; *B(o + 11) = 0; *B(o + 9) = 0;
    *B(o + 12) = 1;
    *B(o + 13) = 0xD;
    *H(o + 24) = 6;
    *H(o + 18) = 0xC0;
    *H(o + 22) = 0x48;
    c->r[29] -= 0x20;
    c->r[6] = 0xC0;
    set_anim(c, o, *W(0x08A3C8C8u));
    c->r[29] += 0x20;
}

/* (object, entry): while the entry's +2 is under the object's +119, its step (+5) from 0x08AB6688 with
   (object, entry) and drawn; otherwise step 2. */
SFA_FUNC(089D3174, step_while_under_119) {
    gaddr o = ARG(0), e = ARG(1), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S16(e + 2) < (int32_t)*B(o + 119)) {
        gaddr fn = *W(0x08AB6688u + (uint32_t)*B(o + 5) * 4);
        c->r[4] = o; c->r[5] = e; c->r[6] = fn;
        rt_call_indirect(c, fn);
        c->r[4] = o;
        f_088215C4(c);
    } else {
        *B(o + 4) = 2;
        c->r[4] = 2;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
