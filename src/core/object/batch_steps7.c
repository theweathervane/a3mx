/* Small steps (0.135.0): 26 more of the profile's ready functions of 25 - 40 instructions. Objects
   stepping by a record's count, set by their owner, picking animations by a level; fighters' script
   counts and branches, a CPU's special opponent; the ad-hoc link's flags, thread end and target, the
   session's agreement. Offsets stay numbers until something shows what they are. What the original
   leaves in a0 - t0 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define ADHOC     0x09523740u   /* the ad-hoc link's record (+0 thread, +4 data, +8 matching, +16 flags) */
#define NSTATE    0x09523920u   /* the network session's state */

void f_0880A740(Cpu *c);   /* text_record_take */
void f_088DD3DC(Cpu *c);   /* session_end_unless_waiting */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08821368(Cpu *c);   /* game_random */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08857678(Cpu *c);   /* fighter_by_layout_3 */
void f_088888B4(Cpu *c);   /* step_088888B4 */
void f_0889DB8C(Cpu *c);   /* place_kind */
void f_08877F44(Cpu *c);   /* fighter_to_routine_1 */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */

/* ---------------------------------------------------------------- the network */

/* (what, data): the ad-hoc link's event flags (+16): 0 the data arrived (+4 = data, bit 0); 1 joined
   (bit 1, +190 = 1); 2 left (bit 2, +190 cleared); 3 bit 3. */
SFA_FUNC(088DE6B8, adhoc_event) {
    uint32_t what = ARG(0);
    c->r[6] = ADHOC;
    if (what == 0) { *W(ADHOC + 4) = ARG(1); c->r[4] = *W(ADHOC + 16) | 1; *W(ADHOC + 16) = c->r[4]; return; }
    c->r[5] = 2;
    if (what == 1) { *B(ADHOC + 190) = 1; c->r[5] = 1; c->r[4] = *W(ADHOC + 16) | 2; *W(ADHOC + 16) = c->r[4]; return; }
    c->r[5] = 3;
    if (what == 2) { *B(ADHOC + 190) = 0; c->r[4] = *W(ADHOC + 16) | 4; *W(ADHOC + 16) = c->r[4]; return; }
    if (what == 3) { c->r[4] = *W(ADHOC + 16) | 8; *W(ADHOC + 16) = c->r[4]; }
}

/* (): 1 while the link is busy (+188); else its thread (+0) waited for, ended and deleted (+0 = -1), and
   its data (+4) answered. */
SFA_FUNC(088DEC74, adhoc_end_thread) {
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
        }
        RETURN(*W(ADHOC + 4));
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (target): the matching (+8) asked to pair with the target (its MAC at +0, 4 bytes from +0x88): +140 = 2
   when it was sent (or already pending, 0x80410810); else the link's first error kept (+4). */
SFA_FUNC(088DF188, adhoc_select_target) {
    gaddr t = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = t; c->r[17] = ADHOC;
    int32_t r = os_net_adhoc_matching_select_target(c, (int32_t)*W(ADHOC + 8), t, 4, t + 0x88);
    c->r[4] = 2;
    if (r >= 0 || (uint32_t)r == 0x80410810u) *W(t + 140) = 2;
    else {
        c->r[4] = *W(ADHOC + 4);
        if (c->r[4] == 0) *W(ADHOC + 4) = (uint32_t)r;
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (): 1 unless the session's choice is on (+48) and its members' bytes +39 + n (n from the session's
   end down to 1) and +38 are not all the same as +39. */
SFA_FUNC(088E36EC, session_all_agree) {
    gaddr s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = NSTATE;
    int ok = 1;
    if (S8(NSTATE + 48) != 0) {
        f_088DD3DC(c);
        int32_t n = (int32_t)c->r[2];
        int8_t first = S8(NSTATE + 39);
        c->r[4] = (uint32_t)(int32_t)first;
        for (; n > 0; n--) {
            c->r[6] = (uint32_t)(int32_t)S8(NSTATE + (uint32_t)n + 39);
            if ((int8_t)c->r[6] != first) { ok = 0; break; }
        }
        c->r[5] = (uint32_t)(n - (ok ? 0 : 1));
        if (ok && S8(NSTATE + 38) != first) ok = 0;
    }
    RETURN(ok);
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (box, x, y, byte, kind): a text record (text_record_take) for an open box (+13 set, +14 clear) with its
   word the box's +4 rounded up past 32 bytes; the box's +12 = 4. */
SFA_FUNC(088DDAEC, box_text_record) {
    gaddr box = ARG(0);
    int16_t x = (int16_t)ARG(1), y = (int16_t)ARG(2);
    uint32_t byte = ARG(3) & 0xFF, kind = c->r[8] & 0xFF;
    c->r[29] -= 0x30;
    c->r[9] = c->r[8]; c->r[10] = ARG(3);
    c->r[8] = (uint32_t)(int32_t)x; c->r[7] = (uint32_t)(int32_t)y; c->r[6] = byte; c->r[5] = kind;
    if (box != 0 && S8(box + 13) != 0) {
        c->r[9] = (uint32_t)(int32_t)S8(box + 14);
        if (c->r[9] == 0) {
            *B(box + 12) = 4;
            c->r[10] = 4; c->r[11] = 0xFFFFFFE0u;
            c->r[4] = kind; c->r[5] = (uint32_t)(int32_t)x; c->r[6] = (uint32_t)(int32_t)y;
            c->r[7] = (*W(box + 4) & 0xFFFFFFE0u) + 0x20;
            c->r[8] = byte;
            f_0880A740(c);
        } else c->r[9] = byte;
    }
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- objects */

/* (owner, object): the object set from the owner: +121 / +122, +346 from +194, +80, +11 - +14, +38,
   +16 / +20; +347 / +255 / +112 cleared, +88 = the owner; the owner's +228 = the object and its count
   +536 one more. */
SFA_FUNC(088F84BC, object_from_owner) {
    gaddr s = ARG(0), d = ARG(1);
    *B(d + 122) = *B(s + 122);
    *B(d + 121) = *B(s + 121);
    *B(d + 346) = *B(s + 194);
    *B(d + 347) = 0;
    *B(d + 80) = *B(s + 80);
    *B(d + 255) = 0;
    *B(d + 14) = *B(s + 14);
    *B(d + 11) = *B(s + 11);
    *B(d + 12) = *B(s + 12);
    *B(d + 13) = *B(s + 13);
    *H(d + 38) = *H(s + 38);
    *W(d + 16) = *W(s + 16);
    *H(d + 112) = 0;
    *W(d + 20) = *W(s + 20);
    *W(d + 88) = s;
    *W(s + 228) = d;
    c->r[6] = *B(s + 536);
    c->r[5] = c->r[6] + 1;
    *B(s + 536) = (uint8_t)c->r[5];
}

/* (object, record): until the record's +2 reaches +119 its part (+5) from 0x08AB6758 (with the record),
   then drawn; after, step 2. */
SFA_FUNC(089D4CA4, parts_until_119) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = r;
    if (S16(r + 2) < *B(o + 119)) {
        gaddr h = *W(0x08AB6758u + *B(o + 5) * 4u);
        uint32_t ra = c->r[31];
        c->r[4] = o; c->r[5] = r; c->r[6] = h; c->r[31] = 0x089D4CF8u;
        rt_call_indirect(c, h);
        c->r[31] = ra;
        c->r[4] = o; f_088215C4(c);
    } else { c->r[4] = 2; *B(o + 4) = 2; }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4) once the record's +2 reaches +119; before it its part (+5) from
   0x08AB6B10 (with the record), drawn if on screen. */
SFA_FUNC(089DA7F0, parts_then_draw_until_119) {
    gaddr o = ARG(0), r = ARG(1), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = r;
    if (!(S16(r + 2) < *B(o + 119))) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        gaddr h = *W(0x08AB6B10u + *B(o + 5) * 4u);
        uint32_t ra = c->r[31];
        c->r[4] = o; c->r[5] = r; c->r[6] = h; c->r[31] = 0x089DA854u;
        rt_call_indirect(c, h);
        c->r[31] = ra;
        c->r[4] = o; f_08821424(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4) once the record's +2 reaches +119; before it its part (+5) from
   0x08AB6C80 (with the record). */
SFA_FUNC(089DB780, parts_until_119_undrawn) {
    gaddr o = ARG(0), r = ARG(1);
    c->r[29] -= 0x20;
    c->r[5] = o; c->r[4] = r;
    if (!(S16(r + 2) < *B(o + 119))) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else {
        gaddr h = *W(0x08AB6C80u + *B(o + 5) * 4u);
        uint32_t ra = c->r[31];
        c->r[4] = o; c->r[5] = r; c->r[6] = r; c->r[7] = h; c->r[31] = 0x089DB7F0u;
        rt_call_indirect(c, h);
        c->r[31] = ra;
    }
    c->r[29] += 0x20;
}

/* (record, -, fighter): unless the record's +43 / +40 / +72, when its side's bits 4 - 9 at 0x08E75BAC
   (0x08E75BD8 for the second side, +337) are up, the fighter's +348 counts down: +346 = 0x7F and
   +350 = 1 while it lasts, +346 = 1 and +350 cleared at 0. */
SFA_FUNC(08834298, fighter_hold_348) {
    gaddr r = ARG(0), f = ARG(2);
    if (S8(r + 43) != 0 || S8(r + 40) != 0) return;
    c->r[5] = 0x08E70000u;
    if (S8(r + 72) != 0) return;
    c->r[5] = 0x08E75BA0u;
    c->r[7] = (uint32_t)(int32_t)S8(f + 337);
    uint32_t bits = (c->r[7] == 0 ? *W(0x08E75BA0u + 12) : *W(0x08E75BA0u + 56)) & 0xFFFF;
    c->r[4] = bits & 0x3F0;
    if (c->r[4] == 0) return;
    uint32_t n = *H(f + 348);
    c->r[4] = 1;
    if (n == 0) { *H(f + 350) = 0; *H(f + 346) = 1; c->r[5] = 0; return; }
    *H(f + 348) = (uint16_t)(n - 1);
    *H(f + 350) = 1;
    *H(f + 346) = 0x7F;
    c->r[5] = 0x7F;
}

/* (object): +18 from the owner (+88), step 1, +86 = 4; the animation for its type (+3) from 0x08A165A8,
   the second of the pair once the owner's +22 is from 65 down; drawn if on screen. */
SFA_FUNC(08853F3C, start_by_owner_height) {
    gaddr o = ARG(0), owner = *W(o + 88), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 18) = (uint16_t)S16(owner + 18);
    *B(o + 4) = 1;
    *H(o + 86) = 4;
    gaddr t = 0x08A165A8u + *B(o + 3) * 4u;
    c->r[6] = S16(owner + 22) < 65; c->r[7] = 1;
    c->r[5] = c->r[6] ? *W(t) : *W(t + 4);
    c->r[4] = o; f_0882196C(c);
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +82 counts down; at 0, unless +73 is set, the next part (+5): +148 = 11, +156 = 0x08A16962,
   animation 0x08A16A90's eighteenth, drawn. */
SFA_FUNC(08857008, next_part_after_82) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    c->r[4] = (uint32_t)(int32_t)n;
    if (n == 0) {
        c->r[4] = (uint32_t)(int32_t)S8(o + 73);
        if (c->r[4] == 0) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *W(o + 148) = 0xB;
            *W(o + 156) = 0x08A16962u;
            c->r[4] = o; c->r[5] = *W(0x08A16A90u + 68);
            f_0882196C(c);
            c->r[4] = o; f_088215C4(c);
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): before intro stage 4 (BATTLE +44) drawn; from it the next part (+5), and when layout 3's
   fighter (fighter_by_layout_3) is not the CPU, part 3, +82 = 18 and a list-3 object kind 5 type 8
   with this one as its owner (+124); drawn when it is. */
SFA_FUNC(0885708C, make_5_8_at_stage_4) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int draw = 1;
    if (!(S8(BATTLE + 44) < 4)) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        c->r[4] = o;
        f_08857678(c);
        c->r[4] = 3;
        if (!(S8(c->r[2] + 373) > 0)) {
            draw = 0;
            *B(o + 5) = 3;
            *H(o + 82) = 0x12;
            c->r[4] = 5; c->r[5] = 8;
            f_08823338(c);
            c->r[4] = c->r[2];
            if (c->r[2] != 0) *W(c->r[2] + 124) = o;
        }
    }
    if (draw) { c->r[4] = o; f_088215C4(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): placed at the owner's (+88) point for its type (+3) and the owner's frame kind (+296 -> +9),
   from the routine table's +40 list (4 further on with GAME +59 bit 0), mirrored with the owner's
   facing (+11); its own +11 the other way. */
SFA_FUNC(08880A30, place_at_owner_point) {
    gaddr o = ARG(0), owner = *W(o + 88);
    uint32_t i = ((*B(*W(owner + 296) + 9) & 0x1Fu) << 3) + *B(o + 3);
    c->r[8] = GAME;
    if (S8(GAME + 59) & 1) i += 4;
    gaddr p = *W(*W(owner + 416) + 40) + i * 2;
    int32_t dx = S16(p);
    uint8_t face = *B(owner + 11);
    *B(o + 11) = face;
    if (face != 0) dx = -dx;
    *H(o + 18) = (uint16_t)(S16(owner + 18) + dx);
    c->r[6] = (uint32_t)(int32_t)S16(owner + 22);
    c->r[5] = (uint32_t)(S16(owner + 22) + S16(p + 2));
    *H(o + 22) = (uint16_t)c->r[5];
    c->r[9] = *B(o + 11);
    c->r[7] = c->r[9] ^ 1;
    *B(o + 11) = (uint8_t)c->r[7];
}

/* (object): placed as kind 0 at (0x174, 0), the next step (+4), +148 = 1, one of the first two of
   0x08A29870 at random, +22 down by +77, +100 = -1.5. */
SFA_FUNC(0889D694, start_falling_random) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = 0x174; c->r[7] = 0;
    f_0889DB8C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 1;
    f_08821368(c);
    c->r[4] = o; c->r[5] = *W(0x08A29870u + (c->r[2] & 1) * 4);
    f_0882196C(c);
    c->r[5] = (uint32_t)(int32_t)S16(o + 22);
    *H(o + 22) = (uint16_t)(S16(o + 22) + *B(o + 77));
    c->r[4] = 0xFFFE8000u;
    *W(o + 100) = 0xFFFE8000u;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): its animation 0x08A3F748 (shown, +6 = 1) or 0x08A3F74C (+6 = 0) by whether the level byte
   0x08B03F8C is at least `from`, set only when it changes; +1 = 1. */
static void animation_by_level(Cpu *c, uint32_t from) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 1;
    uint32_t shown = *B(o + 6);
    c->r[4] = shown;
    if (!(*B(0x08B03F8Cu) < from)) {
        c->r[5] = 1;
        if (shown != 1) {
            c->r[4] = o; c->r[5] = *W(0x08A3F740u + 8);
            f_0882196C(c);
            *B(o + 6) = 1;
        }
    } else if (shown != 0) {
        c->r[4] = o; c->r[5] = *W(0x08A3F740u + 12);
        f_0882196C(c);
        *B(o + 6) = 0;
    }
    *B(o + 1) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(088ECAA8, animation_by_level_0) { animation_by_level(c, 0); }
SFA_FUNC(088ECB2C, animation_by_level_1) { animation_by_level(c, 1); }
SFA_FUNC(088ECBB0, animation_by_level_2) { animation_by_level(c, 2); }
SFA_FUNC(088ECC38, animation_by_level_3) { animation_by_level(c, 3); }

/* (object): a box's settings: +2 cleared, +55 = 7, +56 = 7, +136 = 0xA0, +6 = 0x70, +14 = 0xA0, +26 = 12,
   +12 / +24 cleared, the colour bytes +36 - +39 = 0, +42 - +45 and +48 - +51 = 0xA0; then its step. */
SFA_FUNC(088885D4, box_setup_a0) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *B(o + 55) = 7; *B(o + 56) = 7;
    *B(o + 2) = 0;
    *B(o + 136) = 0xA0;
    *H(o + 6) = 0x70;
    *H(o + 24) = 0; *H(o + 12) = 0;
    *H(o + 14) = 0xA0;
    *H(o + 26) = 0xC;
    for (uint32_t k = 36; k <= 39; k++) *B(o + k) = 0;
    for (uint32_t k = 42; k <= 45; k++) *B(o + k) = 0xA0;
    for (uint32_t k = 48; k <= 51; k++) *B(o + k) = 0xA0;
    c->r[5] = 0xA0; c->r[6] = 0xA0; c->r[7] = 0xC;
    f_088888B4(c);
    c->r[29] += 0x20;
}

/* (object): the same with +56 = 5, +10 = 0xA0, +14 / +26 cleared, colours 0x80 / 0x20 / 0x80. */
SFA_FUNC(08888548, box_setup_80) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *B(o + 55) = 7; *B(o + 56) = 5;
    *B(o + 2) = 0;
    *B(o + 136) = 0xA0;
    *H(o + 6) = 0x70;
    *H(o + 10) = 0xA0;
    *H(o + 14) = 0; *H(o + 12) = 0; *H(o + 26) = 0; *H(o + 24) = 0;
    for (uint32_t k = 36; k <= 39; k++) *B(o + k) = 0x80;
    for (uint32_t k = 42; k <= 45; k++) *B(o + k) = 0x20;
    for (uint32_t k = 48; k <= 51; k++) *B(o + k) = 0x80;
    c->r[5] = 0x80; c->r[6] = 0x20;
    f_088888B4(c);
    c->r[29] += 0x20;
}

/* (object): another box's settings: +93 / +89 / +137 / +56 = 1, +3 / +88 cleared, +55 = 7, +10 = 9,
   +14 = 0x280, +26 = 0x20, +6 / +12 / +24 cleared, +136 = 0xC0, colours 0x76 / 0x76 / 0xBC. */
SFA_FUNC(08888828, box_setup_76) {
    gaddr o = ARG(0);
    *B(o + 93) = 1; *B(o + 3) = 0; *B(o + 89) = 1; *B(o + 137) = 1; *B(o + 88) = 0;
    *B(o + 55) = 7;
    *H(o + 6) = 0; *H(o + 10) = 9; *H(o + 24) = 0; *H(o + 12) = 0;
    *H(o + 14) = 0x280; *H(o + 26) = 0x20;
    *B(o + 56) = 1;
    *B(o + 136) = 0xC0;
    for (uint32_t k = 36; k <= 39; k++) *B(o + k) = 0x76;
    for (uint32_t k = 42; k <= 45; k++) *B(o + k) = 0x76;
    for (uint32_t k = 48; k <= 51; k++) *B(o + k) = 0xBC;
    c->r[5] = 0x76; c->r[6] = 0xBC; c->r[7] = 0x20;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter, default): 2 for the CPU (+373) outside mode 0 against character 40 in a layout (BATTLE +56)
   other than 0, or against character 41 with BATTLE +66 = 1; else the default. */
SFA_FUNC(088983D0, cpu_special_opponent) {
    gaddr f = ARG(0), opp = *W(f + 92);
    c->r[4] = opp;
    RETURN(ARG(1));
    if (S8(f + 373) == 0) return;
    if (S8(GAME + 114) != 0) return;
    int32_t ch = S8(opp + 338);
    c->r[4] = (uint32_t)ch;
    if (ch == 40) {
        if (S16(BATTLE + 56) != 0) { RETURN(2); return; }
        return;
    }
    c->r[6] = 0x29;
    if (ch != 41) return;
    c->r[4] = 1;
    if (S8(BATTLE + 66) == 1) RETURN(2);
}

/* A script command that reads on or leaves its block, as the originals set the registers. */
static void read_or_leave(Cpu *c, int leave, gaddr f, gaddr p) {
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (leave) f_08879C8C(c); else f_0887A10C(c);
}

/* (fighter, script): reads on when the opponent (+92) has +73 = 1 and +78 clear, else leaves the block. */
SFA_FUNC(08878FC0, script_if_opponent_73) {
    gaddr f = ARG(0), p = ARG(1), opp = *W(f + 92);
    c->r[29] -= 0x20;
    c->r[7] = p; c->r[9] = 1;
    c->r[8] = (uint32_t)(int32_t)S8(opp + 73);
    read_or_leave(c, !(S8(opp + 73) == 1 && S8(opp + 78) == 0), f, p);
    c->r[29] += 0x20;
}

/* (fighter, script): with objects out (+536) and one at +228, its script part 9 (+482 cleared, +483 = 9)
   back to routine 1 (fighter_to_routine_1); else it reads on. */
SFA_FUNC(08877AD4, script_if_object_out) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    c->r[7] = *B(f + 536);
    if (c->r[7] != 0 && *W(f + 228) != 0) {
        c->r[6] = p;
        *B(f + 482) = 0;
        *B(f + 483) = 9;
        c->r[4] = f; c->r[5] = p;
        f_08877F44(c);
    } else {
        c->r[6] = p;
        c->r[4] = f; c->r[5] = p;
        f_0887A10C(c);
    }
    c->r[29] += 0x20;
}

/* (fighter, script): +490 = the script's count and +506 its next word; below +508 it reads on, else its
   script ends there (routine 1 / 0 / 2 / 0, +482 = 1, +483 = 6, +78 / +497 = 1). Returns the script
   after both. */
SFA_FUNC(08878244, script_count_to_508_6) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    int16_t v = S16(p);
    *H(f + 506) = (uint16_t)S16(p + 2);
    int16_t d = (int16_t)(v - S16(f + 508));
    *H(f + 490) = (uint16_t)v;
    c->r[6] = (uint32_t)(int32_t)d; c->r[8] = (uint32_t)(int32_t)S16(f + 508);
    RETURN(p + 4);
    if (d < 0) {
        c->r[5] = p + 4;
        f_0887A10C(c);
    } else {
        *B(f + 482) = 1; *B(f + 483) = 6;
        *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
        *B(f + 78) = 1; *B(f + 497) = 1;
        c->r[5] = 1; c->r[6] = 2;
    }
    c->r[29] += 0x20;
}
