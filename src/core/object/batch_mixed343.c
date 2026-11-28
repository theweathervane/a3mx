/* Nine that the library's third helping set free: the strips drawn every frame, the save
   screen's small ends, the menus' frame by mode, and the link's two ways back into a fight.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE 0x08D5C7C0u /* the save dialog's parameters */

void f_089E4F94(Cpu *c); void f_088E3550(Cpu *c); void f_088D7E80(Cpu *c); void f_088E3FE0(Cpu *c);
void f_089E352C(Cpu *c); void f_089E3EA8(Cpu *c); void f_08816AE4(Cpu *c); void f_08816FBC(Cpu *c);
void f_08816EDC(Cpu *c); void f_088480EC(Cpu *c); void f_088348F8(Cpu *c); void f_0883584C(Cpu *c);
void f_0882E0D0(Cpu *c); void f_08822934(Cpu *c); void f_08823304(Cpu *c); void f_08821F9C(Cpu *c);
void f_088E9458(Cpu *c); void f_088E95DC(Cpu *c); void f_089E3158(Cpu *c); void f_089E36D0(Cpu *c);
void hle_sceUtilitySavedataInitStart(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* The ten strips of 0x18 bytes from 0x092DF840 that are in use and shown (+0, +1) are drawn
   (0x089E4F94 with +12, +6, +10, +16 and +20); with neither 0x088E3550 nor 0x088D7E80 holding it,
   a strip is then given up and 0x088E3FE0 runs. A 0x30 frame keeping s0, s1. */
SFA_FUNC(0880F2FC, strips_draw) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s1; *W(sp + 32) = s0; *W(sp + 40) = ra;
    gaddr e = 0x092DF840u;
    for (uint32_t left = 10; left != 0; left--, e += 0x18) {
        c->r[16] = left; c->r[17] = e;
        if (*B(e) == 0 || *B(e + 1) == 0) continue;
        ARG(0) = *B(e + 12);
        ARG(1) = (uint32_t)(int32_t)S16(e + 6);
        ARG(2) = (uint32_t)(int32_t)S16(e + 10);
        ARG(3) = *W(e + 16);
        c->r[8] = *B(e + 20);
        call0(c, f_089E4F94, 0x0880F348u);
        call0(c, f_088E3550, 0x0880F350u);
        if (c->r[2] != 0) continue;
        call0(c, f_088D7E80, 0x0880F360u);
        if (c->r[2] != 0) continue;
        *B(e) = 0;
        call0(c, f_088E3FE0, 0x0880F370u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* The save dialog's list mode behind a 0x10 frame. */
SFA_FUNC(08816B74, save_list_start_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    call0(c, f_089E352C, 0x08816B84u);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The byte at 0x08A077CC is cleared once the word at 0x08AD7CD4 is 0. A 0x10 frame keeping s0. */
SFA_FUNC(08816E84, busy_byte_clear) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0;
    c->r[16] = 0x08A00000u;
    *W(c->r[29] + 4) = ra;
    if (*B(0x08A077CCu) != 0) {
        call0(c, f_089E3EA8, 0x08816EA8u);
        if (c->r[2] == 0) *B(0x08A077CCu) = 0;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}

/* The memory sticks are counted, and 0x08816FBC (0, slot) runs for each slot that has one. A
   frame keeping s0, s1. */
SFA_FUNC(08816F5C, sticks_each) {
    const gaddr slots = 0x09523C70u;
    uint32_t sv[3];
    enter2(c, sv);
    c->r[17] = slots;
    call0(c, f_08816AE4, 0x08816F7Cu);
    for (uint32_t i = 0; i < 8; i++) {
        c->r[16] = i;
        if (S8(slots + i) != 0) call2(c, 0, i, f_08816FBC, 0x08816F98u);
    }
    leave2(c, sv);
}

/* (p): p's +4 goes to 0x4F and +2 to 9, and the load its +10 names starts. */
SFA_FUNC(08817D30, load_by_10) {
    gaddr p = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(p + 4) = 0x4F;
    *B(p + 2) = 9;
    c->r[6] = (uint32_t)(int32_t)S8(p + 3);
    *W(c->r[29] + 16) = ra;
    call2(c, (uint32_t)(int32_t)S8(p + 10), c->r[6], f_08816EDC, 0x08817D5Cu);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): while x's +89 is set, the menu's frame: 0x0883584C with x's +138 set, the menu at
   0x0950FF60 in the mode 2, and 0x088348F8 otherwise. */
SFA_FUNC(08828B88, menu_frame_by_mode) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(x + 89) != 0) {
        c->r[6] = y;
        if (S8(x + 138) != 0) call2(c, x, y, f_0883584C, 0x08828BF8u);
        else if (S8(x + 114) == 2) call2(c, y, x, f_088480EC, 0x08828BC8u);
        else call2(c, x, y, f_088348F8, 0x08828BE0u);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): the link's fight from the start: 0x0882E0D0 outside the mode 6, 0x08822934,
   0x08823304, 0x08821F9C, the fighters and the fight set up again, the two blocks of 0x354 bytes
   at 0x09358528 and 0x0935887C cleared, and x's +2 at 1. A frame keeping s0..s2. */
SFA_FUNC(088E967C, link_fight_from_start) {
    gaddr x = ARG(0), y = ARG(1);
    const gaddr base = 0x09358410u;
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2;
    int32_t mode = S8(x + 114);
    *W(sp + 20) = s1;
    c->r[17] = x; c->r[18] = base;
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = y;
    if (mode != 6) call0(c, f_0882E0D0, 0x088E96B4u);
    call0(c, f_08822934, 0x088E96BCu);
    call0(c, f_08823304, 0x088E96C4u);
    call0(c, f_08821F9C, 0x088E96CCu);
    call2(c, x, y, f_088E9458, 0x088E96D8u);
    call2(c, x, y, f_088E95DC, 0x088E96E4u);
    for (uint32_t i = 0; i < 0x354; i++) *B(base + 0x118 + i) = 0;
    for (uint32_t i = 0; i < 0x354; i++) *B(base + 0x46C + i) = 0;
    *H(x + 2) = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (x, y): the link's next fight: 0x0882E0D0 outside the mode 6, 0x08821F9C, the fighters and the
   fight set up again, and x's +2 at 2. A frame keeping s0, s1. */
SFA_FUNC(088E973C, link_fight_next) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    int32_t mode = S8(x + 114);
    enter2(c, sv);
    c->r[17] = x; c->r[16] = y;
    if (mode != 6) call0(c, f_0882E0D0, 0x088E9768u);
    call0(c, f_08821F9C, 0x088E9770u);
    call2(c, x, y, f_088E9458, 0x088E977Cu);
    call2(c, x, y, f_088E95DC, 0x088E9788u);
    *H(x + 2) = 2;
    leave2(c, sv);
}

/* (.., a, d, n): a load into d: 0x089E3158's answer for a, the parameters with n blocks of 512
   bytes (no limit for 0), and the dialog. Returns -1 when the dialog refuses; else the word at
   0x08AD3D38 goes to 0x100 and 0 comes back. A frame keeping s0..s2. */
SFA_FUNC(089E3B6C, save_load_into) {
    uint32_t a = ARG(1), d = ARG(2), n = ARG(3);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2; *W(sp + 20) = s1;
    c->r[17] = d; c->r[18] = SAVE;
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = n;
    ARG(0) = a;
    if (n != 0) {
        call0(c, f_089E3158, 0x089E3BA0u);
        ARG(3) = n << 9;
        ARG(0) = SAVE; ARG(1) = c->r[2]; ARG(2) = d;
        call0(c, f_089E36D0, 0x089E3BB4u);
    } else {
        call0(c, f_089E3158, 0x089E3BC4u);
        ARG(0) = SAVE; ARG(1) = c->r[2]; ARG(2) = d; ARG(3) = 0xFFFFFFFFu;
        call0(c, f_089E36D0, 0x089E3BD8u);
    }
    ARG(0) = SAVE;
    call0(c, hle_sceUtilitySavedataInitStart, 0x089E3BE0u);
    if (c->r[2] & 0x80000000u) c->r[2] = 0xFFFFFFFFu;
    else {
        *W(0x08AD0000u + 15672) = 0x100;
        c->r[2] = 0;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
