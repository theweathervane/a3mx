/* Move steps of 37 - 38 instructions: a glide, landings into a move by strength, a pair of steps that
   stop on one check and step on with a creep on another, a part character's hop, a hop that lifts on
   a frame flag and a jump that shifts by side with a cue (0x0890E248). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08862004(Cpu *c);
void f_0896F62C(Cpu *c); void f_0898F9DC(Cpu *c); void f_08993090(Cpu *c); void f_089930D8(Cpu *c);
void f_08995134(Cpu *c); void f_0899939C(Cpu *c); void f_089993E4(Cpu *c); void f_088D3EC0(Cpu *c);
void f_089CA064(Cpu *c); void f_088D3848(Cpu *c); void f_088D3180(Cpu *c); void f_088D31AC(Cpu *c);
void f_0890E6C0(Cpu *c); void f_0887C408(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static uint32_t toward(gaddr o, uint32_t v) { return *B(o + 11) != 0 ? v : 0u - v; }

/* (fighter): +543 = 2; once +96 turns, +7 steps on; else (without a frame flag it also rises: +73 = 1,
   +20 by +100 by +108) it moves by +96 toward its facing, +96 by +104; ticked. */
SFA_FUNC(089316C8, move_glide_b) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t v = *W(o + 96);
    frame_in(c, &ra);
    *B(o + 543) = 2;
    if ((int32_t)v < 0) bump(o + 7);
    else {
        uint32_t x = *W(o + 16);
        if (S8(o + 77) == 0) {
            uint32_t vy = *W(o + 100);
            *B(o + 73) = 1;
            *W(o + 20) = *W(o + 20) + vy;
            *W(o + 100) = vy + *W(o + 108);
        }
        *W(o + 16) = x + toward(o, v);
        *W(o + 96) = v + *W(o + 104);
    }
    call1(c, o, f_08861848, 0x08931750u);
    frame_out(c, ra);
}

/* (fighter): when 0x08862004 lands it: at the floor, +73 cleared, then step 0xB and move 0x15 for
   strength 2 without +824 = 2, else step 7 and move 0x16; otherwise ticked. */
SFA_FUNC(08946104, move_land_15_16) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x08946118u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x08946128u);
    else {
        *W(o + 20) = (uint32_t)S16(o + 132) << 16;
        *B(o + 73) = 0;
        if (S8(o + 194) == 2 && *W(o + 824) != 2) { *B(o + 7) = 0xB; call2(c, o, 0x15, f_088616EC, 0x08946188u); }
        else { *B(o + 7) = 7; call2(c, o, 0x16, f_088616EC, 0x08946170u); }
    }
    leave(c, sv);
}

/* (fighter): 0x0896F62C; falling at or below the floor +7 steps on, at the floor, +20 / +73 cleared
   and move 7 + 2 x strength; else ticked. */
SFA_FUNC(0896D7CC, move_fall_land_7_2k) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0896F62C, 0x0896D7E0u);
    int32_t floor = S16(o + 132);
    if ((int32_t)*W(o + 100) < 0 && !((floor & 0xFFFF) < (S16(o + 22) & 0xFFFF))) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 194);
        *B(o + 7) = (uint8_t)(st + 1);
        *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
        call2(c, o, (uint32_t)(k + k + 7) & 0xFFFF, f_088616EC, 0x0896D838u);
    } else call1(c, o, f_08861848, 0x0896D850u);
    leave(c, sv);
}

/* (fighter): with +123 `alt`; else `stop` gives x speeds cleared, `go` steps +7 on with a creep (x 0.5)
   into move 6 + strength; ticked. */
static void stop_or_go(Cpu *c, void (*alt)(Cpu *), void (*stop)(Cpu *), void (*go)(Cpu *), const uint32_t b[5]) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f != 0) call1(c, o, alt, b[0]);
    else {
        call1(c, o, stop, b[1]);
        if (c->r[2] != 0) { *W(o + 96) = 0; *W(o + 104) = 0; }
        call1(c, o, go, b[2]);
        if (c->r[2] != 0) {
            uint32_t st = *B(o + 7);
            int32_t k = S8(o + 194);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 96) = 0x8000; *W(o + 104) = 0;
            call2(c, o, (uint32_t)(k + 6) & 0xFFFF, f_088616EC, b[3]);
        }
        call1(c, o, f_08861848, b[4]);
    }
    leave(c, sv);
}
SFA_FUNC(0898F814, move_stop_or_go) {
    static const uint32_t b[5] = { 0x0898F834u, 0x0898F844u, 0x0898F85Cu, 0x0898F890u, 0x0898F898u };
    stop_or_go(c, f_0898F9DC, f_08993090, f_089930D8, b);
}
SFA_FUNC(08994F6C, move_stop_or_go_b) {
    static const uint32_t b[5] = { 0x08994F8Cu, 0x08994F9Cu, 0x08994FB4u, 0x08994FE8u, 0x08994FF0u };
    stop_or_go(c, f_08995134, f_0899939C, f_089993E4, b);
}

/* (character): the part ticked, 0x089CA064; frame flag +76 starts a hop (+7 on, +73 = 1, x 3.375, y 4.0,
   fall -0.25, 0x088D3848, 0x088D3180); else 0x088D31AC until a frame flag. */
SFA_FUNC(089C8A14, part_flag_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8A28u);
    call1(c, o, f_089CA064, 0x089C8A30u);
    if (S8(o + 76) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1); *B(o + 73) = 1;
        *W(o + 96) = 0x36000; *W(o + 100) = 0x40000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFC000u;
        call1(c, o, f_088D3848, 0x089C8A78u);
        call1(c, o, f_088D3180, 0x089C8A80u);
    } else if (S8(o + 77) == 0) call1(c, o, f_088D31AC, 0x089C8A98u);
    leave(c, sv);
}

/* (fighter): ticked; a frame flag (cleared) lifts it 40 with step 3 and +82 = 10; then it moves by +96
   toward its facing (+96 by +104) and +20 by +100 by +108. */
SFA_FUNC(088FA594, move_tick_lift_fly) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x088FA5A8u);
    int32_t k = S8(o + 77);
    uint32_t x = *W(o + 16), v = *W(o + 96);
    if (k != 0) {
        int32_t y = S16(o + 22);
        *B(o + 7) = 3; *H(o + 22) = (uint16_t)(y + 0x28); *B(o + 77) = 0; *H(o + 82) = 10;
    }
    x += toward(o, v);
    uint32_t a = *W(o + 104), y20 = *W(o + 20);
    *W(o + 16) = x;
    uint32_t vy = *W(o + 100);
    *W(o + 96) = v + a;
    uint32_t g = *W(o + 108);
    *W(o + 20) = y20 + vy;
    *W(o + 100) = vy + g;
    leave(c, sv);
}

/* (fighter): a jump: step 2, +73 = 1, 0x90 toward its side and 56 up, x 4.0, y 1.0, fall -0.164;
   0x0890E6C0, cue 0x220 from its side and move 0x38. */
SFA_FUNC(0890E248, move_side_jump_38) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = 2; *B(o + 73) = 1;
    uint32_t d = S8(o + 337) != 0 ? 0x90u : 0xFF70u;
    int32_t x = S16(o + 18), y = S16(o + 22);
    *H(o + 18) = (uint16_t)(x + d);
    *H(o + 22) = (uint16_t)(y + 0x38);
    *W(o + 96) = 0x40000; *W(o + 104) = 0; *W(o + 100) = 0x10000; *W(o + 108) = 0xFFFFD600u;
    call1(c, o, f_0890E6C0, 0x0890E2B4u);
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x220; c->r[31] = 0x0890E2C4u; f_0887C408(c);
    call2(c, o, 0x38, f_0886163C, 0x0890E2D0u);
    leave(c, sv);
}
