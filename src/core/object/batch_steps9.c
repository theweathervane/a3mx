/* Small steps (0.137.0): the 12 functions of 9 - 40 instructions made ready by the batches before, and
   the first 6 of 41 - 60. Fade levels, fighters' trails in turn, objects scrolling with the camera and
   bouncing at its edges, ending objects moving the camera layers, the network menu's head row and
   message panel, the session's lists and records. Offsets stay numbers until something shows what they
   are. What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define BATTLE    0x092E1228u
#define NET       0x09523980u
#define TRAILS    0x09523CA8u   /* the fighters' trail records, 0x480 apart */
#define FADE_ALL  0x092E1220u
#define FADE_GAME 0x08E75C44u
#define CAM_A     0x092E88A0u   /* camera layer records (0x092E8800 + 0xA0 n) */
#define CAM_B     0x092E8940u
#define ECAM      0x0951F950u   /* the ending's cameras */

void f_0882D6FC(Cpu *c);   /* fighter_1_trail_step */
void f_0882D794(Cpu *c);   /* fighter_2_trail_step */
void f_0882D82C(Cpu *c);   /* trail_fighter_3 */
void f_0882D8C4(Cpu *c);   /* trail_fighter_4 */
void f_0882DE00(Cpu *c);   /* fade_slots */
void f_0884FACC(Cpu *c);   /* place_by_camera */
void f_0884FC04(Cpu *c);   /* random_82_by_type */
void f_0884FB5C(Cpu *c);   /* animation_by_type_and_direction */
void f_0884FCF4(Cpu *c);   /* step_then_draw_58 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_089D4D60(Cpu *c);   /* frames_until_120 */
void f_0880A6CC(Cpu *c);   /* game_text */
void f_088EB790(Cpu *c);   /* net_menu_on_row */
void f_088D605C(Cpu *c);   /* random_eight_of_14 */
void f_088E0698(Cpu *c);   /* list_take_first */
void f_088DC2C4(Cpu *c);   /* unmark_under_lock */
void f_089F3470(Cpu *c);   /* game_memset */
void f_088E3B8C(Cpu *c);   /* net_message_panel */
void f_088E4044(Cpu *c);   /* text_kind_record */
void f_089DBF60(Cpu *c);   /* speeds_to_fixed */
void f_089DBEC0(Cpu *c);   /* swing_100 */
void f_089DA5DC(Cpu *c);   /* step_089DA5DC */
void f_088219B0(Cpu *c);   /* object_animate */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */

/* Fade levels: slots in runs of (first, count) at `level`, then the rest and the game's at `level`. */
static void fade_runs(Cpu *c, const uint8_t (*runs)[2], int n, uint32_t level) {
    for (int i = 0; i < n; i++) {
        c->r[4] = runs[i][0]; c->r[5] = runs[i][1]; c->r[6] = level;
        f_0882DE00(c);
    }
}

/* ---------------------------------------------------------------- fades and trails */

/* (): the trails of fighter 1, 3 and 4 recorded in turn. */
SFA_FUNC(0882D648, trails_1_3_4) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = TRAILS;
    c->r[4] = TRAILS; f_0882D6FC(c);
    c->r[4] = TRAILS + 0x480; f_0882D82C(c);
    c->r[4] = TRAILS + 0x900; f_0882D8C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the trails of fighter 1, 2 and 3 recorded in turn. */
SFA_FUNC(0882D6C0, trails_1_2_3) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = TRAILS;
    c->r[4] = TRAILS; f_0882D6FC(c);
    c->r[4] = TRAILS + 0x480; f_0882D794(c);
    c->r[4] = TRAILS + 0x900; f_0882D82C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the rest and the game's fade levels 0x80, slots 0x38 - 0x3D at 0x80. */
SFA_FUNC(0882DB10, fade_80) {
    c->r[29] -= 0x20;
    *B(FADE_ALL) = 0x80;
    *H(FADE_GAME) = 0x80;
    static const uint8_t runs[1][2] = { { 0x38, 6 } };
    fade_runs(c, runs, 1, 0x80);
    c->r[29] += 0x20;
}

/* (): fade levels 0x60 for both sides' slots (0 - 7, 0x40 - 0x4F) and the stage's runs; the rest and
   the game's 0x60. */
SFA_FUNC(0882DD5C, fade_both_60) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = 0x60;
    static const uint8_t runs[7][2] = { { 0, 8 }, { 0x40, 8 }, { 0x48, 8 }, { 0x15, 6 }, { 0x0F, 5 }, { 0x1B, 5 }, { 0x38, 6 } };
    fade_runs(c, runs, 7, 0x60);
    c->r[4] = 0x60; c->r[5] = 0x092E0000u;
    *B(FADE_ALL) = 0x60;
    c->r[4] = 0x08E70000u;
    *H(FADE_GAME) = 0x60;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- objects */

/* (object): +82 counts down; at 0 +124's 0x10000 bit flips and +82 is random again; placed by the camera
   at 14 / 16. */
SFA_FUNC(0884F918, flip_then_place) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int16_t n = S16(o + 82);
    if (n != 0) *H(o + 82) = (uint16_t)(n - 1);
    else {
        *W(o + 124) ^= 0x10000;
        c->r[4] = o; f_0884FC04(c);
    }
    c->r[4] = o; c->r[5] = 0xE;
    f_0884FACC(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +130 counts down; at 0 +120 moves by +96, which turns round (with its animation and +82
   again) once +122 reaches 481; placed by the camera at 14 / 16. */
SFA_FUNC(0884F97C, bounce_then_place) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = o + 0x60;
    int16_t n = S16(o + 130);
    if (n != 0) *H(o + 130) = (uint16_t)(n - 1);
    else {
        uint32_t x = *W(o + 120) + *W(o + 96);
        *W(o + 120) = x;
        c->r[6] = x & 0xFFFF0000u; c->r[7] = 0xFFFF0000u;
        if (!((int32_t)((x >> 16) & 0xFFFF) < 481)) {
            *W(o + 96) = 0u - *W(o + 96);
            c->r[4] = o; c->r[5] = *W(o + 96);
            f_0884FB5C(c);
            c->r[4] = o; f_0884FC04(c);
        }
    }
    c->r[4] = o; c->r[5] = 0xE;
    f_0884FACC(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): step 1, +15 = 1, its type (+3) from its facing (+11, then cleared), +12 cleared, animation
   0x08A15388's twelfth; then its step and drawing (step_then_draw_58). */
SFA_FUNC(0884FC9C, start_type_from_facing) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 15) = 1;
    *B(o + 4) = 1;
    *B(o + 3) = *B(o + 11);
    *B(o + 12) = 0;
    *B(o + 11) = 0;
    c->r[4] = o; c->r[5] = *W(0x08A15388u + 44);
    f_0882196C(c);
    c->r[4] = o; f_0884FCF4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): +138 one less, the next part (+5), +82 = +138; then framed until the record's +2
   reaches +120 (frames_until_120). */
SFA_FUNC(089D4D20, next_part_by_138) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *B(o + 138) = (uint8_t)(S8(o + 138) - 1);
    c->r[7] = (uint32_t)(int32_t)S8(o + 138);
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[6] = c->r[7];
    *H(o + 82) = (uint16_t)c->r[6];
    f_089D4D60(c);
    c->r[29] += 0x20;
}

/* (object): at intro stage 5 (BATTLE +44), unless +73 is set, part 5 at (0x70, 0xB1), +73 = -1, +148 =
   11, +156 = 0x08A16962, animation 0x08A16A90's eighteenth; then its part (+5) from 0x08A16F7C. */
SFA_FUNC(08856F00, part_5_at_stage_5) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[6] = (uint32_t)(int32_t)S8(BATTLE + 44); c->r[5] = 5;
    if (c->r[6] == 5) {
        c->r[4] = 5;
        if (S8(o + 73) == 0) {
            *B(o + 5) = 5;
            *B(o + 73) = 0xFF;
            *H(o + 18) = 0x70;
            *H(o + 22) = 0xB1;
            *W(o + 148) = 0xB;
            *W(o + 156) = 0x08A16962u;
            c->r[4] = o; c->r[5] = *W(0x08A16A90u + 68);
            f_0882196C(c);
        }
    }
    gaddr h = *W(0x08A16F7Cu + *B(o + 5) * 4u);
    uint32_t ra = c->r[31];
    c->r[4] = o; c->r[5] = h; c->r[31] = 0x08856F94u;
    rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4), its speeds to fixed point, +88 = -(+100), +14 = 2; the camera
   layers 0x092E88A0 / 0x092E8940 at the ending cameras' (0x0951F950 +162 / +166, +314 / +318); +16 / +20
   cleared; then the step 0x089DA5DC with the record. */
SFA_FUNC(089D9B10, ending_start_with_cameras) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = r;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; f_089DBF60(c);
    *W(o + 88) = 0u - *W(o + 100);
    *B(o + 14) = 2;
    *H(CAM_A + 18) = (uint16_t)S16(ECAM + 162);
    *H(CAM_A + 22) = (uint16_t)S16(ECAM + 166);
    *H(CAM_B + 18) = (uint16_t)S16(ECAM + 314);
    *H(CAM_B + 22) = (uint16_t)S16(ECAM + 318);
    *H(o + 20) = 0; *H(o + 16) = 0;
    c->r[6] = CAM_B; c->r[7] = (uint32_t)(int32_t)S16(ECAM + 166);
    c->r[4] = o; c->r[5] = r;
    f_089DA5DC(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object, record): the next step (+4) once the record's +2 reaches +119; before it +20 moves by +100,
   the ending camera +312 and the layer 0x092E8940 +16 by +96; swinging, animated, drawn if on screen. */
SFA_FUNC(089DA6A8, ending_pan_and_swing) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (!(S16(r + 2) < *B(o + 119))) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *W(o + 20) += *W(o + 100);
        *W(ECAM + 312) += *W(o + 96);
        *W(CAM_B + 16) += *W(o + 96);
        c->r[6] = CAM_B;
        c->r[4] = o; f_089DBEC0(c);
        c->r[4] = o; f_088219B0(c);
        c->r[4] = o; f_08821424(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4) once the record's +2 reaches +119; before it the cameras move
   instead of the object: ending camera +164 and layer 0x092E88A0 +20 by -(+100), +312 and 0x092E8940
   +16 by +96; swinging. */
SFA_FUNC(089DA74C, ending_pan_cameras) {
    gaddr o = ARG(0), r = ARG(1);
    c->r[29] -= 0x20;
    c->r[6] = 0x09520000u;
    if (!(S16(r + 2) < *B(o + 119))) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        *W(ECAM + 164) -= *W(o + 100);
        *W(ECAM + 312) += *W(o + 96);
        *W(CAM_A + 20) -= *W(o + 100);
        *W(CAM_B + 16) += *W(o + 96);
        c->r[5] = CAM_B;
        f_089DBEC0(c);
    }
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- the network */

/* (object): the network menu's head row: for row +92 0 - 2 its text from 0x08A3F698 (lit, colour 11,
   when NET +46 is that row, else 10); then, off row 3, +129 = 1 and +93 cleared. */
SFA_FUNC(088EB8F8, net_menu_head) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x30;
    c->r[16] = o;
    uint32_t row = *B(o + 92);
    c->r[4] = NET;
    if (row <= 2) {
        int32_t on = S8(NET + 46);
        static const uint16_t xy[3][2] = { { 0x90, 0x60 }, { 0x90, 0x70 }, { 0x70, 0x80 } };
        c->r[4] = *W(0x08A3F698u + row * 4);
        c->r[5] = 2; c->r[6] = xy[row][0]; c->r[7] = xy[row][1];
        c->r[8] = on == (int32_t)row ? 0xB : 0xA;
        f_0880A6CC(c);
    }
    c->r[4] = 3;
    f_088EB790(c);
    c->r[4] = 1;
    if (c->r[2] == 0) { *B(o + 129) = 1; *B(o + 93) = 0; }
    c->r[29] += 0x30;
    c->r[16] = s16;
}

/* (record, fighter): a fighter's entry in a network record: +22 = its +1173 - 1, +0 / +2 / +5 / +8 / +11
   from +438 / +338 / +386 / +376 / +439, +1 cleared, eight random bytes (random_eight_of_14). */
SFA_FUNC(088D60D0, net_fighter_record) {
    gaddr r = ARG(0), f = ARG(1);
    c->r[29] -= 0x20;
    *B(r + 22) = (uint8_t)(S8(f + 1173) - 1);
    *B(r + 1) = 0;
    *B(r) = *B(f + 438);
    *B(r + 2) = *B(f + 338);
    *B(r + 5) = *B(f + 386);
    *B(r + 8) = *B(f + 376);
    c->r[5] = (uint32_t)(int32_t)S8(f + 439);
    c->r[6] = (uint32_t)(int32_t)S8(f + 376);
    *B(r + 11) = (uint8_t)c->r[5];
    f_088D605C(c);
    c->r[29] += 0x20;
}

/* (list): each item taken off and unmarked, then the list's three words cleared. */
SFA_FUNC(088E06D0, list_unmark_all) {
    gaddr l = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = l;
    for (;;) {
        c->r[4] = l;
        f_088E0698(c);
        c->r[4] = c->r[2];
        if (c->r[2] == 0) break;
        f_088DC2C4(c);
    }
    c->r[4] = l; c->r[5] = 0; c->r[6] = 0xC;
    f_089F3470(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (record): its slot (+15) in 0x09523C20 cleared, its data (+4) and itself unmarked. */
SFA_FUNC(088DD84C, record_release) {
    gaddr r = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = r;
    if (r != 0) {
        uint32_t slot = *B(r + 15);
        c->r[5] = 0x09520000u;
        if (slot != 0) *W(0x09523C20u + slot * 4 - 4) = 0;
        if (*W(r + 4) != 0) {
            c->r[4] = *W(r + 4);
            f_088DC2C4(c);
            *W(r + 4) = 0;
        }
        c->r[4] = r; f_088DC2C4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +30 counts up; from 181 its text (kind 20 at 0, 0) made once into +20, and the message panel
   shown (net_message_panel 0). */
SFA_FUNC(088E307C, net_message_after_181) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 30) = (uint16_t)(S16(o + 30) + 1);
    c->r[5] = (uint32_t)(int32_t)S16(o + 30);
    c->r[4] = S16(o + 30) < 181;
    if (c->r[4] == 0) {
        c->r[4] = 0x14;
        if (*W(o + 20) == 0) {
            c->r[5] = 0; c->r[6] = 0;
            f_088E4044(c);
            *W(o + 20) = c->r[2];
        }
        c->r[4] = 0;
        f_088E3B8C(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (target, a1, a2, data, kind): under the list's semaphore (0x09523728), the target's +144 = a1, +152 =
   a2 and, for kind 4, +140 = the word at data. */
SFA_FUNC(088DCA5C, adhoc_target_set) {
    gaddr t = ARG(0);
    uint32_t a1 = ARG(1), a2 = ARG(2), data = ARG(3), kind = c->r[8];
    uint32_t s[6] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21] };
    c->r[29] -= 0x30;
    c->r[21] = 0x09523720u; c->r[20] = t; c->r[19] = a1; c->r[18] = a2; c->r[17] = data; c->r[16] = kind;
    if (*W(0x09523720u + 8) != 0) os_wait_sema(c, (int32_t)*W(0x09523720u + 8), 1, 0);
    *W(t + 144) = a1;
    *H(t + 152) = (uint16_t)a2;
    c->r[4] = 4;
    if (kind == 4) { c->r[4] = *W(data); c->r[5] = t + 0x8C; *W(t + 140) = c->r[4]; }
    c->r[4] = *W(0x09523720u + 8);
    if (c->r[4] != 0) os_signal_sema(c, (int32_t)c->r[4], 1);
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4]; c->r[21] = s[5];
}
