/* Small functions the scenarios reach, 33 - 36 instructions: the four text lines drawn each frame,
   the select screen's colours copied, level objects tied to their owner's +139, fighters' move
   starts. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08812284(Cpu *c); void f_088D7E80(Cpu *c); void f_088E3550(Cpu *c); void f_088E3FE0(Cpu *c);
void f_08815478(Cpu *c); void f_089E11C8(Cpu *c); void f_0882324C(Cpu *c); void f_089B7B14(Cpu *c);
void f_089CCD1C(Cpu *c); void f_089CCDE8(Cpu *c); void f_0886C5D0(Cpu *c); void f_0886C2B4(Cpu *c);
void f_0886DBE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* Each frame the four 20-byte text lines at 0x092DF930 with text (+0) and characters (+1) are drawn
   (0x08812284); a line drops (+0 cleared, 0x088E3FE0) once neither 0x088E3550 nor 0x088D7E80 holds
   it. */
SFA_FUNC(0880F278, text_lines_draw) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    *W(c->r[29] + 16) = s0;
    *W(c->r[29] + 24) = ra;
    gaddr line = 0x092DF930u;
    for (uint32_t left = 4; left != 0; left--, line += 0x14) {
        c->r[16] = left; c->r[17] = line;
        if (*B(line) == 0 || *B(line + 1) == 0) continue;
        call_on(c, line, 0x0880F2B4u, f_08812284);
        c->r[31] = 0x0880F2BCu; f_088E3550(c);
        if (c->r[2] != 0) continue;
        c->r[31] = 0x0880F2CCu; f_088D7E80(c);
        if (c->r[2] != 0) continue;
        *B(line) = 0;
        c->r[31] = 0x0880F2DCu; f_088E3FE0(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* With a portrait chosen (0x08A05A9E above 0), its 0x100 colours (from (choice - 1) * 0x200 of list 39's
   palette buffer, made first via 0x08815478) copied into palette line 0x50 (0x089E11C8) and marked
   changed. */
SFA_FUNC(088155C0, select_portrait_colours) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    int32_t n = (int32_t)*B(0x08A058A0u + 510);
    c->r[17] = (uint32_t)n;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    if (n > 0) {
        gaddr k = 0x08A0577Cu;
        c->r[16] = k;
        uint32_t off = (((uint32_t)n << 8) - 0x100) & 0xFFFF;
        c->r[17] = off;
        if (*W(k + 156) == 0) { c->r[31] = 0x08815604u; f_08815478(c); }
        ARG(0) = 0x092E0380u; ARG(1) = *W(k + 156) + 2 * off; ARG(2) = 0x100;
        c->r[31] = 0x08815624u; f_089E11C8(c);
        *W(0x092E0980u) = 1;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): while it matches its owner's (+88) +139 it runs its step and keeps the owner's side and
   place; otherwise it is freed (0x0882324C). */
static void tied_to_owner(Cpu *c, void (*step)(Cpu *), uint32_t back_free, uint32_t back_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 139) != S8(p + 139)) call_on(c, o, back_free, f_0882324C);
    else {
        call_on(c, o, back_step, step);
        *B(o + 11) = *B(p + 11);
        *W(o + 16) = *W(p + 16);
        *W(o + 20) = *W(p + 20);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(089B7A2C, object_tied_level_1) { tied_to_owner(c, f_089B7B14, 0x089B7A58u, 0x089B7A74u); }
SFA_FUNC(089CCB20, object_tied_level_2) { tied_to_owner(c, f_089CCD1C, 0x089CCB4Cu, 0x089CCB68u); }
SFA_FUNC(089CCBA4, object_tied_level_3) { tied_to_owner(c, f_089CCDE8, 0x089CCBD0u, 0x089CCBECu); }

/* (fighter): a super (gauge +366 at 144 or more, +386 not above 0) once 0x0886C5D0 allows it: step
   1/0/8, +236 = 0xB, +11 = +697, +194 / +552 = 2; 1 then, else 0. */
SFA_FUNC(088F7684, fighter_super_start_b) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (!(S8(f + 386) > 0) && !(S16(f + 366) < 144)) {
        call_on(c, f, 0x088F76BCu, f_0886C5D0);
        if (c->r[2] != 0) {
            *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 8; *B(f + 7) = 0;
            *B(f + 236) = 0xB;
            *B(f + 11) = (uint8_t)S8(f + 697);
            *B(f + 194) = 2;
            c->r[17] = 1;
            *B(f + 552) = 2;
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): in the air (+73) and over 0x10 above its ground (+132), once 0x0886C2B4 allows it, an air
   move: +236 = 5, +235 = 1, step 1/0/7, +196 cleared; 1 then, else 0. */
SFA_FUNC(089A6138, fighter_air_move_start) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (S8(f + 73) != 0) {
        call_on(c, f, 0x089A6160u, f_0886C2B4);
        if (c->r[2] != 0 && S16(f + 132) + 0x10 < S16(f + 22)) {
            *B(f + 236) = 5;
            *B(f + 235) = 1;
            *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
            c->r[17] = 1;
            *B(f + 196) = 0;
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): 32 or more above its ground (+132), once 0x0886C2B4 allows it, step 1/0/7 with +236 =
   0xA, +696 cleared, +11 = +697, and 0x0886DBE8; 1 then, else 0. */
SFA_FUNC(08973880, fighter_high_move_start) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = ra;
    uint32_t r = 0;
    if (!(S16(f + 22) - S16(f + 132) < 32)) {
        call_on(c, f, 0x089738B0u, f_0886C2B4);
        if (c->r[2] != 0) {
            *B(f + 4) = 1; *B(f + 5) = 0; *B(f + 6) = 7; *B(f + 7) = 0;
            *B(f + 236) = 0xA;
            *B(f + 696) = 0;
            *B(f + 11) = (uint8_t)S8(f + 697);
            call_on(c, f, 0x089738E8u, f_0886DBE8);
            r = 1;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
