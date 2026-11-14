/* More steps of objects' routines: the objects of the screens around a fight (the versus screen, the
 * intro, the result), which wait on their owner's flags (+424 of the fighter they are given), start
 * animations by number and move towards their goals.
 *
 * Fields as in motion.c; here also +4 / +5 / +6 the routine's step, substep and part, +44 / +48 / +52
 * pointers to other objects, +78 the character, +86 a draw setting, +139 / +140 a grid place.
 */
#include "step.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

void f_088215C4(Cpu *c);   /* object_draw */
void f_088219B0(Cpu *c);   /* object_animate */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_0889CE64(Cpu *c);   /* object_move_x_to_target */
void f_0889CEA4(Cpu *c);   /* object_move_y_to_target */
void f_0889E704(Cpu *c);   /* object_scroll_y */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_088772C0(Cpu *c);   /* fighter_run_484 */
void f_0882DB00(Cpu *c);
void f_0887A154(Cpu *c);

static void next_part(gaddr o)    { *B(o + 6) = (uint8_t)(*B(o + 6) + 1); }
static void next_substep(gaddr o) { *B(o + 6) = 0; *B(o + 5) = (uint8_t)(*B(o + 5) + 1); }
static void next_step(gaddr o)    { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); *B(o + 6) = 0; *B(o + 5) = 0; }

/* Calls a routine on the object within a frame of the original's size. */
static void call_on(Cpu *c, GuestFunc fn, gaddr o, uint32_t frame) {
    c->r[29] -= frame;
    c->r[4] = o;
    fn(c);
    c->r[29] += frame;
}

/* ---- waiting on the owner's flags (+424) ---- */

SFA_FUNC(08899DC8, step_on_owner_flag_1)  { if (S16(ARG(1) + 424) & 1) next_substep(ARG(0)); }
SFA_FUNC(0889A194, step_on_owner_flag_2)  { if (S16(ARG(1) + 424) & 2) next_substep(ARG(0)); }
SFA_FUNC(0889ADFC, step_on_owner_flag_4)  { if (S16(ARG(1) + 424) & 4) next_substep(ARG(0)); }
SFA_FUNC(0889A728, routine_on_owner_flag_8) { if (S16(ARG(1) + 424) & 8) next_step(ARG(0)); }
SFA_FUNC(0889AFC8, routine_on_owner_78_bit_3) { if (S8(ARG(1) + 78) & 8) next_step(ARG(0)); }

/* On flag 4: the next substep, +79 cleared, and the object at +44 (if any) sent to routine 3. */
SFA_FUNC(0889A478, step_on_flag_4_and_send) {
    gaddr o = ARG(0);
    if ((S16(ARG(1) + 424) & 4) == 0) return;
    next_substep(o);
    *B(o + 79) = 0;
    if (*W(o + 44) != 0) *B(*W(o + 44) + 4) = 3;
}

/* On flag 2: the next substep, and the objects at +48 and +52 sent to routine 3. */
SFA_FUNC(0889C604, step_on_flag_2_and_send) {
    gaddr o = ARG(0);
    if ((S16(ARG(1) + 424) & 2) == 0) return;
    next_substep(o);
    *B(*W(o + 48) + 4) = 3;
    *B(*W(o + 52) + 4) = 3;
}

/* The next substep once the low byte of GAME + 22 is 7. */
SFA_FUNC(088996AC, step_on_game_22_at_7) { if ((S16(GAME + 22) & 0xFF) == 7) next_substep(ARG(0)); }

/* After one frame's wait (+6): the next substep, or the one after next for an owner with flag 1; and
   the owner's grid place and character are taken (+139 row, +140 column, +78). */
SFA_FUNC(088996E0, step_and_take_place) {
    gaddr o = ARG(0), f = ARG(1);
    if (*B(o + 6) == 0) { *B(o + 6) = 1; return; }
    next_substep(o);
    if (S16(f + 424) & 1) *B(o + 5) = (uint8_t)(*B(o + 5) + 2);
    *B(o + 139) = (uint8_t)S16(f + 22);
    *B(o + 140) = (uint8_t)S16(f + 18);
    *B(o + 78) = *B(f + 338);
}

/* Once GAME + 20 is negative: the next step, aiming at the goal from a standstill (in y). */
SFA_FUNC(0889EBC8, aim_y_when_ready) {
    gaddr o = ARG(0);
    if (S16(GAME + 20) >= 0) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(o + 114) = (uint16_t)S16(o + 112);
    *W(o + 100) = 0;
}

/* ---- moving and drawing ---- */

SFA_FUNC(0889B6BC, animate_and_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088219B0(c);
    c->r[4] = c->r[16]; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Draws the object; before that, for an owner with bit 2 of +78, the next part, aiming at the goal from
   a standstill. */
SFA_FUNC(0889B844, draw_and_aim_on_owner_78) {
    gaddr o = ARG(0);
    if (S8(ARG(1) + 78) & 4) {
        next_part(o);
        *H(o + 114) = (uint16_t)S16(o + 112);
        *W(o + 96) = 0;
    }
    call_on(c, f_088215C4, o, 0x20);
}

/* Moves in x towards the target from the x kept in +36 (the next part when it arrives), keeps the new x
   there, and draws. */
SFA_FUNC(0889B888, move_x_from_36_and_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 18) = *H(o + 36);
    f_0889CE64(c);
    if (c->r[2] != 0) next_part(o);
    *H(o + 36) = (uint16_t)S16(o + 18);
    c->r[4] = o; c->r[5] = (uint32_t)S16(o + 18);
    f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Moves in y towards the target; the next routine step when it arrives (the part at +6 is left). */
SFA_FUNC(0889BE88, move_y_then_routine) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_0889CEA4(c);
    if (c->r[2] != 0) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The substep's handler (0x08A2B104), then the wrap of its y, then drawn. */
SFA_FUNC(0889E5AC, step_scroll_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    gaddr fn = *W(0x08A2B104u + *B(o + 5) * 4u);
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = o; c->r[6] = fn;
    rt_call_indirect(c, fn);
    c->r[4] = c->r[16]; f_0889E704(c);
    c->r[4] = c->r[16]; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Drawn unless the stage's byte in 0x08A2AF54 says not. */
SFA_FUNC(0889E3F8, draw_unless_stage) {
    c->r[29] -= 0x20;
    if (*B(0x08A2AF54u + (uint32_t)S16(BATTLE)) == 0) f_088215C4(c);
    c->r[29] += 0x20;
}

/* Two speeds at once: +32 by +112 and +112 by +114; +36 by +16 and +16 by +20 (all 16 bits). */
SFA_FUNC(08899514, move_two_speeds) {
    gaddr o = ARG(0);
    uint16_t v1 = *H(o + 112), v2 = *H(o + 16);
    *H(o + 32) = (uint16_t)(*H(o + 32) + v1);
    *H(o + 112) = (uint16_t)(v1 + *H(o + 114));
    *H(o + 36) = (uint16_t)(*H(o + 36) + v2);
    *H(o + 16) = (uint16_t)(v2 + *H(o + 20));
}

/* Counts +18 down, the next substep when it runs out, and calls 0x0882DB00 on the object. */
SFA_FUNC(08886118, count_18_and_call) {
    gaddr o = ARG(0);
    int8_t left = (int8_t)(S8(o + 18) - 1);
    *B(o + 18) = (uint8_t)left;
    if (left < 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    call_on(c, f_0882DB00, o, 0x10);
}

/* Counts the timer down, and frees the object (to the pool of 32) when it has run out. */
SFA_FUNC(0887DCCC, free_after_timer) {
    gaddr o = ARG(0);
    int16_t t = S16(o + 82);
    if (t != 0) { *H(o + 82) = (uint16_t)(t - 1); return; }
    call_on(c, f_088231C8, o, 0x20);
}

/* ---- starting animations by number ---- */

static void set_animation(Cpu *c, gaddr entry) {
    c->r[29] -= 0x20;
    c->r[5] = entry;
    f_0882196C(c);
    c->r[29] += 0x20;
}
SFA_FUNC(0889CDC8, set_animation_a)       { set_animation(c, *W(0x08A27744u + (ARG(1) & 0xFFFF) * 4u)); }
SFA_FUNC(0889CDF8, set_animation_a_entry) { set_animation(c, *W(0x08A27744u + (ARG(1) & 0xFFFF) * 4u) + (ARG(2) & 0xFFFF) * 8u); }
SFA_FUNC(0889CE34, set_animation_b)       { set_animation(c, *W(0x08A28950u + (ARG(1) & 0xFFFF) * 4u)); }

/* ---- setting objects up ---- */

SFA_FUNC(0889EEC4, setup_0889EEC4) {
    gaddr o = ARG(0);
    *W(o + 148) = 1; *H(o + 86) = 8; *B(o + 77) = 0x60;
    set_animation(c, *W(0x08A283ECu + 16));
}

SFA_FUNC(0889B080, setup_0889B080) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 86) = 0xB; *W(o + 148) = 0; *W(o + 164) = 1;
}

/* The object takes its fighter's side (+121) and puts 0x08A27FFC in 0x092E3E9C. */
SFA_FUNC(0889BF68, setup_0889BF68) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 121) = *B(ARG(1) + 337);
    *W(0x092E3E98u + 4) = 0x08A27FFCu;
    *W(o + 148) = 2; *H(o + 86) = 8;
}

/* A record's layer (+9) set and its work fields cleared, +24 = 7; the second with layer 0. */
static void record_setup(gaddr r, uint8_t layer) {
    *B(r + 9) = layer;
    *H(r + 20) = 0; *H(r + 16) = 0; *H(r + 38) = 0;
    *B(r + 15) = 0; *B(r + 14) = 0; *B(r + 13) = 0; *B(r + 12) = 0; *B(r + 11) = 0;
    *H(r + 24) = 7;
}
SFA_FUNC(0889CFE8, record_setup_layer) { record_setup(ARG(0), (uint8_t)ARG(1)); }
SFA_FUNC(08898954, record_setup_0)     { record_setup(ARG(0), 0); }

SFA_FUNC(08896068, setup_08896068) {
    gaddr o = ARG(0);
    *H(o + 10) = 7; *H(o + 22) = 0xC0;
    *B(BATTLE + 81) = 0xFF;
    *W(0x092E8800u + 20) = 0; *W(0x092E8800u + 136) = 0;
    *H(o + 20) = (uint16_t)(S16(o + 20) & 0xFF00);
}

/* Two background layers set for a screen: both at x 64.0, y -16.0. */
SFA_FUNC(088967C4, layers_setup_088967C4) {
    gaddr a = 0x092E88A0u, b = 0x092E8940u;
    *B(a + 123) = 6; *B(a + 122) = 2; *B(b + 122) = 1;
    *W(b + 16) = 0x400000; *W(a + 16) = 0x400000;
    *W(b + 20) = 0xFFF00000u; *W(a + 20) = 0xFFF00000u;
}

SFA_FUNC(08889C88, setup_08889C88) {
    gaddr o = ARG(0);
    *B(o + 54) = 0x18; *B(o + 88) = 2; *B(o + 55) = 2;
    *H(o + 10) = 0; *H(o + 6) = 0; *B(o + 56) = 0; *B(o + 136) = 0xFF;
    *W(o + 132) = 0x08A26700u; *B(o + 93) = 1;
}

/* Shown, with all twelve corner colour bytes (+36, +42, +48, four each) at full. */
SFA_FUNC(08889FC0, setup_full_colour) {
    gaddr o = ARG(0);
    *B(o + 93) = 1; *B(o + 3) = 0; *B(o + 89) = 1; *B(o + 137) = 1; *B(o + 136) = 0xFF;
    for (int i = 3; i >= 0; i--) *B(o + 48 + i) = 0xFF;
    for (int i = 3; i >= 0; i--) *B(o + 42 + i) = 0xFF;
    for (int i = 3; i >= 0; i--) *B(o + 36 + i) = 0xFF;
}

/* Eight values copied from a 16-byte description into the object's two parts. */
SFA_FUNC(08889F7C, setup_from_description) {
    gaddr o = ARG(0), d = ARG(1);
    *H(o + 12) = 0; *H(o + 24) = 0;
    *H(o + 14) = *H(d); *H(o + 26) = *H(d + 2);
    *W(o + 60) = *W(d + 4);
    *H(o + 64) = *H(d + 8); *H(o + 76) = *H(d + 10); *H(o + 66) = *H(d + 12); *H(o + 78) = *H(d + 14);
}

/* The object's four bytes at +6 from entry `n` of the table 0x08A25758, which +12 then points at. */
SFA_FUNC(08885910, setup_from_table_25758) {
    gaddr o = ARG(0), p = *W(0x08A25758u + (ARG(1) & 0xFFFF) * 4u);
    *W(o + 12) = p;
    *B(o + 6) = *B(p); *B(o + 7) = *B(p + 1); *B(o + 8) = *B(p + 2); *B(o + 9) = *B(p + 3);
}

/* Takes the single object at 0x092E5100 if it is free, with its kind and type; 1 if it was. */
SFA_FUNC(088858D8, single_object_take) {
    gaddr o = 0x092E5100u;
    if (*B(o) != 0) { RETURN(0); return; }
    *B(o) = 1; *B(o + 2) = (uint8_t)ARG(0); *B(o + 3) = (uint8_t)ARG(1);
    RETURN(1);
}

/* Its first 16 bytes cleared and +8 = 8, with GAME + 40. */
SFA_FUNC(088865F8, single_object_free) {
    gaddr o = ARG(0);
    W(o)[0] = 0; W(o)[1] = 0; W(o)[2] = 0; W(o)[3] = 0;
    *B(o + 8) = 8;
    *B(GAME + 40) = 0;
}

/* ---- handlers picked with a condition ---- */

/* The substep's handler (0x08A25898) unless GAME + 145 (the pause). */
SFA_FUNC(08885BF8, run_substep_unless_paused) {
    if (S8(GAME + 145) != 0) return;
    run_step(c, *B(ARG(0) + 5), 0x08A25898u, 5, 0x08885C34u);
}

/* The routine's handler (0x08A25A18) for objects that have something at +44. */
SFA_FUNC(08886EA4, run_step_if_44) {
    if (*W(ARG(0) + 44) == 0) return;
    run_step(c, *B(ARG(0) + 4), 0x08A25A18u, 5, 0x08886ED8u);
}

/* A fighter's own routine: the pointer at +84 of what its +416 points at. */
SFA_FUNC(0887B504, fighter_run_own_routine) {
    gaddr fn = *W(*W(ARG(0) + 416) + 84);
    c->r[29] -= 0x20;
    c->r[5] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* The fighter's +480 to +484 set to 0, 0, 0, 0, 2 and its +484 routine run. */
SFA_FUNC(0887B7CC, fighter_restart_484) {
    gaddr f = ARG(0);
    *B(f + 484) = 2; *B(f + 483) = 0; *B(f + 482) = 0; *B(f + 481) = 0; *B(f + 480) = 0;
    c->r[5] = 2;
    call_on(c, f_088772C0, f, 0x20);
}

/* A script command read: the word at `p` into the fighter's +488 and the place after it into +528, one
   more command allowed this frame (0x08B0268E); if the command's high byte (+489) is -1 it is handed
   to 0x0887A154. Answers the place after the word (or what 0x0887A154 answers). */
SFA_FUNC(0887A10C, fighter_script_read) {
    gaddr f = ARG(0), p = ARG(1);
    *H(0x08B0268Eu) = (uint16_t)(*H(0x08B0268Eu) + 1);
    *H(f + 488) = *H(p);
    *W(f + 528) = p + 2;
    c->r[2] = p + 2;
    if (S8(f + 489) != -1) return;
    c->r[29] -= 0x20;
    c->r[5] = p + 2;
    f_0887A154(c);
    c->r[29] += 0x20;
}

/* ---- more of the same (0.67.0) ---- */

void f_0889B6BC(Cpu *c);   /* animate_and_draw */
void f_08899514(Cpu *c);   /* move_two_speeds */
void f_0889CDF8(Cpu *c);   /* set_animation_a_entry */
void f_0882324C(Cpu *c);   /* object_free_42 */
void f_0887DA58(Cpu *c);   /* object_follow_owner */
void f_0887D990(Cpu *c);   /* object_set_animation_by_type */

/* Runs `fn` on the object with s0 holding it, as the originals do; answers what it answered. */
static uint32_t call_keeping(Cpu *c, GuestFunc fn, gaddr o) {
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[4] = o;
    fn(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
    return c->r[2];
}
static void snap_x_to_target(gaddr o) { *W(o + 16) = (uint32_t)S16(o + 114) << 16; }
/* The object reports that it has arrived: its bit (by type, +3) in GAME + 20. */
static void arrived(gaddr o) {
    *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | (1 << (*B(o + 3) & 31)));
}

SFA_FUNC(0889E6A8, move_x_then_arrived_a) { if (call_keeping(c, f_0889CE64, ARG(0))) arrived(ARG(0)); }
SFA_FUNC(0889F894, move_x_then_arrived_b) { if (call_keeping(c, f_0889CE64, ARG(0))) arrived(ARG(0)); }
SFA_FUNC(0889EBFC, move_y_then_arrived)   { if (call_keeping(c, f_0889CEA4, ARG(0))) arrived(ARG(0)); }

/* The same, reporting bit 4, and only for objects with +10 clear. */
SFA_FUNC(0889F050, move_x_then_arrived_bit_4) {
    gaddr o = ARG(0);
    if (!call_keeping(c, f_0889CE64, o)) return;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1); *B(o + 5) = 0;
    if (*B(o + 10) == 0) *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 0x10);
}

/* Moves in x to the target (the next part on arriving), animates and draws; the first also snaps onto
   the target. */
SFA_FUNC(0889B5EC, move_x_snap_animate_draw) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_0889CE64, o)) { next_part(o); snap_x_to_target(o); }
    call_keeping(c, f_0889B6BC, o);
}
SFA_FUNC(0889B67C, move_x_animate_draw) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_0889CE64, o)) next_part(o);
    call_keeping(c, f_0889B6BC, o);
}

/* As 0x0889B888 (from the x kept in +36), snapping onto the target on arriving. */
SFA_FUNC(0889B7E8, move_x_from_36_snap_draw) {
    gaddr o = ARG(0);
    *H(o + 18) = *H(o + 36);
    if (call_keeping(c, f_0889CE64, o)) { next_part(o); snap_x_to_target(o); }
    *H(o + 36) = (uint16_t)S16(o + 18);
    call_keeping(c, f_088215C4, o);
}

/* Animated and drawn; before that, for an owner with bit 1 of +78, the next part, aiming at the goal. */
SFA_FUNC(0889B638, animate_draw_and_aim_on_owner_78) {
    gaddr o = ARG(0);
    if (S8(ARG(1) + 78) & 2) {
        next_part(o);
        *H(o + 114) = (uint16_t)S16(o + 112);
        *W(o + 96) = 0;
    }
    call_on(c, f_0889B6BC, o, 0x20);
}

/* y by its speed, wrapped up from below 0 by the height +76 / +77 give, and drawn. */
SFA_FUNC(0889AFF4, scroll_up_and_draw) {
    gaddr o = ARG(0);
    int32_t y = (int32_t)(*W(o + 20) + *W(o + 100));
    if (y < 0) {
        uint32_t height = (uint32_t)((S8(o + 76) << 8) + (S8(o + 77) & 0xFF)) << 16;
        do y = (int32_t)((uint32_t)y + height); while (y < 0);
    }
    *W(o + 20) = (uint32_t)y;
    call_on(c, f_088215C4, o, 0x20);
}

/* The two speeds (0x08899514), +34 following +32; the next routine step once +32 reaches 0x4000. */
SFA_FUNC(088994D0, move_two_speeds_to_4000) {
    gaddr o = ARG(0);
    call_keeping(c, f_08899514, o);
    *H(o + 34) = (uint16_t)S16(o + 32);
    if (S16(o + 32) >= 16384) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* On the owner's flag 1: the next substep, and the object at +48 (if any) freed. */
SFA_FUNC(0889AB10, step_on_flag_1_and_free) {
    gaddr o = ARG(0);
    if ((S16(ARG(1) + 424) & 1) == 0) return;
    next_substep(o);
    if (*W(o + 48) == 0) return;
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[4] = *W(o + 48);
    f_0882324C(c);
    *W(o + 48) = 0;
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object, fighter, entry): the animation for the fighter's side, entry `entry`; for objects with +10
   set the side is replaced by 2 or 3 (the side plus GAME + 32, odd or even). */
SFA_FUNC(08899A30, set_animation_for_side) {
    gaddr o = ARG(0);
    uint32_t side = *B(ARG(1) + 337), entry = ARG(2) & 0xFFFF;
    if (*B(o + 10) != 0) side = ((side + *B(GAME + 32)) & 1) + 2;
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = side; c->r[6] = entry;
    f_0889CDF8(c);
    c->r[29] += 0x20;
}

/* The goal, target, y speed and y acceleration from entry `n` of the table 0x08A2A148 (four s16: the
   goal, which is also the y to start from; the target; the speed and acceleration in 256ths). */
SFA_FUNC(0889CF90, setup_y_motion) {
    gaddr o = ARG(0), t = 0x08A2A148u + (((ARG(1) & 0xFFFF) << 2) & 0xFFFF) * 2u;
    *H(o + 112) = *H(t);
    *H(o + 114) = *H(t + 2);
    *W(o + 100) = (uint32_t)S16(t + 4) << 8;
    *H(o + 22) = *H(t);
    *W(o + 108) = (uint32_t)S16(t + 6) << 8;
}

SFA_FUNC(0889EDA8, setup_0889EDA8) {
    gaddr o = ARG(0);
    *W(o + 148) = 1; *H(o + 86) = 8; *H(o + 22) = 0x80;
    if ((S16(BATTLE + 72) & 1) == 0) *H(o + 22) = (uint16_t)(S16(o + 22) + 8);
    set_animation(c, *W(0x08A283ECu + 20));
}

/* The s16 the table 0x08A2B2D8 holds for BATTLE + 72. */
SFA_FUNC(0889FB94, value_by_battle_72) { RETURN((uint32_t)S16(0x08A2B2D8u + (uint32_t)*H(BATTLE + 72) * 2u)); }

/* The top layer (0x092E8800) falls: its y by the speed at +136, which grows by +144; at y 256 it stops
   there and the object's counter (+10) goes up, with bit 7 set in its +20. */
SFA_FUNC(0889610C, layer_fall_to_256) {
    gaddr o = ARG(0), layer = 0x092E8800u;
    int32_t y = (int32_t)(*W(layer + 20) + *W(layer + 136));
    *W(layer + 136) += *W(layer + 144);
    if (y >= 0x1000000) {
        *H(o + 10) = (uint16_t)(*H(o + 10) + 1);
        *H(o + 20) = (uint16_t)(S16(o + 20) | 0x80);
        y = 0x1000000;
    }
    *W(layer + 20) = (uint32_t)y;
}

/* An object following its owner (a fighter): layer 2, the owner's place moved by the character's
   offsets (its table's +24), the animation for its type; objects with +121 clear also get +12 = -1 and
   +13 = 2. */
SFA_FUNC(0887D8A0, owned_follow_and_animate) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 2;
    *B(o + 9) = 2;
    c->r[4] = o; c->r[5] = *W(*W(*W(o + 88) + 416) + 24);
    f_0887DA58(c);
    if (S8(o + 121) == 0) { *B(o + 12) = 0xFF; *B(o + 13) = 2; }
    c->r[4] = o;
    f_0887D990(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* Hidden while +33 counts down; then the next substep with +40 = +32, or, when +32 has bit 7, straight
   to substep 2 with the counter +35 at its end (+34 - 1). */
SFA_FUNC(08886FBC, count_33_then_start) {
    gaddr o = ARG(0);
    uint8_t n = *B(o + 33);
    *B(o + 33) = (uint8_t)(n - 1);
    if (n != 0) { *B(o + 1) = 0; return; }
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(o + 40) = *B(o + 32);
    if (*B(o + 32) & 0x80) { *B(o + 5) = 2; *B(o + 35) = (uint8_t)(*B(o + 34) - 1); }
}

/* Waits 24 frames (+18), or not at all when BATTLE + 2 is set, then the next routine step. */
SFA_FUNC(0888710C, wait_24_unless_battle_2) {
    gaddr o = ARG(0);
    if (*B(o + 5) == 0) {
        if (S16(BATTLE + 2) == 0) { *B(o + 5) = 1; *B(o + 18) = 0x18; return; }
    } else {
        *B(o + 18) = (uint8_t)(*B(o + 18) - 1);
        if (*B(o + 18) != 0) return;
    }
    *B(o + 5) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* ---- 0.68.0 ---- */

void f_08899A30(Cpu *c);   /* set_animation_for_side */
void f_0889AFF4(Cpu *c);   /* scroll_up_and_draw */
void f_08821368(Cpu *c);   /* a random number */

/* The animation for the fighter's side, drawn; the next substep on the owner's flag 1. */
SFA_FUNC(088997BC, animate_for_side_and_draw) {
    gaddr o = ARG(0), f = ARG(1), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S16(f + 424) & 1) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[4] = o; c->r[5] = f; c->r[6] = 0;
    f_08899A30(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The three moves of 0x0889B5EC, 0x0889B638 and 0x0889B67C, ending with the scroll (0x0889AFF4)
   instead of the animation; the middle one waits on bit 3 of the owner's +78. */
SFA_FUNC(0889AEF8, move_x_snap_scroll_draw) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_0889CE64, o)) { next_part(o); snap_x_to_target(o); }
    call_keeping(c, f_0889AFF4, o);
}
SFA_FUNC(0889AF88, move_x_scroll_draw) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_0889CE64, o)) next_part(o);
    call_keeping(c, f_0889AFF4, o);
}
SFA_FUNC(0889AF44, scroll_draw_and_aim_on_owner_78) {
    gaddr o = ARG(0);
    if (S8(ARG(1) + 78) & 8) {
        next_part(o);
        *H(o + 114) = (uint16_t)S16(o + 112);
        *W(o + 96) = 0;
    }
    call_on(c, f_0889AFF4, o, 0x20);
}

/* Entry `k` of animation `n`, from the tables 0x08A283EC and 0x08A27FE4, or of the object's own table
   (+96). */
SFA_FUNC(0889FBBC, set_animation_c_entry) { set_animation(c, *W(0x08A283ECu + (ARG(1) & 0xFFFF) * 4u) + (ARG(2) & 0xFFFF) * 8u); }
SFA_FUNC(0889FBF8, set_animation_d_entry) { set_animation(c, *W(0x08A27FE4u + (ARG(1) & 0xFFFF) * 4u) + (ARG(2) & 0xFFFF) * 8u); }
SFA_FUNC(088A055C, set_animation_own)     { set_animation(c, *W(*W(ARG(0) + 96) + (ARG(1) & 0xFFFF) * 4u)); }

/* One of two animation sets by the flag at 0x09525DE3: its entry 6, or 7. */
static gaddr set_by_flag(void) { return S8(0x09525DE3u) != 0 ? 0x08A2C9C8u : 0x08A2CB30u; }
SFA_FUNC(088AD470, set_animation_by_flag_6) { set_animation(c, *W(set_by_flag() + 24)); }
SFA_FUNC(088AD4DC, set_animation_by_flag_7) { set_animation(c, *W(set_by_flag() + 28)); }

/* The next routine step, taking the look of another object (its +12, +13, +24, +86, +164, +171). */
SFA_FUNC(088A041C, take_look_of) {
    gaddr o = ARG(0), src = ARG(1);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 12) = *B(src + 12); *B(o + 13) = *B(src + 13);
    *H(o + 86) = *H(src + 86); *W(o + 164) = *W(src + 164); *B(o + 171) = *B(src + 171);
    *W(o + 148) = 3;
    *H(o + 24) = *H(src + 24);
}

/* The routine steps on; from its second visit the object leaves: its owner's pointer to it (the word
   for its +78 at owner + 40) is cleared and it goes back to the pool of 42. */
SFA_FUNC(088A04EC, leave_owner_and_free) {
    gaddr o = ARG(0), owner = ARG(1);
    uint8_t was = *B(o + 4);
    *B(o + 4) = (uint8_t)(was + 1);
    if (was == 0) return;
    *W(owner + 0x28 + (uint32_t)(S8(o + 78) << 2)) = 0;
    call_on(c, f_0882324C, o, 0x20);
}

/* Moves left, speeding up; at x 192 or less the next routine step. */
SFA_FUNC(088ADAE4, move_left_to_192) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    *W(o + 16) -= v;
    *W(o + 96) = v + *W(o + 104);
    if (S16(o + 18) < 193) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); }
}

/* +96 = a random 1 to 32. */
SFA_FUNC(088B1AD0, set_random_96) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x10;
    c->r[16] = o;
    f_08821368(c);
    *W(o + 96) = (c->r[2] & 0x1F) + 1;
    c->r[29] += 0x10;
    c->r[16] = saved;
}

/* +100 cleared, then the substep's handler (0x08A30238). */
SFA_FUNC(088B16EC, clear_100_and_run_substep) {
    *W(ARG(0) + 100) = 0;
    run_step(c, *B(ARG(0) + 5), 0x08A30238u, 5, 0x088B1740u);
}

/* One of two handlers (0x08A30240), alternating with the frame (GAME + 32) and the object's turn in its
   list (GAME + 66). */
SFA_FUNC(088B18A4, run_alternating) {
    uint32_t which = (uint32_t)((int8_t)((int8_t)*B(GAME + 32) + S8(GAME + 66))) & 1;
    run_step(c, which, 0x08A30240u, 5, 0x088B18F0u);
}

/* Three panels that slide while GAME + 18 is set and back when it is not, drawn: down from y 240 and
   back up by 2; up from y 64 and back down by 2; left to x 272 by 16, or up by 10. */
SFA_FUNC(088BB59C, panel_slide_a) {
    gaddr o = ARG(0);
    int16_t y = S16(o + 22);
    if (S16(GAME + 18) == 0) *H(o + 22) = (uint16_t)(y + 2);
    else if (y != 0xF0) *H(o + 22) = (uint16_t)(y - 2);
    call_on(c, f_088215C4, o, 0x20);
}
SFA_FUNC(088BB6A8, panel_slide_b) {
    gaddr o = ARG(0);
    int16_t y = S16(o + 22);
    if (S16(GAME + 18) == 0) *H(o + 22) = (uint16_t)(y - 2);
    else if (y != 0x40) *H(o + 22) = (uint16_t)(y + 2);
    call_on(c, f_088215C4, o, 0x20);
}
SFA_FUNC(088BB79C, panel_slide_c) {
    gaddr o = ARG(0);
    if (S16(GAME + 18) == 0) *H(o + 22) = (uint16_t)(S16(o + 22) - 10);
    else if (S16(o + 18) != 0x110) *H(o + 18) = (uint16_t)(S16(o + 18) - 16);
    call_on(c, f_088215C4, o, 0x20);
}

/* A row of a list set to come in: row +78 is at y 48 + 16 a row; the first starts off screen at x -352,
   the second 240 below its place at x 48. Both wait 6 frames and step on. */
SFA_FUNC(088BBAE4, row_start_from_left) {
    gaddr o = ARG(0);
    *H(o + 22) = (uint16_t)((S8(o + 78) << 4) + 0x30);
    *H(o + 18) = 0xFEA0; *H(o + 82) = 6;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}
SFA_FUNC(088BBD18, row_start_from_below) {
    gaddr o = ARG(0);
    *H(o + 132) = (uint16_t)((S8(o + 78) << 4) + 0x30);
    *H(o + 22) = (uint16_t)(S16(o + 132) + 0xF0);
    *H(o + 18) = 0x30; *H(o + 82) = 6;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
}

/* +72 and +73 from the word the table 0x08A2C67C holds for +73: its high and low bytes. */
SFA_FUNC(088AA980, next_pair_from_table) {
    gaddr o = ARG(0);
    uint16_t w = *H(0x08A2C67Cu + *B(o + 73) * 2u);
    *B(o + 72) = (uint8_t)(w >> 8);
    *B(o + 73) = (uint8_t)w;
}

/* ---- 0.69.0 ---- */

void f_0887DF88(Cpu *c);   /* object_set_animation_n (motion.c) */
void f_08887C24(Cpu *c);   /* object_take_stage_colour */
void f_08889F7C(Cpu *c);   /* setup_from_description */

/* An owned object starts: shown, routine 1, and its animation by number (+78); number 0x4F instead
   takes its owner's side and +164 and the animation at +176 of the owner's character table. */
SFA_FUNC(0887E094, owned_start_animation) {
    gaddr o = ARG(0);
    *B(o + 72) = 1; *B(o + 4) = 1; *H(o + 86) = 5;
    if (S8(o + 78) == 0x4F) {
        gaddr owner = *W(o + 88);
        *H(o + 86) = (uint16_t)S8(owner + 337);
        *W(o + 164) = *W(owner + 164);
        set_animation(c, *W(*W(owner + 416) + 176));
    } else {
        c->r[29] -= 0x20;
        c->r[5] = (uint32_t)S8(o + 78) & 0xFFFF;
        f_0887DF88(c);
        c->r[29] += 0x20;
    }
}

/* One frame of a part's animation (the part keeps its entry at +12 and its count at +6, as an object
   does at +28 and +74): at the count's end the next entry, or -1 in +7 when the entry ends the animation
   (also set a frame early, when the object's own count is at 1). */
SFA_FUNC(08885950, part_animate) {
    gaddr o = ARG(0), part = ARG(1), entry = *W(part + 12);
    *B(part + 6) = (uint8_t)(*B(part + 6) - 1);
    int ends = (S8(entry + 1) & 0x40) != 0;
    if (*B(part + 6) != 0) {
        if (S8(o + 74) == 1 && ends) *B(part + 7) = 0xFF;
        return;
    }
    if (ends) { *B(part + 7) = 0xFF; return; }
    gaddr next = entry + (uint32_t)(S16(entry + 6) << 3);
    *W(part + 12) = next;
    *B(part + 6) = *B(next);
}

/* A counter's start: the next routine step, BATTLE + 82 cleared, hidden, 8 lower, and its length (+34)
   read from the bytes +44 points at (a first byte of 0 means the length is the next), plus one; +36
   keeps the place after it. */
SFA_FUNC(08886EE4, counter_start) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(BATTLE + 82) = 0;
    *B(o + 9) = 0; *B(o + 1) = 0; *B(o + 10) = 7;
    *H(o + 26) = (uint16_t)(S16(o + 26) + 8);
    *B(o + 35) = 0;
    gaddr p = *W(o + 44);
    uint8_t length = *B(p++);
    if (length == 0) length = *B(p++);
    *W(o + 36) = p;
    *B(o + 34) = (uint8_t)(length + 1);
}

SFA_FUNC(08887BBC, setup_08887BBC) {
    gaddr o = ARG(0);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1; *B(o + 55) = 7; *B(o + 3) = 1; *B(o + 56) = 3; *B(o + 2) = 0; *B(o + 136) = 0xA0;
    *H(o + 12) = 0; *H(o + 14) = 0x180; *H(o + 24) = 0xE8; *H(o + 26) = 0x30;
    call_on(c, f_08887C24, o, 0x20);
}

SFA_FUNC(08889E80, setup_08889E80) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 1;
    *B(o + 54) = 0x12; *B(o + 88) = 1; *B(o + 55) = 7;
    *H(o + 6) = 0x20; *H(o + 10) = 0x178; *B(o + 56) = 2; *B(o + 136) = 0xC0;
    c->r[4] = o; c->r[5] = 0x08A26660u;
    f_08889F7C(c);
    *B(o + 93) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* A marker beside the object +124 points at: 48 to its right at its height, with one of two pictures
   and sizes by the marker's type. */
SFA_FUNC(088BBF48, marker_follow) {
    gaddr o = ARG(0), target = *W(o + 124);
    *H(o + 12) = (uint16_t)(S16(target + 18) + 0x30);
    *H(o + 24) = (uint16_t)S16(target + 22);
    if (*B(o + 92) == 0) { *H(o + 14) = 0x70; *H(o + 26) = 0x20; }
    else { *H(o + 14) = 0x130; *H(o + 26) = 0x30; }
}

/* Third-list objects of types 14 to 17 fading out: +98 counts frames and +136 (how solid it is) drops
   by a step for part of that time; when +136 reaches 0 the object's step (+128) moves on. Type 14
   waits 14 frames and then drops by 18; 15 drops by 10 for 14 frames; 16 by 18 for 21 frames; 17 by 3.
   A drop that would land exactly on 0 is not stored (the original stores only what is above 0, and 0
   only for what is below). */
static void fade_step(gaddr o, int by) {
    int v = *B(o + 136) - by;
    if (v > 0) *B(o + 136) = (uint8_t)v;
    else if (v < 0) *B(o + 136) = 0;
}
SFA_FUNC(088B2C8C, list_3_fade_out) {
    gaddr o = ARG(0);
    uint8_t type = *B(o + 92);
    if (type < 14 || type > 17) return;
    *H(o + 98) = (uint16_t)(*H(o + 98) + 1);
    uint16_t frames = *H(o + 98);
    if (type >= 16) *B(o + 55) = 7;
    if (type == 17) *B(o + 56) = 7;
    if (type == 14) { if (frames >= 14) fade_step(o, 18); }
    else if (type == 15) { if (frames < 14) fade_step(o, 10); }
    else if (type == 16) { if (frames < 21) fade_step(o, 18); }
    else if (frames < 256) fade_step(o, 3);
    if (*B(o + 136) == 0) *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
}

/* ---- 0.69.0: the versus screen's objects ---- */

void f_0889995C(Cpu *c);   /* object_place_on_grid (motion.c) */
void f_0889CE34(Cpu *c);   /* set_animation_b */
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

/* The fighter's side is one of those GAME + 41 and + 57 mark (a bit a side). */
static int side_marked(gaddr f, int low_16) {
    uint32_t marks = (uint32_t)(S8(GAME + 57) | S8(GAME + 41));
    if (low_16) marks &= 0xFFFF;
    return (marks & (1u << (S8(f + 337) & 31))) != 0;
}

/* The substep's handler with the fighter, for a marked side (the third also for objects with bit 7 of
   +10). */
SFA_FUNC(08899AC8, run_for_marked_side_a) { if (side_marked(ARG(1), 0)) run_step_with(c, *B(ARG(0) + 5), 0x08A2A8DCu, ARG(1), 0x08899B2Cu); }
SFA_FUNC(0889A818, run_for_marked_side_b) { if (side_marked(ARG(1), 1)) run_step_with(c, *B(ARG(0) + 5), 0x08A2A9A4u, ARG(1), 0x0889A880u); }
SFA_FUNC(0889B0A4, run_for_marked_side_c) {
    if (side_marked(ARG(1), 1) || (*B(ARG(0) + 10) & 0x80)) run_step_with(c, *B(ARG(0) + 5), 0x08A2AA08u, ARG(1), 0x0889B11Cu);
}
SFA_FUNC(0889BFA4, run_for_marked_side_d) { if (side_marked(ARG(1), 1)) run_step_with(c, *B(ARG(0) + 5), 0x08A2AAE4u, ARG(1), 0x0889C00Cu); }

/* Arrives (0x0889CE64 / 0x0889CEA4, or at once while BATTLE + 81) and stops on its target; drawn. The y
   one reports bit 2 in GAME + 20. Both only in substep 0. */
static void arrive_then_draw(Cpu *c, gaddr o, GuestFunc move, uint32_t position, int report) {
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        int there = S8(BATTLE + 81) != 0;
        if (!there) { c->r[4] = o; move(c); there = c->r[2] != 0; }
        if (there) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *W(o + position) = (uint32_t)S16(o + 114) << 16;
            if (report) *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 4);
        }
    }
    c->r[4] = o;
    f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(08897D78, arrive_y_then_draw) { arrive_then_draw(c, ARG(0), f_0889CEA4, 20, 1); }
SFA_FUNC(08898308, arrive_x_then_draw) { arrive_then_draw(c, ARG(0), f_0889CE64, 16, 0); }

/* Arrives in x (or at once while BATTLE + 81): the next substep, on its target, reporting its bit (by
   type) in GAME + 20. */
SFA_FUNC(0889E5FC, arrive_x_and_report) {
    gaddr o = ARG(0);
    if (S8(BATTLE + 81) == 0 && !call_keeping(c, f_0889CE64, o)) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(o + 18) = (uint16_t)S16(o + 114);
    *H(o + 16) = 0;
    *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | (1 << (*B(o + 3) & 31)));
}

/* The two speeds until +32 comes up to 0: then the next substep with +32 to +36 cleared, reporting its
   bit (by +10) in GAME + 22. */
SFA_FUNC(088993C8, move_two_speeds_to_0) {
    gaddr o = ARG(0);
    call_keeping(c, f_08899514, o);
    *H(o + 34) = (uint16_t)S16(o + 32);
    if (S16(o + 32) < 0) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *H(o + 36) = 0; *H(o + 34) = 0; *H(o + 32) = 0;
    *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | (1 << (*B(o + 10) & 31)));
}

/* Placed on the grid by its fighter, with the animation for the fighter's side, and drawn; the next
   routine step once the high byte of GAME + 22 is set. */
SFA_FUNC(08899810, place_animate_draw) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    if (S16(GAME + 22) & 0xFF00) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[4] = o; c->r[5] = f; f_0889995C(c);
    c->r[4] = o; c->r[5] = f; c->r[6] = 0; f_08899A30(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* Animated and drawn at the x kept in +36; while GAME + 20's low byte names this object (+10) it is
   lit (+12 = 0, else -1) and nudged 4 towards the middle, by its fighter's side. */
SFA_FUNC(0889A754, animate_draw_lit) {
    gaddr o = ARG(0), saved = c->r[16];
    uint16_t x = *H(o + 36);
    uint8_t dim = 0xFF;
    if ((uint32_t)(S16(GAME + 20) & 0xFF) == *B(o + 10)) {
        dim = 0;
        x = (uint16_t)(x + (S8(ARG(1) + 337) != 0 ? -4 : 4));
    }
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 12) = dim;
    *H(o + 18) = x;
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Animation 8 of the second table for objects with +10 set past their substep 0; otherwise x from the
   whole part of +96 and animation 4 or 5 by the fighter's side. */
SFA_FUNC(0889BEC4, set_animation_by_side_or_8) {
    gaddr o = ARG(0), f = ARG(1);
    c->r[29] -= 0x20;
    c->r[4] = o;
    if (*B(o + 10) != 0 && *B(o + 5) != 0) c->r[5] = 8;
    else {
        *H(o + 18) = (uint16_t)((int32_t)*W(o + 96) >> 16);
        c->r[5] = (uint32_t)(S8(f + 337) + 4) & 0xFFFF;
    }
    f_0889CE34(c);
    c->r[29] += 0x20;
}

/* (object, fighter, n): row n: +80 = n, y from the table 0x08A2AB4C (entry n + 1), and entry n + 1 of
   animation 12 or 14 by the fighter's side. */
SFA_FUNC(0889C6A0, set_row) {
    gaddr o = ARG(0);
    uint32_t n = ((ARG(2) & 0xFFFF) + 1) & 0xFFFF;
    *B(o + 80) = (uint8_t)ARG(2);
    *H(o + 22) = *H(0x08A2AB4Cu + n * 2u);
    uint32_t side = (uint32_t)S8(ARG(1) + 337) & 0xFFFF;
    c->r[29] -= 0x20;
    c->r[5] = (((side * 2) & 0xFFFF) + 0xC) & 0xFFFF; c->r[6] = n;
    f_0889CDF8(c);
    c->r[29] += 0x20;
}

/* The substep's handler (0x08A2B164); objects with +10 = 2 then scroll down one a frame, round from 96
   to 0; drawn. */
SFA_FUNC(0889EF1C, step_scroll_96_draw) {
    gaddr o = ARG(0), saved = c->r[16];
    gaddr fn = *W(0x08A2B164u + *B(o + 5) * 4u);
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[4] = o; c->r[5] = fn;
    rt_call_indirect(c, fn);
    if (*B(o + 10) == 2) {
        int32_t y = (int32_t)(*W(o + 20) + 0x10000);
        if (y >= 0x600000) y -= 0x600000;
        *W(o + 20) = (uint32_t)y;
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* 54 parts (0x40 bytes each from +0x34) slide while +4 counts down by 2: those in use move +10 up by 2,
   the others down. When the count is at 0 the step (+2) moves on. */
SFA_FUNC(0888D71C, parts_slide) {
    gaddr o = ARG(0);
    if (S16(o + 4) != 0) {
        *H(o + 4) = (uint16_t)(S16(o + 4) - 2);
        for (int i = 0; i < 54; i++) {
            gaddr part = o + 0x34 + (uint32_t)i * 0x40u;
            *H(part + 10) = (uint16_t)(S16(part + 10) + (*B(part) != 0 ? 2 : -2));
        }
    }
    if (S16(o + 4) == 0) *B(o + 2) = (uint8_t)(*B(o + 2) + 1);
}

/* Counts +22 down; when it runs out the counter +10 goes up and the second record shows a character:
   +18 = 127, +66 = fighter 1's or 2's character by bit 0 of +57. */
SFA_FUNC(088958C0, count_22_then_show_character) {
    gaddr o = ARG(0), p = ARG(1);
    int16_t left = (int16_t)(S16(o + 22) - 1);
    *H(o + 22) = (uint16_t)left;
    if (left >= 0) return;
    *B(o + 43) = 0;
    *H(o + 10) = (uint16_t)(*H(o + 10) + 1);
    *B(p + 65) = 0;
    *H(p + 18) = 0x7F;
    *B(p + 66) = *B(((S8(o + 57) & 1) ? FIGHTER_1 : FIGHTER_2) + 338);
}

SFA_FUNC(08889E00, setup_08889E00) {
    gaddr o = ARG(0);
    *B(o + 88) = 0; *B(o + 55) = 7;
    *H(o + 6) = 0; *H(o + 10) = 9; *H(o + 24) = 0; *H(o + 12) = 0; *H(o + 14) = 0x282; *H(o + 26) = 0x20;
    *B(o + 56) = 1; *B(o + 136) = 0xC0;
    for (int i = 3; i >= 0; i--) *B(o + 36 + i) = 0x76;
    for (int i = 3; i >= 0; i--) *B(o + 42 + i) = 0x76;
    for (int i = 3; i >= 0; i--) *B(o + 48 + i) = 0xBC;
    *B(o + 93) = 1;
}

/* ---- 0.70.0 ---- */

void f_08821368(Cpu *c);   /* a random number */
void f_08822EAC(Cpu *c);   /* object_make (kind, type) in the fifth list */
void f_0889CFE8(Cpu *c);
void f_0889A754(Cpu *c);   /* animate_draw_lit */
void f_08885950(Cpu *c);   /* part_animate */

/* The x kept in +36 moved to its target (the first also stops on it), the next part on arriving; then
   lit and drawn. */
static void slide_kept_x(Cpu *c, int snap) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    *H(o + 18) = *H(o + 36);
    c->r[4] = o; f_0889CE64(c);
    if (c->r[2] != 0) {
        next_part(o);
        if (snap) snap_x_to_target(o);
    }
    *H(o + 36) = (uint16_t)S16(o + 18);
    c->r[4] = o; c->r[5] = f; f_0889A754(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(0889A600, slide_kept_x_and_stop) { slide_kept_x(c, 1); }
SFA_FUNC(0889A6C4, slide_kept_x_on)       { slide_kept_x(c, 0); }

/* Follows the record `p` while it is shown, 24 above it; animated while GAME + 20's low byte names
   the record (+10). */
SFA_FUNC(088A0248, follow_24_above) {
    gaddr o = ARG(0), p = ARG(1);
    if (*B(p + 1) == 0) return;
    *B(o + 1) = 1;
    *B(o + 12) = *B(p + 12); *B(o + 13) = *B(p + 13);
    *W(o + 16) = *W(p + 16);
    *W(o + 20) = *W(p + 20) - 0x180000u;
    if (*B(p + 10) == (uint32_t)(S16(GAME + 20) & 0xFF)) call_on(c, f_088219B0, o, 0x20);
}

/* Follows the record `p` while it is shown, 44 to the side of it by its owner's side; animated while
   the record's +79 is the owner's +438. */
SFA_FUNC(088A0374, follow_44_beside) {
    gaddr o = ARG(0), p = ARG(1), owner = *W(o + 92);
    if (*B(p + 1) == 0) return;
    *B(o + 1) = 1;
    *B(o + 12) = *B(p + 12); *B(o + 13) = *B(p + 13);
    *W(o + 16) = *W(p + 16) + (S8(owner + 337) != 0 ? 0x2C0000u : 0xFFD40000u);
    *W(o + 20) = *W(p + 20);
    if ((uint32_t)*B(p + 79) == (uint32_t)S8(owner + 438)) call_on(c, f_088219B0, o, 0x20);
}

/* The animation for the high byte of the s16 at 0x0951CE52, through a table of numbers. */
static void set_animation_by_ce52(Cpu *c, gaddr numbers) {
    uint32_t n = *B(numbers + (uint32_t)(*H(0x0951CE52u) >> 8));
    gaddr set = set_by_flag();
    c->r[6] = set;
    set_animation(c, *W(set + n * 4u));
}
SFA_FUNC(088AD328, set_animation_by_ce52_a) { set_animation_by_ce52(c, 0x08A2CC60u); }
SFA_FUNC(088AD3B8, set_animation_by_ce52_b) { set_animation_by_ce52(c, 0x08A2CC74u); }

/* Thirteen frames of 12 a frame, right or (with +78) left: answers 1 on the last, with the count (+10)
   back at 0 and those going left 13 further. */
SFA_FUNC(088AD5A8, shift_x_13_frames) {
    gaddr o = ARG(0);
    int left = S8(o + 78) != 0;
    *H(o + 18) = (uint16_t)(S16(o + 18) + (left ? -12 : 12));
    *B(o + 10) = (uint8_t)(*B(o + 10) + 1);
    if (*B(o + 10) != 13) { RETURN(0); return; }
    if (left) *H(o + 18) = (uint16_t)(S16(o + 18) - 13);
    *B(o + 10) = 0;
    RETURN(1);
}

/* The same for the s16 +6, by the type (+92): 4 goes up, 5 down, with the count in +98. */
SFA_FUNC(088B2754, shift_6_13_frames) {
    gaddr o = ARG(0);
    uint32_t type = *B(o + 92);
    if (type == 4) *H(o + 6) = (uint16_t)(S16(o + 6) + 12);
    else if (type == 5) *H(o + 6) = (uint16_t)(S16(o + 6) - 12);
    *H(o + 98) = (uint16_t)(*H(o + 98) + 1);
    if (*H(o + 98) != 13) { RETURN(0); return; }
    if (type == 5) *H(o + 6) = (uint16_t)(S16(o + 6) - 13);
    *H(o + 98) = 0;
    RETURN(1);
}

/* An animation picked at random: one of 64 numbers (0x08A30248) into the table 0x08A30140, kept in
   +112; or one of 32 (0x08A30288) as an entry of the list at 0x08A2FFFC, kept in +114. */
SFA_FUNC(088B18FC, set_animation_at_random_a) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 1;
    *H(o + 26) = 1;
    f_08821368(c);
    int32_t n = S8(0x08A30248u + (c->r[2] & 0x3F));
    c->r[18] = (uint32_t)n;
    c->r[4] = o; c->r[5] = *W(0x08A30140u + ((uint32_t)n << 2));
    f_0882196C(c);
    *H(o + 112) = (uint16_t)n;
    *H(o + 36) = 1;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}
SFA_FUNC(088B1978, set_animation_at_random_b) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 26) = 3;
    f_08821368(c);
    int32_t n = S8(0x08A30288u + (c->r[2] & 0x1F));
    c->r[17] = (uint32_t)n;
    c->r[4] = o; c->r[5] = *W(0x08A2FFB0u + 76) + ((uint32_t)n << 3);
    f_0882196C(c);
    *H(o + 114) = (uint16_t)n;
    *H(o + 36) = 2;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

SFA_FUNC(088BB728, setup_088BB728) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 2; *H(o + 24) = 6; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 18) = 0x290; *H(o + 22) = 0x70;
    *B(o + 12) = 0xFF; *B(o + 13) = 0x34;
    *H(o + 86) = 0x15;
    *H(o + 84) = *B(0x08A38154u + *B(o + 10));
    *W(o + 148) = 3;
}

/* Scrolls down, round from 224 to 0, sliding 2 a frame out to x 0 while GAME + 18 is set and back in
   while it is not; the second scrolls up and slides to x 352. Drawn. */
SFA_FUNC(088BB8A0, scroll_down_slide_left) {
    gaddr o = ARG(0);
    int16_t x = S16(o + 18);
    if (S16(GAME + 18) == 0) *H(o + 18) = (uint16_t)(x - 2);
    else if (x != 0) *H(o + 18) = (uint16_t)(x + 2);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 1);
    if (S16(o + 22) >= 224) *H(o + 22) = (uint16_t)(S16(o + 22) - 224);
    call_on(c, f_088215C4, o, 0x20);
}
SFA_FUNC(088BB9BC, scroll_up_slide_right) {
    gaddr o = ARG(0);
    int16_t x = S16(o + 18);
    if (S16(GAME + 18) == 0) *H(o + 18) = (uint16_t)(x + 2);
    else if (x != 0x160) *H(o + 18) = (uint16_t)(x - 2);
    *H(o + 22) = (uint16_t)(S16(o + 22) - 1);
    if (S16(o + 22) <= 0) *H(o + 22) = (uint16_t)(S16(o + 22) + 224);
    call_on(c, f_088215C4, o, 0x20);
}

/* One more object of kind 0x71 made for the same +10. */
static gaddr make_0x71(Cpu *c, gaddr o, uint32_t type, uint8_t number) {
    c->r[4] = 0x71; c->r[5] = type;
    f_08822EAC(c);
    gaddr made = c->r[2];
    *B(made + 78) = number;
    *B(made + 10) = *B(o + 10);
    return made;
}

/* Counts +82 down; then the next step, having made the next of its row (+78 one more) unless it is
   the last. */
static void count_then_make_next(Cpu *c, uint32_t type, int last) {
    gaddr o = ARG(0), saved = c->r[16];
    *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    if (S16(o + 82) != 0) return;
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 78) != last) make_0x71(c, o, type, (uint8_t)(S8(o + 78) + 1));
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(088BBB14, count_then_make_next_5) { count_then_make_next(c, 5, 3); }
SFA_FUNC(088BBD54, count_then_make_next_6) { count_then_make_next(c, 6, 4); }

/* 4 a frame to x 368: there the next step, the fourth of the row (+78 = 3) starting the next row. */
SFA_FUNC(088BBBEC, move_to_368_then_next_row) {
    gaddr o = ARG(0), saved = c->r[16];
    if (S16(o + 18) != 0x170) { *H(o + 18) = (uint16_t)(S16(o + 18) + 4); return; }
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    if (S8(o + 78) == 3) make_0x71(c, o, 6, 0);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* Jittered once (unless +135): x and y each by one of sixteen s16 at 0x08A242F4, at random. */
SFA_FUNC(0887D9D0, jitter_once) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 135) == 0) {
        int16_t x = S16(o + 18);
        c->r[17] = (uint32_t)x; c->r[18] = 0x08A242F4u;
        f_08821368(c);
        int16_t dx = S16(0x08A242F4u + (c->r[2] & 0xF) * 2u);
        int16_t y = S16(o + 22);
        c->r[19] = (uint32_t)y;
        *H(o + 18) = (uint16_t)(x + dx);
        f_08821368(c);
        *H(o + 22) = (uint16_t)(y + S16(0x08A242F4u + (c->r[2] & 0xF) * 2u));
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* The six parts (16 bytes each from +0x20) move +4 by this frame's entry of the table 0x08A25818; an
   entry of 0 ends it: the next substep, +7 = 2, +18 = 23. */
SFA_FUNC(08886160, parts_move_by_table) {
    gaddr o = ARG(0);
    int8_t i = S8(o + 16);
    *B(o + 16) = (uint8_t)(i + 1);
    int8_t by = S8(0x08A25818u + (uint32_t)(int32_t)i);
    if (by == 0) { next_substep(o); *B(o + 7) = 2; *B(o + 18) = 0x17; }
    for (int k = 0; k < 6; k++) {
        gaddr part = o + 0x20 + (uint32_t)k * 0x10u;
        *H(part + 4) = (uint16_t)(S16(part + 4) + by);
    }
}

/* The first four parts animated (one whose +7 goes negative is put out); +18 counts down, then
   step 3. */
SFA_FUNC(08886314, parts_animate_then_step_3) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o;
    for (int k = 0; k < 4; k++) {
        gaddr part = o + 0x20 + (uint32_t)k * 0x10u;
        c->r[17] = part; c->r[18] = (uint32_t)(3 - k);
        c->r[4] = o; c->r[5] = part;
        f_08885950(c);
        if (S8(part + 7) < 0) *B(part) = 0;
    }
    *B(o + 18) = (uint8_t)(S8(o + 18) - 1);
    if (S8(o + 18) < 0) *B(o + 4) = 3;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* Set up by its fighter's side: the next step, +13 and +121 the side, +12 = -1 for the second side;
   those with +10 get +160 = 0x80. */
SFA_FUNC(08899590, setup_by_side) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = f;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = 0; *W(o + 148) = 0; *H(o + 86) = 8; *B(o + 78) = 0xFF;
    c->r[4] = o; c->r[5] = 1;
    f_0889CFE8(c);
    *B(o + 13) = (uint8_t)S8(f + 337);
    *B(o + 121) = (uint8_t)S8(f + 337);
    *B(o + 12) = S8(f + 337) != 0 ? 0xFF : 0;
    if (*B(o + 10) != 0) *B(o + 160) = 0x80;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* +82 goes round by 128 (back to 0 at 2560), then the substep's handler for a marked side. */
SFA_FUNC(0889961C, turn_82_run_for_marked_side) {
    gaddr o = ARG(0);
    *H(o + 82) = (uint16_t)(S16(o + 82) + 0x80);
    if (S16(o + 82) >= 2560) *H(o + 82) = 0;
    if (side_marked(ARG(1), 0)) run_step_with(c, *B(o + 5), 0x08A2A8B8u, ARG(1), 0x088996A0u);
}

/* The first of its kind (+10 = 0) shows its fighter's character: when that changes, the animation by
   side (0x08A287FE, 8 bytes each) is given the character's picture (0x127 + its number from
   0x08ACCAE6) and started. Drawn. */
SFA_FUNC(08899DF0, show_character_draw) {
    gaddr o = ARG(0), f = ARG(1), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(o + 10) == 0 && S8(f + 338) != S8(o + 78)) {
        *B(o + 78) = (uint8_t)S8(f + 338);
        gaddr entry = 0x08A287FEu + (uint32_t)(S8(f + 337) << 3);
        *H(entry + 4) = (uint16_t)(*B(0x08ACCAE6u + (uint32_t)(int32_t)S8(f + 338)) + 0x127);
        c->r[4] = o; c->r[5] = 0x08A287FEu + (uint32_t)(S8(f + 337) << 3);
        f_0882196C(c);
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

void f_0887D9D0(Cpu *c);   /* jitter_once */
void f_088988FC(Cpu *c);
void f_088A055C(Cpu *c);

/* Jittered, with a second object (kind 4, type its own + 14) made beside it sharing its +88 and +121;
   then its animation by type. */
SFA_FUNC(0887D684, jitter_make_second) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 9) = 0;
    f_0887D9D0(c);
    if (S8(o + 121) == 0) { *B(o + 12) = 0xFF; *B(o + 13) = 2; }
    c->r[4] = 4; c->r[5] = (*B(o + 3) + 0xEu) & 0xFF;
    f_08822EAC(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n + 121) = (uint8_t)S8(o + 121);
        *B(n + 135) = 0xFF;
        *W(n + 88) = *W(o + 88);
        *H(n + 82) = 0;
        *B(n + 119) = 0xFF;
    }
    c->r[4] = o;
    f_0887D990(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The length an object's scroll goes round in: the s8 +76 as the high byte with +77 as the low. */
static uint32_t round_length(gaddr o) { return (uint32_t)((S8(o + 76) << 8) + (S8(o + 77) & 0xFF)); }

/* Moves in y by its speed (+100), kept within 0 and its round length; drawn. */
SFA_FUNC(0889A1BC, scroll_y_round_draw) {
    gaddr o = ARG(0);
    int32_t length = (int32_t)(round_length(o) & 0xFFFF);
    *W(o + 20) += *W(o + 100);
    int16_t y = S16(o + 22);
    if (y < 0) {
        do y = (int16_t)(y + length); while (y < 0);
        *H(o + 22) = (uint16_t)y;
        y = S16(o + 22);
    }
    if ((int32_t)(uint16_t)y >= length) {
        do y = (int16_t)(y - length); while ((int32_t)(uint16_t)y >= length);
        *H(o + 22) = (uint16_t)y;
    }
    call_on(c, f_088215C4, o, 0x20);
}

/* The next step once GAME + 20 has bits 7 and 15; those with +10 move in x by +96, coming round by
   their length when they pass 0. Drawn. */
SFA_FUNC(0889FB04, scroll_x_round_draw) {
    gaddr o = ARG(0);
    if ((S16(GAME + 20) & 0x8080) == 0x8080) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    if (*B(o + 10) != 0) {
        uint32_t length = round_length(o) << 16;
        int32_t x = (int32_t)(*W(o + 96) + *W(o + 16));
        while (x < 0) x = (int32_t)(length + (uint32_t)x);
        *W(o + 16) = (uint32_t)x;
    }
    call_on(c, f_088215C4, o, 0x20);
}

/* Arrives in y: the next substep on its target, reporting bit 2 in GAME + 18 (those with +10 start
   moving in x at 72 with +9 = 1); and one substep more while the fighter's +424 is set. */
SFA_FUNC(0889BD08, arrive_y_report_18) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    f_0889CEA4(c);
    if (c->r[2] != 0) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        if (*B(o + 10) != 0) { *W(o + 96) = 0x480000; *B(o + 9) = 1; }
        *W(o + 20) = (uint32_t)S16(o + 114) << 16;
        *H(GAME + 18) = (uint16_t)(S16(GAME + 18) | 4);
        if (S16(f + 424) != 0) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* Once GAME + 20 has bit 15: the next substep, leaving for x -32 (or 416, by what 0x088988FC says of
   its owner) with +104 = -2.7 or 2.7 and +96 at 0. */
SFA_FUNC(0889F810, leave_by_owner_side) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    if (S16(GAME + 20) >= 0) return;
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = 0xFFFD4C00u; c->r[18] = 0xFFFFFFE0u;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    c->r[4] = *W(o + 92);
    f_088988FC(c);
    int right = c->r[2] != 0;
    *H(o + 114) = right ? 0x1A0 : 0xFFE0;
    *W(o + 104) = right ? 0x2B400u : 0xFFFD4C00u;
    *W(o + 96) = 0;
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (object, record, n): n kept in +36; type 3 goes to 0x088A055C with 16, the others start entry
   n / 8 (of its low byte) of the animation list picked from +96 by the record's +76. */
SFA_FUNC(088A0104, set_animation_by_n) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t n = ARG(2);
    *H(o + 36) = (uint16_t)n;
    c->r[29] -= 0x20;
    c->r[4] = o;
    if (*B(o + 3) == 3) { c->r[5] = 0x10; c->r[6] = n & 0xFFFF; f_088A055C(c); }
    else {
        gaddr list = *W(*W(o + 96) + (uint32_t)(S8(p + 76) << 2));
        c->r[5] = list + (((n & 0xFF) >> 3) << 3);
        f_0882196C(c);
    }
    c->r[29] += 0x20;
}

/* ---- 0.71.0: arriving, then the object's frame ---- */

void f_0889A1BC(Cpu *c);   /* scroll_y_round_draw */
void f_08899DF0(Cpu *c);   /* show_character_draw */
void f_088AD5A8(Cpu *c);   /* shift_x_13_frames */
void f_088B2754(Cpu *c);   /* shift_6_13_frames */

/* (object): moved in x to its target, the next part on arriving (and stopped on it with `snap`); then
   it scrolls round and is drawn. */
static void arrive_x_then_scroll(Cpu *c, int snap) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_0889CE64(c);
    if (c->r[2] != 0) { next_part(o); if (snap) snap_x_to_target(o); }
    c->r[4] = o;
    f_0889A1BC(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(0889A0C4, arrive_x_stop_then_scroll_a) { arrive_x_then_scroll(c, 1); }
SFA_FUNC(0889A3A8, arrive_x_stop_then_scroll_b) { arrive_x_then_scroll(c, 1); }
SFA_FUNC(0889B9E0, arrive_x_stop_then_scroll_c) { arrive_x_then_scroll(c, 1); }
SFA_FUNC(0889A154, arrive_x_then_scroll_a)      { arrive_x_then_scroll(c, 0); }
SFA_FUNC(0889A438, arrive_x_then_scroll_b)      { arrive_x_then_scroll(c, 0); }
SFA_FUNC(0889BA70, arrive_x_then_scroll_c)      { arrive_x_then_scroll(c, 0); }

/* (object, fighter): the same, then the fighter's character shown and drawn. */
static void arrive_x_then_show(Cpu *c, int snap) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    f_0889CE64(c);
    if (c->r[2] != 0) { next_part(o); if (snap) snap_x_to_target(o); }
    c->r[4] = o; c->r[5] = f;
    f_08899DF0(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(08899CCC, arrive_x_stop_then_show) { arrive_x_then_show(c, 1); }
SFA_FUNC(08899D78, arrive_x_on_then_show)    { arrive_x_then_show(c, 0); }

/* (object, fighter): on a bit of the fighter's +78 the next part, with the target (+114) back to its
   start (+112) and (`stop`) the speed +96 cleared; then the object's frame routine. */
static void on_fighter_bit(Cpu *c, uint32_t bit, int stop, GuestFunc then) {
    gaddr o = ARG(0), f = ARG(1);
    if (S8(f + 78) & bit) {
        next_part(o);
        *H(o + 114) = *H(o + 112);
        if (stop) *W(o + 96) = 0;
    }
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = f;
    then(c);
    c->r[29] += 0x20;
}
SFA_FUNC(08899D28, on_fighter_bit_1_show)   { on_fighter_bit(c, 1, 0, f_08899DF0); }
SFA_FUNC(0889A110, on_fighter_bit_2_scroll) { on_fighter_bit(c, 2, 1, f_0889A1BC); }
SFA_FUNC(0889A3F4, on_fighter_bit_4_scroll) { on_fighter_bit(c, 4, 1, f_0889A1BC); }
SFA_FUNC(0889BA2C, on_fighter_bit_8_scroll) { on_fighter_bit(c, 8, 1, f_0889A1BC); }
SFA_FUNC(0889A670, on_fighter_bit_8_lit)    { on_fighter_bit(c, 8, 1, f_0889A754); }

/* Jittered once, set to its first colour unless it is the second of a pair; then its animation by
   type. */
SFA_FUNC(0887D63C, jitter_then_animate) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 9) = 0;
    f_0887D9D0(c);
    if (S8(o + 121) == 0) { *B(o + 12) = 0xFF; *B(o + 13) = 2; }
    c->r[4] = o;
    f_0887D990(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* The thirteen-frame shifts as steps: the next substep (or +129 one more) on the last frame. */
SFA_FUNC(088AD1D4, shift_x_then_next_substep) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_088AD5A8, o)) *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
}
SFA_FUNC(088B270C, shift_6_then_count_129) {
    gaddr o = ARG(0);
    if (call_keeping(c, f_088B2754, o)) *B(o + 129) = (uint8_t)(*B(o + 129) + 1);
}

/* Set up by its type (+92): three words from 0x08A26950 (entry 3 × type - 51 on) for +8, +104 and the
   12-byte record at 0x08A26700 kept in +132. */
SFA_FUNC(08889CC8, setup_by_type_26950) {
    gaddr o = ARG(0);
    uint32_t i = *B(o + 92) * 3u - 51u;
    gaddr table = 0x08A26950u;
    *B(o + 54) = 0x12; *B(o + 88) = 2; *B(o + 55) = 5;
    *W(o + 4) = 0;
    *W(o + 8) = *W(table + i * 4u);
    *W(o + 104) = *W(table + (i + 1) * 4u);
    *B(o + 56) = 0; *B(o + 136) = 0x60;
    *W(o + 132) = 0x08A26700u + *W(table + (i + 2) * 4u) * 12u;
    *B(o + 93) = 1;
}

/* ---- 0.72.0 ---- */

/* As 0x08889CC8, from 0x08A26980 (entry 3 × type - 63 on): +4, +108 and the record kept in +132. */
SFA_FUNC(08889D64, setup_by_type_26980) {
    gaddr o = ARG(0);
    uint32_t i = *B(o + 92) * 3u - 63u;
    gaddr table = 0x08A26980u;
    *B(o + 54) = 0x12; *B(o + 88) = 2; *B(o + 55) = 5;
    *W(o + 4) = *W(table + i * 4u);
    *W(o + 8) = 0;
    *W(o + 108) = *W(table + (i + 1) * 4u);
    *B(o + 56) = 0; *B(o + 136) = 0x60;
    *W(o + 132) = 0x08A26700u + *W(table + (i + 2) * 4u) * 12u;
    *B(o + 93) = 1;
}

/* The record at 0x092E51C0 with its 54 parts (0x40 bytes from +0x34, the ones 0x0888D71C slides) set
   up: each shown, white (its three colours 0xFF), +55 = 7, +54 = 9, +60 = 0x8000. */
SFA_FUNC(0888D5B0, parts_record_setup) {
    gaddr r = 0x092E51C0u;
    *B(r + 3) = 0; *B(r + 2) = 0; *B(r) = 1;
    for (uint32_t i = 0; i < 54; i++) {
        gaddr part = r + 0x34 + i * 0x40u;
        *B(part + 55) = 7;
        *B(part + 1) = 1; *B(part + 3) = 0;
        for (uint32_t k = 0; k < 4; k++) { *B(part + 36 + k) = 0xFF; *B(part + 42 + k) = 0xFF; *B(part + 48 + k) = 0xFF; }
        *H(part + 60) = 0x8000;
        *B(part + 54) = 9;
    }
}

SFA_FUNC(088B1374, setup_088B1374) {
    gaddr o = ARG(0);
    *H(o + 32) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 7; *H(o + 86) = 0x13; *W(o + 164) = 1; *B(o + 9) = 1;
    *B(o + 78) = 0; *H(o + 38) = 0; *B(o + 15) = 0; *B(o + 11) = 0; *B(o + 12) = 0; *B(o + 13) = 0;
    *B(o + 14) = 2; *H(o + 24) = 7; *H(o + 26) = 3;
    *H(o + 18) = 0x250; *H(o + 22) = 0xFF90;
    set_animation(c, *W(0x08A2FFB0u + 72));
}

SFA_FUNC(088B1BB4, setup_088B1BB4) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0; *H(o + 86) = 0xC; *W(o + 164) = 0;
    *H(o + 38) = 0; *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0; *H(o + 82) = 0; *B(o + 10) = 0;
    *B(o + 12) = 0xFF; *B(o + 13) = 1; *H(o + 24) = 1; *H(o + 26) = 2;
    *H(o + 18) = 0; *H(o + 22) = 0xF0;
    set_animation(c, *W(0x08A30140u + 136));
}

/* Out of sight (+1 = 0), drawn while the high byte of GAME + 16 is set; types below 4 take the next
   step when GAME + 20 has bit 12 and the low byte of GAME + 16 is not their +32. */
SFA_FUNC(088B1D10, draw_by_game_16) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = GAME;
    *B(o + 1) = 0;
    if (S16(GAME + 16) & 0xFF00) { c->r[4] = o; f_088215C4(c); }
    if (*B(o + 3) < 4 && ((S16(GAME + 20) & 0xFF00) >> 8 & 0x10) && (S16(GAME + 16) & 0xFF) != S16(o + 32))
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* A record set up at (x, y) with a size (+14, +26), its colours cleared, +128 one more. */
static void record_place(gaddr r, uint32_t x, uint32_t y, uint16_t w, uint16_t h) {
    *B(r + 128) = (uint8_t)(*B(r + 128) + 1);
    *B(r + 93) = 1; *B(r + 55) = 7; *B(r + 56) = 0; *B(r + 1) = 0; *B(r + 3) = 0;
    *W(r + 4) = x; *W(r + 8) = y;
    for (uint32_t k = 0; k < 4; k++) { *B(r + 48 + k) = 0; *B(r + 42 + k) = 0; *B(r + 36 + k) = 0; }
    *H(r + 12) = 0; *H(r + 24) = 0;
    *H(r + 14) = w; *H(r + 26) = h;
}
SFA_FUNC(088B2104, record_place_a) { record_place(ARG(0), 0, 0xF00000, 0x180, 0x30); }
SFA_FUNC(088B2184, record_place_b) { record_place(ARG(0), 0, 0x400000, 0x180, 0x30); }
SFA_FUNC(088B232C, record_place_c) { *H(ARG(0) + 98) = 0; record_place(ARG(0), 0xFF260000u, 0xF00000, 0xC8, 0xF0); }
SFA_FUNC(088B23B4, record_place_d) { *H(ARG(0) + 98) = 0; record_place(ARG(0), 0x01B00000u, 0xF00000, 0x88, 0xF0); }

void f_0889FB94(Cpu *c);

/* (object, fighter): while the fighter's character is the one shown (+78) the object scrolls in x round
   the length +77 (in whole pixels); a change hides it (+78 = -1, y -64) while the fighter's +81 is
   set, else restarts its part (+6 = 1). Drawn. */
SFA_FUNC(0889C2EC, scroll_x_while_character) {
    gaddr o = ARG(0), f = ARG(1);
    if ((uint32_t)(uint8_t)S8(f + 338) != (uint32_t)(int32_t)S8(o + 78)) {
        if (S8(f + 81) != 0) { *B(o + 78) = 0xFF; *H(o + 22) = 0xFFC0; return; }
        *B(o + 6) = 1;
    }
    int32_t speed = (int32_t)*W(o + 96);
    int32_t length = (int32_t)(((uint32_t)S8(o + 77) & 0xFF) << 16);
    int32_t x = (int32_t)(*W(o + 16) + (uint32_t)speed);
    *W(o + 16) = (uint32_t)x;
    if (speed < 0) { if (x < 0) *W(o + 16) = (uint32_t)(x + length); }
    else if (!(x < length)) *W(o + 16) = (uint32_t)(x - length);
    call_on(c, f_088215C4, o, 0x20);
}

/* (object, n): row n chosen: +79 = n, y from 0x08A2AB68, the current frame's sprite (+156 by the
   animation's +4) given picture 0x8C + n; of the four buttons for its side (0x08A2882E, 64 bytes a
   side) the chosen one is lit (0x4000), the others show 0x8C + k and 0x8B; the last marked as the end. */
SFA_FUNC(0889C9FC, choose_row) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1);
    *B(o + 79) = (uint8_t)n;
    *H(o + 22) = *H(0x08A2AB68u + n * 2u);
    gaddr sprite = *W(o + 156) + (uint32_t)*H(*W(o + 28) + 4) * 8u;
    uint8_t colour = (uint8_t)((n << 1) + 0x24);
    *H(sprite + 2) = 0; *H(sprite + 4) = (uint16_t)(n + 0x8C); *B(sprite + 7) = colour;
    *H(sprite + 10) = 0; *B(sprite + 15) = colour; *H(sprite + 12) = 0x808B;
    gaddr e = 0x08A2882Eu + (uint32_t)(S8(*W(o + 92) + 337) << 6);
    for (uint32_t k = 0; k < 4; k++, e += 16) {
        if (k == n) { *H(e + 4) = 0x4000; *H(e + 12) = 0x4000; }
        else { *H(e + 4) = (uint16_t)(k + 0x8C); *H(e + 12) = 0x8B; }
    }
    *H(e - 4) = (uint16_t)(*H(e - 4) | 0x8000);
}

/* (object, k): entry k (8 bytes) of the motion table for the mode (0x08A2A3A0 / 0x08A2A5F8 in modes 15
   and 8 by bit 0 of GAME + 53, else 0x08A2A148): x and its target (+112, +18; +114), speeds +96, +104. */
SFA_FUNC(0889CEE4, set_motion_by_mode) {
    gaddr o = ARG(0);
    uint32_t i = ((ARG(1) & 0xFFFF) << 2 & 0xFFFF) * 2u;
    int8_t mode = S8(GAME + 114);
    gaddr table = (mode == 15 || mode == 8) ? ((S8(GAME + 53) & 1) ? 0x08A2A3A0u : 0x08A2A5F8u) : 0x08A2A148u;
    gaddr p = table + i;
    *H(o + 112) = (uint16_t)S16(p);
    *H(o + 114) = (uint16_t)S16(p + 2);
    *W(o + 96) = (uint32_t)(S16(p + 4) << 8);
    *H(o + 18) = (uint16_t)S16(p);
    *W(o + 104) = (uint32_t)(S16(p + 6) << 8);
}

/* y from 0x0889FB94; of the 6-byte entries at 0x08A2824E the eight from BATTLE + 72 shown and the
   ninth marked as the end; animation 0x08A283EC + 24. */
SFA_FUNC(0889EE08, show_eight_from_72) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *W(o + 148) = 2; *H(o + 86) = 8;
    c->r[4] = o;
    f_0889FB94(c);
    *H(o + 22) = (uint16_t)c->r[2];
    int32_t first = S16(BATTLE + 72);
    gaddr entries = 0x08A2824Eu;
    for (int32_t j = first; j < first + 8; j++) {
        gaddr e = entries + (uint32_t)(j * 6);
        *H(e + 4) = (uint16_t)(*H(e + 4) & 0x7FFF);
    }
    gaddr end = entries + (uint32_t)(first * 6);
    *H(end + 52) = (uint16_t)(*H(end + 52) | 0x8000);
    c->r[4] = o; c->r[5] = *W(0x08A283ECu + 24);
    f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* ---- 0.73.0 ---- */

void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0889C2EC(Cpu *c);   /* scroll_x_while_character */
void f_088B1D10(Cpu *c);   /* draw_by_game_16 */

/* Counts +82 down to 0; drawn while on the screen. */
SFA_FUNC(0887DD04, count_82_draw_if_on_screen) {
    gaddr o = ARG(0);
    if (S16(o + 82) != 0) *H(o + 82) = (uint16_t)(S16(o + 82) - 1);
    call_on(c, f_08821424, o, 0x20);
}

/* (object, fighter): arriving in y (the next part, stopped on its target), on bit 0 of the fighter's
   +78 (the next part, target back to +112, +100 cleared), or arriving in y or with the fighter's +81
   (the next part); then it scrolls while the character is shown. */
static void then_scroll_while_character(Cpu *c, gaddr o, gaddr f) {
    c->r[4] = o; c->r[5] = f;
    f_0889C2EC(c);
}
SFA_FUNC(0889C1E0, arrive_y_stop_then_scroll_character) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    f_0889CEA4(c);
    if (c->r[2] != 0) { next_part(o); *W(o + 20) = (uint32_t)S16(o + 114) << 16; }
    then_scroll_while_character(c, o, f);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(0889C23C, on_fighter_bit_1_scroll_character) {
    gaddr o = ARG(0), f = ARG(1);
    if (S8(f + 78) & 1) { next_part(o); *H(o + 114) = *H(o + 112); *W(o + 100) = 0; }
    c->r[29] -= 0x20;
    then_scroll_while_character(c, o, f);
    c->r[29] += 0x20;
}
SFA_FUNC(0889C290, arrive_y_or_81_then_scroll_character) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    f_0889CEA4(c);
    if (c->r[2] != 0 || S8(f + 81) != 0) next_part(o);
    then_scroll_while_character(c, o, f);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* Animated, then drawn by GAME + 16 (0x088B1D10). */
SFA_FUNC(088B1314, animate_then_draw_by_game_16) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088219B0(c);
    c->r[4] = o; f_088B1D10(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object, type, a, b): a jittered copy (kind 5) made beside the object: +9 = a, +78 = b, its
   +11, +14 and +24, x and y each moved by one of sixteen s8 at 0x08A2445A at random. Answers it. */
SFA_FUNC(0887E6EC, make_jittered_copy) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19];
    uint8_t a = (uint8_t)ARG(2), b = (uint8_t)ARG(3);
    c->r[29] -= 0x30;
    c->r[16] = o; c->r[18] = b; c->r[19] = a;
    c->r[4] = 5; c->r[5] = ARG(1) & 0xFF;
    f_08822EAC(c);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n != 0) {
        *B(n + 9) = a; *B(n + 78) = b;
        *B(n + 14) = *B(o + 14); *B(n + 11) = *B(o + 11); *B(n + 12) = 0; *H(n + 24) = *H(o + 24);
        int16_t x = S16(o + 18);
        c->r[18] = (uint32_t)(int32_t)x;
        f_08821368(c);
        c->r[19] = 0x08A2445Au;
        *H(n + 18) = (uint16_t)(x + S8(0x08A2445Au + (c->r[2] & 0xF)));
        int16_t y = S16(o + 22);
        c->r[16] = (uint32_t)(int32_t)y;
        f_08821368(c);
        *H(n + 22) = (uint16_t)(y + S8(0x08A2445Au + (c->r[2] & 0xF)));
    }
    RETURN(n);
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19;
}

/* Thrown (until +75 goes negative, then the next step): falls by +100 (growing by +108), and swings
   about x +96 by +104 (growing by 0xC000), to one side or the other on alternate frames (+82 and
   GAME + 66). Animated, drawn while on the screen. */
SFA_FUNC(08881EB4, swing_and_fall) {
    gaddr o = ARG(0), saved = c->r[16];
    if (S8(o + 75) < 0) { *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    c->r[29] -= 0x20;
    c->r[16] = o;
    *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
    uint32_t fall = *W(o + 100);
    *W(o + 20) += fall;
    *W(o + 100) = fall + *W(o + 108);
    uint32_t x = *W(o + 96);
    *W(o + 16) = x;
    uint32_t swing = *W(o + 104) + 0xC000u;
    *W(o + 104) = swing;
    *W(o + 16) = ((S16(o + 82) + S8(GAME + 66)) & 1) ? x - swing : x + swing;
    c->r[4] = o; f_088219B0(c);
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object, record): while the record is shown, +18 counts down; then the next frame of the table
   0x08A25808 (by +16) goes to the record's +11 and +18 is the frame after; a negative one ends it:
   the record put out, +16 back to 0 and +129 of the object at +20 set. Answers 0 for a record not
   shown, else 1. */
SFA_FUNC(0888639C, frames_from_table) {
    gaddr o = ARG(0), p = ARG(1);
    if (*B(p + 1) == 0) { RETURN(0); return; }
    *B(o + 18) = (uint8_t)(S8(o + 18) - 1);
    RETURN(1);
    if (S8(o + 18) >= 0) return;
    gaddr table = 0x08A25808u;
    int8_t i = S8(o + 16);
    *B(o + 16) = (uint8_t)(i + 1);
    int8_t wait = S8(table + (uint32_t)(int32_t)i);
    *B(o + 18) = (uint8_t)wait;
    int8_t j = S8(o + 16);
    *B(o + 16) = (uint8_t)(j + 1);
    if (wait >= 0) { *B(p + 11) = (uint8_t)S8(table + (uint32_t)(int32_t)j); return; }
    *B(p + 1) = 0;
    *B(p + 11) = (uint8_t)S8(table + (uint32_t)(int32_t)j);
    *B(o + 16) = 0;
    *B(*W(o + 20) + 129) = 1;
}

/* The next entry of the list at +120: -1 ends it (+128 one more); otherwise the record is placed at
   the object at +124 (x, y) with the size from 0x08A262E0 (by the entry's low seven bits) and shown;
   an entry with bit 7 only while that object's routine is below 2 (else +128 one more). */
SFA_FUNC(088894B8, record_follow_list) {
    gaddr r = ARG(0), at = *W(r + 120);
    *B(r + 93) = 0;
    int8_t v = S8(at);
    if (v == -1) { *B(r + 128) = (uint8_t)(*B(r + 128) + 1); return; }
    gaddr o = *W(r + 124);
    if (v >= 0) *W(r + 120) = at + 1;
    else if (!(*B(o + 4) < 2)) { *B(r + 128) = (uint8_t)(*B(r + 128) + 1); return; }
    *B(r + 93) = 1;
    *H(r + 6) = (uint16_t)S16(o + 18);
    *H(r + 10) = (uint16_t)S16(o + 22);
    gaddr e = 0x08A262E0u + (uint32_t)(v & 0x7F) * 8u;
    *H(r + 12) = (uint16_t)S16(e); *H(r + 14) = (uint16_t)S16(e + 2);
    *H(r + 24) = (uint16_t)S16(e + 4); *H(r + 26) = (uint16_t)S16(e + 6);
}

/* Arrives (in y for those with +10 bit 1, else in x; at once while BATTLE + 81) and goes to y 120,
   reporting bits 0 and 1 in GAME + 20; drawn. Only in substep 0. */
SFA_FUNC(08897B80, arrive_then_y_120) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (*B(o + 5) == 0) {
        int there = S8(BATTLE + 81) != 0;
        if (*B(o + 10) & 2) {
            if (!there) { c->r[4] = o; f_0889CEA4(c); there = c->r[2] != 0; }
        } else {
            if (!there) { c->r[4] = o; f_0889CE64(c); there = c->r[2] != 0; }
            if (there) *W(o + 16) = (uint32_t)S16(o + 114) << 16;
        }
        if (there) {
            *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
            *W(o + 20) = 0x780000;
            *H(GAME + 20) = (uint16_t)(S16(GAME + 20) | 3);
        }
    }
    c->r[4] = o; f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}

/* (object, fighter): with the fighter's +424 bit 1 the next substep; otherwise set up once more (the
   next part): +82 = 0x600, +80 / +81 = -128, colours by side, motion entry 65 + side, animation 8 +
   side. */
SFA_FUNC(0889AB98, setup_by_side_unless_flag_2) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    if (S16(f + 424) & 2) { *B(o + 6) = 0; *B(o + 5) = (uint8_t)(*B(o + 5) + 1); return; }
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    next_part(o);
    *W(o + 148) = 0; *H(o + 86) = 8; *H(o + 82) = 0x600; *B(o + 81) = 0x80; *B(o + 80) = 0x80;
    c->r[4] = o; c->r[5] = 1;
    f_0889CFE8(c);
    int32_t side = S8(f + 337);
    c->r[16] = (uint32_t)side;
    *B(o + 12) = 1;
    *B(o + 13) = (uint8_t)(side + 0x14);
    c->r[4] = o; c->r[5] = (uint32_t)(side + 0x41) & 0xFFFF;
    f_0889CEE4(c);
    c->r[4] = o; c->r[5] = (uint32_t)(side + 8) & 0xFFFF;
    f_0889CDC8(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---- 0.74.0 ---- */

void f_08823338(Cpu *c);   /* object_make_list_3 (pool.c) */
void rt_call_indirect(Cpu *c, uint32_t addr);

/* (object): jittered copies of type 0 with +78 = 16 or 17 (0x0887E6EC). */
static void jittered_copy(Cpu *c, uint32_t b) {
    c->r[29] -= 0x20;
    c->r[5] = 0; c->r[6] = 0; c->r[7] = b;
    f_0887E6EC(c);
    c->r[29] += 0x20;
}
SFA_FUNC(0887E230, jittered_copy_16) { jittered_copy(c, 0x10); }
SFA_FUNC(0887E254, jittered_copy_17) { jittered_copy(c, 0x11); }

/* (object): while BATTLE + 78 is clear, animated (or, when +75 is negative, counted out and freed
   instead); then +82 counted down and drawn while on the screen. With `follow`, the object first
   takes its owner's place by the owner's character's offsets (routine table + 24). */
static void animate_then_count_82(Cpu *c, int follow) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(BATTLE + 78) == 0) {
        if (S8(o + 75) < 0) {
            c->r[4] = o; f_0887DCCC(c);
            c->r[29] += 0x20;
            c->r[16] = saved;
            return;
        }
        if (follow) {
            c->r[4] = o; c->r[5] = *W(*W(*W(o + 88) + 416) + 24);
            f_0887DA58(c);
        }
        c->r[4] = o; f_088219B0(c);
    }
    c->r[4] = o; f_0887DD04(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(0887DAFC, animate_then_count_82_a)        { animate_then_count_82(c, 0); }
SFA_FUNC(0887DC54, follow_animate_then_count_82)   { animate_then_count_82(c, 1); }

/* (object): +1 cleared; drawn while on the screen when GAME + 16's high byte is 0; +4 counts on
   while GAME + 16's low byte is not +32. Four objects share it. */
static void draw_until_game_16(Cpu *c) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = GAME;
    *B(o + 1) = 0;
    if ((*H(GAME + 16) >> 8) == 0) { c->r[4] = o; f_08821424(c); }
    if ((S16(GAME + 16) & 0xFF) != S16(o + 32)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}
SFA_FUNC(088AD538, draw_until_game_16_a) { draw_until_game_16(c); }
SFA_FUNC(088ADC44, draw_until_game_16_b) { draw_until_game_16(c); }
SFA_FUNC(088AF2D0, draw_until_game_16_c) { draw_until_game_16(c); }
SFA_FUNC(088B01A0, draw_until_game_16_d) { draw_until_game_16(c); }

/* (record): the next step (+128); +55 = 7, +1 = 1, +3 = 0; +120 and +56 from the tables 0x08A26468
   (words) and 0x08A2649C (bytes) by +92; +36, +42 and +48 set; then it follows its list. */
SFA_FUNC(08889440, record_start_by_92) {
    gaddr r = ARG(0);
    uint8_t n = *B(r + 92);
    *B(r + 128) = (uint8_t)(*B(r + 128) + 1);
    *B(r + 55) = 7; *B(r + 1) = 1; *B(r + 3) = 0;
    *W(r + 120) = *W(0x08A26468u + n * 4u);
    *B(r + 56) = *B(0x08A2649Cu + n);
    *B(r + 36) = 1; *B(r + 42) = 1; *B(r + 48) = 1;
    call_on(c, f_088894B8, r, 0x20);
}

/* (record): +2 = 0 runs the routine 0x08A43C50[+4], +2 = 31 the routine 0x08A43C60[+4], with
   (record, +88); other values nothing. */
SFA_FUNC(088F1718, record_run_by_2) {
    gaddr r = ARG(0);
    uint8_t kind = *B(r + 2);
    gaddr arg = *W(r + 88), table;
    if (kind == 0) table = 0x08A43C50u;
    else if (kind == 0x1F) table = 0x08A43C60u;
    else return;
    gaddr fn = *W(table + *B(r + 4) * 4u);
    c->r[29] -= 0x20;
    c->r[4] = r; c->r[5] = arg; c->r[6] = arg; c->r[7] = fn;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* (object): the next step (+4); shown (+9 = 1), +24 = 7, +14 / +11 cleared, at (x, 16), +12 = 1,
   +13 = 0x31, +86 = 0x15, +148 = 6, +156 = 0x08A38100; animation 0x08A3810C. */
static void start_at_x(Cpu *c, int16_t x) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 1; *H(o + 24) = 7; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 18) = (uint16_t)x; *H(o + 22) = 0x10;
    *B(o + 12) = 1; *B(o + 13) = 0x31; *H(o + 86) = 0x15;
    *W(o + 148) = 6; *W(o + 156) = 0x08A38100u;
    c->r[29] -= 0x20;
    c->r[4] = o; c->r[5] = 0x08A3810Cu;
    f_0882196C(c);
    c->r[29] += 0x20;
}
SFA_FUNC(088BB820, start_at_left)  { start_at_x(c, -0x20); }
SFA_FUNC(088BB93C, start_at_right) { start_at_x(c, 0x180); }

/* (object): the next step (+4); hidden (+9 = 0), +24 = 7, +14 / +11 cleared, at (0, y), +12 = 1,
   +13 = 0x30, +86 = 0x15, +84 = n, +148 = 3; a list-3 object of type 9 made with `b`, its +124
   pointing back. */
static void start_with_partner(Cpu *c, uint16_t y, uint16_t n, uint32_t b) {
    gaddr o = ARG(0), saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0; *H(o + 24) = 7; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 18) = 0; *H(o + 22) = y;
    *B(o + 12) = 1; *B(o + 13) = 0x30; *H(o + 86) = 0x15; *H(o + 84) = n;
    *W(o + 148) = 3;
    gaddr p = CALL2(f_08823338, 9, b);
    if (p != 0) *W(p + 124) = o;
    c->r[29] += 0x20;
    c->r[16] = saved;
}
SFA_FUNC(088BB510, start_with_partner_low)  { start_with_partner(c, 0x120, 3, 0); }
SFA_FUNC(088BB61C, start_with_partner_high) { start_with_partner(c, 0x10, 2, 1); }

/* (record): the next step (+128); +93 = 1, +55 = 7, +98 / +56 / +1 / +3 / +4 cleared, +8 = `w`, the
   bytes +36 - +39, +42 - +45, +48 - +51 cleared; +12 / +24 = 0, +14 = 0x180, +26 = `h`. */
static void record_reset_at(gaddr r, uint32_t w, uint16_t h) {
    *B(r + 128) = (uint8_t)(*B(r + 128) + 1);
    *H(r + 98) = 0; *B(r + 93) = 1; *B(r + 55) = 7; *B(r + 56) = 0; *B(r + 1) = 0; *B(r + 3) = 0;
    *W(r + 4) = 0; *W(r + 8) = w;
    static const uint8_t cleared[] = { 51, 50, 49, 48, 45, 44, 43, 42, 39, 38, 37, 36 };
    for (unsigned i = 0; i < sizeof cleared; i++) *B(r + cleared[i]) = 0;
    *H(r + 12) = 0; *H(r + 24) = 0; *H(r + 14) = 0x180; *H(r + 26) = h;
}
SFA_FUNC(088B243C, record_reset_f0_b0) { record_reset_at(ARG(0), 0xF00000u, 0xB0); }
SFA_FUNC(088B24C0, record_reset_50_40) { record_reset_at(ARG(0), 0x500000u, 0x40); }

/* (record, values): +12 / +24 cleared; +14, +26, +60, +64, +76, +66, +78 from the values' u16 0, 1,
   word 4, u16 8, 10, 12, 14. */
SFA_FUNC(088EB1A4, record_set_from_values) {
    gaddr r = ARG(0), v = ARG(1);
    *H(r + 12) = 0; *H(r + 24) = 0;
    *H(r + 14) = *H(v);
    *H(r + 26) = *H(v + 2);
    *W(r + 60) = *W(v + 4);
    *H(r + 64) = *H(v + 8);
    *H(r + 76) = *H(v + 10);
    *H(r + 66) = *H(v + 12);
    *H(r + 78) = *H(v + 14);
}

/* (object, fighter): shown (+4 = 1); +86, +164, +24, +9, +12, +13 and +14 from the fighter, +3 its
   character (+338); +78 cleared. */
SFA_FUNC(0892EFB8, object_copy_from_fighter) {
    gaddr o = ARG(0), f = ARG(1);
    *B(o + 4) = 1;
    *H(o + 86) = *H(f + 86);
    *W(o + 164) = *W(f + 164);
    *H(o + 24) = *H(f + 24);
    *B(o + 9) = *B(f + 9);
    *B(o + 3) = *B(f + 338);
    *B(o + 12) = *B(f + 12);
    *B(o + 13) = *B(f + 13);
    uint8_t b14 = *B(f + 14);
    *B(o + 78) = 0;
    *B(o + 14) = b14;
}

/* (object): x (+16, 16.16) moved by the speed +96 (backwards while +11 is 0), and the speed changed
   by +104. */
SFA_FUNC(0892EA9C, object_move_x_by_speed) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 96);
    *W(o + 16) += *B(o + 11) == 0 ? 0u - speed : speed;
    *W(o + 96) = speed + *W(o + 104);
}

/* (object): a blink: +832 counts frames, +828 is 1 for the frame it turns over, after 23 frames the
   first time (+836 clear) and 15 after. */
SFA_FUNC(0892B188, object_blink_timer) {
    gaddr o = ARG(0);
    uint32_t n = *W(o + 832) + 1, again = *W(o + 836);
    *W(o + 832) = n;
    *W(o + 828) = 0;
    if (n != (again != 0 ? 15u : 23u)) return;
    *W(o + 832) = 0; *W(o + 828) = 1; *W(o + 836) = 1;
}

/* (object): +82 counts up, the next step (+4) once past 40; +10 counts up, and from 14 on the object
   sinks (+20 += 0x18000) while above y 152 (+22); then it is drawn as 0x088B01A0. */
SFA_FUNC(088AFC90, count_then_sink_draw) {
    gaddr o = ARG(0);
    int16_t t = (int16_t)(S16(o + 82) + 1);
    *H(o + 82) = (uint16_t)t;
    if (t >= 41) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    uint8_t n = (uint8_t)(*B(o + 10) + 1);
    *B(o + 10) = n;
    if (n >= 14 && S16(o + 22) < 152) *W(o + 20) += 0x18000u;
    call_on(c, f_088B01A0, o, 0x20);
}

/* (object, fighter): with the fighter's +424 bit 3, the next step (+5 cleared, +4 on); otherwise the
   next part: +148 = 1, +86 = 8, layer 1, motion entry 23 + side, +20 cleared, +100 = -2.0, animation
   0x13 (0x0889CE34). */
SFA_FUNC(0889AE58, setup_side_23_unless_flag_8) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    if (S16(f + 424) & 8) { *B(o + 5) = 0; *B(o + 4) = (uint8_t)(*B(o + 4) + 1); return; }
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    next_part(o);
    *W(o + 148) = 1; *H(o + 86) = 8;
    c->r[4] = o; c->r[5] = 1; f_0889CFE8(c);
    c->r[4] = o; c->r[5] = (uint32_t)(S8(f + 337) + 0x17) & 0xFFFF; f_0889CEE4(c);
    *W(o + 20) = 0; *W(o + 100) = 0xFFFE0000u;
    c->r[4] = o; c->r[5] = 0x13; f_0889CE34(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object, fighter): with the fighter's +424 bit 1, the next substep's count (+5) on; otherwise the
   next part: +148 / +164 cleared, +86 = 8, +160 = 0xC0, layer 1, y 0x80, motion entry 8 + side,
   animation 4 + 2 * side (0x0889CDC8). */
SFA_FUNC(0889B544, setup_side_8_unless_flag_2) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    if (S16(f + 424) & 2) { *B(o + 5) = (uint8_t)(*B(o + 5) + 1); return; }
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    next_part(o);
    *W(o + 148) = 0; *H(o + 86) = 8; *W(o + 164) = 0; *B(o + 160) = 0xC0;
    c->r[4] = o; c->r[5] = 1; f_0889CFE8(c);
    *H(o + 22) = 0x80;
    int32_t side = S8(f + 337);
    c->r[16] = (uint32_t)side;
    c->r[4] = o; c->r[5] = (uint32_t)(side + 8) & 0xFFFF; f_0889CEE4(c);
    c->r[4] = o; c->r[5] = (uint32_t)(side * 2 + 4) & 0xFFFF; f_0889CDC8(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (record): +94 = GAME + 16's low byte; the next step (+128); +93 = 1, +55 = 6, +56 / +1 / +3 / +4
   cleared, +8 = 0xFF0000, the bytes +36 - +39, +42 - +45, +48 - +51 = 0xFF; +136 = 0xFF for +92 of
   14, 16 and 17, 0x7F for 15; +12 / +24 = 0, +14 = 0x180, +26 = 0xF0; +55 / +56 = 7 for +92 = 17. */
SFA_FUNC(088B2B98, record_reset_by_92) {
    gaddr r = ARG(0);
    *H(r + 94) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(r + 128) = (uint8_t)(*B(r + 128) + 1);
    *B(r + 93) = 1; *B(r + 55) = 6; *B(r + 56) = 0; *B(r + 1) = 0; *B(r + 3) = 0;
    *W(r + 4) = 0; *W(r + 8) = 0xFF0000u;
    static const uint8_t full[] = { 39, 38, 37, 36, 45, 44, 43, 42, 51, 50 };
    for (unsigned i = 0; i < sizeof full; i++) *B(r + full[i]) = 0xFF;
    uint8_t k = *B(r + 92);
    *B(r + 49) = 0xFF; *B(r + 48) = 0xFF;
    if (k == 14 || k == 16 || k == 17) *B(r + 136) = 0xFF;
    else if (k == 15) *B(r + 136) = 0x7F;
    *H(r + 12) = 0;
    *H(r + 24) = 0; *H(r + 14) = 0x180; *H(r + 26) = 0xF0;
    if (k == 0x11) { *B(r + 56) = 7; *B(r + 55) = 7; }
}
