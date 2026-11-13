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
