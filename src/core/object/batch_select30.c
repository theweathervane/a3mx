/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 165 - 185
   instructions: the VS screen's fighter name banner, a select-screen fighter picture's set-up by the
   match. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_0882196C(Cpu *c); void f_088969BC(Cpu *c); void f_08898308(Cpu *c); void f_088988FC(Cpu *c);
void f_0889CEE4(Cpu *c); void f_0889CFE8(Cpu *c); void f_088D5E44(Cpu *c); void f_0889697C(Cpu *c);
void f_08898954(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* Whether this match shows the "special" banner rows: a line-up 3 match is handled by the caller; mode
   (GAME +114) 0x13 / 0xD / 0x11, or with BATTLE +79, GAME +137 or BATTLE +159 set the object's +127 = 4. */
static int special_rows(gaddr o) {
    int32_t m = S8(GAME + 114);
    if (m == 0x13 || m == 0xD || m == 0x11) return 1;
    if (S8(BATTLE + 79) != 0 || S8(GAME + 137) != 0 || S8(BATTLE + 159) != 0) return S8(o + 127) == 4;
    return 0;
}

/* (banner): the VS screen's fighter name banner (its fighter at +92). First frame: +5 on, +148 / +86
   = 0 / 8 unless 0x088D5E44 says so without GAME +146, set up (0x0889CFE8), y 0x35 (0x3D for line-up 3
   or the special rows), +12 = 1, +13 = 0xC + side, its picture 0x3D + side, palette 0x145 + colour
   (0x147 for the boss flagged at 0x09358528 + 0x354 x side) into slot +13, +4 = 2 when the fighter
   is out (+431 with the CPU clear, or +371 = 2), and its name animation (0x08A27730's list) unless
   0x088D5E44 says otherwise. Later: at once with BATTLE +81 (0x08898308), or on at GAME +20 = 0x1F. */
SFA_FUNC(08898F5C, vs_name_banner) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = s2; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = ra;
    c->r[16] = o; c->r[18] = BATTLE;
    if (*B(o + 5) != 0) {
        if (S8(BATTLE + 81) != 0) {
            *B(o + 5) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            call_on(c, o, 0x088991A4u, f_08898308);
        } else if (((uint32_t)(int32_t)S16(GAME + 20) & 0xFF) == 0x1F) {
            *B(o + 5) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        }
        goto done;
    }
    gaddr f = *W(o + 92);
    c->r[17] = f;
    *B(o + 5) = 1;
    call_on(c, 1, 0x08898F98u, f_088D5E44);
    if (c->r[2] == 0 || S8(GAME + 146) != 0) { *W(o + 148) = 0; *H(o + 86) = 8; }
    call2(c, o, 0, 0x08898FCCu, f_0889CFE8);
    *H(o + 22) = 0x35;
    if (S16(BATTLE + 56) == 3 || special_rows(o)) *H(o + 22) = 0x3D;
    call_on(c, f, 0x08899050u, f_088988FC);
    int32_t side = (int16_t)(c->r[2] & 0xFF);
    uint32_t slot = (uint32_t)(side + 0xC);
    c->r[18] = slot;
    *B(o + 12) = 1;
    *B(o + 13) = (uint8_t)slot;
    call2(c, o, (uint32_t)(side + 0x3D), 0x08899078u, f_0889CEE4);
    int32_t s = S8(f + 337);
    uint32_t boss = 0;
    if (s < 2 && S8(0x09358410u + (uint32_t)(s * 0x354) + 280) != 0) boss = 1;
    c->r[19] = boss;
    if (boss) call3(c, 0x147, slot, 1, 0x088990C4u, f_088969BC);
    else call3(c, (uint32_t)(int32_t)S8(f + 386) + 0x145, slot, 1, 0x088990E0u, f_088969BC);
    c->r[18] = (uint32_t)(int32_t)S8(f + 431);
    if (c->r[18] != 0 && S8(f + 373) == 0) *B(o + 4) = 2;
    else if (S8(f + 371) == 2) *B(o + 4) = 2;
    c->r[31] = 0x0889911Cu; f_088D5E44(c);
    if (c->r[2] == 0 || S8(GAME + 146) != 0) {
        if (boss) call2(c, o, *W(0x08A27730u + 16), 0x0889913Cu, f_0882196C);
        else call2(c, o, *W(0x08A27730u) + ((uint32_t)(int32_t)S8(f + 386) << 3) + 8, 0x08899160u, f_0882196C);
    }
done:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (picture): a select-screen fighter picture's set-up (its fighter at +92): +4 on, +86 = 0xB, +148
   cleared, +164 = 1, 0x08898954, then by the match: its direction (+11, from the side), slot (+13 = 4 x
   the row) and picture (0x1B + 8 x +10, +6 for kind 4) by the line-up (BATTLE +56), the fighter's side,
   who is human (GAME +57), the mode and the special rows (rows from 0x08A2A126; character 0xA shade 2,
   0x1F the other row); y 0x78 (or for +10 from 2 the step kept and y from +114); then its palette
   (0x0889697C, 0x25 as 0x15) and animation (0x08A27A50's lists by colour). */
SFA_FUNC(08898988, select_fighter_picture) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    gaddr f = *W(o + 92);
    c->r[17] = f;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    uint32_t shade = (uint32_t)(int32_t)S8(f + 376);
    *W(c->r[29] + 24) = s2;
    *H(o + 86) = 0xB;
    *W(o + 148) = 0;
    *W(o + 164) = 1;
    shade &= 0xFFFF;
    c->r[18] = shade;
    *W(c->r[29] + 28) = s3; *W(c->r[29] + 32) = ra;
    call_on(c, o, 0x088989E0u, f_08898954);
    call_on(c, f, 0x088989E8u, f_088988FC);
    uint32_t side = c->r[2];
    uint32_t pic = side, row = side;
    int32_t m = S16(BATTLE + 56);
    c->r[19] = 0x78;
    if (m != 0) {
        int32_t s = S8(f + 337);
        row = (uint32_t)s;
        int32_t g57 = S8(GAME + 57);
        int to_rows;
        if (m == 3) to_rows = 1;
        else if ((g57 & (1 << (s & 31))) || S8(BATTLE + 79) != 0 || S8(o + 127) == 4) to_rows = 1;
        else to_rows = 0;
        if (!to_rows) {
            int32_t ch = S8(f + 338);
            if (ch == 0xA) { c->r[18] = shade = 2; row = side; }
            else {
                uint32_t r9 = (g57 & 2) == 0;
                if (ch == 0x1F) r9 ^= 1;
                pic = (g57 & 1) == 0 ? r9 + 2 : 5 - r9;
                row = ch == 0x1F ? 2 : side;
            }
        } else if (special_rows(o)) {
            int32_t mm = m;
            if (m == 1) {
                if ((s & 2) == 0) pic = (uint32_t)(s + 2);
                else {
                    pic = (uint32_t)(7 - s);
                    if (s == 2) {
                        *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
                        mm = S16(BATTLE + 56);
                        s = S8(f + 337);
                    }
                }
            } else pic = (s & 2) == 0 ? (uint32_t)(5 - s) : (uint32_t)s;
            row = *B(0x08A2A126u + ((uint32_t)mm << 2) + (uint32_t)s);
        } else if ((s & 2) == 0) {
            pic = (S8(GAME + 57) & 1) == 0 ? (uint32_t)(5 - s) : (uint32_t)(s + 2);
        }
    }
    *B(o + 12) = 0xFF;
    *B(o + 11) = (uint8_t)((side ^ 1) & 1);
    *B(o + 13) = (uint8_t)(row << 2);
    if (*B(o + 3) == 4) pic += 6;
    call2(c, o, pic + ((uint32_t)*B(o + 10) << 3) + 0x1B, 0x08898BC8u, f_0889CEE4);
    if ((int32_t)*B(o + 10) < 2) *H(o + 22) = 0x78;
    else {
        *W(o + 100) = *W(o + 96);
        *W(o + 108) = *W(o + 104);
        *H(o + 22) = (uint16_t)S16(o + 114);
        *H(o + 114) = 0x78;
    }
    uint32_t ch = (uint32_t)(int32_t)S8(f + 338);
    if (ch == 0x25) ch = 0x15;
    c->r[19] = ch;
    ARG(0) = ch; ARG(1) = (uint32_t)(int32_t)S8(f + 386); ARG(2) = c->r[18]; ARG(3) = *B(o + 13);
    c->r[31] = 0x08898C28u; f_0889697C(c);
    call2(c, o, *W(0x08A27A4Cu + ((uint32_t)(int32_t)S8(f + 386) << 2) + 4) + (ch << 3), 0x08898C50u, f_0882196C);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}
