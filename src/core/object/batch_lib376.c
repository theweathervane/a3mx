/* Seven more of the library: a drawing state enabled and disabled, the viewport and one more
   command for the list in use, a sequence stopped from outside, the sound core's start, and the
   end of a routine that begins a drawing list. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define LIST 0x08E69348u /* points at the drawing list being written */
#define GE   0x08E69300u /* the drawing state; its +68 has a bit for each state that is on */

void f_089E9BFC(Cpu *c); void f_089E97F0(Cpu *c); void f_089E99E0(Cpu *c); void f_089EF6E0(Cpu *c);
void f_089E6E40(Cpu *c); void f_089ECA14(Cpu *c);
void hle___sceSasInit(Cpu *c);

/* The switch of one drawing state on the list in use; the frame both callers share. */
static void state_switch(Cpu *c, uint32_t on, uint32_t back, uint32_t sv[3]) {
    uint32_t state = ARG(0);
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = sv[0];
    c->r[16] = GE;
    *W(sp + 4) = sv[1];
    c->r[17] = state;
    *W(sp + 8) = sv[2];
    ARG(0) = *W(GE + 72); ARG(1) = state; ARG(2) = on;
    c->r[31] = back; f_089E9BFC(c);
}

/* (state): the drawing state is switched on, and its bit set. A 0x10 frame keeping s0, s1. */
SFA_FUNC(089E7E34, draw_state_enable) {
    uint32_t sv[3];
    state_switch(c, 1, 0x089E7E64u, sv);
    ARG(1) = *W(GE + 68);
    c->r[3] = 1u << (c->r[17] & 31);
    ARG(0) = ARG(1) | c->r[3];
    *W(GE + 68) = ARG(0);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x10;
}

/* (state): the drawing state is switched off, and its bit cleared. A 0x10 frame keeping s0,
   s1. */
SFA_FUNC(089E7E8C, draw_state_disable) {
    uint32_t sv[3];
    state_switch(c, 0, 0x089E7EBCu, sv);
    ARG(2) = 1;
    ARG(1) = *W(GE + 68);
    c->r[3] = ~(1u << (c->r[17] & 31));
    ARG(0) = ARG(1) & c->r[3];
    *W(GE + 68) = ARG(0);
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x10;
}

/* (cx, cy, w, h): the viewport on the list in use. A 0x10 frame. */
SFA_FUNC(089E8384, list_viewport_here) {
    uint32_t a = ARG(0), b = ARG(1), d = ARG(2), e = ARG(3);
    uint32_t ra = c->r[31];
    c->r[9] = *W(LIST);
    c->r[2] = a; c->r[10] = b; c->r[11] = d;
    c->r[29] -= 0x10;
    c->r[8] = e;
    ARG(0) = c->r[9]; ARG(1) = a; ARG(2) = b;
    *W(c->r[29]) = ra;
    ARG(3) = d;
    c->r[31] = 0x089E83B8u; f_089E97F0(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (a; f12): the command 0xC8 on the list in use. A 0x10 frame. */
SFA_FUNC(089E8844, list_cmd_c8_here) {
    uint32_t ra = c->r[31];
    c->r[3] = 0x08E70000u;
    ARG(2) = *W(LIST);
    c->r[29] -= 0x10;
    ARG(1) = ARG(0);
    *W(c->r[29]) = ra;
    ARG(0) = ARG(2);
    c->r[31] = 0x089E8860u; f_089E99E0(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (i): a sequence stopped, or 0x80450001 before the sound is started. A 0x10 frame. */
SFA_FUNC(089EE300, sequence_stop_checked) {
    uint32_t ra = c->r[31];
    c->r[7] = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    ARG(2) = 0xFFFFFFFFu;
    *W(c->r[29]) = ra;
    ARG(1) = 0x80450001u;
    if (c->r[7] != 0xFFFFFFFFu) {
        c->r[31] = 0x089EE328u; f_089EF6E0(c);
        ARG(1) = c->r[2];
    }
    c->r[2] = ARG(1);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The end of a routine that begins a drawing list (the drawing state in s0): unless the list is
   a called one (+12) or no buffer is chosen (+40), the list takes the state's format and size and
   begins with the buffer's three commands (0xD2, 0x9C, 0x9D). The frame's end returns 0. */
SFA_FUNC(089E6DAC, tail_list_begin) {
    gaddr st = c->r[16];
    ARG(0) = *W(st + 12);
    c->r[3] = 0;
    if (ARG(0) == 0) {
        ARG(3) = *W(st + 40);
        if (ARG(3) != 0) {
            gaddr list = *W(st + 72);
            c->r[11] = list;
            *W(list + 24) = *W(st + 36);
            *W(list + 28) = *W(st + 60);
            *W(list + 32) = *W(st + 64);
            uint32_t addr = *W(st + 44), width = *W(st + 40);
            gaddr p = *W(list + 8);
            *W(p) = *W(st + 36) | 0xD2000000u;
            *W(list + 8) = p + 12;
            *W(p + 4) = (addr & 0x00FFFFFFu) | 0x9C000000u;
            *W(p + 8) = (((addr >> 24) & 0xF) << 16) | width | 0x9D000000u;
            c->r[3] = 0;
        }
    }
    f_089E6E40(c);
}

/* (a): the sound core is started, once (0x80420101 the second time): 32 voices at 44100, then
   each voice made neutral by the loop's own routine, which sets the started flag and returns 0.
   The core's own error is returned as it is. A 0x20 frame keeping s0..s3. */
SFA_FUNC(089EC9A0, sound_core_start) {
    uint32_t a = ARG(0);
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 4) = c->r[17];
    c->r[17] = 0x08E70000u;
    ARG(2) = *W(0x08E73000u);
    *W(sp + 16) = c->r[31];
    ARG(1) = a;
    c->r[3] = 0x80420101u;
    *W(sp + 12) = c->r[19]; *W(sp + 8) = c->r[18]; *W(sp) = c->r[16];
    if (ARG(2) == 0) {
        c->r[18] = 0x08E70000u;
        ARG(0) = 0x08E73040u; ARG(2) = 0x20; ARG(3) = 0; c->r[8] = 0xAC44;
        c->r[31] = 0x089EC9E8u; hle___sceSasInit(c);
        c->r[3] = c->r[2];
        if (c->r[2] == 0) {
            c->r[16] = 0;
            c->r[19] = c->r[17];
            ARG(0) = 1;
            f_089ECA14(c);
            return;
        }
    }
    c->r[31] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[3];
    c->r[29] += 0x20;
}
