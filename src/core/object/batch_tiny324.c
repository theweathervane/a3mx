/* More small ones whose callees are all done: who leads by the mode, a random partner, and moves'
   starts by the choice. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define FIGHTER_2 0x092E4338u

void f_08828308(Cpu *c); void f_08828200(Cpu *c); void f_0882AC40(Cpu *c); void f_0882F9D0(Cpu *c);
void f_0883021C(Cpu *c); void f_088F1E74(Cpu *c); void f_088616EC(Cpu *c); void f_08821330(Cpu *c);
void f_0890E558(Cpu *c); void f_0886163C(Cpu *c); void f_089140AC(Cpu *c); void f_0892545C(Cpu *c);
void f_0886C380(Cpu *c); void f_0893567C(Cpu *c); void f_08982B1C(Cpu *c); void f_08861674(Cpu *c);
void f_089815C0(Cpu *c); void f_08984440(Cpu *c); void f_088D3514(Cpu *c); void f_089B760C(Cpu *c);
void f_089BDE08(Cpu *c); void f_089C338C(Cpu *c); void f_089CA13C(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, gaddr o, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
    c->r[16] = o;
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (a, b): who leads on vitality: in mode 0x12 (a + 114) 0x08828308; else in the first mode
   (b + 79 = 0) the plain 0x08828200, otherwise the one with the handicap. */
SFA_FUNC(08828184, vitality_lead_by_mode) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t mode = S8(a + 114);
    *W(c->r[29] + 16) = ra;
    c->r[6] = b; c->r[7] = (uint32_t)mode; c->r[8] = 0x12;
    if (mode == 0x12) call2(c, a, b, f_08828308, 0x088281B4u);
    else if (S8(b + 79) == 0) call2(c, a, b, f_08828200, 0x088281DCu);
    else call2(c, a, b, f_0882AC40, 0x088281F4u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (-, y): fighter 2 becomes a random opponent (0x0882F9D0 on its +338, +386, +438 and +1173), and
   y's +100 what 0x0883021C makes of the character. A frame keeping s0, s1. */
SFA_FUNC(0882FABC, random_fighter_2) {
    gaddr y = ARG(1);
    uint32_t sv[3];
    enter2(c, y, sv);
    c->r[17] = FIGHTER_2;
    ARG(0) = FIGHTER_2 + 0x152; ARG(1) = FIGHTER_2 + 0x182; ARG(2) = FIGHTER_2 + 0x1B6; ARG(3) = FIGHTER_2 + 0x495;
    call0(c, f_0882F9D0, 0x0882FAECu);
    call1(c, (uint32_t)(int32_t)S8(FIGHTER_2 + 338), f_0883021C, 0x0882FAF4u);
    *H(y + 100) = (uint16_t)c->r[2];
    leave2(c, sv);
}

/* (fighter): +260 cleared, 0x20 forward, the pair of objects (0x088F1E74) and the animation
   0x2B. */
SFA_FUNC(088F0B94, pair_and_step_forward) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    uint8_t facing = *B(o + 11);
    *B(o + 260) = 0;
    int32_t x = S16(o + 18);
    *H(o + 18) = (uint16_t)(facing != 0 ? x + 0x20 : x - 0x20);
    call1(c, o, f_088F1E74, 0x088F0BD0u);
    call2(c, o, 0x2B, f_088616EC, 0x088F0BDCu);
    leave(c, sv);
}

/* (fighter): step 5, hidden: 0x90 further from its own side, 8.0 across with no pull, +108 from
   where 0x08821330's object stands, +82 = -1, the four timed objects and the cue 0x39. */
SFA_FUNC(0890E400, vanish_with_four) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 5;
    *B(o + 1) = 0;
    int32_t off = S8(o + 337) != 0 ? 0x90 : 0xFF70;
    *H(o + 18) = (uint16_t)(S16(o + 18) + off);
    *W(o + 96) = 0x80000;
    *W(o + 104) = 0;
    call1(c, o, f_08821330, 0x0890E44Cu);
    *W(o + 108) = (uint32_t)(int32_t)S16(c->r[2] + 18);
    *H(o + 82) = 0xFFFF;
    call1(c, o, f_0890E558, 0x0890E464u);
    call2(c, o, 0x39, f_0886163C, 0x0890E470u);
    leave(c, sv);
}

/* (fighter): step 8: where it stands is kept at +0x340, it moves 0x20 from its own side, the
   seven objects (0x089140AC), no fall, +10 cleared, 0x78 frames, the cue 0x24 from its third
   frame on. */
SFA_FUNC(0890E804, seven_and_wait) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t x = S16(o + 18);
    *B(o + 7) = 8;
    *H(o + 0x334 + 12) = (uint16_t)x;
    int32_t off = S8(o + 337) != 0 ? 0x20 : 0xFFE0;
    ARG(1) = (uint32_t)(S16(o + 18) + off);
    *H(o + 18) = (uint16_t)(S16(o + 18) + off);
    call1(c, o, f_089140AC, 0x0890E850u);
    *W(o + 100) = 0;
    *B(o + 10) = 0;
    *H(o + 82) = 0x78;
    call2(c, o, 0x24, f_0886163C, 0x0890E86Cu);
    *W(o + 28) = *W(o + 28) + 0x14;
    leave(c, sv);
}

/* (fighter): step 0x1B: stopped, +3 cleared, +591 set, the animation marked ended (+75 = -1),
   0x08821330 and the three animations in a row (0x0892545C). */
SFA_FUNC(0892540C, three_in_a_row_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 0x1B;
    *W(o + 96) = 0;
    *B(o + 3) = 0;
    *B(o + 591) = 1;
    *B(o + 75) = 0xFF;
    c->r[5] = 0xFFFFFFFFu;
    call1(c, o, f_08821330, 0x08925444u);
    call1(c, o, f_0892545C, 0x0892544Cu);
    leave(c, sv);
}

/* (fighter): when 0x0886C380 allows, the move 0x0893567C. Returns 0 when it started, else 1. */
SFA_FUNC(08935630, start_3567c_if) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_0886C380, 0x08935644u);
    if (c->r[2] == 0) c->r[2] = 1;
    else {
        call1(c, o, f_0893567C, 0x08935654u);
        c->r[2] = 0;
    }
    leave(c, sv);
}

/* (fighter): the next step: +108 from where it stands, placed against 0x08821330's object (0x30
   before it from the first side, 0x1B0 past it from the second), 2.0 across with no pull, +10
   cleared, the five objects (0x08982B1C) and the animation 0x1F. */
SFA_FUNC(0897AA50, five_and_enter) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    uint8_t step = *B(o + 7);
    uint32_t at = *W(o + 16);
    *B(o + 7) = (uint8_t)(step + 1);
    *W(o + 108) = at;
    call1(c, o, f_08821330, 0x0897AA7Cu);
    int32_t base = S16(c->r[2] + 18);
    *H(o + 18) = (uint16_t)(S8(o + 337) != 0 ? base + 0x1B0 : base - 0x30);
    *W(o + 96) = 0x20000;
    *W(o + 104) = 0;
    *B(o + 10) = 0;
    call1(c, o, f_08982B1C, 0x0897AAB4u);
    call2(c, o, 0x1F, f_08861674, 0x0897AAC0u);
    leave(c, sv);
}

/* (object, fighter): its +5 moves on, 0x089815C0 runs with it, the fighter becomes its +92 and
   the fighter's +0x338 is -1. A frame keeping s0, s1. */
SFA_FUNC(0898147C, attach_to_fighter) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t sv[3];
    enter2(c, o, sv);
    c->r[17] = f;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    call1(c, o, f_089815C0, 0x089814A8u);
    *W(o + 92) = f;
    *B(f + 0x334 + 4) = 0xFF;
    leave2(c, sv);
}

/* (fighter): when 0x0886C380 allows, the move 0x08984440. Returns whether. A frame keeping s0,
   s1. */
SFA_FUNC(089843F8, start_84440_if) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    c->r[17] = 0;
    call0(c, f_0886C380, 0x08984414u);
    if (c->r[2] != 0) {
        c->r[17] = 1;
        call1(c, o, f_08984440, 0x08984428u);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter): a move's start by the choice: the choice 0 gets the move / 1 at its own strength,
   facing as +697 says, +777 set, 0x088D3514 and the animation; the others their own start. */
static void move_start_by_choice(Cpu *c, void (*others)(Cpu *), uint8_t move, uint8_t strength, uint32_t anim,
                                 const uint32_t back[3]) {
    gaddr o = ARG(0);
    int32_t choice = S8(o + 386);
    uint32_t sv[2];
    enter(c, o, sv);
    if (choice != 0) call1(c, o, others, back[0]);
    else {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = move;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        *B(o + 194) = strength;
        *B(o + 777) = 1;
        c->r[5] = strength == 2 ? 2u : (uint32_t)(int32_t)facing;
        call1(c, o, f_088D3514, back[1]);
        call2(c, o, anim, f_088616EC, back[2]);
    }
    leave(c, sv);
}
SFA_FUNC(089B75A0, move_start_by_choice_b75a0) {
    static const uint32_t back[3] = { 0x089B75C0u, 0x089B75F0u, 0x089B75FCu };
    move_start_by_choice(c, f_089B760C, 8, 1, 0x2C, back);
}
SFA_FUNC(089BDD9C, move_start_by_choice_bdd9c) {
    static const uint32_t back[3] = { 0x089BDDBCu, 0x089BDDECu, 0x089BDDF8u };
    move_start_by_choice(c, f_089BDE08, 9, 1, 0x44, back);
}
SFA_FUNC(089C3320, move_start_by_choice_c3320) {
    static const uint32_t back[3] = { 0x089C3340u, 0x089C3370u, 0x089C337Cu };
    move_start_by_choice(c, f_089C338C, 0xA, 1, 0x28, back);
}
SFA_FUNC(089CA0CC, move_start_by_choice_ca0cc) {
    static const uint32_t back[3] = { 0x089CA0ECu, 0x089CA120u, 0x089CA12Cu };
    move_start_by_choice(c, f_089CA13C, 0xC, 2, 0x3F, back);
}
