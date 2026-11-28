/* Middle-sized ones whose callees are all done: the two-on-one fight's first partner and its
   round's end, who leads on vitality, a list of opponents, a record's comparison, a link peer's
   removal, two follower objects, a leap's start and a few moves' starts. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define PAIR      0x095179E0u /* the two-on-one fight's own state */
#define PEERS     0x09523820u /* the link's peers: the list at +232, the count at +236 */

void f_08824AA0(Cpu *c); void f_08861724(Cpu *c); void f_08830334(Cpu *c); void f_08830348(Cpu *c);
void f_089F33E4(Cpu *c); void f_088E0B04(Cpu *c); void f_088DF6D4(Cpu *c); void f_088DDB68(Cpu *c);
void f_088DC2C4(Cpu *c); void f_088DD040(Cpu *c); void f_0882196C(Cpu *c); void f_088219B0(Cpu *c);
void f_0887C408(Cpu *c); void f_0886C380(Cpu *c); void f_088616EC(Cpu *c); void f_0886D678(Cpu *c);
void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886DD68(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (x, y): the two-on-one fight's partners at a round's start. With PAIR + 15 clear both are marked
   (+413), y's +107 and the two waits are cleared, and 0x08824AA0 runs. With it set: the first time
   (x + 8) only fighter 4 is marked and fighter 3 hidden, with y's +15 and x's +72, +43 and +125
   cleared; afterwards fighter 3 starts down and out (state 1 / 6 / 0 / 0xFF, 0x100 up, vitality
   -1, the animation 0x1B), the first wait is 0x3C, y's +107 is 1, and 0x08824AA0 runs. A frame
   that keeps x, y and PAIR across the call. */
SFA_FUNC(08824AC8, partners_round_start) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[9] = PAIR; c->r[8] = 1; c->r[7] = FIGHTER_3;
    *W(sp + 28) = ra;
    if (S8(PAIR + 15) == 0) {
        *B(FIGHTER_3 + 413) = 1; *B(FIGHTER_4 + 413) = 1;
        c->r[6] = FIGHTER_4;
        *B(y + 107) = 0;
        *H(PAIR + 18) = 0; *H(PAIR + 16) = 0;
        call0(c, f_08824AA0, 0x08824B0Cu);
    } else if (*H(x + 8) == 0) {
        *B(y + 15) = 0;
        uint16_t n = *H(x + 8);
        *B(x + 72) = 0; *B(x + 43) = 0; *B(x + 125) = 0;
        *H(x + 8) = (uint16_t)(n + 1);
        *B(FIGHTER_3 + 413) = 0; *B(FIGHTER_4 + 413) = 1;
        *B(FIGHTER_3 + 1) = 0;
    } else {
        const gaddr f = FIGHTER_3;
        *W(sp + 24) = PAIR; *W(sp + 20) = y; *W(sp + 16) = x;
        *B(f + 4) = 1; *B(f + 5) = 6; *B(f + 6) = 0; *B(f + 7) = 0xFF;
        *B(f + 599) = 1;
        *B(f + 562) = 0; *B(f + 560) = 0; *B(f + 561) = 0; *B(f + 126) = 0;
        *B(f + 579) = 1; *B(f + 671) = 1; *B(f + 591) = 0;
        *B(f + 772) = 8;
        *H(f + 22) = 0x100;
        *H(f + 112) = 0xFFFF;
        c->r[6] = f;
        call2(c, f, 0x1B, f_08861724, 0x08824BCCu);
        *H(*W(sp + 24) + 16) = 0x3C;
        gaddr y2 = *W(sp + 20);
        *B(y2 + 107) = 1;
        call2(c, *W(sp + 16), y2, f_08824AA0, 0x08824BECu);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b, f, g, h): a round's end for the winner f: its wins (+426) go up to 999 at most and
   0x08830348 takes what 0x08830334 makes of g's character. By the mode (b + 79): 7 keeps f's
   vitality and meter (+420, +422), does the same with h's character and adds PAIR + 4 to the
   count at PAIR; 8 keeps them and counts one. 4 and 5 keep them for f and h and copy the wins to
   fighters 1 and 2; then, as in every other mode, when b's +20 has reached +72, a's +72 and b's
   +70 become -1. A 0x30 frame keeping s0..s3. */
SFA_FUNC(08825FF8, round_end_winner) {
    gaddr a = ARG(0), b = ARG(1), f = ARG(2), g = ARG(3), h = c->r[8];
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[2]; c->r[18] = b;
    int32_t wins = S16(f + 426);
    *W(sp + 28) = sv[3]; c->r[19] = a;
    *W(sp + 20) = sv[1]; c->r[17] = f;
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4];
    c->r[16] = h;
    c->r[5] = (uint32_t)wins;
    if (wins < 999) { c->r[5] = (uint32_t)(wins + 1); *H(f + 426) = (uint16_t)(wins + 1); }
    call1(c, (uint32_t)(int32_t)S8(g + 338), f_08830334, 0x08826040u);
    call2(c, f, c->r[2], f_08830348, 0x0882604Cu);
    int32_t mode = S8(b + 79);
    int tail = 1;
    if (mode == 7 || mode == 8 || mode == 4 || mode == 5) {
        *H(f + 420) = (uint16_t)S16(f + 112);
        *H(f + 422) = (uint16_t)S16(f + 366);
    }
    if (mode == 7) {
        call1(c, (uint32_t)(int32_t)S8(h + 338), f_08830334, 0x08826074u);
        call2(c, f, c->r[2], f_08830348, 0x08826080u);
        *W(PAIR) = *W(PAIR) + *W(PAIR + 4);
        tail = 0;
    } else if (mode == 8) {
        *W(PAIR) = *W(PAIR) + 1;
        tail = 0;
    } else if (mode == 4 || mode == 5) {
        *H(h + 420) = (uint16_t)S16(h + 112);
        *H(h + 422) = (uint16_t)S16(h + 366);
        int32_t n = S16(f + 426);
        *H(FIGHTER_1 + 426) = (uint16_t)n;
        *H(FIGHTER_2 + 426) = (uint16_t)n;
    }
    if (tail && S16(b + 20) == S16(b + 72)) { *B(a + 72) = 0xFF; *B(b + 70) = 0xFF; }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (-, y): who leads on vitality, into y's +12 (1 the left side, 2 the right, 3 level), by the kind
   of fight (y + 56): 0 fighter 1 against 2; 1 twice fighter 1 against 3 and 4 together; 2 twice
   fighter 2 against 3 and 4; 3 fighters 1 and 2 against twice fighter 3. Below 0 it is 1, from 4
   on 3. */
SFA_FUNC(08828200, vitality_lead) {
    gaddr y = ARG(1);
    int32_t kind = S16(y + 56);
    int32_t hp1 = S16(FIGHTER_1 + 112), hp2 = S16(FIGHTER_2 + 112);
    int32_t hp3 = S16(FIGHTER_3 + 112), hp4 = S16(FIGHTER_4 + 112);
    int32_t left, right;
    if (kind < 0) { left = 1; right = kind; }
    else if (kind == 0) { left = hp1; right = hp2; }
    else if (kind == 1) { left = hp1 << 1; right = hp3 + hp4; }
    else if (kind == 2) { left = hp2 << 1; right = hp3 + hp4; }
    else if (kind == 3) { left = hp1 + hp2; right = hp3 << 1; }
    else { left = 0; right = 0; }
    *B(y + 12) = left == right ? 3 : right < left ? 1 : 2;
}

/* (-, y, choices): the list of opponents for the mode (y + 79, from 1): how many more than one
   from the bytes at 0x08A0F22E (kept at y + 72), and who from the bytes at 0x08A0F23E past the
   mode's word at 0x08A0F234. Each goes to 0x09517988 and its choice to 0x095179B0: 0 for those
   the bytes at 0x08A0F2D2 mark, else the next of the given choices, where -1 turns the characters
   4 and 6 into 0x12 and 0x14. Whoever follows the character 0x29 is also kept at GAME + 192. */
SFA_FUNC(0882EBB0, opponents_list) {
    gaddr y = ARG(1), choices = ARG(2);
    int32_t mode = (int16_t)(S8(y + 79) - 1);
    int32_t more = S8(0x08A0F22Eu + (uint32_t)mode);
    gaddr list = 0x08A0F23Eu + (uint32_t)(int32_t)S16(0x08A0F234u + (uint32_t)(mode * 2));
    *H(y + 72) = (uint16_t)more;
    const gaddr who = 0x09517988u, how = 0x095179B0u;
    if (0 < more + 1) { c->r[2] = who; c->r[3] = 6; }
    for (int32_t i = 0; i < more + 1; i = (int16_t)(i + 1)) {
        uint32_t ch = *B(list++);
        uint32_t choice = 0;
        if (S8(0x08A0F2D2u + ch) == 0) {
            choice = *B(choices++);
            if (choice == 0xFF) {
                if (ch == 4) ch = 0x12;
                if (ch == 6) ch = 0x14;
            }
        }
        *B(who + (uint32_t)i) = (uint8_t)ch;
        *B(how + (uint32_t)i) = (uint8_t)choice;
        if (i > 0 && *B(who + (uint32_t)i - 1) == 0x29) *B(GAME + 192) = *B(who + (uint32_t)i);
    }
}

/* (a, record): does a's result keep its place against the record? A record that is not set
   (+12) loses when a's +472 is set. Else by the word at a + 478 against the record's +10 (less
   wins outright, more loses), then by the three bytes +469..+471 against +13..+15 the other way,
   then by a's +452 against the record's first word. Returns 0 when a beats it, else 1. */
SFA_FUNC(088934BC, record_keeps_place) {
    gaddr a = ARG(0), rec = ARG(1);
    if (S8(a + 472) != 0 && S8(rec + 12) == 0) { c->r[2] = 1; return; }
    int32_t a1 = *H(a + 478), b1 = *H(rec + 10);
    if (a1 < b1) { c->r[2] = 0; return; }
    if (b1 < a1) { c->r[2] = 1; return; }
    int32_t a2 = (int32_t)(((uint32_t)S8(a + 469) << 16) | ((uint32_t)S8(a + 470) << 8) | (uint32_t)S8(a + 471));
    int32_t b2 = (int32_t)(((uint32_t)S8(rec + 13) << 16) | ((uint32_t)S8(rec + 14) << 8) | (uint32_t)S8(rec + 15));
    if (b2 < a2) { c->r[2] = 0; return; }
    if (a2 < b2) { c->r[2] = 1; return; }
    c->r[2] = (int32_t)*W(a + 452) < (int32_t)*W(rec) ? 0 : 1;
}

/* (address): the link peer with this six-byte address is taken off the list (unless PEERS + 240
   is set): unlinked, its two parts closed (0x088E0B04 on +0xAC, 0x088DF6D4 on +0x98), its +148
   freed (0x088DDB68) and itself (0x088DC2C4), 0x088DD040 when the link play's +44 is 1, and the
   count one down. Returns the count. A 0x30 frame keeping s0..s4. */
SFA_FUNC(088DCF0C, link_peer_remove) {
    gaddr address = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[4]; c->r[20] = PEERS;
    *W(sp + 28) = sv[3];
    gaddr at = *W(PEERS + 232), before = 0;
    c->r[19] = at; c->r[18] = 0;
    *W(sp + 24) = sv[2]; *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 36) = sv[5];
    c->r[16] = address;
    if (S8(PEERS + 240) == 0) {
        while (at != 0) {
            c->r[17] = at + 4;
            call3(c, address, at + 4, 6, f_089F33E4, 0x088DCF84u);
            gaddr next = *W(at);
            if (c->r[2] == 0) {
                if (before == 0) *W(PEERS + 232) = next;
                else *W(before) = next;
                call1(c, at + 0xAC, f_088E0B04, 0x088DCFA4u);
                call1(c, at + 0x98, f_088DF6D4, 0x088DCFACu);
                call1(c, *W(at + 148), f_088DDB68, 0x088DCFBCu);
                *W(at + 148) = 0;
                call1(c, at, f_088DC2C4, 0x088DCFC8u);
                if (*B(0x09523980u + 44) == 1) call2(c, 1, 1, f_088DD040, 0x088DCFE8u);
                *W(PEERS + 236) = *W(PEERS + 236) - 1;
                break;
            }
            before = at; at = next;
            c->r[18] = before; c->r[19] = at;
        }
    }
    c->r[2] = *W(PEERS + 236);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}

/* (object, owner): a piece that follows its owner. Without a live owner its state moves on; while
   the owner's frame has no +3 it is hidden with +78 cleared. Else it is shown at the owner's
   height, facing the same way, the word from `offsets` (by its +10) in front; a new +3 is kept at
   +78 and starts the animation from the table `anims` by the byte at `picks` (twice the +3 plus
   +10); the same one is ticked. Two characters have one each. */
typedef struct { uint32_t offsets, picks, anims, back_tick, back_start; } Follower;

static void follower_piece(Cpu *c, const Follower *t) {
    gaddr o = ARG(0), own = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 1) = 0;
    uint8_t live = *B(own);
    *W(c->r[29] + 16) = ra;
    if (live == 0) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    else if (S8(*W(own + 28) + 3) == 0) *B(o + 78) = 0;
    else {
        uint8_t facing = *B(own + 11);
        uint32_t k = *B(o + 10);
        *B(o + 11) = facing;
        int32_t dx = S16(t->offsets + k * 2);
        if (facing != 0) dx = (int16_t)-dx;
        *H(o + 18) = (uint16_t)(S16(own + 18) + dx);
        int16_t y = (int16_t)S16(own + 22);
        *B(o + 1) = 1;
        *H(o + 22) = (uint16_t)y;
        int32_t v = S8(*W(own + 28) + 3);
        c->r[6] = (uint32_t)v;
        if (v == S8(o + 78)) call1(c, o, f_088219B0, t->back_tick);
        else {
            *B(o + 78) = (uint8_t)v;
            int32_t idx = S8(t->picks + (uint32_t)(S8(*W(own + 28) + 3) << 1) + k);
            uint32_t anim = *W(t->anims + (uint32_t)(idx << 2));
            c->r[6] = anim;
            call2(c, o, anim, f_0882196C, t->back_start);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(088F1938, follower_piece_f1938) {
    static const Follower t = { 0x08A43C70u, 0x08A43C74u, 0x08A3F9E0u, 0x088F1A48u, 0x088F1A34u };
    follower_piece(c, &t);
}
SFA_FUNC(089661D4, follower_piece_661d4) {
    static const Follower t = { 0x08A8AAD4u, 0x08A8AAD8u, 0x08A861B0u, 0x089662E4u, 0x089662D0u };
    follower_piece(c, &t);
}

/* (fighter): a leap's start: state 1 / 0 / 3 / 1, the cue 0x20F, 6.0 up with gravity 0.28. Across:
   1.5 back with down held, 1.5 forward with back held, else nothing, turned by its facing; then
   4.0 more one way or the other by where it stands against the object at 0x092E88A0: within 96
   past it 4.0 on, more than 0x120 past it 4.0 off, and between by the way it was already going
   (or its facing when still). Going off, the pull +104 is 0x500 instead of -0x500. +78 is -1, or
   an odd value made of the facing and the speed. */
SFA_FUNC(08901688, leap_start_1688) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3;
    int32_t side = S8(o + 337);
    *B(o + 7) = 1;
    *B(o + 235) = 0;
    *W(c->r[29] + 20) = ra;
    call3(c, o, (uint32_t)side, 0x20F, f_0887C408, 0x089016C4u);
    *W(o + 100) = 0x60000;
    *W(o + 108) = 0xFFFFB800u;
    uint32_t stick = *H(o + 290);
    uint32_t facing = *B(o + 11);
    uint32_t vx = (stick & 2) ? 0xFFFE8000u : (stick & 1) ? 0x18000u : 0;
    uint32_t pull = 0xFFFFFB00u;
    if (facing != 0) vx = 0 - vx;
    int32_t d = (int16_t)(S16(o + 18) - S16(0x092E88A0u + 18));
    int on;
    if (d < 96) on = 1;
    else if (0x120 < d) on = 0;
    else {
        int32_t was = (int32_t)*W(o + 96);
        on = was > 0 || (was == 0 && facing != 0);
    }
    if (on) vx += 0x40000;
    else {
        if (!(d < 96)) pull = 0x500;
        vx += 0xFFFC0000u;
    }
    *W(o + 96) = vx;
    *W(o + 104) = pull;
    uint32_t mark = facing ^ ((vx << 1) | ((vx & 0x8000) != 0));
    if ((mark & 1) == 0) mark = 0xFFFFFFFFu;
    *B(o + 78) = (uint8_t)mark;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): when 0x0886C380 allows, a grab from state 1 / 0 / 7 / 0, standing, facing as +697
   says, its +127 cleared: for the choice 0 the move 6 / 1 with the opponent's +127 = 0x12, +573 =
   0x16 and the animation 5; for the others the move 4 / 1 with 0x11, 0x15 and the animation 6.
   Returns whether. A frame keeping s0..s2. */
SFA_FUNC(089103A8, grab_start_103a8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    gaddr other = *W(o + 92);
    c->r[18] = other;
    call0(c, f_0886C380, 0x089103C8u);
    if (c->r[2] == 0) c->r[2] = 0;
    else {
        int plain = S8(o + 386) == 0;
        int8_t facing = (int8_t)S8(o + 697);
        c->r[17] = 1; c->r[6] = 1;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = plain ? 6 : 4;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        *B(o + 696) = 0;
        *B(o + 127) = 0;
        *B(other + 127) = plain ? 0x12 : 0x11;
        *B(o + 573) = plain ? 0x16 : 0x15;
        if (plain) call2(c, o, 5, f_088616EC, 0x089104A8u);
        else call2(c, o, 6, f_088616EC, 0x0891042Cu);
        c->r[2] = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a move's two steps. First: facing as +697 says, +235 set, 0x08863114, 0x0886CA2C with
   the strength. Past the first strength with back or forward held: the animation 0x1E for the
   middle one, 0x1F with +310 set for the heavy. Else 6 + twice the strength, one more when far:
   for a player with the choice 1 with the stick let go, for the others further than the word from
   0x08A8B790 by strength. Afterwards 0x0886D678. A frame keeping s0, s1. */
SFA_FUNC(08966EF8, move_near_or_far_66ef8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    uint32_t step = *B(o + 7);
    c->r[17] = step;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = o;
    if (step != 0) call1(c, o, f_0886D678, 0x08967010u);
    else {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 1;
        c->r[17] = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        call1(c, o, f_08863114, 0x08966F34u);
        call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08966F40u);
        int32_t strength = S8(o + 194);
        if (strength != 0 && (*H(o + 290) & 3) != 0) {
            if (strength == 1) call2(c, o, 0x1E, f_08861674, 0x08966F6Cu);
            else {
                *B(o + 310) = 1;
                call2(c, o, 0x1F, f_08861674, 0x08966F84u);
            }
        } else {
            uint32_t anim = (uint32_t)((strength << 1) + 6) & 0xFFFF;
            if (S8(o + 373) == 0 && S8(o + 386) == 1) {
                if (*H(o + 290) == 0) anim = (anim + 1) & 0xFFFF;
                call2(c, o, anim, f_08861674, 0x08966FC4u);
            } else {
                if (S16(0x08A8B790u + (uint32_t)(strength * 2)) < S16(o + 508)) anim = (anim + 1) & 0xFFFF;
                call2(c, o, anim, f_08861674, 0x08967000u);
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a super move's start needing half a meter (0x48): never with +634 set; in the air +251
   must be clear and 0x0886C2B4 allow, on the ground both 0x0886C9F0 and 0x0886C7E4. Then the move
   8 from state 1 / 0 / 8 / 0, facing as +697 says, and 0x0886DD68. Returns whether. */
SFA_FUNC(089B08D8, super_start_b08d8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int air = 0;
    int ok = S8(o + 634) == 0 && !((int32_t)*H(o + 366) < 72);
    if (ok) {
        air = S8(o + 73) != 0;
        if (air) {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, 0x089B0924u);
                ok = c->r[2] != 0;
            }
        } else {
            call1(c, o, f_0886C9F0, 0x089B0974u);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, 0x089B0984u);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = 8;
        *B(o + 11) = (uint8_t)facing;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_0886DD68, air ? 0x089B0958u : 0x089B09B8u);
    }
    c->r[2] = (uint32_t)ok;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
