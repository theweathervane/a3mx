/* The library's loose ends: the last few instructions of longer routines that the original
   reaches from more than one place, and a handful of one-line leaves. Each of the ends only
   restores what its routine kept and leaves its frame. Offsets stay numbers until something shows
   what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void hle_sceUtilitySavedataGetStatus(Cpu *c); void hle_sceDisplayGetCurrentHcount(Cpu *c);
void hle_sceDisplayWaitVblank(Cpu *c); void hle_sceKernelCpuResumeIntr(Cpu *c);

/* Returns r3 from a 0x30 frame that kept s0..s3 at +16. */
SFA_FUNC(089E6CA4, tail_e6ca4) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 32);
    c->r[19] = *W(sp + 28);
    c->r[18] = *W(sp + 24);
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[2] = c->r[3];
    c->r[29] += 0x30;
}

/* Leaves a 0x20 frame that kept s0..s5. */
SFA_FUNC(089E73B8, tail_e73b8) {
    gaddr sp = c->r[29];
    c->r[21] = *W(sp + 20);
    c->r[20] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[29] += 0x20;
}

/* Leaves a 0x10 frame. */
SFA_FUNC(089E7B1C, tail_e7b1c) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[29] += 0x10;
}

/* Leaves a 0x10 frame. */
SFA_FUNC(089EA5E8, tail_ea5e8) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[29] += 0x10;
}

/* Leaves a 0x10 frame. */
SFA_FUNC(089ECEF0, tail_ecef0) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[29] += 0x10;
}

/* Returns r3 from a 0x10 frame. */
SFA_FUNC(089EA740, tail_ea740) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[2] = c->r[3];
    c->r[29] += 0x10;
}

/* Returns r3 from a 0x20 frame that kept s0..s3. */
SFA_FUNC(089EAC54, tail_eac54) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[3];
    c->r[29] += 0x20;
}

/* Returns r3 from a 0x10 frame that kept s0. */
SFA_FUNC(089ED33C, tail_ed33c) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[3];
    c->r[29] += 0x10;
}

/* Returns r5 from a 0x10 frame that kept s0. */
SFA_FUNC(089EDA4C, tail_eda4c) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[5];
    c->r[29] += 0x10;
}

/* Returns r7 from a 0x10 frame. */
SFA_FUNC(089EDE3C, tail_ede3c) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp);
    c->r[2] = c->r[7];
    c->r[29] += 0x10;
}

/* Returns r6 from a 0x20 frame that kept s0, s1 at +16. */
SFA_FUNC(089EEF3C, tail_eef3c) {
    gaddr sp = c->r[29];
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[2] = c->r[6];
    c->r[29] += 0x20;
}

/* Returns r7 from a 0x20 frame. */
SFA_FUNC(089EF08C, tail_ef08c) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[2] = c->r[7];
    c->r[29] += 0x20;
}

/* Returns r7 from a 0x20 frame. */
SFA_FUNC(089EF110, tail_ef110) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[2] = c->r[7];
    c->r[29] += 0x20;
}

/* Returns r7 from a 0x20 frame. */
SFA_FUNC(089EF194, tail_ef194) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[2] = c->r[7];
    c->r[29] += 0x20;
}

/* Returns r6 from a 0x10 frame that kept s0..s2. */
SFA_FUNC(089EF2B0, tail_ef2b0) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[6];
    c->r[29] += 0x10;
}

/* Returns 0 from a 0x10 frame that kept s0, s1. */
SFA_FUNC(089F0248, tail_f0248) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = 0;
    c->r[29] += 0x10;
}

/* Returns 0 from a 0x30 frame that kept s0..s4 at +16. */
SFA_FUNC(089F06CC, tail_f06cc) {
    gaddr sp = c->r[29];
    c->r[20] = *W(sp + 32);
    c->r[19] = *W(sp + 28);
    c->r[18] = *W(sp + 24);
    c->r[17] = *W(sp + 20);
    c->r[16] = *W(sp + 16);
    c->r[2] = 0;
    c->r[29] += 0x30;
}

/* Returns r5 from a 0x10 frame that kept s0, s1. */
SFA_FUNC(089F1670, tail_f1670) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[5];
    c->r[29] += 0x10;
}

/* Returns r6 from a 0x10 frame that kept s0. */
SFA_FUNC(089F1898, tail_f1898) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[6];
    c->r[29] += 0x10;
}

/* Returns 0 from a 0x10 frame. */
SFA_FUNC(089F1CE0, tail_f1ce0) {
    gaddr sp = c->r[29];
    c->r[5] = 0;
    c->r[31] = *W(sp);
    c->r[2] = 0;
    c->r[29] += 0x10;
}

/* Returns r6 from a 0x10 frame. */
SFA_FUNC(089F1F48, tail_f1f48) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp);
    c->r[2] = c->r[6];
    c->r[29] += 0x10;
}

/* Leaves a 0x30 frame. */
SFA_FUNC(089FB958, tail_fb958) {
    gaddr sp = c->r[29];
    (void)sp;
    c->r[29] += 0x30;
}

/* Returns r5 from a 0x40 frame. */
SFA_FUNC(089FBA78, tail_fba78) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 48);
    c->r[2] = c->r[5];
    c->r[29] += 0x40;
}

/* The save dialog's state is asked for and kept at 0x08AD7CD0. A 0x10 frame. */
SFA_FUNC(089E3E78, save_status_fetch) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089E3E88u; hle_sceUtilitySavedataGetStatus(c);
    ARG(0) = 0x08AD0000u;
    *W(0x08AD7CD0u) = c->r[2];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* 0. */
SFA_FUNC(089E4034, zero_e4034) {
    c->r[2] = 0;
}

/* 0. */
SFA_FUNC(089E403C, zero_e403c) {
    c->r[2] = 0;
}

/* (v): the byte at 0x08E65C65. */
SFA_FUNC(089E40B4, byte_8e65c65_set) {
    c->r[5] = 0x08E60000u;
    *B(0x08E65C65u) = (uint8_t)ARG(0);
}

/* The word at 0x08AD7EA0 is cleared. */
SFA_FUNC(089E43FC, word_8ad7ea0_clear) {
    ARG(0) = 0x08AD0000u;
    *W(0x08AD7EA0u) = 0;
}

/* The display's line count is kept at 0x08E65C50. A 0x10 frame. */
SFA_FUNC(089E48F8, hcount_fetch) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089E4908u; hle_sceDisplayGetCurrentHcount(c);
    ARG(0) = 0x08E60000u;
    *W(0x08E65C50u) = c->r[2];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* Waits for the next frame. A 0x10 frame. */
SFA_FUNC(089E4924, vblank_wait) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089E4934u; hle_sceDisplayWaitVblank(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The end of a routine that held interrupts off: they come back on, and s1 is returned from a
   0x10 frame that kept s0, s1. */
SFA_FUNC(089E7064, tail_intr_resume) {
    c->r[31] = 0x089E706Cu; hle_sceKernelCpuResumeIntr(c);
    gaddr sp = c->r[29];
    c->r[5] = c->r[17];
    c->r[31] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[5];
    c->r[29] += 0x10;
}

/* (v): the low 24 bits of v go to +72 of the record the word at 0x08E69348 points at. */
SFA_FUNC(089E8290, record_72_set_24) {
    c->r[2] = 0x08E70000u;
    c->r[6] = *W(0x08E69348u);
    c->r[7] = 0x00FF0000u;
    c->r[5] = 0x00FFFFFFu;
    c->r[3] = ARG(0) & 0x00FFFFFFu;
    *W(c->r[6] + 72) = c->r[3];
}

/* The end of a routine that adds a word to a list: r6 | r2 goes to the place in r3 and the
   list's +8 (r8) moves past it. */
SFA_FUNC(089E9C2C, tail_list_word_or) {
    c->r[9] = c->r[6] | c->r[2];
    c->r[6] = c->r[3] + 4;
    *W(c->r[3]) = c->r[9];
    *W(c->r[8] + 8) = c->r[6];
}

/* The same with r3 at the place in r2 and the list in r4. */
SFA_FUNC(089EA538, tail_list_word) {
    *W(c->r[2]) = c->r[3];
    c->r[3] = c->r[2] + 4;
    *W(c->r[4] + 8) = c->r[3];
}

/* The word at 0x08E69C38 is turned over: 1 when it was 0, else 0. */
SFA_FUNC(089EAB00, word_8e69c38_flip) {
    ARG(0) = 0x08E70000u;
    ARG(1) = *W(0x08E69C38u);
    c->r[3] = ARG(1) < 1;
    *W(0x08E69C38u) = c->r[3];
}

/* (a, b, d): the words at 0x08E71C48, 0x08E71C54 and 0x08E71C58. */
SFA_FUNC(089EAB14, three_words_1c48) {
    *W(0x08E71C48u) = ARG(0);
    c->r[2] = 0x08E70000u;
    c->r[3] = 0x08E70000u;
    *W(0x08E71C54u) = ARG(1);
    *W(0x08E71C58u) = ARG(2);
}

/* (.., b, d): the words at 0x08E71C4C and 0x08E71C50. */
SFA_FUNC(089EAB30, two_words_1c4c) {
    c->r[3] = 0x08E70000u;
    c->r[2] = 0x08E70000u;
    *W(0x08E71C4Cu) = ARG(1);
    *W(0x08E71C50u) = ARG(2);
}

/* r7. */
SFA_FUNC(089ED51C, tail_return_r7) {
    c->r[2] = c->r[7];
}

/* (i, v): the i-th word of the table at 0x08E74084. */
SFA_FUNC(089EDC04, table_4084_set) {
    c->r[6] = ARG(0) << 2;
    c->r[7] = 0x08E74084u;
    c->r[3] = c->r[6] + c->r[7];
    *W(c->r[3]) = ARG(1);
}

/* The answer cut to sixteen bits. */
SFA_FUNC(089EECF8, tail_answer_16) {
    c->r[2] &= 0xFFFF;
}

/* The end of a routine that stores its answer: r2 to the place in r5. */
SFA_FUNC(089FBC3C, tail_store_at_r5) {
    *W(c->r[5]) = c->r[2];
}

/* The same to the place in r10. */
SFA_FUNC(089FBFA0, tail_store_at_r10) {
    *W(c->r[10]) = c->r[2];
}
