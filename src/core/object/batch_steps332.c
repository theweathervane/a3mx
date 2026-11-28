/* The fight's frame for the round's judge, a side's place in its list, the link's set-ups by
   mode, and a character's two moves that turn into each other. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define NETP      0x09523980u

void f_08821F58(Cpu *c); void f_08828C30(Cpu *c); void f_088261E0(Cpu *c); void f_08828184(Cpu *c);
void f_08825C08(Cpu *c); void f_08825C88(Cpu *c); void f_08825D08(Cpu *c); void f_08833068(Cpu *c);
void f_088259E8(Cpu *c); void f_08826148(Cpu *c); void f_08829D70(Cpu *c); void f_0883311C(Cpu *c);
void f_0882659C(Cpu *c); void f_08826424(Cpu *c); void f_08836F24(Cpu *c); void f_0882EACC(Cpu *c);
void f_088D6258(Cpu *c); void f_088D6120(Cpu *c); void f_0894CC44(Cpu *c); void f_0894D408(Cpu *c);
void f_0894D0EC(Cpu *c); void f_08863114(Cpu *c); void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c);
void f_0886D678(Cpu *c); void f_08862004(Cpu *c); void f_08864888(Cpu *c); void f_08861848(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (x, y): the round's judge, every frame of a fight (not in mode 2). With x's +38 = 1 the round is
   called off: +38, y's +14 and x's +72 are -1, y's +15 and +45 cleared, and 0x08821F58 runs.
   Otherwise, unless x's +39 is set: 0x08828C30, and 0x088261E0 says who won (kept at y + 12).
   With no winner yet (and x's +89 clear): the clock (y + 9 seconds, y + 10 frames of 60) runs
   unless it is off (x + 47 = 3) or the fight is held (y + 36, + 37), and at 0 time is up (y + 13 =
   -1) and whoever leads on vitality wins (0x08828184); mode 7 runs the partners' frame and
   0x08825C88, mode 0x12 0x08825D08, and x's +146 0x08833068.
   With a winner (1, 2, or 3 for a draw): a draw clears y's +36 and sets +76; the fights through
   lists (y + 155) have their own round's end (0x088259E8); the others take the two fighters from
   0x08826148 as winner and loser: a draw keeps them at y + 112 and + 116 with +1172 cleared, a win
   runs the round's end (0x08829D70); then 0x0883311C and 0x0882659C. The winner and the kind of
   fight go to the link play (+41, +52), 0x08826424 runs, and without time up y's +28..+30 are
   0x3C, 3, 3. Then the next phase: x's +6 up and +8 cleared, y's +14 and +7 and x's +43 set, x's
   +72 = -1, y's +45 and all four fighters' +237 cleared, and 0x08836F24. A 0x30 frame keeping
   s0..s3; the two fighters come back at +16 and +20 of it. */
SFA_FUNC(08829A90, round_judge) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    int32_t mode = S8(x + 114);
    *W(sp + 28) = sv[1]; c->r[17] = x;
    *W(sp + 24) = sv[0]; *W(sp + 32) = sv[2]; *W(sp + 36) = sv[3]; *W(sp + 40) = sv[4];
    c->r[16] = y;
    if (mode == 2) goto out;
    c->r[19] = 1;
    if (S8(x + 38) == 1) {
        *B(x + 38) = 0xFF;
        *B(y + 14) = 0xFF;
        *B(x + 72) = 0xFF;
        *B(y + 15) = 0;
        *B(y + 45) = 0;
        call1(c, 0xFFFFFFFFu, f_08821F58, 0x08829AFCu);
        goto out;
    }
    if (S8(x + 39) != 0) goto out;
    call2(c, x, y, f_08828C30, 0x08829B20u);
    call2(c, x, y, f_088261E0, 0x08829B2Cu);
    *B(y + 12) = (uint8_t)c->r[2];
    if ((int8_t)c->r[2] == 0) {
        if (S8(x + 89) != 0) goto out;
        if (S8(x + 47) != 3 && (S8(y + 37) | S8(y + 36)) == 0) {
            int32_t frames = S8(y + 10) - 1;
            *B(y + 10) = (uint8_t)frames;
            if ((int8_t)frames == 0) {
                int32_t seconds = S8(y + 9) - 1;
                *B(y + 10) = 0x3B;
                *B(y + 9) = (uint8_t)seconds;
                if ((int8_t)seconds < 0) {
                    *B(y + 9) = 0;
                    *B(y + 13) = 0xFF;
                    call2(c, x, y, f_08828184, 0x08829BC0u);
                }
            }
        }
        if (S8(x + 114) == 7) {
            call2(c, x, y, f_08825C08, 0x08829BD8u);
            call0(c, f_08825C88, 0x08829BE0u);
        }
        if (S8(x + 114) == 0x12) call1(c, 0x12, f_08825D08, 0x08829BF8u);
        if (S8(x + 146) != 0) call2(c, x, y, f_08833068, 0x08829C0Cu);
    }
    {
        int32_t won = S8(y + 12);
        if (won == 0) goto out;
        int32_t lists = S8(y + 155);
        c->r[18] = 3;
        if (won == 3 && lists != 0) { *B(y + 36) = 0; *B(y + 76) = 1; }
        if (lists != 0) call2(c, x, y, f_088259E8, 0x08829C3Cu);
        else {
            if (won == 3) {
                *B(y + 36) = 0; *B(y + 76) = 1;
                call3(c, y, sp + 0x10, sp + 0x14, f_08826148, 0x08829C68u);
                gaddr a = *W(sp + 16), b = *W(sp + 20);
                *W(y + 112) = a; *W(y + 116) = b;
                *B(a + 1172) = 0;
                *B(*W(sp + 20) + 1172) = 0;
            } else {
                call3(c, y, sp + 0x10, sp + 0x14, f_08826148, 0x08829C94u);
                ARG(2) = *W(sp + 16); ARG(3) = *W(sp + 20);
                call2(c, x, y, f_08829D70, 0x08829CA8u);
            }
            call1(c, y, f_0883311C, 0x08829CB0u);
            call2(c, x, y, f_0882659C, 0x08829CBCu);
        }
        *B(NETP + 41) = (uint8_t)S8(y + 12);
        *H(NETP + 52) = (uint16_t)S16(y + 56);
        call2(c, x, y, f_08826424, 0x08829CE0u);
        if (S8(y + 13) == 0) { *B(y + 28) = 0x3C; *B(y + 29) = 3; *B(y + 30) = 3; }
        uint16_t phase = *H(x + 6);
        *H(x + 8) = 0;
        *H(x + 6) = (uint16_t)(phase + 1);
        *B(y + 14) = 1;
        *B(x + 43) = 1;
        *B(x + 72) = 0xFF;
        *B(y + 7) = 1;
        *B(y + 45) = 0;
        *B(FIGHTER_1 + 237) = 0; *B(FIGHTER_2 + 237) = 0; *B(FIGHTER_3 + 237) = 0; *B(FIGHTER_4 + 237) = 0;
        call1(c, FIGHTER_4, f_08836F24, 0x08829D54u);
    }
out:
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (x, y, fighter): a side's place for the next fight. In the modes 7 and 8 (y + 79), once the list
   is through (y + 20 has reached y + 72), the fighter's +400 and +404 are cleared, a new list is
   made (0x0882EACC) and y's +20 starts over at -1; in the other modes y's +48 is the fighter's
   side. */
SFA_FUNC(0882F194, side_next_fight) {
    gaddr x = ARG(0), y = ARG(1), f = ARG(2);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = y;
    int32_t mode = S8(y + 79);
    *W(c->r[29] + 20) = ra;
    if (mode == 7 || mode == 8) {
        if (!(S16(y + 20) < S16(y + 72))) {
            *H(f + 404) = 0;
            *W(f + 400) = 0;
            call2(c, x, y, f_0882EACC, 0x0882F200u);
            *H(y + 20) = 0xFFFF;
        }
    } else *B(y + 48) = (uint8_t)S8(f + 337);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* What goes out over the link before a match, in mode 0xC or with NETP + 34 set: the two lists in
   mode 6, the two fighters in mode 1. A 0x10 frame. */
SFA_FUNC(088D6378, link_send_by_mode) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    int32_t mode = S8(GAME + 114);
    *W(c->r[29]) = ra;
    if (mode == 0xC || *B(NETP + 34) != 0) {
        if (mode == 6) call0(c, f_088D6258, 0x088D63C0u);
        else if (mode == 1) call0(c, f_088D6120, 0x088D63DCu);
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The usual first step of a move: facing as +697 says, +235 set, 0x08863114 and 0x0886CA2C with
   the strength. */
static void first_step(Cpu *c, gaddr o, uint32_t back0, uint32_t back1) {
    int8_t facing = (int8_t)S8(o + 697);
    *B(o + 7) = 1;
    *B(o + 11) = (uint8_t)facing;
    *B(o + 235) = 1;
    call1(c, o, f_08863114, back0);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, back1);
}

/* The standing move's two steps: the first with the animation twice the strength (6 more with
   +193), one more when far (a player with the choice 1: back not held; the others: further than
   the word from `far` by strength); afterwards 0x0886D678. `back`: the return addresses of
   0x0886D678, the first step's two calls and the animation. */
static void standing_two_steps(Cpu *c, gaddr o, uint32_t far, const uint32_t back[4]) {
    uint8_t step = *B(o + 7);
    *B(o + 696) = 0;
    if (step != 0) { call1(c, o, f_0886D678, back[0]); return; }
    c->r[17] = 1;
    first_step(c, o, back[1], back[2]);
    int32_t strength = S8(o + 194);
    int32_t anim = (int16_t)((S8(o + 193) != 0 ? 6 : 0) + (strength << 1));
    int is_far;
    if (S8(o + 373) == 0 && S8(o + 386) == 1) is_far = !(*H(o + 290) & 1);
    else is_far = S16(far + (uint32_t)(strength * 2)) < S16(o + 508);
    if (is_far) anim = (int16_t)(anim + 1);
    call2(c, o, (uint32_t)anim & 0xFFFF, f_08861674, back[3]);
}

/* The animation's push (+76) moves the fighter forward by hand, once. */
static void push_forward(gaddr o) {
    int32_t push = S8(o + 76);
    if (push == 0) return;
    if (*B(o + 11) == 0) push = (int16_t)-push;
    int32_t x = S16(o + 18);
    *B(o + 76) = 0;
    *H(o + 18) = (uint16_t)(x + push);
}

/* (fighter): a punch move. The cpu with +491's bit 4 set turns it into the other move first
   (0x0894CC44 with 0x13, then 0x0894D408). With +192, crouching: two steps, the first with the
   animation 0xC + strength (3 more with +193), then 0x0886D678. Without it the standing two
   steps, with the table 0x08A79430; for the choice -1 the table 0x08A7943C and the animation's
   push. A frame keeping s0, s1. */
SFA_FUNC(0894D0EC, punch_move_4d0ec) {
    static const uint32_t back_a[4] = { 0x0894D238u, 0x0894D18Cu, 0x0894D198u, 0x0894D228u };
    static const uint32_t back_c[4] = { 0x0894D3ACu, 0x0894D2F0u, 0x0894D2FCu, 0x0894D38Cu };
    gaddr o = ARG(0);
    int32_t choice = S8(o + 386);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = o;
    if (choice != -1 && S8(o + 373) != 0 && (S8(o + 491) & 0x10)) {
        call2(c, o, 0x13, f_0894CC44, 0x0894D148u);
        call1(c, o, f_0894D408, 0x0894D150u);
        goto out;
    }
    {
        int32_t crouch = S8(o + 192);
        uint32_t step = *B(o + 7);
        c->r[17] = step;
        if (crouch != 0) {
            *B(o + 696) = 1;
            if (step != 0) call1(c, o, f_0886D678, 0x0894D2B4u);
            else {
                first_step(c, o, 0x0894D268u, 0x0894D274u);
                int32_t anim = (int16_t)(S8(o + 194) + 0xC);
                if (S8(o + 193) != 0) anim = (int16_t)(anim + 3);
                c->r[17] = (uint32_t)anim;
                call2(c, o, (uint32_t)anim & 0xFFFF, f_08861674, 0x0894D2A4u);
            }
        } else if (choice != -1) standing_two_steps(c, o, 0x08A79430u, back_a);
        else {
            standing_two_steps(c, o, 0x08A7943Cu, back_c);
            push_forward(o);
        }
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the other move. The cpu without +491's bit 4 turns it into the punch move first
   (0x0894CC44 with 0x11, then 0x0894D0EC). By +192: 0 the standing two steps with the table
   0x08A79448; 1, crouching, two steps: the first with the animation 0xC + strength (3 more with
   +193; the heavy one without +193 sets +644), then 0x0886D678; any other: 0x08864888 once
   0x08862004 says so, else on the cue (+76) with +123 set and the heavy kick (the cpu: down) held
   the animation 0x1E (0x1F with +78 set), otherwise ticked. Except for +192 = 2 the animation's
   push follows. A frame keeping s0..s2. */
SFA_FUNC(0894D408, kick_move_4d408) {
    static const uint32_t back_a[4] = { 0x0894D550u, 0x0894D4A4u, 0x0894D4B0u, 0x0894D540u };
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = o;
    if (cpu != 0 && !(S8(o + 491) & 0x10)) {
        call2(c, o, 0x11, f_0894CC44, 0x0894D458u);
        call1(c, o, f_0894D0EC, 0x0894D460u);
        goto out;
    }
    {
        int32_t kind = S8(o + 192);
        c->r[18] = 2;
        if (kind == 0) standing_two_steps(c, o, 0x08A79448u, back_a);
        else {
            c->r[17] = 1;
            if (kind == 1) {
                uint8_t step = *B(o + 7);
                *B(o + 696) = 1;
                if (step != 0) call1(c, o, f_0886D678, 0x0894D5E4u);
                else {
                    first_step(c, o, 0x0894D588u, 0x0894D594u);
                    int32_t alt = S8(o + 193);
                    int32_t strength = S8(o + 194);
                    if (alt == 0 && strength == 2) *B(o + 644) = 1;
                    int32_t anim = (int16_t)(strength + 0xC);
                    if (alt != 0) anim = (int16_t)(anim + 3);
                    call2(c, o, (uint32_t)anim & 0xFFFF, f_08861674, 0x0894D5D4u);
                }
            } else {
                call1(c, o, f_08862004, 0x0894D5F4u);
                if (c->r[2] != 0) call1(c, o, f_08864888, 0x0894D604u);
                else {
                    int turn = S8(o + 123) != 0 && S8(o + 76) != 0;
                    if (turn) turn = S8(o + 373) != 0 ? (*H(o + 290) & 2) != 0 : (*H(o + 760) & 0x200) != 0;
                    if (turn) call2(c, o, S8(o + 78) == 0 ? 0x1E : 0x1F, f_08861674, 0x0894D670u);
                    else call1(c, o, f_08861848, 0x0894D680u);
                }
            }
        }
        if (S8(o + 192) != 2) push_forward(o);
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
