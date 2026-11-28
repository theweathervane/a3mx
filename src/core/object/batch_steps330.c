/* What the last ones set free: four characters' move inputs as plain lists of tries, the run's
   lines for both sides, and three moves' steps. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

void f_0883323C(Cpu *c); void f_0886469C(Cpu *c); void f_0890F03C(Cpu *c); void f_0886BE50(Cpu *c);
void f_0890F16C(Cpu *c); void f_08861848(Cpu *c); void f_08921850(Cpu *c); void f_08922B08(Cpu *c);
void f_08922AC0(Cpu *c); void f_089229C0(Cpu *c);
void f_089B5168(Cpu *c); void f_089B5238(Cpu *c); void f_089B52EC(Cpu *c); void f_089B543C(Cpu *c);
void f_089B54E8(Cpu *c); void f_089B53A0(Cpu *c); void f_089B5594(Cpu *c); void f_089B563C(Cpu *c);
void f_089B56DC(Cpu *c); void f_089B577C(Cpu *c);
void f_089B92E0(Cpu *c); void f_089B9500(Cpu *c); void f_089B93A8(Cpu *c); void f_089B9454(Cpu *c);
void f_089B9614(Cpu *c); void f_089B9734(Cpu *c); void f_089B9A10(Cpu *c); void f_089B97DC(Cpu *c);
void f_089B9880(Cpu *c); void f_089B9AC4(Cpu *c); void f_089B9B50(Cpu *c); void f_089B9BF0(Cpu *c);
void f_089C0980(Cpu *c); void f_089C09B8(Cpu *c); void f_089C0AA8(Cpu *c); void f_089C0B54(Cpu *c);
void f_089C0C08(Cpu *c); void f_089C0D20(Cpu *c); void f_089C0DC0(Cpu *c); void f_089C0E68(Cpu *c);
void f_089C0F04(Cpu *c); void f_089C0FB0(Cpu *c); void f_089C1104(Cpu *c); void f_089C1064(Cpu *c);
void f_089C6E10(Cpu *c); void f_089C77A0(Cpu *c); void f_089C7594(Cpu *c); void f_089C7640(Cpu *c);
void f_089C7220(Cpu *c); void f_089C76FC(Cpu *c); void f_089C7360(Cpu *c); void f_089C7474(Cpu *c);
void f_089C72C0(Cpu *c); void f_089C785C(Cpu *c); void f_089C6F74(Cpu *c); void f_089C6ED8(Cpu *c);

typedef void (*Fn)(Cpu *);

static void call1(Cpu *c, uint32_t a, Fn fn, uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* Each in turn with the fighter until one returns nonzero; their return addresses are 16 apart
   from `back`. Returns whether one did. */
static int tries(Cpu *c, gaddr o, const Fn *t, int n, uint32_t back) {
    for (int i = 0; i < n; i++) {
        call1(c, o, t[i], back + 16u * (uint32_t)i);
        if (c->r[2] != 0) return 1;
    }
    return 0;
}

/* (x, y): the run's lines for both sides in the fight proper (x's first three words 1, 2, 3): both
   fighters' +412 are cleared, and with y's +36 clear the side x's +53 says gets its set; then,
   unless y's +23 is set, 0x0883323C draws fighter 1's at 0x128 / 0xE4 and fighter 2's at 0x14 /
   0x34. A 0x30 frame keeping s0..s2 from +32. */
SFA_FUNC(08832898, run_lines) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0; *W(sp + 36) = s1;
    c->r[17] = x; c->r[16] = y;
    *W(sp + 40) = s2; *W(sp + 44) = ra;
    c->r[18] = FIGHTER_2;
    if (*H(x) == 1 && *H(x + 2) == 2 && *H(x + 4) == 3) {
        *B(FIGHTER_1 + 412) = 0; *B(FIGHTER_2 + 412) = 0;
        if (S8(y + 36) == 0) *B(((S8(x + 53) & 1) ? FIGHTER_1 : FIGHTER_2) + 412) = 1;
        if (S8(y + 23) == 0) {
            ARG(0) = x; ARG(1) = y; ARG(2) = FIGHTER_1; ARG(3) = 0x128; c->r[8] = 0xE4;
            c->r[31] = 0x0883293Cu; f_0883323C(c);
            ARG(0) = x; ARG(1) = y; ARG(2) = FIGHTER_2; ARG(3) = 0x14; c->r[8] = 0x34;
            c->r[31] = 0x08832954u; f_0883323C(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): a roll's step: at the animation's end +0x334 is cleared and it ends (0x0886469C).
   The cpu goes by 0x0890F03C. A player, on the cue's top bit (+77): when 0x0886BE50 says so,
   +567 = -1, the step back to 0 and ticked; otherwise the roll's turn (0x0890F16C) either way. */
SFA_FUNC(0890F0C0, roll_step_f0c0) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t ended = S8(o + 75) & 0x80;
    enter(c, o, sv);
    if (ended) {
        *B(o + 0x334) = 0;
        call1(c, o, f_0886469C, 0x0890F0ECu);
    } else if (S8(o + 373) != 0) call1(c, o, f_0890F03C, 0x0890F108u);
    else if (!(S8(o + 77) & 0x80)) call1(c, o, f_0890F16C, 0x0890F15Cu);
    else {
        call1(c, o, f_0886BE50, 0x0890F124u);
        if (c->r[2] != 0) {
            *B(o + 567) = 0xFF;
            *B(o + 7) = 0;
            call1(c, o, f_08861848, 0x0890F14Cu);
        } else call1(c, o, f_0890F16C, 0x0890F134u);
    }
    leave(c, sv);
}

/* (fighter): an air move's player side (the cpu's is 0x089229C0): 0x08922AC0 once 0x08921850 says
   so. Near the top of its flight (less than 6 up or down a frame) a kick just pressed leaps back
   from the opponent (0x08922B08 with 0, 1 or 2 for the light, middle and heavy kick); otherwise
   ticked. */
SFA_FUNC(089228E0, air_move_player_228e0) {
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t sv[2];
    enter(c, o, sv);
    if (cpu != 0) { call1(c, o, f_089229C0, 0x089229B0u); leave(c, sv); return; }
    call1(c, o, f_08921850, 0x08922900u);
    if (c->r[2] != 0) call1(c, o, f_08922AC0, 0x089229A0u);
    else {
        int32_t vy = (int32_t)*W(o + 100);
        int32_t whole = S16(o + 102);
        if (vy < 0) whole = (int16_t)-whole;
        if (!((int32_t)((uint32_t)whole & 0xFFFF) < 6)) call1(c, o, f_08861848, 0x08922990u);
        else {
            uint32_t kicks = *H(o + 760);
            if (kicks & 0x70) {
                ARG(1) = (kicks & 0x10) ? 0 : (kicks & 0x20) ? 1 : 2;
                call1(c, o, f_08922B08, 0x08922968u);
            } else {
                ARG(1) = 2;
                call1(c, o, f_08861848, 0x08922978u);
            }
        }
    }
    leave(c, sv);
}

/* (fighter): a character's move inputs: ten tries in the game's order, the throw's input last. */
SFA_FUNC(089B5050, move_tries_b5050) {
    static const Fn t[10] = { f_089B5168, f_089B5238, f_089B52EC, f_089B543C, f_089B54E8,
                              f_089B53A0, f_089B5594, f_089B563C, f_089B56DC, f_089B577C };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    tries(c, o, t, 10, 0x089B5064u);
    leave(c, sv);
}

/* (fighter): the same for another character, twelve tries. */
SFA_FUNC(089B920C, move_tries_b920c) {
    static const Fn t[12] = { f_089B92E0, f_089B9500, f_089B93A8, f_089B9454, f_089B9614, f_089B9734,
                              f_089B9A10, f_089B97DC, f_089B9880, f_089B9AC4, f_089B9B50, f_089B9BF0 };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    tries(c, o, t, 12, 0x089B9220u);
    leave(c, sv);
}

/* (fighter): another character: 0x089C0980 first, whatever it says, then eleven tries. */
SFA_FUNC(089C08B4, move_tries_c08b4) {
    static const Fn t[11] = { f_089C09B8, f_089C0AA8, f_089C0B54, f_089C0C08, f_089C0D20, f_089C0DC0,
                              f_089C0E68, f_089C0F04, f_089C0FB0, f_089C1104, f_089C1064 };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_089C0980, 0x089C08C8u);
    tries(c, o, t, 11, 0x089C08D0u);
    leave(c, sv);
}

/* (fighter): another character: one try, then six with GAME + 74 set and eight without (two more,
   in another order), then three more, the throw's input among them. */
SFA_FUNC(089C6CC4, move_tries_c6cc4) {
    static const Fn first[1] = { f_089C6E10 };
    static const Fn set[6] = { f_089C77A0, f_089C7594, f_089C7640, f_089C7220, f_089C76FC, f_089C7360 };
    static const Fn clear[8] = { f_089C7474, f_089C77A0, f_089C7594, f_089C72C0,
                                 f_089C7640, f_089C7220, f_089C76FC, f_089C7360 };
    static const Fn last[3] = { f_089C785C, f_089C6F74, f_089C6ED8 };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!tries(c, o, first, 1, 0x089C6CD8u)) {
        int took = S8(GAME + 74) != 0 ? tries(c, o, set, 6, 0x089C6CF8u) : tries(c, o, clear, 8, 0x089C6D60u);
        if (!took) tries(c, o, last, 3, 0x089C6DE0u);
    }
    leave(c, sv);
}
