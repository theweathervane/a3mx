/* The game's own doors to the library's small pieces: the sound thread's flags and tempo, the lists
   of pictures a screen loads, a step by the table, and whether the link's fight is at its end.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u
#define NETP 0x09523980u

void f_089E6550(Cpu *c); void f_089E6500(Cpu *c); void f_089E6528(Cpu *c); void f_089E6484(Cpu *c);
void f_089E0A74(Cpu *c); void f_089E3A40(Cpu *c);

static void list2(Cpu *c, uint32_t list, uint32_t first, uint32_t back) {
    ARG(0) = list; ARG(1) = first; c->r[31] = back; f_089E0A74(c);
}

/* (object): its +1 and +2 are cleared. */
SFA_FUNC(088E9380, clear_bytes_1_2) {
    gaddr o = ARG(0);
    *B(o + 2) = 0;
    *B(o + 1) = 0;
}

/* (n): the tempo's other way (0x089E6550) behind a frame. */
SFA_FUNC(08809BC8, sound_tempo_other_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = ARG(0) & 0xFFFF;
    c->r[31] = 0x08809BD8u; f_089E6550(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The sound thread's bit 0x10 set, behind a 0x10 frame. */
SFA_FUNC(08809FE8, sound_flag_10_set_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x08809FF8u; f_089E6500(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The sound thread's bit 0x10 cleared, behind a 0x10 frame. */
SFA_FUNC(0880A004, sound_flag_10_clear_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x0880A014u; f_089E6528(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The sound thread's bit 8, behind a 0x10 frame. Returns 0. */
SFA_FUNC(08809FC8, sound_flag_8_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x08809FD8u; f_089E6484(c);
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The lists 0x08A05524 (after the entries are given back), 0x08A05040 and 0x08A05050. */
SFA_FUNC(088157A0, lists_5524_5040_5050) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    list2(c, 0x08A05524u, 1, 0x088157B8u);
    list2(c, 0x08A05040u, 0, 0x088157C8u);
    list2(c, 0x08A05050u, 0, 0x088157D8u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The lists 0x09358408 (after the entries are given back) and 0x08A054C0. */
SFA_FUNC(088157E4, lists_8408_54c0) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    list2(c, 0x09358408u, 1, 0x088157FCu);
    list2(c, 0x08A054C0u, 0, 0x0881580Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The lists 0x08A053CC (after the entries are given back) and 0x08A051B4. */
SFA_FUNC(08815818, lists_53cc_51b4) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    list2(c, 0x08A053CCu, 1, 0x08815830u);
    list2(c, 0x08A051B4u, 0, 0x08815840u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The lists 0x08A053CC (after the entries are given back) and 0x08A05070. */
SFA_FUNC(08815870, lists_53cc_5070) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    list2(c, 0x08A053CCu, 1, 0x08815888u);
    list2(c, 0x08A05070u, 0, 0x08815898u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): its step from the table 0x08A3D2C8, by its +128. */
SFA_FUNC(088D1564, step_by_128) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    uint32_t fn = *W(0x08A3D2C8u + ((uint32_t)*B(o + 128) << 2));
    c->r[5] = fn; c->r[6] = 0x08A3D2C8u;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088D158Cu; rt_call_indirect(c, fn);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* 1 when the link is in use (the mode 0xC, or NETP + 34 set) and its +42 stands at 2 or 3. */
SFA_FUNC(088D7DB8, link_at_2_or_3) {
    int32_t mode = S8(GAME + 114);
    c->r[2] = 0;
    if (mode == 0xC || *B(NETP + 34) != 0) {
        uint32_t at = *B(NETP + 42);
        if (at == 3 || at == 2) c->r[2] = 1;
    }
}

/* The callback 0x08816840 runs by hand (0x089E3A40 with 0xF, 0xFF and 0x08AE03CC), then the words
   and bytes from 0x09523C70 to 0x09523C83 and the byte at 0x0951F94B are cleared. */
SFA_FUNC(088167CC, callback_6840_and_clear) {
    const gaddr at = 0x09523C60u;
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = at;
    ARG(0) = 0xF; ARG(1) = 0xFF; ARG(2) = 0x08AE03CCu; ARG(3) = 0x08816840u;
    c->r[31] = 0x088167FCu; f_089E3A40(c);
    *W(at + 32) = 0; *W(at + 28) = 0; *W(at + 24) = 0;
    for (int i = 23; i >= 16; i--) *B(at + i) = 0;
    *B(0x0951F94Bu) = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
