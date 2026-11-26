/* Move steps of 34 instructions: landings with a slide, frame-flag jumps and dashes with their speeds by
   strength (with a cue), a slide that taunts in some modes (0x089A2DE4), and part characters'
   teleport arrivals. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08961448(Cpu *c);
void f_0896F62C(Cpu *c); void f_0887C614(Cpu *c); void f_0887C408(Cpu *c); void f_08881F70(Cpu *c);
void f_08862004(Cpu *c); void f_089A8A18(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D31EC(Cpu *c);
void f_089CA064(Cpu *c); void f_088D35D0(Cpu *c); void f_088D36D0(Cpu *c); void f_088D2DBC(Cpu *c);
void f_088D3960(Cpu *c);

#define GAME 0x08E75910u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (fighter): 0x08961448; when it is negative and the fighter at or below the floor, +7 steps on at the
   floor with a slide (x 3.0 / -0.47) and move 0x24; else ticked. */
SFA_FUNC(0895E848, move_check_land_slide_24) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08961448, 0x0895E85Cu);
    int32_t floor = S16(o + 132);
    if ((int32_t)c->r[2] < 0 && !(floor < S16(o + 22))) {
        uint32_t st = *B(o + 7);
        *H(o + 22) = (uint16_t)floor;
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 20) = 0;
        *W(o + 96) = 0x30000; *W(o + 104) = 0xFFFF8800u; *W(o + 100) = 0; *W(o + 108) = 0;
        call2(c, o, 0x24, f_088616EC, 0x0895E8B0u);
    } else call1(c, o, f_08861848, 0x0895E8C0u);
    leave(c, sv);
}

/* (fighter): ticked until a frame flag; 0x0896F62C; falling at or below the floor it lands (+7 on, +20 /
   +73 cleared, +74 = 1, cue 0x22 / 0x23). */
SFA_FUNC(0896DFA4, move_tick_fall_land_cue) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) call1(c, o, f_08861848, 0x0896DFC4u);
    call1(c, o, f_0896F62C, 0x0896DFCCu);
    if ((int32_t)*W(o + 100) < 0) {
        int32_t floor = S16(o + 132);
        if (!((int32_t)((uint32_t)floor & 0xFFFF) < (int32_t)((uint32_t)S16(o + 22) & 0xFFFF))) {
            uint32_t st = *B(o + 7);
            *H(o + 22) = (uint16_t)floor;
            *B(o + 7) = (uint8_t)(st + 1);
            *H(o + 20) = 0; *B(o + 73) = 0; *B(o + 74) = 1;
            call1(c, o, f_0887C614, 0x0896E01Cu);
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; a frame flag starts a jump: +7 on, +73 = 1, 40 up, the four speeds by strength
   (0x08A8DEEC: x, x brake, y, y fall). */
SFA_FUNC(0896E02C, move_tick_flag_jump5) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0896E040u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        int32_t y = S16(o + 22);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1;
        int32_t k = S8(o + 194);
        *B(o + 77) = 0;
        *H(o + 22) = (uint16_t)(y + 0x28);
        gaddr e = 0x08A8DEECu + ((uint32_t)k << 4);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4); *W(o + 100) = *W(e + 8); *W(o + 108) = *W(e + 12);
    }
    leave(c, sv);
}

/* (fighter): a frame flag starts a dash: +7 on, +73 = 1, +82 = 7, x speeds by strength (`tab`), cue
   0x213 from its side; ticked. */
static void flag_dash_cue(Cpu *c, gaddr tab, uint32_t b_cue, uint32_t b_tick) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        uint32_t st = *B(o + 7);
        int32_t s = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *B(o + 73) = 1; *H(o + 82) = 7;
        gaddr e = tab + ((uint32_t)s << 3);
        int32_t side = S8(o + 337);
        *W(o + 96) = *W(e); *W(o + 104) = *W(e + 4);
        ARG(0) = o; ARG(1) = (uint32_t)side; ARG(2) = 0x213; c->r[31] = b_cue; f_0887C408(c);
    }
    call1(c, o, f_08861848, b_tick);
    leave(c, sv);
}
SFA_FUNC(0898F6F4, move_flag_dash_cue) { flag_dash_cue(c, 0x08A96624u, 0x0898F764u, 0x0898F76Cu); }
SFA_FUNC(08994E4C, move_flag_dash_cue_b) { flag_dash_cue(c, 0x08A98720u, 0x08994EBCu, 0x08994EC4u); }

/* (fighter): slides (+16 by +96, +96 by +104) while the +82 timer runs (past it +7 steps on); unless the
   game's +59 bits 0 - 1 are set, 0x08881F70; ticked. */
SFA_FUNC(089A2DE4, move_slide_timer) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t v = *W(o + 96);
    *W(o + 16) = *W(o + 16) + v;
    int32_t h = S16(o + 82);
    *W(o + 96) = v + *W(o + 104);
    uint32_t t = (uint32_t)h - 1;
    *H(o + 82) = (uint16_t)t;
    enter(c, o, sv);
    if ((int16_t)t < 0) bump(o + 7);
    if ((S8(GAME + 59) & 3) == 0) call1(c, o, f_08881F70, 0x089A2E54u);
    call1(c, o, f_08861848, 0x089A2E5Cu);
    leave(c, sv);
}

/* (fighter): 0x08862004; ticked until a frame flag; each flag counts +82 down, and past 0 +7 steps on
   with a fall of -0.375 and move 0x22. */
SFA_FUNC(089A4998, move_rise_flag_count) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x089A49ACu);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089A49C0u);
    else {
        *B(o + 77) = 0;
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t >= 0) call1(c, o, f_08861848, 0x089A49ECu);
        else {
            uint32_t st = *B(o + 7);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 108) = 0xFFFFA000u;
            call2(c, o, 0x22, f_088616EC, 0x089A4A10u);
        }
    }
    leave(c, sv);
}

/* (fighter): step 0: step 1, 0x089A8A18 (1) and move 0x22; later standing once ended, else ticked. */
SFA_FUNC(089A5848, move_start_22_8a18) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t st = *B(o + 7);
    enter(c, o, sv);
    if (st == 0) {
        *B(o + 7) = 1;
        call2(c, o, 1, f_089A8A18, 0x089A5874u);
        call2(c, o, 0x22, f_0886163C, 0x089A5880u);
    } else if (S8(o + 75) < 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
        call2(c, o, 0, f_0886163C, 0x089A58B0u);
    } else call1(c, o, f_08861848, 0x089A58C0u);
    leave(c, sv);
}

/* (character): the part ticked, 0x088D31EC, 0x089CA064; when 0x088D35D0 gives 1, +73 cleared, +7 steps
   on, 0x088D36D0, cue 0x22, 0x088D3960 (0, 0) and move `base` + strength. */
static void part_arrive(Cpu *c, int32_t base, const uint32_t b[8]) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, b[0]);
    call1(c, o, f_088D31EC, b[1]);
    call1(c, o, f_089CA064, b[2]);
    call1(c, o, f_088D35D0, b[3]);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D36D0, b[4]);
        call1(c, o, f_088D2DBC, b[5]);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = b[6]; f_088D3960(c);
        call2(c, o, (uint32_t)(S8(o + 194) + base), f_088616EC, b[7]);
    }
    leave(c, sv);
}
SFA_FUNC(089C81C8, part_arrive_12) {
    static const uint32_t b[8] = { 0x089C81DCu, 0x089C81E4u, 0x089C81ECu, 0x089C81F4u,
                                   0x089C8218u, 0x089C8220u, 0x089C8230u, 0x089C8240u };
    part_arrive(c, 0x12, b);
}
SFA_FUNC(089C8C28, part_arrive_38) {
    static const uint32_t b[8] = { 0x089C8C3Cu, 0x089C8C44u, 0x089C8C4Cu, 0x089C8C54u,
                                   0x089C8C78u, 0x089C8C80u, 0x089C8C90u, 0x089C8CA0u };
    part_arrive(c, 0x38, b);
}
