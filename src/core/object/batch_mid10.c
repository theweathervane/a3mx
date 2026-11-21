/* More steps (0.115.0): the screen set up for the demo, a fighter put into a throw's hold, a CPU script
 * command that tests its layout, the fight's continue checks, panels lit by mode, a row of particles,
 * and the special-match check for character 9. Offsets stay numbers until something shows what they
 * are. Arguments of native callees, and what the original leaves in a0 - a3 / t0 at a call through a
 * table, are set as the original sets them; s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define P1      0x092E3EA0u
#define P2      0x092E4338u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

void f_0880A39C(Cpu *c);   /* fill_092DCA40 */
void f_08822934(Cpu *c);   /* pool_4_reset */
void f_08823304(Cpu *c);   /* game_objects_3_clear */
void f_088D2B54(Cpu *c);   /* nothing_088D2B54 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08837058(Cpu *c);   /* add_one_unless_373 */
void f_08889F7C(Cpu *c);   /* setup_from_description */
void f_0888A224(Cpu *c);   /* record_lit_by_mode */
void f_08891DF0(Cpu *c);   /* step_08891DF0 */
void f_08892A58(Cpu *c);   /* check_058a0 */
void f_088892C4(Cpu *c);   /* record_flash_or_fade */
void f_088D7C1C(Cpu *c);   /* game_continuing_match */
void f_088B54FC(Cpu *c);   /* clear_5c_7f */
void f_088B4578(Cpu *c);   /* fill_trail_with_place */
void f_088B544C(Cpu *c);   /* anim_by_3_and_2e08 */
void f_088215C4(Cpu *c);   /* object_draw */
void f_0889CFE8(Cpu *c);   /* record_setup_layer */
void f_0889CEE4(Cpu *c);   /* set_motion_by_mode */
void f_0889CE34(Cpu *c);   /* set_animation_b */
void f_08899FD0(Cpu *c);   /* row_for_character */
void f_08876BE4(Cpu *c);   /* fighter_run_by_layout */
void f_08877258(Cpu *c);   /* fighter_chance_by_level */
void f_0887A10C(Cpu *c);   /* fighter_script_read */

/* ---------------------------------------------------------------- the game */

/* (screen): step 7.0; the tables filled, pools and lists reset, two kind-0x99 objects (steps 12, 13),
   three list-3 objects of kind 0x18 (types 0 - 2) and two of kind 6 (0x10, 0x19); +4 = 0x4F. */
SFA_FUNC(08817950, demo_screen_setup) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 2) = 7;
    *B(o + 3) = 0;
    c->r[4] = 7;
    f_0880A39C(c);
    f_08822934(c);
    f_08823304(c);
    f_088D2B54(c);
    c->r[4] = 0x99; c->r[5] = 0xC; f_08822EAC(c);
    c->r[4] = 0x99; c->r[5] = 0xD; f_08822EAC(c);
    for (c->r[17] = 0; (int32_t)c->r[17] < 3; c->r[17] = (c->r[17] + 1) & 0xFF) {
        c->r[4] = 0x18; c->r[5] = c->r[17];
        f_08823338(c);
    }
    c->r[4] = 6; c->r[5] = 0x10; f_08823338(c);
    c->r[4] = 6; c->r[5] = 0x19; f_08823338(c);
    *B(o + 4) = 0x4F;
    c->r[4] = 0x4F;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (screen, record, fighter): continue counting (+340) for the modes that always do (8, 15, 17, 18), for
   character 10 with +430, or when 0x08A058A0 says so; otherwise the fighter's step (0x08891DF0) unless
   the screen's +43 is set with the record's +4 and +6 clear. */
SFA_FUNC(0883436C, continue_count_or_step) {
    gaddr a = ARG(0), b = ARG(1), f = ARG(2), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = b; c->r[18] = a;
    int32_t m = S8(a + 114);
    int count = m == 8 || m == 15 || m == 17 || m == 18;
    if (!count && S8(f + 338) == 10 && S8(f + 430) != 0) count = 1;
    if (!count) {
        c->r[4] = (uint32_t)(S8(f + 338) == 10 ? S8(f + 430) : S8(f + 338));
        c->r[5] = 10;
        f_08892A58(c);
        count = c->r[2] != 0;
    }
    if (count) {
        *B(f + 340) = (uint8_t)(S8(f + 340) + 1);
    } else if (S8(a + 43) == 0 || S8(b + 4) != 0 || S8(b + 6) != 0) {
        c->r[4] = f;
        f_08891DF0(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (fighter): in a continuing match, its pick for the next round (+157): 6 / 7 by its +13 when it is a
   human (+12 = 3) or a CPU with +76; otherwise 5 with +13, or by its pad (+112 +248: 0x20 gives 4, 0x40
   3, else 0x08A0F81C), +158 the side (-1 for the first case). */
SFA_FUNC(0883311C, continue_pick) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_088D7C1C(c);
    if (c->r[2] != 0) {
        int32_t a = S8(o + 12), b = S8(o + 13);
        if (a == 3 || S8(o + 76) != 0 || a == 0) {
            *B(o + 158) = 0xFF;
            *B(o + 157) = b != 0 ? 7 : 6;
        } else {
            *B(o + 158) = (uint8_t)a;
            if (b != 0) *B(o + 157) = 5;
            else {
                int32_t v = S8(*W(o + 112) + 248);
                if (v & 0x20) *B(o + 157) = 4;
                else if (v & 0x40) *B(o + 157) = 3;
                else *B(o + 157) = (uint8_t)S8(0x08A0F81Cu + (uint32_t)(v & 0xF));
            }
        }
    }
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (screen, record): the special match for character 9 (GAME +114 = 9, or BATTLE +20 = 9 with the right
   opponent): its two fighters (+112, +116; swapped by +57 bit 0) and characters (+50, +51) noted; +69
   set for a human, +70 and the screen's +72 = -1. Not in a team battle, at +79 or +61. */
SFA_FUNC(0882659C, special_match_9) {
    (void)c;
    gaddr a = ARG(0), b = ARG(1), x = P1, y = P2;
    if (S16(b + 56) != 0 || S8(b + 79) != 0 || S8(b + 61) != 0) return;
    if ((S8(a + 57) & 1) == 0) { x = P2; y = P1; }
    if (S8(a + 114) != 9) {
        if (S16(b + 20) != 9) return;
        if ((uint32_t)*B(0x09517988u + 9) != (uint32_t)(int32_t)S8(y + 338)) return;
    }
    int32_t v = S8(b + 70);
    *B(b + 69) = 0;
    if (v != 0) return;
    if (S8(b + 5) == 0) {
        if (S8(b + 63) == 0) return;
        *W(b + 112) = y;
        *W(b + 116) = x;
        *B(b + 50) = (uint8_t)S8(y + 338);
        *B(b + 51) = (uint8_t)S8(x + 338);
    }
    if (S8(a + 114) != 9) return;
    if (S8(y + 373) != 0) *B(b + 69) = 1;
    *B(b + 70) = 0xFF;
    *B(a + 72) = 0xFF;
}

/* ---------------------------------------------------------------- fighters */

/* (thrower, held): the held fighter put into the hold (state 1.3, +82 = 4, +135 = -1) and the thrower's
   flags (+135 = 1, +80 with +251); both fighters' throw fields cleared. */
SFA_FUNC(0886ABB4, throw_hold) {
    gaddr a = ARG(0), b = ARG(1), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = b; c->r[17] = a; c->r[18] = 1;
    *B(b + 4) = 1; *B(b + 5) = 3; *B(b + 6) = 0; *B(b + 7) = 0;
    *B(b + 135) = 0xFF;
    *B(a + 135) = 1;
    *B(b + 698) = 1;
    *H(b + 82) = 4;
    *B(b + 579) = 1;
    *B(b + 539) = 1;
    *B(b + 1145) = 0;
    c->r[4] = a; c->r[5] = 1;
    f_08837058(c);
    if (S8(a + 251) != 0) *B(a + 80) = 1;
    *B(a + 644) = 0; *B(a + 639) = 0; *B(a + 629) = 0; *B(a + 628) = 0;
    *B(a + 502) = 0; *B(b + 502) = 0;
    *B(a + 235) = 0; *B(b + 235) = 0;
    *B(b + 777) = 0;
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (fighter, script): script command: read on while the layout's run (0x08876BE4) says no; otherwise state
   1.6.4 with +482 = 6 and +500 from +488, kept only with +499, +554 and a chance from 0x08A228A0. */
SFA_FUNC(08879EB4, fighter_script_cmd_layout) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    f_08876BE4(c);
    if (c->r[2] == 0) {
        c->r[4] = f; c->r[5] = p;
        f_0887A10C(c);
    } else {
        *B(f + 482) = 6;
        *B(f + 483) = 0;
        *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 6;
        *B(f + 500) = (uint8_t)S8(f + 488);
        *B(f + 7) = 4;
        if (S8(f + 500) != 0) {
            int keep = 0;
            if (S8(f + 499) != 0 && S8(f + 554) != 0) {
                c->r[4] = f; c->r[5] = 0x08A228A0u;
                f_08877258(c);
                keep = c->r[2] != 0;
            }
            if (!keep) *B(f + 500) = 0;
        }
        c->r[2] = p;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---------------------------------------------------------------- objects */

/* (panel): set up from 0x08A264C0's 16 bytes for +92, placed from 0x08A268F8 (16 up; type 7 also counts
   +78), lit by mode (0x0888A224) and shown. */
SFA_FUNC(08889968, panel_by_92_lit) {
    gaddr o = ARG(0), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    uint32_t k = *B(o + 92);
    c->r[16] = o; c->r[17] = 1; c->r[18] = k;
    *B(o + 54) = 0x12;
    *B(o + 88) = 1;
    *B(o + 55) = 7;
    c->r[4] = o; c->r[5] = 0x08A264C0u + (k << 4);
    f_08889F7C(c);
    gaddr e = 0x08A268F8u + k * 4;
    *H(o + 6) = *H(e);
    *H(o + 10) = (uint16_t)(*H(e + 2) - 16);
    if (*B(o + 92) == 7) *H(o + 78) = (uint16_t)(*H(o + 78) + 1);
    *B(o + 56) = 2;
    c->r[4] = o;
    f_0888A224(c);
    *B(o + 138) = 1;
    *B(o + 93) = 1;
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (panel): +128 moves on; a 0x180 x 0x100 panel from y 0x100, its alpha and blend from 0x08A262D5 and
   its colour from 0x08A262D0 by +92; then its flash or fade (0x088892C4). */
SFA_FUNC(08889200, panel_flash_by_92) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 55) = 7;
    *B(o + 3) = 0;
    *B(o + 2) = 0;
    *H(o + 6) = 0;
    *H(o + 10) = 0x100;
    *H(o + 12) = 0;
    uint32_t k = *B(o + 92);
    *H(o + 14) = 0x180;
    *H(o + 24) = 0;
    *H(o + 26) = 0x100;
    gaddr e = 0x08A262D5u + k * 2;
    *B(o + 136) = (uint8_t)S8(e);
    *B(o + 56) = (uint8_t)S8(e + 1);
    uint8_t v = (uint8_t)S8(0x08A262D0u + k);
    for (int i = 36; i <= 39; i++) *B(o + i) = v;
    for (int i = 42; i <= 45; i++) *B(o + i) = v;
    for (int i = 48; i <= 51; i++) *B(o + i) = v;
    *B(o + 2) = v;
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)(int32_t)(int8_t)v; c->r[6] = 0x08A262D0u; c->r[7] = k; c->r[8] = 0x08A20000u;
    f_088892C4(c);
    c->r[29] += 0x20;
}

/* (particle): cleared, +4 moves on; picture +78 (colour -1 when set), x from 0x08A37390 by +3 at y 0x80,
   its trail filled, its picture, drawn. */
SFA_FUNC(088B4014, particle_row_start) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088B54FC(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    int32_t k = S8(o + 78);
    *H(o + 86) = 0xA;
    *B(o + 12) = k != 0 ? 0xFF : 0;
    *B(o + 13) = (uint8_t)k;
    *B(o + 9) = (uint8_t)k;
    *H(o + 36) = 0; *H(o + 38) = 0; *H(o + 20) = 0; *H(o + 16) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *B(o + 73) = 8;
    *H(o + 24) = 5;
    *H(o + 18) = (uint16_t)S16(0x08A37390u + (uint32_t)*B(o + 3) * 2);
    *H(o + 22) = 0x80;
    c->r[4] = o; f_088B4578(c);
    c->r[4] = o; f_088B544C(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (record, fighter): +4 moves on with the fighter's +424 bit 3; otherwise +6 moves on and it is set up
   on layer 1, rising at 1, with the motion and animation for the fighter's side, and its row. */
SFA_FUNC(0889B90C, row_rise_by_side) {
    gaddr o = ARG(0), f = ARG(1), s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    if (S16(f + 424) & 8) {
        *B(o + 5) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    } else {
        c->r[4] = o; c->r[5] = 1;
        f_0889CFE8(c);
        *W(o + 20) = 0;
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *W(o + 100) = 0xFFFF0000u;
        *W(o + 148) = 2;
        *H(o + 86) = 8;
        *W(o + 164) = 0;
        *W(0x092E3E98u + 4) = 0x08A27FFCu;
        c->r[18] = (uint32_t)(S8(f + 337) * 2);
        c->r[4] = o; c->r[5] = (c->r[18] + 3) & 0xFFFF;
        f_0889CEE4(c);
        c->r[4] = o; c->r[5] = (c->r[18] + 0xD) & 0xFFFF;
        f_0889CE34(c);
        c->r[4] = o; c->r[5] = f;
        f_08899FD0(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}
