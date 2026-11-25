/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in), 14 - 84
   instructions: a pad's buttons read each frame (held, pressed, released and auto-repeat), the select
   portrait's timer, two mode-screen set-ups. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08895B6C(Cpu *c); void f_08804EB0(Cpu *c); void f_0880AEA8(Cpu *c); void f_088AD2D8(Cpu *c);
void f_0882196C(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void palette(Cpu *c, uint32_t from, uint32_t back) {
    ARG(0) = 1; ARG(1) = from; ARG(2) = 0; ARG(3) = 1; c->r[8] = 2;
    c->r[31] = back; f_0880AEA8(c);
}

/* (screen): +20 counted down; at 0 the portrait is cleared (0x08895B6C). */
SFA_FUNC(08895B34, select_portrait_timer) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int16_t left = (int16_t)(S16(o + 20) - 1);
    *H(o + 20) = (uint16_t)left;
    ARG(1) = (uint32_t)(int32_t)left; ARG(2) = (uint32_t)left << 16;
    if (left == 0) { c->r[31] = 0x08895B60u; f_08895B6C(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (pad, input): a pad's buttons for this frame. With no input (input +2 = 0) only 0x08804EB0. Else the
   last frame's held buttons move to +8; with input +2 = 1 the 16 buttons are read from input +4 through
   the masks at 0x08A02DF4 (button n held when input +4 has any of mask n's bits) into +4, +0 = 1, and
   pressed (+12) / released (+16) found; otherwise 0x08804EB0 does that. Then the repeat (+20): a
   pressed button fires and waits +40 frames; a held one counts down its byte at +24 + n and fires again
   every +41 frames. */
SFA_FUNC(08804F50, pad_read) {
    gaddr pad = ARG(0), in = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = pad;
    if (*H(in + 2) == 0) {
        call_on(c, pad, 0x08805080u, f_08804EB0);
    } else {
        uint32_t prev = *W(pad + 4);
        *W(pad + 4) = 0;
        *W(pad + 8) = prev;
        c->r[17] = 1;
        uint32_t held, pressed;
        if (*H(in + 2) == 1) {
            *H(pad) = 1;
            held = *W(pad + 4);
            uint32_t mask = 0;
            for (int n = 15; n >= 0; n--) {
                mask = *W(0x08A02DF4u + 4u * (uint32_t)n);
                if (*W(in + 4) & mask) { held |= 1u << n; *W(pad + 4) = held; }
            }
            c->r[2] = mask;
            pressed = held & ~prev;
            *W(pad + 12) = pressed;
            *W(pad + 16) = prev & ~held;
        } else {
            call_on(c, pad, 0x08804FFCu, f_08804EB0);
            held = *W(pad + 4);
            pressed = *W(pad + 12);
        }
        uint8_t wait = *B(pad + 40), every = *B(pad + 41);
        *W(pad + 20) = 0;
        uint32_t fire = 0;
        for (int n = 0; n < 16; n++) {
            uint32_t bit = 1u << n;
            if (pressed & bit) { *B(pad + 24 + n) = wait; fire |= bit; }
            else if (held & bit) {
                uint32_t v = (uint32_t)*B(pad + 24 + n) - 1u;
                *B(pad + 24 + n) = (uint8_t)v;
                c->r[2] = v & 0xFF;
                if (c->r[2] == 0) { *B(pad + 24 + n) = every; fire |= bit; }
            }
        }
        *W(pad + 20) = fire;
        ARG(0) = held; ARG(1) = 16; ARG(2) = wait; ARG(3) = every;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (screen): a mode screen's set-up by its kind (+3, from 3) and +78: +10 cleared, its 8-byte row of
   0x08A2CBE4 into +100 / +108 (low and high halves) and +22, the high halves of +96 / +104 from the
   4-byte row of 0x08A2CC14 (+16 = +96), the palette 0x08A31718 into slot 1, then 0x088AD2D8. */
SFA_FUNC(088AD00C, mode_screen_setup) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    int16_t k2 = (int16_t)((int16_t)(*B(o + 3) - 3) * 2);
    int16_t row = (int16_t)(k2 + S8(o + 78));
    *B(o + 10) = 0;
    gaddr e = 0x08A2CBE4u + 2u * (uint32_t)(int32_t)(int16_t)(row << 2);
    *W(o + 100) = (uint32_t)*H(e) | ((uint32_t)*H(e + 4) << 16);
    *W(o + 108) = (uint32_t)*H(e + 2) | ((uint32_t)*H(e + 6) << 16);
    *H(o + 22) = *H(e);
    gaddr g = 0x08A2CC14u + 2u * (uint32_t)(int32_t)k2;
    uint32_t w96 = (*W(o + 96) | ((uint32_t)*H(g) << 16)) & 0xFFFF0000u;
    *W(o + 104) = *W(o + 104) | ((uint32_t)*H(g + 2) << 16);
    *W(o + 96) = w96;
    *W(o + 16) = w96;
    palette(c, 0x08A31718u, 0x088AD138u);
    call_on(c, o, 0x088AD140u, f_088AD2D8);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (screen): a mode screen's other set-up: +4 on, +32 = GAME +16's low byte, +148 = 1, +86 = 0xC, +164
   cleared, 0x092E3E98 = 0x08A2CCA0 or (with 0x09525DE3 clear) 0x08A2CE88, +12 = +13 = 1, +26 = 1, +11 /
   +14 / +15 / +38 / +20 cleared, x 0x198, step 0x10, +104 = +108 = 0x18000, the palette 0x08A31758
   into slot 1 and its animation by +78 (through 0x08A2D08C) from 0x08A2CE68 or 0x08A2D044. */
SFA_FUNC(088AD89C, mode_screen_setup_2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = s1; *W(c->r[29] + 40) = s2; *W(c->r[29] + 44) = ra;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 32) = (uint16_t)((uint32_t)(int32_t)S16(GAME + 16) & 0xFF);
    *W(o + 148) = 1;
    *H(o + 86) = 0xC;
    *W(o + 164) = 0;
    c->r[18] = 0x09520000u; c->r[17] = 0x08A2D08Cu;
    int alt = S8(0x09525DE3u) != 0;
    *W(0x092E3E98u) = alt ? 0x08A2CCA0u : 0x08A2CE88u;
    *B(o + 12) = 1; *B(o + 13) = 1;
    *H(o + 38) = 0; *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 26) = 1;
    *W(o + 16) = 0x01980000u;
    *W(o + 96) = 0x00100000u;
    *W(o + 104) = 0x18000u;
    *W(o + 20) = 0;
    *W(o + 108) = 0x18000u;
    palette(c, 0x08A31758u, 0x088AD984u);
    uint32_t k = *B(0x08A2D08Cu + (uint32_t)(int32_t)S8(o + 78));
    alt = S8(0x09525DE3u) != 0;
    if (alt) call2(c, o, *W(0x08A2CE68u + k * 4), 0x088AD9B4u, f_0882196C);
    else call2(c, o, *W(0x08A2D044u + k * 4), 0x088AD9D4u, f_0882196C);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}
