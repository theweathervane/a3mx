/* Small steps (0.132.0): 54 of the profile's ready functions of 17 - 24 instructions (two more are step
   dispatchers in steps.c). Objects moving, counting down and growing their speeds, fighters' script
   branches, the network session's lists, flags and ad-hoc calls. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "../decomp.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u   /* four fighters, 0x498 apart */
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define SESSION   0x09523820u   /* the network session's records */
#define NET       0x09523980u   /* the network task's state */
#define ADHOC     0x09523740u   /* the ad-hoc link's record (+4 its data, +16 flags, +20 state) */

void f_08876998(Cpu *c);   /* copy_five_from_128 */
void f_08855D80(Cpu *c);   /* intro_place */
void f_0886B468(Cpu *c);   /* battle_136_or_140 */
void f_0886DC3C(Cpu *c);   /* fighter_clear_644 */
void f_0880AA74(Cpu *c);   /* game_start_fade_out */
void f_088D7F18(Cpu *c);   /* net_session_failed */
void f_088E3604(Cpu *c);   /* net_session_down */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0882235C(Cpu *c);   /* small_objects_reset */
void f_088858D8(Cpu *c);   /* single_object_take */
void f_08836E60(Cpu *c);   /* add_to_09524 */
void f_08881D9C(Cpu *c);   /* setup_kind_at */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_08821044(Cpu *c);   /* object_move */
void f_08821AA8(Cpu *c);   /* object_next_frame */
void f_089F3470(Cpu *c);   /* game_memset */
void f_088215C4(Cpu *c);   /* object_draw */
void f_088219B0(Cpu *c);   /* object_animate */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_0884FEAC(Cpu *c);   /* move_by_96_100 */
void f_08880E74(Cpu *c);   /* move_down_wrapping */
void f_08821368(Cpu *c);   /* game_random */
void f_089DC074(Cpu *c);   /* ending_animation */
void f_089DA5DC(Cpu *c);   /* step_089DA5DC */

/* Calls f(object) with s0 holding the object, as the originals with a 0x20 frame do. */
static void call_keeping_s0(Cpu *c, GuestFunc f, gaddr o) {
    gaddr s16 = c->r[16];
    c->r[16] = o; c->r[4] = o;
    f(c);
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- objects moving */

/* (object): +5 = 2 above 64 (+18); moved by its speeds, then animated. */
SFA_FUNC(0884FDC4, move_then_animate_64) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S16(o + 18) < 64) *B(o + 5) = 2;
    c->r[4] = o; f_0884FEAC(c);
    c->r[4] = o; f_088219B0(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +20 moved by +100; at or above 0 its high half (+22) goes down by +77; then drawn. */
SFA_FUNC(0889D71C, rise_then_drop_by_77) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int32_t y = (int32_t)(*W(o + 20) + *W(o + 100));
    *W(o + 20) = (uint32_t)y;
    c->r[6] = (uint32_t)y;
    if (!(y > 0)) {
        c->r[5] = (uint32_t)(S16(o + 22) + *B(o + 77));
        *H(o + 22) = (uint16_t)c->r[5];
    }
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): +16 moved by +96 (back when +11, its facing, is 0), +96 by +104; once +96 is below 0 both
   stop (0) and the answer is 0, else +96. */
SFA_FUNC(0895E620, slow_to_stop) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if (*B(o + 11) == 0) *W(o + 16) -= v; else *W(o + 16) += v;
    v += *W(o + 104);
    *W(o + 96) = v;
    c->r[5] = v;
    if ((int32_t)v < 0) { *W(o + 96) = 0; *W(o + 104) = 0; RETURN(0); return; }
    RETURN(v);
}

/* (object): +16 moved by +96 (against facing as 0x0895E620), +20 by +100; both speeds grow by +104 /
   +108. Returns +100. */
SFA_FUNC(08961448, move_accelerating_facing) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if (*B(o + 11) == 0) *W(o + 16) -= v; else *W(o + 16) += v;
    *W(o + 96) = v + *W(o + 104);
    uint32_t vy = *W(o + 100);
    *W(o + 20) += vy;
    vy += *W(o + 108);
    *W(o + 100) = vy;
    RETURN(vy);
}

/* (object): +100 grows by +108 until it reaches +88 (towards +108's sign); then it is +88, and +108
   and +88 turn round. */
SFA_FUNC(089DBEC0, swing_100) {
    gaddr o = ARG(0);
    int32_t acc = (int32_t)*W(o + 108);
    int32_t v = (int32_t)(*W(o + 100) + (uint32_t)acc);
    *W(o + 100) = (uint32_t)v;
    int32_t limit = (int32_t)*W(o + 88);
    int32_t a = v, b = limit;
    if (acc < 0) { a = -v; b = -limit; }
    c->r[7] = o + 88; c->r[9] = (uint32_t)acc;
    if (a < b) { c->r[5] = (uint32_t)-acc; return; }
    *W(o + 100) = (uint32_t)limit;
    *W(o + 108) = (uint32_t)-acc;
    *W(o + 88) = (uint32_t)-limit;
}

/* (object): +18 up by 16, stopping at 160 with the next part (+5) and +82 = 16; then moved down
   (move_down_wrapping). */
SFA_FUNC(08880DDC, rise_to_160) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int16_t y = (int16_t)(S16(o + 18) - 16);
    if (y < 160) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 16;
        y = 0xA0;
    }
    *H(o + 18) = (uint16_t)y;
    f_08880E74(c);
    c->r[29] += 0x20;
}

/* +16 moved by +96 until it is below `limit`; there it stops, with the next part (+5) and +82 = 16;
   then moved down (move_down_wrapping). */
static void slide_left_to(Cpu *c, int32_t limit) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int32_t x = (int32_t)(*W(o + 16) + *W(o + 96));
    *W(o + 16) = (uint32_t)x;
    c->r[5] = (uint32_t)limit;
    if (x < limit) {
        *W(o + 16) = (uint32_t)limit;
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *W(o + 96) = 0;
        *H(o + 82) = 16;
        c->r[5] = 16;
    }
    f_08880E74(c);
    c->r[29] += 0x20;
}
SFA_FUNC(08881920, slide_left_to_88) { slide_left_to(c, 0x880000); }
SFA_FUNC(08881ACC, slide_left_to_78) { slide_left_to(c, 0x780000); }

/* (object): from intro stage 7 (BATTLE +44) the next step (+4); before it, +16 moved by +96 (which
   slows by one pixel a frame), the next step once +16 is left of -112. */
SFA_FUNC(08857254, slide_off_left) {
    gaddr o = ARG(0);
    if (!(S8(BATTLE + 44) < 7)) { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    uint32_t v = *W(o + 96);
    int32_t x = (int32_t)(v + *W(o + 16));
    *W(o + 96) = v - 0x10000;
    *W(o + 16) = (uint32_t)x;
    if (x < -112) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* (object): from intro stage 7 the next step (+4); before it, +82 counts down, the next part (+5) at 0. */
SFA_FUNC(08857200, count_82_until_stage_7) {
    gaddr o = ARG(0);
    if (!(S8(BATTLE + 44) < 7)) { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
}

/* (object): while +96 is not 0, it grows by +104 and moves +16, stopping (0) at 0xC00000 (192) when it
   gets there or past; then drawn. */
SFA_FUNC(0889D37C, slide_to_192) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t v = *W(o + 96);
    if (v != 0) {
        v += *W(o + 104);
        uint32_t x = v + *W(o + 16);
        *W(o + 16) = x;
        *W(o + 96) = v;
        c->r[5] = 0xC00000;
        if ((int32_t)(v ^ (x - 0xC00000u)) >= 0) { *W(o + 96) = 0; *W(o + 16) = 0xC00000; }
    }
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object, table): +96 the next of 128 places in the table: +11 from its bytes (+1024), +28 from its
   words, +18 / +22 from its half-words (+512, +768). */
SFA_FUNC(0887CAF4, next_of_128) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t n = (*W(o + 96) + 1) & 0x7F;
    *W(o + 96) = n;
    *B(o + 11) = *B(t + n + 1024);
    *W(o + 28) = *W(t + n * 4);
    *H(o + 18) = (uint16_t)S16(t + n * 2 + 512);
    *H(o + 22) = (uint16_t)S16(t + n * 2 + 768);
}

/* +82 counts down, the next part (+5) at 0; +18 is the table's +82-th half-word. */
static void count_82_y_from(Cpu *c, gaddr table) {
    gaddr o = ARG(0);
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[6] = table;
    *H(o + 18) = (uint16_t)S16(table + (uint32_t)(int32_t)S16(o + 82) * 2);
}
SFA_FUNC(08880E2C, count_82_y_from_24AA4) { count_82_y_from(c, 0x08A24AA4u); }
SFA_FUNC(08881978, count_82_y_from_24BA8) { count_82_y_from(c, 0x08A24BA8u); }
SFA_FUNC(08881B24, count_82_y_from_24BE4) { count_82_y_from(c, 0x08A24BE4u); }

/* (object): the next step (+4) unless GAME +69 is not negative and GAME +42 is set; then drawn. */
SFA_FUNC(08881768, step_unless_42_then_draw) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (S8(GAME + 69) < 0 || S8(GAME + 42) == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): animated; when +77 is set the next part (+5) and +13 one less. */
SFA_FUNC(08961958, animate_then_part_on_77) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    call_keeping_s0(c, f_088219B0, o);
    if (S8(o + 77) != 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *B(o + 13) = (uint8_t)(*B(o + 13) - 1);
    }
    c->r[29] += 0x20;
}

/* (object): placed for the intro (kind 6 at 0xC0, 0x80), the next step (+4), +73 and +82 cleared. */
SFA_FUNC(08856EB4, intro_place_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 6; c->r[6] = 0xC0; c->r[7] = 0x80;
    f_08855D80(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 73) = 0;
    *H(o + 82) = 0;
    c->r[4] = 0; c->r[5] = 0;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, source): the next step (+4); +24, +12 - +14, +86 and +164 copied from the source, +3 from its
   character (+338); +78 cleared. */
SFA_FUNC(08908C84, copy_from_fighter) {
    gaddr o = ARG(0), s = ARG(1);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(s + 24);
    *B(o + 3) = *B(s + 338);
    *B(o + 12) = *B(s + 12);
    *B(o + 13) = *B(s + 13);
    *B(o + 78) = 0;
    *B(o + 14) = *B(s + 14);
    *H(o + 86) = *H(s + 86);
    *W(o + 164) = *W(s + 164);
}

/* (object): the animation for its type (+3) and direction (+96 not negative) from 0x08A15488 /
   0x08A15388. */
SFA_FUNC(0884FB5C, animation_by_type_and_direction) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t i = *B(o + 3) * 2u + ((int32_t)*W(o + 96) >= 0);
    int32_t n = S8(0x08A15488u + i);
    c->r[6] = 0x08A15388u;
    c->r[5] = *W(0x08A15388u + (uint32_t)n * 4);
    f_0882196C(c);
    c->r[29] += 0x20;
}

/* (object): step 1, +86 = 4, the animation for its type (+3) from 0x08A165A8; drawn if on screen. */
SFA_FUNC(08853EE4, start_animation_by_type) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = 1;
    *H(o + 86) = 4;
    c->r[6] = 4;
    c->r[5] = *W(0x08A165A8u + *B(o + 3) * 4u);
    c->r[4] = o; f_0882196C(c);
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): the next step (+4), +100 = 1.0; set up at (0, 400) (y from +22), animation 0x08A299FC's first. */
SFA_FUNC(08880D10, start_at_400) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[7] = (uint32_t)(int32_t)S16(o + 22);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 100) = 0x10000;
    c->r[4] = o; c->r[5] = 0; c->r[6] = 0x190;
    f_08881D9C(c);
    c->r[5] = *W(0x08A299FCu);
    c->r[4] = o; f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): set up at (0, -200, 0x80), the next step (+4), +96 = 16.0, animation 0x08A299FC's fifth. */
SFA_FUNC(088810AC, start_at_minus_200) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = (uint32_t)-200; c->r[7] = 0x80;
    f_08881D9C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 96) = 0x100000;
    c->r[5] = *W(0x08A299FCu + 16);
    c->r[4] = o; f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, owner): a list-3 object of kind 13, type 6, with +10 / +14 = 0x100 / 0x180, +26 = 0x90 and
   the owner at +124. */
SFA_FUNC(089CD490, make_13_6) {
    gaddr owner = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = owner;
    c->r[4] = 0xD; c->r[5] = 6;
    f_08823338(c);
    gaddr o = c->r[2];
    c->r[4] = o; c->r[5] = 0x100;
    if (o != 0) {
        *H(o + 6) = 0;
        *H(o + 10) = 0x100;
        *H(o + 14) = 0x180;
        *H(o + 26) = 0x90;
        *W(o + 124) = owner;
        c->r[5] = 0x90; c->r[6] = 0x180;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4) once the record's +2 reaches +119, else drawn if on screen. */
SFA_FUNC(089DB634, step_at_119_else_draw) {
    gaddr o = ARG(0), r = ARG(1);
    int32_t limit = *B(o + 119);
    c->r[29] -= 0x20;
    c->r[6] = (uint32_t)limit;
    if (!(S16(r + 2) < limit)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else f_08821424(c);
    c->r[29] += 0x20;
}

/* The next step (+4), the animation for +116 from the ending's list, then the step 0x089DA5DC with the
   second argument. */
static void ending_next_animation(Cpu *c) {
    gaddr o = ARG(0), a1 = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = a1;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; c->r[5] = *B(o + 116);
    f_089DC074(c);
    c->r[4] = o; c->r[5] = a1;
    f_089DA5DC(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(089D9BB4, ending_next_animation_089D9BB4) { ending_next_animation(c); }
SFA_FUNC(089DA338, ending_next_animation_089DA338) { ending_next_animation(c); }

/* (object): moved; +82 counts down, and past 0 it starts again from +138 with the next frame. */
SFA_FUNC(089D4DD8, move_then_frame_by_82) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_08821044(c);
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    c->r[4] = (uint32_t)(int32_t)n;
    if (n < 0) {
        c->r[5] = (uint32_t)(int32_t)S8(o + 138);
        *H(o + 82) = (uint16_t)c->r[5];
        c->r[4] = o;
        f_08821AA8(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +28 = the 8-byte frame +78 / 8 of the list at +96; +74 - +77 its first four bytes; +152
   cleared. */
SFA_FUNC(0884E518, frame_by_78) {
    gaddr o = ARG(0);
    gaddr p = *W(o + 96) + (uint32_t)((S8(o + 78) >> 3) << 3);
    *W(o + 28) = p;
    *B(o + 74) = *B(p);
    *B(o + 75) = *B(p + 1);
    *B(o + 76) = *B(p + 2);
    *B(o + 77) = *B(p + 3);
    c->r[6] = (uint32_t)(int32_t)S8(p + 3);
    c->r[5] = 1;
    *W(o + 152) = 0;
}

/* (object, kind, x, y): +9 = kind, +24 = 7, +86 = 8, +11 - +15 and +38 cleared, at (x, y). */
SFA_FUNC(0889DB8C, place_kind) {
    gaddr o = ARG(0);
    int16_t x = (int16_t)ARG(2), y = (int16_t)ARG(3);
    *B(o + 9) = (uint8_t)ARG(1);
    *H(o + 24) = 7;
    *B(o + 13) = 0; *B(o + 12) = 0;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *W(o + 16) = (uint32_t)(int32_t)x << 16;
    *W(o + 20) = (uint32_t)(int32_t)y << 16;
    *H(o + 86) = 8;
    c->r[5] = 8; c->r[6] = (uint32_t)(int32_t)y << 16; c->r[7] = (uint32_t)(int32_t)y;
}

/* (object): +0, +98 and +100 one of the fourteen bytes at 0x08A3EAAD, at random. */
SFA_FUNC(088D63E8, random_of_14) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_08821368(c);
    uint32_t r = c->r[2];
    c->lo = r / 14; c->hi = r % 14;
    uint16_t v = *B(0x08A3EAADu + c->hi);
    c->r[5] = c->hi; c->r[4] = v;
    *H(o) = v; *H(o + 98) = v; *H(o + 100) = v;
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- records */

/* (record, object): the record's step 4, +4 - +22 (but +12) and +69 / +72 cleared, +43 = 1; the
   object's +37 / +81 = 1, +15 / +6 cleared, step 1. */
SFA_FUNC(0882B578, record_step_4) {
    gaddr r = ARG(0), o = ARG(1);
    *H(r + 2) = 4;
    *H(r + 22) = 0; *H(r + 20) = 0; *H(r + 18) = 0; *H(r + 16) = 0; *H(r + 14) = 0;
    *H(r + 10) = 0; *H(r + 8) = 0; *H(r + 6) = 0; *H(r + 4) = 0;
    *B(r + 72) = 0; *B(r + 69) = 0;
    *B(o + 37) = 1; *B(o + 81) = 1; *B(o + 15) = 0;
    *B(r + 43) = 1;
    *B(o + 6) = 0; *B(o + 4) = 1;
    c->r[6] = 1;
}

/* (record): the next step (+8) once none of the four fighters has +372 set. */
SFA_FUNC(0882507C, step_when_fighters_372_clear) {
    gaddr r = ARG(0);
    if ((*B(FIGHTER_3 + 372) | *B(FIGHTER_4 + 372) | *B(FIGHTER_1 + 372) | *B(FIGHTER_2 + 372)) == 0)
        *H(r + 8) = (uint16_t)(*H(r + 8) + 1);
}

/* (record): when the single object kind 14 is taken, 0x092E5100 +10 cleared, the next part (+6), +40 =
   -1, and the small objects reset. */
SFA_FUNC(0882804C, take_single_14) {
    gaddr r = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = r;
    c->r[4] = 0; c->r[5] = 0xE;
    f_088858D8(c);
    c->r[4] = 0x092E0000u;
    if (c->r[2] != 0) {
        *B(0x092E5100u + 10) = 0;
        uint16_t n = (uint16_t)(*H(r + 6) + 1);
        *H(r + 6) = n;
        *B(r + 40) = 0xFF;
        c->r[4] = n; c->r[5] = 0xFFFFFFFFu;
        f_0882235C(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (record): +0 counts to 100: at 100 +20 = 0x08A3EB14 and +24 cleared, else +8 cleared, +34 = 1 and +24
   one more; +0, +2 and +47 cleared either way. */
SFA_FUNC(088D6E1C, count_to_100) {
    gaddr r = ARG(0);
    if (*H(r) == 100) {
        *W(r + 20) = 0x08A3EB14u;
        *H(r + 24) = 0;
    } else {
        *W(r + 8) = 0;
        *B(r + 34) = 1;
        *H(r + 24) = (uint16_t)(*H(r + 24) + 1);
        c->r[6] = 1;
    }
    *H(r + 2) = 0; *H(r) = 0; *B(r + 47) = 0;
}

/* (fighter or record, a, b): a and b to fighter 1 when +337 has bit 1; else a = fighter 3 and b =
   fighter 4 (fighter 3 when fighter 4's +810 is 4). */
SFA_FUNC(088633BC, pair_for_side) {
    gaddr a = ARG(1), b = ARG(2);
    if (S8(ARG(0) + 337) & 2) {
        *W(b) = FIGHTER_1; *W(a) = FIGHTER_1;
        c->r[4] = FIGHTER_1;
        return;
    }
    c->r[4] = FIGHTER_4; c->r[5] = FIGHTER_3; c->r[7] = 4;
    *W(a) = FIGHTER_3;
    c->r[8] = (uint32_t)(int32_t)S8(FIGHTER_4 + 810);
    *W(b) = (int32_t)c->r[8] != 4 ? FIGHTER_4 : FIGHTER_3;
}

/* (): the 20-byte ad-hoc record at 0x09523720 cleared and its semaphore made (+8). */
SFA_FUNC(088DC3D4, adhoc_init_sema) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x30;
    c->r[16] = 0x09523720u;
    c->r[4] = 0x09523720u; c->r[5] = 0; c->r[6] = 0x14;
    f_089F3470(c);
    int32_t id = os_create_sema(c, 0x08A012C8u, 0, 1, 1, 0);
    if (id >= 0) *W(0x09523720u + 8) = (uint32_t)id;
    c->r[29] += 0x30;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): the script's entry for its type (+346) and health (+112) from 0x08A43C38's tables into
   +128, then copied on (copy_five_from_128). */
SFA_FUNC(088F16D4, script_by_health) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    gaddr t = *W(0x08A43C44u + (uint32_t)(int32_t)S8(f + 346) * 4 - 12);
    c->r[7] = 0x08A43C44u;
    c->r[6] = (uint32_t)((int32_t)S16(f + 112) << 2);
    c->r[5] = *W(t + c->r[6]);
    *W(f + 128) = c->r[5];
    f_08876998(c);
    c->r[29] += 0x20;
}

/* (fighter): +7 = 3 when it or its opponent (+92) is the CPU (+373) and the opponent is character 11;
   else the next part (+7). */
SFA_FUNC(08905718, part_3_against_11) {
    gaddr f = ARG(0), opp = *W(f + 92);
    c->r[6] = (uint32_t)(S8(f + 373) | S8(opp + 373));
    if (c->r[6] != 0) {
        c->r[6] = 0xB;
        if (S8(opp + 338) == 11) { *B(f + 7) = 3; c->r[5] = 3; return; }
    }
    c->r[5] = *B(f + 7) + 1;
    *B(f + 7) = (uint8_t)c->r[5];
}

/* (fighter): its record for this layout (BATTLE +56) and slot (+78): 0x09523CA8 + 0x480 n, n from
   0x08A22DC0. */
SFA_FUNC(0887CA88, fighter_layout_record) {
    gaddr f = ARG(0);
    int32_t n = S8(0x08A22DC0u + (uint32_t)((int32_t)S16(BATTLE + 56) * 4 + S8(f + 78)));
    c->r[6] = (uint32_t)n << 7;
    c->r[5] = 0x09520000u;
    c->r[4] = 0x09523CA8u;
    RETURN(0x09523CA8u + (uint32_t)n * 0x480);
}

/* (fighter): +0 = 1, +1 / +2 cleared, the CPU (+373) on; +420 = 0x90, +422 cleared; outside mode 16
   +376 = (ISM +386 + 1) * 2. */
SFA_FUNC(08824114, fighter_cpu_start) {
    gaddr f = ARG(0);
    *B(f) = 1;
    *B(f + 2) = 0; *B(f + 1) = 0;
    *B(f + 373) = 1;
    c->r[6] = (uint32_t)(int32_t)S8(GAME + 114);
    if ((int32_t)c->r[6] != 16) *B(f + 376) = (uint8_t)((S8(f + 386) + 1) << 1);
    c->r[5] = 0x90;
    *H(f + 420) = 0x90;
    *H(f + 422) = 0;
}

/* (fighter, flag): unless flag, the battle record's (battle_136_or_140) +356 = 2; then +696 / +270
   cleared, +81 from +564, and +644 cleared (fighter_clear_644). */
SFA_FUNC(0886C040, fighter_reset_81) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (ARG(1) == 0) {
        c->r[4] = f; f_0886B468(c);
        c->r[4] = 2;
        *B(c->r[2] + 356) = 2;
    }
    c->r[4] = (uint32_t)(int32_t)S8(f + 564);
    *B(f + 696) = 0;
    *B(f + 81) = (uint8_t)c->r[4];
    *B(f + 270) = 0;
    c->r[4] = f; f_0886DC3C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): +502 = 1; a human's button strength (+194) * 2 + 2 added (add_to_09524) unless GAME +57 is 3. */
SFA_FUNC(088370D8, fighter_add_strength) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    *B(f + 502) = 1;
    c->r[6] = (uint32_t)(int32_t)S8(GAME + 57); c->r[5] = 3;
    if ((int32_t)c->r[6] != 3) {
        c->r[5] = (uint32_t)(int32_t)S8(f + 373);
        if (c->r[5] == 0) {
            c->r[4] = (uint32_t)(int32_t)(int16_t)(S8(f + 194) * 2 + 2);
            f_08836E60(c);
        }
    }
    c->r[29] += 0x20;
}

/* (object, other): the other's +260 = 14 when the object's +7 is 10, else 12; +311 = 2, +312 = 8. */
SFA_FUNC(088EF8C8, set_260_by_7) {
    gaddr p = ARG(1);
    c->r[6] = 0xA;
    uint8_t v = *B(ARG(0) + 7) == 10 ? 0xE : 0xC;
    *B(p + 260) = v;
    *B(p + 311) = 2;
    *B(p + 312) = 8;
    c->r[4] = 2; c->r[6] = 8;
}

/* A script command that reads on (fighter_script_read) or leaves its block (fighter_script_leave_block),
   with the fighter and the script as given; `t7` is what the original leaves in t3. */
static void script_read_or_leave(Cpu *c, int leave, uint32_t t7) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    c->r[7] = t7;
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (leave) f_08879C8C(c); else f_0887A10C(c);
    c->r[29] += 0x20;
}
/* (fighter, script): leaves the block when the fighter's ISM (+386) is X (0), else reads on. */
SFA_FUNC(088798BC, script_if_not_x) {
    int32_t ism = S8(ARG(0) + 386);
    script_read_or_leave(c, ism == 0, (uint32_t)ism);
}
/* (fighter, script): leaves the block when the opponent's (+92) +696 is clear, else reads on. */
SFA_FUNC(08878C80, script_if_opponent_696) {
    script_read_or_leave(c, S8(*W(ARG(0) + 92) + 696) == 0, ARG(1));
}
/* (fighter, script): leaves the block when the ISM (+386) is V (1), else reads on. */
SFA_FUNC(08879A18, script_if_not_v) {
    int32_t ism = S8(ARG(0) + 386);
    c->r[8] = 1;
    script_read_or_leave(c, ism == 1, (uint32_t)ism);
}

/* ---------------------------------------------------------------- the network session */

/* (): 0x10 unless the session's +44 is 1; then 3 in states 2 / 3 (NET +42), else 8. */
SFA_FUNC(088D595C, net_state_3_8_or_10) {
    c->r[4] = NET; c->r[5] = 1;
    if (*B(NET + 44) != 1) { c->r[6] = *B(NET + 44); RETURN(0x10); return; }
    uint32_t state = *B(NET + 42);
    c->r[5] = state;
    if (state == 3 || state == 2) { c->r[4] = 3; c->r[6] = state; RETURN(3); return; }
    c->r[4] = 8; c->r[6] = 2;
    RETURN(8);
}

/* (): the link's data (+4) once its flag bit 0 (+16) is up, taking the bit down; 0 in states 1, 3, 5
   (+20) or without the flag. */
SFA_FUNC(088DF068, adhoc_take_data) {
    uint32_t state = *W(ADHOC + 20);
    c->r[4] = ADHOC;
    if (state == 1 || state == 3 || state == 5) { RETURN(0); return; }
    uint32_t flags = *W(ADHOC + 16);
    if (!(flags & 1)) { RETURN(0); return; }
    RETURN(*W(ADHOC + 4));
    *W(ADHOC + 16) = flags & ~1u;
}

/* (link): state 3 (+20); a flag bit 2 (+16) taken down gives state 4 and 1; a bit 0 taken down gives
   the link's data (+4). 0 when neither. */
SFA_FUNC(088DF46C, adhoc_take_flags) {
    gaddr l = ARG(0);
    uint32_t flags = *W(l + 16);
    *W(l + 20) = 3;
    RETURN(0);
    if (flags & 4) {
        flags &= ~4u;
        *W(l + 16) = flags;
        *W(l + 20) = 4;
        RETURN(1);
    }
    if (flags & 1) {
        flags &= ~1u;
        RETURN(*W(l + 4));
        *W(l + 16) = flags;
    }
}

/* (link): an ad-hoc group connected (named at +0xB0); +20 = 1 unless it failed. */
SFA_FUNC(088DF4BC, adhoc_connect) {
    gaddr l = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = l;
    int32_t r = os_net_adhocctl_connect(c, l + 0xB0);
    c->r[4] = 1;
    if (r >= 0) *W(l + 20) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (link): its matching (+8) stopped and deleted if there is one, the matching library ended, +8 = -1. */
SFA_FUNC(088DF0B8, adhoc_matching_end) {
    gaddr l = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = l; c->r[17] = 0xFFFFFFFFu;
    int32_t id = (int32_t)*W(l + 8);
    if (id > 0) {
        os_net_adhoc_matching_stop(c, id);
        os_net_adhoc_matching_delete(c, (int32_t)*W(l + 8));
        *W(l + 8) = 0xFFFFFFFFu;
    }
    os_net_adhoc_matching_term(c);
    *W(l + 8) = 0xFFFFFFFFu;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): while the session is up, the next step (+4), starting the fade out first unless the session
   failed. */
SFA_FUNC(088E9CE8, step_while_session_up) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_088E3604(c);
    if (c->r[2] != 0) {
        f_088D7F18(c);
        if (c->r[2] == 0) f_0880AA74(c);
        c->r[4] = (uint32_t)*H(o + 4) + 1;
        *H(o + 4) = (uint16_t)c->r[4];
    }
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (id): the session member with this id (+228; the first is the session itself, then its list +232),
   as the address of its record (+4); the list's last when none has it, 0 when the list is empty. */
SFA_FUNC(088DD23C, session_member) {
    gaddr p = SESSION;
    if (*W(SESSION + 228) != ARG(0)) {
        p = *W(SESSION + 232);
        while (p != 0 && *W(p + 228) != ARG(0)) {
            gaddr next = *W(p);
            if (next == 0) break;
            p = next;
        }
    }
    c->r[5] = p;
    RETURN(p != 0 ? p + 4 : 0);
}
