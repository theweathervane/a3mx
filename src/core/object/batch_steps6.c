/* Small steps (0.134.0): 25 more of the profile's ready functions of 25 - 40 instructions. Objects
   stepping by tables while the screen allows it, made at intro stages, counting down by frames; text
   records; a fighter's target among two, the side ahead on rounds, script counts; the ad-hoc link's
   end, flags and parameters; the network error and menu checks. Offsets stay numbers until something
   shows what they are. What the original leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define NET       0x09523980u   /* the network task's state */
#define NET_STEP  0x09523960u   /* the network step's state (+0 result, +4 error) */
#define TEXTS     0x092DF840u   /* ten 24-byte text records */

void f_088DC2C4(Cpu *c);   /* unmark_under_lock */
void f_088E3B28(Cpu *c);   /* result_number */
void f_08822BB0(Cpu *c);   /* clear_0950 */
void f_088219B0(Cpu *c);   /* object_animate */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_088214E8(Cpu *c);   /* object_draw_if_on_screen_margin */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08821368(Cpu *c);   /* game_random */
void f_0896181C(Cpu *c);   /* count_82_then_move */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_0887684C(Cpu *c);   /* fighter_set_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_0886EB48(Cpu *c);   /* fighter_unpack_lists */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08821AA8(Cpu *c);   /* object_next_frame */
void f_0884E518(Cpu *c);   /* frame_by_78 */
void f_0884E570(Cpu *c);   /* random_82 */

/* Calls a handler with a1 holding it; `back` is where the original's call returns to. */
static void call_handler(Cpu *c, gaddr handler, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[5] = handler;
    c->r[31] = back;
    rt_call_indirect(c, handler);
    c->r[31] = ra;
}

/* ---------------------------------------------------------------- the network */

/* (link): its marked data (+184) unmarked, its matching (+8) stopped, deleted and ended if there is
   one, and the ad-hoc group left. */
SFA_FUNC(088DF400, adhoc_close) {
    gaddr l = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = l;
    if (*W(l + 184) != 0) {
        c->r[4] = *W(l + 184);
        f_088DC2C4(c);
        *W(l + 184) = 0;
    }
    int32_t id = (int32_t)*W(l + 8);
    if (id >= 0) {
        os_net_adhoc_matching_stop(c, id);
        os_net_adhoc_matching_delete(c, (int32_t)*W(l + 8));
        os_net_adhoc_matching_term(c);
        *W(l + 8) = 0xFFFFFFFFu;
    }
    os_net_adhocctl_disconnect(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (code): the network step's first result kept: +0 its number (result_number), +4 the code if an
   error, +8 - +16, +22, +24, +25 cleared. Later ones are left while +4 is set. */
SFA_FUNC(088E3ED8, net_keep_result) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    uint32_t code = ARG(0);
    c->r[29] -= 0x20;
    c->r[17] = NET_STEP; c->r[16] = code;
    c->r[5] = *W(NET_STEP + 4);
    if (c->r[5] == 0) {
        *B(NET_STEP + 22) = 0;
        c->r[4] = code;
        f_088E3B28(c);
        *W(NET_STEP) = c->r[2];
        c->r[4] = (int32_t)code >= 0 ? 0 : code;
        *W(NET_STEP + 4) = c->r[4];
        *B(NET_STEP + 24) = 0;
        *W(NET_STEP + 8) = 0; *W(NET_STEP + 12) = 0; *W(NET_STEP + 16) = 0;
        *B(NET_STEP + 25) = 0;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (link): state 1 (+20); a flag bit 1 (+16) taken down reads the group's parameters into +0x1C (its
   error answered if that fails, else 1); a bit 0 taken down gives the link's data (+4). 0 when neither. */
SFA_FUNC(088DF500, adhoc_take_parameters) {
    gaddr l = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = l;
    uint32_t flags = *W(l + 16);
    *W(l + 20) = 1;
    RETURN(0);
    if (flags & 2) {
        *W(l + 16) = flags & ~2u;
        if (os_net_adhocctl_get_parameter(c, l + 0x1C) < 0) goto out;
        RETURN(1);
        flags = *W(l + 16);
    }
    c->r[4] = flags;
    if (flags & 1) {
        RETURN(*W(l + 4));
        *W(l + 16) = flags & ~1u;
    }
out:
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (row): 1 when the network menu is open (NET +47) on this row: NET +0's screens 3 - 5 are rows 3 - 5,
   the others below 6 row 2, the rest row 0. */
SFA_FUNC(088EB790, net_menu_on_row) {
    uint32_t row = ARG(0) & 0xFF;
    c->r[4] = row;
    if (S8(NET + 47) == 0) { RETURN(0); return; }
    uint32_t screen = *H(NET), r;
    if (!(screen < 6)) r = 0;
    else if (screen == 3) r = 3;
    else if (screen == 4) r = 4;
    else if (screen == 5) r = 5;
    else r = 2;
    c->r[5] = r;
    RETURN(row == r);
}

/* ---------------------------------------------------------------- text */

/* (kind, x, y, word, byte): the first free of the ten text records (0x092DF840): in use, +6 / +10 = x /
   y, +12 = kind, +16 = word, +20 = byte. 0 when all are in use. */
SFA_FUNC(0880A740, text_record_take) {
    uint8_t kind = (uint8_t)ARG(0), b = (uint8_t)c->r[8];
    int16_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    uint32_t word = ARG(3);
    gaddr p = TEXTS;
    for (uint32_t i = 0; i < 10; i++, p += 0x18) {
        if (*B(p) != 0) continue;
        *B(p + 1) = 1; *B(p) = 1;
        *H(p + 6) = (uint16_t)x;
        *H(p + 10) = (uint16_t)y;
        *B(p + 12) = kind;
        *B(p + 20) = b;
        *W(p + 16) = word;
        c->r[4] = 1; c->r[5] = b; c->r[6] = (uint32_t)(int32_t)y; c->r[8] = (uint32_t)(int32_t)x; c->r[9] = kind;
        RETURN(p);
        return;
    }
    RETURN(0);
}

/* ---------------------------------------------------------------- objects */

/* (object, fighter): once, when +57 has the fighter's side (+337) and its slot's record (0x08E75BA0 +
   44 n by +775) is clear, +138 = that slot's bit and 0x0950 cleared (clear_0950). */
SFA_FUNC(08835B30, mark_slot_138) {
    gaddr o = ARG(0), f = ARG(1);
    c->r[29] -= 0x10;
    if (S8(o + 138) == 0) {
        c->r[6] = 1;
        c->r[7] = (uint32_t)S8(o + 57) & (1u << (S8(f + 337) & 31));
        if (c->r[7] != 0) {
            int32_t n = S8(f + 775);
            c->r[7] = *H(0x08E75BA0u + (uint32_t)n * 44);
            c->r[5] = 1u << (n & 31);
            if (c->r[7] == 0) {
                *B(o + 138) = (uint8_t)c->r[5];
                f_08822BB0(c);
            }
        }
    }
    c->r[29] += 0x10;
}

/* (object): the next step (+4) unless GAME +69 is not negative and GAME +42 is set; then its part
   (+5) from `table` and drawn. */
static void part_while_42(Cpu *c, gaddr table, uint32_t back) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(GAME + 69) < 0 || S8(GAME + 42) == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        c->r[4] = o;
        call_handler(c, *W(table + *B(o + 5) * 4u), back);
        c->r[4] = o; f_088215C4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(08880D68, part_while_42_24A98) { part_while_42(c, 0x08A24A98u, 0x08880DC4u); }
SFA_FUNC(088818AC, part_while_42_24B9C) { part_while_42(c, 0x08A24B9Cu, 0x08881908u); }
SFA_FUNC(08881A58, part_while_42_24BD8) { part_while_42(c, 0x08A24BD8u, 0x08881AB4u); }

/* (object): the next step (+4) while BATTLE +42 is set or once +75 is negative; else animated and drawn
   if on screen. */
SFA_FUNC(08908EDC, animate_until_75) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 42) != 0 || S8(o + 75) < 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        c->r[4] = o; f_088219B0(c);
        c->r[4] = o; f_08821424(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): when the owner's (+88) +77 is set, the next part (+5): +82 = 19, thrown (2.0 forward,
   6.0 up, falling by 0.25); then counting down and moving (count_82_then_move). */
SFA_FUNC(089617AC, thrown_when_owner_77) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (S8(*W(o + 88) + 77) != 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0x13;
        *W(o + 96) = 0x20000;
        *W(o + 104) = 0;
        *W(o + 100) = 0x60000;
        *W(o + 108) = 0xFFFFC000u;
        c->r[6] = 0xFFFFC000u;
        if (*B(o + 11) != 0) *W(o + 96) = 0u - *W(o + 96);
        c->r[5] = *W(o + 96);
        f_0896181C(c);
    }
    c->r[29] += 0x20;
}

/* (object): +82 counts down; past 0 +78 steps round 32 places, +82 = 1 and +20 moves by the place's
   word at 0x08A85F14; animated. */
SFA_FUNC(089619E0, bob_by_78) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n < 0) {
        *B(o + 78) = (uint8_t)((S8(o + 78) + 1) & 0x1F);
        *H(o + 82) = 1;
        c->r[7] = 0x08A85F14u;
        c->r[5] = *W(o + 20) + *W(0x08A85F14u + (uint32_t)S8(o + 78) * 4);
        *W(o + 20) = c->r[5];
    }
    f_088219B0(c);
    c->r[29] += 0x20;
}

/* (object): +82 counts down from where it is; at 0 the next part (+5), +82 = 11, at (0x70, 0xB1), and a
   list-3 object kind 5 type 7 with this one as its owner (+124). */
SFA_FUNC(08857114, make_5_7_after_82) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int16_t n = S16(o + 82);
    *H(o + 82) = (uint16_t)(n - 1);
    if (n == 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0xB;
        *H(o + 18) = 0x70;
        *H(o + 22) = 0xB1;
        c->r[4] = 5; c->r[5] = 7;
        f_08823338(c);
        c->r[4] = c->r[2];
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): at intro stage 6 (BATTLE +44) the next part (+5), +96 = -16.0, +82 = 3 and a list-3 object
   kind 5 type 9 with this one as its owner (+124); else drawn. */
SFA_FUNC(08857188, make_5_9_at_stage_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[6] = (uint32_t)(int32_t)S8(BATTLE + 44); c->r[5] = 6;
    if (c->r[6] == 6) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *W(o + 96) = 0xFFF00000u;
        *H(o + 82) = 3;
        c->r[4] = 5; c->r[5] = 9;
        f_08823338(c);
        c->r[4] = c->r[2];
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
    } else {
        c->r[4] = o; f_088215C4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): eight random bytes at +14 - +21, each one of the fourteen at 0x08A3EAAD. */
SFA_FUNC(088D605C, random_eight_of_14) {
    gaddr o = ARG(0);
    uint32_t s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 0x08A3EAADu; c->r[18] = 0xE;
    for (uint32_t i = 0; i < 8; i++) {
        c->r[19] = i;
        f_08821368(c);
        c->lo = c->r[2] / 14; c->hi = c->r[2] % 14;
        *B(o + 14 + i) = *B(0x08A3EAADu + c->hi);
        c->r[19] = i + 1;
        c->r[4] = o + i; c->r[5] = i + 1 < 8; c->r[6] = *B(0x08A3EAADu + c->hi);
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (object): the first time (+5 from 0) the owner's (+88) +228 cleared and frame 6; after that the next
   frame, back to step 3 once +75 is negative; drawn if on screen. */
SFA_FUNC(088F1624, frames_until_75) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    gaddr owner = *W(o + 88);
    c->r[5] = owner;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(owner + 228) = 0;
        c->r[4] = o; c->r[5] = 6;
        f_0887684C(c);
    } else {
        if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        c->r[4] = o; f_088768F0(c);
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4) once the record's +2 reaches +118: +11, +14, +38 cleared and
   the animation for +116 from the record's list (+280). */
SFA_FUNC(089CF3D8, next_animation_at_118) {
    gaddr o = ARG(0), r = ARG(1);
    c->r[29] -= 0x20;
    int32_t limit = *B(o + 118);
    c->r[7] = r; c->r[5] = o; c->r[4] = r;
    c->r[6] = S16(r + 2) < limit;
    if (c->r[6] == 0) {
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *B(o + 14) = 0;
        *H(o + 38) = 0;
        *B(o + 11) = 0;
        c->r[7] = 0;
        c->r[5] = *W(*W(r + 280) + *B(o + 116) * 4u);
        c->r[6] = c->r[5];
        c->r[4] = o;
        f_0882196C(c);
    }
    c->r[29] += 0x20;
}

/* (object, record): the next part (+5) once the record's +2 reaches +120; before it +82 counts down,
   and past 0 starts again from +138 with the next frame. */
SFA_FUNC(089D4D60, frames_until_120) {
    gaddr o = ARG(0), r = ARG(1);
    c->r[29] -= 0x20;
    int32_t limit = *B(o + 120);
    c->r[6] = (uint32_t)limit;
    if (!(S16(r + 2) < limit)) { *B(o + 5) = (uint8_t)(*B(o + 5) + 1); }
    else {
        int16_t n = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)n;
        c->r[5] = (uint32_t)(int32_t)n;
        if (n < 0) {
            c->r[6] = (uint32_t)(int32_t)S8(o + 138);
            *H(o + 82) = (uint16_t)c->r[6];
            f_08821AA8(c);
        }
    }
    c->r[29] += 0x20;
}

/* (object): step 1, +15 = 1, +12 cleared, +78 = 8, the animation for its type (+3) from 0x08A150C8 with
   its list (+28) at +96; the frame for +78, +82 random, drawn if within 1 of the screen. */
SFA_FUNC(0884E3A4, start_with_frames) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 15) = 1;
    *B(o + 4) = 1;
    *B(o + 12) = 0;
    *B(o + 78) = 8;
    c->r[4] = o; c->r[5] = *W(0x08A150C8u + *B(o + 3) * 4u);
    f_0882196C(c);
    *W(o + 96) = *W(o + 28);
    c->r[4] = o; c->r[5] = *W(o + 28);
    f_0884E518(c);
    c->r[4] = o; f_0884E570(c);
    c->r[4] = o; c->r[5] = 1;
    f_088214E8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, a, b): its target (+92) the nearer of a and b with health left (+112), looked at again
   every 30 frames (60 for the second side, +337 bit 1; always 30 in mode 13) by +574, or at once when
   one of them is out; a +135 set keeps to a unless a's +135 is clear. Two fighters at step 9 (+5) start
   the count over. */
SFA_FUNC(088594D0, fighter_pick_target) {
    gaddr f = ARG(0), a = ARG(1), b = ARG(2);
    if (S8(f + 135) != 0) {
        c->r[7] = (uint32_t)(int32_t)S8(a + 135);
        *W(f + 92) = c->r[7] == 0 ? b : a;
        return;
    }
    if (*B(a + 5) == 9 && *B(b + 5) == 9) *B(f + 574) = 0;
    if (!(S16(a + 112) < 0) && !(S16(b + 112) < 0)) {
        int32_t n = S8(f + 574);
        if (n != 0) { c->r[5] = (uint32_t)(n - 1); *B(f + 574) = (uint8_t)(n - 1); return; }
    }
    int16_t x = S16(f + 18);
    uint8_t wait = 0x1E;
    if (S8(GAME + 114) != 13 && (S8(f + 337) & 2)) wait = 0x3C;
    *B(f + 574) = wait;
    int16_t da = (int16_t)(S16(a + 18) - x), db = (int16_t)(S16(b + 18) - x);
    int16_t ada = da < 0 ? (int16_t)-da : da, adb = db < 0 ? (int16_t)-db : db;
    c->r[9] = (uint32_t)(int32_t)ada; c->r[10] = (uint32_t)(int32_t)adb; c->r[7] = (uint32_t)(-(int32_t)db);
    c->r[8] = (uint32_t)(int32_t)S16(a + 112);
    if (!(S16(a + 112) < 0)) {
        *W(f + 92) = a;
        c->r[5] = adb < ada;
        if (!(adb < ada)) return;
    }
    c->r[5] = (uint32_t)(int32_t)S16(b + 112);
    if (S16(b + 112) < 0) return;
    *W(f + 92) = b;
}

/* (record): the fighter ahead: in round kind 3 (+42) the one with more +348, then more +346 (fighter 1
   on a tie); else fighter 1 for odd kinds, fighter 2 for even. */
SFA_FUNC(0882B4E4, fighter_ahead) {
    int32_t kind = S8(ARG(0) + 42);
    c->r[5] = 3;
    if (kind != 3) {
        c->r[4] = (uint32_t)kind & 1;
        RETURN(c->r[4] ? FIGHTER_1 : FIGHTER_2);
        return;
    }
    int32_t w1 = *H(FIGHTER_1 + 348), w2 = *H(FIGHTER_2 + 348);
    c->r[6] = FIGHTER_2; c->r[5] = (uint32_t)w2;
    RETURN(FIGHTER_1);
    c->r[4] = w1 < w2;
    if (w2 < w1) { c->r[7] = 1; return; }
    c->r[7] = 0;
    if (w1 < w2) { c->r[4] = 1; RETURN(FIGHTER_2); return; }
    int32_t p1 = *H(FIGHTER_1 + 346), p2 = *H(FIGHTER_2 + 346);
    c->r[5] = (uint32_t)p2; c->r[4] = p1 < p2;
    if (p1 < p2) RETURN(FIGHTER_2);
}

/* (fighter, script): +490 = the script's count; below +508 it reads on, else its script ends there
   (routine 1 / 0 / 2 / 0, +482 / +483 = 1, +78 / +497 = 1). Returns the script after the count. */
SFA_FUNC(0887802C, script_count_to_508) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    int16_t v = S16(p);
    *H(f + 490) = (uint16_t)v;
    int16_t d = (int16_t)(v - S16(f + 508));
    c->r[6] = (uint32_t)(int32_t)d;
    c->r[7] = (uint32_t)(int32_t)(v - S16(f + 508));
    RETURN(p + 2);
    if (d < 0) {
        c->r[5] = p + 2;
        f_0887A10C(c);
    } else {
        *B(f + 483) = 1; *B(f + 482) = 1;
        *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
        *B(f + 78) = 1; *B(f + 497) = 1;
        c->r[5] = 1; c->r[6] = 2;
    }
    c->r[29] += 0x20;
}

/* (fighter): +692 a random one of the bytes at 0x08A85E90: of the first 16 below 128 health (+112), of
   the second 16 from 128, of the third 16 whenever the layout (BATTLE +56) is not 0. */
SFA_FUNC(089610E4, random_692_by_health) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = f;
    f_08821368(c);
    uint32_t i = c->r[2] & 0xF;
    c->r[5] = (uint32_t)(int32_t)S16(BATTLE + 56);
    if (c->r[5] != 0) i = (i + 0x20) & 0xFF;
    else if (!(S16(f + 112) < 128)) i = (i + 0x10) & 0xFF;
    c->r[5] = 0x08A85E90u;
    c->r[4] = *B(0x08A85E90u + i);
    *B(f + 692) = (uint8_t)c->r[4];
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* +171 cleared, +12 = -1, the 39 words at `src` copied to the routine table (+416), its lists unpacked
   (fighter_unpack_lists with n and `lists`), +164 = 1. */
static void fighter_table_from(Cpu *c, gaddr src, uint32_t n, gaddr lists) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    gaddr d = *W(f + 416);
    for (uint32_t i = 0; i < 0x27; i++) *W(d + i * 4) = *W(src + i * 4);
    c->r[7] = 0;
    c->r[4] = f; c->r[5] = n; c->r[6] = lists;
    f_0886EB48(c);
    c->r[4] = 1;
    *W(f + 164) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(0895BEEC, fighter_table_from_858C4) { fighter_table_from(c, 0x08A858C4u, 0x16, 0x08A85960u); }
SFA_FUNC(089A2778, fighter_table_from_9D3B8) { fighter_table_from(c, 0x08A9D3B8u, 0x22, 0x08A9D454u); }
