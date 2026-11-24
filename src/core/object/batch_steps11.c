/* Small steps (0.140.0): the last 3 ready functions of 9 - 40 instructions and the other 18 of 41 - 60.
   Objects started from a fighter and following its picture, fighters' tables and their followers, CPU
   levels by layout, a fighter turning toward the nearest, the highest of three heights, the frame's
   run by screen, the network session's members, records and thread, and its message box. Offsets stay
   numbers until something shows what they are. What the original leaves in a0 - t1 at a call is set
   as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER0  0x092E3EA0u
#define FIGHTER1  0x092E4338u
#define FIGHTER2  0x092E47D0u
#define FIGHTER3  0x092E4C68u
#define SESSION   0x09523820u   /* the network session's records (0x09523920 its state) */
#define NET       0x09523980u
#define NET_THREAD 0x09523740u  /* the network thread's record: +0 thread, +4 result, +188 / +189 flags */
#define GROWING   0x09524A68u   /* s16 kept in 0 - 255, growing on a timer */
#define SCREENS   0x08A3F310u   /* the screens' routines by GAME +4 */

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_088F5B9C(Cpu *c);   /* follow_picture_43f8c */
void f_088FC514(Cpu *c);   /* follow_picture_45700 */
void f_08933420(Cpu *c);   /* follow_picture_6c0f8 */
void f_088DD8AC(Cpu *c);   /* record_release_next */
void f_088DD81C(Cpu *c);   /* net_clear_09523C20 */
void f_088D5E7C(Cpu *c);   /* net_waits_for_host */
void f_0881EB14(Cpu *c);   /* camera_height_by_pair */
void f_088632C4(Cpu *c);   /* fighters_both_in_step_6 */
void f_0886C040(Cpu *c);   /* fighter_reset_81 */
void f_0880A7B0(Cpu *c);   /* text_from_list_3cc0 */
void f_088E3B8C(Cpu *c);   /* net_message_panel */
void f_088E4044(Cpu *c);   /* text_kind_record */
void f_088E40D8(Cpu *c);   /* game_text_8 */
void f_0880B6CC(Cpu *c);   /* game_end_of_frame */
void f_0881DF10(Cpu *c);   /* game_fight_effects */
void f_08837690(Cpu *c);   /* game_objects_1 */
void f_088379B8(Cpu *c);   /* game_objects_2 */
void f_08887AFC(Cpu *c);   /* game_objects_3 */
void f_08822F54(Cpu *c);   /* object_make_42 */
void f_0886EB48(Cpu *c);   /* fighter_unpack_lists */
void f_088215C4(Cpu *c);   /* object_draw */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_0887CA88(Cpu *c);   /* fighter_layout_record */
void f_0887CAF4(Cpu *c);   /* next_of_128 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_0887684C(Cpu *c);   /* fighter_set_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_08876998(Cpu *c);   /* copy_five_from_128 */
void f_089A5490(Cpu *c);   /* step_089A5490 */
void f_08821384(Cpu *c);   /* off_screen_routine_3 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */

/* ---------------------------------------------------------------- objects following a fighter */

/* An object started from a fighter: running, its +24, character, +12 - +14, picture 0; then `follow`. */
static void start_following(Cpu *c, void (*follow)(Cpu *)) {
    gaddr o = ARG(0), f = ARG(1);
    c->r[29] -= 0x20;
    *B(o + 4) = 1;
    *H(o + 24) = *H(f + 24);
    *B(o + 3) = (uint8_t)S8(f + 338);
    *B(o + 12) = (uint8_t)S8(f + 12);
    *B(o + 13) = *B(f + 13);
    *B(o + 78) = 0;
    *B(o + 14) = *B(f + 14);
    c->r[6] = *B(f + 14);
    follow(c);
    c->r[29] += 0x20;
}

/* (object, fighter): start_following with follow_picture_43f8c. */
SFA_FUNC(088F5B50, start_following_43f8c) { start_following(c, f_088F5B9C); }
/* (object, fighter): start_following with follow_picture_45700. */
SFA_FUNC(088FC4C8, start_following_45700) { start_following(c, f_088FC514); }
/* (object, fighter): start_following with follow_picture_6c0f8. */
SFA_FUNC(089333D4, start_following_6c0f8) { start_following(c, f_08933420); }

/* (object): placed on its owner (+88): running, layer 1, +72 set, picture by the owner's character,
   the owner's +24, +11 - +14, +38, place and +86 / +164; +12 = 1, +13 = 0x19; moved by +100 (turned
   round when facing right, kept at 0x08B03F94) and +108; animation 0x08A518E8. */
SFA_FUNC(08908DF4, placed_on_owner_offset) {
    gaddr o = ARG(0), own = *W(o + 88);
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 72) = 1; *B(o + 9) = 1;
    *B(o + 78) = (uint8_t)S8(own + 338);
    *H(o + 24) = *H(own + 24);
    *B(o + 13) = *B(own + 13);
    *B(o + 14) = *B(own + 14);
    *B(o + 11) = *B(own + 11);
    *H(o + 38) = (uint16_t)S16(own + 38);
    *H(o + 18) = (uint16_t)S16(own + 18);
    *B(o + 12) = 1;
    *H(o + 22) = (uint16_t)S16(own + 22);
    *B(o + 13) = 0x19;
    *H(0x08B03F94u) = (uint16_t)*W(o + 100);
    int32_t d = S16(0x08B03F94u);
    if (*B(o + 11) != 0) { *H(0x08B03F94u) = (uint16_t)(0 - d); d = S16(0x08B03F94u); }
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
    *H(o + 22) = (uint16_t)(S16(o + 22) + *W(o + 108));
    *H(o + 86) = *H(own + 86);
    *W(o + 164) = *W(own + 164);
    c->r[4] = o; c->r[5] = *W(0x08A518E8u); f_0882196C(c);
    c->r[29] += 0x20;
}

/* (object): placed on its owner (+88) at the owner's +132 height, layer 1, +72 set, the owner's +24,
   +11 - +14, +38; type 0 animation 0x08A84534; else facing turned, 0x80 further on the side it now
   faces, 0x20 lower, +13 + 4 and animation 0x08A84538. */
SFA_FUNC(0896162C, placed_on_owner_height) {
    gaddr o = ARG(0), own = *W(o + 88);
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 72) = 1;
    *H(o + 24) = *H(own + 24);
    *B(o + 12) = (uint8_t)S8(own + 12);
    *B(o + 13) = *B(own + 13);
    *B(o + 11) = *B(own + 11);
    *B(o + 14) = *B(own + 14);
    *H(o + 38) = (uint16_t)S16(own + 38);
    *H(o + 18) = (uint16_t)S16(own + 18);
    *H(o + 22) = (uint16_t)S16(own + 132);
    *B(o + 9) = 1;
    c->r[6] = 1;
    if (*B(o + 3) == 0) c->r[5] = *W(0x08A84534u);
    else {
        *B(o + 13) = (uint8_t)(*B(o + 13) + 4);
        uint8_t face = (uint8_t)(*B(o + 11) ^ 1);
        *B(o + 11) = face;
        *H(o + 18) = (uint16_t)(S16(o + 18) + (face != 0 ? -0x80 : 0x80));
        *H(o + 22) = (uint16_t)(S16(o + 22) + 0x20);
        c->r[5] = *W(0x08A84538u);
    }
    c->r[4] = o; f_0882196C(c);
    c->r[29] += 0x20;
}

/* (object): first a layout record taken and stepped (layer 2, +12 -1, the owner's +24); then, unless
   BATTLE +37, +32 counts up while +82 counts down; at its end +32 back to +34 and the next record
   taken, unless type 2 with the record's +0 bit 7, or another type whose owner word +4 is not
   0x010007xx / 0x0107xxxx (then +4 = 2). Drawn while the owner's +1 is set; freed from step 2. */
SFA_FUNC(088FC7F4, follower_records) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    gaddr own = *W(o + 88);
    c->r[16] = o; c->r[17] = own;
    uint32_t st = *B(o + 4);
    if (st == 0) {
        *B(o + 4) = 1; *B(o + 9) = 2; *B(o + 12) = 0xFF;
        *H(o + 24) = *H(own + 24);
        c->r[4] = o; f_0887CA88(c);
        c->r[4] = o; c->r[5] = c->r[2]; f_0887CAF4(c);
    } else if (st == 1) {
        int ended = 0;
        if (S8(BATTLE + 37) == 0) {
            int16_t n = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)n;
            if (n >= 0) *H(o + 32) = (uint16_t)(S16(o + 32) + 1);
            else {
                *H(o + 32) = (uint16_t)S16(o + 34);
                *H(o + 82) = 0;
                int next;
                if (*B(o + 3) == 2) next = (*B(*W(o + 100)) & 0x80) == 0;
                else {
                    uint32_t w = *W(own + 4);
                    next = (w & 0xFFFFFF00u) == 0x01000700u || (w & 0xFFFF0000u) == 0x01070000u;
                }
                if (next) {
                    c->r[4] = o; f_0887CA88(c);
                    c->r[4] = o; c->r[5] = c->r[2]; f_0887CAF4(c);
                } else { *B(o + 4) = 2; ended = 1; }
            }
        }
        if (!ended && *B(own + 1) != 0) { c->r[4] = o; f_088215C4(c); }
    } else {
        c->r[4] = o; f_088231C8(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): unless BATTLE +37 / +78: types 1 and 4 fall by +96 / +100 until 16 below their +132
   height, then stand there (routine 2, frame 0x10); others move by +96 / +100. Then the next frame
   and off-screen check. Drawn if on screen. */
SFA_FUNC(088FC034, fall_to_height) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if ((S8(BATTLE + 37) | S8(BATTLE + 78)) == 0) {
        uint32_t t = *B(o + 3);
        int moves = 1;
        if (t == 4 || t == 1) {
            int16_t y = S16(o + 132);
            if (!(y + 16 < S16(o + 22))) {
                moves = 0;
                *B(o + 4) = 2; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
                *H(o + 22) = (uint16_t)y;
                c->r[4] = o; c->r[5] = 0x10; f_0887684C(c);
            }
        }
        if (moves) {
            *W(o + 16) += *W(o + 96);
            *W(o + 20) += *W(o + 100);
        }
        c->r[4] = o; f_088768F0(c);
        c->r[4] = o; f_08821384(c);
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): turned one step (+78 by 8) toward the nearer of fighters 1 and 2: 0 / 8 / 16 / 24 as the
   fighter is past -128 / 0 / 128 of it. 0 when already facing that way, else 1 (a +78 above the
   wanted one stays). */
SFA_FUNC(0884E5A0, turn_toward_nearest) {
    gaddr o = ARG(0);
    int16_t x = S16(o + 18);
    int16_t d0 = (int16_t)(S16(FIGHTER0 + 18) - x), d1 = (int16_t)(S16(FIGHTER1 + 18) - x);
    if (d0 < 0) d0 = (int16_t)-d0;
    if (d1 < 0) d1 = (int16_t)-d1;
    gaddr near = d0 < d1 ? FIGHTER0 : FIGHTER1;
    int16_t diff = (int16_t)(S16(near + 18) - x);
    int32_t want = 0, at = -128;
    for (int i = 0; i < 3 && !(diff < at); i++) { want += 8; at += 0x80; }
    int32_t cur = S8(o + 78);
    if (cur == want) { RETURN(0); return; }
    *B(o + 78) = (uint8_t)(cur + (cur < want ? 8 : 0));
    RETURN(1);
}

/* (out, a, b, c): with a fighter's +579 set the camera height of the other pair (a skips first, then
   b, then c); with none, *out = the highest of their heights (+22). */
SFA_FUNC(0881EBCC, height_of_three) {
    gaddr a = ARG(0), b = ARG(1), d = ARG(2), out = ARG(3);
    c->r[29] -= 0x20;
    if (S8(a + 579) != 0) { c->r[4] = d; c->r[5] = b; c->r[6] = out; c->r[7] = out; f_0881EB14(c); }
    else if (S8(b + 579) != 0) { c->r[4] = a; c->r[5] = d; c->r[6] = out; c->r[7] = a; c->r[8] = (uint32_t)(int32_t)S8(b + 579); f_0881EB14(c); }
    else if (S8(d + 579) != 0) { c->r[4] = a; c->r[5] = b; c->r[6] = out; c->r[7] = a; c->r[8] = (uint32_t)(int32_t)S8(d + 579); f_0881EB14(c); }
    else {
        int32_t ya = S16(a + 22), yb = S16(b + 22), yd = S16(d + 22);
        int32_t lo = yd < yb ? yd : yb, hi = yd < yb ? yb : yd, v = lo;
        if (v < ya) v = ya;
        if (v < hi) v = hi;
        *H(out) = (uint16_t)v;
    }
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* A fighter's table (+416) filled with 39 words from `src`, its lists unpacked (`kind`, `lists`),
   +86 its side, +164 = 1, and an object of `kind` made (+40) that follows it with those and +12 -
   +14. */
static void fighter_start_table(Cpu *c, gaddr src, uint32_t kind, gaddr lists, int clear_549) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    gaddr dst = *W(f + 416);
    for (uint32_t i = 0; i < 39; i++) *W(dst + i * 4) = *W(src + i * 4);
    c->r[4] = f; c->r[5] = kind; c->r[6] = lists; f_0886EB48(c);
    if (clear_549) *B(f + 549) = 0;
    *H(f + 86) = (uint16_t)(int16_t)S8(f + 337);
    *W(f + 164) = 1;
    c->r[4] = kind; c->r[5] = 0; f_08822F54(c);
    gaddr o = c->r[2];
    if (o != 0) {
        *W(o + 88) = f;
        *H(o + 86) = (uint16_t)(int16_t)S8(f + 337);
        *W(o + 164) = *W(f + 164);
        *W(f + 40) = o;
        *B(o + 12) = (uint8_t)S8(f + 12);
        *B(o + 13) = *B(f + 13);
        *B(o + 14) = *B(f + 14);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): fighter_start_table from 0x08A45328, kind 1, lists 0x08A453C4. */
SFA_FUNC(088F2138, fighter_start_table_1) { fighter_start_table(c, 0x08A45328u, 1, 0x08A453C4u, 0); }
/* (fighter): fighter_start_table from 0x08A49504, kind 2, lists 0x08A495A0, +549 cleared. */
SFA_FUNC(088F601C, fighter_start_table_2) { fighter_start_table(c, 0x08A49504u, 2, 0x08A495A0u, 1); }
/* (fighter): fighter_start_table from 0x08A6D9A0, kind 0xD, lists 0x08A6DA3C. */
SFA_FUNC(0892F4A8, fighter_start_table_d) { fighter_start_table(c, 0x08A6D9A0u, 0xD, 0x08A6DA3Cu, 0); }

/* (fighter, _, arg): +125 / +126 / +778 cleared; unless both fighters are in step 6, it is reset
   (fighter_reset_81 with arg) and 1 returned when +251 is set (and BATTLE +84 without +36 doesn't
   apply) or none of BATTLE +13 / +14 and GAME +38 is. Else 0. */
SFA_FUNC(0886C08C, fighter_try_reset) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    uint32_t arg = ARG(2);
    c->r[29] -= 0x20;
    c->r[17] = f; c->r[16] = arg;
    *B(f + 125) = 0; *B(f + 126) = 0; *H(f + 778) = 0;
    f_088632C4(c);
    int reset = 0;
    if (c->r[2] == 0) {
        int halted = (S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38)) != 0;
        if (S8(BATTLE + 84) != 0 && S8(BATTLE + 36) == 0) reset = !halted;
        else reset = S8(f + 251) != 0 || !halted;
    }
    if (reset) {
        c->r[4] = f; c->r[5] = arg; f_0886C040(c);
        RETURN(1);
    } else RETURN(0);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): CPU levels (+375) by the fight's layout (BATTLE +56): 1 fighter 1 at 0x18, 3 and 4 by the
   growing value / 8; 2 fighter 2 at 0x18, 3 and 4 the same; 3 fighters 1 and 2 at 8, 3 by it. */
SFA_FUNC(0883680C, cpu_levels_by_layout) {
    int32_t l = S16(BATTLE + 56);
    uint8_t v = (uint8_t)(((int32_t)S16(GROWING) >> 3) & 0x1F);
    if (l == 1)      { *B(FIGHTER0 + 375) = 0x18; *B(FIGHTER3 + 375) = v; *B(FIGHTER2 + 375) = v; }
    else if (l == 2) { *B(FIGHTER1 + 375) = 0x18; *B(FIGHTER3 + 375) = v; *B(FIGHTER2 + 375) = v; }
    else if (l == 3) { *B(FIGHTER0 + 375) = 8; *B(FIGHTER1 + 375) = 8; *B(FIGHTER2 + 375) = v; }
}

/* (object, fighter): rows of 6 bytes at 0x08A297EC set from the fighter's character (0x08A2AC6C
   word: low byte the step, bit 15 lifts the second of each pair by 8; 0x08ACCABC byte + 0xF4 the
   value): heights from -256, 0x28 then the step in turn with the value on every second row, up to the
   byte + 0x11C, eleven at most; the rest at -256. +77 = step + 0x28. */
SFA_FUNC(0889D830, rows_by_character) {
    gaddr o = ARG(0), f = ARG(1);
    int32_t ch = S8(f + 338);
    uint32_t h = *H(0x08A2AC6Cu + (uint32_t)ch * 2), up = h & 0x8000, step = h & 0xFF;
    uint32_t top = *B(0x08ACCABCu + (uint32_t)ch) + 0xF4;
    *B(o + 77) = (uint8_t)(step + 0x28);
    int32_t lim = (int32_t)top + 0x28, y = -256;
    gaddr p = 0x08A297ECu;
    uint32_t k = 0xB, n = 0xA;
    int tog = 0xFF;
    if (!(lim < -256)) {
        for (;;) {
            if (tog) { y += 0x28; *H(p + 2) = (uint16_t)y; }
            else {
                y += (int32_t)step;
                *H(p + 2) = (uint16_t)y;
                if (up) *H(p + 2) = (uint16_t)(S16(p + 2) - 8);
                *H(p + 4) = (uint16_t)top;
            }
            k = n; n = (k - 1) & 0xFFFF; p += 6;
            if (k == 0) break;
            if (lim < y) break;
            tog ^= 0xFF;
        }
    }
    if (k != 0) do { k = n; *H(p + 2) = 0xFF00; n = (k - 1) & 0xFFFF; p += 6; } while (k != 0);
    RETURN(top);
}

/* (object): a fighter-like object's start: running, layer 0, +344 = -1, its owner's +24, its lists
   0x08A9C3C8 / 0x08A9C3D8 / 0x08A9C524 / 0x08A9C42C, five copied from +128; speed by +346 from
   0x08A9D770 (turned round facing left) and 0x60 to the side it faces, 0x44 lower; frame +346, then
   its step. */
SFA_FUNC(089A53A8, start_from_owner_346) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    gaddr own = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(own + 24);
    *W(o + 352) = 0x08A9C3C8u;
    *W(o + 356) = 0x08A9C3D8u;
    *W(o + 128) = 0x08A9C524u;
    *W(o + 200) = 0x08A9C42Cu;
    c->r[4] = o; f_08876998(c);
    int32_t k = S8(o + 346);
    *B(o + 73) = 0;
    *W(o + 100) = 0;
    uint32_t v = *W(0x08A9D770u + (uint32_t)k * 4);
    if (*B(o + 11) != 0) { *W(o + 96) = v; *H(o + 18) = (uint16_t)(S16(o + 18) + 0x60); }
    else { *W(o + 96) = 0u - v; *H(o + 18) = (uint16_t)(S16(o + 18) - 0x60); }
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x44);
    c->r[4] = o; c->r[5] = (uint32_t)k & 0xFFFF; f_0887684C(c);
    c->r[4] = o; f_089A5490(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- the frame */

/* (game, battle): a frame by screen (+4, routines at 0x08A3F310): in mode 6 on screen 0 only that
   routine (game, battle); else, unless paused (+145) or the battle's +23 skips it, the screen's routine
   and the three object lists; then the fight's effects (not in mode 6) and the end of the frame. */
SFA_FUNC(088E9DD8, frame_by_screen) {
    gaddr g = ARG(0), bt = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = g; c->r[17] = 6;
    uint32_t scr = *H(g + 4);
    if (S8(g + 114) == 6 && scr == 0) {
        c->r[4] = g; c->r[5] = bt; c->r[6] = bt;
        rt_call_indirect(c, *W(SCREENS));
    } else {
        if (S8(g + 145) == 0 && S8(bt + 23) == 0) {
            c->r[4] = g; c->r[5] = 0;
            rt_call_indirect(c, *W(SCREENS + *H(g + 4) * 4));
            f_08837690(c);
            f_088379B8(c);
            f_08887AFC(c);
        }
        if (S8(g + 114) != 6) f_0881DF10(c);
        f_0880B6CC(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---------------------------------------------------------------- the network session */

/* (): every released record taken off 0x08B02EA8, 0x09523C20 cleared; under the lock at 0x09523728
   +148 cleared down the list at 0x09523720, then down the session's members (+232) and the session. */
SFA_FUNC(088DD8F4, net_records_release_all) {
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    gaddr l = 0x09523720u;
    c->r[17] = l; c->r[16] = SESSION; c->r[18] = 0x08B00000u;
    do { c->r[4] = *W(0x08B02EA8u); c->r[5] = 0; f_088DD8AC(c); } while (c->r[2] != 0);
    f_088DD81C(c);
    if (*W(l + 8) != 0) os_wait_sema(c, (int32_t)*W(l + 8), 1, 0);
    for (gaddr p = *W(l); p != 0; p = *W(p)) *W(p + 148) = 0;
    if (*W(l + 8) != 0) os_signal_sema(c, (int32_t)*W(l + 8), 1);
    for (gaddr p = *W(SESSION + 232); p != 0; p = *W(p)) *W(p + 148) = 0;
    *W(SESSION + 148) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (count): *count down by one; the session for 0 and below (0 returned at exactly 0); while waiting
   for the host the member whose +228 is NET +12 (the session's own first, else down +232's list, the
   last if none); else the member at index count - 1 down +232's list (0 past its end). */
SFA_FUNC(088DD418, session_member_by_count) {
    gaddr p = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = p; c->r[17] = SESSION;
    int32_t n = (int32_t)*W(p);
    uint32_t r = 0;
    if (n <= 0) {
        if (n != 0) { r = SESSION; *W(p) = (uint32_t)(n - 1); }
        c->r[4] = (uint32_t)(n - 1);
    } else {
        f_088D5E7C(c);
        int32_t k = (int32_t)*W(p) - 1;
        if (c->r[2] != 0) {
            uint32_t want = *W(NET + 12);
            if (*W(SESSION + 228) == want) r = SESSION;
            else {
                r = *W(SESSION + 232);
                if (r != 0) while (*W(r + 228) != want && *W(r) != 0) r = *W(r);
            }
            *W(p) = (uint32_t)k;
        } else {
            *W(p) = (uint32_t)k;
            r = *W(SESSION + 232);
            if (k > 0) {
                for (;;) {
                    if (r == 0) break;
                    k--; r = *W(r);
                    if (k <= 0) break;
                }
            }
        }
        c->r[4] = (uint32_t)k;
    }
    RETURN(r);
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (box): the network message box: its text record (+8, kind 8 from +0 - 1, unless +0 is 0x100) made
   when first shown or missing; with +24 its two texts (0x08A013C8 at 96, 152 into +12; list text 0 at
   144, 152 into +16, colour 8) made when first shown or +12 is missing; then the panel shown by +4,
   and +25 set. */
SFA_FUNC(088E3CD8, net_message_box) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x30;
    c->r[16] = o;
    if (S8(o + 25) == 0 || *W(o + 8) == 0) {
        uint32_t v = *W(o);
        if (v != 0x100) {
            c->r[4] = (v - 1) & 0xFF; c->r[5] = 8; c->r[6] = 0xFFFFFFF0u;
            f_088E4044(c);
            *W(o + 8) = c->r[2];
        }
    }
    if (S8(o + 24) != 0 && (S8(o + 25) == 0 || *W(o + 12) == 0)) {
        c->r[4] = 0x08A013C8u; c->r[5] = 1; c->r[6] = 0x60; c->r[7] = 0x98; c->r[8] = 0xB;
        f_088E40D8(c);
        *W(o + 12) = c->r[2];
        c->r[4] = 0; c->r[5] = 0x90; c->r[6] = 0x98;
        f_0880A7B0(c);
        *W(o + 16) = c->r[2];
        if (c->r[2] != 0) *B(c->r[2] + 3) = 8;
    }
    c->r[4] = *W(o + 4) != 0;
    f_088E3B8C(c);
    *B(o + 25) = 1;
    c->r[29] += 0x30;
    c->r[16] = s16;
}

/* (): the network thread (0x08A01308, entry 0x088DEA78, priority 0x40, 0x2000 stack) made and started
   once (+189); 0 then, or already running. If either fails: the result into +4 when it is still 0,
   the thread's info freed and the thread deleted, +0 = -1, +189 cleared, the result returned. */
SFA_FUNC(088DECF4, net_thread_start) {
    gaddr t = NET_THREAD, s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x30;
    c->r[17] = t;
    int32_t r = *B(t + 189);
    if (r != 0) RETURN(0);
    else {
        *B(t + 189) = 1;
        r = os_create_thread(c, 0x08A01308u, 0x088DEA78u, 0x40, 0x2000, 0, 0);
        int failed = r < 0;
        if (!failed) {
            *W(t) = (uint32_t)r;
            *B(t + 188) = 1;
            r = os_start_thread(c, r, 0, 0);
            if (r < 0) { failed = 1; os_delete_thread(c, (int32_t)*W(t)); }
        }
        if (!failed) RETURN(0);
        else {
            if (*W(t + 4) == 0) *W(t + 4) = (uint32_t)r;
            if ((int32_t)*W(t) >= 0) {
                c->r[4] = *W(t); hle_sceNetFreeThreadinfo(c);
                os_delete_thread(c, (int32_t)*W(t));
            }
            *W(t) = 0xFFFFFFFFu;
            *B(t + 189) = 0;
            RETURN((uint32_t)r);
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17;
}
