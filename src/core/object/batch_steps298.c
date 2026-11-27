/* More steps the newly written leaves set free, run by the test scenarios: the pieces that follow
   their fighter's frame (five characters' versions), a projectile's aim by button, a third guard move,
   a hop's landing, and the fight task's step. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089BD2B4(Cpu *c); void f_089BD30C(Cpu *c); void f_089BD364(Cpu *c); void f_088D3EC0(Cpu *c);
void f_088D31AC(Cpu *c); void f_0886A1E4(Cpu *c); void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c);
void f_0886C2B4(Cpu *c); void f_089B5104(Cpu *c); void f_0886DD68(Cpu *c); void f_0892F0DC(Cpu *c);
void f_0882196C(Cpu *c); void f_088219B0(Cpu *c); void f_08900590(Cpu *c); void f_0890D3B8(Cpu *c);
void f_08947D44(Cpu *c); void f_08917AC4(Cpu *c); void f_0895D85C(Cpu *c); void f_0895D820(Cpu *c);
void f_08862004(Cpu *c); void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_088D5E44(Cpu *c);
void f_08832C28(Cpu *c); void f_0883A388(Cpu *c); void f_0883D368(Cpu *c); void f_08823970(Cpu *c);
void f_0880A39C(Cpu *c); void f_08823BEC(Cpu *c);

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

/* (fighter): a held projectile's release. A player lets go by kick: light 0x089BD2B4, middle
   0x089BD30C, heavy 0x089BD364. The cpu the same by its plan (+490, low four bits: 0, 1, 2) once the
   wait +82 is under 20. Otherwise the wait counts down: at 0 the light one; before that 0x088D3EC0 and,
   with +77 clear, 0x088D31AC. */
SFA_FUNC(089BCE48, held_shot_release) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    void (*fire)(Cpu *) = 0; uint32_t back = 0;
    if (cpu == 0) {
        uint32_t b = *H(o + 760);
        if (b & 0x80) { fire = f_089BD2B4; back = 0x089BCEE4u; }
        else if (b & 0x100) { fire = f_089BD30C; back = 0x089BCECCu; }
        else if (b & 0x200) { fire = f_089BD364; back = 0x089BCEB4u; }
    } else {
        int32_t plan = S16(o + 490) & 0xF;
        if (plan <= 2 && S16(o + 82) < 20) {
            if (plan == 0) { fire = f_089BD2B4; back = 0x089BCF48u; }
            else if (plan == 1) { fire = f_089BD30C; back = 0x089BCF70u; }
            else { fire = f_089BD364; back = 0x089BCF94u; }
        }
    }
    if (fire) call1(c, o, fire, back);
    else {
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) call1(c, o, f_089BD2B4, 0x089BCEA4u);
        else {
            call1(c, o, f_088D3EC0, 0x089BCFACu);
            if (S8(o + 77) == 0) call1(c, o, f_088D31AC, 0x089BCFC0u);
        }
    }
    leave(c, sv);
}

/* (fighter): a character's guard move 9, as 0x089B92E0 with 0x089B5104 before the start: on its
   command (0x0886A1E4), with +634 clear: on the ground from outside state 1 / 0 / 5 when the move
   out of a guard is allowed, facing the opponent; in the air with +251 clear when 0x0886C2B4 allows
   it: state 1 / 0 / 8 / 0 and the timings by the buttons: 1; else 0. */
SFA_FUNC(089B5168, guard_move_9) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x089B517Cu; f_0886A1E4(c);
    uint32_t done = 0;
    if (c->r[2] != 0 && S8(o + 634) == 0) {
        int ok = 0;
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, 0x089B51A4u);
            if (c->r[2] != 0) {
                call1(c, o, f_0886C7E4, 0x089B51B4u);
                if (c->r[2] != 0) { *B(o + 11) = (uint8_t)S8(o + 697); ok = 1; }
            }
        } else if (S8(o + 251) == 0) {
            call1(c, o, f_0886C2B4, 0x089B51D8u);
            ok = c->r[2] != 0;
        }
        if (ok) {
            call1(c, o, f_089B5104, 0x089B51E8u);
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 9;
            call1(c, o, f_0886DD68, 0x089B5210u);
            done = 1;
        }
    }
    c->r[2] = done;
    leave(c, sv);
}

/* The pieces below follow their fighter: hidden (+1) at first; a piece whose character (+3) is no
   longer the fighter's goes to its end; while the fighter's frame (+296) names no picture for it
   (+10 = 0) its own +78 is cleared; else it takes the fighter's place and facing, is shown, and when
   the picture number differs from its +78 starts that animation from the character's table
   (0x0882196C), else goes on with the one it has (0x088219B0). The versions differ in where the
   fighter comes from, whether the place is copied whole or by pixel, and whether +78 keeps the
   number. */

/* (piece, fighter): the whole place; +78 is not kept; the table 0x08A6A780. */
SFA_FUNC(0892F008, piece_follow_6a780) {
    gaddr x = ARG(0), own = ARG(1);
    *B(x + 1) = 0;
    uint32_t mine = *B(x + 3);
    int32_t who = S8(own + 338);
    uint32_t ra = enter0(c);
    if (mine != (uint32_t)who) { c->r[6] = own; call2(c, x, own, f_0892F0DC, 0x0892F03Cu); }
    else {
        uint32_t k = *B(*W(own + 296) + 10);
        if (k == 0) *B(x + 78) = 0;
        else {
            *W(x + 16) = *W(own + 16); *W(x + 20) = *W(own + 20);
            uint8_t facing = *B(own + 11);
            int32_t had = S8(x + 78);
            *B(x + 11) = facing; *B(x + 1) = 1;
            if (k == (uint32_t)had) call1(c, x, f_088219B0, 0x0892F0C0u);
            else call2(c, x, *W(0x08A6A780u + (k << 2)), f_0882196C, 0x0892F0ACu);
        }
    }
    leave0(c, ra);
}

/* (piece, fighter): by pixel (+18, +22); +78 keeps the number; the table 0x08A49DD8. */
SFA_FUNC(089004B8, piece_follow_49dd8) {
    gaddr x = ARG(0), own = ARG(1);
    *B(x + 1) = 0;
    int32_t who = S8(own + 338);
    uint32_t mine = *B(x + 3);
    uint32_t ra = enter0(c);
    if ((uint32_t)who != mine) { c->r[6] = own; call2(c, x, own, f_08900590, 0x08900504u); }
    else {
        gaddr frame = *W(own + 296);
        if (*B(frame + 10) == 0) *B(x + 78) = 0;
        else {
            *H(x + 18) = (uint16_t)S16(own + 18);
            *H(x + 22) = (uint16_t)S16(own + 22);
            uint8_t facing = *B(own + 11);
            *B(x + 1) = 1; *B(x + 11) = facing;
            int32_t had = S8(x + 78);
            uint32_t k = *B(frame + 10);
            if ((uint32_t)had == k) call1(c, x, f_088219B0, 0x08900578u);
            else {
                *B(x + 78) = (uint8_t)k;
                call2(c, x, *W(0x08A49DD8u + (uint32_t)(S8(x + 78) << 2)), f_0882196C, 0x08900564u);
            }
        }
    }
    leave0(c, ra);
}

/* (piece): the fighter is its owner (+88); by pixel; +78 keeps the number; the table 0x08A53DF0, or
   0x08A53FFC for a fighter whose choice is below 0. */
SFA_FUNC(0890D2E0, piece_follow_53df0) {
    gaddr x = ARG(0), own = *W(x + 88);
    *B(x + 1) = 0;
    uint32_t mine = *B(x + 3);
    int32_t who = S8(own + 338);
    uint32_t ra = enter0(c);
    c->r[5] = own;
    if (mine != (uint32_t)who) { c->r[31] = 0x0890D308u; f_0890D3B8(c); }
    else {
        uint32_t k = *B(*W(own + 296) + 10);
        if (k == 0) *B(x + 78) = 0;
        else {
            *H(x + 18) = (uint16_t)S16(own + 18);
            *H(x + 22) = (uint16_t)S16(own + 22);
            uint8_t facing = *B(own + 11);
            int32_t had = S8(x + 78);
            *B(x + 11) = facing; *B(x + 1) = 1;
            if (k == (uint32_t)had) { c->r[31] = 0x0890D3ACu; f_088219B0(c); }
            else {
                *B(x + 78) = (uint8_t)k;
                if (S8(own + 386) < 0) call2(c, x, *W(0x08A53FFCu + (k << 2)), f_0882196C, 0x0890D398u);
                else call2(c, x, *W(0x08A53DF0u + (k << 2)), f_0882196C, 0x0890D378u);
            }
        }
    }
    leave0(c, ra);
}

/* (piece, fighter): the whole place; +78 keeps the number; the table 0x08A74988. */
SFA_FUNC(08947C6C, piece_follow_74988) {
    gaddr x = ARG(0), own = ARG(1);
    *B(x + 1) = 0;
    uint32_t mine = *B(x + 3);
    uint32_t k = *B(*W(own + 296) + 10);
    int32_t who = S8(own + 338);
    uint32_t ra = enter0(c);
    if (mine != (uint32_t)who) { c->r[6] = own; call2(c, x, own, f_08947D44, 0x08947CA8u); }
    else if (k == 0) *B(x + 78) = 0;
    else {
        *W(x + 16) = *W(own + 16); *W(x + 20) = *W(own + 20);
        uint8_t facing = *B(own + 11);
        int32_t had = S8(x + 78);
        *B(x + 11) = facing; *B(x + 1) = 1;
        if (k == (uint32_t)had) call1(c, x, f_088219B0, 0x08947D38u);
        else {
            *B(x + 78) = (uint8_t)k;
            call2(c, x, *W(0x08A74988u + (k << 2)), f_0882196C, 0x08947D24u);
        }
    }
    leave0(c, ra);
}

/* (piece, fighter): by pixel; +78 keeps the number; the table 0x08A5B458; a piece of another
   character has its state moved on twice (once here, once by 0x08917AC4). */
SFA_FUNC(089179DC, piece_follow_5b458) {
    gaddr x = ARG(0), own = ARG(1);
    *B(x + 1) = 0;
    int32_t who = S8(own + 338);
    uint32_t mine = *B(x + 3);
    uint32_t ra = enter0(c);
    if ((uint32_t)who != mine) {
        *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
        c->r[6] = own;
        call2(c, x, own, f_08917AC4, 0x08917A1Cu);
    } else if (*B(*W(own + 296) + 10) == 0) *B(x + 78) = 0;
    else {
        *H(x + 18) = (uint16_t)S16(own + 18);
        *H(x + 22) = (uint16_t)S16(own + 22);
        uint8_t facing = *B(own + 11);
        *B(x + 1) = 1; *B(x + 11) = facing;
        uint32_t k = *B(*W(own + 296) + 10);
        int32_t had = S8(x + 78);
        if ((uint32_t)had == k) call1(c, x, f_088219B0, 0x08917AB0u);
        else {
            *B(x + 78) = (uint8_t)k;
            uint32_t now = *B(*W(own + 296) + 10);
            call2(c, x, *W(0x08A5B458u + (now << 2)), f_0882196C, 0x08917AA4u);
        }
    }
    leave0(c, ra);
}

/* (fighter): a hop: while +10, the hop back when +123 is negative (0x0895D85C), else the wait +0x52
   and then the launch (0x0895D820). Landed (0x08862004): the next step on the ground, 3.0 across with
   the brake by strength (table 0x08A85A54) and the animation 0x10; else ticked. */
SFA_FUNC(0895D5C8, hop_step) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint8_t on = *B(o + 10);
    enter(c, o, sv);
    if (on != 0) {
        if (S8(o + 123) < 0) call1(c, o, f_0895D85C, 0x0895D5F4u);
        else {
            int32_t t = S8(o + 0x52) - 1;
            *B(o + 0x52) = (uint8_t)t;
            if ((int8_t)t < 0) call1(c, o, f_0895D820, 0x0895D620u);
        }
    }
    call1(c, o, f_08862004, 0x0895D628u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x0895D638u);
    else {
        uint8_t step = *B(o + 7);
        int16_t ground = S16(o + 132);
        *B(o + 7) = (uint8_t)(step + 1);
        *H(o + 22) = (uint16_t)ground; *H(o + 20) = 0;
        *B(o + 73) = 0;
        int32_t strength = S8(o + 194);
        *W(o + 100) = 0; *W(o + 108) = 0; *W(o + 96) = 0x30000;
        *W(o + 104) = *W(0x08A85A54u + (uint32_t)(strength << 2));
        call2(c, o, 0x10, f_088616EC, 0x0895D690u);
    }
    leave(c, sv);
}

/* (game, battle): the fight task's step outside the network (0x088D5E44; else 0x08832C28): by the
   game's +4: 0 the set-up 0x0883A388; 1 the fight's sub-step (0x0883D368) until +10, then 2; 2 back to
   0 with 0x08823970 and 0x0880A39C; 3 to +2 = 2 / +4 = 5 with +56 / +41 cleared and 0x08823BEC. A
   frame keeping s0, s1. */
SFA_FUNC(08837B90, fight_task_step) {
    gaddr g = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = b;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = g;
    c->r[31] = 0x08837BACu; f_088D5E44(c);
    if (c->r[2] != 0) call2(c, g, b, f_08832C28, 0x08837C60u);
    else {
        uint32_t step = *H(g + 4);
        if (step == 0) call2(c, g, b, f_0883A388, 0x08837BCCu);
        else if (step == 1) {
            if (*H(g + 10) != 0) *H(g + 4) = 2;
            else { ARG(0) = 2; ARG(1) = 0; c->r[31] = 0x08837BF0u; f_0883D368(c); }
        } else if (step == 2) {
            *H(g + 4) = 0;
            call2(c, g, b, f_08823970, 0x08837C18u);
            c->r[31] = 0x08837C20u; f_0880A39C(c);
        } else if (step == 3) {
            *H(g + 2) = 2; *H(g + 4) = 5; *B(g + 56) = 0; *B(g + 41) = 0;
            call2(c, g, b, f_08823BEC, 0x08837C50u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
