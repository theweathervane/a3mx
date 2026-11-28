/* Three more of the library: a drawing list handed to the system when it begins, the wait for the
   lists at a routine's end, and the sound's sequences made ready. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E6D9C(Cpu *c); void f_089E6E40(Cpu *c); void f_089E7B1C(Cpu *c); void f_089ED064(Cpu *c);
void f_089ED33C(Cpu *c); void f_089EFE9C(Cpu *c); void f_089F0E28(Cpu *c);
void hle_sceGeListEnQueue(Cpu *c); void hle_sceGeListSync(Cpu *c);

/* A part of the routine that begins a drawing list (the drawing state in s0): when s3 is 0 the
   list in use (+72) is handed to the system with the state's +32 and the callbacks at 0x08A02460,
   the word at 0x08E692F8 cleared first; its number is kept at +24, or its error returned. */
SFA_FUNC(089E6D94, tail_list_enqueue) {
    gaddr st = c->r[16];
    if (c->r[19] == 0) {
        c->r[3] = *W(st + 72);
        ARG(2) = *W(st + 32);
        c->r[25] = 0x08A00000u;
        ARG(1) = *W(c->r[3] + 8);
        ARG(0) = *W(c->r[3] + 4);
        c->r[24] = 0x08E70000u;
        ARG(3) = 0x08A02460u;
        *W(0x08E692F8u) = 0;
        c->r[31] = 0x089E6ECCu; hle_sceGeListEnQueue(c);
        c->r[3] = c->r[2];
        if ((int32_t)c->r[2] < 0) { f_089E6E40(c); return; }
        *W(st + 24) = c->r[2];
    }
    f_089E6D9C(c);
}

/* The end of a routine: the drawing lists are waited for, then the frame's end. */
SFA_FUNC(089E7B34, tail_list_sync) {
    c->r[31] = 0x089E7B3Cu; hle_sceGeListSync(c);
    c->r[31] = *W(c->r[29]);
    f_089E7B1C(c);
}

/* (n): the sound's sequences are made ready: the voices, a buffer of 0x100, and the sequencer for
   n; then the 128 words at 0x08E73E80 are cleared, the 32 at 0x08E74084 set to 127, n kept at
   0x08E74080, the word at 0x08AD80D0 cleared and the 32 records of 16 bytes at 0x08E74104
   cleared. The first error is returned, or 0. A 0x10 frame keeping s0. */
SFA_FUNC(089ED320, sequences_init) {
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    *W(sp + 4) = c->r[31];
    c->r[16] = ARG(0);
    c->r[31] = 0x089ED334u; f_089ED064(c);
    c->r[3] = c->r[2];
    if (c->r[2] != 0) { f_089ED33C(c); return; }
    ARG(0) = 0x100; c->r[31] = 0x089ED358u; f_089EFE9C(c);
    ARG(0) = c->r[16]; c->r[31] = 0x089ED360u; f_089F0E28(c);
    c->r[3] = c->r[2];
    if (c->r[2] != 0) { f_089ED33C(c); return; }
    for (uint32_t i = 0; i < 128; i++) *W(0x08E73E80u + 4 * i) = 0;
    for (uint32_t i = 0; i < 32; i++) *W(0x08E74084u + 4 * i) = 0x7F;
    ARG(1) = 0x08E70000u; ARG(0) = 0x08AE0000u;
    *W(0x08E74080u) = c->r[16];
    *W(0x08AD80D0u) = 0;
    gaddr rec = 0x08E74104u;
    for (uint32_t i = 0; i < 32; i++, rec += 0x10) {
        *W(rec + 4) = 0; *W(rec) = 0; *W(rec + 12) = 0; *W(rec + 8) = 0;
    }
    c->r[2] = 0xFFFFFFFFu;
    c->r[3] = 0;
    f_089ED33C(c);
}

void f_089E7B18(Cpu *c);
void hle_sceGeDrawSync(Cpu *c); void hle_sceUtilitySavedataUpdate(Cpu *c);
void hle_sceUtilitySavedataShutdownStart(Cpu *c); void hle_sceKernelPowerUnlock(Cpu *c);

/* (mode, a): the wait for drawing: mode 0 waits for all of it (with a), 3 and 4 wait for one list
   (the numbers at 0x08E69318 and 0x08E6931C), anything else returns 0. A 0x10 frame the ends
   close. */
SFA_FUNC(089E7AE4, draw_sync) {
    uint32_t mode = ARG(0);
    c->r[29] -= 0x10;
    c->r[3] = mode < 5;
    *W(c->r[29]) = c->r[31];
    if (mode >= 5) { c->r[2] = 0; f_089E7B18(c); return; }
    c->r[7] = 0x08A00000u;
    c->r[6] = mode << 2;
    c->r[2] = 0x08A024D4u;
    ARG(0) = c->r[6] + c->r[2];
    c->r[3] = *W(ARG(0));
    switch (mode) {
    case 0:
        ARG(0) = ARG(1);
        c->r[31] = 0x089E7B18u; hle_sceGeDrawSync(c);
        f_089E7B18(c);
        return;
    case 3:
        c->r[8] = 0x08E70000u;
        ARG(0) = *W(0x08E69318u);
        f_089E7B34(c);
        return;
    case 4:
        c->r[9] = 0x08E70000u;
        ARG(0) = *W(0x08E6931Cu);
        f_089E7B34(c);
        return;
    default:
        c->r[2] = 0;
        f_089E7B18(c);
        return;
    }
}

/* The save dialog's step, by its state (0x08AD7CD0): 3 starts its shutdown and unlocks the power
   switch (the byte at 0x08AD7CCD cleared), 2 waits for the drawing and updates it; both return
   -1, as does any other state but 0 and 0x80110005, which clear the word at 0x08AD3D38 and
   return 0. A 0x20 frame. */
SFA_FUNC(089E32CC, save_dialog_step) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t state = *W(0x08AD7CD0u);
    ARG(0) = state;
    *W(c->r[29] + 16) = ra;
    c->r[2] = 0xFFFFFFFFu;
    if (state == 3) {
        ARG(1) = 2;
        c->r[31] = 0x089E3344u; hle_sceUtilitySavedataShutdownStart(c);
        ARG(0) = 0; c->r[31] = 0x089E334Cu; hle_sceKernelPowerUnlock(c);
        ARG(0) = 0x08AD0000u;
        *B(0x08AD7CCDu) = 0;
        c->r[2] = 0xFFFFFFFFu;
    } else if (state == 2) {
        ARG(0) = 3; ARG(1) = 0; c->r[31] = 0x089E3324u; f_089E7AE4(c);
        ARG(0) = 1; c->r[31] = 0x089E332Cu; hle_sceUtilitySavedataUpdate(c);
        c->r[2] = 0xFFFFFFFFu;
    } else {
        ARG(1) = 0x80110000u;
        if (state != 0) ARG(1) = 0x80110005u;
        if (state == 0 || state == 0x80110005u) {
            ARG(0) = 0x08AD0000u;
            *W(0x08AD3D38u) = 0;
            c->r[2] = 0;
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The save dialog's step while it is up (the word at 0x08AD3D38 has 1 in its second byte);
   otherwise the word is cleared and 0 returned. A 0x10 frame. */
SFA_FUNC(089E3C30, save_dialog_poll) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    ARG(0) = 0x08AD0000u;
    ARG(2) = *W(0x08AD3D38u) & 0xFF00;
    ARG(1) = 0x100;
    *W(c->r[29]) = ra;
    if (ARG(2) != 0x100) {
        *W(0x08AD3D38u) = 0;
        c->r[2] = 0;
    } else {
        c->r[31] = 0x089E3C58u; f_089E32CC(c);
    }
    c->r[31] = ra; c->r[29] += 0x10;
}
