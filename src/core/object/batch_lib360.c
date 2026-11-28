/* More of the library's small pieces: commands added to the drawing list, the two callback tables
   cleared, an event flag asked without waiting, and a few more ends of longer routines. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE 0x08D5C7C0u /* the save dialog's parameters */
#define LIST 0x08E69348u /* points at the drawing list being written; its +8 is the next place */

void f_089E3400(Cpu *c); void f_089F3470(Cpu *c); void f_089E5784(Cpu *c); void f_089E04EC(Cpu *c);
void hle_sceKernelPollEventFlag(Cpu *c); void hle_sceAtracReleaseAtracID(Cpu *c);
void hle_sceGeEdramGetAddr(Cpu *c);

/* (a, b): one command of the drawing list: op with b above a. */
static void list_cmd_ab(Cpu *c, uint32_t op) {
    c->r[10] = 0x08E70000u;
    c->r[7] = *W(LIST);
    c->r[9] = ARG(1) << 8;
    c->r[8] = c->r[9] | ARG(0);
    c->r[6] = *W(c->r[7] + 8);
    ARG(0) = op;
    c->r[3] = c->r[8] | op;
    c->r[2] = c->r[6] + 4;
    *W(c->r[6]) = c->r[3];
    *W(c->r[7] + 8) = c->r[2];
}

/* While the save dialog's mode (+48) is 0, 0x089E3400's answer for it; else 0xFFFF0000. */
SFA_FUNC(089E3838, save_answer_mode_0) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(0) = SAVE;
    ARG(1) = *W(SAVE + 48);
    *W(c->r[29] + 16) = ra;
    c->r[2] = 0xFFFF0000u;
    if (ARG(1) == 0) { c->r[31] = 0x089E385Cu; f_089E3400(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (.., v): the word at 0x08AD7CD4 is set for 1 and cleared for 2. Returns 0. */
SFA_FUNC(089E3EB4, word_7cd4_by_event) {
    uint32_t v = ARG(1);
    if (v == 1) {
        ARG(0) = 1; ARG(1) = 0x08AD0000u;
        *W(0x08AD7CD4u) = 1;
    } else {
        ARG(0) = 2;
        if (v == 2) *W(0x08AD7CD4u) = 0;
        ARG(0) = 0x08AD0000u;
    }
    c->r[2] = 0;
}

/* The first callback table (0x380 bytes at 0x09525E10) is cleared. */
SFA_FUNC(089E575C, table_7_clear) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x09525E10u; ARG(1) = 0; ARG(2) = 0x380;
    c->r[31] = 0x089E5778u; f_089F3470(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The second callback table (0x280 bytes at 0x09526190) is cleared. */
SFA_FUNC(089E5968, table_5_clear) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0x09526190u; ARG(1) = 0; ARG(2) = 0x280;
    c->r[31] = 0x089E5984u; f_089F3470(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (six words): an entry of kind 1 in the first callback table (1 and the six). */
SFA_FUNC(089E57D0, table_enter_7_kind1) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    c->r[10] = c->r[9]; c->r[9] = c->r[8]; c->r[8] = ARG(3);
    ARG(3) = ARG(2); ARG(2) = ARG(1); ARG(1) = ARG(0);
    *W(c->r[29] + 32) = ra;
    ARG(0) = 1; c->r[31] = 0x089E57F8u; f_089E5784(c);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (flag, bits): 1 when any of the bits is set in the event flag, asked without waiting. */
SFA_FUNC(089E5B68, event_flag_poll) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0;
    c->r[16] = 0;
    ARG(3) = sp + 0x10;
    *W(sp + 24) = ra;
    ARG(2) = 1;
    c->r[31] = 0x089E5B84u; hle_sceKernelPollEventFlag(c);
    c->r[2] = c->r[2] == 0 ? 1 : 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (place): the ATRAC3 handle kept at the place, when there is one, is given back. */
SFA_FUNC(089E5BC4, atrac_release) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(0) = *W(ARG(0));
    *W(c->r[29] + 16) = ra;
    if ((int32_t)ARG(0) >= 0) { c->r[31] = 0x089E5BE0u; hle_sceAtracReleaseAtracID(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The buffer of 0xC8002 bytes kept at 0x08AD7F24 is taken (0x089E04EC) the first time. */
SFA_FUNC(089E5ED0, buffer_c8002_once) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    ARG(1) = *W(0x08AD7F24u);
    *W(c->r[29] + 16) = ra;
    if (ARG(1) == 0) {
        ARG(0) = 0xC8002;
        c->r[31] = 0x089E5EF0u; f_089E04EC(c);
        ARG(0) = 0x08AD0000u;
        *W(0x08AD7F24u) = c->r[2];
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): the drawing list's command 0x36. */
SFA_FUNC(089E7774, list_cmd_36) {
    list_cmd_ab(c, 0x36000000u);
}

/* (a): the drawing list's command 0x53 with the low three bits of a. */
SFA_FUNC(089E77A0, list_cmd_53) {
    c->r[9] = 0x08E70000u;
    c->r[6] = *W(LIST);
    c->r[7] = ARG(0) & 7;
    c->r[8] = 0x53000000u;
    c->r[5] = *W(c->r[6] + 8);
    c->r[2] = c->r[7] | c->r[8];
    c->r[3] = c->r[5] + 4;
    *W(c->r[5]) = c->r[2];
    *W(c->r[6] + 8) = c->r[3];
}

/* (a, b): the drawing list's command 0xC6. */
SFA_FUNC(089E877C, list_cmd_c6) {
    list_cmd_ab(c, 0xC6000000u);
}

/* (a, b): the drawing list's command 0xC7. */
SFA_FUNC(089E8818, list_cmd_c7) {
    list_cmd_ab(c, 0xC7000000u);
}

/* The end of a routine that adds three commands past the place in r5: 0xD5 with r4, 0x15, and
   0x16 with r4; the list's +8 (r8) moves past them. */
SFA_FUNC(089E9C8C, tail_list_d5_15_16) {
    c->r[12] = c->r[5] + 4;
    c->r[25] = 0x16000000u;
    c->r[24] = 0xD5000000u;
    c->r[15] = c->r[12] + 0xC;
    c->r[13] = c->r[4] | c->r[25];
    c->r[14] = c->r[4] | c->r[24];
    *W(c->r[8] + 8) = c->r[15];
    c->r[8] = 0x15000000u;
    *W(c->r[12]) = c->r[14];
    *W(c->r[12] + 4) = c->r[8];
    *W(c->r[12] + 8) = c->r[13];
}

/* The video memory's address is kept at 0x08E71C44 and the word at 0x08E69C38 cleared. A 0x10
   frame. */
SFA_FUNC(089EAAD4, edram_address_fetch) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089EAAE4u; hle_sceGeEdramGetAddr(c);
    ARG(0) = 0x08E70000u;
    c->r[3] = 0x08E70000u;
    *W(0x08E71C44u) = c->r[2];
    *W(0x08E69C38u) = 0;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* Returns r6 from a 0x40 frame that kept s0..s7 and fp at +16. */
SFA_FUNC(089ED620, tail_ed620) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 52);
    c->r[30] = *W(sp + 48);
    for (int i = 7; i >= 0; i--) c->r[16 + i] = *W(sp + 16 + 4 * (uint32_t)i);
    c->r[2] = c->r[6];
    c->r[29] += 0x40;
}

/* Returns r4 from a 0x30 frame that kept s0..s4 at +16. */
SFA_FUNC(089ED778, tail_ed778) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 36);
    for (int i = 4; i >= 0; i--) c->r[16 + i] = *W(sp + 16 + 4 * (uint32_t)i);
    c->r[2] = c->r[4];
    c->r[29] += 0x30;
}

/* Returns r6 from a 0x20 frame that kept s0..s5. */
SFA_FUNC(089ED860, tail_ed860) {
    gaddr sp = c->r[29];
    c->r[31] = *W(sp + 24);
    for (int i = 5; i >= 0; i--) c->r[16 + i] = *W(sp + 4 * (uint32_t)i);
    c->r[2] = c->r[6];
    c->r[29] += 0x20;
}

/* (i, a, b, d, e): the i-th entry of four words at 0x08E74104. */
SFA_FUNC(089EDB58, table_4104_set) {
    c->r[9] = ARG(0) << 4;
    c->r[10] = 0x08E74104u;
    c->r[3] = c->r[9] + c->r[10];
    *W(c->r[3] + 12) = c->r[8];
    *W(c->r[3]) = ARG(1);
    *W(c->r[3] + 4) = ARG(2);
    *W(c->r[3] + 8) = ARG(3);
}

/* Returns 0 from a 0x30 frame that kept s0..s7 and fp. */
SFA_FUNC(089EF3F4, tail_ef3f4) {
    gaddr sp = c->r[29];
    c->r[30] = *W(sp + 32);
    for (int i = 7; i >= 0; i--) c->r[16 + i] = *W(sp + 4 * (uint32_t)i);
    c->r[2] = 0;
    c->r[29] += 0x30;
}

/* (i, v): the i-th of the records listed at 0x08E74308 gets v at +9 and its first byte cleared.
   Returns 0. */
SFA_FUNC(089EFED8, record_9_set) {
    c->r[9] = 0x08E70000u;
    c->r[6] = ARG(0) << 2;
    c->r[8] = 0x08E74308u;
    c->r[3] = c->r[6] + c->r[8];
    c->r[7] = *W(c->r[3]);
    c->r[2] = 0;
    *B(c->r[7] + 9) = (uint8_t)ARG(1);
    ARG(1) = *W(c->r[3]);
    *B(ARG(1)) = 0;
}

/* Returns 0 from a 0x20 frame that kept s0..s6. */
SFA_FUNC(089F0930, tail_f0930) {
    gaddr sp = c->r[29];
    for (int i = 6; i >= 0; i--) c->r[16 + i] = *W(sp + 4 * (uint32_t)i);
    c->r[2] = 0;
    c->r[29] += 0x20;
}
