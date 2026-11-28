/* The frame's start on the game's side, and seven more of the library: a sound entry's report,
   the two callbacks the system calls back, the display's mode, a colour on the drawing list, a
   sound bank entered, and a voice reset. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE 0x08E69300u /* the drawing state */

void f_0880A020(Cpu *c); void f_089E3138(Cpu *c); void f_089E0BF4(Cpu *c); void f_089E0BFC(Cpu *c);
void f_088098CC(Cpu *c); void f_089E4034(Cpu *c); void f_089E403C(Cpu *c); void f_089F3470(Cpu *c);
void f_089ECECC(Cpu *c); void f_089EAB14(Cpu *c); void f_089E73B8(Cpu *c); void f_089ED51C(Cpu *c);
void hle_sceKernelCreateCallback(Cpu *c); void hle_sceKernelDeleteCallback(Cpu *c);
void hle_scePowerRegisterCallback(Cpu *c); void hle_sceIoDevctl(Cpu *c);
void hle_sceDisplaySetMode(Cpu *c); void hle_sceDisplaySetFrameBuf(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* The frame's start: the frame count at 0x08E75C58 and the one at 0x08A05908 (unless it is -1)
   move on; 0x0880A020 and 0x089E3138 run; a colour waiting at 0x08E75C5C goes to 0x089E0BF4 and
   one at 0x08E75A14 to 0x089E0BFC; then 0x088098CC, and the two calls that answer 0. A frame
   keeping s0, s1. */
SFA_FUNC(088059E8, frame_start) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    *W(0x08E75C58u) = *W(0x08E75C58u) + 1;
    uint32_t n = *W(0x08A058A0u + 104);
    ARG(0) = n; ARG(1) = 0x08A058A0u; ARG(2) = 0xFFFFFFFFu;
    c->r[17] = 0x08E70000u; c->r[16] = 0x08E70000u;
    if (n != 0xFFFFFFFFu) {
        ARG(0) = n + 1;
        *W(0x08A058A0u + 104) = n + 1;
    }
    call0(c, f_0880A020, 0x08805A34u);
    call0(c, f_089E3138, 0x08805A3Cu);
    if (*W(0x08E75C5Cu) != 0) {
        ARG(1) = *W(0x08E75C64u);
        call1(c, *W(0x08E75C60u), f_089E0BF4, 0x08805A58u);
        *W(0x08E75C5Cu) = 0;
    }
    if (*W(0x08E75A14u) != 0) {
        call1(c, *W(0x08E75A10u), f_089E0BFC, 0x08805A70u);
        *W(0x08E75A14u) = 0;
    }
    call0(c, f_088098CC, 0x08805A7Cu);
    call0(c, f_089E4034, 0x08805A84u);
    if (c->r[2] == 6 || c->r[2] == 0) call0(c, f_089E403C, 0x08805AA4u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (entry, out): the 0x1C bytes at out are cleared, and for one of the sixteen entries of
   0x08AD3AF8 whose voice has not ended out's +24 is set. Returns 0. A frame keeping s0..s2. */
SFA_FUNC(089E203C, entry_report) {
    gaddr entry = ARG(0), out = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[16] = out; c->r[17] = entry;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    call3(c, out, 0, 0x1C, f_089F3470, 0x089E2068u);
    uint32_t i = 0;
    while (i < 16 && 0x08AD3AF8u + i * 0x14 != entry) i++;
    c->r[18] = i;
    if (i != 16) {
        call0(c, f_089ECECC, 0x089E20A8u);
        if (((~c->r[2]) >> (i & 31)) & 1) *W(out + 24) = 1;
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The two callbacks the system calls back are made: the memory stick's (0x089E3EB4, named at
   0x08A01DD4, kept at 0x08D64DD4 and given to the stick's driver; deleted again when that fails)
   and the power switch's (0x089E3EE4, named at 0x08A01DDC, kept at 0x08D64DD8 and registered,
   the slot kept at 0x08AD7CD8; deleted again when that fails). A 0x30 frame keeping s0, s1. */
SFA_FUNC(089E3F00, callbacks_make) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s0; *W(sp + 40) = s1; *W(sp + 44) = ra;
    call3(c, 0x08A01DD4u, 0x089E3EB4u, 0, hle_sceKernelCreateCallback, 0x089E3F28u);
    c->r[17] = 0x08D60000u;
    c->r[16] = c->r[2];
    *W(0x08D64DD4u) = c->r[2];
    if ((int32_t)c->r[2] >= 0) {
        *W(sp + 32) = c->r[2];
        ARG(3) = 4; c->r[8] = 0; c->r[9] = 0;
        call3(c, 0x08A01DC0u, 0x02415821u, sp + 0x20, hle_sceIoDevctl, 0x089E3F60u);
        c->r[16] = 0x08D60000u;
        if ((int32_t)c->r[2] < 0) call1(c, *W(0x08D64DD4u), hle_sceKernelDeleteCallback, 0x089E3F70u);
        call3(c, 0x08A01DDCu, 0x089E3EE4u, 0, hle_sceKernelCreateCallback, 0x089E3F88u);
        uint32_t cb = c->r[2];
        *W(0x08D64DD8u) = cb;
        if ((int32_t)cb >= 0) {
            ARG(1) = cb;
            call1(c, 0xFFFFFFFFu, hle_scePowerRegisterCallback, 0x089E3FA0u);
            *W(0x08AD7CD8u) = c->r[2];
            if ((int32_t)c->r[2] < 0) call1(c, *W(0x08D64DD8u), hle_sceKernelDeleteCallback, 0x089E3FB4u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* (format, w, h, offset, width): the display's mode: the three go to 0x08E71C48 on, the drawing
   state keeps them all (+36, +60, +64, +48, +40), the display is set to w by h, and while the
   state's +8 is 1 the screen's buffer is shown again. A 0x20 frame keeping s0..s5. */
SFA_FUNC(089E732C, display_mode_set) {
    uint32_t format = ARG(0), w = ARG(1), h = ARG(2), offset = ARG(3), width = c->r[8];
    uint32_t sv[6];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    for (int i = 0; i < 6; i++) *W(sp + 4 * (uint32_t)i) = sv[i];
    *W(sp + 24) = ra;
    c->r[21] = format; c->r[20] = width; c->r[18] = offset; c->r[17] = h; c->r[16] = w;
    call0(c, f_089EAB14, 0x089E7364u);
    c->r[19] = GE;
    c->r[3] = *W(GE + 40);
    *W(GE + 60) = w;
    *W(GE + 64) = h;
    *W(GE + 48) = offset;
    *W(GE + 36) = format;
    *W(GE + 40) = width;
    call3(c, 0, *W(GE + 60), *W(GE + 64), hle_sceDisplaySetMode, 0x089E739Cu);
    c->r[2] = *W(GE + 8);
    ARG(0) = 1; ARG(1) = width; ARG(2) = format; ARG(3) = 1;
    if (c->r[2] == 1) {
        c->r[9] = *W(GE + 48);
        c->r[8] = *W(GE + 20);
        call1(c, c->r[8] + c->r[9], hle_sceDisplaySetFrameBuf, 0x089E73E8u);
    }
    c->r[31] = *W(sp + 24);
    f_089E73B8(c);
}

/* (list, which, colour): the colour on the list given, for each of the bits of `which`: 1 the
   commands 0x55 and 0x58 (its top byte), 2 the command 0x56, 4 the command 0x57. */
SFA_FUNC(089EA1E8, list_colour) {
    gaddr list = ARG(0);
    uint32_t which = ARG(1), colour = ARG(2);
    uint32_t low = colour & 0x00FFFFFFu;
    c->r[2] = 0x00FFFFFFu;
    c->r[24] = 0x55000000u; c->r[14] = 0x56000000u;
    c->r[3] = colour >> 24;
    c->r[10] = low | 0x57000000u;
    c->r[11] = which & 4;
    c->r[7] = low | 0x55000000u;
    ARG(1) = which & 2;
    c->r[8] = low | 0x56000000u;
    ARG(2) = (colour >> 24) | 0x58000000u;
    c->r[12] = which & 1;
    if (which & 1) {
        c->r[3] = *W(list + 8);
        c->r[12] = c->r[3] + 4;
        c->r[25] = c->r[12] + 4;
        *W(c->r[3]) = c->r[7];
        *W(list + 8) = c->r[25];
        *W(c->r[12]) = ARG(2);
    }
    if (which & 2) {
        ARG(2) = *W(list + 8);
        ARG(1) = ARG(2) + 4;
        *W(ARG(2)) = c->r[8];
        *W(list + 8) = ARG(1);
    }
    c->r[9] = 0x58000000u;
    if (which & 4) {
        c->r[9] = *W(list + 8);
        c->r[11] = c->r[9] + 4;
        *W(c->r[9]) = c->r[10];
        *W(list + 8) = c->r[11];
    }
}

/* (bank, a, b): a sound bank is entered in the first free place of the 0x80 at 0x08E73E80, with
   a and b kept in its first two words. Returns the place's number; 0x80450001 before the sound
   is started, 0x8054000A without any of the three, and 0x8054000F when there is no room, the 32
   entries at 0x08E74104 being cleared then. */
SFA_FUNC(089ED490, bank_enter) {
    gaddr bank = ARG(0);
    uint32_t a = ARG(1), b = ARG(2);
    c->r[9] = *W(0x08AD80D0u);
    c->r[7] = 0x80450001u;
    if (c->r[9] != 0xFFFFFFFFu) {
        if (bank == 0 || a == 0 || b == 0) c->r[7] = 0x8054000Au;
        else {
            gaddr slot = 0x08E73E80u;
            uint32_t i = 0;
            int found = 0;
            for (; i < 0x80; i++, slot += 4) {
                if (*W(slot) == 0) { found = 1; break; }
            }
            c->r[8] = slot;
            c->r[7] = i;
            if (found) {
                *W(bank + 4) = b;
                *W(slot) = bank;
                *W(bank) = a;
            } else {
                gaddr e = 0x08E74104u;
                for (int k = 0; k < 32; k++, e += 0x10) {
                    *W(e + 4) = 0; *W(e) = 0; *W(e + 12) = 0; *W(e + 8) = 0;
                }
                c->r[2] = e;
                c->r[7] = 0x8054000Fu;
            }
        }
    }
    f_089ED51C(c);
}

/* (voice): a sound voice of 0x68 bytes back to its start: 1 at +28, -1 at +16, +20, +44, +84,
   +88 and the byte +92, 0x40 at the bytes +9, +11, +56, +64, +97 and +99, and the rest 0. */
SFA_FUNC(089F2898, voice_reset) {
    gaddr v = ARG(0);
    ARG(2) = 0x40; ARG(1) = 0xFFFFFFFFu; c->r[3] = 1;
    *W(v + 28) = 1;
    *B(v + 92) = 0xFF;
    *B(v + 99) = 0x40;
    *W(v + 100) = 0;
    *B(v + 8) = 0; *B(v + 9) = 0x40; *B(v + 10) = 0; *B(v + 11) = 0x40;
    *W(v + 12) = 0;
    *W(v + 16) = 0xFFFFFFFFu;
    *W(v + 20) = 0xFFFFFFFFu;
    *W(v + 32) = 0; *W(v + 36) = 0;
    *B(v + 40) = 0;
    *W(v + 44) = 0xFFFFFFFFu;
    *W(v + 52) = 0;
    *B(v + 56) = 0x40;
    *W(v + 60) = 0;
    *B(v + 64) = 0x40;
    *W(v + 68) = 0; *W(v + 72) = 0;
    *B(v + 76) = 0; *B(v + 77) = 0; *B(v + 78) = 0; *B(v + 79) = 0; *B(v + 80) = 0;
    *W(v + 84) = 0xFFFFFFFFu;
    *W(v + 88) = 0xFFFFFFFFu;
    *B(v + 93) = 0;
    *B(v + 96) = 0;
    *B(v + 97) = 0x40;
    *B(v + 98) = 0;
}
