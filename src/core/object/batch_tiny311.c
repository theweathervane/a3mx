/* More small ones whose callees are all done: the two boss partners' set-up, a cursor's moves, a
   stage's height, the stored voice, an object's copy, and more moves' starts and steps. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_08824AA0(Cpu *c); void f_0887C4CC(Cpu *c); void f_088D7C1C(Cpu *c); void f_08821368(Cpu *c);
void f_08838D48(Cpu *c); void f_08822EAC(Cpu *c); void f_089CCEC0(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c); void f_0886D678(Cpu *c); void f_0886C9F0(Cpu *c);
void f_0886C7E4(Cpu *c); void f_0886C2B4(Cpu *c); void f_0886DD68(Cpu *c); void f_0886BF84(Cpu *c);
void f_08828C04(Cpu *c); void f_088616EC(Cpu *c); void f_08837098(Cpu *c); void f_0887C408(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
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

/* (x, y): the first time (x + 8): y's +15 and x's +72 and +43 cleared, the first fighter of the
   side x's +57 says is hidden, and fighters 3 and 4 become the characters 0x1E and 0x1F with the
   choice 0; in mode 7 (x + 114) their +413 is set, y's +107 cleared and the two words at 0x095179F0
   cleared. Afterwards 0x08824AA0. */
SFA_FUNC(08825130, partners_setup) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t n = *H(x + 8);
    *W(c->r[29] + 16) = ra;
    c->r[7] = n; c->r[6] = n + 1;
    if (n != 0) call0(c, f_08824AA0, 0x088251E4u);
    else {
        *H(x + 8) = 1;
        *B(y + 15) = 0;
        *B(x + 72) = 0;
        int32_t side = S8(x + 57) & 1;
        *B(x + 43) = 0;
        *B((side ? FIGHTER_2 : FIGHTER_1) + 1) = 0;
        *B(FIGHTER_3 + 338) = 0x1E; *B(FIGHTER_4 + 338) = 0x1F;
        *B(FIGHTER_3 + 386) = 0; *B(FIGHTER_4 + 386) = 0;
        if (S8(x + 114) == 7) {
            *B(FIGHTER_3 + 413) = 1; *B(FIGHTER_4 + 413) = 1;
            *B(y + 107) = 0;
            *H(0x095179E0u + 18) = 0; *H(0x095179E0u + 16) = 0;
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): the cursor at 0x08B01F34 moves on a grid four wide: one way it goes back three from the
   end of a row, back one from 10 and otherwise on one; the other way on three from a row's start,
   on one from 9 and otherwise back one. Below 8 the sound 0x38 plays (0x0887C4CC 0, 0x38, 0xC0).
   The object's +79 is the byte for it from the table 0x08A0E778. */
static void cursor_move(Cpu *c, int forward, uint32_t back) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    c->r[17] = 0x08B00000u;
    int32_t v = S8(0x08B01F34u);
    int32_t col = v % 4;
    if (forward) v = col == 3 ? v - 3 : v == 10 ? v - 1 : v + 1;
    else v = col == 0 ? v + 3 : v == 9 ? v + 1 : v - 1;
    *B(0x08B01F34u) = (uint8_t)v;
    v = S8(0x08B01F34u);
    if (v < 8) {
        call3(c, 0, 0x38, 0xC0, f_0887C4CC, back);
        v = S8(0x08B01F34u);
    }
    *B(o + 79) = *B(0x08A0E778u + (uint32_t)v);
    leave2(c, sv);
}
SFA_FUNC(0882C410, cursor_next) { cursor_move(c, 1, 0x0882C4A4u); }
SFA_FUNC(0882C558, cursor_prev) { cursor_move(c, 0, 0x0882C5E8u); }

/* (x, n): x's +98 is its first word, and +100 a byte from a table: below 7 at +79 the table
   0x08A0F604 by n (a random one of the 0x29 when n is 0x29 and 0x088D7C1C says 0), else the table
   0x08A0F62E by +79 less 7. A 0x10 frame keeping s0..s2. */
SFA_FUNC(08831A78, pick_table_byte) {
    gaddr x = ARG(0);
    int32_t n = (int16_t)ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0; *W(sp + 4) = s1;
    c->r[16] = x;
    int32_t v = S8(x + 79);
    *H(x + 98) = (uint16_t)S16(x);
    *W(sp + 8) = s2; *W(sp + 12) = ra;
    c->r[17] = (uint32_t)n;
    if (v < 7) {
        ARG(0) = (uint32_t)v; ARG(1) = 1;
        call0(c, f_088D7C1C, 0x08831AB4u);
        if (c->r[2] == 0) {
            c->r[18] = 0x29;
            if (n == 0x29) {
                call0(c, f_08821368, 0x08831AD0u);
                DIV(c, c->r[2], 0x29);
                n = (int16_t)c->hi;
                c->r[17] = (uint32_t)n;
            }
        }
        *H(x + 100) = (uint16_t)S8(0x08A0F604u + (uint32_t)n);
    } else *H(x + 100) = (uint16_t)S8(0x08A0F62Eu + (uint32_t)v - 7);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x10;
}

/* (fighter, which): its +815 says whether there is a stored block for which (0 to 2: the words at
   0x09517A78 + 924, + 928, + 932); one there is goes to 0x08838D48 with the fighter's own place
   (0x0951CE68, or 0x0951D910 for the second side). */
SFA_FUNC(08838C88, stored_block_start) {
    gaddr o = ARG(0);
    uint32_t which = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    const gaddr t = 0x09517A78u;
    *W(c->r[29] + 16) = ra;
    gaddr place = S8(o + 337) != 0 ? 0x0951D910u : 0x0951CE68u;
    c->r[6] = t; c->r[7] = place;
    *B(o + 815) = 0;
    if (which < 3) {
        static const uint32_t back[3] = { 0x08838CD8u, 0x08838D0Cu, 0x08838D3Cu };
        if (which != 0) { c->r[8] = 1; c->r[9] = 2; }
        if (*W(t + 924 + which * 4) != 0) {
            *B(o + 815) = 1;
            call2(c, place, *W(t + 924 + which * 4), f_08838D48, back[which]);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (owner): an object (0x08822EAC 0x40 / 0) that copies the owner's place, facing, +24, +86, +164,
   +12, +38, +14 and +171, with the owner's colour (+13) three on; for the character 0x29 four on
   and 0x089CCEC0. */
SFA_FUNC(08882634, make_copy_object) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call2(c, 0x40, 0, f_08822EAC, 0x08882650u);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 16) = *W(o + 16); *W(n + 20) = *W(o + 20);
        *B(n + 11) = *B(o + 11);
        *H(n + 24) = *H(o + 24); *H(n + 86) = *H(o + 86);
        *W(n + 164) = *W(o + 164);
        *B(n + 12) = *B(o + 12);
        *H(n + 38) = *H(o + 38);
        *W(n + 88) = o;
        *B(n + 14) = *B(o + 14);
        *B(n + 171) = *B(o + 171);
        int32_t who = S8(o + 338);
        uint32_t colour = *B(o + 13);
        c->r[16] = colour;
        if (who == 0x29) {
            *B(n + 13) = (uint8_t)(colour + 4);
            c->r[6] = 0x29;
            call2(c, n, colour + 4, f_089CCEC0, 0x088826CCu);
        } else *B(n + 13) = (uint8_t)(colour + 3);
    }
    leave(c, sv);
}

/* (fighter): a move's two steps. First: facing as +697 says, +235 set, 0x08863114, 0x0886CA2C with
   the strength, and the animation twice the strength, one more when far: for the cpu (and a player
   whose choice is not above 0) further than the word from 0x08A58262 by strength, for the others
   with back not held. Afterwards 0x0886D678. */
SFA_FUNC(08909110, move_near_or_far) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (*B(o + 7) != 0) call1(c, o, f_0886D678, 0x089091C8u);
    else {
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 7) = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 235) = 1;
        call1(c, o, f_08863114, 0x08909148u);
        call2(c, o, (uint32_t)(int32_t)S8(o + 194), f_0886CA2C, 0x08909154u);
        int32_t strength = S8(o + 194);
        uint32_t anim = (uint32_t)strength << 1;
        int far;
        if (S8(o + 373) == 0 && S8(o + 386) > 0) far = !(*H(o + 290) & 1);
        else far = (int32_t)*H(0x08A58262u + (uint32_t)(strength * 2)) < S16(o + 508);
        if (far) anim += 1;
        call2(c, o, anim & 0xFFFF, f_08861674, 0x089091B8u);
    }
    leave(c, sv);
}

/* (fighter): a super move's start like the others (never with +634; on the ground 0x0886C9F0 and
   0x0886C7E4, in the air +251 clear and 0x0886C2B4), the move 8 from state 1 / 0 / 8 / 0; this one
   faces as +697 says in the air too. Returns whether. */
SFA_FUNC(08915024, super_start_15024) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok = S8(o + 634) == 0;
    if (ok) {
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, 0x08915050u);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, 0x08915060u);
                ok = c->r[2] != 0;
            }
            if (ok) *B(o + 11) = (uint8_t)S8(o + 697);
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, 0x089150C4u);
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
        call1(c, o, f_0886DD68, 0x089150A0u);
    }
    c->r[2] = (uint32_t)ok;
    leave(c, sv);
}

/* (fighter): with +251 or +254 set and 0x0886BF84 allowing: the move 7 / 0 from state 1 / 0 / 7 /
   0, facing as +697 says, +196..+199 cleared, and 0x08828C04. Returns whether. */
SFA_FUNC(08919264, move7_if_flag_19264) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    c->r[17] = 0;
    if (S8(o + 251) == 0 && S8(o + 254) == 0) { c->r[2] = 0; leave2(c, sv); return; }
    call1(c, o, f_0886BF84, 0x08919298u);
    if (c->r[2] != 0) {
        *B(o + 236) = 7; *B(o + 235) = 0;
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 199) = 0; *B(o + 198) = 0; *B(o + 197) = 0;
        c->r[17] = 1;
        *B(o + 196) = 0;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_08828C04, 0x08919304u);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}

/* (fighter, n): the next step with the animation n + 0x14: it turns to the opponent and leaps
   back from them: 3.5 up with gravity 0.25, 7.0 across slowing by 0.25; then 0x0886CA2C (6) and
   0x08837098. */
SFA_FUNC(08922B08, leap_back_from) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1) & 0xFFFF;
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, (n + 0x14) & 0xFFFF, f_088616EC, 0x08922B38u);
    gaddr other = *W(o + 92);
    *B(o + 11) = S16(other + 18) < S16(o + 18) ? 0 : 1;
    *W(o + 100) = 0x38000;
    *W(o + 108) = 0xFFFFC000u;
    if (*B(o + 11) != 0) { *W(o + 96) = 0xFFF90000u; *W(o + 104) = 0x4000; }
    else { *W(o + 96) = 0x70000; *W(o + 104) = 0xFFFFC000u; }
    call2(c, o, 6, f_0886CA2C, 0x08922BA8u);
    call1(c, o, f_08837098, 0x08922BB0u);
    leave(c, sv);
}

/* (fighter): a super move's start of the usual shape, the move 1. A frame keeping s0, s1. */
SFA_FUNC(0892FEC0, super_start_2fec0) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int ok = S8(o + 634) == 0;
    if (ok) {
        if (S8(o + 73) == 0) {
            call1(c, o, f_0886C9F0, 0x0892FEF0u);
            ok = c->r[2] != 0;
            if (ok) {
                call1(c, o, f_0886C7E4, 0x0892FF00u);
                ok = c->r[2] != 0;
            }
            if (ok) *B(o + 11) = (uint8_t)S8(o + 697);
        } else {
            ok = S8(o + 251) == 0;
            if (ok) {
                call1(c, o, f_0886C2B4, 0x0892FF24u);
                ok = c->r[2] != 0;
            }
        }
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8;
        c->r[17] = 1;
        *B(o + 7) = 0; *B(o + 236) = 1;
        call1(c, o, f_0886DD68, 0x0892FF54u);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}

/* (fighter): on the ground, when 0x0886BF84 allows, the move 7 facing as +697 says; in the air,
   not on the way down and when 0x0886C2B4 allows, the move 9. From state 1 / 0 / 7 / 0, then
   0x08828C04. Returns whether. */
SFA_FUNC(0892A878, move7_or_air9) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, o, sv);
    int ok;
    if (S8(o + 73) == 0) {
        call1(c, o, f_0886BF84, 0x0892A89Cu);
        ok = c->r[2] != 0;
        if (ok) {
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 236) = 7;
            *B(o + 11) = (uint8_t)facing;
        }
    } else {
        ok = !((int32_t)*W(o + 100) < 0);
        if (ok) {
            call1(c, o, f_0886C2B4, 0x0892A900u);
            ok = c->r[2] != 0;
        }
        if (ok) *B(o + 236) = 9;
    }
    if (ok) {
        c->r[17] = 1; c->r[5] = 1;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        call1(c, o, f_08828C04, 0x0892A8D4u);
    }
    c->r[2] = (uint32_t)ok;
    leave2(c, sv);
}

/* (fighter): in the air more than 16 above the ground, when 0x0886C2B4 allows: the move 0xE / 1
   from state 1 / 0 / 7 / 0, facing as +697 says, +196..+199 cleared, with +78 = -1 with back held
   and 1 without. Returns 0 when it started, else 1. */
SFA_FUNC(089351EC, air_move_e) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_0886C2B4, 0x08935200u);
    int ok = c->r[2] != 0 && S8(o + 73) != 0 &&
             (int32_t)((uint32_t)(S16(o + 132) + 0x10) & 0xFFFF) < (int32_t)*H(o + 22);
    if (ok) {
        *B(o + 78) = (*H(o + 752) & 1) ? 0xFF : 1;
        *B(o + 236) = 0xE;
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        *B(o + 235) = 1;
    }
    c->r[2] = (uint32_t)!ok;
    leave(c, sv);
}

/* (fighter): unless +542 is set while +251 is clear, when 0x0886BF84 allows: the move 3 / 0 from
   state 1 / 0 / 7 / 0, facing as +697 says, +196..+199 cleared, with +78 = -1 with back held and
   1 without. Returns 0 when it started, else 1. */
SFA_FUNC(089352B4, move3_or_not) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok = S8(o + 251) != 0 || S8(o + 542) == 0;
    if (ok) {
        call1(c, o, f_0886BF84, 0x089352E0u);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 236) = 3;
        *B(o + 4) = 1; *B(o + 5) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0;
        uint32_t stick = *H(o + 752);
        *B(o + 199) = 0;
        *B(o + 235) = 0;
        *B(o + 78) = (stick & 1) ? 0xFF : 1;
    }
    c->r[2] = (uint32_t)!ok;
    leave(c, sv);
}

/* (fighter): the next step: 0x0886CA2C (4), the cues 0x30 and 0x209, no fall, +82 = 7 and +83 from
   the byte table 0x08A7098C by +78, 3.0 forward with 0.5 more each frame, and the animation 0x22 + +78. */
SFA_FUNC(08938998, slide_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    call2(c, o, 4, f_0886CA2C, 0x089389BCu);
    call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x30, f_0887C408, 0x089389CCu);
    call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x209, f_0887C408, 0x089389DCu);
    *W(o + 108) = 0; *W(o + 100) = 0;
    *B(o + 82) = 7;
    *B(o + 83) = (uint8_t)S8(0x08A7098Cu + (uint32_t)(int32_t)S8(o + 78));
    if (*B(o + 11) != 0) { *W(o + 96) = 0x30000; *W(o + 104) = 0x8000; }
    else { *W(o + 96) = 0xFFFD0000u; *W(o + 104) = 0xFFFF8000u; }
    call2(c, o, (uint32_t)(S8(o + 78) + 0x22) & 0xFFFF, f_088616EC, 0x08938A4Cu);
    leave(c, sv);
}
