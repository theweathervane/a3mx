/* More small ones whose callees are all done: the next opponents out of a list, an object's steps,
   and more moves' starts and steps. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_08880B1C(Cpu *c); void f_08880A30(Cpu *c); void f_0887DF88(Cpu *c); void f_088219B0(Cpu *c);
void f_088215C4(Cpu *c); void f_0886B058(Cpu *c); void f_088FA880(Cpu *c); void f_088FA8F0(Cpu *c);
void f_088FAA74(Cpu *c); void f_088FAAE8(Cpu *c); void f_0886C2B4(Cpu *c); void f_0886C9F0(Cpu *c);
void f_0886C7E4(Cpu *c); void f_0886DD68(Cpu *c); void f_0886C380(Cpu *c); void f_088FDA00(Cpu *c);
void f_0899B32C(Cpu *c); void f_088616EC(Cpu *c); void f_088632C4(Cpu *c); void f_0886DC3C(Cpu *c);
void f_08837098(Cpu *c); void f_08877258(Cpu *c); void f_08861848(Cpu *c); void f_08925138(Cpu *c);
void f_089253E8(Cpu *c); void f_08822EAC(Cpu *c); void f_0886163C(Cpu *c); void f_08861674(Cpu *c);
void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_0886D678(Cpu *c); void f_0886C8F4(Cpu *c);
void f_0886D3E0(Cpu *c); void f_0886BF84(Cpu *c); void f_0897C42C(Cpu *c);

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
/* The same keeping s0..s2, ra at +28. */
static void enter3(Cpu *c, gaddr o, uint32_t sv[4]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[18]; sv[3] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2]; *W(c->r[29] + 28) = sv[3];
    c->r[16] = o;
}
static void leave3(Cpu *c, const uint32_t sv[4]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[31] = sv[3]; c->r[29] += 0x20;
}

/* (x, y): the next entry of the opponents' list (y + 20 goes up): the choice from the bytes at
   0x095179B0 and the characters from the four-byte entries at 0x095179F8. With y's +56 clear it is
   one opponent, the first fighter of the side x's +53 says (2 means the first side); else the
   two partners, fighters 3 and 4. Returns y's +56. */
SFA_FUNC(0882FFE4, next_opponent) {
    gaddr x = ARG(0), y = ARG(1);
    int32_t both = S16(y + 56);
    *H(y + 20) = (uint16_t)(S16(y + 20) + 1);
    uint32_t idx = (uint32_t)(int32_t)S16(y + 20);
    uint8_t choice = *B(0x095179B0u + idx);
    gaddr entry = 0x095179F8u + (idx << 2);
    if (both == 0) {
        gaddr f = S8(x + 53) == 2 ? FIGHTER_1 : FIGHTER_2;
        *B(f + 386) = choice;
        *B(f + 338) = (uint8_t)S8(entry + 2);
    } else {
        *B(FIGHTER_4 + 386) = choice; *B(FIGHTER_3 + 386) = choice;
        *B(FIGHTER_3 + 338) = (uint8_t)S8(entry + 2);
        *B(FIGHTER_4 + 338) = (uint8_t)S8(entry + 3);
    }
    c->r[2] = (uint32_t)both;
}

/* (object): state 2 while BATTLE + 42 is set. Else by +5: 0 starts (0x08880B1C with 0, 0x08880A30);
   1 runs 0x08880A30 and 0x088219B0, with the cue 0x33 once its owner is on the ground (then 2);
   2 is ticked (0x088219B0) to the animation's end, then state 2. Then 0x088215C4. */
SFA_FUNC(08880938, object_880938) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(BATTLE + 42) != 0) { *B(o + 4) = 2; leave(c, sv); return; }
    uint32_t k = *B(o + 5);
    if (k == 0) {
        *B(o + 5) = 1;
        call2(c, o, 0, f_08880B1C, 0x08880984u);
        call1(c, o, f_08880A30, 0x0888098Cu);
    } else if (k == 1) {
        if (S8(*W(o + 88) + 73) == 0) {
            *B(o + 5) = 2;
            call2(c, o, 0x33, f_0887DF88, 0x088809BCu);
        }
        call1(c, o, f_08880A30, 0x088809C4u);
        call1(c, o, f_088219B0, 0x088809CCu);
    } else if (S8(o + 75) < 0) *B(o + 4) = 2;
    else call1(c, o, f_088219B0, 0x088809ECu);
    call1(c, o, f_088215C4, 0x088809F4u);
    leave(c, sv);
}

/* (fighter): on the way down, for the one of the two that +193 picks: within 25 of an opponent who
   is not at a wall (+238) the check 0x0886B058 can turn it into `hit`; otherwise the time +82 runs
   down and at 0 `end` runs. */
static void dive_check(Cpu *c, int want193, uint32_t check, void (*hit)(Cpu *), void (*end)(Cpu *),
                       uint32_t back_check, uint32_t back_hit, uint32_t back_end) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if ((int32_t)*W(o + 100) < 0 && (S8(o + 193) != 0) == want193) {
        gaddr other = *W(o + 92);
        if (S8(other + 238) == 0) {
            int32_t d = (int16_t)(S16(o + 18) - S16(other + 18));
            if (d < 0) d = (int16_t)-d;
            if (!(d < 25)) { leave(c, sv); return; }
            call2(c, o, check, f_0886B058, back_check);
            if (c->r[2] != 0) { call1(c, o, hit, back_hit); leave(c, sv); return; }
        }
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) call1(c, o, end, back_end);
    }
    leave(c, sv);
}
SFA_FUNC(088FA730, dive_check_fa730) {
    dive_check(c, 0, 0x5E, f_088FA880, f_088FA8F0, 0x088FA7A0u, 0x088FA7D4u, 0x088FA7ECu);
}
SFA_FUNC(088FA924, dive_check_fa924) {
    dive_check(c, 1, 0x5F, f_088FAA74, f_088FAAE8, 0x088FA994u, 0x088FA9C8u, 0x088FA9E0u);
}

/* (fighter): a super move's start in a frame that keeps s1 for the answer: never with +634 set; on
   the ground both 0x0886C9F0 and 0x0886C7E4 must allow, and it faces as +697 says; in the air +251
   must be clear and 0x0886C2B4 allow. Then the move from state 1 / 0 / 8 / 0 and 0x0886DD68.
   Returns whether. `back`: the three checks' return addresses, then 0x0886DD68's on the ground and
   in the air. */
static void super_start_s1(Cpu *c, uint8_t move, int early_s1, const uint32_t back[5]) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    if (early_s1) c->r[17] = 0;
    int air = 0;
    int ok = S8(o + 634) == 0;
    if (ok) {
        air = S8(o + 73) != 0;
        if (!air) {
            call1(c, o, f_0886C9F0, back[0]);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, back[1]);
                ok = c->r[2] != 0;
            }
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, back[2]);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        *B(o + 236) = move;
        c->r[17] = 1;
        if (!air) {
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 11) = (uint8_t)facing;
            c->r[5] = (uint32_t)(int32_t)facing;
        }
        if (!early_s1) c->r[5] = 1;
        call1(c, o, f_0886DD68, back[air ? 4 : 3]);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}
SFA_FUNC(088FD370, super_start_fd370) {
    static const uint32_t back[5] = { 0x088FD3FCu, 0x088FD40Cu, 0x088FD3ACu, 0x088FD3DCu, 0x088FD3DCu };
    super_start_s1(c, 8, 0, back);
}
SFA_FUNC(08909DD4, super_start_09dd4) {
    static const uint32_t back[5] = { 0x08909E08u, 0x08909E18u, 0x08909E68u, 0x08909E50u, 0x08909E98u };
    super_start_s1(c, 9, 1, back);
}
SFA_FUNC(0891931C, super_start_1931c) {
    static const uint32_t back[5] = { 0x08919350u, 0x08919360u, 0x089193B0u, 0x08919398u, 0x089193E0u };
    super_start_s1(c, 9, 1, back);
}
SFA_FUNC(0898431C, super_start_8431c) {
    static const uint32_t back[5] = { 0x08984350u, 0x08984360u, 0x089843B0u, 0x08984398u, 0x089843E0u };
    super_start_s1(c, 8, 1, back);
}

/* (fighter): when 0x0886C380 allows: for the choice 0 a move / 1 from state 1 / 0 / 7 / 0, facing
   as +697 says, standing (+696), its +127 cleared and the opponent's set, +573 and +777 set, with
   the animation 5; for the others `other`. Returns whether. */
static void grab_start(Cpu *c, uint8_t move, uint8_t v573, uint8_t their127, void (*other)(Cpu *),
                       uint32_t back0, uint32_t back_other, uint32_t back_anim) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    call1(c, o, f_0886C380, back0);
    if (c->r[2] == 0) c->r[2] = 0;
    else {
        int32_t choice = S8(o + 386);
        c->r[17] = 1;
        if (choice != 0) call1(c, o, other, back_other);
        else {
            *B(o + 4) = 1; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 5) = 0;
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 236) = move;
            *B(o + 11) = (uint8_t)facing;
            *B(o + 235) = 1;
            *B(o + 696) = 0;
            *B(o + 127) = 0;
            *B(o + 573) = v573;
            *B(o + 777) = 1;
            *B(*W(o + 92) + 127) = their127;
            call2(c, o, 5, f_088616EC, back_anim);
        }
        c->r[2] = 1;
    }
    leave2(c, sv);
}
SFA_FUNC(088FD81C, grab_start_fd81c) {
    grab_start(c, 5, 0x1D, 0x19, f_088FDA00, 0x088FD834u, 0x088FD850u, 0x088FD8D8u);
}
SFA_FUNC(0899B1D0, grab_start_9b1d0) {
    grab_start(c, 2, 0x1C, 0x18, f_0899B32C, 0x0899B1E8u, 0x0899B204u, 0x0899B28Cu);
}

/* (fighter): a held move's step. While the fight is held (BATTLE + 13, + 14, GAME + 38) it goes
   straight on: the next step, +262 cleared, the animation 0xA + twice the strength. Else, unless
   0x088632C4 ends it: the opponent's +592 becomes its +270; on the animation's cue (+76 with +74 =
   1) 0x0886DC3C and 0x08837098 run and +551 becomes -1 if it was 0; then with +193 the time +82
   runs down, and without it 0x08877258 (0x08A512D4) says, when it goes on; until then ticked. */
SFA_FUNC(089028AC, held_move_step) {
    gaddr o = ARG(0);
    uint32_t held = (uint32_t)(S8(BATTLE + 13) | S8(BATTLE + 14) | S8(GAME + 38));
    uint32_t sv[2];
    enter(c, o, sv);
    int go = held != 0;
    if (!go) {
        call1(c, o, f_088632C4, 0x089028F0u);
        if (c->r[2] != 0) { leave(c, sv); return; }
        gaddr other = *W(o + 92);
        if (S8(other + 592) != 0) { *B(other + 270) = 1; *B(other + 592) = 0; }
        if (S8(o + 76) != 0 && S8(o + 74) == 1) {
            *B(o + 76) = 0;
            call1(c, o, f_0886DC3C, 0x08902938u);
            call1(c, o, f_08837098, 0x08902940u);
            if (S8(o + 551) == 0) *B(o + 551) = 0xFF;
            if (S8(o + 193) != 0) {
                int32_t t = S16(o + 82) - 1;
                *H(o + 82) = (uint16_t)t;
                go = (int16_t)t == 0;
            } else {
                call2(c, o, 0x08A512D4u, f_08877258, 0x08902990u);
                go = c->r[2] != 0;
            }
        }
        if (!go) call1(c, o, f_08861848, 0x089029A0u);
    }
    if (go) {
        uint8_t step = *B(o + 7);
        int32_t strength = S8(o + 194);
        *B(o + 7) = (uint8_t)(step + 1);
        *B(o + 262) = 0;
        call2(c, o, (uint32_t)((strength << 1) + 0xA) & 0xFFFF, f_088616EC, 0x089029CCu);
    }
    leave(c, sv);
}

/* (fighter): by the choice: -1 0x08925138, 0 0x089253E8; the others go to step 4, 0x50 further
   toward the middle from their side, +656 = 0x15, with two objects of their own (0x08822EAC 0x18 /
   0 and / 1) unless the byte at 0x095176F3 is below 2, and the cue 0x30. */
SFA_FUNC(08924C28, entrance_by_choice) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int32_t choice = S8(o + 386);
    if (choice == -1) call1(c, o, f_08925138, 0x08924C50u);
    else if (choice == 0) call1(c, o, f_089253E8, 0x08924C68u);
    else {
        *B(o + 7) = 4;
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(x + (S8(o + 337) == 0 ? 0x50 : -0x50));
        *B(o + 656) = 0x15;
        if (!(S8(0x095176F3u) < 2)) {
            call2(c, 0x18, 0, f_08822EAC, 0x08924CB8u);
            if (c->r[2] != 0) {
                *W(c->r[2] + 88) = o;
                call2(c, 0x18, 1, f_08822EAC, 0x08924CD4u);
                if (c->r[2] != 0) *W(c->r[2] + 88) = o;
            }
        }
        call2(c, o, 0x30, f_0886163C, 0x08924CF0u);
    }
    leave(c, sv);
}

/* (fighter): three animations in a row (+3 counts them, from the words at 0x08A6A36C) at each
   one's 0x40 mark, then the cue 0x15 and the next step with +82, +127 and +3 cleared. Between the
   marks it is ticked, and when the frame changes the wait +74 becomes three quarters of itself
   plus one. A frame keeping s0, s1. */
SFA_FUNC(0892545C, three_in_a_row) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    if (S8(o + 75) & 0x40) {
        uint32_t k = *B(o + 3);
        if (k < 3) {
            call2(c, o, *H(0x08A6A36Cu + k * 2), f_08861674, 0x089254A8u);
            *B(o + 3) = (uint8_t)(*B(o + 3) + 1);
        } else {
            call2(c, o, 0x15, f_0886163C, 0x089254C4u);
            uint8_t step = *B(o + 7);
            *H(o + 82) = 0;
            *B(o + 127) = 0;
            *B(o + 3) = 0;
            *B(o + 7) = (uint8_t)(step + 1);
        }
    } else {
        uint32_t frame = *W(o + 28);
        c->r[17] = frame;
        call1(c, o, f_08861848, 0x089254E8u);
        if (frame != *W(o + 28)) *B(o + 74) = (uint8_t)(S8(o + 74) * 3 / 4 + 1);
    }
    leave2(c, sv);
}

/* (fighter): the next step of a move: facing as +697 says, +235 set, 0x08863114, 0x0886CA2C with
   the strength, and the animation twice the strength, one more when far: for a player with the
   choice 1 with back not held, for the others further than the word from 0x08A6A380 by strength.
   A frame keeping s0, s1. */
SFA_FUNC(08925760, move_near_or_far_25760) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    uint8_t step = *B(o + 7);
    int8_t facing = (int8_t)S8(o + 697);
    *B(o + 7) = (uint8_t)(step + 1);
    c->r[17] = 1;
    *B(o + 11) = (uint8_t)facing;
    *B(o + 235) = 1;
    call1(c, o, f_08863114, 0x08925798u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x089257A4u);
    int32_t strength = S8(o + 194);
    uint32_t anim = (uint32_t)(strength << 1) & 0xFFFF;
    int far;
    if (S8(o + 373) == 0 && S8(o + 386) == 1) far = !(*H(o + 290) & 1);
    else far = S16(0x08A6A380u + (uint32_t)(strength * 2)) < S16(o + 508);
    if (far) anim = (anim + 1) & 0xFFFF;
    call2(c, o, anim, f_08861674, 0x08925818u);
    leave2(c, sv);
}

/* (fighter): in the air on the way down, within 32 of the ground, when 0x0886C2B4 allows: the move
   2 / 0 from state 1 / 0 / 7 / 0, facing as +697 says, +196..+199 cleared, with +78 = 1 with down
   held and -1 without. Returns 0 when it started, else 1. */
SFA_FUNC(089350E4, air_move_2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0886C2B4, 0x089350F8u);
    int ok = c->r[2] != 0 && S8(o + 73) != 0 && (int32_t)*W(o + 100) < 0 &&
             !((int32_t)((uint32_t)(S16(o + 132) + 0x20) & 0xFFFF) < (int32_t)*H(o + 22));
    if (ok) {
        *B(o + 78) = (*H(o + 752) & 2) ? 1 : 0xFF;
        *B(o + 236) = 2;
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        *B(o + 235) = 0;
    }
    c->r[2] = (uint32_t)!ok;
    leave(c, sv);
}

/* (fighter): a move's two steps. First: facing as +697 says, +235 set, 0x0886CA2C with the
   strength, 0x08863114, and the animation 6 + twice the strength, one more when far: for a player
   with the choice 1 with back held, for the others further than 0x20 + 16 times the strength.
   Afterwards 0x0886D678. A frame keeping s0..s2. */
SFA_FUNC(08942C80, move_near_or_far_42c80) {
    gaddr o = ARG(0);
    uint32_t sv[4];
    enter3(c, o, sv);
    if (*B(o + 7) != 0) call1(c, o, f_0886D678, 0x08942CA8u);
    else {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 1;
        uint32_t strength = (uint32_t)S8(o + 194) & 0xFF;
        c->r[17] = strength; c->r[18] = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        call2(c, o, strength, f_0886CA2C, 0x08942CDCu);
        call1(c, o, f_08863114, 0x08942CE4u);
        uint32_t anim = strength * 2 + 6;
        int far;
        if (S8(o + 373) == 0 && S8(o + 386) == 1) far = (*H(o + 290) & 1) != 0;
        else far = (int32_t)((strength << 4) + 0x20) < S16(o + 508);
        if (far) anim += 1;
        call2(c, o, anim & 0xFFFF, f_08861674, 0x08942D3Cu);
    }
    leave3(c, sv);
}

/* (fighter): for the character 0x13 with a choice other than 1 and a third of a meter (0x30), in
   the air at least 0x18 above the ground, when 0x0886C8F4 allows: the move 7 / 1 from state 1 / 0
   / 8 / 0, facing as +697 says, and 0x0886D3E0. Returns whether. */
SFA_FUNC(08949778, air_super_7) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int ok = S8(o + 338) == 0x13;
    if (ok) {
        c->r[17] = 1;
        ok = S8(o + 386) != 1 && !(S16(o + 366) < 48) && S8(o + 73) != 0 &&
             !(S16(o + 22) < S16(o + 132) + 0x18);
    }
    if (ok) {
        call1(c, o, f_0886C8F4, 0x089497E4u);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = 7;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_0886D3E0, 0x0894981Cu);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}

/* (fighter): when 0x0886BF84 allows, state 1 / 0 / 7 / 0 facing as +697 says: the move 0 without
   back held, the move 1 with it (not with +542 set while +251 is clear). Returns whether. */
SFA_FUNC(0896C7A0, move_0_or_1) {
    gaddr o = ARG(0);
    uint32_t stick = *H(o + 752);
    uint32_t sv[2];
    enter(c, o, sv);
    int back_held = (stick & 1) != 0;
    int ok = !back_held || S8(o + 251) != 0 || S8(o + 542) == 0;
    if (ok) {
        call1(c, o, f_0886BF84, back_held ? 0x0896C820u : 0x0896C7C8u);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 0;
        *B(o + 236) = back_held ? 1 : 0;
        *B(o + 11) = (uint8_t)facing;
    }
    c->r[2] = (uint32_t)ok;
    leave(c, sv);
}

/* (fighter): when 0x0886C380 allows: for the choice 0 the move 0xA / 1 from state 1 / 0 / 7 / 0,
   standing, facing as +697 says, its +127 cleared and the opponent's 0x15, +573 = 0x19, +777 set,
   with the animation 0x18 (returns 1); for the others what 0x0897C42C returns. A frame keeping
   s0..s2. */
SFA_FUNC(0897BD74, grab_start_7bd74) {
    gaddr o = ARG(0);
    uint32_t sv[4];
    enter3(c, o, sv);
    gaddr other = *W(o + 92);
    c->r[17] = other;
    call1(c, o, f_0886C380, 0x0897BD94u);
    if (c->r[2] == 0) c->r[2] = 0;
    else if (S8(o + 386) != 0) call1(c, o, f_0897C42C, 0x0897BDB0u);
    else {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 0xA;
        *B(o + 696) = 0;
        c->r[18] = 1;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 235) = 1;
        *B(o + 127) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(other + 127) = 0x15;
        *B(o + 573) = 0x19;
        *B(o + 777) = 1;
        call2(c, o, 0x18, f_088616EC, 0x0897BE38u);
        c->r[2] = 1;
    }
    leave3(c, sv);
}
