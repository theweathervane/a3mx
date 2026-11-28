/* More of what the small ones set free: the next opponent a list allows, a menu's own thread, the
   link's match-up steps, and moves' steps in the air and on the way down. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u

void f_08830384(Cpu *c); void f_0882FC2C(Cpu *c); void f_08831A78(Cpu *c); void f_088CC690(Cpu *c);
void f_0880A6CC(Cpu *c); void f_08837690(Cpu *c); void f_0883771C(Cpu *c); void f_08887AFC(Cpu *c);
void f_088379B8(Cpu *c); void f_0880B6CC(Cpu *c); void f_088054F0(Cpu *c); void f_088DF778(Cpu *c);
void f_088E3ED8(Cpu *c); void f_088DD114(Cpu *c); void f_088DD164(Cpu *c); void f_088D5680(Cpu *c);
void f_088DFEF4(Cpu *c); void f_088E02C0(Cpu *c); void f_08861848(Cpu *c); void f_088616EC(Cpu *c);
void f_088FA7FC(Cpu *c); void f_088FA9F0(Cpu *c); void f_088FA730(Cpu *c); void f_088FA924(Cpu *c);
void f_08912FEC(Cpu *c); void f_0887C614(Cpu *c); void f_08910830(Cpu *c); void f_089107E8(Cpu *c);
void f_089166B0(Cpu *c); void f_0886469C(Cpu *c); void f_089216B4(Cpu *c); void f_08921988(Cpu *c);
void f_08921850(Cpu *c); void f_08921940(Cpu *c); void f_08867BB0(Cpu *c); void f_089218B8(Cpu *c);
void f_0886163C(Cpu *c); void f_08821330(Cpu *c); void f_0887CF24(Cpu *c); void f_0887C408(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
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

/* (a, b, d, fighter): the first opponent on the list (0x09517988) that 0x08830384 does not hold
   back (with d), up to the list's length (b + 72); its place is kept at b + 20. Within the list it
   becomes the fighter's character and choice (the bytes at 0x095179B0): the given fighter, or with
   b's +56 set the first fighter of a's side (+57). Then in mode 7 (a + 114) 0x08831A78 with the
   character, else 0x0882FC2C. A 0x40 frame keeping s0..s7. */
SFA_FUNC(0882FEB8, first_free_opponent) {
    const gaddr who = 0x09517988u;
    gaddr a = ARG(0), b = ARG(1);
    uint32_t d = ARG(2), f = ARG(3);
    uint32_t sv[9];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = sv[6]; *W(sp + 36) = sv[5]; *W(sp + 44) = sv[7];
    for (uint32_t i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *H(b + 20) = 0xFFFF;
    *W(sp + 32) = sv[4]; *W(sp + 48) = sv[8];
    c->r[22] = who; c->r[19] = a; c->r[18] = b; c->r[17] = d; c->r[16] = f;
    int32_t at = -1, length;
    gaddr next = who;
    uint32_t ch;
    for (;;) {
        at += 1;
        ch = *B(who + (uint32_t)at);
        c->r[23] = (uint32_t)at; c->r[20] = ch; c->r[21] = next;
        call2(c, *B(next), d, f_08830384, 0x0882FF20u);
        length = S16(b + 72);
        if (c->r[2] == 0) break;
        next += 1;
        if (!(at < length)) break;
    }
    c->r[21] = next;
    *H(b + 20) = (uint16_t)at;
    if (!(length < at)) {
        if (S16(b + 56) != 0) f = (S8(a + 57) & 1) ? FIGHTER_2 : FIGHTER_1;
        c->r[16] = f;
        *B(f + 338) = (uint8_t)ch;
        *B(f + 386) = *B(0x095179B0u + (uint32_t)at);
        if (S8(a + 114) == 7) call2(c, b, (uint32_t)(int32_t)S8(f + 338), f_08831A78, 0x0882FFB8u);
        else {
            ARG(3) = ch;
            call3(c, a, b, d, f_0882FC2C, 0x0882FFA4u);
        }
    }
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[8]; c->r[29] += 0x40;
}

/* A menu's own thread, for ever: the step from the table 0x08A3C678 (by GAME + 2) with the menu's
   state 0x08B02E70; then with that state's +1 set in game mode 1 the clean-up 0x088CC690, and
   otherwise the screen's four parts, with the text 0x08A00078 first unless it is game mode 2 with
   +1 set; then 0x0880B6CC and a frame's wait (0x088054F0). A 0x30 frame keeping s0 and s1; it does
   not return. */
SFA_FUNC(088CC5A4, menu_thread) {
    const gaddr menu = 0x08B02E70u;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0; *W(sp + 36) = s1; *W(sp + 40) = ra;
    c->r[16] = menu;
    for (;;) {
        c->r[17] = GAME;
        uint32_t fn = *W(0x08A3C678u + ((uint32_t)*H(GAME + 2) << 2));
        ARG(0) = menu; c->r[31] = 0x088CC5E4u;
        rt_call_indirect(c, fn);
        if (*H(GAME + 6) == 1 && S8(menu + 1) == 1) call0(c, f_088CC690, 0x088CC678u);
        else {
            if (!(*H(GAME + 6) == 2 && S8(menu + 1) == 1)) {
                ARG(1) = 2; ARG(2) = 0x70; ARG(3) = 5; c->r[8] = 0xB;
                call1(c, 0x08A00078u, f_0880A6CC, 0x088CC648u);
            }
            call0(c, f_08837690, 0x088CC650u);
            call0(c, f_0883771C, 0x088CC658u);
            call0(c, f_08887AFC, 0x088CC660u);
            call0(c, f_088379B8, 0x088CC668u);
        }
        call0(c, f_0880B6CC, 0x088CC680u);
        call1(c, 1, f_088054F0, 0x088CC688u);
    }
}

/* (x): the link's match-up, by its step (x + 0):
   0, 1  the flags +2 = 3, waits of 0x12C and 0x4B0, 0x088E3ED8 (1); then who to connect to (+16):
         with more than one peer in the kinds 2 and 3 (+42) what 0x088DD164 (-1) picks, else the
         peer +12 with the flags 1, or 2 when +32 is set, and a wait of 0x12C;
   2     while flags are left, against the timeout +6 (0x81001001): the flag 1 polls 0x088DFEF4 on
         the peer +12 until it is done; the flag 2 waits out +4 and then steps the connection
         (0x088E02C0 on +16) until it is done; errors are reported (0x088D5680);
   3     a wait of 0xB4, which 4 counts down;
   5     done: +24 goes up and the steps start over.
   Any other step resets (0x088DF778, the handler 0x08A3EB14). It ends by drawing the message for
   +42 + +43 from the table 0x08A3EA04. A 0x30 frame keeping s0, s1. */
SFA_FUNC(088DA67C, link_match_up) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s0; c->r[16] = x;
    uint32_t step = *H(x);
    *W(sp + 36) = s1; *W(sp + 40) = ra;
    c->r[17] = 2;
    switch (step) {
    case 0:
        *H(x) = 1;
        *H(x + 2) = 3;
        *H(x + 4) = 0x12C;
        *H(x + 6) = 0x4B0;
        call1(c, 1, f_088E3ED8, 0x088DA6F4u);
        /* fall through */
    case 1: {
        uint32_t kind = *B(x + 42);
        int many = 0;
        if (kind == 3 || kind == 2) {
            call0(c, f_088DD114, 0x088DA720u);
            many = !((int32_t)c->r[2] < 2);
        }
        if (many) {
            call1(c, 0xFFFFFFFFu, f_088DD164, 0x088DA734u);
            uint16_t s = *H(x);
            *W(x + 16) = c->r[2];
            *H(x) = (uint16_t)(s + 1);
        } else {
            uint32_t peer = *W(x + 12);
            uint16_t s = *H(x);
            *W(x + 16) = peer;
            *H(x + 2) = *B(x + 32) != 0 ? 2 : 1;
            *H(x + 4) = 0x12C;
            *H(x) = (uint16_t)(s + 1);
        }
        break;
    }
    case 2: {
        uint32_t flags = *H(x + 2);
        if (flags == 0) { *H(x) = (uint16_t)(step + 1); break; }
        int32_t t = S16(x + 6) - 1;
        *H(x + 6) = (uint16_t)t;
        if ((int16_t)t == 0) { call2(c, x, 0x81001001u, f_088D5680, 0x088DA7B4u); break; }
        if (flags & 1) {
            call1(c, *W(x + 12), f_088DFEF4, 0x088DA7D0u);
            int32_t r = (int32_t)c->r[2];
            if (r < 0) call2(c, x, (uint32_t)r, f_088D5680, 0x088DA7E4u);
            else if (r > 0) *H(x + 2) = (uint16_t)(*H(x + 2) & 2);
            flags = *H(x + 2);
        }
        if (!(flags & 2)) break;
        int32_t wait = S16(x + 4);
        if (wait != 0) { *H(x + 4) = (uint16_t)(wait - 1); break; }
        call1(c, *W(x + 16), f_088E02C0, 0x088DA828u);
        int32_t r = (int32_t)c->r[2];
        if (r < 0) call2(c, x, (uint32_t)r, f_088D5680, 0x088DA83Cu);
        else if (r > 0) *H(x + 2) = (uint16_t)(*H(x + 2) & 1);
        break;
    }
    case 3:
        *H(x + 4) = 0xB4;
        *H(x) = (uint16_t)(step + 1);
        break;
    case 4: {
        int32_t t = S16(x + 4) - 1;
        *H(x + 4) = (uint16_t)t;
        if ((int16_t)t == 0) *H(x) = (uint16_t)(step + 1);
        break;
    }
    case 5: {
        uint16_t n = *H(x + 24);
        *H(x + 2) = 0;
        *H(x + 24) = (uint16_t)(n + 1);
        *H(x) = 0;
        *B(x + 47) = 0;
        break;
    }
    default:
        call2(c, step, 0, f_088DF778, 0x088DA8B8u);
        *W(x + 20) = 0x08A3EB14u;
        *H(x + 24) = 0; *H(x + 2) = 0; *H(x) = 0;
        *B(x + 47) = 0;
        break;
    }
    ARG(1) = 2; ARG(2) = 0x30; ARG(3) = 4; c->r[8] = 9;
    call1(c, *W(0x08A3EA04u + (((uint32_t)*B(x + 42) + *B(x + 43)) << 2)), f_0880A6CC, 0x088DA908u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): a dive's flight: ticked and moved by hand (across by its facing, with its pulls).
   Down at the ground: step 4, crouching, the strength's pull from the table 0x08A49874 and the
   animation 0x32 + strength. Still up: the two checks for a hit on the way (a player's 0x088FA7FC
   and 0x088FA9F0, the cpu's 0x088FA730 and 0x088FA924). */
SFA_FUNC(088FA62C, dive_flight) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_08861848, 0x088FA640u);
    uint8_t facing = *B(o + 11);
    uint32_t x = *W(o + 16), vx = *W(o + 96);
    int32_t ground = S16(o + 132);
    uint32_t y = *W(o + 20), vy = *W(o + 100), ax = *W(o + 104);
    *W(o + 16) = facing != 0 ? x + vx : x - vx;
    *W(o + 20) = y + vy;
    *W(o + 96) = vx + ax;
    *W(o + 100) = vy + *W(o + 108);
    if (!(ground < S16(o + 22))) {
        *B(o + 7) = 4;
        int32_t strength = S8(o + 194);
        *B(o + 73) = 0;
        *H(o + 22) = (uint16_t)ground;
        *B(o + 696) = 1;
        *W(o + 104) = *W(0x08A49874u + (uint32_t)(strength << 2));
        *B(o + 235) = 1;
        call2(c, o, (uint32_t)(strength + 0x32) & 0xFFFF, f_088616EC, 0x088FA6E4u);
    } else if (S8(o + 373) == 0) {
        call1(c, o, f_088FA7FC, 0x088FA700u);
        call1(c, o, f_088FA9F0, 0x088FA708u);
    } else {
        call1(c, o, f_088FA730, 0x088FA718u);
        call1(c, o, f_088FA924, 0x088FA720u);
    }
    leave(c, sv);
}

/* (fighter): a leap's way down after 0x08912FEC: ticked while it rises. Falling, below the ground
   it lands: step 7, +262 cleared, 0x0887C614 and the animation 0xB. Else a player goes by
   0x08910830, and the cpu by the timed 0x089107E8, clearing the opponent's +696 when the opponent
   is not at a wall (+238). (The original also has a hit check here that it can never reach.) A
   frame keeping s0, s1. */
SFA_FUNC(089106D4, leap_down) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    gaddr other = *W(o + 92);
    c->r[17] = other;
    call0(c, f_08912FEC, 0x089106F0u);
    if (!(*W(o + 100) & 0x80000000u)) call1(c, o, f_08861848, 0x089107D4u);
    else {
        int32_t ground = S16(o + 132);
        if (S16(o + 22) < ground) {
            *B(o + 7) = 7;
            *W(o + 20) = (uint32_t)ground << 16;
            *B(o + 73) = 0;
            *B(o + 262) = 0;
            call1(c, o, f_0887C614, 0x08910734u);
            call2(c, o, 0xB, f_088616EC, 0x08910740u);
        } else if (S8(o + 373) == 0) call1(c, o, f_08910830, 0x0891075Cu);
        else if (S8(other + 238) != 0) call1(c, o, f_089107E8, 0x089107C4u);
        else {
            *B(other + 696) = 0;
            call1(c, o, f_089107E8, 0x089107B4u);
        }
    }
    leave2(c, sv);
}

/* (fighter): a move's repeat by strength: the light one is 0x089166B0. The others move on by hand
   until the count +0x338 is 2 and are ticked to the animation's end; then the count goes up: before
   3 step 1 with the animation 0x2E + strength; at 3 step 4 with the animation 0x3A for the middle
   one and 0x3B for the heavy, which without the opponent's +702 ends instead (0x0886469C). */
SFA_FUNC(0891675C, repeat_step_1675c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t strength = S8(o + 194);
    *W(c->r[29] + 16) = ra;
    gaddr other = *W(o + 92);
    c->r[5] = (uint32_t)strength; c->r[6] = other;
    if (strength == 0) call0(c, f_089166B0, 0x08916794u);
    else {
        gaddr p = o + 0x334;
        c->r[5] = p;
        if (S8(p + 4) != 2) {
            uint32_t x = *W(o + 16), v = *W(o + 96), a = *W(o + 104);
            *W(o + 16) = x + v;
            *W(o + 96) = v + a;
        }
        if (!(S8(o + 75) & 0x80)) call0(c, f_08861848, 0x089167D0u);
        else {
            int32_t n = S8(p + 4) + 1;
            *B(p + 4) = (uint8_t)n;
            strength = S8(o + 194);
            if ((int8_t)n != 3) {
                *B(o + 7) = 1;
                call2(c, o, (uint32_t)(strength + 0x2E) & 0xFFFF, f_088616EC, 0x0891680Cu);
            } else if (strength != 1 && S8(other + 702) == 0) call0(c, f_0886469C, 0x08916854u);
            else {
                *B(o + 7) = 4;
                call2(c, o, strength == 1 ? 0x3A : 0x3B, f_088616EC, 0x0891686Cu);
            }
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): an air move's player side (the cpu's is 0x089216B4): 0x08921988, then 0x08921940 once
   0x08921850 says so. Rising it is ticked; falling, a kick held or just pressed (0x08867BB0 0x2B)
   turns it to one of three others (0x089218B8 with 0, 1 or 2 for the light, middle and heavy
   kick), and without one it is ticked. A frame keeping s0, s1. */
SFA_FUNC(089215B8, air_move_player) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    if (S8(o + 373) != 0) { call1(c, o, f_089216B4, 0x089216A0u); leave2(c, sv); return; }
    call1(c, o, f_08921988, 0x089215DCu);
    call1(c, o, f_08921850, 0x089215E4u);
    if (c->r[2] != 0) call1(c, o, f_08921940, 0x08921690u);
    else {
        c->r[17] = 0;
        if (!((int32_t)*W(o + 100) < 0)) call1(c, o, f_08861848, 0x08921680u);
        else {
            call2(c, o, 0x2B, f_08867BB0, 0x08921604u);
            uint32_t held = *H(o + 752), pressed = c->r[2] & 0xFFFF;
            int which = -1;
            for (int k = 0; k < 3 && which < 0; k++) {
                c->r[17] = (uint32_t)k;
                if ((held & (0x10u << k)) || (pressed & (0x80u << k))) which = k;
            }
            if (which < 0) call1(c, o, f_08861848, 0x08921664u);
            else call2(c, o, (uint32_t)which, f_089218B8, 0x08921654u);
        }
    }
    leave2(c, sv);
}

/* (fighter): an entrance by who the opponent is. The character 0: step 7, +234 set, 0x13 frames,
   +100 from where it stands, the cue 0x33. The characters 2 and 0x24: step 2, flying in at 7.0
   from the far side of what 0x08821330 returns (0x30 before it facing right, 0x1B0 past it facing
   left), +108 set from it too, with the trail objects (0x0887CF24 with its +10), the cue 0x221 and
   the animation 0x32. Anyone else: the next step, turned round, the cue 0x33. */
SFA_FUNC(08961CFC, entrance_by_opponent) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    gaddr other = *W(o + 92);
    *B(o + 1) = 1;
    int32_t ch = S8(other + 338);
    if (ch == 0) {
        *B(o + 6) = 7;
        *B(o + 234) = 1;
        uint32_t at = *W(o + 16);
        *H(o + 82) = 0x13;
        *W(o + 100) = at;
        call2(c, o, 0x33, f_0886163C, 0x08961D50u);
    } else if (ch == 2 || ch == 0x24) {
        *B(o + 6) = 2;
        call1(c, o, f_08821330, 0x08961D78u);
        gaddr p = c->r[2];
        *W(o + 100) = *W(o + 16);
        int32_t off;
        if (*B(o + 11) != 0) {
            *W(o + 96) = 0x70000;
            *H(o + 18) = (uint16_t)(S16(p + 18) - 0x30);
            off = 0xA0;
        } else {
            *W(o + 96) = 0xFFF90000u;
            *H(o + 18) = (uint16_t)(S16(p + 18) + 0x1B0);
            off = 0xE0;
        }
        int32_t base = S16(p + 18);
        *B(o + 10) = 0;
        *W(o + 108) = (uint32_t)(base + off) << 16;
        c->r[6] = p;
        call2(c, o, o + 0xA, f_0887CF24, 0x08961DDCu);
        call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x221, f_0887C408, 0x08961DECu);
        call2(c, o, 0x32, f_088616EC, 0x08961DF8u);
    } else {
        uint8_t facing = *B(o + 11);
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(o + 11) = facing ^ 1;
        call2(c, o, 0x33, f_0886163C, 0x08961E20u);
    }
    leave(c, sv);
}
