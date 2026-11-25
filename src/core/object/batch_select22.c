/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 25 - 120
   instructions: the select screen's portraits freed, the select screen's big portrait kept to its
   fighter, the VS screen's fighter picture set-up. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_08812DDC(Cpu *c); void f_089E0C24(Cpu *c); void f_088215C4(Cpu *c); void f_0882196C(Cpu *c);
void f_088219B0(Cpu *c); void f_088969BC(Cpu *c); void f_0889693C(Cpu *c); void f_088988FC(Cpu *c);
void f_0889CFE8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }

/* List 13's buffers freed (0x08812DDC) and the portraits' textures 0x70 + n (n under 0x08A05074). */
SFA_FUNC(08814320, select_portraits_free) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    call_on(c, 0xD, 0x08814338u, f_08812DDC);
    c->r[16] = 0x08A05070u;
    for (c->r[17] = 0; c->r[17] < *W(0x08A05070u + 4); c->r[17]++) call_on(c, c->r[17] + 0x70, 0x0881435Cu, f_089E0C24);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (portrait, fighter): the select screen's big portrait kept to its fighter: +73 / +81 / +117 follow
   +78 / +80 / +116, +171 set for characters 0x26 - 0x29; when the character or colour (+376)
   changed: +78 / +80 (+386) / +116 set, its animation (0x08A296C0, 0x25 as 0x15) and palettes
   (0x088969BC: 0x538 + 6 x character + colour into slot 0x20 + 2 x side; two of them at 2 x colour for
   character 0x29; characters 20 and 0x1C also 0x63A / 0x63B into the next slot); else 0x088219B0.
   Then drawn (0x088215C4). */
SFA_FUNC(0889D074, select_portrait_follow) {
    gaddr o = ARG(0), f = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = s3;
    *W(c->r[29] + 32) = ra;
    c->r[16] = o; c->r[17] = f;
    uint32_t was = (uint32_t)(int32_t)S8(o + 78), col = (uint32_t)(int32_t)S8(o + 80), v116 = *B(o + 116);
    if (was != (uint32_t)(int32_t)S8(o + 73) || col != (uint32_t)(int32_t)S8(o + 81) || v116 != *B(o + 117)) {
        *B(o + 73) = (uint8_t)was; *B(o + 81) = (uint8_t)col; *B(o + 117) = (uint8_t)v116;
    }
    uint32_t ch = (uint32_t)(int32_t)S8(f + 338), pal = (uint32_t)(int32_t)S8(f + 386);
    uint32_t shade = (uint32_t)(int32_t)S8(f + 376);
    c->r[18] = ch; c->r[19] = shade;
    *B(o + 171) = (ch >= 0x26 && ch <= 0x29) ? 1 : 0;
    if (was == ch && v116 == shade) {
        call_on(c, o, 0x0889D220u, f_088219B0);
    } else {
        *B(o + 78) = (uint8_t)ch;
        *B(o + 80) = (uint8_t)pal;
        *B(o + 116) = (uint8_t)shade;
        if (ch == 0x25) ch = 0x15;
        call2(c, o, *W(0x08A296C0u + (ch << 2)), 0x0889D15Cu, f_0882196C);
        uint32_t base = 6u * ch + 0x538;
        uint32_t slot = ((uint32_t)(int32_t)S8(f + 337) << 1) + 0x20;
        c->r[18] = 6u * ch;
        if (S8(f + 338) == 0x29) call3(c, base + 2u * shade, slot, 2, 0x0889D198u, f_088969BC);
        else call3(c, base + shade, slot, 1, 0x0889D1B4u, f_088969BC);
        int32_t now = S8(f + 338);
        c->r[19] = (uint32_t)now;
        uint32_t next = ((uint32_t)(int32_t)S8(f + 337) << 1) + 0x21;
        if (now == 20) call3(c, 0x63A, next, 1, 0x0889D1E8u, f_088969BC);
        else if (now == 0x1C) call3(c, 0x63B, next, 1, 0x0889D210u, f_088969BC);
    }
    call_on(c, o, 0x0889D228u, f_088215C4);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (picture): the VS screen's fighter picture (its fighter at +92). First frame: set up (0x0889CFE8, +5
   on, +14 = 1, y 0xD0); with +10 set a sliding banner (+148 = 1, +86 = 8, step x -1, +76 / +77 = 0 /
   0x10, the animation 0x08A283EC by GAME +32 bit 0); else by the fighter's side (0x088988FC; in a
   line-up 3 match the partner side as side + 2 with +12 = -1, +13 = 4) its direction and x from the
   4-byte row of 0x08A2B2C8, its animation (0x08A27A6C's list, 8 bytes by character, 0x25 as 0x15) and
   palette (0x0889693C). Later frames: at once with BATTLE +81, or on at GAME +20 high byte 5 (+5
   cleared, +4 on). */
SFA_FUNC(0889F924, vs_fighter_picture) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19];
    c->r[29] -= 0x30;
    gaddr f = *W(o + 92);
    *W(c->r[29] + 28) = s3; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2;
    *W(c->r[29] + 32) = ra;
    c->r[19] = f; c->r[16] = o;
    if (*B(o + 5) != 0) {
        if (S8(BATTLE + 81) != 0 || ((uint32_t)(int32_t)S16(GAME + 20) & 0xFF00) == 0x500) {
            *B(o + 5) = 0;
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        }
        goto done;
    }
    call2(c, o, 0, 0x0889F958u, f_0889CFE8);
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 14) = 1;
    c->r[18] = 8; c->r[17] = 0xD0;
    if (*B(o + 10) != 0) {
        *W(o + 148) = 1;
        *H(o + 86) = 8;
        *W(o + 16) = 0;
        *B(o + 11) = 0;
        *H(o + 22) = 0xD0;
        *W(o + 96) = 0xFFFF0000u;
        *B(o + 76) = 0; *B(o + 77) = 0x10;
        call2(c, o, *W(0x08A283ECu + ((uint32_t)(*B(GAME + 32) & 1) << 2)), 0x0889FAA4u, f_0882196C);
        goto done;
    }
    *H(o + 86) = 8;
    *W(o + 148) = 0;
    call_on(c, f, 0x0889F98Cu, f_088988FC);
    uint32_t side = c->r[2];
    if (S16(BATTLE + 56) == 3 && ((S8(GAME + 57) + S8(f + 337)) & 1) == 0) {
        *B(o + 12) = 0xFF; *B(o + 13) = 4;
        side += 2;
    }
    gaddr row = 0x08A2B2C8u + (side << 2);
    *B(o + 11) = (uint8_t)S16(row);
    *H(o + 22) = 0xD0;
    *H(o + 18) = (uint16_t)S16(row + 2);
    uint32_t ch = (uint32_t)(int32_t)S8(f + 338);
    if (ch == 0x25) ch = 0x15;
    c->r[18] = ch;
    call2(c, o, *W(0x08A27A4Cu + 32) + (ch << 3), 0x0889FA30u, f_0882196C);
    ARG(0) = ch; ARG(1) = 0; ARG(2) = (uint32_t)(int32_t)S8(f + 376); ARG(3) = *B(o + 13);
    c->r[31] = 0x0889FA44u; f_0889693C(c);
done:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra;
    c->r[29] += 0x30;
}
