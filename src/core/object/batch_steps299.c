/* More steps the newly written leaves set free: a new entry for the link's list, a move's step with
   a dive, a fourth guard move, a sixth piece that follows its fighter, and the mark that sits beside
   a fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_088DC18C(Cpu *c); void f_089F3470(Cpu *c); void f_088DE4A0(Cpu *c); void f_088F6AC0(Cpu *c);
void f_088F6C4C(Cpu *c); void f_088F6EBC(Cpu *c); void f_08864888(Cpu *c); void f_08861848(Cpu *c);
void f_0886A1E4(Cpu *c); void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DD68(Cpu *c); void f_08928BBC(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c);
void f_088A0588(Cpu *c); void f_088A0104(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
/* A 0x20 frame with only the return address, at +16. */
static uint32_t enter0(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave0(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void pass(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }

/* (list, after, record): a new entry (0xE8 bytes from 0x088DC18C, cleared) for the link's list: the
   record's first eight bytes at +4, its name (128 bytes from +8) at +12, or the name the system has for
   that address when the record has none; hung after `after` (its +0), or as the list's first (+232);
   the list's count (+236) up: the entry, or 0 with no memory. A 0x30 frame keeping s0..s3. */
SFA_FUNC(088DCC54, net_entry_add) {
    gaddr head = ARG(0), after = ARG(1), rec = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2;
    c->r[16] = head; c->r[18] = rec; c->r[17] = after;
    *W(sp + 28) = s3; *W(sp + 32) = ra;
    call1(c, 0xE8, f_088DC18C, 0x088DCC80u);
    gaddr n = c->r[2];
    c->r[19] = n;
    if (n != 0) {
        c->r[6] = 0xE8;
        call2(c, n, 0, f_089F3470, 0x088DCC98u);
        *W(n + 228) = 0;
        uint32_t a = *W(rec), b = *W(rec + 4);
        *W(n + 4) = a; *W(n + 8) = b;
        c->r[18] = rec + 8;
        if (S8(rec + 8) == 0) { c->r[6] = 0; call2(c, n + 4, n + 0xC, f_088DE4A0, 0x088DCCC8u); }
        else {
            for (uint32_t i = 0; i < 0x80; i += 8) {
                uint32_t lo = *W(rec + 8 + i), hi = *W(rec + 12 + i);
                *W(n + 0xC + i) = lo; *W(n + 0x10 + i) = hi;
            }
        }
        if (after != 0) *W(after) = n;
        else *W(head + 232) = n;
        *W(head + 236) = *W(head + 236) + 1;
    }
    c->r[2] = n;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): a move's step by +192: 0 the steps 0x088F6AC0 (0x088F6C4C with +193), +696 cleared; 1
   the step 0x088F6EBC, +696 set; else the dive: started at step 0 (5.75 down, 4.0 across by its
   facing), moved by hand each frame, and at the ground (+132) onto it (0x08864888), ticked before. */
SFA_FUNC(088F69D0, move_step_or_dive) {
    gaddr o = ARG(0);
    int32_t k = S8(o + 192);
    uint32_t ra = enter0(c);
    c->r[6] = (uint32_t)k;
    if (k == 0) {
        int32_t second = S8(o + 193);
        c->r[5] = (uint32_t)second;
        *B(o + 696) = 0;
        if (second != 0) pass(c, f_088F6C4C, 0x088F6A0Cu);
        else pass(c, f_088F6AC0, 0x088F69F8u);
    } else if (k == 1) {
        c->r[5] = 1;
        *B(o + 696) = 1;
        pass(c, f_088F6EBC, 0x088F6A2Cu);
    } else {
        uint32_t step = *B(o + 7);
        int32_t ground = S16(o + 132);
        if (step == 0) {
            *B(o + 7) = 1;
            *W(o + 100) = 0xFFFB4000u;
            *W(o + 96) = *B(o + 11) != 0 ? 0x40000u : 0xFFFC0000u;
        }
        uint32_t y = *W(o + 20), x = *W(o + 16), vy = *W(o + 100), vx = *W(o + 96);
        *W(o + 20) = y + vy; *W(o + 16) = x + vx;
        c->r[5] = (uint32_t)ground;
        if (ground < S16(o + 22)) pass(c, f_08861848, 0x088F6AB4u);
        else { *H(o + 22) = (uint16_t)ground; pass(c, f_08864888, 0x088F6AA0u); }
    }
    leave0(c, ra);
}

/* (fighter): a character's guard move 11, as 0x089C6E10, but not from step 5 (+6) with +193 and the
   strength both 0: on its command (0x0886A1E4), with +634 clear: on the ground from outside state 1 /
   0 / 5 when the move out of a guard is allowed, facing the opponent; in the air with +251 clear when
   0x0886C2B4 allows it: state 1 / 0 / 8 / 0 and the timings by the buttons: 1; else 0. */
SFA_FUNC(089C09B8, guard_move_11_not_5) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x089C09CCu; f_0886A1E4(c);
    uint32_t done = 0;
    if (c->r[2] != 0 && S8(o + 634) == 0 && !(*B(o + 6) == 5 && S8(o + 193) == 0 && S8(o + 194) == 0)) {
        int ok = 0;
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, 0x089C0A1Cu);
            if (c->r[2] != 0) {
                call1(c, o, f_0886C7E4, 0x089C0A2Cu);
                if (c->r[2] != 0) { *B(o + 11) = (uint8_t)S8(o + 697); ok = 1; }
            }
        } else if (S8(o + 251) == 0) {
            call1(c, o, f_0886C2B4, 0x089C0A50u);
            ok = c->r[2] != 0;
        }
        if (ok) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 0xB;
            call1(c, o, f_0886DD68, 0x089C0A80u);
            done = 1;
        }
    }
    c->r[2] = done;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (piece, fighter): a piece that follows its fighter, as the five in batch_steps298: by pixel, 0x30
   in front of it for picture 16; +78 keeps the number; the table 0x08A679B8. */
SFA_FUNC(08928AB0, piece_follow_679b8) {
    gaddr x = ARG(0), own = ARG(1);
    *B(x + 1) = 0;
    int32_t who = S8(own + 338);
    uint32_t mine = *B(x + 3);
    uint32_t ra = enter0(c);
    if ((uint32_t)who != mine) { c->r[6] = own; call2(c, x, own, f_08928BBC, 0x08928AE4u); }
    else if (*B(*W(own + 296) + 10) == 0) *B(x + 78) = 0;
    else {
        *H(x + 18) = (uint16_t)S16(own + 18);
        *H(x + 22) = (uint16_t)S16(own + 22);
        uint32_t facing = *B(own + 11);
        if (*B(*W(own + 296) + 10) == 0x10) {
            *H(x + 18) = (uint16_t)(S16(x + 18) + (facing == 0 ? 0x30 : -0x30));
            facing = *B(own + 11);
        }
        *B(x + 11) = (uint8_t)facing; *B(x + 1) = 1;
        uint32_t k = *B(*W(own + 296) + 10);
        int32_t had = S8(x + 78);
        if ((uint32_t)had == k) call1(c, x, f_088219B0, 0x08928BB0u);
        else {
            *B(x + 78) = (uint8_t)k;
            uint32_t now = *B(*W(own + 296) + 10);
            call2(c, x, *W(0x08A679B8u + (now << 2)), f_0882196C, 0x08928B9Cu);
        }
    }
    leave0(c, ra);
}

/* (mark, fighter): the mark beside a fighter: hidden (0x088A0588) while the fighter is not in use or
   its +76 / +77 pair is 0; else shown, restarted (0x088A0104, with a 16-frame shake) when the pair is
   not what it shows (+36); placed at the fighter's height, shaking up and down while +82 runs, 0x30
   to one side of it: by GAME + 53 in modes 15 and 8, else by the opponent's side (+337). A frame
   keeping s0..s2. */
SFA_FUNC(0889FFF4, side_mark_place) {
    gaddr x = ARG(0), f = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t pair = (uint32_t)((S8(f + 76) << 8) + S8(f + 77)) & 0xFFFF;
    *W(c->r[29] + 20) = s1; c->r[17] = x;
    uint8_t used = *B(f + 1);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = f;
    if (used == 0 || pair == 0) call1(c, x, f_088A0588, 0x088A003Cu);
    else {
        uint32_t shown = *H(x + 36);
        *B(x + 1) = 1;
        c->r[18] = GAME;
        if (shown != pair) {
            c->r[6] = pair;
            *H(x + 82) = 0x10;
            call2(c, x, f, f_088A0104, 0x088A0070u);
        }
        int32_t shake = S16(x + 82);
        if (shake != 0) {
            *H(x + 82) = (uint16_t)(shake - 1);
            if (shake & 1) shake = -shake;
            shake >>= 1;
        }
        *H(x + 22) = (uint16_t)(shake + S16(f + 22));
        gaddr other = *W(f + 92);
        int32_t mode = S8(GAME + 114), d = -0x30;
        if (mode == 0xF || mode == 8) { if (!(S8(GAME + 53) & 1)) d = 0x30; }
        else if (S8(other + 337) != 0) d = 0x30;
        *H(x + 18) = (uint16_t)(d + S16(f + 18));
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
