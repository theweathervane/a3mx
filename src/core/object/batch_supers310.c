/* Moves' starts behind their checks: nineteen super moves of one shape, and a few near it. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886C2B4(Cpu *c); void f_0886DD68(Cpu *c);
void f_089208A4(Cpu *c); void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_08828C04(Cpu *c);
void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c);
void f_0886D678(Cpu *c); void f_08863114(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (fighter): a super move's start, never with +634 set. On the ground both 0x0886C9F0 and
   0x0886C7E4 must allow, and it faces as +697 says; in the air +251 must be clear and 0x0886C2B4
   allow. Then the move from state 1 / 0 / 8 / 0 and 0x0886DD68. Returns whether. `back`: the
   return addresses of the four calls in that order. */
static void super_start(Cpu *c, uint8_t move, const uint32_t back[4]) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok = S8(o + 634) == 0;
    if (ok) {
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, back[0]);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, back[1]);
                ok = c->r[2] != 0;
            }
            if (ok) *B(o + 11) = (uint8_t)S8(o + 697);
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, back[3]);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        *B(o + 236) = move;
        call1(c, o, f_0886DD68, back[2]);
    }
    c->r[2] = (uint32_t)ok;
    leave(c, sv);
}
SFA_FUNC(088EEC60, super_start_eec60) {
    static const uint32_t back[4] = { 0x088EEC8Cu, 0x088EEC9Cu, 0x088EECD4u, 0x088EECF8u };
    super_start(c, 8, back);
}
SFA_FUNC(088F2B94, super_start_f2b94) {
    static const uint32_t back[4] = { 0x088F2BC0u, 0x088F2BD0u, 0x088F2C08u, 0x088F2C2Cu };
    super_start(c, 9, back);
}
SFA_FUNC(088F7404, super_start_f7404) {
    static const uint32_t back[4] = { 0x088F7430u, 0x088F7440u, 0x088F7478u, 0x088F749Cu };
    super_start(c, 0xF, back);
}
SFA_FUNC(08902260, super_start_02260) {
    static const uint32_t back[4] = { 0x0890228Cu, 0x0890229Cu, 0x089022D4u, 0x089022F8u };
    super_start(c, 0xC, back);
}
SFA_FUNC(08906770, super_start_06770) {
    static const uint32_t back[4] = { 0x0890679Cu, 0x089067ACu, 0x089067E4u, 0x08906808u };
    super_start(c, 9, back);
}
SFA_FUNC(0890FD3C, super_start_0fd3c) {
    static const uint32_t back[4] = { 0x0890FD68u, 0x0890FD78u, 0x0890FDB0u, 0x0890FDD4u };
    super_start(c, 8, back);
}
SFA_FUNC(08925D8C, super_start_25d8c) {
    static const uint32_t back[4] = { 0x08925DB8u, 0x08925DC8u, 0x08925E00u, 0x08925E24u };
    super_start(c, 0xA, back);
}
SFA_FUNC(0892A7BC, super_start_2a7bc) {
    static const uint32_t back[4] = { 0x0892A854u, 0x0892A864u, 0x0892A824u, 0x0892A7F4u };
    super_start(c, 8, back);
}
SFA_FUNC(08962C20, super_start_62c20) {
    static const uint32_t back[4] = { 0x08962C4Cu, 0x08962C5Cu, 0x08962CB0u, 0x08962C80u };
    super_start(c, 8, back);
}
SFA_FUNC(089672EC, super_start_672ec) {
    static const uint32_t back[4] = { 0x08967318u, 0x08967328u, 0x0896735Cu, 0x08967380u };
    super_start(c, 0, back);
}
SFA_FUNC(089734F0, super_start_734f0) {
    static const uint32_t back[4] = { 0x0897351Cu, 0x0897352Cu, 0x08973560u, 0x08973584u };
    super_start(c, 0, back);
}
SFA_FUNC(0897BCC0, super_start_7bcc0) {
    static const uint32_t back[4] = { 0x0897BCECu, 0x0897BCFCu, 0x0897BD34u, 0x0897BD58u };
    super_start(c, 9, back);
}
SFA_FUNC(0898A5EC, super_start_8a5ec) {
    static const uint32_t back[4] = { 0x0898A618u, 0x0898A628u, 0x0898A65Cu, 0x0898A680u };
    super_start(c, 0, back);
}
SFA_FUNC(0899EC38, super_start_9ec38) {
    static const uint32_t back[4] = { 0x0899EC64u, 0x0899EC74u, 0x0899ECA8u, 0x0899ECCCu };
    super_start(c, 0, back);
}
SFA_FUNC(089A30F0, super_start_a30f0) {
    static const uint32_t back[4] = { 0x089A311Cu, 0x089A312Cu, 0x089A3160u, 0x089A3184u };
    super_start(c, 0, back);
}
SFA_FUNC(089A5EA0, super_start_a5ea0) {
    static const uint32_t back[4] = { 0x089A5ECCu, 0x089A5EDCu, 0x089A5F2Cu, 0x089A5F00u };
    super_start(c, 0, back);
}

/* (fighter): never with +634 set. In the air what 0x089208A4 says. On the ground, when both
   0x0886C9F0 and 0x0886C7E4 allow: the move 0xA from state 1 / 0 / 8 / 0, facing as +697 says, and
   0x0886DD68. Returns whether. */
SFA_FUNC(089207F4, super_a_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 634) != 0) c->r[2] = 0;
    else if (S8(o + 73) != 0) call1(c, o, f_089208A4, 0x08920894u);
    else {
        call1(c, o, f_0886C9F0, 0x08920820u);
        int ok = c->r[2] != 0;
        if (ok) {
            call1(c, o, f_0886C7E4, 0x08920830u);
            ok = c->r[2] != 0;
        }
        if (ok) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 236) = 0xA;
            *B(o + 11) = (uint8_t)facing;
            c->r[5] = (uint32_t)(int32_t)facing;
            call1(c, o, f_0886DD68, 0x08920864u);
        }
        c->r[2] = (uint32_t)ok;
    }
    leave(c, sv);
}

/* (fighter): not for a choice below 0, nor with +542 set while +251 is clear. When 0x0886BF84
   allows: the move from state 1 / 0 / 7 / 0, facing as +697 says, and 0x0886DB94. Returns
   whether. */
static void move_unless_542(Cpu *c, uint8_t move, uint32_t back0, uint32_t back1) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[17] = 0; c->r[16] = o;
    if (S8(o + 386) < 0 || (S8(o + 251) == 0 && S8(o + 542) != 0)) { c->r[2] = 0; leave2(c, sv); return; }
    call1(c, o, f_0886BF84, back0);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = move;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)facing;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_0886DB94, back1);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}
SFA_FUNC(088F2CEC, move6_unless_542) { move_unless_542(c, 6, 0x088F2D2Cu, 0x088F2D84u); }
SFA_FUNC(088F7530, move13_unless_542) { move_unless_542(c, 0xD, 0x088F7570u, 0x088F75C8u); }

/* (fighter, count, b): the count goes to +0x339; when 0x0886BF84 allows, the move 1 / 1 from state
   1 / 0 / 7 / 0 with b at +193, facing as +697 says, at the strength of the count (4 at most).
   Returns whether. A frame keeping s0..s2. */
SFA_FUNC(08914D38, move1_counted) {
    gaddr o = ARG(0);
    uint32_t count = ARG(1), b = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 24) = s2; *W(sp + 20) = s1; *W(sp + 28) = ra;
    c->r[16] = o; c->r[17] = o + 0x334;
    c->r[18] = (uint32_t)(int32_t)(int16_t)b;
    *B(o + 0x334 + 5) = (uint8_t)count;
    c->r[31] = 0x08914D64u; f_0886BF84(c);
    if (c->r[2] == 0) c->r[2] = 0;
    else {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = 1;
        *B(o + 193) = (uint8_t)b;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        int32_t n = S8(o + 0x334 + 5);
        *B(o + 194) = (uint8_t)(n < 5 ? n : 4);
        c->r[2] = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with +251 or +254 set and 0x0886BF84 allowing: the move 8 / 0 from state 1 / 0 / 7 /
   0, facing as +697 says, +196..+199 cleared, and 0x08828C04. Returns 0 when it started, else 1. */
SFA_FUNC(08935768, move8_or_not) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok = S8(o + 251) != 0 || S8(o + 254) != 0;
    if (ok) {
        call1(c, o, f_0886BF84, 0x08935794u);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 236) = 8;
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        *B(o + 235) = 0;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_08828C04, 0x089357DCu);
    }
    c->r[2] = (uint32_t)!ok;
    leave(c, sv);
}

/* (fighter): for the choice 0 with +536 clear, a third of a meter (0x30) and 0x0886C6EC allowing:
   the move 0xD / 1 from state 1 / 0 / 8 / 0, facing as +697 says, and 0x0886D2EC. Returns
   whether. */
SFA_FUNC(0893D508, super_d_start) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = o;
    int ok = S8(o + 386) == 0 && *B(o + 536) == 0 && !((int32_t)*H(o + 366) < 48);
    if (ok) {
        call1(c, o, f_0886C6EC, 0x0893D54Cu);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = 0xD;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_0886D2EC, 0x0893D588u);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}

/* The same shape for the move 1, in a frame that keeps s1 as well and leaves 1 in it. */
static void super_start_1(Cpu *c, const uint32_t back[4]) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = o;
    int ok = S8(o + 634) == 0;
    if (ok) {
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, back[0]);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, back[1]);
                ok = c->r[2] != 0;
            }
            if (ok) *B(o + 11) = (uint8_t)S8(o + 697);
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, back[3]);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8;
        c->r[17] = 1;
        *B(o + 7) = 0; *B(o + 236) = 1;
        call1(c, o, f_0886DD68, back[2]);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}
SFA_FUNC(0895C594, super_start_5c594) {
    static const uint32_t back[4] = { 0x0895C5C4u, 0x0895C5D4u, 0x0895C60Cu, 0x0895C634u };
    super_start_1(c, back);
}
SFA_FUNC(0898EA04, super_start_8ea04) {
    static const uint32_t back[4] = { 0x0898EA34u, 0x0898EA44u, 0x0898EA7Cu, 0x0898EAA4u };
    super_start_1(c, back);
}
SFA_FUNC(089941D0, super_start_941d0) {
    static const uint32_t back[4] = { 0x08994200u, 0x08994210u, 0x08994248u, 0x08994270u };
    super_start_1(c, back);
}

/* (fighter): step 2 of a move: +0x3BB = -1 and 0x0886CA2C with the strength. Further away than the
   word from 0x08A74370 by strength, +0x3BB is cleared and the animation is 0x3A + strength (the
   strength alone for the choice -1); nearer, 0x18 + strength. A frame keeping s0..s2. */
SFA_FUNC(0893BE84, move_far_or_near_3be84) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2;
    *B(o + 7) = 2;
    c->r[18] = 0xFFFFFFFFu; c->r[17] = o + 0x3B8;
    *B(o + 0x3B8 + 3) = 0xFF;
    *W(sp + 28) = ra;
    ARG(1) = (uint32_t)(int32_t)S8(o + 194);
    call1(c, o, f_0886CA2C, 0x0893BEBCu);
    int32_t strength = S8(o + 194);
    uint32_t anim, back;
    if ((int32_t)*H(0x08A74370u + (uint32_t)(strength * 2)) < (int32_t)*H(o + 508)) {
        *B(o + 0x3B8 + 3) = 0;
        int32_t choice = S8(o + 386);
        strength = S8(o + 194);
        if (choice == -1) { anim = (uint32_t)strength & 0xFFFF; back = 0x0893BF1Cu; }
        else { anim = (uint32_t)(strength + 0x3A) & 0xFFFF; back = 0x0893BF08u; }
    } else { anim = (uint32_t)(strength + 0x18) & 0xFFFF; back = 0x0893BF30u; }
    ARG(1) = anim;
    call1(c, o, f_08861674, back);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with +251 or +254 set: on the ground, when 0x0886BF84 allows, the move 0xA; in the
   air, not on the way down and when 0x0886C2B4 allows, the move 0xB. From state 1 / 0 / 7 / 0,
   facing as +697 says, then 0x08828C04. */
SFA_FUNC(0893D0E8, move_a_or_air_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 251) != 0 || S8(o + 254) != 0) {
        int air = S8(o + 73) != 0;
        int ok = 1;
        if (!air) call1(c, o, f_0886BF84, 0x0893D120u);
        else if ((int32_t)*W(o + 100) < 0) ok = 0;
        else call1(c, o, f_0886C2B4, 0x0893D16Cu);
        if (ok && c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 236) = air ? 0xB : 0xA;
            *B(o + 11) = (uint8_t)facing;
            ARG(1) = (uint32_t)(int32_t)facing;
            call1(c, o, f_08828C04, air ? 0x0893D1A0u : 0x0893D154u);
        }
    }
    leave(c, sv);
}

/* (fighter): a crouching move's two steps (+696 set). First: facing as +697 says, +235 set, +123
   cleared when +193 is set, 0x08863114, 0x0886CA2C with the strength, and the animation by
   strength from 0xC, or with +193 set from 0xF (0x1D past the first strength with +290's bit 0).
   Afterwards 0x0886D678. A frame keeping s0, s1. */
SFA_FUNC(08942D54, crouch_move_42d54) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = o;
    uint8_t step = *B(o + 7);
    *B(o + 696) = 1;
    if (step != 0) call1(c, o, f_0886D678, 0x08942D80u);
    else {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 1;
        *B(o + 11) = (uint8_t)facing;
        int32_t alt = S8(o + 193);
        *B(o + 235) = 1;
        if (alt != 0) *B(o + 123) = 0;
        uint32_t strength = (uint32_t)S8(o + 194) & 0xFF;
        c->r[17] = strength;
        call1(c, o, f_08863114, 0x08942DB8u);
        ARG(1) = strength;
        call1(c, o, f_0886CA2C, 0x08942DC4u);
        uint32_t base = 0xC;
        if (S8(o + 193) != 0) {
            base = 0xF;
            if (strength != 0 && (*H(o + 290) & 1)) base = 0x1D;
        }
        ARG(1) = (base + strength) & 0xFFFF;
        call1(c, o, f_08861674, 0x08942DFCu);
    }
    leave2(c, sv);
}
