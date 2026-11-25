/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 91 - 101
   instructions: a menu cursor's pulsing shade, whether a character may be picked (not one already
   taken), the VS screen's fighter banner set-up, the fighters' palettes for the VS screen. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08888EE4(Cpu *c); void f_08831798(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c);
void f_088FBFD4(Cpu *c); void f_088FC444(Cpu *c); void f_0880AEA8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (cursor): with the menu state (0x0950FF4A) 0 just 0x08888EE4. Until it starts (+129) it waits for
   state 2. Then its shade (+42, three copies at +43 - +45; +48 with copies at +49 - +51) pulses by 2 a
   frame between 0x40 and 0xA0 (+131 the direction); +93 is cleared in state 3, else set with +10
   from the 4-byte row of 0x08A261BC by 0x0950FF48 (its first half while 0x0950FF40 is 1). */
SFA_FUNC(088889A0, cursor_pulse) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    gaddr menu = 0x0950FF40u;
    if (*B(menu + 10) == 0) { c->r[31] = 0x088889C4u; f_08888EE4(c); goto done; }
    if (*B(o + 129) == 0) {
        if (*B(menu + 10) == 2) *B(o + 129) = 1;
        goto done;
    }
    {
        uint8_t dir = *B(o + 131);
        int32_t v;
        if (dir == 0) {
            v = (int32_t)*B(o + 48) - 2;
            if (v < 64) { *B(o + 131) = (uint8_t)(dir ^ 1); *B(o + 42) = 0x40; *B(o + 48) = 0x40; }
            else { *B(o + 48) = (uint8_t)v; *B(o + 42) = (uint8_t)(*B(o + 42) - 2); }
        } else {
            v = (int32_t)*B(o + 48) + 2;
            if (v < 161) { *B(o + 48) = (uint8_t)v; *B(o + 42) = (uint8_t)(*B(o + 42) + 2); }
            else { *B(o + 131) = (uint8_t)(dir ^ 1); *B(o + 42) = 0xA0; *B(o + 48) = 0xA0; }
        }
        uint8_t a = *B(o + 42), b = *B(o + 48);
        *B(o + 45) = a; *B(o + 44) = a; *B(o + 43) = a;
        *B(o + 51) = b; *B(o + 50) = b; *B(o + 49) = b;
        if (*B(menu + 10) == 3) { *B(o + 93) = 0; goto done; }
        *B(o + 93) = 1;
        gaddr row = 0x08A261BCu + 2u * *B(menu + 8);
        *H(o + 10) = (uint16_t)(*B(menu) == 1 ? S16(row) : S16(row + 2));
    }
done:
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (character, n, list): 1 when the character may be picked: not among the n already taken at
   0x09517988 (4 also not with 0x12 there, 6 not with 0x14), not the same as the last one taken by
   0x08831798, and not in the 0xFF-ended list before its end. */
SFA_FUNC(0882F024, character_free) {
    uint32_t ch = ARG(0) & 0xFF, n = ARG(1) & 0xFF;
    gaddr list = ARG(2);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    for (int k = 0; k < 9; k++) *W(sp + 32 + 4u * (uint32_t)k) = sv[k];
    *W(sp + 68) = ra;
    const gaddr taken = 0x09517988u;
    c->r[16] = ch; c->r[17] = n; c->r[18] = taken;
    *W(sp + 16) = n + taken;
    *W(sp + 20) = list;
    c->r[22] = 4; c->r[21] = 0x12; c->r[20] = 6; c->r[19] = 0x14; c->r[30] = 0xFF;
    uint32_t free = 0;
    for (uint32_t j = 0;; j = (j + 1) & 0xFF) {
        c->r[23] = j;
        for (uint32_t i = 0; (int32_t)i < (int32_t)n; i = (i + 1) & 0xFF) {
            uint32_t x = *B(taken + i);
            if (x == ch || (ch == 4 && x == 0x12) || (ch == 6 && x == 0x14)) goto out;
        }
        if ((int32_t)n > 0) {
            *W(sp + 24) = 0xFF;
            call_on(c, *B(*W(sp + 16) - 1), 0x0882F0F0u, f_08831798);
            *W(sp + 28) = c->r[2];
            call_on(c, ch, 0x0882F0FCu, f_08831798);
            if (*W(sp + 28) == c->r[2]) goto out;
        }
        uint32_t y = *B(*W(sp + 20) + j);
        if (y == ch) goto out;
        if (y == 0xFF) { free = 1; goto out; }
    }
out:
    c->r[2] = free;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[21] = sv[5]; c->r[22] = sv[6]; c->r[23] = sv[7]; c->r[30] = sv[8]; c->r[31] = ra;
    c->r[29] += 0x50;
}

/* (banner): the VS screen's fighter banner, from its parent (+88): on (+4 = 1, +9 cleared), +13 / +350
   the parent's +13, its tables (+128 = 0x08A46154, +200 = 0x08A4604C, +352 = 0x08A45F50, +356 =
   0x08A45FD0), 0x08876998, +24 / +86 / +164 the parent's, 0x088FC444 for kinds after 0; y by the kind
   (0x08A4941A), x and step x by the kind / +346 (0x08A49410 / 0x08A49424, turned for +11 clear), +100
   from 0x08A49460, 0x0887684C (+346); kind 1 under a parent with +78 = -1 a step faster; then
   0x088FBFD4. */
SFA_FUNC(088FBE4C, vs_banner_setup) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    gaddr p = *W(o + 88);
    c->r[17] = p;
    *B(o + 4) = 1;
    *B(o + 9) = 0;
    *B(o + 13) = *B(p + 13);
    *B(o + 350) = *B(o + 13);
    *W(o + 128) = 0x08A46154u;
    *W(o + 200) = 0x08A4604Cu;
    call_on(c, o, 0x088FBEA0u, f_08876998);
    *H(o + 24) = *H(p + 24);
    *H(o + 86) = *H(p + 86);
    *W(o + 164) = *W(p + 164);
    if (*B(o + 3) != 0) call_on(c, o, 0x088FBEC8u, f_088FC444);
    uint32_t k2 = 2u * *B(o + 3);
    *H(o + 22) = (uint16_t)(S16(o + 22) + S16(0x08A4941Au + k2));
    int32_t side = S8(o + 346);
    int32_t dx = S16(0x08A49410u + k2);
    uint32_t k4 = (uint32_t)side << 2;
    *W(o + 100) = *W(0x08A49460u + k4);
    uint32_t sx = *W(0x08A49424u + k4);
    if (*B(o + 11) == 0) { dx = (int16_t)-dx; sx = 0u - sx; }
    *W(o + 96) = sx;
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *W(o + 352) = 0x08A45F50u;
    *W(o + 356) = 0x08A45FD0u;
    call2(c, o, (uint32_t)side & 0xFFFF, 0x088FBF7Cu, f_0887684C);
    if (*B(o + 3) == 1 && S8(p + 78) == -1)
        *W(o + 96) = *W(o + 96) + (*B(o + 11) != 0 ? 0xFFFF0000u : 0x10000u);
    call_on(c, o, 0x088FBFC0u, f_088FBFD4);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* One fighter's colours into palette slot `slot` of set 0: its side's palette list (0x092D9E00 + 4 x
   +337) at its colour (+376): 8 palettes of 256 bytes for character 0x29, else 5 of 160. */
static void fighter_colours(Cpu *c, gaddr f, uint32_t slot, uint32_t back8, uint32_t back5) {
    gaddr base = *W(0x092D9E00u + ((uint32_t)(int32_t)S8(f + 337) << 2));
    uint32_t col = (uint32_t)(int32_t)S8(f + 376);
    int big = S8(f + 338) == 0x29;
    ARG(0) = 0; ARG(1) = base + (big ? col << 8 : col * 160u); ARG(2) = 0; ARG(3) = slot; c->r[8] = big ? 8 : 5;
    c->r[31] = big ? back8 : back5; f_0880AEA8(c);
}

/* (fighter, fighter, partner or 0): the VS screen's fighters' colours into palette slots 0, 0x40 and
   (with a partner) 0x48. */
SFA_FUNC(0881304C, vs_fighter_colours) {
    gaddr f1 = ARG(0), f2 = ARG(1), f3 = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x40;
    *W(c->r[29] + 44) = s3; *W(c->r[29] + 32) = s0; *W(c->r[29] + 40) = s2; *W(c->r[29] + 36) = s1;
    *W(c->r[29] + 48) = ra;
    c->r[19] = 0x092D9E00u; c->r[16] = f2; c->r[18] = 0x29; c->r[17] = f3;
    fighter_colours(c, f1, 0, 0x088130B4u, 0x088130F4u);
    fighter_colours(c, f2, 0x40, 0x08813130u, 0x08813158u);
    if (f3 != 0) fighter_colours(c, f3, 0x48, 0x08813198u, 0x088131C4u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x40;
}
