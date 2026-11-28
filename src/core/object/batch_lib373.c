/* Four more of the library: a drawing state switched on or off, a sequence stopped, a voice taken
   or given back from outside, and a record's sequence loaded. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E9C2C(Cpu *c); void f_089E9C8C(Cpu *c); void f_089F1FB8(Cpu *c); void f_089EDAC0(Cpu *c);
void f_089ED808(Cpu *c); void f_089ED8F4(Cpu *c);

/* (list, state, on): one of the 22 drawing states is switched on or off on the list given. Most
   are a single command with the switch in it (the table below); the scissor (2) puts its
   rectangle, or the whole screen, in the two scissor commands and keeps the switch at the list's
   +36; the last (21) keeps the switch at +96 and sends the command 0xC9 with it and the list's
   +100 and +104. */
SFA_FUNC(089E9BFC, list_state_switch) {
    static const uint8_t cmd[21] = { 0x22, 0x23, 0, 0x24, 0x21, 0x1D, 0x20, 0x1F, 0x1C, 0x1E, 0x17,
                                     0x18, 0x19, 0x1A, 0x1B, 0x25, 0x26, 0x27, 0x28, 0x51, 0x38 };
    gaddr list = ARG(0);
    uint32_t state = ARG(1), on = ARG(2);
    c->r[3] = state < 0x16;
    c->r[8] = list;
    if (!(state < 0x16)) return;
    ARG(3) = state << 2;
    ARG(1) = 0x08A00000u;
    ARG(0) = (state << 2) + 0x08A024E8u;
    if (state == 2) {
        if (on != 0) {
            ARG(3) = *W(list + 44);
            ARG(0) = *W(list + 40);
            c->r[14] = *W(list + 52);
            c->r[12] = *W(list + 48);
            c->r[25] = ARG(3) << 10;
            ARG(1) = *W(list + 8);
            c->r[15] = c->r[25] | ARG(0);
            c->r[24] = 0xD4000000u;
            c->r[11] = c->r[14] << 10;
            c->r[10] = c->r[15] | 0xD4000000u;
            c->r[13] = 1;
            ARG(0) = c->r[11] | c->r[12];
            *W(list + 36) = 1;
            *W(ARG(1)) = c->r[10];
        } else {
            c->r[2] = *W(list + 32);
            ARG(1) = *W(list + 8);
            c->r[11] = *W(list + 28);
            c->r[3] = 0xD4000000u;
            c->r[10] = c->r[2] - 1;
            *W(list + 36) = 0;
            c->r[9] = c->r[11] - 1;
            ARG(2) = c->r[10] << 10;
            *W(ARG(1)) = 0xD4000000u;
            ARG(0) = ARG(2) | c->r[9];
        }
        f_089E9C8C(c);
    } else if (state == 21) {
        if (on != 0) { ARG(1) = 1; *W(list + 96) = 1; }
        else *W(list + 96) = 0;
        c->r[2] = *W(list + 104);
        c->r[15] = *W(list + 96);
        c->r[12] = *W(list + 100);
        ARG(2) = *W(list + 8);
        c->r[14] = c->r[2] << 8;
        c->r[13] = c->r[15] << 16;
        c->r[11] = c->r[13] | c->r[14];
        c->r[9] = c->r[11] | c->r[12];
        c->r[10] = 0xC9000000u;
        ARG(0) = c->r[9] | 0xC9000000u;
        ARG(3) = ARG(2) + 4;
        *W(ARG(2)) = ARG(0);
        *W(list + 8) = ARG(3);
    } else {
        c->r[3] = *W(list + 8);
        c->r[2] = (uint32_t)cmd[state] << 24;
        f_089E9C2C(c);
    }
}

/* (voice, on): a voice taken (1) or given back (0) from outside the sound's own code; 0x80450001
   before the sound is started and 0x8054000A for another value. A 0x10 frame. */
SFA_FUNC(089EDF0C, voice_take_or_free_checked) {
    uint32_t ra = c->r[31];
    c->r[9] = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    c->r[8] = 0xFFFFFFFFu;
    c->r[7] = 0x80450001u;
    *W(c->r[29]) = ra;
    c->r[3] = ARG(1) < 2;
    if (c->r[9] != 0xFFFFFFFFu) {
        c->r[7] = 0x8054000Au;
        if (c->r[3] != 0) {
            c->r[31] = 0x089EDF44u; f_089F1FB8(c);
            c->r[7] = c->r[2];
        }
    }
    c->r[2] = c->r[7];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (.., bank, n): the record (bank, n) of 0x08D52380 with data (+20) has its sequence loaded, with
   the state at its +28 and the bank's own number (+8 of the bank's first record), and keeps the
   answer at +24. Returns 0. */
SFA_FUNC(089E1DB4, record_sequence_load) {
    int32_t bank = (int8_t)ARG(1), n = (int8_t)ARG(2);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    int64_t m = (int64_t)bank * 0xA414;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr first = 0x08D52380u + c->lo;
    gaddr rec = first + (uint32_t)n * 0x148;
    *W(c->r[29] + 16) = s0;
    c->r[16] = rec;
    uint32_t data = *W(rec + 20);
    *W(c->r[29] + 20) = ra;
    c->r[8] = data;
    if (data != 0) {
        ARG(0) = rec + 0x1C; ARG(1) = data; ARG(2) = *W(first + 8);
        c->r[31] = 0x089E1E28u; f_089EDAC0(c);
        *W(rec + 24) = c->r[2];
    }
    c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (i): the i-th sequence, playing or paused (1 or 2), is stopped: each of its sixteen channels
   has its holds cleared and its voices let go (by the channel's word at +72), and its state goes
   back to the start of its data, stopped. Returns 0; 0x80540007 without the sequence and
   0x8054000D for one in another state. A 0x10 frame keeping s0..s2. */
SFA_FUNC(089EF6E0, sequence_stop) {
    uint32_t i = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 12) = ra; *W(sp + 8) = s2; *W(sp + 4) = s1; *W(sp) = s0;
    uint32_t answer = 0x80540007u;
    if (i < 0x81) {
        gaddr slot = 0x08E74308u + (i << 2);
        gaddr p = *W(slot);
        if (p != 0) {
            answer = 0x8054000Du;
            if ((((uint32_t)*B(p) - 1) & 0xFF) < 2) {
                c->r[18] = slot;
                for (uint32_t ch = 0; ch < 0x10; ch++) {
                    c->r[16] = ch << 4;
                    c->r[17] = ch + 1;
                    ARG(0) = ch; ARG(1) = 0; ARG(2) = *W(*W(slot) + (ch << 4) + 72);
                    c->r[31] = 0x089EF768u; f_089ED808(c);
                    ARG(0) = *W(*W(slot) + (ch << 4) + 72);
                    c->r[31] = 0x089EF778u; f_089ED8F4(c);
                }
                gaddr q = *W(slot);
                *B(q + 8) = 0xFF;
                gaddr q2 = *W(slot);
                uint32_t start = *W(q + 4);
                *B(q2 + 12) = 1;
                gaddr q3 = *W(slot);
                *W(q + 28) = start + 0x16;
                *B(q3) = 0;
                *W(q2 + 48) = 0;
                *W(q2 + 60) = 0;
                answer = 0;
            }
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x10;
}
