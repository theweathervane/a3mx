/* Small functions the scenarios reach, 37 - 38 instructions: fighter objects' starts, an effect step
   by character, the select screen's textures freed, a menu cursor's move. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_0886C6EC(Cpu *c);
void f_089E0540(Cpu *c); void f_089E0C24(Cpu *c); void f_08822F1C(Cpu *c); void f_088AC360(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

static void on_7(gaddr o) { *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; }

/* (object, k): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k; +11 = +697) and
   0x0886DBE8; 1 then, else 0. */
SFA_FUNC(0896C58C, object_start_k_dbe8b) {
    gaddr o = ARG(0);
    uint32_t k = (uint32_t)(int32_t)(int8_t)ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[17] = k; c->r[16] = o;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[31] = 0x0896C5B0u; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o);
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[18] = 1;
        *B(o + 235) = 1;
        *B(o + 236) = (uint8_t)k;
        call_on(c, o, 0x0896C5E8u, f_0886DBE8);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 clear, a full gauge (+366 from 144) and 0x0886C6EC allowing it, a super move 9
   (+4 = 1, +6 = 8, +235 = 1, +194 / +552 = 2, +236 = 9, +11 = +697); 0x0886C6EC's answer then, else
   0. */
SFA_FUNC(0899EEC4, object_super_9b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int ok = 0;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 144)) {
        c->r[31] = 0x0899EEF4u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 9;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 235) = 1; *B(o + 194) = 2; *B(o + 552) = 2;
            ok = 1;
        }
    }
    if (!ok) c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): with +386 not negative and 0x0886BF84 allowing it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 7;
   +196 cleared, +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(089A60A4, object_start_386_7) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[2] = 0;
    if (S8(o + 386) >= 0) {
        c->r[31] = 0x089A60C8u; f_0886BF84(c);
        if (c->r[2] != 0) {
            on_7(o);
            *B(o + 236) = 7;
            c->r[17] = 1;
            *B(o + 235) = 1;
            *B(o + 11) = (uint8_t)S8(o + 697);
            *B(o + 196) = 0;
            call_on(c, o, 0x089A6108u, f_0886DB94);
            c->r[2] = 1;
        } else c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): its step by +7 from 0x08AA7000 for a fighter (+92) of character 0x10, 0x08AA7014 for 0xB,
   else 0x08AA6FF4. */
SFA_FUNC(089B43C4, effect_step_char10_0b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t ch = S8(*W(o + 92) + 338);
    uint32_t k = (uint32_t)*B(o + 7) << 2;
    gaddr table = ch == 0x10 ? 0x08AA7000u : ch == 0xB ? 0x08AA7014u : 0x08AA6FF4u;
    gaddr h = *W(table + k);
    ARG(1) = h;
    c->r[31] = ch == 0x10 ? 0x089B43FCu : ch == 0xB ? 0x089B442Cu : 0x089B444Cu;
    rt_call_indirect(c, h);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* The select screen's textures freed: every 12-byte entry of list 34's tile list (0x08A050B0 +136:
   its +8 texture, 0x089E0C24), then the buffer at 0x08A05848 (0x089E0540) let go. */
SFA_FUNC(088152B0, select_textures_free) {
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    gaddr list = *W(0x08A050B0u + 136);
    c->r[19] = list;
    *W(sp + 24) = sv[2];
    c->r[18] = 0;
    *W(sp + 16) = sv[0];
    c->r[16] = 0x08A05834u;
    *W(sp + 20) = sv[1]; *W(sp + 32) = ra;
    if (c->r[18] < *W(list + 4)) {
        for (c->r[17] = 0; c->r[18] < *W(list + 4); c->r[17] += 12) {
            call_on(c, *W(*W(list) + c->r[17] + 8), 0x08815300u, f_089E0C24);
            c->r[18]++;
            if (!(c->r[18] < *W(list + 4))) { c->r[17] += 12; break; }
        }
    }
    if (*W(0x08A05834u + 20) != 0) {
        call_on(c, *W(0x08A05834u + 20), 0x08815328u, f_089E0540);
        *W(0x08A05834u + 20) = 0;
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (menu): when 0x088AC360 says the cursor moved, a list-0x7F type-3 object (0x08822F1C, +0 = 1), +80 =
   1, +17 on, +21 cleared and the three tickers' speeds set (0x08B027D2 / D6, 0x08B02802 / 06,
   0x08B02832 / 36). */
SFA_FUNC(088ABBA4, menu_cursor_moved) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    call_on(c, o, 0x088ABBB8u, f_088AC360);
    if (c->r[2] != 0) {
        call2(c, 0x7F, 3, 0x088ABBC8u, f_08822F1C);
        if (c->r[2] != 0) *B(c->r[2]) = 1;
        *B(o + 80) = 1;
        *B(o + 17) = (uint8_t)(*B(o + 17) + 1);
        *B(o + 21) = 0;
        *H(0x08B027C8u + 10) = 0x2D2D; *H(0x08B027C8u + 14) = 0xFD2E;
        *H(0x08B027F8u + 10) = 0x12D2; *H(0x08B027F8u + 14) = 0xFED3;
        *H(0x08B02828u + 10) = 0x2D2D; *H(0x08B02828u + 14) = 0xFD2E;
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7; +11 = +697): with +760 / +762 bits 4 - 6
   +236 = 2 and 0x0886DB94, else +236 = 9 and 0x0886DBE8, whose answer it gives; else 0. */
SFA_FUNC(08914DEC, object_start_by_760) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08914E00u; f_0886BF84(c);
    if (c->r[2] != 0) {
        on_7(o);
        *B(o + 11) = (uint8_t)S8(o + 697);
        if (((*H(o + 760) | *H(o + 762)) & 0x70) != 0) { *B(o + 236) = 2; call_on(c, o, 0x08914E74u, f_0886DB94); }
        else { *B(o + 236) = 9; call_on(c, o, 0x08914E48u, f_0886DBE8); }
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = 0xC with +386 negative,
   else 0; +11 = +697) and 0x0886DB94; 1 then, else 0. */
SFA_FUNC(08926100, object_start_386_c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = 0x08926118u; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 236) = S8(o + 386) < 0 ? 0xC : 0;
        on_7(o);
        c->r[17] = 1;
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)S8(o + 697);
        call_on(c, o, 0x08926180u, f_0886DB94);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
