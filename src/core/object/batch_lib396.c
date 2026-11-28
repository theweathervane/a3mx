/* The drawing's start: a drawing list begun, and the whole display set up at the program's
   start. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE 0x08E69300u /* the drawing state: +0 set once it is ready, +12 the kind of list in use,
                          +72 that list's record, +76.. a record of 252 bytes for each kind
                          (its +248 the kind in use before it, below 0 while it is not open) */

void f_089E6E40(Cpu *c); void f_089E6D94(Cpu *c); void f_089E6BB8(Cpu *c); void f_089E7B6C(Cpu *c);
void f_089E73F0(Cpu *c); void f_089E7BD8(Cpu *c); void f_089E7DD8(Cpu *c); void f_089E843C(Cpu *c);
void f_089E8384(Cpu *c); void f_089E85E4(Cpu *c); void f_089E8844(Cpu *c); void f_089E886C(Cpu *c);
void f_089E77F4(Cpu *c); void f_089E895C(Cpu *c); void f_089E7E34(Cpu *c); void f_089E8818(Cpu *c);
void f_089E8AF8(Cpu *c); void f_089E8BC0(Cpu *c); void f_089E8C8C(Cpu *c); void f_089E7E8C(Cpu *c);
void f_089E8510(Cpu *c); void f_089E8290(Cpu *c); void f_089E7894(Cpu *c); void f_089E7AE4(Cpu *c);
void hle_sceKernelCpuSuspendIntr(Cpu *c); void hle_sceKernelCpuResumeIntr(Cpu *c);
void hle_sceGeEdramGetAddr(Cpu *c); void hle_sceDisplaySetMode(Cpu *c);
void hle_sceDisplaySetFrameBuf(Cpu *c); void hle_sceDisplayWaitVblank(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void call4(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t e, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; ARG(3) = e; c->r[31] = back; fn(c);
}

/* The end of a routine that begins a drawing list when it cannot: no list is in use any more,
   and r3 is returned. */
SFA_FUNC(089E6F5C, tail_list_begin_fail) {
    *W(c->r[17] + 72) = 0;
    f_089E6E40(c);
}

/* (kind, list, flags): a drawing list of that kind is begun at `list`. 0x80000001 before the
   drawing is ready, 0x80000021 when that kind is open already. Otherwise its record becomes the
   one in use (the kind in use before kept in it), with the list's start, its next place and the
   flags. Kinds 3 and 4 take the address as it is; the others take it as an uncached one and,
   unless bit 30 of the address is set, clear the list from its next place up to the next
   boundary of 64 bytes. The routine's other parts hand the list to the system and write its
   first commands. A 0x30 frame keeping s0..s4 and f20, which the ends close. */
SFA_FUNC(089E6CEC, list_begin) {
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = c->r[20];
    c->r[20] = 0x08E70000u;
    *W(sp + 12) = c->r[19];
    c->r[19] = ARG(2);
    *W(sp + 8) = c->r[18];
    c->r[18] = ARG(1);
    *W(sp + 4) = c->r[17];
    c->r[17] = GE;
    *W(sp) = c->r[16];
    *W(sp + 20) = c->r[31];
    *W(sp + 32) = c->f[20];
    c->r[3] = *W(GE);
    uint32_t kind = ARG(0);
    c->r[16] = kind;
    if (c->r[3] == 0) {
        c->r[2] = 0x80000000u;
        c->r[3] = 0x80000001u;
        f_089E6F5C(c);
        return;
    }
    c->r[31] = 0x089E6D30u; hle_sceKernelCpuSuspendIntr(c);
    uint32_t off = kind * 252;
    ARG(1) = off;
    gaddr rec = GE + off;
    c->r[6] = rec;
    c->r[3] = *W(rec + 324);
    ARG(0) = c->r[2];
    if ((int32_t)c->r[3] >= 0) {
        c->r[31] = 0x089E6F54u; hle_sceKernelCpuResumeIntr(c);
        ARG(0) = 0x80000000u;
        c->r[3] = 0x80000021u;
        f_089E6F5C(c);
        return;
    }
    c->r[10] = *W(GE + 12);
    c->r[11] = GE + 0x4C;
    c->r[9] = off + GE + 0x4C;
    *W(rec + 324) = c->r[10];
    *W(GE + 72) = c->r[9];
    *W(GE + 12) = kind;
    c->r[31] = 0x089E6D68u; hle_sceKernelCpuResumeIntr(c);
    c->r[8] = kind - 3;
    c->r[7] = c->r[8] < 2;
    gaddr list = c->r[18];
    if (c->r[7] != 0) {
        gaddr in_use = *W(GE + 72);
        c->r[12] = in_use;
        c->r[13] = list & 0x1FFFFFFFu;
        *W(in_use + 12) = c->r[19];
        *W(in_use + 8) = c->r[13];
        *W(in_use + 4) = c->r[13];
    } else {
        gaddr in_use = *W(GE + 72);
        c->r[15] = in_use;
        c->r[16] = 0x40000000u;
        ARG(0) = 0x40000000u | (list & 0x1FFFFFFFu);
        c->r[14] = (list >> 30) & 1;
        *W(in_use + 12) = c->r[19];
        *W(in_use + 8) = ARG(0);
        *W(in_use + 4) = ARG(0);
        if (c->r[14] == 0) {
            c->r[31] = 0x089E6F04u; hle_sceKernelCpuSuspendIntr(c);
            ARG(0) = c->r[2];
            in_use = *W(GE + 72);
            gaddr p = *W(in_use + 8);
            ARG(1) = p;
            c->r[17] = p & 0x3F;
            c->r[6] = in_use;
            c->r[3] = in_use;
            if (c->r[17] != 0) {
                *W(p) = 0;
                for (;;) {
                    p += 4;
                    c->r[18] = p & 0x3F;
                    if (c->r[18] == 0) break;
                    *W(p) = 0;
                }
                c->r[3] = p;
                *W(in_use + 8) = p;
            }
            c->r[16] = GE;
            c->r[31] = 0x089E6F44u; hle_sceKernelCpuResumeIntr(c);
            c->r[19] = *W(GE + 12);
            f_089E6D94(c);
            return;
        }
    }
    c->r[16] = GE;
    c->r[19] = *W(GE + 12);
    f_089E6D94(c);
}

/* The display is set up at the program's start: the video memory's four areas noted (the two
   screens and the depth buffer, 0x44000 bytes each), the mode 480 by 272 set with no picture
   yet, the drawing started, and a first list (at 0x08D65C40) that gives every setting its
   starting value: the buffers, the offset and viewport for the middle of a 4096 square, the
   states switched on (9, 4, 0) and off (twelve of them), and so on; the list is finished and
   waited for, and one screen refresh waited. A 0x30 frame keeping s0 and f20. */
SFA_FUNC(089E4110, display_setup) {
    uint32_t s0 = c->r[16], ra = c->r[31], f20 = c->f[20];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = f20;
    *W(sp + 36) = s0; *W(sp + 40) = ra;
    c->r[31] = 0x089E4128u; hle_sceGeEdramGetAddr(c);
    uint32_t base = c->r[2];
    *W(0x08D65C00u) = base;
    *W(0x08D65C04u) = base + 0x44000;
    *W(0x08D65C08u) = base + 0x88000;
    *W(0x08D65C0Cu) = base + 0xCC000;
    *W(0x09525E08u) = base + 0xCC000 - 0x1000;
    *B(0x08E65C65u) = 0;
    *W(0x08E65C44u) = 0;
    *B(0x08E65C64u) = 0;
    c->r[16] = 0x08E60000u;
    *W(0x08E65C48u) = 0x08D65C40u;
    call3(c, 0, 0x1E0, 0x110, hle_sceDisplaySetMode, 0x089E41A0u);
    call4(c, 0, 0x200, 1, 1, hle_sceDisplaySetFrameBuf, 0x089E41B4u);
    c->r[31] = 0x089E41BCu; f_089E6BB8(c);
    call1(c, 1, f_089E7B6C, 0x089E41C4u);
    call3(c, 0, *W(0x08E65C48u), 4, f_089E6CEC, 0x089E41D4u);
    call3(c, 1, 0, 0x200, f_089E73F0, 0x089E41E4u);
    call4(c, 0x1E0, 0x110, 0x44000, 0x200, f_089E7BD8, 0x089E41FCu);
    call2(c, 0x88000, 0x200, f_089E7DD8, 0x089E420Cu);
    call2(c, 0x710, 0x778, f_089E843C, 0x089E4218u);
    call4(c, 0x800, 0x800, 0x1E0, 0x110, f_089E8384, 0x089E422Cu);
    call1(c, 1, f_089E85E4, 0x089E4234u);
    c->f[20] = 0;
    c->f[12] = 0;
    call1(c, 0, f_089E8844, 0x089E4244u);
    call3(c, 0, 0, 0, f_089E886C, 0x089E4254u);
    ARG(0) = 0x3F800000u;
    c->f[12] = 0x3F800000u; c->f[13] = 0x3F800000u;
    c->r[31] = 0x089E4264u; f_089E77F4(c);
    c->f[12] = c->f[20]; c->f[13] = c->f[20];
    c->r[31] = 0x089E4270u; f_089E895C(c);
    call1(c, 9, f_089E7E34, 0x089E4278u);
    call1(c, 4, f_089E7E34, 0x089E4280u);
    call1(c, 0, f_089E7E34, 0x089E4288u);
    call2(c, 0, 0, f_089E8818, 0x089E4294u);
    call3(c, 6, 0, 0xFF, f_089E8AF8, 0x089E42A4u);
    call4(c, 0, 0, 0x1E0, 0x110, f_089E8BC0, 0x089E42B8u);
    c->r[8] = 0;
    call4(c, 0, 2, 3, 0, f_089E8C8C, 0x089E42D0u);
    static const uint8_t off[12] = { 5, 7, 6, 0xA, 0x10, 3, 0x12, 0x11, 0xF, 8, 1, 2 };
    for (uint32_t i = 0; i < 12; i++) call1(c, off[i], f_089E7E8C, 0x089E42D8u + 8 * i);
    call1(c, 0xFFFFFFFFu, f_089E8510, 0x089E4338u);
    call1(c, 0, f_089E8290, 0x089E4340u);
    c->r[31] = 0x089E4348u; f_089E7894(c);
    call2(c, 0, 0, f_089E7AE4, 0x089E4354u);
    c->r[31] = 0x089E435Cu; hle_sceDisplayWaitVblank(c);
    c->f[20] = *W(sp + 32);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}
