/* More of what the small ones set free: a round's start by the kind of fight, the link's incoming
   messages, two characters' follower pieces, and moves' starts. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088249A0(Cpu *c); void f_088D5E44(Cpu *c); void f_08825130(Cpu *c); void f_08824AC8(Cpu *c);
void f_088E0698(Cpu *c); void f_088E428C(Cpu *c); void f_088DC2C4(Cpu *c); void f_088F1860(Cpu *c);
void f_088F1938(Cpu *c); void f_089660FC(Cpu *c); void f_089661D4(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c); void f_0890EED0(Cpu *c); void f_0890EDD0(Cpu *c); void f_0890EE10(Cpu *c);
void f_08861674(Cpu *c); void f_0893BE84(Cpu *c); void f_0893C254(Cpu *c); void f_08837098(Cpu *c);
void f_0897B4FC(Cpu *c); void f_088616EC(Cpu *c); void f_0899311C(Cpu *c); void f_08999428(Cpu *c);
void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c); void f_0898EFC4(Cpu *c); void f_08994834(Cpu *c);
void hle_sceKernelWaitSema(Cpu *c); void hle_sceKernelSignalSema(Cpu *c);

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

/* (x, y): a round's start by the kind of fight. With x's +38 = 1, 0x088249A0. Else, unless x's
   +39 is set: in the first mode (y + 79 = 0), or when 0x088D5E44 says so and the word at
   0x09517A78 + 940 is set, the kind's own start from the table 0x08A0E1B0 (by y's +59); otherwise
   the two-on-one starts: 0x08825130 for the kind 7, 0x08824AC8 for the others. A frame keeping
   s0, s1. */
SFA_FUNC(088249CC, round_start_by_kind) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    enter2(c, y, sv);
    c->r[17] = x;
    if (S8(x + 38) == 1) call2(c, x, y, f_088249A0, 0x088249FCu);
    else if (S8(x + 39) == 0) {
        call1(c, 0, f_088D5E44, 0x08824A18u);
        int32_t kind = S8(y + 59);
        if ((c->r[2] != 0 && *W(0x09517A78u + 940) != 0) || S8(y + 79) == 0) {
            uint32_t fn = *W(0x08A0E1B0u + (uint32_t)(kind << 2));
            ARG(0) = x; ARG(1) = y; c->r[6] = fn; c->r[31] = 0x08824A60u;
            rt_call_indirect(c, fn);
        } else if (kind == 7) call2(c, x, y, f_08825130, 0x08824A7Cu);
        else call2(c, x, y, f_08824AC8, 0x08824A8Cu);
    }
    leave2(c, sv);
}

/* The link's incoming messages: while the link is up (0x095236C0 + 14), nothing is pending (its
   first word) and the queue at 0x095236EC has something (+8), the next message is taken off under
   the lock at 0x08A3EB88 (0x088E0698), passed on (0x088E428C with its +4, +8 and +12) and freed.
   A 0x30 frame keeping s0..s3. */
SFA_FUNC(088E17B0, link_incoming) {
    const gaddr link = 0x095236C0u, queue = 0x095236ECu, lock = 0x08A3EB88u;
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3]; c->r[19] = link;
    int32_t up = S8(link + 14);
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4];
    if (up != 0) {
        gaddr m = *W(link);
        c->r[16] = m;
        if (m == 0) {
            c->r[18] = queue; c->r[17] = 0x08A40000u;
            while (m == 0 && *W(queue + 8) != 0) {
                uint32_t sema = *W(lock);
                if (sema != 0) { call3(c, sema, 1, 0, hle_sceKernelWaitSema, 0x088E181Cu); }
                call1(c, queue, f_088E0698, 0x088E1824u);
                sema = *W(lock);
                m = c->r[2];
                c->r[16] = m;
                if (sema != 0) call2(c, sema, 1, hle_sceKernelSignalSema, 0x088E1838u);
                if (m == 0) break;
                call3(c, *W(m + 4), m + 8, m + 0xC, f_088E428C, 0x088E1850u);
                call1(c, m, f_088DC2C4, 0x088E1858u);
                m = *W(link);
                c->r[16] = m;
            }
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (piece, owner): a follower piece's start: its state moves on and it takes the owner's +24, +12,
   +13 and +14, with +78 cleared (one of the two characters also copies the side and +164 here).
   By the piece's +2: 0 belongs to the fighter itself: its side at +86, its +164 and its character
   at +3, then `first`; 0x1F belongs to another piece: that one's owner's side and +164, that one's
   +3, then the follower's step. Others: nothing more. */
static void follower_start(Cpu *c, int early, void (*first)(Cpu *), uint32_t back_first, void (*step)(Cpu *),
                           uint32_t back_step) {
    gaddr o = ARG(0), own = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 24) = *H(own + 24);
    *B(o + 12) = (uint8_t)S8(own + 12);
    *B(o + 13) = *B(own + 13);
    uint8_t carried = *B(own + 14);
    *B(o + 78) = 0;
    *B(o + 14) = carried;
    if (early) {
        *H(o + 86) = (uint16_t)S8(own + 337);
        *W(o + 164) = *W(own + 164);
    }
    uint32_t k = *B(o + 2);
    *W(c->r[29] + 16) = ra;
    if (k == 0) {
        *H(o + 86) = (uint16_t)S8(own + 337);
        *W(o + 164) = *W(own + 164);
        int32_t ch = S8(own + 338);
        *B(o + 3) = (uint8_t)ch;
        c->r[6] = o; c->r[7] = (uint32_t)ch;
        call2(c, o, own, first, back_first);
    } else if (k == 0x1F) {
        gaddr top = *W(own + 88);
        *H(o + 86) = (uint16_t)S8(top + 337);
        *W(o + 164) = *W(top + 164);
        uint8_t sub = *B(own + 3);
        *B(o + 3) = sub;
        c->r[6] = o; c->r[7] = sub;
        call2(c, o, own, step, back_step);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(088F1790, follower_start_f1790) {
    follower_start(c, 0, f_088F1860, 0x088F180Cu, f_088F1938, 0x088F1854u);
}
SFA_FUNC(08966014, follower_start_66014) {
    follower_start(c, 1, f_089660FC, 0x089660A8u, f_089661D4, 0x089660F0u);
}

/* (fighter): the next step of a move: facing as +697 says, +235 set, +123 cleared, 0x08863114,
   0x0886CA2C with the strength. The light one: 0x0890EED0. With +567 set: 0x0890EDD0. The middle
   one with down held: step 2 with the animation 0x1E. Otherwise 0x0890EE10. A frame keeping s0,
   s1. */
SFA_FUNC(0890ECFC, move_start_0ecfc) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    uint8_t step = *B(o + 7);
    int8_t facing = (int8_t)S8(o + 697);
    *B(o + 7) = (uint8_t)(step + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)facing;
    *B(o + 235) = 1;
    *B(o + 123) = 0;
    call1(c, o, f_08863114, 0x0890ED38u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0890ED44u);
    int32_t strength = S8(o + 194);
    if (strength == 0) call1(c, o, f_0890EED0, 0x0890ED58u);
    else if (S8(o + 567) != 0) call1(c, o, f_0890EDD0, 0x0890ED70u);
    else if (strength != 1) call1(c, o, f_0890EE10, 0x0890ED88u);
    else if (!(*H(o + 290) & 2)) call1(c, o, f_0890EE10, 0x0890EDA4u);
    else {
        *B(o + 7) = 2;
        call2(c, o, 0x1E, f_08861674, 0x0890EDBCu);
    }
    leave2(c, sv);
}

/* (fighter): a move's start: facing as +697 says, +235 set, 0x08863114. The cpu and the choice -1
   go by distance (`by_distance`). A player otherwise picks: the next step with +0x3BB cleared and
   0x0886CA2C with the strength; then without back held the far animation (`far` + strength), and
   with it one more step, +0x3BB = -1 and the near one (`near` + strength). A frame keeping
   s0..s2. */
static void move_far_or_near_pick(Cpu *c, void (*by_distance)(Cpu *), uint32_t far, uint32_t near,
                                  const uint32_t back[5]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *B(o + 11) = (uint8_t)S8(o + 697);
    *B(o + 235) = 1;
    *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    call1(c, o, f_08863114, back[0]);
    int pick = S8(o + 373) == 0;
    if (pick) {
        c->r[18] = 0xFFFFFFFFu;
        pick = S8(o + 386) != -1;
    }
    if (!pick) call1(c, o, by_distance, back[4]);
    else {
        c->r[17] = o + 0x3B8;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 0x3B8 + 3) = 0;
        call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, back[1]);
        if (!(*H(o + 290) & 1)) call2(c, o, (uint32_t)(S8(o + 194) + (int32_t)far) & 0xFFFF, f_08861674, back[2]);
        else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 0x3B8 + 3) = 0xFF;
            call2(c, o, (uint32_t)(S8(o + 194) + (int32_t)near) & 0xFFFF, f_08861674, back[3]);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(0893C060, move_pick_3c060) {
    static const uint32_t back[5] = { 0x0893C090u, 0x0893C0CCu, 0x0893C0F0u, 0x0893C118u, 0x0893C128u };
    move_far_or_near_pick(c, f_0893BE84, 0x3A, 0x18, back);
}
SFA_FUNC(0893C174, move_pick_3c174) {
    static const uint32_t back[5] = { 0x0893C1A4u, 0x0893C1E0u, 0x0893C204u, 0x0893C22Cu, 0x0893C23Cu };
    move_far_or_near_pick(c, f_0893C254, 0x3D, 0x46, back);
}

/* (fighter): 0x0886CA2C with the strength + 3 and 0x08837098; then, standing with +262 set: with
   +0x338's top bit step 4, an object (0x0897B4FC 0x11, 0x55, 8, the same three kept at 0x08B03FCC)
   and the animation 0x1C; else the next step with 4 frames and the animation 0x15 + strength. */
SFA_FUNC(0897CFB4, throw_object_or_wait) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call2(c, o, (uint32_t)(S8(o + 194) + 3), f_0886CA2C, 0x0897CFD0u);
    call1(c, o, f_08837098, 0x0897CFD8u);
    if (S8(o + 0x334 + 4) & 0x80) {
        *B(o + 7) = 4;
        *B(o + 262) = 1;
        *B(o + 696) = 0;
        *W(0x08B03FCCu) = 0x11; *W(0x08B03FD0u) = 0x55; *W(0x08B03FD4u) = 8;
        ARG(3) = 8;
        call3(c, o, 0x11, 0x55, f_0897B4FC, 0x0897D034u);
        call2(c, o, 0x1C, f_088616EC, 0x0897D040u);
    } else {
        uint8_t step = *B(o + 7);
        int32_t strength = S8(o + 194);
        *B(o + 7) = (uint8_t)(step + 1);
        *B(o + 262) = 1;
        *B(o + 696) = 0;
        *H(o + 82) = 4;
        call2(c, o, (uint32_t)(strength + 0x15) & 0xFFFF, f_088616EC, 0x0897D078u);
    }
    leave(c, sv);
}

/* (fighter): for the choice 0 with a third of a meter (0x30), when `near` and 0x0886C6EC allow:
   the move 0x11 / 1 from state 1 / 0 / 8 / 0, facing as +697 says, +196..+199 cleared, 0x0886D2EC
   and the owner's throw (`thrown`). Returns whether. A frame keeping s0, s1. */
static void super_throw(Cpu *c, void (*near)(Cpu *), void (*thrown)(Cpu *), const uint32_t back[4]) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int ok = S8(o + 386) == 0 && !(S16(o + 366) < 48);
    if (ok) {
        call1(c, o, near, back[0]);
        ok = c->r[2] != 0;
    }
    if (ok) {
        call1(c, o, f_0886C6EC, back[1]);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        *B(o + 236) = 0x11;
        c->r[17] = 1;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0; *B(o + 196) = 0;
        call1(c, o, f_0886D2EC, back[2]);
        call1(c, o, thrown, back[3]);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}
SFA_FUNC(0898EEFC, super_throw_8eefc) {
    static const uint32_t back[4] = { 0x0898EF30u, 0x0898EF40u, 0x0898EF8Cu, 0x0898EF94u };
    super_throw(c, f_0899311C, f_0898EFC4, back);
}
SFA_FUNC(0899476C, super_throw_9476c) {
    static const uint32_t back[4] = { 0x089947A0u, 0x089947B0u, 0x089947FCu, 0x08994804u };
    super_throw(c, f_08999428, f_08994834, back);
}
