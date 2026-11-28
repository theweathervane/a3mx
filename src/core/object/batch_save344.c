/* The save screen's loop and a save's start, with the link's step back into a fight. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE 0x08D5C7C0u /* the save dialog's parameters */

void f_089E3B6C(Cpu *c); void f_088E973C(Cpu *c); void f_0880ABD4(Cpu *c); void f_088E967C(Cpu *c);
void f_088D7F48(Cpu *c); void f_08818CB8(Cpu *c); void f_08816AE4(Cpu *c); void f_08887AFC(Cpu *c);
void f_08837690(Cpu *c); void f_0880A6CC(Cpu *c); void f_088D2B5C(Cpu *c); void f_08818CD8(Cpu *c);
void f_0880B6CC(Cpu *c); void f_088054F0(Cpu *c); void f_08816F5C(Cpu *c); void f_089E3868(Cpu *c);
void f_089E3248(Cpu *c); void f_089F3430(Cpu *c); void f_089E35F4(Cpu *c); void f_089E3158(Cpu *c);
void f_089E3620(Cpu *c); void f_089E3618(Cpu *c);
void hle_sceKernelPowerLock(Cpu *c); void hle_sceUtilitySavedataInitStart(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (n, slot): the save screen's load: the slot and n are kept (0x09523C66, 0x09523C6C) with
   0x09523C68 cleared, and the n-th entry of 0x08A07768 is loaded into 0x08AEF4D8. */
SFA_FUNC(08816A78, save_screen_load) {
    uint32_t n = ARG(0), slot = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(0x09523C60u + 6) = (uint8_t)slot;
    *B(0x09523C60u + 12) = (uint8_t)n;
    *B(0x09523C60u + 8) = 0;
    *W(c->r[29] + 16) = ra;
    ARG(0) = slot; ARG(1) = 0x08A07768u + (n << 4); ARG(2) = 0x08AEF4D8u; ARG(3) = 0;
    call0(c, f_089E3B6C, 0x08816ABCu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): unless x's +93 is set, the link's step by x's +22: 0 the next fight and 1 the fight from
   the start, each followed by 0x0880ABD4; 2 the link's start. */
SFA_FUNC(088E9D38, link_step_by_22) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(x + 93) == 0) {
        int32_t k = S16(x + 22);
        if (k == 0) {
            call2(c, x, y, f_088E973C, 0x088E9D74u);
            call0(c, f_0880ABD4, 0x088E9D7Cu);
        } else if (k == 1) {
            call2(c, x, y, f_088E967C, 0x088E9DB0u);
            call0(c, f_0880ABD4, 0x088E9DB8u);
        } else if (k == 2) call2(c, x, x, f_088D7F48, 0x088E9DCCu);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): the save screen's own loop, a and b kept at 0x09523C68 and 0x09523C6A. Every frame:
   0x08818CB8, the memory sticks counted, the step the byte at 0x09523C62 names in the table
   0x08A077F0, the screen's two parts, a line of text while the step is not 0 (the one at
   0x089FE098 with a set, the one at 0x089FE0A4 without), 0x088D2B5C, 0x08818CD8, 0x0880B6CC and
   the frame's end; until the step is 0. Then 0x08816F5C. A 0x40 frame keeping s0..s4. */
SFA_FUNC(08817834, save_screen_loop) {
    uint32_t a = ARG(0), b = ARG(1);
    const gaddr at = 0x09523C60u;
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 5; i++) *W(sp + 32 + 4 * i) = sv[i];
    c->r[20] = at;
    *B(at + 8) = (uint8_t)a;
    c->r[19] = 0x08A077F0u; c->r[18] = 0x089FE098u; c->r[17] = 0x089FE0A4u;
    *B(at + 10) = (uint8_t)b;
    c->r[16] = a;
    *W(sp + 52) = sv[5];
    do {
        call0(c, f_08818CB8, 0x08817884u);
        call0(c, f_08816AE4, 0x0881788Cu);
        uint32_t fn = *W(0x08A077F0u + ((uint32_t)(int32_t)S8(at + 2) << 2));
        c->r[5] = fn;
        ARG(0) = at; c->r[31] = 0x088178A4u;
        rt_call_indirect(c, fn);
        call0(c, f_08887AFC, 0x088178ACu);
        call0(c, f_08837690, 0x088178B4u);
        if (S8(at + 2) != 0) {
            ARG(1) = 2; ARG(2) = 0x78; ARG(3) = 5; c->r[8] = 0xB;
            if (a != 0) call1(c, 0x089FE098u, f_0880A6CC, 0x088178E0u);
            else call1(c, 0x089FE0A4u, f_0880A6CC, 0x088178FCu);
        }
        call0(c, f_088D2B5C, 0x08817904u);
        call0(c, f_08818CD8, 0x0881790Cu);
        call0(c, f_0880B6CC, 0x08817914u);
        call1(c, 1, f_088054F0, 0x0881791Cu);
    } while (S8(at + 2) != 0);
    call0(c, f_08816F5C, 0x08817930u);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x40;
}

/* Nothing. */
SFA_FUNC(089E3618, save_nothing) {
    (void)c;
}

/* (n, unit): n rounded up to a multiple of unit. */
SFA_FUNC(089E3868, round_up_to_unit) {
    uint32_t unit = ARG(1);
    DIVU(c, ARG(0) + unit - 1, unit);
    ARG(0) = c->lo;
    int64_t m = (int64_t)(int32_t)ARG(0) * (int32_t)unit;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    c->r[2] = c->lo;
}

/* The save's picture: the 0x2AFD bytes at 0x08AD51D0 into the dialog's +1412 to +1420. */
SFA_FUNC(089E35F4, save_picture_set) {
    ARG(1) = SAVE; ARG(2) = 0x08AD51D0u; ARG(0) = 0x2AFD;
    *W(SAVE + 1412) = 0x08AD51D0u;
    *W(SAVE + 1416) = 0x2AFD;
    *W(SAVE + 1420) = 0x2AFD;
}

/* (n): the room a save of n bytes needs, in the stick's units (the word at 0x08D5CDC0): n, the
   0x1330 bytes of its header and the 0x2AFD of its picture each rounded up, and one unit more
   than four divided by a thirty-second of the unit. A frame keeping s0..s2. */
SFA_FUNC(089E39B8, save_room_needed) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[5] = 0x08D60000u;
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    uint32_t unit = *W(0x08D5CDC0u);
    c->r[17] = unit; c->r[16] = 0x2AFD;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    ARG(1) = unit; call0(c, f_089E3868, 0x089E39E0u);
    c->r[18] = c->r[2];
    call2(c, 0x1330, unit, f_089E3868, 0x089E39F0u);
    c->r[18] += c->r[2];
    call2(c, 0x2AFD, unit, f_089E3868, 0x089E3A00u);
    ARG(0) = 4; ARG(1) = (uint32_t)((int32_t)unit >> 5);
    DIVU(c, 4, ARG(1));
    c->r[2] = c->r[18] + c->r[2];
    int64_t m = (int64_t)(int32_t)(c->lo + 1) * (int32_t)unit;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    ARG(0) = c->lo;
    c->r[2] += c->lo;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (params, i, d, e): the dialog's parameters for a save: filled (0x089E3248), the mode 1 at +48,
   1 at +56, d at +116 and e at +120 and +124, sixteen bytes from 0x08AD3D28 to +0x5DC, the i-th
   block of 0x504 bytes from 0x08AD3DC0 to +0x80, and the picture. A 0x30 frame keeping s0..s3. */
SFA_FUNC(089E3620, save_params_save) {
    gaddr p = ARG(0);
    uint32_t i = ARG(1), d = ARG(2), e = ARG(3);
    uint32_t sv[5];
    for (int k = 0; k < 4; k++) sv[k] = c->r[16 + k];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int k = 0; k < 4; k++) *W(sp + 16 + 4 * k) = sv[k];
    *W(sp + 32) = sv[4];
    c->r[16] = e; c->r[17] = i; c->r[18] = d; c->r[19] = p;
    call0(c, f_089E3248, 0x089E364Cu);
    *W(p + 48) = 1;
    *W(p + 56) = 1;
    *W(p + 116) = d;
    *W(p + 120) = e;
    *W(p + 124) = e;
    ARG(2) = 0x10; call2(c, p + 0x5DC, 0x08AD3D28u, f_089F3430, 0x089E367Cu);
    ARG(2) = 0x504; call2(c, p + 0x80, 0x08AD3DC0u + i * 0x504, f_089F3430, 0x089E36ACu);
    call1(c, p, f_089E35F4, 0x089E36B4u);
    for (int k = 0; k < 4; k++) c->r[16 + k] = sv[k];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (.., a, d, n): a save of d: 0x089E3158's answer for a, the parameters with n blocks of 512
   bytes, the byte at 0x08AD7CCD set, the power switch held, and the dialog. Returns -1 when the
   dialog refuses; else the word at 0x08AD3D38 goes to 0x100 and 0 comes back. A frame keeping
   s0..s2. */
SFA_FUNC(089E3AB0, save_start) {
    uint32_t a = ARG(1), d = ARG(2), n = ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[16] = n; c->r[17] = d;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = SAVE;
    call2(c, a, 0x08D60000u, f_089E3158, 0x089E3ADCu);
    ARG(3) = n << 9; ARG(2) = d;
    call2(c, SAVE, c->r[2], f_089E3620, 0x089E3AF0u);
    *B(0x08AD7CCDu) = 1;
    call1(c, 0, hle_sceKernelPowerLock, 0x089E3B04u);
    call1(c, SAVE, hle_sceUtilitySavedataInitStart, 0x089E3B0Cu);
    if (c->r[2] & 0x80000000u) {
        call0(c, f_089E3618, 0x089E3B24u);
        c->r[2] = 0xFFFFFFFFu;
    } else {
        *W(0x08AD0000u + 15672) = 0x100;
        c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
