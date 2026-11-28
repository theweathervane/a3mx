/* Small pieces of the game's library that many callers wait for: the event flag's two wrappers, the
   sound thread's tempo, and two tables that callbacks are entered in. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND 0x08E69200u /* the sound driver's state */

void f_089E5B30(Cpu *c); void f_089E5B4C(Cpu *c); void f_089E65BC(Cpu *c); void f_089E5784(Cpu *c);
void f_089E5990(Cpu *c);
void hle_sceKernelSetEventFlag(Cpu *c); void hle_sceKernelClearEventFlag(Cpu *c);

/* (flag, bits): sceKernelSetEventFlag behind a frame. */
SFA_FUNC(089E5B30, event_flag_set) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x089E5B40u; hle_sceKernelSetEventFlag(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (flag, bits): the bits are cleared: sceKernelClearEventFlag with the others kept. */
SFA_FUNC(089E5B4C, event_flag_clear) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(1) = ~ARG(1);
    c->r[31] = 0x089E5B5Cu; hle_sceKernelClearEventFlag(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): the sound thread's flag (SOUND + 148) gets its bit 0x40 set and 0x20 cleared, and the
   tempo is kept: 0x8000 at +156 and 0x8000 divided by a quarter of n at +160. A frame keeping s0,
   s1. */
SFA_FUNC(089E65BC, sound_tempo) {
    uint32_t n = ARG(0) & 0xFFFF;
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = n;
    *W(sp + 20) = s1; c->r[17] = SOUND;
    uint32_t flag = *W(SOUND + 148);
    *W(sp + 24) = ra;
    ARG(0) = flag; ARG(1) = 0x40; c->r[31] = 0x089E65E4u; f_089E5B30(c);
    ARG(0) = *W(SOUND + 148); ARG(1) = 0x20; c->r[31] = 0x089E65F0u; f_089E5B4C(c);
    uint32_t quarter = (uint32_t)((int32_t)(n + ((uint32_t)((int32_t)n >> 2) >> 30)) >> 2);
    DIV(c, 0x8000, quarter);
    *W(SOUND + 156) = 0x8000;
    *W(SOUND + 160) = c->lo;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (n): the same from the game's side, behind a frame. */
SFA_FUNC(08809BE4, sound_tempo_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = ARG(0) & 0xFFFF;
    c->r[31] = 0x08809BF4u; f_089E65BC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (seven words): the first free entry (its first word 0) of the 32 at 0x09525E10, 0x1C bytes
   each, takes the seven words. Returns its number, or -1 when all are taken. */
SFA_FUNC(089E5784, table_enter_7) {
    gaddr at = 0x09525E10u;
    for (uint32_t i = 0; i < 32; i++, at += 0x1C) {
        c->r[11] = at;
        if (*W(at) != 0) { c->r[3] = (i + 1) < 32; continue; }
        for (uint32_t k = 0; k < 7; k++) *W(at + 4 * k) = c->r[4 + k];
        c->r[3] = 0;
        c->r[2] = i;
        return;
    }
    c->r[11] = at;
    c->r[2] = 0xFFFFFFFFu;
}

/* (five words): the same for the 32 entries of 0x14 bytes at 0x09526190. */
SFA_FUNC(089E5990, table_enter_5) {
    gaddr at = 0x09526190u;
    for (uint32_t i = 0; i < 32; i++, at += 0x14) {
        c->r[9] = at;
        if (*W(at) != 0) { c->r[10] = (i + 1) < 32; continue; }
        for (uint32_t k = 0; k < 5; k++) *W(at + 4 * k) = c->r[4 + k];
        c->r[10] = 0;
        c->r[2] = i;
        return;
    }
    c->r[9] = at;
    c->r[2] = 0xFFFFFFFFu;
}

/* (a): an entry of kind 2 with a in the first table (2, a, and five zeros). */
SFA_FUNC(089E5804, table_enter_7_kind2) {
    uint32_t a = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = ra;
    ARG(0) = 2; ARG(1) = a; ARG(2) = 0; ARG(3) = 0; c->r[8] = 0; c->r[9] = 0; c->r[10] = 0;
    c->r[31] = 0x089E582Cu; f_089E5784(c);
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (a): an entry of kind 2 with a in the second table (2, 0, a, 0, 0). */
SFA_FUNC(089E5A00, table_enter_5_kind2) {
    uint32_t a = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = ra;
    ARG(0) = 2; ARG(1) = 0; ARG(2) = a; ARG(3) = 0; c->r[8] = 0;
    c->r[31] = 0x089E5A20u; f_089E5990(c);
    c->r[31] = ra; c->r[29] += 0x30;
}
