/* Small steps (0.136.0): the last 23 of the profile's ready functions of 25 - 40 instructions. Pool 7's
   objects run, objects placed against the camera and sliding to a stop, box settings, the pause values,
   fade levels for a layout; fighters' button strength, target pairs and trails; the ad-hoc link's
   thread end and members; the network menu's texts. Offsets stay numbers until something shows what
   they are. What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define POOL_7    0x09513F70u   /* 16 objects of 0xC0 */
#define NET       0x09523980u   /* the network task's state */
#define ADHOC     0x09523740u   /* the ad-hoc link's record */
#define NET_MENU  0x08A3EA34u   /* the network menu's rows, 11 bytes each */
#define NET_OPT   0x08B03F8Au   /* the network options' values */

void f_088215C4(Cpu *c);   /* object_draw */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_088219B0(Cpu *c);   /* object_animate */
void f_08961A6C(Cpu *c);   /* move_accelerating */
void f_088B9CEC(Cpu *c);   /* game_pause_value_ud */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_0887684C(Cpu *c);   /* fighter_set_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_089DBF60(Cpu *c);   /* speeds_to_fixed */
void f_089DC074(Cpu *c);   /* ending_animation */
void f_089DA5DC(Cpu *c);   /* step_089DA5DC */
void f_0886DC3C(Cpu *c);   /* fighter_clear_644 */
void f_089F33E4(Cpu *c);   /* game_memcmp */
void f_0882D95C(Cpu *c);   /* fighter_record_trail */
void f_0882DE00(Cpu *c);   /* fade_slots */

/* ---------------------------------------------------------------- objects */

/* (): each object of pool 7 in use run by its kind's (+2) handler from 0x08A3CB30; GAME +66 counts
   the sixteen down as it goes. */
SFA_FUNC(088D08E8, pool_7_run) {
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[17] = GAME; c->r[18] = POOL_7; c->r[16] = 0x08A3CB30u;
    *B(GAME + 66) = 0x10;
    gaddr o = POOL_7;
    for (;;) {
        c->r[18] = o;
        if (*B(o) != 0) {
            uint32_t ra = c->r[31];
            gaddr h = *W(0x08A3CB30u + *B(o + 2) * 4u);
            c->r[4] = o; c->r[5] = h; c->r[31] = 0x088D0940u;
            rt_call_indirect(c, h);
            c->r[31] = ra;
        }
        int8_t n = (int8_t)(S8(GAME + 66) - 1);
        *B(GAME + 66) = (uint8_t)n;
        c->r[4] = (uint32_t)(int32_t)n;
        o += 0xC0;
        if (n == 0) break;
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (object): +82 counts down, past 0 the next step (+4); before it moved with growing speeds and
   animated, drawn on alternate frames by its owner's side (GAME +59 + owner +337). */
SFA_FUNC(08961884, move_then_flicker) {
    gaddr o = ARG(0), owner = *W(o + 88), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = owner;
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        c->r[4] = o; f_08961A6C(c);
        c->r[4] = o; f_088219B0(c);
        c->r[4] = (uint32_t)(S8(GAME + 59) + S8(owner + 337)) & 1;
        if (c->r[4] == 0) { c->r[4] = o; f_088215C4(c); }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object, k): +16 / +20 at the camera's (0x092E88A0) scroll times k / 16 (x with 0x092E5F78 +112) plus
   its two offsets each (+112 / +120, +116 / +124). */
SFA_FUNC(0884FACC, place_by_camera) {
    gaddr o = ARG(0), cam = 0x092E88A0u;
    int32_t k = (int32_t)ARG(1);
    int32_t dx = (int32_t)(*W(cam + 8) - *W(cam + 32) + *W(0x092E5F78u + 112));
    int32_t m = (int32_t)((int64_t)k * dx);
    int32_t v = (m + (int32_t)((uint32_t)(m >> 4) >> 28)) >> 4;
    *W(o + 16) = (uint32_t)v + *W(o + 112) + *W(o + 120);
    int32_t dy = (int32_t)(*W(cam + 12) - *W(cam + 36));
    int64_t m2 = (int64_t)k * dy;
    c->lo = (uint32_t)m2; c->hi = (uint32_t)(m2 >> 32);
    int32_t lo = (int32_t)c->lo;
    int32_t w = (lo + (int32_t)((uint32_t)(lo >> 4) >> 28)) >> 4;
    c->r[5] = (uint32_t)w + *W(o + 116);
    c->r[6] = *W(o + 124);
    c->r[7] = (uint32_t)w; c->r[8] = (uint32_t)(lo >> 4) >> 28;
    *W(o + 20) = c->r[5] + c->r[6];
    c->r[9] = *W(o + 120); c->r[10] = (uint32_t)v; c->r[11] = (uint32_t)(m >> 4) >> 28;
}

/* (object): the next step (+4) unless GAME +69 is not negative and GAME +42 is set; then, while +96 is
   not 0, +16 moves by twice it, stopping (+96 = 0) at 0xB80000 (184); animated and drawn. */
SFA_FUNC(08881104, slide_to_184) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(GAME + 69) < 0 || S8(GAME + 42) == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        uint32_t v = *W(o + 96);
        if (v != 0) {
            int32_t x = (int32_t)(*W(o + 16) + v + v);
            *W(o + 16) = (uint32_t)x;
            c->r[4] = 0xB80000;
            if (!(x < 0xB80000)) { *W(o + 16) = 0xB80000; *W(o + 96) = 0; }
        }
        c->r[4] = o; f_088219B0(c);
        c->r[4] = o; f_088215C4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +128 one more; a box's settings: +6 = 0xA0, +10 = 0x89, +14 = 0x40, +26 = 0x10, +12 / +24
   cleared, colours +36 - +45 = 0 and +48 - +51 = 0xC0, +1 / +3 / +56 cleared, +55 = 7, +136 = 0x80, +93 = 1. */
SFA_FUNC(0888A954, box_setup_c0) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *H(o + 6) = 0xA0; *H(o + 10) = 0x89; *H(o + 14) = 0x40; *H(o + 26) = 0x10;
    for (uint32_t k = 36; k <= 39; k++) *B(o + k) = 0;
    for (uint32_t k = 42; k <= 45; k++) *B(o + k) = 0;
    for (uint32_t k = 48; k <= 51; k++) *B(o + k) = 0xC0;
    *H(o + 24) = 0; *H(o + 12) = 0;
    *B(o + 56) = 0; *B(o + 3) = 0; *B(o + 1) = 0;
    *B(o + 55) = 7; *B(o + 136) = 0x80; *B(o + 93) = 1;
    c->r[5] = 1; c->r[6] = 0x80;
}

/* (-, which, values): the pause value `which` (0 - 2) moved by the pad (game_pause_value_ud, one of 11)
   from the three settings rows at 0x08E75BA0 (44 bytes apart). */
SFA_FUNC(088B932C, pause_values) {
    gaddr v = ARG(2), s16 = c->r[16];
    uint32_t which = ARG(1) & 0xFF;
    c->r[29] -= 0x20;
    c->r[16] = v; c->r[4] = which;
    if (which <= 2) {
        c->r[4] = 0x08E75BA0u + which * 0x2C;
        c->r[5] = (uint32_t)(int32_t)S8(v + which);
        c->r[6] = 0xB;
        f_088B9CEC(c);
        *B(v + which) = (uint8_t)c->r[2];
    } else c->r[5] = 2;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4), its speeds to fixed point, +88 = -(+100), the camera layer
   0x092E8940's +18 / +22 from 0x0951F950 +314 / +318, +16 / +20 cleared; the animation for +116 from the
   ending's list, then the step 0x089DA5DC with the record. */
SFA_FUNC(089D9A80, ending_start_with_camera) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = r;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; f_089DBF60(c);
    *W(o + 88) = 0u - *W(o + 100);
    *H(0x092E8940u + 18) = (uint16_t)S16(0x0951F950u + 314);
    *H(0x092E8940u + 22) = (uint16_t)S16(0x0951F950u + 318);
    *H(o + 20) = 0; *H(o + 16) = 0;
    c->r[6] = 0x092E8940u;
    c->r[4] = o; c->r[5] = *B(o + 116);
    f_089DC074(c);
    c->r[4] = o; c->r[5] = r;
    f_089DA5DC(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): the first time (+5 from 0) frame 16 for types (+3) 1 and 4, else 15; after that the next
   frame, back to step 3 once +75 is negative; drawn if on screen. */
SFA_FUNC(088FC37C, frames_by_type) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        uint32_t t = *B(o + 3);
        c->r[6] = t == 1 ? 1 : 4;
        c->r[4] = o; c->r[5] = (t == 1 || t == 4) ? 0x10 : 0xF;
        f_0887684C(c);
    } else {
        if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        c->r[4] = o; f_088768F0(c);
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (kind, x, y): the first free of four 20-byte text records at 0x092DF930 for the string of this kind
   (0x08A05834 +20's 128-byte rows: its first byte at +17, the rest at +12), at (x + 0x33, y + 0x68);
   +16 = 8. 0 when all are in use. */
SFA_FUNC(088E4044, text_kind_record) {
    uint32_t kind = ARG(0) & 0xFF;
    int16_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    gaddr p = 0x092DF930u;
    for (uint32_t i = 0; i < 4; i++, p += 0x14) {
        if (*B(p) != 0) continue;
        gaddr t = *W(0x08A05834u + 20) + kind * 128;
        *B(p + 17) = *B(t);
        *B(p) = 1; *B(p + 1) = 1;
        *B(p + 18) = 0;
        *B(p + 16) = 8;
        *H(p + 10) = (uint16_t)(y + 0x68);
        *W(p + 12) = t + 1;
        *H(p + 6) = (uint16_t)(x + 0x33);
        c->r[4] = t + 1; c->r[5] = (uint32_t)(int32_t)(x + 0x33); c->r[6] = (uint32_t)(int32_t)x; c->r[7] = 8; c->r[8] = *B(t);
        RETURN(p);
        return;
    }
    c->r[4] = 4; c->r[5] = (uint32_t)(int32_t)y; c->r[6] = (uint32_t)(int32_t)x; c->r[7] = kind;
    RETURN(0);
}

/* (object, a, b): both fighter 3 for the second side (+337 bit 1 clear); else a = fighter 1 and b =
   fighter 2, but while GAME +137 is set a fighter at step 1 / part 6 stands for both. */
SFA_FUNC(0886346C, pair_for_side_2) {
    gaddr a = ARG(1), b = ARG(2);
    if ((S8(ARG(0) + 337) & 2) == 0) { *W(b) = FIGHTER_3; *W(a) = FIGHTER_3; c->r[4] = FIGHTER_3; return; }
    *W(a) = FIGHTER_1; *W(b) = FIGHTER_2;
    c->r[8] = FIGHTER_1; c->r[7] = FIGHTER_2;
    c->r[4] = (uint32_t)(int32_t)S8(GAME + 137);
    if (c->r[4] == 0) return;
    c->r[4] = 1;
    if (*B(FIGHTER_1 + 4) == 1 && *B(FIGHTER_1 + 5) == 6) { *W(b) = FIGHTER_1; return; }
    c->r[6] = *B(FIGHTER_2 + 4);
    if (c->r[6] != 1) return;
    c->r[4] = *B(FIGHTER_2 + 5); c->r[6] = 6;
    if (c->r[4] != 6) return;
    *W(a) = FIGHTER_2;
}

/* (level): the fade levels for this layout (BATTLE +56, its routine from 0x08A0F1B4 with the level) and
   slots 0x38 - 0x3D, 0x15 - 0x18, 0x0F - 0x13, 0x1A - 0x1F; the rest (0x092E1220) and 0x08E75C44 = 0x60. */
SFA_FUNC(0882DE3C, fade_layout_60) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = 0x60;
    gaddr h = *W(0x08A0F1B4u + (uint32_t)((int32_t)S16(BATTLE + 56) * 4));
    uint32_t ra = c->r[31];
    c->r[4] = 0x60; c->r[5] = h; c->r[31] = 0x0882DE74u;
    rt_call_indirect(c, h);
    c->r[31] = ra;
    static const uint8_t runs[4][2] = { { 0x38, 6 }, { 0x15, 4 }, { 0x0F, 5 }, { 0x1A, 6 } };
    for (int i = 0; i < 4; i++) {
        c->r[4] = runs[i][0]; c->r[5] = runs[i][1]; c->r[6] = 0x60;
        f_0882DE00(c);
    }
    c->r[4] = 0x60; c->r[5] = 0x092E0000u;
    *B(0x092E1220u) = 0x60;
    c->r[4] = 0x08E70000u;
    *H(0x08E75C44u) = 0x60;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- the network */

/* (): 1 while the link is busy (+188); else its thread (+0) ended as 0x088DEC74 does, the calling
   thread's net info freed, and its data (+4) answered. With `clear_189`, +189 cleared with the thread. */
static void end_threads(Cpu *c, int clear_189) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = ADHOC;
    if (*B(ADHOC + 188) != 0) RETURN(1);
    else {
        if ((int32_t)*W(ADHOC) >= 0) {
            os_net_free_threadinfo(c, (int32_t)*W(ADHOC));
            os_wait_thread_end(c, (int32_t)*W(ADHOC), 0);
            os_terminate_thread(c, (int32_t)*W(ADHOC));
            os_delete_thread(c, (int32_t)*W(ADHOC));
            c->r[4] = 0xFFFFFFFFu;
            *W(ADHOC) = 0xFFFFFFFFu;
            if (clear_189) *B(ADHOC + 189) = 0;
        }
        os_net_free_threadinfo(c, os_get_thread_id(c));
        RETURN(*W(ADHOC + 4));
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(088DEFD8, adhoc_end_thread_and_own) { end_threads(c, 0); }
SFA_FUNC(088DEDE4, adhoc_end_thread_and_own_189) { end_threads(c, 1); }

/* (mac): the first record of the list at 0x09523720 whose six bytes at +4 are this address, looked for
   under its semaphore (+8) when there is one; 0 if none. */
SFA_FUNC(088DC6A0, adhoc_member_by_address) {
    gaddr mac = ARG(0);
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[18] = 0x09523720u; c->r[16] = mac;
    if (*W(0x09523720u + 8) != 0) os_wait_sema(c, (int32_t)*W(0x09523720u + 8), 1, 0);
    gaddr p = *W(0x09523720u);
    c->r[17] = p;
    while (p != 0) {
        c->r[4] = p + 4; c->r[5] = mac; c->r[6] = 6;
        f_089F33E4(c);
        if (c->r[2] == 0) break;
        p = *W(p);
        c->r[17] = p;
    }
    c->r[4] = *W(0x09523720u + 8);
    if (c->r[4] != 0) os_signal_sema(c, (int32_t)c->r[4], 1);
    RETURN(p);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (row): the network menu's row's byte (NET +47's row of 0x08A3EA34) as the option is set: +5 / +6
   with the option at 0x08B03F8F / 90 (0x08A01624 / 1628, 0x08A0161C / 1620). */
static uint32_t net_menu_byte(uint32_t k) { return *B(NET_MENU + (uint32_t)S8(NET + 47) * 11 + k); }

/* A text request for the network menu: the string, kind 1 at y 0xC0, the row's byte, colour 11. */
static void net_text(Cpu *c, gaddr str, uint32_t b) {
    c->r[4] = str; c->r[5] = 1; c->r[6] = 0xC0; c->r[7] = b; c->r[8] = 0xB;
    f_0880A6CC(c);
}

/* (): the network option 0's text (NET_OPT +0: 0 - 3, 0x08A015A4 - B0) by the row's byte +1. */
SFA_FUNC(088EC7D0, net_text_option_0) {
    c->r[29] -= 0x30;
    uint32_t b = net_menu_byte(1), v = *B(NET_OPT);
    c->r[5] = b; c->r[4] = v; c->r[6] = v < 2;
    if (v < 2) net_text(c, v == 0 ? 0x08A015A4u : 0x08A015A8u, b);
    else if (v < 3) net_text(c, 0x08A015ACu, b);
    else if (v < 4) net_text(c, 0x08A015B0u, b);
    c->r[29] += 0x30;
}

/* (): the network option 1's text (NET_OPT +1: 0 - 2, 0x08A015B4 - BC) by the row's byte +2. */
SFA_FUNC(088EC8DC, net_text_option_1) {
    c->r[29] -= 0x30;
    uint32_t b = net_menu_byte(2), v = *B(NET_OPT + 1);
    c->r[5] = b; c->r[4] = v; c->r[6] = v < 2;
    if (v == 0) net_text(c, 0x08A015B4u, b);
    else if (v < 2) net_text(c, 0x08A015B8u, b);
    else if (v < 3) net_text(c, 0x08A015BCu, b);
    c->r[29] += 0x30;
}

/* (): the network option 5's text (on 0x08A01624, off 0x08A01628) by the row's byte +5. */
SFA_FUNC(088ECFB8, net_text_option_5) {
    c->r[29] -= 0x30;
    uint32_t b = net_menu_byte(5);
    net_text(c, *B(NET_OPT + 5) != 0 ? 0x08A01624u : 0x08A01628u, b);
    c->r[29] += 0x30;
}

/* (): the network option 6's text (on 0x08A0161C, off 0x08A01620) by the row's byte +6. */
SFA_FUNC(088ED04C, net_text_option_6) {
    c->r[29] -= 0x30;
    uint32_t b = net_menu_byte(6);
    net_text(c, *B(NET_OPT + 6) != 0 ? 0x08A0161Cu : 0x08A01620u, b);
    c->r[29] += 0x30;
}

/* (object): the network menu's row (NET +47 on): +129 = 3 - 5 on screens 3 - 5 (NET +0), 0 from 6 up,
   else 2 with +130 = -1; with the menu off +128 one more. */
SFA_FUNC(088EB808, net_menu_row_129) {
    gaddr o = ARG(0);
    c->r[5] = NET;
    if (S8(NET + 47) == 0) { c->r[5] = *B(o + 128) + 1; *B(o + 128) = (uint8_t)c->r[5]; return; }
    uint32_t screen = *H(NET);
    if (!(screen < 6)) { *B(o + 129) = 0; return; }
    if (screen == 3 || screen == 4 || screen == 5) { c->r[5] = screen; *B(o + 129) = (uint8_t)screen; return; }
    *B(o + 129) = 2; *B(o + 130) = 0xFF;
    c->r[5] = 2; c->r[6] = 0xFF;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, kick): its button strength (+194, also +552) from the punch (or kick) buttons pressed or
   released: the heavy one gives 2 from +366 = 144, 1 from 96, else 0; the medium one 1 from 96, else 0;
   none 0. Then +644 cleared (fighter_clear_644). */
SFA_FUNC(0886D34C, fighter_strength_by_366) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    uint32_t b = (*H(f + 760) | *H(f + 762)) & 0xFFFF;
    c->r[8] = ARG(1);
    uint32_t heavy = ARG(1) == 0 ? b & 0x40 : b & 0x200, mid = ARG(1) == 0 ? b & 0x20 : b & 0x100;
    int32_t m = S16(f + 366);
    uint32_t s = 0;
    if (heavy) s = !(m < 144) ? 2 : !(m < 96) ? 1 : 0;
    else if (mid) s = !(m < 96) ? 1 : 0;
    c->r[5] = s;
    *B(f + 194) = (uint8_t)s;
    *B(f + 552) = (uint8_t)s;
    f_0886DC3C(c);
    c->r[29] += 0x20;
}

/* (value): fighter 3's (or 4's) trail recorded with its counter (0x09523C9C / 9A, round 128), unless
   another fighter's +237 is set while its own +237 is clear and +549 set. */
static void trail_for(Cpu *c, gaddr f, gaddr o1, gaddr o2, gaddr o3, gaddr counter) {
    uint32_t v = ARG(0);
    c->r[29] -= 0x20;
    c->r[5] = f; c->r[6] = v;
    int go = 1;
    if (S8(f + 237) == 0 && (S8(o1 + 237) | S8(o2 + 237) | S8(o3 + 237)) != 0) {
        c->r[6] = (uint32_t)(int32_t)S8(f + 549);
        if (c->r[6] != 0) go = 0; else c->r[6] = v;
    }
    if (go) {
        uint16_t n = (uint16_t)((*H(counter) + 1) & 0x7F);
        *H(counter) = n;
        c->r[4] = f; c->r[5] = v; c->r[6] = n; c->r[7] = n;
        f_0882D95C(c);
    }
    c->r[29] += 0x20;
}
SFA_FUNC(0882D82C, trail_fighter_3) { trail_for(c, FIGHTER_3, FIGHTER_1, FIGHTER_2, FIGHTER_4, 0x09523C9Cu); }
SFA_FUNC(0882D8C4, trail_fighter_4) { trail_for(c, FIGHTER_4, FIGHTER_1, FIGHTER_2, FIGHTER_3, 0x09523C9Au); }

/* (fighter): +696 = 0 and its step by +7 from 0x08A9D4BC when +192 is clear; else +696 = 1 and from
   0x08A9D4CC with +193, 0x08A9D4C4 without. */
SFA_FUNC(089A2AF0, step_by_192_193) {
    gaddr f = ARG(0);
    uint32_t s = *B(f + 7);
    if (S8(f + 192) == 0) { *B(f + 696) = 0; run_step(c, s, 0x08A9D4BCu, 5, 0x089A2B24u); }
    else {
        *B(f + 696) = 1;
        c->r[7] = (uint32_t)(int32_t)S8(f + 193);
        if (c->r[7] != 0) run_step(c, s, 0x08A9D4CCu, 5, 0x089A2B7Cu);
        else run_step(c, s, 0x08A9D4C4u, 5, 0x089A2B58u);
    }
}
