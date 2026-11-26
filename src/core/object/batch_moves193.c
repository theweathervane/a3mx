/* Steps of 59 - 62 instructions: a part character's hold-to-charge (0x089B6274), a landing that slows
   the match into move 0x2F, an afterimage-casting dash (0x089B2370), a part arrival with a hop by
   strength, the round's intro effect (0x08983498), the three-part flash on a super start
   (0x08880568), a carry that drops the opponent (0x088EDF00) and a leap aimed at the opponent's
   height (0x08922794). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_088D3EC0(Cpu *c);
void f_088D2E04(Cpu *c); void f_0896F62C(Cpu *c); void f_0896F678(Cpu *c); void f_0887E560(Cpu *c);
void f_089B2A64(Cpu *c); void f_08822D2C(Cpu *c); void f_088D31EC(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D36D0(Cpu *c); void f_088D2DBC(Cpu *c); void f_088D3960(Cpu *c); void f_088D3180(Cpu *c);
void f_08822EAC(Cpu *c); void f_08821368(Cpu *c); void f_0886E3CC(Cpu *c); void f_0887C408(Cpu *c);
void f_08861BCC(Cpu *c); void f_08837098(Cpu *c);

#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (character): the part ticked; the first frame flag 1 marks +820 with bank cues 0x16 / 0x1B; while the
   strength's button (0x08AA70D8) is held (+290) +828 counts down, released +832 does; when both are out
   +7 steps on with bank cue 0x17 into move 0x15 + strength. */
SFA_FUNC(089B6274, part_hold_charge) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B6288u);
    if (*W(o + 820) == 0 && S8(o + 77) == 1) {
        *B(o + 77) = 0; *W(o + 820) = 1;
        call2(c, o, 0x16, f_088D2E04, 0x089B62B8u);
        call2(c, o, 0x1B, f_088D2E04, 0x089B62C4u);
    }
    int32_t s = S8(o + 194);
    uint32_t mask = *H(0x08AA70D8u + (uint32_t)(s + s));
    uint32_t a = *W(o + 828);
    if ((*H(o + 290) & mask) == 0) *W(o + 832) = 0;
    uint32_t b = *W(o + 832), left;
    if (a != 0) { *W(o + 828) = a - 1; left = (a - 1) | b; }
    else if (b == 0) left = 0;
    else { *W(o + 832) = b - 1; left = b - 1; }
    if (left == 0) {
        uint32_t st = *B(o + 7);
        *W(o + 832) = 0; *B(o + 7) = (uint8_t)(st + 1); *W(o + 828) = 0;
        call2(c, o, 0x17, f_088D2E04, 0x089B6340u);
        call2(c, o, (uint32_t)(S8(o + 194) + 0x15), f_088616EC, 0x089B6350u);
    }
    leave(c, sv);
}

/* (fighter): step 8 into move 0x2F (after 0x0896F678) when +238 holds its facing's bit, when a pending
   +123 (cleared) runs the +82 wait out, or on landing (0x0896F62C; at the floor, the match slowed: +35 =
   16, 0x0887E560); else ticked. */
SFA_FUNC(0896EAE8, move_land_slow_2f) {
    gaddr o = ARG(0); uint32_t sv[2];
    int bit = (S8(o + 238) & (1 << (*B(o + 11) & 31))) != 0;
    enter(c, o, sv);
    int go = 0;
    if (bit) go = 1;
    else if (S8(o + 123) != 0) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *B(o + 123) = 0;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) go = 1;
        else call1(c, o, f_08861848, 0x0896EB44u);
    } else {
        call1(c, o, f_0896F62C, 0x0896EB5Cu);
        int32_t floor = S16(o + 132);
        if ((floor & 0xFFFF) < (S16(o + 22) & 0xFFFF)) call1(c, o, f_08861848, 0x0896EBC8u);
        else {
            *H(o + 22) = (uint16_t)floor;
            *B(MATCH + 35) = 0x10;
            call1(c, o, f_0887E560, 0x0896EB94u);
            go = 1;
        }
    }
    if (go) {
        *B(o + 7) = 8;
        call1(c, o, f_0896F678, 0x0896EBA4u);
        call2(c, o, 0x2F, f_088616EC, 0x0896EBB0u);
    }
    leave(c, sv);
}

/* (fighter): without a frame flag +7 steps on, 0x089B2A64 (7), an afterimage (0x08822D2C 0x1D / 0: the
   fighter's picture, place, facing and side, +255 = 5, +346 = strength) and a dash of 13.0 + strength
   / -0.375; ticked. */
SFA_FUNC(089B2370, move_dash_afterimage) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k == 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, 7, f_089B2A64, 0x089B23A0u);
        ARG(0) = o; ARG(1) = 0x1D; ARG(2) = 0; c->r[31] = 0x089B23B0u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121);
            int32_t s = S8(o + 194);
            *B(e + 255) = 5; *B(e + 346) = (uint8_t)s; *B(e + 347) = 0;
            *B(e + 14) = *B(o + 14); *B(e + 11) = *B(o + 11);
            *H(e + 38) = *H(o + 38); *H(e + 18) = *H(o + 18);
            uint32_t y = *H(o + 22);
            *H(e + 112) = 0; *H(e + 22) = (uint16_t)y;
            *W(e + 88) = o;
            *H(e + 86) = (uint16_t)S8(o + 337);
            *W(e + 164) = *W(o + 164);
        }
        *W(o + 96) = ((uint32_t)S8(o + 194) << 16) + 0xD0000u;
        *W(o + 104) = 0xFFFFA000u;
    }
    call1(c, o, f_08861848, 0x089B2450u);
    leave(c, sv);
}

/* (character): the part ticked, 0x088D31EC; when 0x088D35D0 gives 1, +73 cleared, +7 steps on,
   0x088D36D0, cue 0x22, 0x088D3960 (0, 0), a hop (x 1.5, y 3.0 / 3.5 / 4.0 by strength, fall -0.125),
   0x088D3180, move 0xF + strength and +262 cleared. */
SFA_FUNC(089C8074, part_arrive_hop_f) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8088u);
    call1(c, o, f_088D31EC, 0x089C8090u);
    call1(c, o, f_088D35D0, 0x089C8098u);
    if (c->r[2] == 1) {
        uint32_t st = *B(o + 7);
        *B(o + 73) = 0; *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_088D36D0, 0x089C80BCu);
        call1(c, o, f_088D2DBC, 0x089C80C4u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089C80D4u; f_088D3960(c);
        int32_t s = S8(o + 194);
        if (s == 0) *W(o + 100) = 0x30000;
        else if (s == 1) *W(o + 100) = 0x38000;
        else if (s == 2) *W(o + 100) = 0x40000;
        *W(o + 96) = 0x18000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFE000u;
        call1(c, o, f_088D3180, 0x089C8140u);
        call2(c, o, (uint32_t)(S8(o + 194) + 0xF), f_088616EC, 0x089C8150u);
        *B(o + 262) = 0;
    }
    leave(c, sv);
}

/* (fighter): intro: at +6 = 0 (no clock +56 / +2 and +415 set) its effect (0x08822EAC 2 / its state)
   is tied both ways; +6 steps on; at +6 = 1 standing (with +1 = 1) once ended, else ticked. */
SFA_FUNC(08983498, move_intro_effect) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    uint32_t n = *B(o + 6);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (n == 0) {
        if (S16(MATCH + 56) == 0 && S16(MATCH + 2) == 0 && S8(o + 415) != 0) {
            ARG(0) = 2; ARG(1) = (uint32_t)S8(o + 338) & 0xFF; c->r[31] = 0x089834F4u; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e != 0) {
                *B(e + 1) = 1; *W(e + 88) = o;
                *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13); *B(e + 14) = *B(o + 14);
                *W(o + 88) = e;
            }
        }
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
    } else if (n == 1) {
        c->r[17] = 1;
        if (S8(o + 75) < 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0;
            call2(c, o, 0, f_0886163C, 0x08983574u);
            *B(o + 1) = 1;
        } else call1(c, o, f_08861848, 0x08983550u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): alive, three flash parts (0x08822EAC 9; kind 2 for character 0x25, else by its bit 0) tied
   to it with its state, index (+73) and guard from 0x08A249F8; then +703 = 1, 0x0886E3CC (6, 300) and
   cue 0x2D. */
SFA_FUNC(08880568, super_start_flash) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[4];
    for (int i = 0; i < 4; i++) s[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    int32_t life = S16(o + 112);
    for (int i = 0; i < 4; i++) *W(c->r[29] + 16 + 4 * i) = s[i];
    *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    if (life >= 0) {
        c->r[18] = 2;
        c->r[31] = 0x08880594u; f_08821368(c);
        uint32_t k = c->r[2];
        uint32_t kind = k == 0x25 ? 2u : (k & 1) ? 1u : 0u;
        c->r[18] = kind; c->r[19] = 0; c->r[17] = 0x08A249F8u;
        int ok = 1;
        for (uint32_t i = 0; i < 3; i++) {
            ARG(0) = 9; ARG(1) = kind; c->r[31] = 0x088805D0u; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e == 0) { ok = 0; break; }
            *B(e + 14) = *B(o + 14);
            int32_t st = S8(o + 338);
            *W(e + 88) = o;
            *H(e + 82) = (uint16_t)st;
            int32_t g = S8(0x08A249F8u + i);
            *B(e + 73) = (uint8_t)i; *B(e + 78) = (uint8_t)g;
            c->r[19] = i + 1;
            *B(e + 10) = 0;
        }
        if (ok) {
            *B(o + 703) = 1;
            ARG(0) = o; ARG(1) = 6; ARG(2) = 0x12C; c->r[31] = 0x08880634u; f_0886E3CC(c);
            ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = 0x2D; c->r[31] = 0x08880644u; f_0887C408(c);
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): carries the opponent (its x by +96 by +104, its y by +100 by +108); when it reaches the
   floor it is dropped there (x at the carried +96, guard cleared), our place's low halves cleared,
   +6 = 1, turned around and move 0x22; else the +82 wait runs down (0x08861BCC once out) and ticked. */
SFA_FUNC(088EDF00, move_carry_drop) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    gaddr opp = *W(o + 92);
    *W(opp + 16) = *W(opp + 16) + *W(o + 96);
    uint32_t vy = *W(o + 100);
    *W(o + 96) = *W(o + 96) + *W(o + 104);
    *W(opp + 20) = *W(opp + 20) + vy;
    int32_t floor = S16(o + 132);
    *W(o + 100) = *W(o + 100) + *W(o + 108);
    if (!(floor < S16(opp + 22))) {
        *W(opp + 20) = (uint32_t)floor << 16;
        uint32_t x = *W(o + 16), y = *W(o + 20);
        *W(o + 16) = x & 0xFFFF0000u; *W(o + 20) = y & 0xFFFF0000u;
        uint32_t ox = *W(opp + 96);
        *B(opp + 78) = 0; *W(opp + 16) = ox;
        uint32_t f = *B(o + 11);
        *B(o + 6) = 1; *B(o + 11) = (uint8_t)(f ^ 1);
        call2(c, o, 0x22, f_0886163C, 0x088EDFB4u);
    } else {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) call1(c, o, f_08861BCC, 0x088EDFE0u);
        call1(c, o, f_08861848, 0x088EDFE8u);
    }
    leave(c, sv);
}

/* (fighter): a leap at the opponent: +7 on, +656 / +123 cleared, 0x08837098, x 1/32 of the distance, y
   6.0 / -0.3125 plus 1/32 of the height gap (capped at 144) raised by the target height from the table
   by its state (0x08A66A6C down / 0x08A66ABC crouched / 0x08A66B0C standing) kept at +0x3A0; move
   0xA. */
SFA_FUNC(08922794, move_leap_at_height) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    *B(o + 656) = 0; *B(o + 7) = (uint8_t)(st + 1); *B(o + 123) = 0;
    *W(sp + 24) = ra;
    call1(c, o, f_08837098, 0x089227C0u);
    gaddr opp = *W(o + 92);
    uint32_t dx = *W(opp + 16) - *W(o + 16);
    *W(o + 104) = 0;
    *W(o + 96) = (uint32_t)((int32_t)dx >> 5);
    *W(o + 100) = 0x60000; *W(o + 108) = 0xFFFFB000u;
    gaddr tab = S8(opp + 73) != 0 ? 0x08A66A6Cu : S8(opp + 696) != 0 ? 0x08A66ABCu : 0x08A66B0Cu;
    gaddr h = o + 0x3A0;
    *H(h) = (uint16_t)S16(tab + (uint32_t)(S8(opp + 338) * 2));
    *W(sp + 16) = *W(opp + 20) - *W(o + 20);
    *H(sp + 18) = (uint16_t)(S16(sp + 18) + S16(h));
    if (0x900000u < *W(sp + 16)) *W(sp + 16) = 0x900000u;
    *W(o + 100) = *W(o + 100) + (uint32_t)((int32_t)*W(sp + 16) >> 5);
    call2(c, o, 0xA, f_088616EC, 0x0892287Cu);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
