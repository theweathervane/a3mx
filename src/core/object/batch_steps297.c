/* More steps the newly written leaves set free, run by the test scenarios: move steps that choose by
   button or by the cpu's plan, a grab out of a move, an object that follows its owner's state, the
   link's time-outs, and two characters' guard-move starts. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_0886AC80(Cpu *c); void f_08861A14(Cpu *c); void f_0886469C(Cpu *c); void f_08861848(Cpu *c);
void f_088DD124(Cpu *c); void f_088DD114(Cpu *c); void f_088DD380(Cpu *c); void f_08907700(Cpu *c);
void f_089079C8(Cpu *c); void f_089079EC(Cpu *c); void f_0891D014(Cpu *c); void f_08822EAC(Cpu *c);
void f_0887C408(Cpu *c); void f_08821424(Cpu *c); void f_088EE9E0(Cpu *c); void f_08863A50(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c); void f_08863114(Cpu *c); void f_0890F564(Cpu *c);
void f_0890FA58(Cpu *c); void f_08936470(Cpu *c); void f_089376A8(Cpu *c); void f_089363D0(Cpu *c);
void f_08934CC0(Cpu *c); void f_08934D08(Cpu *c); void f_08936414(Cpu *c); void f_08936178(Cpu *c);
void f_0886A1E4(Cpu *c); void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DD68(Cpu *c); void f_0890F88C(Cpu *c); void f_0890F83C(Cpu *c);

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
/* A 0x20 frame with s0 at +16 and the return address at +20; s0 holds the object. */
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void pass(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }

/* (fighter): with +123 = 1 the opponent becomes +316 and the throw check 0x0886AC80 (0x70) is tried:
   when it passes the next step, 12.0 up with gravity 0x5000 and 0x08861A14 (0x24, 2); else standing at
   the animation's end, or ticked. */
SFA_FUNC(089A4C18, grab_out_of_move) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 123);
    enter(c, o, sv);
    int grab = 0;
    if (k == 1) {
        *W(o + 92) = *W(o + 316);
        call2(c, o, 0x70, f_0886AC80, 0x089A4C48u);
        grab = c->r[2] != 0;
    }
    if (grab) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 100) = 0xC0000; *W(o + 108) = 0xFFFFB000u;
        c->r[6] = 2;
        call2(c, o, 0x24, f_08861A14, 0x089A4C94u);
    } else if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x089A4C64u);
    else call1(c, o, f_08861848, 0x089A4CACu);
    leave(c, sv);
}

/* (): the link's time-outs, while 0x09523708 is set: every entry but our own (0x088DD124), from the
   highest number (0x088DD114) down, counts its +0 down; one that has run out gets -1 and +2 = 2. A
   0x30 frame keeping s0..s3. */
SFA_FUNC(088E4794, net_timeouts_tick) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    int8_t on = S8(0x095236E0u + 40);
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = s3; *W(sp + 32) = ra;
    if (on != 0) {
        c->r[31] = 0x088E47C8u; f_088DD124(c);
        uint32_t own = c->r[2];
        c->r[19] = own;
        c->r[31] = 0x088E47D0u; f_088DD114(c);
        int32_t n = (int32_t)c->r[2];
        c->r[18] = (uint32_t)n; c->r[17] = 0xFFFFFFFFu;
        if (!(n < 0)) {
            c->r[16] = 2;
            do {
                if ((uint32_t)n != own) {
                    c->r[18] = (uint32_t)n;
                    call1(c, (uint32_t)n, f_088DD380, 0x088E47F0u);
                    gaddr e = c->r[2];
                    if (e != 0) {
                        int32_t t = S16(e);
                        if (t <= 0) { *H(e) = 0xFFFF; *B(e + 2) = 2; }
                        else *H(e) = (uint16_t)(t - 1);
                    }
                }
                n--;
            } while (n >= 0);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): a player with no attack button pressed (+760) takes 0x08907700, with a punch 0x089079C8,
   with a kick the leap 0x089079EC; the cpu the same by its +193: 0, 2 the leap, others 0x089079C8. */
SFA_FUNC(08907914, follow_by_button) {
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t ra = enter0(c);
    if (cpu == 0) {
        uint32_t b = *H(o + 760);
        if ((b & 0x3F0) == 0) pass(c, f_08907700, 0x08907940u);
        else if (b & 0x70) pass(c, f_089079C8, 0x0890795Cu);
        else pass(c, f_089079EC, 0x08907970u);
    } else {
        int32_t k = S8(o + 193);
        if (k == 0) pass(c, f_08907700, 0x0890798Cu);
        else if (k == 2) pass(c, f_089079EC, 0x089079A8u);
        else pass(c, f_089079C8, 0x089079BCu);
    }
    leave0(c, ra);
}

/* (object): an object that follows its owner's move (+88), unless BATTLE + 37 / 78: once the owner's
   state (+4..+6) no longer matches what it kept (+336..+339) it goes on to its next state. Else it
   stays on the owner and, by its step +5: waits for the owner's +77; the meter by 5; the owner's
   +0x338 up to 7, the meter by 0x12, a new object 0x29 / 3 at the place of +340 with the owner's
   facing and looks, and the cue 0x222; the meter by 5 and the cue 0x223 (twice). 0x08821424 last. A
   frame keeping s0..s2. */
SFA_FUNC(0891F2B0, object_follow_move) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = x;
    int8_t held = S8(BATTLE + 37);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    if (held == 0 && S8(BATTLE + 78) == 0) {
        gaddr own = *W(x + 88);
        c->r[17] = own;
        uint32_t now = ((uint32_t)*B(own + 4) << 24) | ((uint32_t)*B(own + 5) << 16) | ((uint32_t)*B(own + 6) << 8);
        uint32_t kept = ((uint32_t)*B(x + 336) << 24) | ((uint32_t)*B(x + 337) << 16) |
                        ((uint32_t)*B(x + 338) << 8) | *B(x + 339);
        if (now != kept) {
            *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
            c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
            return;
        }
        *H(x + 18) = (uint16_t)S16(own + 18);
        int16_t y = S16(own + 22);
        uint32_t step = *B(x + 5);
        *H(x + 22) = (uint16_t)y;
        *B(x + 11) = *B(own + 11);
        if (step == 0) {
            if (!(S8(own + 77) < 0)) *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
        } else if (step == 1) {
            *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
            call2(c, own, 5, f_0891D014, 0x0891F3C4u);
        } else if (step == 2) {
            uint32_t v = *B(own + 0x334 + 4);
            if (v != 7) *B(own + 0x334 + 4) = (uint8_t)(v + 1);
            *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
            call2(c, own, 0x12, f_0891D014, 0x0891F404u);
            gaddr at = *W(x + 340);
            c->r[18] = at;
            call2(c, 0x29, 3, f_08822EAC, 0x0891F414u);
            gaddr n = c->r[2];
            if (n != 0) {
                *W(n + 16) = *W(at + 16); *W(n + 20) = *W(at + 20);
                *B(n + 11) = *B(own + 11); *B(n + 14) = *B(own + 14);
                *H(n + 24) = *H(own + 24);
                *B(n + 12) = (uint8_t)S8(own + 12); *B(n + 13) = *B(own + 13);
                *H(n + 86) = *H(own + 86);
                *W(n + 164) = *W(own + 164);
            }
            c->r[6] = 0x222;
            call2(c, own, (uint32_t)(int32_t)S8(own + 337), f_0887C408, 0x0891F478u);
        } else if (step == 3 || step == 4) {
            *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
            call2(c, own, 5, f_0891D014, step == 3 ? 0x0891F4A0u : 0x0891F4D8u);
            c->r[6] = 0x223;
            call2(c, own, (uint32_t)(int32_t)S8(own + 337), f_0887C408, step == 3 ? 0x0891F4B0u : 0x0891F4E8u);
        }
    }
    call1(c, x, f_08821424, 0x0891F4F0u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a cpu whose plan (+491) has bit 3 takes move 3's start (0x088EE9E0); else step 3 with
   +235 set, 0x08863A50, the meter by the strength, and the animation 0x12 (0x18 with +78; 3 more with
   +193) plus the strength. */
SFA_FUNC(088EE928, step_3_anim_12) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    if (cpu != 0 && (S8(o + 491) & 8)) call1(c, o, f_088EE9E0, 0x088EE990u);
    else {
        *B(o + 7) = 3; *B(o + 235) = 1;
        call2(c, o, 1, f_08863A50, 0x088EE968u);
        call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x088EE974u);
        uint32_t base = S8(o + 78) == 0 ? 0x12 : 0x18;
        if (S8(o + 193) != 0) base = (base + 3) & 0xFFFF;
        call2(c, o, (base + (uint32_t)(int32_t)S8(o + 194)) & 0xFFFF, f_08861674, 0x088EE9D0u);
    }
    leave(c, sv);
}

/* (fighter): the next step facing the opponent, +235 set, +0x334 and +123 cleared, 0x08863114 and the
   meter by the strength; the heavy one goes to step 2 and, for a cpu holding forward, 0x0890FA58;
   everything else 0x0890F564. */
SFA_FUNC(0890F4A4, next_step_heavy_shift) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    uint8_t step = *B(o + 7);
    int8_t facing = S8(o + 697);
    *B(o + 7) = (uint8_t)(step + 1);
    *B(o + 11) = (uint8_t)facing; *B(o + 235) = 1;
    *B(o + 0x334) = 0; *B(o + 123) = 0;
    *W(c->r[29] + 20) = sv[1];
    call2(c, o, (uint32_t)(int32_t)facing, f_08863114, 0x0890F4E4u);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0890F4F0u);
    if (S8(o + 194) != 2) call1(c, o, f_0890F564, 0x0890F554u);
    else {
        int32_t cpu = S8(o + 373);
        *B(o + 7) = 2;
        if (cpu == 0) call1(c, o, f_0890F564, 0x0890F514u);
        else if (*H(o + 290) & 2) call1(c, o, f_0890FA58, 0x0890F544u);
        else call1(c, o, f_0890F564, 0x0890F534u);
    }
    leave(c, sv);
}

/* (fighter): a dive's step: 0x08936470 until +0x3C0 is set, 0x089376A8 and 0x089363D0 (3); once set,
   the next step with +626: the low middle one (0x08934CC0) halves its speed (0x08934D08), others take
   the animation by strength (0x08936414); before that 0x08936178 while above the ground, else ticked.
   A frame keeping s0, s1. */
SFA_FUNC(08936260, dive_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = o + 0x3A0;
    int8_t hit = S8(o + 0x3A0 + 32);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (hit == 0) call1(c, o, f_08936470, 0x08936288u);
    call1(c, o, f_089376A8, 0x08936290u);
    call2(c, o, 3, f_089363D0, 0x0893629Cu);
    if (S8(o + 0x3A0 + 32) != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 626) = 1;
        call1(c, o, f_08934CC0, 0x089362C4u);
        if (c->r[2] != 0) call1(c, o, f_08934D08, 0x089362D4u);
        else call1(c, o, f_08936414, 0x089362E4u);
    } else if (S16(o + 22) < S16(o + 132)) call1(c, o, f_08936178, 0x08936304u);
    else call1(c, o, f_08861848, 0x08936314u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* A character's guard move: on its command (0x0886A1E4), with +634 clear: on the ground from outside
   state 1 / 0 / 5 when the move out of a guard is allowed (0x0886C7E4), facing the opponent; in the
   air with +251 clear when 0x0886C2B4 allows it; then state 1 / 0 / 8 / 0 with the move and the
   timings by the buttons: 1; else 0. */
static void guard_move(Cpu *c, uint32_t move, const uint32_t back[5]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = back[0]; f_0886A1E4(c);
    uint32_t done = 0;
    if (c->r[2] != 0 && S8(o + 634) == 0) {
        int ok = 0;
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, back[1]);
            if (c->r[2] != 0) {
                call1(c, o, f_0886C7E4, back[2]);
                if (c->r[2] != 0) { *B(o + 11) = (uint8_t)S8(o + 697); ok = 1; }
            }
        } else if (S8(o + 251) == 0) {
            call1(c, o, f_0886C2B4, back[3]);
            ok = c->r[2] != 0;
        }
        if (ok) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = (uint8_t)move;
            call1(c, o, f_0886DD68, back[4]);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}
SFA_FUNC(089B92E0, guard_move_12) {
    static const uint32_t b[5] = { 0x089B92F4u, 0x089B931Cu, 0x089B932Cu, 0x089B9350u, 0x089B9380u };
    guard_move(c, 0xC, b);
}
SFA_FUNC(089C6E10, guard_move_11) {
    static const uint32_t b[5] = { 0x089C6E24u, 0x089C6E4Cu, 0x089C6E5Cu, 0x089C6E80u, 0x089C6EB0u };
    guard_move(c, 0xB, b);
}

/* (fighter): the next step facing the opponent, +235 set, +0x334 cleared, 0x08863114 and the meter by
   the strength; then the animation by strength (0x0890F88C), except the heavy one with +567 clear: a
   cpu or player holding forward takes 0x0890F83C, else step 2 at 9.0 across slowing by 0.5 with the
   animation 0x11. */
SFA_FUNC(0890F770, next_step_heavy_run) {
    gaddr o = ARG(0); uint32_t sv[2];
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    int8_t facing = S8(o + 697);
    *B(o + 235) = 1; *B(o + 11) = (uint8_t)facing;
    *B(o + 0x334) = 0;
    *W(c->r[29] + 20) = sv[1];
    call2(c, o, 1, f_08863114, 0x0890F7ACu);
    call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x0890F7B8u);
    if (S8(o + 194) != 2) call1(c, o, f_0890F88C, 0x0890F7D0u);
    else if (S8(o + 567) != 0) call1(c, o, f_0890F88C, 0x0890F7E8u);
    else if (*H(o + 290) & 2) call1(c, o, f_0890F83C, 0x0890F804u);
    else {
        *B(o + 7) = 2;
        *W(o + 96) = 0x90000; *W(o + 104) = 0xFFFF8000u;
        call2(c, o, 0x11, f_08861674, 0x0890F82Cu);
    }
    leave(c, sv);
}
