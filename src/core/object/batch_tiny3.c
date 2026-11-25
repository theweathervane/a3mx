/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 7 - 12
   instructions, ready once batch_tiny1.c / batch_tiny2.c were done: wrappers that set an argument or
   two first (an animation from a table, a counter stepped), places moved by speeds with a pull, fields
   set when a step starts, and the owner's links cleared before an object is freed. Offsets stay
   numbers until something shows what they are.
 */
#include "../decomp.h"
#include "../guest_mem.h"

void f_089E1F70(Cpu *c); void f_088E3B18(Cpu *c); void f_0887E6EC(Cpu *c); void f_08881FB8(Cpu *c);
void f_0882196C(Cpu *c); void f_088764A8(Cpu *c); void f_08979D64(Cpu *c); void f_088231C8(Cpu *c);
void f_089DA960(Cpu *c); void f_088718DC(Cpu *c); void f_0887B1A8(Cpu *c); void f_088230F4(Cpu *c);
void f_089E0038(Cpu *c); void f_089E0128(Cpu *c); void f_088D31AC(Cpu *c); void f_088D31CC(Cpu *c);
void f_08908BA8(Cpu *c); void f_08908C04(Cpu *c); void f_08912FBC(Cpu *c); void f_08913018(Cpu *c);
void f_0882324C(Cpu *c); void f_088219B0(Cpu *c); void f_0887684C(Cpu *c);

/* One call inside a frame of `frame` bytes with the return address kept at `slot`, as the originals do;
   the callee gets the argument registers as they are, and `back` as its return address. */
static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
#define WRAP10(back, fn) wrap(c, 0x10, 0, back, fn)

/* Two calls on the same object, kept in s0 (saved at +16 of a 0x20 frame, the return address at +20). */
static void both(Cpu *c, uint32_t back, void (*first)(Cpu *), void (*second)(Cpu *)) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = back; first(c);
    ARG(0) = o; c->r[31] = back + 8; second(c);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }

/* (object, table, n): set_animation (0x0882196C) with table[n]; a2 keeps n * 4, as the original's. */
static void animation_from(Cpu *c, gaddr table, uint32_t n, uint32_t back) {
    ARG(2) = table;
    ARG(1) = *W(table + n * 4);
    WRAP20(back, f_0882196C);
}

/* ---------------------------------------------------------------- wrappers */
SFA_FUNC(088098CC, call_089E1F70) { WRAP10(0x088098DCu, f_089E1F70); }
SFA_FUNC(088E3BD0, call_088E3B18) { WRAP20(0x088E3BE0u, f_088E3B18); }
/* (object): 0x0887E6EC (object, 0, 1, 0xF). */
SFA_FUNC(0887E278, call_0887E6EC_0F) { ARG(1) = 0; ARG(2) = 1; ARG(3) = 0xF; WRAP20(0x0887E290u, f_0887E6EC); }
/* (object): a kind-0x16 object of type 0 made beside it, shown (0x08881FB8 (object, 0, 1, 0x16)). */
SFA_FUNC(08881F94, make_beside_16) { ARG(1) = 0; ARG(2) = 1; ARG(3) = 0x16; WRAP20(0x08881FACu, f_08881FB8); }
/* (object, table, n): animation table[n]. */
SFA_FUNC(088D3B64, animation_by_index) {
    uint32_t n4 = ARG(2) << 2;
    ARG(2) = n4;
    ARG(1) = *W(ARG(1) + n4);
    WRAP20(0x088D3B7Cu, f_0882196C);
}
/* (object, n): animation n of the tables at 0x08A53DF0 / 0x08A6129C / 0x08A24F80 (n's low half). */
SFA_FUNC(0890DAC8, animation_08A53DF0) { animation_from(c, 0x08A53DF0u, ARG(1), 0x0890DAE8u); }
SFA_FUNC(0891E914, animation_08A6129C) { animation_from(c, 0x08A6129Cu, ARG(1), 0x0891E934u); }
SFA_FUNC(088828CC, animation_08A24F80) { animation_from(c, 0x08A24F80u, ARG(1) & 0xFFFF, 0x088828F0u); }
/* (object): animation by its +78 from 0x08A6F05C. */
SFA_FUNC(0893B460, animation_by_78) { animation_from(c, 0x08A6F05Cu, (uint32_t)(int32_t)S8(ARG(0) + 78), 0x0893B484u); }
/* (object): +4 on, then the animation at 0x08A737BC. */
SFA_FUNC(08941AE8, step_on_4_animation) {
    inc8(ARG(0) + 4);
    ARG(2) = 0x08A70000u;
    ARG(1) = *W(0x08A737BCu);
    WRAP20(0x08941B08u, f_0882196C);
}
/* (object): its owner's (+88) +502 = 1, then 0x088764A8 (object, owner, 1). */
SFA_FUNC(08900A98, owner_502_then_088764A8) {
    gaddr w = *W(ARG(0) + 88);
    ARG(1) = w; ARG(2) = 1;
    *B(w + 502) = 1;
    WRAP20(0x08900AB0u, f_088764A8);
}
/* (object): +4 on, then its routine run (0x08979D64 / 0x088231C8) with the new +4 in a1. */
SFA_FUNC(08979D40, step_on_4_run_a) { ARG(1) = inc8(ARG(0) + 4); WRAP20(0x08979D58u, f_08979D64); }
SFA_FUNC(089885CC, step_on_4_run_b) { ARG(1) = inc8(ARG(0) + 4); WRAP20(0x089885E4u, f_088231C8); }
/* (object): +5 on, then 0x089DA960 (nothing) with the new +5 in a2. */
SFA_FUNC(089DA93C, step_on_5_call_089DA960) { ARG(2) = inc8(ARG(0) + 5); WRAP20(0x089DA954u, f_089DA960); }
/* (fighter): the hit record's (0x09358410) +46 = 1, then 0x088718DC (+117 = 0xD, +698 = 1). */
SFA_FUNC(088718B4, hit_46_then_mark) {
    ARG(3) = 0x09358410u; ARG(2) = 1;
    *B(0x09358410u + 46) = 1;
    WRAP20(0x088718D0u, f_088718DC);
}
/* (fighter): the CPU's first script and command cleared (0x0887B1A8) while +6 is 0. */
SFA_FUNC(0887AF28, clear_481_if_6_a) {
    ARG(1) = *B(ARG(0) + 6);
    if (ARG(1) == 0) WRAP20(0x0887AF44u, f_0887B1A8);
}
SFA_FUNC(0887B180, clear_481_if_6_b) {
    ARG(1) = *B(ARG(0) + 6);
    if (ARG(1) == 0) WRAP20(0x0887B19Cu, f_0887B1A8);
}
/* (object): its owner's (+88) +540 counted down, then 0x088230F4 (object, owner, the count). */
SFA_FUNC(0891D34C, owner_540_down_then_088230F4) {
    gaddr w = *W(ARG(0) + 88);
    int32_t n = S8(w + 540) - 1;
    *B(w + 540) = (uint8_t)n;
    ARG(1) = w; ARG(2) = (uint32_t)n;
    WRAP20(0x0891D368u, f_088230F4);
}
/* (record): 0x089E0038 / 0x089E0128 with its +33 / +41 when that is set. */
SFA_FUNC(089DE694, call_089E0038_by_33) {
    ARG(0) = (uint32_t)(int32_t)S8(ARG(0) + 33);
    if (ARG(0) != 0) WRAP10(0x089DE6B0u, f_089E0038);
}
SFA_FUNC(089DE798, call_089E0128_by_41) {
    ARG(0) = (uint32_t)(int32_t)S8(ARG(0) + 41);
    if (ARG(0) != 0) WRAP10(0x089DE7B4u, f_089E0128);
}
/* (object): moved on x, then on y (0x088D31AC, 0x088D31CC), and the like. */
SFA_FUNC(088D31EC, move_x_then_y) { both(c, 0x088D3200u, f_088D31AC, f_088D31CC); }
SFA_FUNC(08908BD8, both_08908BA8_08908C04) { both(c, 0x08908BECu, f_08908BA8, f_08908C04); }
SFA_FUNC(08912FEC, both_08912FBC_08913018) { both(c, 0x08913000u, f_08912FBC, f_08913018); }
/* (object): its owner's (+88) +228 cleared if it is this object, then 0x088230F4 (object, owner, the old +228). */
SFA_FUNC(0893A6F8, unlink_228_then_088230F4) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint32_t was = *W(w + 228);
    if (was == o) *W(w + 228) = 0;
    ARG(1) = w; ARG(2) = was;
    WRAP20(0x0893A718u, f_088230F4);
}
/* (object): its owner's (+88) +40 / +44 cleared if it is this object, then freed (0x0882324C). */
static void unlink_then_free(Cpu *c, uint32_t at, uint32_t back) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint32_t was = *W(w + at);
    if (was == o) *W(w + at) = 0;
    ARG(1) = w; ARG(2) = was;
    WRAP20(back, f_0882324C);
}
SFA_FUNC(089879C4, unlink_40_then_free) { unlink_then_free(c, 40, 0x089879E4u); }
SFA_FUNC(089CB4E0, unlink_44_then_free) { unlink_then_free(c, 44, 0x089CB500u); }
/* (object): +5 on, +0x52 = 0x1D, then animate (0x088219B0). */
SFA_FUNC(0887F418, step_on_5_52_animate) {
    gaddr o = ARG(0);
    inc8(o + 5);
    *B(o + 0x52) = 0x1D;
    ARG(1) = o + 0x52; ARG(2) = 0x1D;
    WRAP20(0x0887F43Cu, f_088219B0);
}
/* (object): its owner's (+88) +703 cleared unless +73 is set, then its routine run (0x088231C8). */
SFA_FUNC(088804B0, owner_703_then_run) {
    gaddr o = ARG(0);
    int8_t b73 = S8(o + 73);
    ARG(1) = (uint32_t)(int32_t)b73;
    if (b73 == 0) { ARG(1) = *W(o + 88); *B(ARG(1) + 703) = 0; }
    WRAP20(0x088804D4u, f_088231C8);
}
/* (fighter): 0x0887684C (fighter, +346 + 5, or 0 when +347 is clear). */
SFA_FUNC(0891C6C4, call_0887684C_by_346) {
    gaddr f = ARG(0);
    int8_t on = S8(f + 347);
    ARG(2) = (uint32_t)(int32_t)on;
    ARG(1) = on == 0 ? 0 : (uint32_t)(S8(f + 346) + 5) & 0xFFFF;
    WRAP20(0x0891C6E8u, f_0887684C);
}
/* (object): +5 on, its owner's (+88) +228 cleared, then 0x0887684C (object, 0xB). */
SFA_FUNC(08940E38, step_on_5_call_0887684C_0B) {
    gaddr o = ARG(0), w = *W(o + 88);
    inc8(o + 5);
    *W(w + 228) = 0;
    ARG(2) = w; ARG(1) = 0xB;
    WRAP20(0x08940E5Cu, f_0887684C);
}

/* ---------------------------------------------------------------- leaves */
/* (fighter): 0 for the CPU (+373); else 1 when a kick (bits 7 - 9 of +760) was just pressed. */
SFA_FUNC(089746E4, human_kick_pressed) {
    gaddr f = ARG(0);
    if (S8(f + 373) != 0) { RETURN(0); return; }
    RETURN((*H(f + 760) & 0x380) != 0);
}
/* (object, record): the record's +44 = the object and +48 = 0, the other way round when +77 is set. */
SFA_FUNC(08981D94, link_44_or_48) {
    gaddr o = ARG(0), r = ARG(1);
    *W(r + 44) = o; *W(r + 48) = 0;
    if (S8(o + 77) != 0) { *W(r + 44) = 0; *W(r + 48) = o; }
}
/* (object): y moved by its speed, which changes by +108; 1 while the new speed is negative. */
SFA_FUNC(089930D8, move_y_rising) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 100), accel = *W(o + 108);
    *W(o + 20) += speed;
    uint32_t next = speed + accel;
    *W(o + 100) = next;
    RETURN((int32_t)next < 0);
}
/* (object): +0x358, +0x35C, +0x368, +0x36C cleared. */
SFA_FUNC(0899B624, clear_358_36C) {
    gaddr o = ARG(0);
    *W(o + 0x358) = 0; *W(o + 0x35C) = 0; *W(o + 0x368) = 0; *W(o + 0x36C) = 0;
}
/* (object): step 3 (+4 = 3, +5 - +7 cleared), +0 = 2, its owner's (+88) +228 cleared. */
SFA_FUNC(0893A6D0, step_3_unlink) {
    gaddr o = ARG(0);
    *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
    gaddr w = *W(o + 88);
    *B(o) = 2;
    *W(w + 228) = 0;
}
/* (object): +4 on, +1 cleared; its owner's (+88) +228 cleared and +536 counted down. */
SFA_FUNC(089C5AC4, step_on_4_owner_536_down) {
    gaddr o = ARG(0), w = *W(o + 88);
    inc8(o + 4);
    *B(o + 1) = 0;
    uint8_t n = *B(w + 536);
    *W(w + 228) = 0;
    *B(w + 536) = (uint8_t)(n - 1);
}
/* (object): speed +96 and its change +104 turned round, unless facing (+11) is set. */
SFA_FUNC(088D3180, turn_x_speed) {
    gaddr o = ARG(0);
    if (*B(o + 11) != 0) return;
    uint32_t v = *W(o + 96), a = *W(o + 104);
    *W(o + 96) = 0u - v;
    *W(o + 104) = 0u - a;
}
/* (object): +0x150 moved by the s16 at 0x08A64638 by its +346. */
SFA_FUNC(0891D41C, add_150_by_346) {
    gaddr o = ARG(0), p = o + 0x150;
    int16_t d = S16(0x08A64638u + (uint32_t)(int32_t)S8(o + 346) * 2);
    *H(p) = (uint16_t)(S16(p) + d);
}
/* (object): y moved by its speed; while the speed is negative, its change +108 = pull first. */
static uint32_t fall_with_pull(gaddr o, uint32_t pull) {
    uint32_t speed = *W(o + 100);
    if ((int32_t)speed < 0) *W(o + 108) = pull;
    *W(o + 20) += speed;
    uint32_t next = speed + *W(o + 108);
    *W(o + 100) = next;
    return next;
}
SFA_FUNC(089291C0, fall_pull_6000) { fall_with_pull(ARG(0), 0xFFFFA000u); }
SFA_FUNC(08931038, fall_pull_5000) { fall_with_pull(ARG(0), 0xFFFFB000u); }
SFA_FUNC(089320AC, fall_pull_5000_answer) { RETURN(fall_with_pull(ARG(0), 0xFFFFB000u)); }
/* (object, from): step 1 (+4), +114 = -1, +9 = 2, +12 / +13 = from's. */
SFA_FUNC(089A8B48, start_from) {
    gaddr o = ARG(0), s = ARG(1);
    *B(o + 4) = 1;
    *H(o + 114) = 0xFFFF;
    *B(o + 9) = 2;
    *B(o + 13) = *B(s + 13);
    *B(o + 12) = (uint8_t)S8(s + 12);
}
/* (object): x (+18) moved by +76, away from facing (+11 clear: to the left). */
SFA_FUNC(088F501C, nudge_x_by_76) {
    gaddr o = ARG(0);
    int32_t d = S8(o + 76);
    if (*B(o + 11) == 0) d = (int16_t)(-d);
    *H(o + 18) = (uint16_t)(S16(o + 18) + d);
}
/* (object, fighter): the fighter's +311 = 2, +312 = 8, +260 from 0x08A64600 by the object's +0x338 /
   from 0x08A64608 by its +194. */
SFA_FUNC(0891CDCC, set_260_by_338) {
    gaddr o = ARG(0), f = ARG(1);
    *B(f + 311) = 2; *B(f + 312) = 8;
    *B(f + 260) = (uint8_t)S8(0x08A64600u + *B(o + 0x338));
}
SFA_FUNC(0891CEF0, set_260_by_194) {
    gaddr o = ARG(0), f = ARG(1);
    *B(f + 311) = 2; *B(f + 312) = 8;
    *B(f + 260) = (uint8_t)S16(0x08A64608u + (uint32_t)(int32_t)S8(o + 194) * 2);
}
/* (object): x (+16) moved by its speed (+96) the way it faces, the speed changed by +104; the new speed back. */
SFA_FUNC(0893207C, move_x_facing) {
    gaddr o = ARG(0);
    uint32_t speed = *W(o + 96);
    uint32_t d = *B(o + 11) == 0 ? 0u - speed : speed;
    *W(o + 16) += d;
    uint32_t next = speed + *W(o + 104);
    *W(o + 96) = next;
    RETURN(next);
}
/* (fighter): 1 when bit 2 of the held keys (+752) is set and the L button (+760 bit 8) was just pressed. */
SFA_FUNC(089351BC, held_4_pressed_100) {
    gaddr f = ARG(0);
    RETURN((*H(f + 752) & 4) != 0 && (*H(f + 760) & 0x100) != 0);
}
/* (object): +713 = 1, speeds (-2.0, 10.0) with changes (0, -0.5), x / y (+16 / +20 halves) cleared. */
SFA_FUNC(0896F678, launch_m2_10) {
    gaddr o = ARG(0);
    *B(o + 713) = 1;
    *W(o + 96) = 0xFFFE0000u; *W(o + 104) = 0;
    *W(o + 100) = 0xA0000u; *W(o + 108) = 0xFFFF8000u;
    *H(o + 20) = 0; *H(o + 16) = 0;
}
/* (object): +5 on, speeds (0, 5.0) with changes (0, -0.28125), +0 = 2. */
SFA_FUNC(08970344, step_on_5_rise_5) {
    gaddr o = ARG(0);
    *W(o + 104) = 0;
    inc8(o + 5);
    *W(o + 96) = 0;
    *W(o + 100) = 0x50000u; *W(o + 108) = 0xFFFFB800u;
    *B(o) = 2;
}
/* (object): +5 on, +0 = 2, its owner's (+88) +228 cleared, speed +100 = 7.0, +108 = -1.0. */
SFA_FUNC(08980B2C, step_on_5_rise_7) {
    gaddr o = ARG(0), w = *W(o + 88);
    inc8(o + 5);
    *B(o) = 2;
    *W(w + 228) = 0;
    *W(o + 100) = 0x70000u; *W(o + 108) = 0xFFFF0000u;
}
