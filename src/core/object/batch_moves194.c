/* Steps of 63 - 64 instructions: the menu's countdown voice (0x08855DBC, 17,798 calls), the back-flip
   by the character's jump table (0x0885B1A0), the win-pose pick by held button (0x08863504), throw holds
   that place the opponent from the throw table (0x0886DDCC) or the fighter from the opponent
   (0x0886DEC8), a wait that lands into a move, a part character's jump by strength and a menu's
   left / right choice (0x088A26CC). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_0887C4CC(Cpu *c);
void f_08863600(Cpu *c); void f_08867060(Cpu *c); void f_0886E238(Cpu *c); void f_0896F5F8(Cpu *c);
void f_0896F678(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D3848(Cpu *c); void f_088D2E54(Cpu *c);
void f_088D2E04(Cpu *c); void f_088A507C(Cpu *c); void f_088A21C4(Cpu *c); void f_088A4E90(Cpu *c);

#define GAME  0x08E75910u
#define MATCH 0x092E1228u

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void voice(Cpu *c, uint32_t n, uint32_t back) { ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c); }

/* (menu item): the +7 wait runs down; when it runs out, the voice by +79: 0 the announcer (0x10C with
   the match's +63, else 0x100 + the game's +32 bits 0 - 2), 1 from 0x08A16E0A by +32 bits 0 - 1, 2 - 8
   from 0x08A16DFC. */
SFA_FUNC(08855DBC, menu_countdown_voice) {
    gaddr o = ARG(0); uint32_t ra;
    uint32_t t = *B(o + 7);
    frame_in(c, &ra);
    if (t != 0) {
        *B(o + 7) = (uint8_t)(t - 1);
        if (((t - 1) & 0xFF) == 0) {
            uint32_t k = *B(o + 79);
            if (k == 0) {
                if (S8(MATCH + 63) != 0) voice(c, 0x10C, 0x08855E10u);
                else voice(c, (*B(GAME + 32) & 7) + 0x100, 0x08855E38u);
            } else if (k == 1) voice(c, *H(0x08A16E0Au + ((*B(GAME + 32) & 3) << 1)), 0x08855E7Cu);
            else if (k < 9) voice(c, *H(0x08A16DFCu + k + k - 4), 0x08855EACu);
        }
    }
    frame_out(c, ra);
}

/* (fighter): ended: a back-flip from its jump table (+708, by its character +338: x, y, x brake, fall,
   in 1/256 steps; x brake unscaled for characters 0x11 / 0x13 with no choice made), turned around (x
   speeds negated when it then faces left) into move 0x13; else ticked. */
SFA_FUNC(0885B1A0, fighter_backflip) {
    gaddr o = ARG(0); uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) & 0x80) {
        int32_t st = S8(o + 338);
        gaddr p = *W(o + 708) + ((uint32_t)st << 5) + 0x10;
        *B(o + 7) = 1;
        *W(o + 96) = (uint32_t)S16(p) << 8;
        *W(o + 100) = (uint32_t)S16(p + 2) << 8;
        if (S8(o + 386) == -1 && (st == 0x11 || st == 0x13)) *W(o + 104) = (uint32_t)S16(p + 4);
        else *W(o + 104) = (uint32_t)S16(p + 4) << 8;
        uint32_t f = *B(o + 11);
        *W(o + 108) = (uint32_t)S16(p + 6) << 8;
        f ^= 1;
        *B(o + 11) = (uint8_t)f;
        if ((f & 0xFF) != 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
        call2(c, o, 0x13, f_0886163C, 0x0885B27Cu);
    } else call1(c, o, f_08861848, 0x0885B290u);
    frame_out(c, ra);
}

/* (fighter): the win pose (state 1 / 4 / 0 / 0, +573 = 1, a pending +251 / +252 cleared), 0x08863600,
   the pose (+692) by the first held button (+752 bits 4 - 9: 1 - 6, none or locked 0), facing the
   opponent, move 0 and 0x08867060. */
SFA_FUNC(08863504, fighter_win_pose_pick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    *B(o + 4) = 1; *B(o + 5) = 4; *B(o + 6) = 0; *B(o + 7) = 0;
    c->r[29] -= 0x20;
    int32_t pend = S8(o + 251);
    *W(c->r[29] + 20) = s1; c->r[17] = 0;
    *B(o + 573) = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (pend != 0) { *B(o + 251) = 0; *H(o + 252) = 0; }
    call1(c, o, f_08863600, 0x08863554u);
    uint32_t pose = 0;
    if (S8(o + 373) == 0) {
        uint32_t h = *H(o + 752);
        if (h & 0x3F0) {
            for (uint32_t i = 0; i < 6; i++) if (h & (0x10u << i)) { pose = i + 1; break; }
        }
    }
    c->r[17] = pose;
    *B(o + 692) = (uint8_t)pose;
    *B(o + 11) = (uint8_t)S8(o + 697);
    call2(c, o, 0, f_0886163C, 0x088635E4u);
    call1(c, o, f_08867060, 0x088635ECu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The throw table entry for the held opponent (+416 +36, by its state +338, frame +296 +8): x / y
   offsets, then the pose word and the picture. */
static gaddr throw_entry(gaddr o, gaddr opp) {
    uint32_t fr = *B(*W(o + 296) + 8);
    int32_t st = S8(opp + 338);
    gaddr tab = *W(*W(*W(o + 416) + 36) + ((uint32_t)st << 2));
    int32_t i = (int16_t)(fr << 2);
    return tab + (uint32_t)(i + i);
}
static void throw_pose(Cpu *c, gaddr o, gaddr opp, gaddr p, uint32_t back) {
    *B(opp + 261) = S16(p) < 0 ? 0xFF : 0;
    uint32_t f = (uint32_t)(int16_t)(*B(o + 11) & 0xFF);
    *B(opp + 11) = (uint8_t)(f ^ ((uint32_t)S16(p) & 3));
    ARG(0) = opp; ARG(1) = (uint32_t)S16(p + 2) & 0xFFFF;
    c->r[31] = back; f_0886E238(c);
}

/* (fighter): holding a throw (+135): the opponent placed at the entry's offset from us (x mirrored by our
   facing), then posed. */
SFA_FUNC(0886DDCC, throw_hold_place_opp) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (S8(o + 135) != 0) {
        gaddr e = throw_entry(o, opp);
        int32_t dy = S16(e + 2), dx = S16(e);
        int32_t x = S16(o + 18);
        *H(opp + 18) = (uint16_t)(*B(o + 11) != 0 ? x - dx : x + dx);
        *H(opp + 22) = (uint16_t)(S16(o + 22) + dy);
        throw_pose(c, o, opp, e + 4, 0x0886DEBCu);
    }
    frame_out(c, ra);
}

/* (fighter): holding a throw (+135): we are placed at the entry's offset from the opponent, which is
   then posed. */
SFA_FUNC(0886DEC8, throw_hold_place_self) {
    gaddr o = ARG(0); uint32_t ra;
    gaddr opp = *W(o + 92);
    frame_in(c, &ra);
    if (S8(o + 135) != 0) {
        gaddr e = throw_entry(o, opp);
        int32_t dy = S16(e + 2), dx = S16(e);
        int32_t x = S16(opp + 18);
        *H(o + 18) = (uint16_t)(*B(o + 11) != 0 ? x + dx : x - dx);
        *H(o + 22) = (uint16_t)(S16(opp + 22) - dy);
        throw_pose(c, o, opp, e + 4, 0x0886DFB8u);
    }
    frame_out(c, ra);
}

/* (fighter): when +238 holds its facing's bit or the +83 wait runs out: step 8, +83 = 14 and move 7 + 2 x
   strength; else a pending +123 (cleared) counts +82 down and past 0 step 7 (+73 / +234 = 1,
   0x0896F678, ticked); otherwise 0x0896F5F8 and ticked. */
SFA_FUNC(0896DCB8, move_wait_land_7_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    gaddr w = o + 0x52;
    int bit = (S8(o + 238) & (1 << (*B(o + 11) & 31))) != 0;
    enter(c, o, sv);
    int land = bit;
    if (!land) {
        int8_t t = (int8_t)(S8(w + 1) - 1);
        *B(w + 1) = (uint8_t)t;
        land = t < 0;
    }
    if (land) {
        *B(o + 7) = 8; *B(w + 1) = 0xE;
        call2(c, o, (uint32_t)((S8(o + 194) << 1) + 7) & 0xFFFF, f_088616EC, 0x0896DDA4u);
    } else {
        if (S8(o + 123) != 0) {
            *B(o + 123) = 0;
            int8_t u = (int8_t)(S8(w) - 1);
            *B(w) = (uint8_t)u;
            if (u < 0) {
                *B(o + 7) = 7; *B(o + 73) = 1; *B(o + 234) = 1;
                call1(c, o, f_0896F678, 0x0896DD44u);
                call1(c, o, f_08861848, 0x0896DD4Cu);
                leave(c, sv);
                return;
            }
        }
        call1(c, o, f_0896F5F8, 0x0896DD64u);
        call1(c, o, f_08861848, 0x0896DD6Cu);
    }
    leave(c, sv);
}

/* (character): the part ticked; a frame flag steps +7 on (+73 = 1), 0x088D3848 and a jump by strength
   (x 3.75 / 4.75 / 6.0, y 6.0 / 6.5 / 8.0, fall -0.4375 / -0.4375 / -0.5; x toward the facing), then
   0x088D2E54 (0x3C) and bank cue 0xB. */
SFA_FUNC(089B5B7C, part_flag_jump_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089B5B90u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1); *B(o + 73) = 1;
        call1(c, o, f_088D3848, 0x089B5BB8u);
        int32_t s = S8(o + 194);
        if (s == 0) { *W(o + 96) = 0x3C000; *W(o + 100) = 0x60000; *W(o + 108) = 0xFFFF9000u; }
        else if (s == 1) { *W(o + 96) = 0x4C000; *W(o + 100) = 0x68000; *W(o + 108) = 0xFFFF9000u; }
        else if (s == 2) { *W(o + 96) = 0x60000; *W(o + 100) = 0x80000; *W(o + 108) = 0xFFFF8000u; }
        uint32_t f = *B(o + 11);
        *W(o + 104) = 0;
        if (f == 0) *W(o + 96) = 0u - *W(o + 96);
        call2(c, o, 0x3C, f_088D2E54, 0x089B5C5Cu);
        call2(c, o, 0xB, f_088D2E04, 0x089B5C68u);
    }
    leave(c, sv);
}

/* (menu, value at ARG2): the pad (0x088A507C); a confirm (0x088A21C4) or a button (bits 4 - 9) moves it
   on (+5 counts, +82 = 10, 0x088A4E90); left / right (bits 2 / 3) cycle the value through 0 - 2 with
   the cursor sound 0x38. */
SFA_FUNC(088A26CC, menu_cycle_3) {
    gaddr o = ARG(0);
    gaddr val = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; c->r[18] = o;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[16] = val;
    c->r[31] = 0x088A26ECu; f_088A507C(c);
    uint32_t b = c->r[2];
    c->r[17] = b;
    call1(c, o, f_088A21C4, 0x088A26F8u);
    int conf = c->r[2] != 0;
    if (conf || ((b & 0x3F0) & 0xFFFF) != 0) {
        uint32_t n = (uint32_t)*B(o + 5) + 1;
        *B(o + 5) = (uint8_t)n;
        *H(o + 82) = 0xA;
        ARG(0) = n; ARG(1) = 0xA;
        c->r[31] = conf ? 0x088A2718u : 0x088A2744u;
        f_088A4E90(c);
    } else if ((b & 0xC) & 0xFFFF) {
        int32_t v = S8(val);
        uint32_t nv;
        if (b & 8) nv = v != 0 ? (uint32_t)(v - 1) & 0xFFFF : 2;
        else nv = v < 2 ? (uint32_t)(v + 1) & 0xFFFF : 0;
        *B(val) = (uint8_t)nv;
        voice(c, 0x38, 0x088A27B4u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
