/* The main thread's start and the program's end: the arguments split, the library's state for
   the thread made, main called, and what the library does when a program exits. Offsets stay
   numbers until something shows what they are.

   The original has calls here through addresses the linker left at 0 (hooks a program may
   provide); each sits behind a test of that address, so those paths never run and are left out,
   apart from the one call after exit that has no test.
 */
#include "step.h"
#include "../guest_mem.h"

#define REENT 0x08AD84DCu /* points at the library's state */

void f_089F7310(Cpu *c); void f_089F351C(Cpu *c); void f_089F3470(Cpu *c); void f_089F3224(Cpu *c);
void f_089FAEB0(Cpu *c); void f_08804AF8(Cpu *c); void f_088041A4(Cpu *c); void f_0880416C(Cpu *c);
void hle_sceKernelSelfStopUnloadModule(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* The library's state, in r2. */
SFA_FUNC(089F2F1C, lib_state_get) {
    ARG(0) = 0x08AE0000u;
    c->r[2] = *W(REENT);
}

/* (state, block): a chain of blocks is freed, the last first. A 0x20 frame keeping s0, s1. */
SFA_FUNC(089F2F44, lib_chain_free) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = ARG(0);
    gaddr block = ARG(1);
    ARG(0) = *W(block);
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = block;
    if (ARG(0) != 0) call2(c, c->r[17], ARG(0), f_089F2F44, 0x089F2F70u);
    call2(c, c->r[17], c->r[16], f_089F7310, 0x089F2F7Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (state): a library state other than the program's own is taken down: its fifteen lists of
   blocks (+76) and their table, its blocks of routines for the program's end (+328, but not the
   one inside it), the block at +84, and, once it was set up (+56), its clean-up routine (+60)
   and the chain at +472. A 0x30 frame keeping s0..s3. */
SFA_FUNC(089F2F90, lib_state_reclaim) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    gaddr r = ARG(0);
    ARG(1) = *W(REENT);
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = s3; *W(sp + 32) = ra;
    c->r[16] = r;
    if (r != *W(REENT)) {
        c->r[17] = 0;
        if (*W(r + 76) != 0) {
            c->r[18] = 0;
            for (; (int32_t)c->r[17] < 15; c->r[17]++, c->r[18] += 4) {
                c->r[19] = *W(*W(r + 76) + c->r[18]);
                while (c->r[19] != 0) {
                    gaddr block = c->r[19];
                    c->r[19] = *W(block);
                    call2(c, r, block, f_089F7310, 0x089F2FE8u);
                }
            }
            call2(c, r, *W(r + 76), f_089F7310, 0x089F3010u);
        }
        c->r[18] = *W(r + 328);
        c->r[17] = r + 0x14C;
        if (c->r[18] != 0 && c->r[18] != c->r[17]) {
            do {
                gaddr block = c->r[18];
                c->r[18] = *W(block);
                call2(c, r, block, f_089F7310, 0x089F303Cu);
            } while (c->r[18] != c->r[17]);
        }
        if (*W(r + 84) != 0) call2(c, r, *W(r + 84), f_089F7310, 0x089F3058u);
        if (*W(r + 56) != 0) {
            ARG(1) = *W(r + 60);
            ARG(0) = r;
            c->r[31] = 0x089F3070u; rt_call_indirect(c, ARG(1));
            if (*W(r + 472) != 0) call2(c, r, *W(r + 472), f_089F2F44, 0x089F3084u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}

/* (code): the program's last step: the module stops and unloads itself. A 0x20 frame keeping
   s0, s1. */
SFA_FUNC(08804558, program_end) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    c->r[17] = ARG(0);
    c->r[3] = 0;
    *W(sp + 24) = ra;
    *W(sp + 16) = s0;
    ARG(0) = 1; ARG(1) = 0; ARG(2) = 0;
    c->r[31] = 0x08804604u; hle_sceKernelSelfStopUnloadModule(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (code): exit: the routines noted for the program's end run, the newest first, then the
   library's own clean-up (the state's +60), then the program's last step. A 0x30 frame keeping
   s0..s4. */
SFA_FUNC(089F32C8, lib_exit) {
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 24) = sv[2];
    c->r[16] = ARG(0);
    c->r[18] = 0x08AE0000u;
    *W(sp + 20) = sv[1];
    c->r[17] = *W(*W(REENT) + 328);
    *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    for (; c->r[17] != 0; c->r[17] = *W(c->r[17])) {
        for (c->r[20] = *W(c->r[17] + 4) - 1; (int32_t)c->r[20] >= 0; c->r[20]--) {
            c->r[19] = c->r[17] + (c->r[20] << 2);
            ARG(0) = *W(c->r[19] + 8);
            c->r[31] = 0x089F331Cu; rt_call_indirect(c, ARG(0));
        }
    }
    ARG(0) = *W(REENT);
    ARG(1) = *W(ARG(0) + 60);
    if (ARG(1) != 0) { c->r[31] = 0x089F334Cu; rt_call_indirect(c, ARG(1)); }
    ARG(0) = c->r[16];
    c->r[31] = 0x089F3354u; f_08804558(c);
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* The main thread's last part (s4 the number of arguments, the frame's bottom their list): main
   is called and exit given its answer. Exit does not come back; what follows it in the original
   (a routine noted, a call through an empty hook, and main again) is kept as it stands. */
SFA_FUNC(088041A4, main_call) {
    for (;;) {
        ARG(1) = c->r[29];
        c->r[31] = 0x088041ACu; f_08804AF8(c);
        ARG(0) = c->r[2];
        c->r[31] = 0x088041B4u; f_089F32C8(c);
        c->r[31] = 0x088041BCu; f_089F3224(c);
        c->r[31] = 0x088041C4u; rt_call_indirect(c, 0);
        ARG(0) = c->r[20];
    }
}

/* The main thread's middle part: the thread's library state (s5) is put where the system keeps
   it for the thread (+4 of k0), the routine at 0x089FAEB8 is noted for the program's end, and
   the library's start-up routine runs; then main is called. */
SFA_FUNC(0880416C, main_thread_ready) {
    *W(c->r[26] + 4) = c->r[21];
    ARG(1) = 0x08A00000u;
    ARG(0) = 0x089FAEB8u;
    c->r[31] = 0x0880417Cu; f_089F3224(c);
    c->r[31] = 0x08804184u; f_089FAEB0(c);
    c->r[3] = 0;
    c->r[21] = 0;
    c->r[6] = 0;
    ARG(0) = c->r[20];
    f_088041A4(c);
}

/* (size, arguments): the main thread: the arguments (strings one after another, `size` bytes of
   them, at most 20) are listed at the bottom of a 0x3F0 frame with a 0 after the last; a
   library state for the thread is built in the frame from +0x50 (its three files' records
   named at +4, +8 and +12, its name at +52, the random numbers' seed and multiplier at +176
   and +182, and the rest cleared); then the rest of the start. The thread does not return. */
SFA_FUNC(088040BC, main_thread) {
    c->r[29] -= 0x3F0;
    gaddr sp = c->r[29];
    *W(sp + 996) = c->r[21];
    gaddr state = sp + 0x50;
    c->r[21] = state;
    *W(sp + 992) = c->r[20];
    c->r[20] = 0;
    *W(sp + 988) = c->r[19];
    gaddr args = ARG(1);
    c->r[19] = args;
    *W(sp + 984) = c->r[18];
    int32_t size = (int32_t)ARG(0);
    c->r[18] = (uint32_t)size;
    *W(sp + 976) = c->r[16];
    c->r[16] = args;
    *W(sp + 1000) = c->r[31];
    *W(sp + 980) = c->r[17];
    if (size > 0) {
        c->r[17] = sp;
        *W(sp) = args;
        for (;;) {
            ARG(0) = c->r[16];
            c->r[20]++;
            c->r[31] = 0x08804108u; f_089F351C(c);
            c->r[16] = c->r[16] + c->r[2] + 1;
            c->r[3] = c->r[16] - args;
            c->r[17] += 4;
            if ((int32_t)c->r[20] >= 20) break;
            if ((int32_t)c->r[3] >= size) break;
            *W(c->r[17]) = c->r[16];
        }
    }
    *W(sp + (c->r[20] << 2)) = 0;
    *W(state + 8) = state + 0x2C4;
    *W(state + 12) = state + 0x320;
    *W(state) = 0;
    *W(state + 4) = state + 0x268;
    *W(state + 16) = 0;
    for (uint32_t i = 0; i < 25; i++) *B(state + 0x14 + i) = 0;
    *W(state + 52) = 0x089FD034u;
    *W(state + 48) = 0;
    for (uint32_t at = 56; at <= 92; at += 4) *W(state + at) = 0;
    *B(state + 96) = 0;
    for (uint32_t i = 0; i < 0x24; i++) *B(state + 0x7C + i) = 0;
    *W(state + 168) = 1;
    *W(state + 172) = 0;
    *H(state + 176) = 0x330E;
    *H(state + 178) = 0xABCD;
    *H(state + 188) = 0xB;
    *H(state + 180) = 0x1234;
    *H(state + 182) = 0xE66D;
    *H(state + 184) = 0xDEEC;
    *H(state + 186) = 5;
    *W(state + 160) = 0;
    for (uint32_t at = 192; at <= 212; at += 4) *W(state + at) = 0;
    for (uint32_t at = 252; at <= 288; at += 4) *W(state + at) = 0;
    *B(state + 216) = 0;
    *B(state + 224) = 0;
    *W(state + 248) = 0;
    for (uint32_t at = 328; at <= 340; at += 4) *W(state + at) = 0;
    *W(state + 596) = 0;
    *W(state + 468) = 0;
    for (uint32_t at = 600; at <= 612; at += 4) *W(state + at) = 0;
    ARG(0) = state + 0x268; ARG(1) = 0; ARG(2) = 0x114;
    c->r[31] = 0x08804358u; f_089F3470(c);
    *W(state + 892) = 0;
    f_0880416C(c);
}
