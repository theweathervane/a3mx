/* A third helping of the library's small pieces: the frame's two pools (bytes and draw records), a
   wide strip drawn from them, the save dialog's small ends, whether a memory stick is there, and
   two bytes of the sound driver. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE 0x08D5C7C0u /* the save dialog's parameters */

void f_089E40C0(Cpu *c); void f_089E44EC(Cpu *c); void f_089E5990(Cpu *c); void f_089E3400(Cpu *c);
void f_089E3D08(Cpu *c); void f_089E3248(Cpu *c); void f_089F3430(Cpu *c); void f_089E3158(Cpu *c);
void f_089E36D0(Cpu *c); void f_089E37EC(Cpu *c); void f_089F3470(Cpu *c); void f_089E31E0(Cpu *c);
void hle_sceIoDevctl(Cpu *c); void hle_sceUtilitySavedataInitStart(Cpu *c);

/* (n): n bytes, rounded up to 16, from the frame's pool: 0x60000 bytes for each of the two frames
   (the byte at 0x08E65C64) from 0x08D95C40, with the used count at 0x08E65C44. Returns the
   place. */
SFA_FUNC(089E40C0, frame_pool_take) {
    uint32_t n = ARG(0);
    uint32_t blocks = n >> 4;
    if (n & 0xF) blocks++;
    uint32_t frame = *B(0x08E65C64u);
    int64_t m = (int64_t)(int32_t)frame * 0x60000;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    uint32_t used = *W(0x08E65C44u);
    c->r[5] = 0x08E60000u; c->r[6] = used; c->r[7] = 0x08D95C40u; c->r[8] = c->lo;
    c->r[4] = used + (blocks << 4);
    c->r[2] = c->lo + used + 0x08D95C40u;
    *W(0x08E65C44u) = c->r[4];
}

/* (a, record, b): a draw record entered in the frame's list: 8 bytes each from 0x08E55C40 (0x8000
   for each frame), the record, b and a as two words; the count at 0x08E65C40 moves on. */
SFA_FUNC(089E44EC, frame_list_enter) {
    uint32_t a = ARG(0) & 0xFF, b = ARG(2) & 0xFF;
    uint32_t frame = *B(0x08E65C64u);
    uint32_t n = *H(0x08E65C40u);
    gaddr at = (frame << 15) + (n << 3) + 0x08E55C40u;
    c->r[7] = at; c->r[8] = 0x08E60000u; c->r[9] = 0x08E55C40u; c->r[10] = 0x08E50000u;
    c->r[6] = b;
    *W(at) = ARG(1);
    *H(at + 4) = (uint16_t)b;
    *H(at + 6) = (uint16_t)a;
    c->r[4] = (uint32_t)*H(0x08E65C40u) + 1;
    *H(0x08E65C40u) = (uint16_t)c->r[4];
}

/* (a, x, y, texture, kind): a strip of 256 by 32 drawn at (x + 0x30, y - 8): a record of 0x1C
   bytes and four corners of 16 from the frame's pool, the texture, the word the kind names in
   0x08AD7E70, and the record entered in the frame's list under a. Nothing without a texture. A
   0x30 frame keeping s0..s5. */
SFA_FUNC(089E4F94, strip_256x32_draw) {
    uint32_t a = ARG(0), x = ARG(1), y = ARG(2), tex = ARG(3), kind = c->r[8];
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 6; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 40) = sv[6];
    c->r[20] = a; c->r[19] = x; c->r[18] = y; c->r[17] = tex; c->r[16] = kind;
    if (tex != 0) {
        ARG(0) = 0x1C; c->r[31] = 0x089E4FD4u; f_089E40C0(c);
        gaddr rec = c->r[2];
        c->r[21] = rec;
        ARG(0) = 0x40; c->r[31] = 0x089E4FE0u; f_089E40C0(c);
        gaddr v = c->r[2];
        *W(rec) = v;
        for (uint32_t i = 0; i < 4; i++, v += 0x10) {
            *H(v + 8) = (uint16_t)(x + ((i & 1) ? 0x130 : 0x30));
            *H(v + 10) = (uint16_t)(y + ((i & 2) ? 0x18 : 0xFFFFFFF8u));
            *W(v + 4) = 0xFFFFFFFFu;
            *H(v) = (i & 1) ? 0x100 : 0;
            *H(v + 2) = (i & 2) ? 0x20 : 0;
        }
        *H(rec + 20) = 0x100;
        *H(rec + 22) = 0x20;
        *W(rec + 8) = tex;
        *W(rec + 4) = *W(0x08AD7E70u + (kind << 2));
        *W(rec + 12) = 0x80000000u;
        *W(rec + 16) = 4;
        *B(rec + 24) = 0;
        *B(rec + 25) = 2;
        *B(rec + 26) = 0;
        ARG(0) = a & 0xFF; ARG(1) = rec; ARG(2) = 1;
        c->r[31] = 0x089E50B0u; f_089E44EC(c);
    }
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}

/* (a, b, d, e): an entry of kind 1 in the second callback table (1, a, b, d, e). */
SFA_FUNC(089E59D4, table_enter_5_kind1) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    c->r[8] = ARG(3); ARG(3) = ARG(2); ARG(2) = ARG(1); ARG(1) = ARG(0);
    *W(c->r[29] + 32) = ra;
    ARG(0) = 1; c->r[31] = 0x089E59F4u; f_089E5990(c);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (v): the byte at 0x08AD80C0. */
SFA_FUNC(089E688C, sound_byte_80c0) {
    c->r[5] = 0x08AE0000u;
    *B(0x08AD80C0u) = (uint8_t)ARG(0);
}

/* (v): the byte at 0x08AD80C1. */
SFA_FUNC(089E6898, sound_byte_80c1) {
    c->r[5] = 0x08AE0000u;
    *B(0x08AD80C1u) = (uint8_t)ARG(0);
}

/* 1 when the save dialog's +28 is 1. */
SFA_FUNC(089E3CE0, save_word_28_is_1) {
    ARG(0) = *W(SAVE + 28);
    c->r[2] = ARG(0) == 1 ? 1 : 0;
}

/* The word at 0x08AD7CD4. */
SFA_FUNC(089E3EA8, word_at_8ad7cd4) {
    ARG(0) = 0x08AD0000u;
    c->r[2] = *W(0x08AD7CD4u);
}

/* The save's outcome by the word at 0x08AD7CD0: 0x089E3400's answer for 0 and for 0x80110005,
   -0x100 for the other errors, and -1 for the rest. */
SFA_FUNC(089E3C74, save_outcome) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t state = *W(0x08AD7CD0u);
    *W(c->r[29] + 16) = ra;
    if (state == 0x80110005u || state == 0) { ARG(0) = SAVE; c->r[31] = 0x089E3CA0u; f_089E3400(c); }
    else if (state & 0x80000000u) c->r[2] = 0xFFFFFF00u;
    else c->r[2] = 0xFFFFFFFFu;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* One question to the device named at 0x08A01DC0 (the command 0x02425823, four bytes back at
   `out`): -1 or less when it fails, else whether the answer is 1. */
static int32_t stick_ask(Cpu *c, gaddr out, uint32_t back) {
    ARG(0) = 0x08A01DC0u; ARG(1) = 0x02425823u; ARG(2) = 0; ARG(3) = 0;
    c->r[8] = out; c->r[9] = 4;
    c->r[31] = back; hle_sceIoDevctl(c);
    if ((int32_t)c->r[2] < 0) return (int32_t)c->r[2];
    return *W(out) == 1;
}

/* (n): 1 when the memory stick is there, asked twice; 0 for any n but 0. A 0x40 frame keeping
   s0, s1. */
SFA_FUNC(089E3D08, stick_present) {
    uint32_t n = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 40) = s0; *W(sp + 44) = s1; *W(sp + 48) = ra;
    uint32_t found = 0;
    if (n == 0) {
        c->r[17] = 0x08A01DC0u; c->r[16] = 0x02425823u;
        if (stick_ask(c, sp + 0x20, 0x089E3D48u) > 0 && stick_ask(c, sp + 0x24, 0x089E3D8Cu) > 0) found = 1;
    }
    c->r[2] = found;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x40;
}

/* The eight slots' answers go to the bytes from 0x09523C70 and how many are there to 0x09523C67.
   A 0x30 frame keeping s0..s3. */
SFA_FUNC(08816AE4, sticks_count) {
    const gaddr at = 0x09523C60u;
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3]; *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    *W(sp + 32) = sv[4];
    c->r[19] = at; c->r[18] = at + 0x10;
    int32_t count = 0;
    for (int32_t i = 0; i < 8; i = (int8_t)(i + 1)) {
        c->r[16] = (uint32_t)i; c->r[17] = (uint32_t)count;
        ARG(0) = (uint32_t)i; c->r[31] = 0x08816B18u; f_089E3D08(c);
        *B(at + 0x10 + (uint32_t)i) = (uint8_t)c->r[2];
        if ((int8_t)c->r[2] != 0) count = (int8_t)(count + 1);
    }
    *B(at + 7) = (uint8_t)count;
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (params, b, d, e): the dialog's parameters filled (0x089E3248), the mode 0 at +48, d and e at
   +116 and +120, and sixteen bytes from 0x08AD3D28 copied to +0x5DC. A frame keeping s0..s2. */
SFA_FUNC(089E36D0, save_params_load) {
    gaddr p = ARG(0);
    uint32_t d = ARG(2), e = ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[16] = e; c->r[17] = p;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = d;
    c->r[31] = 0x089E36F4u; f_089E3248(c);
    *W(p + 48) = 0;
    *W(p + 116) = d;
    *W(p + 120) = e;
    ARG(0) = p + 0x5DC; ARG(1) = 0x08AD3D28u; ARG(2) = 0x10;
    c->r[31] = 0x089E3714u; f_089F3430(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a): a load starts: 0x089E3158's answer for a, the parameters with the 0x8000 bytes at
   0x08D5CDD4, and the dialog. Returns 0. */
SFA_FUNC(089E37EC, save_load_start) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = 0x08D60000u;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = SAVE;
    c->r[31] = 0x089E3804u; f_089E3158(c);
    ARG(0) = SAVE; ARG(1) = c->r[2]; ARG(2) = 0x08D5CDD4u; ARG(3) = 0x8000;
    c->r[31] = 0x089E381Cu; f_089E36D0(c);
    ARG(0) = SAVE; c->r[31] = 0x089E3824u; hle_sceUtilitySavedataInitStart(c);
    c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): a load of the n-th of the 16-byte entries at 0x08A07768. */
SFA_FUNC(08816EDC, save_load_nth) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = 0x08A07768u;
    *W(c->r[29] + 16) = ra;
    ARG(0) = (ARG(0) << 4) + 0x08A07768u;
    c->r[31] = 0x08816EF8u; f_089E37EC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The dialog's list mode: the 0x14 bytes at 0x08D5CDC0 cleared, the parameters reset
   (0x089E31E0), the mode 8 with that block at +1488, and the dialog. A frame keeping s0, s1. */
SFA_FUNC(089E352C, save_list_start) {
    const gaddr block = 0x08D5CDC0u;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = block;
    *W(sp + 20) = s1; *W(sp + 24) = ra;
    ARG(0) = block; ARG(1) = 0; ARG(2) = 0x14;
    c->r[31] = 0x089E3554u; f_089F3470(c);
    c->r[17] = SAVE;
    ARG(0) = SAVE; c->r[31] = 0x089E3564u; f_089E31E0(c);
    *W(SAVE + 48) = 8;
    *W(SAVE + 1488) = block;
    ARG(0) = SAVE; c->r[31] = 0x089E3578u; hle_sceUtilitySavedataInitStart(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
