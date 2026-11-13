/* Routines that pick a handler from a table, with a condition or a little work around the call. */
#include "step.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline int8_t    S8(gaddr a) { return (int8_t)*B(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline int16_t   S16(gaddr a) { return (int16_t)*H(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }

/* A fighter's routine by +484 (table 0x08A22488), unless BATTLE + 14 or the fighter's +788 stops it;
   +506 is first held to 255. */
SFA_FUNC(088772C0, fighter_run_484) {
    gaddr f = ARG(0);
    if (S8(BATTLE + 14) != 0 || *B(f + 788) != 0) return;
    if (S16(f + 506) >= 256) *H(f + 506) = 0xFF;
    run_step(c, *B(f + 484), 0x08A22488u, 5, 0x0887731Cu);
}

/* An object shown (+1) and run by its step +5 (table 0x08A25A28) until BATTLE + 82 reaches 2: then it is
   hidden and its routine steps on. */
SFA_FUNC(08886F50, object_run_until_82) {
    gaddr o = ARG(0);
    *B(o + 1) = 1;
    if (S8(BATTLE + 82) == 2) {
        *B(o + 1) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        return;
    }
    run_step(c, *B(o + 5), 0x08A25A28u, 5, 0x08886FB0u);
}

/* The object at 0x092E51C0, once a frame: its routine by +2 (table 0x08A26CB0) unless BATTLE + 23, then
   its clock: +6 counts frames (not while GAME + 89 or + 145), +8 counts every fourth and +9 every third. */
#define TICKER 0x092E51C0u
SFA_FUNC(0888D64C, ticker_frame) {
    gaddr s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = TICKER; c->r[17] = GAME;
    if (S8(BATTLE + 23) == 0 && *B(TICKER) != 0) {
        gaddr fn = *W(0x08A26CB0u + *B(TICKER + 2) * 4u);
        c->r[4] = TICKER; c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
    if (S8(GAME + 89) == 0 && S8(GAME + 145) == 0) {
        uint16_t t = (uint16_t)(*H(TICKER + 6) + 1);
        *H(TICKER + 6) = t;
        if ((t & 3) == 0) *B(TICKER + 8) = (uint8_t)(*B(TICKER + 8) + 1);
        if (t % 3 == 0) *B(TICKER + 9) = (uint8_t)(*B(TICKER + 9) + 1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* An object with two parts (at +12 and +32, each starting with a pointer to its fighter and with its
   own step at +4): each part's routine (table 0x08A26CBC) is called with the part, its fighter and 0 or
   1, unless that fighter's +313 is set. */
SFA_FUNC(0888DA7C, object_run_two_parts) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    gaddr first = *W(o + 12), second = *W(o + 32);
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = second;
    if (S8(first + 313) == 0) {
        gaddr fn = *W(0x08A26CBCu + (uint32_t)S8(o + 16) * 4u);
        c->r[4] = o + 12; c->r[5] = first; c->r[6] = 0; c->r[7] = fn;
        rt_call_indirect(c, fn);
    }
    if (S8(c->r[17] + 313) == 0) {
        gaddr fn = *W(0x08A26CBCu + (uint32_t)S8(c->r[16] + 36) * 4u);
        c->r[4] = c->r[16] + 32; c->r[5] = c->r[17]; c->r[6] = 1; c->r[7] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* ---- by stage and by layout ---- */

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define BT_STAGE  0
#define BT_LAYOUT 56

void f_0881E760(Cpu *c);   /* camera_span_by_pair (stage/camera.c) */
void f_0881EB14(Cpu *c);   /* camera_height_by_pair */
void f_0882D6FC(Cpu *c);   /* fighter_1_trail_step (fighter/trail.c) */
void f_0882D794(Cpu *c);   /* fighter_2_trail_step */

/* The stage's own routine (0x08A0893C by stage), if it has one. */
SFA_FUNC(088192C0, stage_routine) {
    gaddr fn = *W(0x08A0893Cu + (uint32_t)S16(BATTLE + BT_STAGE) * 4u);
    c->r[5] = fn; c->r[6] = 0x08A0893Cu;
    if (fn == 0) return;
    c->r[29] -= 0x20;
    rt_call_indirect(c, fn);
    c->r[29] += 0x20;
}

/* The camera's span and height routines for the layout (who fights whom). */
SFA_FUNC(0881E64C, camera_span_by_layout)   { run_step(c, (uint32_t)S16(BATTLE + BT_LAYOUT), 0x08A09428u, 6, 0x0881E67Cu); }
SFA_FUNC(0881EA10, camera_height_by_layout) { run_step(c, (uint32_t)S16(BATTLE + BT_LAYOUT), 0x08A09448u, 5, 0x0881EA40u); }

/* The usual layout's: fighters 1 and 2. */
SFA_FUNC(0881E688, camera_span_1_2) {
    uint32_t lo = ARG(0), hi = ARG(1);
    c->r[29] -= 0x20;
    c->r[4] = FIGHTER_1; c->r[5] = FIGHTER_2; c->r[6] = lo; c->r[7] = hi;
    f_0881E760(c);
    c->r[29] += 0x20;
}
SFA_FUNC(0881EA4C, camera_height_1_2) {
    uint32_t out = ARG(0);
    c->r[29] -= 0x20;
    c->r[4] = FIGHTER_1; c->r[5] = FIGHTER_2; c->r[6] = out;
    f_0881EB14(c);
    c->r[29] += 0x20;
}

/* The trails' routine for the layout, and the usual one: fighter 1's trail at 0x09523CA8 and fighter
   2's 0x480 after it. */
SFA_FUNC(0882D5D8, trails_by_layout) {
    gaddr fn = *W(0x08A0F1A4u + (uint32_t)S16(BATTLE + BT_LAYOUT) * 4u);
    c->r[4] = fn; c->r[5] = 0x08A0F1A4u;
    c->r[29] -= 0x10;
    rt_call_indirect(c, fn);
    c->r[29] += 0x10;
}
#define TRAILS 0x09523CA8u
SFA_FUNC(0882D614, trails_1_2) {
    gaddr saved = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = TRAILS;
    c->r[4] = TRAILS; f_0882D6FC(c);
    c->r[4] = c->r[16] + 0x480; f_0882D794(c);
    c->r[29] += 0x20;
    c->r[16] = saved;
}
