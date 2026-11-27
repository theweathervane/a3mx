/* Small leaves, most of them under character move steps no test scenario plays: they are written
   from the original code alone, and the ones the tests reach are checked by parity as usual. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886D678(Cpu *c); void f_088DC054(Cpu *c); void f_08868100(Cpu *c); void f_088616EC(Cpu *c);
void f_0886163C(Cpu *c); void f_089E3A68(Cpu *c); void f_089E3A70(Cpu *c); void f_089BB148(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08861848(Cpu *c); void f_0887E7AC(Cpu *c);
void hle_sceNetAdhocctlGetNameByAddr(Cpu *c);

/* The frame these keep: 0x20 bytes with only the return address, at +16. */
static uint32_t enter(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
/* A call with the caller's arguments passed on, behind that frame. */
static void pass_on(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = enter(c);
    c->r[31] = back; fn(c);
    leave(c, ra);
}
/* The object's step +7 set and a cue, behind that frame. */
static void step_cue(Cpu *c, uint32_t step, uint32_t cue, void (*fn)(Cpu *), uint32_t back) {
    gaddr o = ARG(0);
    *B(o + 7) = (uint8_t)step;
    uint32_t ra = enter(c);
    ARG(1) = cue; c->r[31] = back; fn(c);
    leave(c, ra);
}

/* (object): +36 and +1 cleared. */
SFA_FUNC(088A0588, clear_36_1) { gaddr o = ARG(0); *H(o + 36) = 0; *B(o + 1) = 0; }

/* (object): stops across: the speed +96 and its change +104 cleared. */
SFA_FUNC(088D36E0, stop_across) { gaddr o = ARG(0); *W(o + 96) = 0; *W(o + 104) = 0; }

/* (object): stops up and down: +100 and +108 cleared. */
SFA_FUNC(088D36EC, stop_upright) { gaddr o = ARG(0); *W(o + 100) = 0; *W(o + 108) = 0; }

/* (object): state 3. */
SFA_FUNC(0892F0DC, state_3) { c->r[5] = 3; *B(ARG(0) + 4) = 3; }

/* (object): the next state (+4 on); five copies of it. */
static void state_next(Cpu *c) {
    gaddr o = ARG(0);
    c->r[5] = (uint32_t)*B(o + 4) + 1;
    *B(o + 4) = (uint8_t)c->r[5];
}
SFA_FUNC(08900590, state_next_00590) { state_next(c); }
SFA_FUNC(0890D3B8, state_next_0d3b8) { state_next(c); }
SFA_FUNC(08917AC4, state_next_17ac4) { state_next(c); }
SFA_FUNC(08928BBC, state_next_28bbc) { state_next(c); }
SFA_FUNC(08947D44, state_next_47d44) { state_next(c); }

/* (...): the link's name for an address, the system's call behind a frame. */
SFA_FUNC(088DE4A0, net_name_by_addr) { pass_on(c, hle_sceNetAdhocctlGetNameByAddr, 0x088DE4B0u); }

/* (...): 0x0886D678 behind a frame; three copies. */
SFA_FUNC(08914BD8, call_0886D678_14bd8) { pass_on(c, f_0886D678, 0x08914BE8u); }
SFA_FUNC(0892582C, call_0886D678_2582c) { pass_on(c, f_0886D678, 0x0892583Cu); }
SFA_FUNC(08925AFC, call_0886D678_25afc) { pass_on(c, f_0886D678, 0x08925B0Cu); }

/* (...): 0x088DC054's result, its low half. */
SFA_FUNC(088E426C, net_dc054_low) {
    uint32_t ra = enter(c);
    c->r[31] = 0x088E427Cu; f_088DC054(c);
    c->r[2] &= 0xFFFF;
    leave(c, ra);
}

/* (a, count): the count's byte up by one, then 0x08868100 with the same arguments. */
SFA_FUNC(088699D8, count_then_08868100) {
    gaddr p = ARG(1);
    uint32_t n = *B(p);
    uint32_t ra = enter(c);
    c->r[6] = n + 1;
    *B(p) = (uint8_t)(n + 1);
    c->r[31] = 0x088699F0u; f_08868100(c);
    leave(c, ra);
}

/* (fighter): step 6 with the animation 0x18. */
SFA_FUNC(089079C8, step_6_anim_18) { step_cue(c, 6, 0x18, f_088616EC, 0x089079E0u); }

/* (fighter): step 6 with the cue 0x32. */
SFA_FUNC(089253E8, step_6_cue_32) { step_cue(c, 6, 0x32, f_0886163C, 0x08925400u); }

/* (base, n): the n-th byte (from 1) of the list at base + 0x17A becomes 4 when it is 0. */
SFA_FUNC(08825BE0, list_17a_mark) {
    gaddr p = ARG(0) + 0x17A + ((ARG(1) & 0xFF) - 1);
    if (S8(p) == 0) *B(p) = 4;
}

/* (out): the link's record at 0x095236D4: its +8; and, with somewhere to put it and a first word set,
   that word + 12 into out. */
SFA_FUNC(088E0984, net_record_get) {
    gaddr out = ARG(0);
    if (out != 0) {
        uint32_t first = *W(0x095236D4u);
        if (first != 0) *W(out) = first + 0xC;
    }
    RETURN(*W(0x095236DCu));
}

/* (x, fighter): for character 27 in use, +0x33C cleared. */
SFA_FUNC(08981640, clear_33c_for_27) {
    gaddr o = ARG(1);
    if (*B(o) != 0 && S8(o + 338) == 0x1B) *B(o + 0x334 + 8) = 0;
}

/* (): registers the two callbacks 0x0881686C and 0x088168C8 with the library (0x089E3A68, 0x089E3A70). */
SFA_FUNC(08816840, callbacks_register) {
    uint32_t ra = enter(c);
    ARG(0) = 0x0881686Cu; c->r[31] = 0x08816854u; f_089E3A68(c);
    ARG(0) = 0x088168C8u; c->r[31] = 0x08816860u; f_089E3A70(c);
    leave(c, ra);
}

/* (object, words): five words copied to the object's +0x9C. */
SFA_FUNC(08838D48, copy_five_to_9c) {
    gaddr to = ARG(0) + 0x9C, from = ARG(1);
    for (uint32_t i = 0; i < 5; i++) *W(to + 4 * i) = *W(from + 4 * i);
}

/* (object): 0x089BB148 with the second layer's place (0x092E88B0) plus 0x04000000. */
SFA_FUNC(089BB0F0, call_089BB148_layer) {
    uint32_t at = *W(0x092E88B0u);
    uint32_t ra = enter(c);
    c->r[6] = 0x04000000u;
    ARG(1) = at + 0x04000000u; c->r[31] = 0x089BB110u; f_089BB148(c);
    leave(c, ra);
}

/* (): numbers the link's list from 1 (each entry's +228; the list from 0x09523908, +0 the next), with
   the head's own +228 cleared; returns 0x0952390C. */
SFA_FUNC(088DD040, net_list_number) {
    gaddr e = *W(0x09523820u + 232);
    *W(0x09523820u + 228) = 0;
    for (uint32_t n = 1; e != 0; n++) { *W(e + 228) = n; e = *W(e); }
    RETURN(*W(0x09523820u + 236));
}

/* Step dispatchers: the handler for a step number from a table, with the caller's arguments. */
SFA_FUNC(088415B8, step_by_4_a12b00)   { run_step(c, *B(ARG(0) + 4), 0x08A12B00u, 5, 0x088415E0u); }
SFA_FUNC(08842690, step_by_128_a12cb0) { run_step(c, *B(ARG(0) + 128), 0x08A12CB0u, 5, 0x088426B8u); }
SFA_FUNC(088F61C8, step_by_7_a495b8)   { run_step(c, *B(ARG(0) + 7), 0x08A495B8u, 6, 0x088F61F0u); }
SFA_FUNC(088F65D0, step_by_6_a495e8)   { run_step(c, *B(ARG(0) + 6), 0x08A495E8u, 6, 0x088F65F8u); }
SFA_FUNC(088F6AC0, step_by_7_a49614)   { run_step(c, *B(ARG(0) + 7), 0x08A49614u, 5, 0x088F6AE8u); }
SFA_FUNC(088F6C4C, step_by_7_a49628)   { run_step(c, *B(ARG(0) + 7), 0x08A49628u, 5, 0x088F6C74u); }
SFA_FUNC(089013B0, step_by_7_a51458)   { run_step(c, *B(ARG(0) + 7), 0x08A51458u, 5, 0x089013D8u); }
SFA_FUNC(089110B0, step_by_194_a5ade8) { run_step(c, (uint32_t)(int32_t)S8(ARG(0) + 194), 0x08A5ADE8u, 5, 0x089110D8u); }
SFA_FUNC(08943028, step_by_199_a764b8) { run_step(c, (uint32_t)(int32_t)S8(ARG(0) + 199), 0x08A764B8u, 5, 0x08943050u); }
SFA_FUNC(0899DEDC, step_by_7_a9be64)   { run_step(c, *B(ARG(0) + 7), 0x08A9BE64u, 5, 0x0899DF04u); }
SFA_FUNC(089AFF78, step_by_7_aa3b9c)   { run_step(c, *B(ARG(0) + 7), 0x08AA3B9Cu, 5, 0x089AFFA0u); }

/* A step, +235 set, and the animation `base` plus the move's strength (+194). */
static void step_anim_by_strength(Cpu *c, uint32_t step, uint32_t base, uint32_t back) {
    gaddr o = ARG(0);
    *B(o + 7) = (uint8_t)step;
    int32_t strength = S8(o + 194);
    c->r[6] = 1;
    *B(o + 235) = 1;
    uint32_t ra = enter(c);
    ARG(1) = (uint32_t)(strength + (int32_t)base) & 0xFFFF; c->r[31] = back; f_088616EC(c);
    leave(c, ra);
}

/* (fighter): step 17 with the animation 0x3F + strength. */
SFA_FUNC(088FA8F0, step_17_anim_3f) { step_anim_by_strength(c, 0x11, 0x3F, 0x088FA918u); }

/* (fighter): step 9 with the animation 0x37 + strength. */
SFA_FUNC(088FAAE8, step_9_anim_37) { step_anim_by_strength(c, 9, 0x37, 0x088FAB10u); }

/* (fighter, move): the move into +236 at step 0, +235 set, and +82 down by 30, not below 0. */
SFA_FUNC(08974578, move_start_less_30) {
    gaddr o = ARG(0);
    int32_t v = S16(o + 82);
    *B(o + 7) = 0;
    *B(o + 236) = (uint8_t)ARG(1);
    *B(o + 235) = 1;
    v -= 30;
    *H(o + 82) = (uint16_t)v;
    if ((int16_t)v < 0) *H(o + 82) = 0;
}

/* (fighter, n): the meter by n (0x0886CA2C), unless +540. */
SFA_FUNC(0891D014, meter_unless_540) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    int32_t off = S8(o + 540);
    uint32_t ra = enter(c);
    if (off == 0) { c->r[6] = o; ARG(0) = o; ARG(1) = n; c->r[31] = 0x0891D040u; f_0886CA2C(c); }
    leave(c, ra);
}

/* (object, plan): for a cpu fighter at the object's +600, its plan: +480 / +481 set, the plan's high
   byte in +482 and low in +483. Two copies. */
static void cpu_plan_set(Cpu *c) {
    gaddr f = *W(ARG(0) + 600);
    uint32_t plan = ARG(1) & 0xFFFF;
    if (f == 0 || S8(f + 373) == 0) return;
    *B(f + 480) = 1; *B(f + 481) = 1; *B(f + 482) = (uint8_t)(plan >> 8); *B(f + 483) = (uint8_t)plan;
}
SFA_FUNC(0899302C, cpu_plan_set_9302c) { cpu_plan_set(c); }
SFA_FUNC(08999338, cpu_plan_set_99338) { cpu_plan_set(c); }

/* (kind, mark): the kind's byte; with the mark 255, kind 4 becomes 0x12 and kind 6 0x14. */
SFA_FUNC(0882EB74, kind_with_mark) {
    uint32_t kind = ARG(0) & 0xFF, v = kind;
    if ((ARG(1) & 0xFF) == 0xFF) {
        if (kind == 4) v = 0x12;
        if (kind == 6) v = 0x14;
    }
    RETURN(v & 0xFF);
}

/* (object): one frame of movement: the place (+16, +20) by the speeds (+96, +100), the speeds by
   their changes (+104, +108). */
SFA_FUNC(089049E4, move_one_frame) {
    gaddr o = ARG(0);
    uint32_t x = *W(o + 16), vx = *W(o + 96), ax = *W(o + 104), y = *W(o + 20);
    *W(o + 16) = x + vx;
    uint32_t vy = *W(o + 100);
    *W(o + 96) = vx + ax;
    uint32_t ay = *W(o + 108);
    *W(o + 20) = y + vy;
    c->r[2] = vy + ay;
    *W(o + 100) = vy + ay;
}

/* (fighter): 20 across, towards the middle by its side (+337), and the cue 0x40. */
SFA_FUNC(08924E58, shift_20_cue_40) {
    gaddr o = ARG(0);
    int32_t side = S8(o + 337);
    uint32_t ra = enter(c);
    int32_t x = S16(o + 18);
    *H(o + 18) = (uint16_t)(side != 0 ? x - 20 : x + 20);
    ARG(1) = 0x40; c->r[31] = 0x08924E88u; f_0886163C(c);
    leave(c, ra);
}

/* (fighter): step 6, moving across at 2.0 and falling by 0x6000 a frame, with the animation 0x34. */
SFA_FUNC(0895D820, step_6_launch_anim_34) {
    gaddr o = ARG(0);
    *B(o + 7) = 6;
    *W(o + 96) = 0x20000; *W(o + 104) = 0; *W(o + 100) = 0; *W(o + 108) = 0xFFFFA000u;
    uint32_t ra = enter(c);
    ARG(1) = 0x34; c->r[31] = 0x0895D850u; f_088616EC(c);
    leave(c, ra);
}

/* (battle): which side its current fighter (+112) is on, by line-up (+56): the first two fighters 0
   (1 in line-up 3), the others 3 (2 in line-up 3). */
SFA_FUNC(088261A0, side_by_lineup) {
    gaddr b = ARG(0);
    int first = S8(*W(b + 112) + 337) < 2;
    if (S16(b + 56) == 3) RETURN(first ? 1 : 2);
    else RETURN(first ? 0 : 3);
}

/* (fighter): ticked while the store +816 holds anything; then step 2 with +584 = 10. */
SFA_FUNC(0885D5D4, wait_store_empty) {
    gaddr o = ARG(0);
    uint32_t store = *W(o + 816);
    uint32_t ra = enter(c);
    if (store != 0) { c->r[31] = 0x0885D5F0u; f_08861848(c); }
    else { *B(o + 7) = 2; *H(o + 584) = 10; }
    leave(c, ra);
}

/* A request record filled (kind, the two values) and sent with 0x0887E7AC (the caller's first argument
   passed on). */
static void request_send(Cpu *c, gaddr rec, uint32_t kind, uint32_t back) {
    *B(rec) = 0; *B(rec + 1) = 1;
    *H(rec + 4) = (uint16_t)ARG(1);
    *B(rec + 2) = (uint8_t)kind;
    *H(rec + 6) = (uint16_t)ARG(2);
    uint32_t ra = enter(c);
    c->r[8] = rec;
    ARG(1) = rec; c->r[31] = back; f_0887E7AC(c);
    leave(c, ra);
}

/* (x, a, b): the request 0x15 through the record at 0x08B026B4. */
SFA_FUNC(0887E4A0, request_15) { request_send(c, 0x08B026B4u, 0x15, 0x0887E4D4u); }

/* (x, a, b): the request 0x4F through the record at 0x08B026CC. */
SFA_FUNC(0887E5A4, request_4f) { request_send(c, 0x08B026CCu, 0x4F, 0x0887E5D8u); }
