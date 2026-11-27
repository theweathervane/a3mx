/* Steps the newly written leaves set free, all of them run by the test scenarios: small choosers
   between two handlers, a jumping attack's follow-ups, an object's bounce and wander, the link's
   record for an address. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089AFF78(Cpu *c); void f_089B0208(Cpu *c); void f_0899DEDC(Cpu *c); void f_0899DE6C(Cpu *c);
void f_088699D8(Cpu *c); void f_088681F8(Cpu *c); void f_08929E18(Cpu *c); void f_08929CD0(Cpu *c);
void f_0890EE6C(Cpu *c); void f_0890EF5C(Cpu *c); void f_08861674(Cpu *c); void f_0892E6BC(Cpu *c);
void f_0886DDCC(Cpu *c); void f_088D3EC0(Cpu *c); void f_089BB11C(Cpu *c); void f_089BB0F0(Cpu *c);
void f_088D2E04(Cpu *c); void f_088D2E54(Cpu *c); void f_0886DEC8(Cpu *c); void f_08910E6C(Cpu *c);
void f_089110B0(Cpu *c); void f_088DD124(Cpu *c); void f_088DD314(Cpu *c); void f_088E1074(Cpu *c);
void f_088E0984(Cpu *c); void f_088F61C8(Cpu *c); void f_088F65D0(Cpu *c); void f_088F6160(Cpu *c);
void f_08948810(Cpu *c); void f_0894885C(Cpu *c); void f_088D04EC(Cpu *c); void f_088D067C(Cpu *c);
void f_088D07AC(Cpu *c); void f_0880AEA8(Cpu *c); void f_0886B058(Cpu *c); void f_088FA880(Cpu *c);
void f_088FA8F0(Cpu *c); void f_088FAA74(Cpu *c); void f_088FAAE8(Cpu *c); void f_0898B67C(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_088616EC(Cpu *c); void f_08821368(Cpu *c);
void f_0884FB5C(Cpu *c); void f_0884FBB0(Cpu *c); void f_0884FACC(Cpu *c); void f_08938718(Cpu *c);
void f_08863A9C(Cpu *c); void f_089385B8(Cpu *c); void f_089376A8(Cpu *c); void f_08864740(Cpu *c);
void f_08861848(Cpu *c); void f_08914BD8(Cpu *c); void f_08914B60(Cpu *c); void f_08910BE0(Cpu *c);
void f_089108D4(Cpu *c);

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
/* The caller's arguments passed on to one of two handlers. */
static void pass(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }

/* (fighter): +6 = 1 and 0x089AFF78 for the cpu, +6 = 2 and 0x089B0208 for a player. */
SFA_FUNC(089AFF38, step_6_by_cpu) {
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t ra = enter0(c);
    if (cpu != 0) { c->r[5] = 1; *B(o + 6) = 1; pass(c, f_089AFF78, 0x089AFF6Cu); }
    else { c->r[5] = 2; *B(o + 6) = 2; pass(c, f_089B0208, 0x089AFF58u); }
    leave0(c, ra);
}

/* (object): 0x0899DEDC when the opponent's character is 40, else 0x0899DE6C. */
SFA_FUNC(0899DE28, step_by_opponent_40) {
    int32_t who = S8(*W(ARG(0) + 92) + 338);
    uint32_t ra = enter0(c);
    c->r[5] = (uint32_t)who; c->r[6] = 0x28;
    if (who == 0x28) pass(c, f_0899DEDC, 0x0899DE4Cu);
    else pass(c, f_0899DE6C, 0x0899DE60u);
    leave0(c, ra);
}

/* (fighter, count, buttons): with one of the buttons pressed (+760), the count goes up and
   0x08868100 runs (0x088699D8: fighter, count); else 0x088681F8 (count, fighter). */
SFA_FUNC(08869860, input_count_or_reset) {
    gaddr o = ARG(0); uint32_t count = ARG(1);
    uint32_t hit = (ARG(2) & 0xFFFF) & *H(o + 760);
    uint32_t ra = enter0(c);
    c->r[7] = count;
    if (hit != 0) { c->r[6] = count; call2(c, o, count, f_088699D8, 0x08869894u); }
    else { c->r[6] = 0; call2(c, count, o, f_088681F8, 0x088698A8u); }
    leave0(c, ra);
}

/* (fighter): 0x08929E18 against character 25 of the other kind (player against cpu), else
   0x08929CD0. */
SFA_FUNC(08929C7C, step_by_opponent_25) {
    gaddr o = ARG(0), other = *W(o + 92);
    int32_t who = S8(other + 338);
    uint32_t ra = enter0(c);
    c->r[5] = other; c->r[6] = 0x19; c->r[7] = (uint32_t)who;
    if (who == 0x19 && S8(o + 373) != S8(other + 373)) pass(c, f_08929E18, 0x08929CB0u);
    else pass(c, f_08929CD0, 0x08929CC4u);
    leave0(c, ra);
}

/* (fighter): a player takes 0x0890EE6C; the cpu the animation 0x22 with forward held (+290 bit 1),
   else 0x0890EF5C. */
SFA_FUNC(0890EE10, anim_by_cpu_stick) {
    gaddr o = ARG(0);
    int32_t cpu = S8(o + 373);
    uint32_t ra = enter0(c);
    if (cpu == 0) pass(c, f_0890EE6C, 0x0890EE2Cu);
    else if (*H(o + 290) & 2) { ARG(1) = 0x22; pass(c, f_08861674, 0x0890EE60u); }
    else { c->r[5] = 0; pass(c, f_0890EF5C, 0x0890EE4Cu); }
    leave0(c, ra);
}

/* (fighter): with +192 the step 0x0892E6BC; else the step's handler (+7 into the table 0x08A6BF20)
   and 0x0886DDCC. */
SFA_FUNC(0892E498, step_by_192) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t alt = S8(o + 192);
    enter(c, o, sv);
    if (alt != 0) call1(c, o, f_0892E6BC, 0x0892E4B8u);
    else {
        gaddr handler = *W(0x08A6BF20u + (uint32_t)*B(o + 7) * 4);
        c->r[5] = handler; ARG(0) = o; c->r[31] = 0x0892E4E0u;
        rt_call_indirect(c, handler);
        call1(c, o, f_0886DDCC, 0x0892E4E8u);
    }
    leave(c, sv);
}

/* (fighter): after 0x088D3EC0, once +77 is clear: the next step into the air (+73, +591): with +820
   set, +78 by its facing (2 and 0x089BB0F0, or 1 and 0x089BB11C) and 6.0 up with gravity 0x6000; else
   12.0 across and 4.0 up with gravity 0x4000, turned by its facing, with 0x088D2E04 (0x13) and
   0x088D2E54 (0x54). Then 0x0886DEC8 while +76, else 0x0886DDCC. */
SFA_FUNC(089BBFFC, leap_start_by_820) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    c->r[31] = 0x089BC010u; f_088D3EC0(c);
    if (S8(o + 77) == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 73) = 1;
        uint32_t held = *W(o + 820);
        *B(o + 591) = 1;
        uint8_t facing = *B(o + 11);
        if (held != 0) {
            if (facing != 0) { *B(o + 78) = 2; call1(c, o, f_089BB0F0, 0x089BC06Cu); }
            else { *B(o + 78) = 1; call1(c, o, f_089BB11C, 0x089BC054u); }
            *W(o + 100) = 0x60000; *W(o + 108) = 0xFFFFA000u;
        } else {
            *W(o + 96) = 0xC0000; *W(o + 104) = 0; *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFC000u;
            *B(o + 78) = 2;
            if (facing == 0) {
                uint32_t vx = *W(o + 96), ax = *W(o + 104);
                *B(o + 78) = 1;
                *W(o + 96) = 0 - vx; *W(o + 104) = 0 - ax;
            }
            call2(c, o, 0x13, f_088D2E04, 0x089BC0D0u);
            call2(c, o, 0x54, f_088D2E54, 0x089BC0DCu);
        }
    }
    if (S8(o + 76) != 0) call1(c, o, f_0886DEC8, 0x089BC100u);
    else call1(c, o, f_0886DDCC, 0x089BC0F0u);
    leave(c, sv);
}

/* (fighter): while the wait +82 runs, a kick (+760) goes to the strength's handler (0x089110B0);
   else 0x08910E6C. */
SFA_FUNC(08910E08, kick_window) {
    gaddr o = ARG(0);
    int32_t t = S16(o + 82);
    uint32_t ra = enter0(c);
    if (t == 0) pass(c, f_08910E6C, 0x08910E24u);
    else {
        uint32_t b = *H(o + 760);
        *H(o + 82) = (uint16_t)(t - 1);
        if (b & 0x380) pass(c, f_089110B0, 0x08910E60u);
        else pass(c, f_08910E6C, 0x08910E4Cu);
    }
    leave0(c, ra);
}

/* (address, out): the link's record for an address: our own (0x088DD124) by 0x088E0984 (out); another's
   by 0x088DD314, then 0x088E1074 (it, out). A frame keeping s0, s1. */
SFA_FUNC(088E1094, net_record_for) {
    uint32_t addr = ARG(0), out = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = out;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = addr;
    c->r[31] = 0x088E10B0u; f_088DD124(c);
    if (addr == c->r[2]) call1(c, out, f_088E0984, 0x088E10E8u);
    else {
        call1(c, addr, f_088DD314, 0x088E10C0u);
        call2(c, c->r[2], out, f_088E1074, 0x088E10CCu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): by the opponent's character: 17 and 19 the steps 0x088F61C8, 23 the steps 0x088F65D0
   (both with the opponent), others 0x088F6160. */
SFA_FUNC(088F60F8, step_by_opponent_17_23) {
    gaddr o = ARG(0), other = *W(o + 92);
    int32_t who = S8(other + 338);
    uint32_t ra = enter0(c);
    c->r[6] = other;
    if (who == 0x11 || who == 0x13) { ARG(1) = other; pass(c, f_088F61C8, 0x088F6124u); }
    else if (who == 0x17) { ARG(1) = other; pass(c, f_088F65D0, 0x088F6140u); }
    else { c->r[5] = (uint32_t)who; pass(c, f_088F6160, 0x088F6154u); }
    leave0(c, ra);
}

/* (fighter): the next step, shown: against character 2 or 36 of the other kind (player against cpu)
   step 2 and 0x0894885C; else 0x08948810. */
SFA_FUNC(0894879C, entrance_by_opponent) {
    gaddr o = ARG(0), other = *W(o + 92);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 1) = 1;
    int32_t who = S8(other + 338);
    uint32_t ra = enter0(c);
    if ((who == 2 || who == 0x24) && (S8(o + 373) ^ S8(other + 373)) != 0) {
        c->r[5] = 2; *B(o + 7) = 2;
        pass(c, f_0894885C, 0x08948804u);
    } else pass(c, f_08948810, 0x089487F0u);
    leave0(c, ra);
}

/* (object): its kind's step (+3: 0x088D04EC, 0x088D067C, 0x088D07AC), then 0x0880AEA8 (2, 0x08ACCCB4,
   0, 3, 1). A 0x30 frame with the return address at +32. */
SFA_FUNC(088D0474, kind_step_then_draw) {
    uint32_t kind = *B(ARG(0) + 3);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = ra;
    c->r[5] = 2; c->r[6] = 1; c->r[7] = kind;
    if (kind == 0) pass(c, f_088D04EC, 0x088D0494u);
    else if (kind == 1) pass(c, f_088D067C, 0x088D04ACu);
    else if (kind == 2) pass(c, f_088D07AC, 0x088D04C4u);
    c->r[6] = 0; c->r[7] = 3; c->r[8] = 1;
    call2(c, 2, 0x08ACCCB4u, f_0880AEA8, 0x088D04E0u);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* A falling attack's follow-up on a button: with a direction held, an opponent whose +238 is clear
   and the throw check passing, the throw's start; else the attack's. Nothing while rising or with no
   such button down. */
static void fall_follow_up(Cpu *c, uint32_t buttons, uint32_t kind, void (*grab)(Cpu *), void (*attack)(Cpu *),
                           const uint32_t back[3]) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t vy = (int32_t)*W(o + 100);
    enter(c, o, sv);
    if (vy < 0 && ((*H(o + 760) | *H(o + 762)) & buttons) != 0) {
        int grabbed = 0;
        if ((*H(o + 290) & 3) != 0 && S8(*W(o + 92) + 238) == 0) {
            call2(c, o, kind, f_0886B058, back[0]);
            if (c->r[2] != 0) { call1(c, o, grab, back[1]); grabbed = 1; }
        }
        if (!grabbed) call1(c, o, attack, back[2]);
    }
    leave(c, sv);
}
SFA_FUNC(088FA7FC, fall_follow_up_punch) {
    static const uint32_t b[3] = { 0x088FA850u, 0x088FA860u, 0x088FA870u };
    fall_follow_up(c, 0x70, 0x5E, f_088FA880, f_088FA8F0, b);
}
SFA_FUNC(088FA9F0, fall_follow_up_kick) {
    static const uint32_t b[3] = { 0x088FAA44u, 0x088FAA54u, 0x088FAA64u };
    fall_follow_up(c, 0x380, 0x5F, f_088FAA74, f_088FAAE8, b);
}

/* (fighter): the cpu with +193 takes 0x0898B67C; else step 2 with +262 set, the meter by 3 (1 once
   +836 is set), 0x08837098 and the animation 15. */
SFA_FUNC(0898B5EC, step_2_meter_anim_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t cpu = S8(o + 373);
    enter(c, o, sv);
    if (cpu != 0 && S8(o + 193) != 0) call1(c, o, f_0898B67C, 0x0898B618u);
    else {
        *B(o + 7) = 2;
        uint32_t again = *W(o + 836);
        *B(o + 262) = 1;
        if (again == 0) call2(c, o, 3, f_0886CA2C, 0x0898B644u);
        else call2(c, o, 1, f_0886CA2C, 0x0898B658u);
        call1(c, o, f_08837098, 0x0898B660u);
        call2(c, o, 0xF, f_088616EC, 0x0898B66Cu);
    }
    leave(c, sv);
}

/* (object): wanders across: while its wait (+0x82) runs it only counts down; else its place (+0x78)
   moves by its speed (+96): at 0xD0, one time in eight (by the random number), a new wait
   (0x0884FBB0); past 480 it turns round (0x0884FB5C) and takes a new wait. Then 0x0884FACC (13). A
   frame keeping s0, s1. */
SFA_FUNC(0884FA10, wander_across) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; c->r[17] = o + 0x60;
    int32_t wait = S16(o + 0x60 + 34);
    *W(c->r[29] + 24) = ra;
    if (wait != 0) *H(o + 0x60 + 34) = (uint16_t)(wait - 1);
    else {
        uint32_t at = *W(o + 0x60 + 24) + *W(o + 96);
        *W(o + 0x60 + 24) = at;
        uint32_t x = (at >> 16) & 0xFFFF;
        c->r[17] = x;
        int rest = 0, inside = (int32_t)x < 481;
        if (x == 0xD0) {
            c->r[31] = 0x0884FA68u; f_08821368(c);
            if ((c->r[2] & 7) == 0) rest = 1;
        }
        if (!rest && !inside) {
            uint32_t v = *W(o + 96);
            c->r[5] = 0 - v;
            *W(o + 96) = 0 - v;
            ARG(0) = o; c->r[31] = 0x0884FA94u; f_0884FB5C(c);
            rest = 1;
        }
        if (rest) call1(c, o, f_0884FBB0, 0x0884FA9Cu);
    }
    call2(c, o, 0xD, f_0884FACC, 0x0884FAB8u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with +77 clear and 0x08938718 saying so, 0x08863A9C fills a byte for +78 and the next
   step starts (0x089385B8); else 0x089376A8, and at the animation's end the opponent's +543 = 5,
   +262 cleared and crouching (0x08864740), otherwise ticked. A 0x20 frame keeping s0 at +20, the
   byte at +16. */
SFA_FUNC(0893851C, step_or_crouch_at_end) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    int32_t busy = S8(o + 77);
    *W(sp + 20) = s0; *W(sp + 24) = ra;
    c->r[16] = o;
    int next = 0;
    if (busy == 0) {
        call1(c, o, f_08938718, 0x0893853Cu);
        next = c->r[2] != 0;
    }
    if (next) {
        call2(c, o, sp + 0x10, f_08863A9C, 0x08938580u);
        uint8_t v = *B(sp + 16);
        c->r[5] = v;
        *B(o + 78) = v;
        call1(c, o, f_089385B8, 0x08938590u);
    } else {
        call1(c, o, f_089376A8, 0x0893854Cu);
        if (S8(o + 75) < 0) {
            *B(*W(o + 92) + 543) = 5;
            *B(o + 262) = 0;
            call1(c, o, f_08864740, 0x08938570u);
        } else call1(c, o, f_08861848, 0x089385A8u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a move's step: with +192, +696 set and 0x08914B60 at step 0 or 0x08914BD8 later; else
   +696 cleared and the step's handler from the table 0x08A5FEB8 (0x08A5FEC8 with +193). */
SFA_FUNC(08914648, move_step_by_192) {
    gaddr o = ARG(0);
    int32_t alt = S8(o + 192);
    uint32_t ra = enter0(c);
    uint32_t step = *B(o + 7);
    if (alt != 0) {
        c->r[5] = step; c->r[6] = 1;
        *B(o + 696) = 1;
        if (step != 0) pass(c, f_08914BD8, 0x089146DCu);
        else pass(c, f_08914B60, 0x089146C8u);
    } else {
        int32_t second = S8(o + 193);
        *B(o + 696) = 0;
        gaddr table = second != 0 ? 0x08A5FEC8u : 0x08A5FEB8u;
        gaddr handler = *W(table + step * 4);
        c->r[5] = handler; c->r[6] = table;
        c->r[31] = second != 0 ? 0x089146A8u : 0x08914684u;
        rt_call_indirect(c, handler);
    }
    leave0(c, ra);
}

/* (fighter): with no punch down ticked; with one: a drop (0x08910BE0) when +238 or +696 is set or the
   throw check fails, else the hop (0x089108D4). */
SFA_FUNC(08910830, punch_drop_or_hop) {
    gaddr o = ARG(0); uint32_t sv[2];
    uint32_t b = ((uint32_t)(*H(o + 760) | *H(o + 762)) & 0xFFFF) & 0x70;
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; c->r[16] = o;
    *W(c->r[29] + 20) = sv[1];
    if (b == 0) call1(c, o, f_08861848, 0x08910864u);
    else if (S8(o + 238) != 0) call1(c, o, f_08910BE0, 0x0891087Cu);
    else if (S8(o + 696) != 0) call1(c, o, f_08910BE0, 0x08910894u);
    else {
        call2(c, o, 0x5F, f_0886B058, 0x089108A4u);
        if (c->r[2] != 0) call1(c, o, f_089108D4, 0x089108B4u);
        else call1(c, o, f_08910BE0, 0x089108C4u);
    }
    leave(c, sv);
}
