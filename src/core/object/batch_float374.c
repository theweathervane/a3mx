/* The library's leaves that work with floating point: three classifications of a float's bits,
   the screen's fade as a float, and drawing list commands that carry a float's top 24 bits.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST 0x08E69348u /* points at the drawing list being written; its +8 is the next place */

/* The float in f12 with its sign cleared, as bits. */
static uint32_t abs_bits(Cpu *c) {
    c->r[5] = 0x7FFFFFFFu;
    c->r[4] = c->f[12] & 0x7FFFFFFFu;
    return c->r[4];
}

/* (f12): 1 when the float is finite. */
SFA_FUNC(089F9F64, float_is_finite) {
    uint32_t bits = abs_bits(c);
    c->r[5] = 0x7F800000u;
    c->r[2] = (bits - 0x7F800000u) >> 31;
}

/* (f12): the float without its sign, in f0. */
SFA_FUNC(089FAC88, float_abs) {
    c->f[0] = abs_bits(c);
}

/* (f12): 1 when the float is not a number. */
SFA_FUNC(089FACA0, float_is_nan) {
    uint32_t bits = abs_bits(c);
    c->r[5] = 0x7F800000u;
    c->r[2] = (0x7F800000u - bits) >> 31;
}

/* (f12): the screen's fade: the float, kept between 0.00001 and 1, becomes the top byte of the
   word at 0x08AD7CE0 as a part of 255. */
SFA_FUNC(089E4978, fade_alpha_set) {
    float v = FS(c, 12);
    if (FCMP(v, 1.0f, 12)) {
        c->f[13] = 0;
        if (FCMP(v, 0.0f, 12)) { v = 1.0e-5f; c->f[12] = 0x3727C5ACu; }
    } else { v = 1.0f; c->f[12] = 0x3F800000u; }
    c->f[13] = 0x437F0000u;
    FSET(c, 12, FS(c, 12) * 255.0f);
    (void)v;
    ARG(0) = 0x08AD0000u;
    ARG(1) = *W(0x08AD7CE0u) & 0x00FFFFFFu;
    c->f[12] = (uint32_t)CVT_TRUNC(c, FS(c, 12));
    ARG(2) = (c->f[12] & 0xFF) << 24;
    ARG(1) |= ARG(2);
    *W(0x08AD7CE0u) = ARG(1);
}

/* The end of a routine: r3 is returned from a 0x30 frame that kept s0..s4 and, at +32, f20. */
SFA_FUNC(089E6E40, tail_e6e40) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 20);
    c->r[20] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->f[20] = *W(sp + 32);
    c->r[2] = c->r[3];
    c->r[29] += 0x30;
}

/* (f12): the drawing list's command 0x5B with the float's top 24 bits. */
SFA_FUNC(089E77C8, list_cmd_5b_float) {
    c->r[9] = 0x08E70000u;
    ARG(2) = *W(LIST);
    c->r[3] = c->f[12];
    c->r[8] = 0x5B000000u;
    ARG(1) = *W(ARG(2) + 8);
    ARG(3) = c->r[3] >> 8;
    ARG(0) = ARG(3) | 0x5B000000u;
    c->r[2] = ARG(1) + 4;
    *W(ARG(1)) = ARG(0);
    *W(ARG(2) + 8) = c->r[2];
}

/* (f12, f13): two commands of the drawing list, a and b, each with a float's top 24 bits. */
static void list_two_floats(Cpu *c, uint32_t a, uint32_t b) {
    c->r[13] = 0x08E70000u;
    c->r[9] = *W(LIST);
    c->r[12] = c->f[12];
    ARG(1) = c->f[13];
    c->r[10] = *W(c->r[9] + 8);
    c->r[11] = a;
    ARG(0) = (c->r[12] >> 8) | a;
    c->r[3] = ARG(1) >> 8;
    ARG(3) = c->r[10] + 4;
    ARG(2) = b;
    c->r[8] = c->r[3] | b;
    c->r[2] = ARG(3) + 4;
    *W(c->r[10]) = ARG(0);
    *W(c->r[9] + 8) = c->r[2];
    *W(ARG(3)) = c->r[8];
}

/* (f12, f13): the drawing list's commands 0x48 and 0x49. */
SFA_FUNC(089E77F4, list_cmd_48_49_floats) {
    list_two_floats(c, 0x48000000u, 0x49000000u);
}

/* (f12, f13): the drawing list's commands 0x4A and 0x4B. */
SFA_FUNC(089E895C, list_cmd_4a_4b_floats) {
    list_two_floats(c, 0x4A000000u, 0x4B000000u);
}

/* The end of a routine that sets the viewport on the list in r4: half the width in r7 and half
   the height in r8 turned over (each halved towards 0), and the centre in r5 and r6, go as
   floats in the commands 0x42, 0x43, 0x45 and 0x46. */
SFA_FUNC(089E97F0, tail_viewport) {
    gaddr list = ARG(0);
    c->r[3] = 0 - c->r[8];
    c->r[9] = ARG(3) >> 31;
    c->r[15] = ARG(3) + c->r[9];
    ARG(3) = c->r[3] >> 31;
    c->r[13] = c->r[3] + ARG(3);
    c->r[14] = (uint32_t)((int32_t)c->r[15] >> 1);
    c->r[2] = (uint32_t)((int32_t)c->r[13] >> 1);
    c->f[0] = c->r[14];
    c->f[3] = ARG(1);
    c->f[8] = ARG(2);
    c->f[2] = c->r[2];
    FSET(c, 7, (float)(int32_t)c->f[0]);
    FSET(c, 5, (float)(int32_t)c->f[3]);
    FSET(c, 6, (float)(int32_t)c->f[2]);
    FSET(c, 4, (float)(int32_t)c->f[8]);
    c->r[15] = *W(list + 8);
    c->r[24] = c->f[6];
    ARG(1) = c->f[4];
    c->r[8] = c->f[5] >> 8;
    c->r[14] = c->r[24] >> 8;
    c->r[9] = c->r[15] + 4;
    c->r[13] = ARG(1) >> 8;
    ARG(2) = 0x43000000u; ARG(3) = 0x45000000u; c->r[3] = 0x46000000u;
    c->r[25] = (c->f[7] >> 8) | 0x42000000u;
    c->r[12] = c->r[14] | 0x43000000u;
    c->r[11] = c->r[8] | 0x45000000u;
    c->r[10] = c->r[13] | 0x46000000u;
    c->r[2] = c->r[9] + 0xC;
    *W(c->r[15]) = c->r[25];
    *W(list + 8) = c->r[2];
    *W(c->r[9]) = c->r[12];
    *W(c->r[9] + 4) = c->r[11];
    *W(c->r[9] + 8) = c->r[10];
}

/* (list, a; f12): the command 0xC8 on the list given, with a and the float times the constant at
   0x08A02DCC, kept between -128 and 127, as a byte above it. */
SFA_FUNC(089E99E0, list_cmd_c8_float) {
    gaddr list = ARG(0);
    c->r[14] = 0x08A00000u;
    c->f[2] = *W(0x08A02DCCu);
    c->r[2] = 0xFFFFFF80u;
    c->r[11] = 0x7F;
    FSET(c, 1, FS(c, 12) * FS(c, 2));
    c->r[9] = *W(list + 8);
    ARG(3) = 0xC8000000u;
    c->f[0] = (uint32_t)CVT_TRUNC(c, FS(c, 1));
    c->r[13] = c->f[0];
    c->r[10] = (uint32_t)MAX((int32_t)c->r[13], -128);
    c->r[12] = (int32_t)c->r[13] < 128;
    if (c->r[12] == 0) c->r[10] = 0x7F;
    c->r[8] = c->r[10] & 0xFF;
    c->r[3] = c->r[8] << 16;
    ARG(2) = c->r[3] | ARG(1);
    ARG(1) = ARG(2) | 0xC8000000u;
    c->r[2] = c->r[9] + 4;
    *W(c->r[9]) = ARG(1);
    *W(list + 8) = c->r[2];
}
