/* Small functions the scenarios reach, 59 - 61 instructions: after-images spread out behind an
   object, a thrown object's launch from its character's table, a two-stage effect with its own
   handlers, CPU fighters' list-0x2D / list-9 helpers, a projectile's start. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0886E3CC(Cpu *c); void f_08881F70(Cpu *c); void f_088215C4(Cpu *c); void f_088231C8(Cpu *c);
void f_08822D2C(Cpu *c); void f_089CB86C(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c);
void f_088F5A34(Cpu *c); void f_0891CEF0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (object): every 4th frame (GAME +32), 0x0886E3CC (7, 4) and 2 (3 with +77) after-images
   (0x08881F70) spread behind it by the x offsets at 0x08A49880 / 0x08A49884 (turned for +11). */
SFA_FUNC(088FABB8, object_after_images) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0];
    c->r[16] = o;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    if ((*B(GAME + 32) & 3) == 0) {
        ARG(0) = o; ARG(1) = 7; ARG(2) = 4;
        c->r[31] = 0x088FABF8u; f_0886E3CC(c);
        int32_t k = S8(o + 77) != 0 ? 2 : 1;
        c->r[19] = (uint32_t)k;
        c->r[18] = 0x08A49884u + 2 * k;
        c->r[17] = 0x08A49880u + 2 * k;
        for (;;) {
            call_on(c, o, 0x088FAC34u, f_08881F70);
            gaddr n = c->r[2];
            if (n == 0) break;
            int32_t dx = S8(o + 77) != 0 ? S16(c->r[18]) : S16(c->r[17]);
            if (*B(o + 11) != 0) dx = (int16_t)-dx;
            *H(n + 18) = (uint16_t)(S16(n + 18) + dx);
            c->r[19]--;
            c->r[18] -= 2; c->r[17] -= 2;
            if ((int32_t)c->r[19] < 0) break;
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (object, -, column): its x / y steps and changes (+96 / +100 / +104 / +108, << 8) from its
   character's (+338) row of the table at +708, the column by +78 (0 -> 0, 1 -> 8, -1 -> 0xC, else
   the one passed); for characters 0x11 / 0x13 with +386 = -1 the x change is not scaled. Turned
   for +11. */
SFA_FUNC(089BD068, object_launch_from_table) {
    gaddr o = ARG(0);
    int32_t v = S8(o + 78);
    uint32_t ch = (uint32_t)S8(o + 338);
    c->r[7] = ch;
    uint32_t col = c->r[6];
    if (v == 0) col = 0;
    else if (v == 1) col = 8;
    else if (v == -1) col = 0xC;
    gaddr t = *W(o + 708) + 2 * ((ch << 4) + col);
    *W(o + 96) = (uint32_t)S16(t) << 8;
    *W(o + 100) = (uint32_t)S16(t + 2) << 8;
    if (S8(o + 386) == -1 && (ch == 0x11 || ch == 0x13)) *W(o + 104) = (uint32_t)S16(t + 4);
    else *W(o + 104) = (uint32_t)S16(t + 4) << 8;
    uint32_t r6 = (uint32_t)S16(t + 6) << 8;
    *W(o + 108) = r6;
    c->r[6] = r6;
    if (*B(o + 11) != 0) {
        *W(o + 96) = (uint32_t)-(int32_t)*W(o + 96);
        *W(o + 104) = (uint32_t)-(int32_t)*W(o + 104);
    }
}

/* (effect): step 0 runs its kind's (+3) start (0x08A64884), +123 cleared; step 1 its kind's frame
   (0x08A648B4) unless halted (BATTLE +37) and is drawn unless +123, or with BATTLE +42 goes to step
   2; step 2 goes to 3; step 3 frees it (0x088231C8). */
SFA_FUNC(0891DEF0, effect_kind_handlers) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t st = *B(o + 4);
    if (st == 0) {
        *B(o + 4) = 1;
        *B(o + 123) = 0;
        uint32_t h = *W(0x08A64884u + 4 * *B(o + 3));
        c->r[5] = h;
        ARG(0) = o; c->r[31] = 0x0891DF38u; rt_call_indirect(c, h);
    } else if (st == 1) {
        if (S8(BATTLE + 42) != 0) *B(o + 4) = 2;
        else {
            if (S8(BATTLE + 37) == 0) {
                uint32_t h = *W(0x08A648B4u + 4 * *B(o + 3));
                c->r[5] = h;
                ARG(0) = o; c->r[31] = 0x0891DF90u; rt_call_indirect(c, h);
            }
            if (S8(o + 123) == 0) call_on(c, o, 0x0891DFA4u, f_088215C4);
        }
    } else if (st == 2) *B(o + 4) = 3;
    else if (st == 3) call_on(c, o, 0x0891DFD0u, f_088231C8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, flag): for the CPU (+251) without flag and with +662 under +286, a list-0x2D helper
   (0x08822D2C) kept at +228, on, pointing back at it (+88) with its +80 / +121 / +122 / +139 / +194 /
   +251 / +14 / +24 / side / +164, +662 (as +280), +3 / +112 cleared; the effect count (+536)
   counted and 0x089CB86C on the helper. */
SFA_FUNC(089CB748, fighter_cpu_helper_2d) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = f;
    if ((ARG(1) & 0xFF) == 0 && S8(f + 251) != 0 && S16(f + 662) < S16(f + 286)) {
        ARG(0) = f; ARG(1) = 0x2D; ARG(2) = 0;
        c->r[31] = 0x089CB78Cu; f_08822D2C(c);
        gaddr n = c->r[2];
        if (n != 0) {
            *W(n + 88) = f;
            *B(n + 3) = 0;
            *B(n + 139) = (uint8_t)S8(f + 139);
            *B(n + 194) = (uint8_t)S8(f + 194);
            *B(n + 121) = (uint8_t)S8(f + 121);
            *B(n) = 1;
            *B(n + 251) = (uint8_t)S8(f + 251);
            *H(n + 24) = *H(f + 24);
            *B(n + 14) = *B(f + 14);
            *H(n + 86) = (uint16_t)(int32_t)S8(f + 337);
            *W(n + 164) = *W(f + 164);
            *B(n + 122) = (uint8_t)S8(f + 122);
            *H(n + 112) = 0;
            *B(n + 121) = (uint8_t)S8(f + 121);
            *B(n + 80) = (uint8_t)S8(f + 80);
            *B(n + 280) = (uint8_t)S16(f + 662);
            *W(f + 228) = n;
            uint32_t cnt = (uint32_t)*B(f + 536) + 1;
            *B(f + 536) = (uint8_t)cnt;
            ARG(0) = n; ARG(1) = cnt;
            c->r[31] = 0x089CB828u; f_089CB86C(c);
        }
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (projectile): on (+4 = 1, +9 / +73 / y step cleared), its owner's (+88) +24 / +86 / +164, its
   pictures 0x08A440DC / 0x08A43FE2 (+128 / +200, 0x08876998), its strength's (+346) speed
   (0x08A45608) its way, 0x59 ahead and 0x36 up, +352 / +356 = 0x08A43E9C / 0x08A43EB8, +344 =
   strength + 4, 0x0887684C (strength) and 0x088F5A34. */
SFA_FUNC(088F5940, projectile_start) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = 1;
    gaddr p = *W(o + 88);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    *W(o + 128) = 0x08A440DCu;
    *W(o + 200) = 0x08A43FE2u;
    *W(c->r[29] + 20) = ra;
    c->r[5] = 0x08A440DCu; c->r[6] = 0x08A40000u;
    call_on(c, o, 0x088F5998u, f_08876998);
    int32_t k = S8(o + 346);
    *W(o + 100) = 0;
    *B(o + 73) = 0;
    uint32_t sp = *W(0x08A45608u + ((uint32_t)k << 2));
    int32_t dx = 0x59;
    if (*B(o + 11) == 0) { dx = -0x59; sp = (uint32_t)-(int32_t)sp; }
    *W(o + 96) = sp;
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x36);
    *W(o + 352) = 0x08A43E9Cu;
    *W(o + 356) = 0x08A43EB8u;
    *B(o + 344) = (uint8_t)(k + 4);
    ARG(0) = o; ARG(1) = (uint32_t)k & 0xFFFF;
    c->r[31] = 0x088F5A1Cu; f_0887684C(c);
    call_on(c, o, 0x088F5A24u, f_088F5A34);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): a list-9 helper (0x08822D2C) kept at +228, pointing back at it (+88), with its +11 -
   +14 / +38 / +80 / +81 / +121 / +122 / +194 (as +346), +112 = 2, +255 = 3, +336 - +339 = 1 / 0 / 8 /
   0, +347 = 1, +348 cleared; the effect count (+536) counted, 0x0891CEF0 (fighter, helper), then
   its +86 / +164. */
SFA_FUNC(0891CDFC, fighter_helper_9) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = f;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    ARG(1) = 9; ARG(2) = 0;
    c->r[31] = 0x0891CE1Cu; f_08822D2C(c);
    gaddr n = c->r[2];
    c->r[17] = n;
    if (n != 0) {
        *B(n + 122) = (uint8_t)S8(f + 122);
        *B(n + 121) = (uint8_t)S8(f + 121);
        *B(n + 80) = (uint8_t)S8(f + 80);
        *B(n + 348) = 0;
        *B(n + 346) = (uint8_t)S8(f + 194);
        *B(n + 255) = 3;
        *B(n + 347) = 1;
        *B(n + 81) = (uint8_t)S8(f + 81);
        *B(n + 14) = *B(f + 14);
        *B(n + 11) = *B(f + 11);
        *B(n + 12) = (uint8_t)S8(f + 12);
        *B(n + 13) = *B(f + 13);
        *H(n + 38) = (uint16_t)S16(f + 38);
        *H(n + 112) = 2;
        *B(n + 336) = 1; *B(n + 337) = 0; *B(n + 338) = 8; *B(n + 339) = 0;
        *W(n + 88) = f;
        *W(f + 228) = n;
        *B(f + 536) = (uint8_t)(*B(f + 536) + 1);
        ARG(0) = f; ARG(1) = n;
        c->r[31] = 0x0891CECCu; f_0891CEF0(c);
        *H(n + 86) = *H(f + 86);
        *W(n + 164) = *W(f + 164);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
