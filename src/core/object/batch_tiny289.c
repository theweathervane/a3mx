/* More small leaves, most of them under character move steps no test scenario plays: written from
   the original code alone, and checked by parity where the tests reach them. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_088EF4B8(Cpu *c); void f_088616EC(Cpu *c); void f_0886CA2C(Cpu *c); void f_088E3ED8(Cpu *c);
void f_088E35B8(Cpu *c); void f_08876998(Cpu *c); void f_08861674(Cpu *c); void f_0886DDCC(Cpu *c);
void f_08821368(Cpu *c); void f_0886A2F4(Cpu *c); void f_0886AE58(Cpu *c); void f_08822EAC(Cpu *c);
void f_0886163C(Cpu *c); void f_0893D900(Cpu *c); void f_0886DC3C(Cpu *c); void f_08863114(Cpu *c);
void f_088D2D14(Cpu *c); void f_0882324C(Cpu *c); void f_088D605C(Cpu *c); void f_08869FEC(Cpu *c);
void f_0886E3CC(Cpu *c); void f_0890EF5C(Cpu *c); void f_08837690(Cpu *c); void f_08887AFC(Cpu *c);
void f_0883771C(Cpu *c); void f_088379B8(Cpu *c); void f_0880B6CC(Cpu *c); void f_0886DEC8(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A 0x20 frame with only the return address, at +16. */
static uint32_t enter(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
/* A 0x20 frame with s0 at +16 and the return address at +20; s0 holds the object. */
static void enter_s0(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave_s0(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* The state bytes +4..+7 of a move's start. */
static void state(gaddr o, uint32_t a, uint32_t b, uint32_t d, uint32_t e) {
    *B(o + 4) = (uint8_t)a; *B(o + 5) = (uint8_t)b; *B(o + 6) = (uint8_t)d; *B(o + 7) = (uint8_t)e;
}
/* The speeds and their changes: across (+96, +104) and upright (+100, +108). */
static void speeds(gaddr o, uint32_t vx, uint32_t ax, uint32_t vy, uint32_t ay) {
    *W(o + 96) = vx; *W(o + 104) = ax; *W(o + 100) = vy; *W(o + 108) = ay;
}

/* (fighter): move 3's start (state 1 / 0 / 7 / 0, +235 set) and 0x088EF4B8. */
SFA_FUNC(088EE9E0, move_3_start_ee9e0) {
    gaddr o = ARG(0);
    state(o, 1, 0, 7, 0);
    *B(o + 236) = 3;
    uint32_t ra = enter(c);
    *B(o + 235) = 1;
    c->r[5] = 3; c->r[6] = 1;
    c->r[31] = 0x088EEA14u; f_088EF4B8(c);
    leave(c, ra);
}

/* (fighter): step 6, falling by 0x5800 a frame from rest, +235 set, with the animation 10. */
SFA_FUNC(08910BE0, step_6_drop_anim_a) {
    gaddr o = ARG(0);
    *B(o + 7) = 6;
    speeds(o, 0, 0, 0, 0xFFFFA800u);
    *B(o + 235) = 1;
    uint32_t ra = enter(c);
    ARG(1) = 0xA; c->r[31] = 0x08910C14u; f_088616EC(c);
    leave(c, ra);
}

/* (fighter): half the speed across, the meter by 1, and the animation 0x39. */
SFA_FUNC(08934D08, halve_speed_anim_39) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *W(o + 96) = (uint32_t)((int32_t)*W(o + 96) >> 1);
    *W(c->r[29] + 20) = sv[1];
    call2(c, o, 1, f_0886CA2C, 0x08934D2Cu);
    call2(c, o, 0x39, f_088616EC, 0x08934D38u);
    leave_s0(c, sv);
}

/* (fighter): step 7, the meter by 4, and the animation 0x1B. */
SFA_FUNC(0895DD5C, step_7_meter_anim_1b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter_s0(c, o, sv);
    *B(o + 7) = 7;
    call2(c, o, 4, f_0886CA2C, 0x0895DD80u);
    call2(c, o, 0x1B, f_088616EC, 0x0895DD8Cu);
    leave_s0(c, sv);
}

/* (fighter): whether the meter (+366) holds a super's cost by the choice (+386: 0x30 for 0, 0x90 for
   -1, else 0x48) with +251 clear. No frame. */
SFA_FUNC(088D3218, meter_holds_cost) {
    gaddr o = ARG(0);
    int32_t choice = S8(o + 386);
    int32_t cost = choice == 0 ? 0x30 : choice == -1 ? 0x90 : 0x48;
    RETURN(!(S16(o + 366) < cost) && S8(o + 251) == 0);
}

/* (fighter): a counter's state: 1 / 0 / 7 / 0, +235 set, +696 / +127 cleared, the opponent's +127 =
   0x15, +573 = 0x19 and +777 set. No frame. */
SFA_FUNC(088D3514, counter_state_15) {
    gaddr o = ARG(0), other = *W(o + 92);
    state(o, 1, 0, 7, 0);
    *B(o + 235) = 1; *B(o + 696) = 0; *B(o + 127) = 0;
    *B(other + 127) = 0x15;
    *B(o + 573) = 0x19; *B(o + 777) = 1;
}

/* (record, value): a record's +8 set once: with it +0 = 100 and +47 cleared, then 0x088E3ED8 and
   0x088E35B8. */
SFA_FUNC(088D5680, net_record_open) {
    gaddr rec = ARG(0); uint32_t v = ARG(1);
    uint32_t have = *W(rec + 8);
    uint32_t ra = enter(c);
    if (have == 0) {
        *W(rec + 8) = v;
        *H(rec) = 100;
        *B(rec + 47) = 0;
        c->r[6] = 0x64;
        call2(c, v, rec, f_088E3ED8, 0x088D56B0u);
        c->r[31] = 0x088D56B8u; f_088E35B8(c);
    }
    leave(c, ra);
}

/* (fighter): the cpu's script (+128) from the list of its kind (+346, the table 0x08A6A630) by its
   vitality (+112), then 0x08876998. */
SFA_FUNC(0892926C, cpu_script_by_kind) {
    gaddr o = ARG(0);
    gaddr list = *W(0x08A6A630u + (uint32_t)(S8(o + 346) << 2));
    uint32_t script = *W(list + (uint32_t)(S16(o + 112) << 2));
    uint32_t ra = enter(c);
    *W(o + 128) = script;
    c->r[31] = 0x089292A4u; f_08876998(c);
    leave(c, ra);
}

/* (fighter): step 5, back at 0.5 and up at 8.0 with gravity 0.5, and the animation 9 + strength. */
SFA_FUNC(0895D85C, step_5_hop_back) {
    gaddr o = ARG(0);
    *B(o + 7) = 5;
    *W(o + 96) = 0xFFFF8000u; *W(o + 104) = 0;
    int32_t strength = S8(o + 194);
    *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFF8000u;
    uint32_t ra = enter(c);
    ARG(1) = (uint32_t)(strength + 9) & 0xFFFF; c->r[31] = 0x0895D894u; f_088616EC(c);
    leave(c, ra);
}

/* (fighter): the animation 15 + strength (0x08861674); the heavy one starts step 2 at 9.0 across,
   slowing by 0.5. */
SFA_FUNC(0890F88C, anim_f_by_strength) {
    gaddr o = ARG(0);
    uint32_t anim = (uint32_t)(S8(o + 194) + 0xF) & 0xFFFF;
    uint32_t ra = enter(c);
    if (anim == 0x11) { *B(o + 7) = 2; *W(o + 96) = 0x90000; *W(o + 104) = 0xFFFF8000u; }
    ARG(1) = anim; c->r[31] = 0x0890F8C8u; f_08861674(c);
    leave(c, ra);
}

/* (fighter): the step's handler (+7 into the table 0x08A6BF34), then 0x0886DDCC. */
SFA_FUNC(0892E6BC, step_then_0886DDCC) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    gaddr handler = *W(0x08A6BF34u + (uint32_t)*B(o + 7) * 4);
    *W(c->r[29] + 20) = sv[1];
    c->r[5] = handler; ARG(0) = o; c->r[31] = 0x0892E6ECu;
    rt_call_indirect(c, handler);
    call1(c, o, f_0886DDCC, 0x0892E6F4u);
    leave_s0(c, sv);
}

/* (fighter): 1 for a choice below 0 with +193 set, the middle strength, and down held (+290 bit 2). */
SFA_FUNC(08934CC0, low_middle_check) {
    gaddr o = ARG(0);
    RETURN(S8(o + 386) < 0 && S8(o + 193) != 0 && S8(o + 194) == 1 && (*H(o + 290) & 4) != 0);
}

/* (fighter): the next step, the meter by 4, and the animation 0x1C + +78. */
SFA_FUNC(089385B8, next_step_meter_anim_1c) {
    gaddr o = ARG(0); uint32_t sv[2]; enter_s0(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, 4, f_0886CA2C, 0x089385DCu);
    call2(c, o, (uint32_t)(S8(o + 78) + 0x1C) & 0xFFFF, f_088616EC, 0x089385F0u);
    leave_s0(c, sv);
}

/* (a, b): compares two names, eight characters at most: 0 when they match up to the end of either, -1
   at the first difference. No frame. */
SFA_FUNC(088DE388, net_names_match) {
    gaddr a = ARG(0), b = ARG(1);
    int32_t left = 7;
    int8_t x = S8(a), y = S8(b);
    for (;;) {
        a++;
        if (x != y) { RETURN(0xFFFFFFFFu); return; }
        x = S8(a); b++;
        if (x == 0) break;
        y = S8(b); left--;
        if (y == 0 || left < 0) break;
    }
    RETURN(0);
}

/* The state's handler (+4 into a table); the object's +1 is cleared afterwards while 0x09525881. */
static void state_step_hide(Cpu *c, gaddr table, uint32_t back) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    gaddr handler = *W(table + (uint32_t)*B(o + 4) * 4);
    *W(c->r[29] + 20) = sv[1];
    c->r[5] = handler; ARG(0) = o; c->r[31] = back;
    rt_call_indirect(c, handler);
    if (S8(0x09525881u) != 0) *B(o + 1) = 0;
    leave_s0(c, sv);
}
SFA_FUNC(088D04EC, state_step_hide_3cb00) { state_step_hide(c, 0x08A3CB00u, 0x088D051Cu); }
SFA_FUNC(088D07AC, state_step_hide_3cb20) { state_step_hide(c, 0x08A3CB20u, 0x088D07DCu); }

/* (object): shown (+1) only while its player's record (0x09524AF0, 0x354 each, by +78) has +58, then
   placed 246 / 65 from its owner (+88). No frame. */
SFA_FUNC(08847F90, show_by_record_at_owner) {
    gaddr o = ARG(0);
    gaddr rec = 0x09524AF0u + (uint32_t)(S8(o + 78) * 0x354);
    gaddr own = *W(o + 88);
    *B(o + 1) = 0;
    if (S8(rec + 58) == 0) return;
    *B(o + 1) = 1;
    *H(o + 18) = (uint16_t)(S16(own + 18) + 0xF6);
    *H(o + 22) = (uint16_t)(S16(own + 22) + 0x41);
}

/* (object): the same with the records at 0x09519588 and the place from the table 0x08A133F0 by +78,
   4 right and 4 up of it. No frame. */
SFA_FUNC(08847F2C, show_by_record_at_table) {
    gaddr o = ARG(0);
    int32_t k = S8(o + 78);
    gaddr rec = 0x09519588u + (uint32_t)(k * 0x354);
    *B(o + 1) = 0;
    if (S8(rec + 58) == 0) return;
    *B(o + 1) = 1;
    gaddr e = 0x08A133F0u + (uint32_t)(k << 2);
    *H(o + 18) = (uint16_t)(*H(e) + 4);
    *H(o + 22) = (uint16_t)(*H(e + 2) - 4);
}

/* (object): +0x82 (the object's +0x60 block's +34) from the table 0x08A15490 by a random 0..7, plus
   the random number's low six bits. A 0x10 frame keeping s0. */
SFA_FUNC(0884FBB0, random_82) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    c->r[31] = 0x0884FBC4u; f_08821368(c);
    uint32_t r = c->r[2];
    *H(o + 0x60 + 34) = (uint16_t)S16(0x08A15490u + (r & 7) * 2);
    *H(o + 0x60 + 34) = (uint16_t)(S16(o + 0x60 + 34) + (int32_t)(r & 0x3F));
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* (fighter, kind): 0x0886A2F4 (fighter, 0); then, unless the opponent crouches (+696), the throw
   check 0x0886AE58 (fighter, opponent, kind), else 0. A frame keeping s0, s1. */
SFA_FUNC(0886B058, throw_check_standing) {
    gaddr o = ARG(0); uint32_t kind = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = kind; c->r[17] = o;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 0, f_0886A2F4, 0x0886B078u);
    gaddr other = *W(o + 92);
    c->r[2] = 0;
    if (S8(other + 696) == 0) { c->r[6] = kind; call2(c, o, other, f_0886AE58, 0x0886B098u); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 20 with +656 = 0x15, a new object of kind 0x18 / 3 owned by the fighter, and the
   cue 0x41. */
SFA_FUNC(08925138, step_20_spawn_18) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = 0x14; *B(o + 656) = 0x15;
    *W(c->r[29] + 20) = sv[1];
    call2(c, 0x18, 3, f_08822EAC, 0x08925164u);
    if (c->r[2] != 0) *W(c->r[2] + 88) = o;
    call2(c, o, 0x41, f_0886163C, 0x0892517Cu);
    leave_s0(c, sv);
}

/* (fighter): move 0's start facing the opponent (state 1 / 0 / 7 / 0), +78 the other way, +0x3BC =
   2, and 0x0893D900. */
SFA_FUNC(0893D8AC, move_0_start_3d8ac) {
    gaddr o = ARG(0);
    int8_t facing = S8(o + 697);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 6) = 7;
    uint8_t now = *B(o + 11);
    *B(o + 7) = 0; *B(o + 236) = 0;
    *B(o + 78) = (uint8_t)(now ^ 1);
    uint32_t ra = enter(c);
    c->r[5] = 2; c->r[6] = o + 0x3B8;
    *B(o + 0x3B8 + 4) = 2;
    c->r[31] = 0x0893D8F4u; f_0893D900(c);
    leave(c, ra);
}

/* (fighter): a new object of kind 0x27 / 0 owned by the fighter; +0x390 stays 1 when there was no
   room for it. A frame keeping s0, s1. */
SFA_FUNC(0896F514, spawn_27_flag_390) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = o + 0x390;
    *B(o + 0x390) = 1;
    *W(c->r[29] + 24) = ra;
    call2(c, 0x27, 0, f_08822EAC, 0x0896F540u);
    if (c->r[2] != 0) { *W(c->r[2] + 88) = o; *B(o + 0x390) = 0; }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x0886DC3C, 31 further on by its facing, 0x08863114 and the animation 0x23. */
SFA_FUNC(0890FA58, shift_31_anim_23) {
    gaddr o = ARG(0); uint32_t sv[2]; enter_s0(c, o, sv);
    c->r[31] = 0x0890FA6Cu; f_0886DC3C(c);
    uint32_t d = *B(o + 11) != 0 ? 0x1F : 0xFFE0;
    uint32_t x = (uint32_t)S16(o + 18) + d;
    *H(o + 18) = (uint16_t)x;
    call2(c, o, x, f_08863114, 0x0890FA94u);
    call2(c, o, 0x23, f_08861674, 0x0890FAA0u);
    leave_s0(c, sv);
}

/* A projectile's step with a pull across (+104), turned by its facing (+11), an animation and
   0x088D2D14. */
static void step_pull_anim(Cpu *c, uint32_t step, uint32_t pull, uint32_t anim, uint32_t back) {
    gaddr o = ARG(0); uint32_t sv[2];
    *B(o + 7) = (uint8_t)step;
    uint8_t facing = *B(o + 11);
    *W(o + 104) = pull;
    enter_s0(c, o, sv);
    if (facing == 0) *W(o + 104) = 0 - *W(o + 104);
    call2(c, o, anim, f_088616EC, back);
    call1(c, o, f_088D2D14, back + 8);
    leave_s0(c, sv);
}
SFA_FUNC(089BAA50, step_2_pull_back_anim_c) { step_pull_anim(c, 2, 0xFFFF8000u, 0xC, 0x089BAA90u); }
SFA_FUNC(089BD2B4, step_2_pull_anim_3c)     { step_pull_anim(c, 2, 0x8000, 0x3C, 0x089BD2F4u); }
SFA_FUNC(089BD364, step_4_pull_anim_40)     { step_pull_anim(c, 4, 0x8000, 0x40, 0x089BD3A4u); }

/* (id): the link's entry numbered id (the head 0x09523820 itself, or down its list at +232): its
   +0xE0; the list's last entry stands in for one not found, and an empty list gives 0. No frame. */
SFA_FUNC(088DD380, net_entry_by_number) {
    uint32_t id = ARG(0);
    gaddr e = 0x09523820u;
    if (*W(e + 228) != id) {
        e = *W(e + 232);
        while (e != 0 && *W(e + 228) != id) {
            gaddr next = *W(e);
            if (next == 0) break;
            e = next;
        }
    }
    RETURN(e != 0 ? e + 0xE0 : 0);
}

/* (fighter): the meter by the strength, and the animation 0x18 + strength (0x1B + with +193). */
SFA_FUNC(08936414, meter_anim_by_strength) {
    gaddr o = ARG(0); uint32_t sv[2]; enter_s0(c, o, sv);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0893642Cu);
    int32_t strength = S8(o + 194);
    if (S8(o + 193) == 0) call2(c, o, (uint32_t)(strength + 0x18) & 0xFFFF, f_08861674, 0x08936448u);
    else call2(c, o, (uint32_t)(strength + 0x1B) & 0xFFFF, f_08861674, 0x08936460u);
    leave_s0(c, sv);
}

/* (object, owner): the object taken out of its owner's four places (+40 to +52, the first that holds
   it), then 0x0882324C with the same arguments. */
SFA_FUNC(0894CFD8, owner_slot_clear) {
    uint32_t o = ARG(0); gaddr own = ARG(1);
    uint32_t first = *W(own + 40);
    uint32_t ra = enter(c);
    if (first == o) *W(own + 40) = 0;
    else if (*W(own + 44) == o) *W(own + 44) = 0;
    else if (*W(own + 48) == o) *W(own + 48) = 0;
    else if (*W(own + 52) == o) *W(own + 52) = 0;
    c->r[31] = 0x0894D028u; f_0882324C(c);
    leave(c, ra);
}

/* (to, from, n): to's +1 = n, and n + 1 entries of four byte columns copied (from +3, +6, +9, +12 to
   +2, +5, +8, +11), then 0x088D605C with the same arguments. */
SFA_FUNC(088D61F8, columns_copy) {
    gaddr to = ARG(0), from = ARG(1); uint32_t n = ARG(2);
    *B(to + 1) = (uint8_t)n;
    uint32_t ra = enter(c);
    uint32_t i = 0;
    do {
        *B(to + i + 2) = (uint8_t)S8(from + i + 3);
        *B(to + i + 5) = (uint8_t)S8(from + i + 6);
        *B(to + i + 8) = (uint8_t)S8(from + i + 9);
        *B(to + i + 11) = (uint8_t)S8(from + i + 12);
        i++;
    } while (!(n < i));
    c->r[7] = i;
    c->r[31] = 0x088D624Cu; f_088D605C(c);
    leave(c, ra);
}

/* (fighter): +193 and +194 cleared; unless all three punches are down (+760 | +762), +193 goes up
   when 0x08869FEC fails. */
SFA_FUNC(088F78F0, strength_reset_check) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t b = (uint32_t)(*H(o + 760) | *H(o + 762));
    *B(o + 193) = 0; *B(o + 194) = 0;
    enter_s0(c, o, sv);
    if ((b & 0x70) != 0x70) {
        call1(c, o, f_08869FEC, 0x088F792Cu);
        if (c->r[2] == 0) *B(o + 193) = (uint8_t)(S8(o + 193) + 1);
    }
    leave_s0(c, sv);
}

/* (fighter): a leap: state 1 / 0 / 8 / 1 with move 1, in the air, 12.0 across slowing by 0.5 and 8.0
   up with gravity 0x6000, and the animation 0x1A. */
SFA_FUNC(089079EC, leap_start_anim_1a) {
    gaddr o = ARG(0);
    state(o, 1, 0, 8, 1);
    *B(o + 236) = 1; *B(o + 73) = 1;
    speeds(o, 0xC0000, 0xFFFF8000u, 0x80000, 0xFFFFA000u);
    uint32_t ra = enter(c);
    ARG(1) = 0x1A; c->r[31] = 0x08907A40u; f_088616EC(c);
    leave(c, ra);
}

/* (fighter): the cpu's script (+128) by its vitality from the list of its kind (+346: 3, 4, others),
   then 0x08876998. */
SFA_FUNC(0892EF20, cpu_script_by_kind_3) {
    gaddr o = ARG(0);
    int32_t kind = S8(o + 346);
    gaddr list = kind == 3 ? 0x08A6AD68u : kind == 4 ? 0x08A6AF44u : 0x08A6B198u;
    int32_t vital = S16(o + 112);
    uint32_t ra = enter(c);
    c->r[6] = o;
    *W(o + 128) = *W(list + (uint32_t)(vital << 2));
    ARG(0) = o; c->r[31] = 0x0892EF74u; f_08876998(c);
    leave(c, ra);
}

/* (fighter): move 10's start facing the opponent: state 1 / 0 / 7 / 0, +235 and +196..+199 cleared,
   +127 cleared and the opponent's = 0x17, +573 = 0x1B, +777 set. No frame. */
SFA_FUNC(08935708, move_10_start) {
    gaddr o = ARG(0);
    *B(o + 236) = 0xA;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7;
    int8_t facing = S8(o + 697);
    *B(o + 7) = 0; *B(o + 235) = 0; *B(o + 11) = (uint8_t)facing;
    *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
    gaddr other = *W(o + 92);
    *B(o + 127) = 0;
    *B(other + 127) = 0x17;
    *B(o + 573) = 0x1B; *B(o + 777) = 1;
}

/* (fighter): move 4's start facing the opponent (state 1 / 0 / 7 / 0, +235 cleared), and 0x0886E3CC
   (the opponent, a byte of 0x08A93D89 by strength, 4). */
SFA_FUNC(08984D38, move_4_start_84d38) {
    gaddr o = ARG(0);
    *B(o + 4) = 1; *B(o + 5) = 0;
    int8_t facing = S8(o + 697);
    *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 0;
    int32_t strength = S8(o + 194);
    *B(o + 236) = 4;
    gaddr other = *W(o + 92);
    uint32_t v = *B(0x08A93D88u + (uint32_t)strength + 1);
    uint32_t ra = enter(c);
    c->r[6] = 4;
    call2(c, other, v, f_0886E3CC, 0x08984D8Cu);
    leave(c, ra);
}

/* (piece): a piece's place and facing from its owner (+88): the owner's place plus the offsets its
   own number (+3) and the owner's frame (+296's +9, five bits) pick from the list +416's +44, turned
   by the owner's facing. No frame. */
SFA_FUNC(08880AB8, piece_place_by_frame) {
    gaddr o = ARG(0), own = *W(o + 88);
    uint32_t at = (((uint32_t)*B(*W(own + 296) + 9) & 0x1F) << 3) + *B(o + 3);
    gaddr e = *W(*W(own + 416) + 44) + at * 2;
    int32_t dx = S16(e);
    uint8_t facing = *B(own + 11);
    *B(o + 11) = facing;
    if (facing != 0) dx = -dx;
    *H(o + 18) = (uint16_t)(S16(own + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(own + 22) + S16(e + 2));
}

/* (fighter): the heavy one with +0x334 = 7 takes the animation 0x22 (0x08861674); else 0x0890EF5C. */
SFA_FUNC(0890EE6C, anim_22_or_0890EF5C) {
    gaddr o = ARG(0);
    int32_t strength = S8(o + 194);
    uint32_t ra = enter(c);
    if (strength != 2) { c->r[31] = 0x0890EE8Cu; f_0890EF5C(c); }
    else if (S8(o + 0x334) != 7) { c->r[31] = 0x0890EEB0u; f_0890EF5C(c); }
    else { ARG(1) = 0x22; c->r[31] = 0x0890EEC4u; f_08861674(c); }
    leave(c, ra);
}

/* (): the fight task's sub-step (GAME + 8 into the table 0x08A123A4, with 0x08B01F90), then
   0x08837690, 0x08887AFC, 0x0883771C, 0x088379B8 and 0x0880B6CC. */
SFA_FUNC(0883D368, fight_substep_run) {
    gaddr handler = *W(0x08A123A4u + (uint32_t)*H(GAME + 8) * 4);
    uint32_t ra = enter(c);
    c->r[5] = handler; ARG(0) = 0x08B01F90u; c->r[31] = 0x0883D39Cu;
    rt_call_indirect(c, handler);
    c->r[31] = 0x0883D3A4u; f_08837690(c);
    c->r[31] = 0x0883D3ACu; f_08887AFC(c);
    c->r[31] = 0x0883D3B4u; f_0883771C(c);
    c->r[31] = 0x0883D3BCu; f_088379B8(c);
    c->r[31] = 0x0883D3C4u; f_0880B6CC(c);
    leave(c, ra);
}

/* (fighter): move 7's counter start facing the opponent: state 1 / 0 / 7 / 0, +235 set, +696 / +127
   cleared, +573 = 0x1C, the opponent's +127 = 0x18, and the animation 5. */
SFA_FUNC(088FDA00, move_7_counter_start) {
    gaddr o = ARG(0);
    *B(o + 4) = 1; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 5) = 0;
    int8_t facing = S8(o + 697);
    *B(o + 236) = 7; *B(o + 11) = (uint8_t)facing; *B(o + 235) = 1; *B(o + 696) = 0; *B(o + 127) = 0;
    *B(o + 573) = 0x1C;
    *B(*W(o + 92) + 127) = 0x18;
    uint32_t ra = enter(c);
    ARG(1) = 5; c->r[31] = 0x088FDA5Cu; f_088616EC(c);
    leave(c, ra);
}

/* (fighter): the next step: 2.0 across and 4.0 up with gravity 0x5800, the meter by 7, the animation
   8 and 0x0886DEC8. */
SFA_FUNC(089108D4, next_step_hop_anim_8) {
    gaddr o = ARG(0); uint32_t sv[2]; enter_s0(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    speeds(o, 0x20000, 0, 0x40000, 0xFFFFA800u);
    call2(c, o, 7, f_0886CA2C, 0x08910918u);
    call2(c, o, 8, f_088616EC, 0x08910924u);
    call1(c, o, f_0886DEC8, 0x0891092Cu);
    leave_s0(c, sv);
}
