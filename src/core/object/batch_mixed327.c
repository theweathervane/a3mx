/* More whose callees are all done: a partner's wait before coming back, three random opponents,
   a fighter's stored block at a round's start, a link peer's node freed, and two moves' steps.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_2 0x092E4338u

void f_08825E6C(Cpu *c); void f_0882F9D0(Cpu *c); void f_0883021C(Cpu *c); void f_088D5E44(Cpu *c);
void f_08838C88(Cpu *c); void f_088DCF0C(Cpu *c); void f_088DDB68(Cpu *c); void f_088DC2C4(Cpu *c);
void f_0890F2D4(Cpu *c); void f_0890F268(Cpu *c); void f_0886A66C(Cpu *c); void f_088616EC(Cpu *c);
void f_08921850(Cpu *c); void f_08922AC0(Cpu *c); void f_08821368(Cpu *c); void f_08922B08(Cpu *c);
void f_08861848(Cpu *c);

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

/* (x, y): the partners that are down (x's +107, a bit each). Both: the one whose wait (y + 16,
   + 18) is shorter comes back (0x08825E6C). One: with x's +109 clear it comes back at once, else
   its wait counts down first. None: nothing. */
SFA_FUNC(08825F50, partner_wait) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t down = S8(x + 107);
    *W(c->r[29] + 16) = ra;
    if (down == 3) call2(c, x, S16(y + 18) < S16(y + 16) ? 1 : 0, f_08825E6C, 0x08825F88u);
    else if (down != 0) {
        int32_t side = (int8_t)(down >> 1);
        if (S8(x + 109) == 0) call2(c, x, (uint32_t)side, f_08825E6C, 0x08825FB4u);
        else {
            gaddr p = y + (uint32_t)(side * 2);
            int32_t t = S16(p + 16) - 1;
            *H(p + 16) = (uint16_t)t;
            if ((int16_t)t < 0) call2(c, x, (uint32_t)side, f_08825E6C, 0x08825FECu);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* Three random opponents for the second side's list (0x09517A50, length 2): fighter 2 is the cpu
   (+373, +441), and 0x0882F9D0 fills the list's characters (+3, +4, +5) and kinds (+6, +7, +8),
   drawing the second again while it is the first (sixteen tries at most) and the third while it is
   either of the others; BATTLE + 100 is what 0x0883021C makes of the first. A 0x30 frame keeping
   s0..s5. */
SFA_FUNC(0882FB0C, random_three) {
    const gaddr list = 0x09517A50u, d = FIGHTER_2 + 0x1B6, e = FIGHTER_2 + 0x495;
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[2];
    *B(FIGHTER_2 + 373) = 1; *B(FIGHTER_2 + 441) = 1;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1];
    *B(list) = 2;
    c->r[17] = d; c->r[16] = list; c->r[18] = e;
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5]; *W(sp + 40) = sv[6];
    ARG(2) = d; ARG(3) = e;
    call2(c, list + 3, list + 6, f_0882F9D0, 0x0882FB6Cu);
    uint32_t tries = 0x10;
    c->r[19] = list + 4; c->r[20] = list + 7;
    do {
        c->r[21] = tries;
        ARG(2) = d; ARG(3) = e;
        call2(c, list + 4, list + 7, f_0882F9D0, 0x0882FB8Cu);
        tries = (tries - 1) & 0xFF;
    } while (tries != 0 && S8(list + 4) == S8(list + 3));
    tries = 0x10;
    c->r[20] = list + 5; c->r[21] = list + 8;
    do {
        c->r[19] = tries;
        ARG(2) = d; ARG(3) = e;
        call2(c, list + 5, list + 8, f_0882F9D0, 0x0882FBCCu);
        tries = (tries - 1) & 0xFF;
    } while (tries != 0 && (S8(list + 5) == S8(list + 3) || S8(list + 5) == S8(list + 4)));
    c->r[19] = tries;
    call1(c, (uint32_t)(int32_t)S8(list + 3), f_0883021C, 0x0882FBFCu);
    *H(BATTLE + 100) = (uint16_t)c->r[2];
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (fighter, y): a fighter's stored block at a round's start. With GAME + 137 set and the fighter's
   side not marked in GAME + 53: the choice 2 becomes 0 with +440 set and +371 = 2 (the character
   0xA also sets +439), and when 0x088D5E44 says so the stored block is started (0x08838C88 with
   y's +1); a marked side ends here. Then the fighter's 0x354 bytes at 0x09358528 (by side) are
   cleared, or copied from the set y's byte +18 (past its +1) picks out of those at 0x09519588. A
   frame keeping s0..s2. */
SFA_FUNC(08831924, stored_block_round_start) {
    gaddr f = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = f;
    *W(sp + 24) = s2;
    int32_t which = S8(y + 1);
    c->r[18] = (uint32_t)which;
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = y;
    if (S8(GAME + 137) != 0) {
        if ((uint32_t)S8(GAME + 53) & (1u << (S8(f + 337) & 31))) goto out;
        if (S8(f + 386) == 2) {
            *B(f + 386) = 0;
            *B(f + 440) = 1;
            int32_t ch = S8(f + 338);
            *B(f + 371) = 2;
            if (ch == 0xA) *B(f + 439) = 1;
        }
        call1(c, 1, f_088D5E44, 0x088319A8u);
        if (c->r[2] != 0) call2(c, f, (uint32_t)which, f_08838C88, 0x088319B8u);
    }
    {
        gaddr dst = 0x09358528u + (uint32_t)(S8(f + 337) * 0x354);
        int32_t set = S8(y + (uint32_t)which + 18);
        if (set == 0) {
            for (uint32_t i = 0; i < 0x354; i++) *B(dst + i) = 0;
        } else {
            gaddr src = 0x09519234u + (uint32_t)(set * 0x354);
            for (uint32_t i = 0; i < 0x354; i += 4) *W(dst + i) = *W(src + i);
        }
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (list, node, before): a link peer's node is taken out of its list (the first at +0, the last at
   +4, the count at +14), unless its +154 is set: the peer is removed by its address (0x088DCF0C
   on node + 4), its +148 freed (0x088DDB68) and the node itself (0x088DC2C4). Returns the node
   after the one that follows (or after the kept one), 0 at the end. A 0x30 frame keeping
   s0..s3. */
SFA_FUNC(088DC48C, link_node_free) {
    gaddr list = ARG(0), node = ARG(1), before = ARG(2);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    int32_t kept = S8(node + 154);
    *W(sp + 20) = sv[1]; c->r[17] = list;
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    c->r[16] = node;
    gaddr at = node;
    if (kept == 0) {
        gaddr next = *W(node);
        c->r[18] = next;
        if (node == *W(list)) *W(list) = next;
        if (node == *W(list + 4)) *W(list + 4) = before;
        c->r[19] = node + 4;
        if (before != 0) *W(before) = *W(node);
        call1(c, node + 4, f_088DCF0C, 0x088DC4E8u);
        call1(c, *W(node + 148), f_088DDB68, 0x088DC4F8u);
        *W(node + 148) = 0;
        call1(c, node, f_088DC2C4, 0x088DC504u);
        *H(list + 14) = (uint16_t)(S16(list + 14) - 1);
        at = next;
    }
    c->r[2] = at != 0 ? *W(at) : 0;
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (fighter): a roll's turn into something else, only with +0x334 = 7 at the heavy strength with
   the heavy kick held and +123 set: without down held 0x0890F268; with it and a choice below 0,
   step 3 and the animation 0x31 when 0x0886A66C (0x64) allows, else 0x0890F268. Everything else
   rolls on (0x0890F2D4). */
SFA_FUNC(0890F16C, roll_turn) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 0x334) != 7) call1(c, o, f_0890F2D4, 0x0890F194u);
    else if (S8(o + 194) != 2) call1(c, o, f_0890F2D4, 0x0890F1B4u);
    else if (!(*H(o + 760) & 0x200)) call1(c, o, f_0890F2D4, 0x0890F1D0u);
    else if (S8(o + 123) == 0) call1(c, o, f_0890F2D4, 0x0890F1E8u);
    else if (!(*H(o + 290) & 4)) call1(c, o, f_0890F268, 0x0890F204u);
    else if (!(S8(o + 386) & 0x80)) call1(c, o, f_0890F2D4, 0x0890F258u);
    else {
        call2(c, o, 0x64, f_0886A66C, 0x0890F220u);
        if (c->r[2] != 0) {
            *B(o + 7) = 3;
            call2(c, o, 0x31, f_088616EC, 0x0890F248u);
        } else call1(c, o, f_0890F268, 0x0890F230u);
    }
    leave(c, sv);
}

/* (object): 0x08922AC0 once 0x08921850 says so. Else near the top of its flight (less than 5 up
   or down a frame), one time in sixteen it leaps back from the opponent (0x08922B08 with 0 to 2;
   a fourth draw does nothing); ticked either way. */
SFA_FUNC(089229C0, object_9229c0) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_08921850, 0x089229D4u);
    if (c->r[2] != 0) call1(c, o, f_08922AC0, 0x08922A58u);
    else {
        int32_t vy = (int32_t)*W(o + 100);
        int32_t whole = S16(o + 102);
        if (vy < 0) whole = (int16_t)-whole;
        if ((int32_t)((uint32_t)whole & 0xFFFF) < 5) {
            call1(c, 1, f_08821368, 0x08922A14u);
            if ((c->r[2] & 0xF) == 0) {
                call1(c, 0, f_08821368, 0x08922A28u);
                uint32_t v = c->r[2] & 3;
                if (v != 3) call2(c, o, v, f_08922B08, 0x08922A40u);
            }
        }
        call1(c, o, f_08861848, 0x08922A48u);
    }
    leave(c, sv);
}
