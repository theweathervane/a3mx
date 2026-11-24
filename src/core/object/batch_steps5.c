/* Small steps (0.133.0): the last 15 of the profile's ready functions of 17 - 24 instructions and the
   first 20 of 25 - 40. Objects bouncing, rising, sliding and starting their animations, the ending's
   next animation, fades for a side, fighters' script branches and CPU choices, the session's members by
   id and by address, its lock and error codes, a character's routine table. Offsets stay numbers until
   something shows what they are. What the original leaves in a0 - t0 at a call is set as the original
   sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define SESSION   0x09523820u   /* the network session's records */

void f_089CEFC8(Cpu *c);   /* bounce_by_88 */
void f_088219B0(Cpu *c);   /* object_animate */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_08881D9C(Cpu *c);   /* setup_kind_at */
void f_0889DB8C(Cpu *c);   /* place_kind */
void f_0882DE00(Cpu *c);   /* fade_slots */
void f_089F33E4(Cpu *c);   /* game_memcmp */
void f_089DC074(Cpu *c);   /* ending_animation */
void f_089DA5DC(Cpu *c);   /* step_089DA5DC */
void f_0896A6C4(Cpu *c);
void f_08821368(Cpu *c);   /* game_random */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_088214E8(Cpu *c);   /* object_draw_if_on_screen_margin */
void f_0887DF88(Cpu *c);   /* object_set_animation_n */
void f_08881F70(Cpu *c);   /* make_beside_15 */
void f_0887684C(Cpu *c);   /* fighter_set_frame */
void f_088768F0(Cpu *c);   /* fighter_next_frame */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_08961A6C(Cpu *c);   /* move_accelerating */
void f_08961778(Cpu *c);   /* step_08961778 */
void f_08961910(Cpu *c);   /* step_08961910 */

/* Calls a handler from a table with a1 holding it, as an object's step does; `back` is where the
   original's call returns to. */
static void call_handler(Cpu *c, gaddr handler, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[5] = handler;
    c->r[31] = back;
    rt_call_indirect(c, handler);
    c->r[31] = ra;
}

/* ---------------------------------------------------------------- the network session */

/* The session member with this id (+228): the session itself, then its list (+232); the list's last
   when none has it, 0 when the list is empty (as 0x088DD23C). */
static gaddr session_find(Cpu *c, uint32_t id) {
    gaddr p = SESSION;
    if (*W(SESSION + 228) != id) {
        p = *W(SESSION + 232);
        while (p != 0 && *W(p + 228) != id) {
            gaddr next = *W(p);
            if (next == 0) break;
            p = next;
        }
    }
    c->r[5] = p;
    return p;
}
/* (id): that member's +0x98 / +0xAC. */
SFA_FUNC(088DD2A8, session_member_98) { gaddr p = session_find(c, ARG(0)); RETURN(p != 0 ? p + 0x98 : 0); }
SFA_FUNC(088DD314, session_member_AC) { gaddr p = session_find(c, ARG(0)); RETURN(p != 0 ? p + 0xAC : 0); }

/* (address): the first of the session's list (+232) whose six bytes at +4 are this address; 0 if none. */
SFA_FUNC(088DCE0C, session_member_by_address) {
    gaddr key = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = key;
    gaddr p = *W(SESSION + 232);
    c->r[17] = p;
    while (p != 0) {
        c->r[4] = key; c->r[5] = p + 4; c->r[6] = 6;
        f_089F33E4(c);
        if (c->r[2] == 0) break;
        p = *W(p);
        c->r[17] = p;
        c->r[5] = p + 4;
    }
    RETURN(p);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (dst, n): eight half-words from the n-th 20-byte record at 0x08A058A0. */
SFA_FUNC(088D0BA8, copy_record_8) {
    gaddr d = ARG(0), s = 0x08A058A0u + (ARG(1) & 0xFF) * 20;
    for (uint32_t k = 0; k < 16; k += 2) *H(d + k) = *H(s + k);
    c->r[5] = *H(s + 14); c->r[6] = *H(s + 12);
}

/* ---------------------------------------------------------------- objects */

/* (object): the first time (+5 from 0) +88 = -(+100), then bouncing (bounce_by_88); animated and drawn. */
SFA_FUNC(089CD960, bounce_then_draw) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint8_t part = *B(o + 5);
    if (part == 0) {
        *B(o + 5) = 1;
        *W(o + 88) = 0u - *W(o + 100);
        c->r[5] = o + 88; c->r[4] = *W(o + 88);
    } else {
        c->r[4] = o; f_089CEFC8(c);
    }
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +20 moved by +100; while it is not negative +22 rises by +77, +74 = 1 and it is animated;
   drawn. */
SFA_FUNC(0889D5DC, rise_by_77_then_draw) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t y = (int32_t)(*W(o + 20) + *W(o + 100));
    *W(o + 20) = (uint32_t)y;
    c->r[5] = (uint32_t)y;
    if (!(y < 0)) {
        *H(o + 22) = (uint16_t)(S16(o + 22) - *B(o + 77));
        *B(o + 74) = 1;
        c->r[5] = 1;
        c->r[4] = o; f_088219B0(c);
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, record): the next step (+4), +14 / +38 cleared, the animation for +116 from the ending's
   list, then the step 0x089DA5DC with the record. */
SFA_FUNC(089DA2DC, ending_next_animation_cleared) {
    gaddr o = ARG(0), a1 = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = a1;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 14) = 0;
    *H(o + 38) = 0;
    c->r[4] = o; c->r[5] = *B(o + 116);
    f_089DC074(c);
    c->r[4] = o; c->r[5] = a1;
    f_089DA5DC(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): GAME +93 and +98 = 1; +110 the top four bits of +66, and for kinds 1 and 3 +162 - +168 from
   +68 - +74. */
SFA_FUNC(089DFBAC, set_kind_from_66) {
    gaddr o = ARG(0);
    *B(GAME + 93) = 1;
    *B(o + 98) = 1;
    uint32_t kind = (uint32_t)((int32_t)S16(o + 66) >> 12) & 0xF;
    *H(o + 110) = (uint16_t)kind;
    c->r[5] = kind; c->r[6] = 1;
    if (kind != 1) { c->r[6] = 3; if (kind != 3) return; }
    *H(o + 162) = (uint16_t)S16(o + 68);
    *H(o + 164) = (uint16_t)S16(o + 70);
    *H(o + 166) = (uint16_t)S16(o + 72);
    *H(o + 168) = (uint16_t)S16(o + 74);
    c->r[5] = (uint32_t)(int32_t)S16(o + 72); c->r[6] = (uint32_t)(int32_t)S16(o + 74);
}

/* (object): step 1, +15 = 1, +12 cleared, +86 = 4, the animation for its type (+3) from 0x08A165A8;
   drawn if on screen. */
SFA_FUNC(08853E34, start_animation_by_type_15) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 15) = 1;
    *B(o + 4) = 1;
    *B(o + 12) = 0;
    *H(o + 86) = 4;
    c->r[5] = *W(0x08A165A8u + *B(o + 3) * 4u);
    c->r[4] = o; f_0882196C(c);
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +18 = 0x20, or 0x120 for type (+3) not 0; from intro stage 4 (BATTLE +44) the next part
   (+7) with +127 / +119 = 21; drawn. */
SFA_FUNC(088580C4, place_until_stage_4) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *H(o + 18) = *B(o + 3) != 0 ? 0x120 : 0x20;
    c->r[5] = (uint32_t)(S8(BATTLE + 44) < 4);
    if (c->r[5] == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 127) = 0x15;
        *B(o + 119) = 0x15;
        c->r[6] = 0x15;
    }
    f_088215C4(c);
    c->r[29] += 0x20;
}

/* (object): the next step (+4); set up as kind 3 at (0xC0, 0x78), +148 cleared, +86 = 13, +164 = 1,
   animation 0x08A24A50. */
SFA_FUNC(08881708, start_kind_3) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[5] = 3; c->r[6] = 0xC0; c->r[7] = 0x78;
    f_08881D9C(c);
    *W(o + 148) = 0;
    *H(o + 86) = 0xD;
    *W(o + 164) = 1;
    c->r[4] = o; c->r[5] = 0x08A24A50u;
    f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): placed as kind 0 at (8, -224), the next step (+4), +148 = 1, animation 0x08A29870's sixth,
   +100 = 1.0. */
SFA_FUNC(0889D9C4, start_from_above) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 0; c->r[6] = 8; c->r[7] = (uint32_t)-224;
    f_0889DB8C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 1;
    c->r[4] = o; c->r[5] = *W(0x08A29870u + 20);
    f_0882196C(c);
    c->r[4] = 0x10000;
    *W(o + 100) = 0x10000;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (level): the fade of fighter 1's slots (0 - 7) set to level unless its +237 is set; failing that,
   fighter 2's (0x40 - 0x47) unless its +237 is. */
SFA_FUNC(0882DED8, fade_side) {
    uint32_t level = ARG(0);
    c->r[29] -= 0x20;
    if (S8(FIGHTER_1 + 237) == 0) {
        c->r[4] = 0; c->r[5] = 8; c->r[6] = level;
        f_0882DE00(c);
    } else {
        c->r[6] = level;
        c->r[5] = (uint32_t)(int32_t)S8(FIGHTER_2 + 237);
        if (c->r[5] == 0) {
            c->r[4] = 0x40; c->r[5] = 8;
            f_0882DE00(c);
        }
    }
    c->r[29] += 0x20;
}

/* (fighter): from type 3 (+3) up, +349 flips between 0 and 1 and +13 = +349 + +350; then its step by
   +5 from 0x08A4991C. */
SFA_FUNC(088FBFD4, flip_349_then_step) {
    gaddr f = ARG(0);
    if (!(*B(f + 3) < 3)) {
        *B(f + 349) = (uint8_t)(S8(f + 349) ^ 1);
        *B(f + 13) = (uint8_t)(S8(f + 349) + S8(f + 350));
    }
    run_step(c, *B(f + 5), 0x08A4991Cu, 5, 0x088FC028u);
}

/* (fighter): +171 cleared, +12 = -1, the 39 words at 0x08A8B6A8 copied to its routine table (+416),
   +164 = 1, then 0x0896A6C4 with 0x18. */
SFA_FUNC(0896688C, fighter_table_from_8B6A8) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    *B(f + 171) = 0;
    *B(f + 12) = 0xFF;
    gaddr d = *W(f + 416), s = 0x08A8B6A8u;
    for (uint32_t i = 0; i < 0x27; i++) *W(d + i * 4) = *W(s + i * 4);
    c->r[6] = s + 0x27 * 4; c->r[7] = 0x27; c->r[8] = 0;
    *W(f + 164) = 1;
    c->r[5] = 0x18;
    f_0896A6C4(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- the first of 25 - 40 instructions */

/* (object): +82 a random byte masked and offset by its type's pair (+3) at 0x08A154A0. */
SFA_FUNC(0884FC04, random_82_by_type) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_08821368(c);
    gaddr t = 0x08A154A0u + *B(o + 3) * 4u;
    int16_t v = (int16_t)((int16_t)(c->r[2] & 0xFF) & S16(t));
    c->r[5] = (uint32_t)(int32_t)v;
    c->r[4] = (uint32_t)(v + S16(t + 2));
    *H(o + 82) = (uint16_t)c->r[4];
    c->r[29] += 0x10;
    c->r[16] = s16;
}

/* (object): at intro stage 2 (BATTLE +44) the next part (+5), +82 = 11, and a list-3 object kind 5
   type 7 with this one as its owner (+124). */
SFA_FUNC(08856FA4, make_5_7_at_stage_2) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[6] = (uint32_t)(int32_t)S8(BATTLE + 44); c->r[5] = 2;
    if (c->r[6] == 2) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0xB;
        c->r[4] = 5; c->r[5] = 7;
        f_08823338(c);
        c->r[4] = c->r[2];
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter, script): reads on when the opponent's (+92) health (+112) is below its own, else leaves the
   block. */
SFA_FUNC(088792C4, script_if_ahead_on_health) {
    gaddr f = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    int32_t mine = *H(f + 112), theirs = S16(*W(f + 92) + 112);
    c->r[7] = (uint32_t)theirs; c->r[8] = p;
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (theirs < mine) f_0887A10C(c); else f_08879C8C(c);
    c->r[29] += 0x20;
}

/* (object): the next step (+4); a list-3 object kind 1 (+90 = 1, +8 from this one's +20, +108 cleared,
   +26 = 0x40); this one freed. */
SFA_FUNC(088817B0, make_1_then_free) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = 1; c->r[5] = 0;
    f_08823338(c);
    gaddr n = c->r[2];
    c->r[4] = n; c->r[5] = 1;
    if (n != 0) {
        *B(n + 90) = 1;
        *W(n + 8) = *W(o + 20);
        *W(n + 108) = 0;
        *H(n + 26) = 0x40;
        c->r[5] = *W(o + 20); c->r[6] = 0x40;
    }
    c->r[4] = o; f_088231C8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* Set up at (0, 400) (y from +22), the next step (+4), +96 = -16.0, +100 = `vy`, the animation 0x08A299FC
   + `anim`. */
static void start_thrown(Cpu *c, uint32_t vy, uint32_t anim) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[7] = (uint32_t)(int32_t)S16(o + 22);
    c->r[5] = 0; c->r[6] = 0x190;
    f_08881D9C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 96) = 0xFFF00000u;
    *W(o + 100) = vy;
    c->r[4] = o; c->r[5] = *W(0x08A299FCu + anim);
    f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(08881848, start_thrown_14000) { start_thrown(c, 0x14000, 28); }
SFA_FUNC(088819F4, start_thrown_18000) { start_thrown(c, 0x18000, 32); }

/* (record): +20, +24, +25 cleared, +0 - +10 = 0 - 5, the three 60-byte rows from +28 cleared. */
SFA_FUNC(088D6440, record_clear_rows) {
    gaddr r = ARG(0);
    *H(r + 20) = 0; *B(r + 25) = 0; *B(r + 24) = 0;
    for (uint32_t i = 0; i < 6; i++) *H(r + i * 2) = (uint16_t)i;
    for (uint32_t k = 0; k < 3; k++)
        for (uint32_t i = 0; i < 0x3C; i++) *B(r + 0x1C + k * 0x3C + i) = 0;
    c->r[4] = r + 0x1C + 2 * 0x3C; c->r[5] = 3; c->r[6] = 0; c->r[7] = c->r[4] + 0x3B; c->r[8] = 0;
}

/* (data): under the lock (semaphore 0x08B02EA4), the word 8 before it marked 0x1AE49CF0 if it was
   0x1AE49CF1. */
SFA_FUNC(088DC2C4, unmark_under_lock) {
    gaddr p = ARG(0) - 8, s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = p; c->r[16] = 0x08B00000u;
    os_wait_sema(c, (int32_t)*W(0x08B02EA4u), 1, 0);
    if (*W(p) == 0x1AE49CF1u) *W(p) = 0x1AE49CF0u;
    os_signal_sema(c, (int32_t)*W(0x08B02EA4u), 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (code): the game's number for a result: n + 1 for 0 and up; for an error the one paired with it in
   the eleven at 0x08A3EBD8 (+ 1), else 0x12 for the 0x8041xxxx errors and 1 for the rest. */
SFA_FUNC(088E3B28, result_number) {
    uint32_t code = ARG(0);
    if ((int32_t)code >= 0) { RETURN(code + 1); return; }
    for (gaddr p = 0x08A3EC28u; p >= 0x08A3EBD8u; p -= 8)
        if (*W(p) == code) { RETURN(*W(p + 4) + 1); return; }
    RETURN((code & 0xFFFF0000u) == 0x80410000u ? 0x12 : 1);
}

/* (fighter): +696 cleared; its step by +7 from 0x08A53884 when +193 is set, else from 0x08A53878. */
SFA_FUNC(08905BC4, step_by_193) {
    gaddr f = ARG(0);
    *B(f + 696) = 0;
    if (S8(f + 193) != 0) run_step(c, *B(f + 7), 0x08A53884u, 5, 0x08905BF8u);
    else run_step(c, *B(f + 7), 0x08A53878u, 5, 0x08905C1Cu);
}

/* (object): the next step (+4) while BATTLE +42 is set; else the step 0x08961778 for type 0, or
   0x08961910. */
SFA_FUNC(08961714, step_by_type_0) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (S8(BATTLE + 42) != 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else if (*B(o + 3) == 0) f_08961778(c);
    else f_08961910(c);
    c->r[29] += 0x20;
}

/* (object): its step by +5 from 0x08A154C0 unless BATTLE +37 or +52 holds it; then drawn if within 0x58
   of the screen. */
SFA_FUNC(0884FCF4, step_then_draw_58) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if ((S8(BATTLE + 37) | S8(BATTLE + 52)) == 0) {
        c->r[4] = o;
        call_handler(c, *W(0x08A154C0u + *B(o + 5) * 4u), 0x0884FD40u);
    }
    c->r[4] = o; c->r[5] = 0x58;
    f_088214E8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): its step by +3 from 0x08A15478 unless BATTLE +37 or +52 holds it; then drawn if within its
   +128 of the screen. */
SFA_FUNC(0884F8AC, step_by_type_then_draw) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if ((S8(BATTLE + 37) | S8(BATTLE + 52)) == 0) {
        c->r[4] = o;
        call_handler(c, *W(0x08A15478u + *B(o + 3) * 4u), 0x0884F8F8u);
    }
    c->r[4] = o; c->r[5] = (uint32_t)(int32_t)S16(o + 128);
    f_088214E8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): step 2 while BATTLE +36 hides the fight's objects; else, for types (+3) other than 11 and
   12, its handler from 0x08A24490. */
SFA_FUNC(0887E984, step_by_type_unless_hidden) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    if (S8(BATTLE + 36) == 0) { c->r[5] = 2; *B(o + 4) = 2; }
    else {
        uint32_t t = *B(o + 3);
        c->r[5] = t; c->r[6] = 0xC;
        if (t != 11 && t != 12) {
            c->r[6] = 0x08A24490u;
            call_handler(c, *W(0x08A24490u + t * 4), 0x0887E9D0u);
        }
    }
    c->r[29] += 0x20;
}

/* (object, n): animation 0x30 + 2n + a random one of the sixteen bytes at 0x08A24A3C. */
SFA_FUNC(08880B1C, random_animation) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t n = (int16_t)ARG(1);
    c->r[17] = (uint32_t)n;
    f_08821368(c);
    int16_t a = (int16_t)(S8(0x08A24A3Cu + (c->r[2] & 0xF)) + n * 2 + 0x30);
    c->r[4] = o; c->r[5] = (uint16_t)a;
    f_0887DF88(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +5 = 5, +16 = 12; its six 16-byte parts from +32 on (+1 = 1, +2 / +4 from 0x08A257E8). */
SFA_FUNC(08886590, six_parts) {
    gaddr o = ARG(0);
    *B(o + 5) = 5;
    *B(o + 16) = 0xC;
    for (uint32_t i = 0; i < 6; i++) {
        gaddr p = o + 0x20 + i * 16, t = 0x08A257E8u + i * 4;
        *B(p + 1) = 1;
        *H(p + 2) = (uint16_t)S16(t);
        *H(p + 4) = (uint16_t)S16(t + 2);
    }
    c->r[4] = o + 0x10; c->r[5] = 1; c->r[6] = 6; c->r[7] = o + 0x20 + 6 * 16;
}

/* (object): +82 counts down, past 0 the next part (+5) and 40 more; moved with growing speeds, animated
   and drawn. */
SFA_FUNC(0896181C, count_82_then_move) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int16_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n < 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *H(o + 82) = 0x28;
    }
    c->r[4] = o; f_08961A6C(c);
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (fighter): for the CPU (+373), +78 = -1 when a random one of the sixteen bits of its level's (+375)
   mask at 0x08A9D5DC is set. */
SFA_FUNC(089A3F24, cpu_random_78) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x10;
    c->r[16] = f;
    if (S8(f + 373) != 0) {
        uint32_t mask = *H(0x08A9D5DCu + (uint32_t)(int32_t)S8(f + 375) * 2);
        c->r[17] = mask;
        f_08821368(c);
        c->r[5] = 1;
        c->r[4] = mask & (1u << (c->r[2] & 0xF));
        if (c->r[4] != 0) *B(f + 78) = 0xFF;
        c->r[4] = 0xFFFFFFFFu;
    }
    c->r[29] += 0x10;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): unless +77 is set, on one frame in four (GAME +32 with its side +337), up to two objects
   made beside it (make_beside_15) while they can be. */
SFA_FUNC(0892F82C, make_beside_every_4) {
    gaddr f = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (S8(f + 77) == 0 && ((*B(GAME + 32) + (S8(f + 337) & 1)) & 3) == 0) {
        int32_t n = 1;
        do {
            c->r[4] = f;
            f_08881F70(c);
            n--;
            c->r[17] = (uint32_t)n;
        } while (c->r[2] != 0 && n >= 0);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter): the first time (+6 from 0) its frame 3, after that the next frame, with the next step (+4)
   once +75 is negative; drawn if on screen. */
SFA_FUNC(089A5550, frames_then_draw) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (*B(f + 6) == 0) {
        *B(f + 6) = 1;
        c->r[4] = f; c->r[5] = 3;
        f_0887684C(c);
    } else {
        if (S8(f + 75) < 0) *B(f + 4) = (uint8_t)(*B(f + 4) + 1);
        c->r[4] = f; f_088768F0(c);
    }
    c->r[4] = f; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
