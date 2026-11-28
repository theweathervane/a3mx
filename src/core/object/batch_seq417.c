/* The sound engine: a note started on a bank's program, and the routines that only run once the
   engine is ready. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SEQ_READY 0x08AD80D0u /* -1 until the sequences are made ready */
#define BANKS     0x08E73E80u /* the banks' slots */
#define VOICE_N   0x08E74080u /* how many voices the engine was started with */

void f_089F2794(Cpu *c); void f_089F25F4(Cpu *c); void f_089F265C(Cpu *c); void f_089F26C4(Cpu *c);
void f_089F272C(Cpu *c); void f_089EF304(Cpu *c); void f_089EE8E4(Cpu *c); void f_089F2188(Cpu *c);
void f_089ED9B0(Cpu *c); void f_089F1C68(Cpu *c); void f_089F1628(Cpu *c); void f_089F1808(Cpu *c);
void f_089EEF64(Cpu *c); void f_089EEEC4(Cpu *c); void f_089ED620(Cpu *c);

/* The routine given, or 0x80450001 before the sequences are ready. A 0x10 frame. */
static void when_ready(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[3] = 0x08AE0000u;
    c->r[8] = *W(SEQ_READY);
    c->r[29] -= 0x10;
    c->r[2] = 0x80450000u;
    c->r[7] = 0xFFFFFFFFu;
    *W(c->r[29]) = ra;
    c->r[6] = 0x80450001u;
    if (c->r[8] != 0xFFFFFFFFu) {
        c->r[31] = back; fn(c);
        c->r[6] = c->r[2];
    }
    c->r[2] = c->r[6];
    c->r[31] = ra; c->r[29] += 0x10;
}

SFA_FUNC(089EDF58, voice_word100_set_checked) { when_ready(c, f_089F2794, 0x089EDF80u); }
SFA_FUNC(089EDF94, voice_byte96_set_checked) { when_ready(c, f_089F25F4, 0x089EDFBCu); }
SFA_FUNC(089EDFD0, voice_byte97_set_checked) { when_ready(c, f_089F265C, 0x089EDFF8u); }
SFA_FUNC(089EE070, voice_byte98_set_checked) { when_ready(c, f_089F26C4, 0x089EE098u); }
SFA_FUNC(089EE0AC, voice_byte99_set_checked) { when_ready(c, f_089F272C, 0x089EE0D4u); }

/* (n): the sequence in slot n let go, with the same check; the answer travels in r5 here. */
SFA_FUNC(089EE210, sequence_slot_release_checked) {
    uint32_t ra = c->r[31];
    c->r[3] = 0x08AE0000u;
    c->r[7] = *W(SEQ_READY);
    c->r[29] -= 0x10;
    c->r[2] = 0x80450000u;
    c->r[6] = 0xFFFFFFFFu;
    *W(c->r[29]) = ra;
    ARG(1) = 0x80450001u;
    if (c->r[7] != 0xFFFFFFFFu) {
        c->r[31] = 0x089EE238u; f_089EF304(c);
        ARG(1) = c->r[2];
    }
    c->r[2] = ARG(1);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* A note's start, in three parts the original enters separately. The eight arguments are worked
   out into the note's settings (from +16 of the 0x80 frame; an error ends it). When the settings
   name a group (+84), every voice already sounding for it (the mask the routine at 0x089F2188
   gives) is stopped first. Then a voice is found (0x80540010 when there is none) and started
   with the settings: its number is returned, or the start's error. Keeps s0..s2.
   `from`: 0 the whole routine, 1 from the voice's finding (0x089EE730), 2 from the next voice
   of the stopping loop (0x089EE7B0). */
static void note_start(Cpu *c, int from) {
    gaddr sp;
    if (from == 0) {
        c->r[29] -= 0x80;
        sp = c->r[29];
        c->r[3] = sp + 0x10;
        *W(sp) = sp + 0x10;
        *W(sp + 124) = c->r[31];
        *W(sp + 120) = c->r[18]; *W(sp + 116) = c->r[17]; *W(sp + 112) = c->r[16];
        c->r[31] = 0x089EE71Cu; f_089EE8E4(c);
        c->r[3] = c->r[2];
        if (c->r[2] != 0) goto done;
        ARG(0) = *W(sp + 84);
        if (ARG(0) == 0) goto find;
        ARG(1) = *W(sp + 88);
        c->r[31] = 0x089EE780u; f_089F2188(c);
        c->r[17] = c->r[2];
        if (c->r[2] == 0) goto find;
        c->r[18] = 0x08E70000u;
        c->r[3] = *W(VOICE_N);
        c->r[16] = 0;
        if ((int32_t)c->r[3] <= 0) goto find;
        for (;;) {
            c->r[7] = c->r[17] >> (c->r[16] & 31);
            c->r[6] = c->r[7] & 1;
            ARG(0) = c->r[16];
            ARG(1) = 0;
            if (c->r[6] != 0) {
                c->r[31] = 0x089EE7D0u; f_089ED9B0(c);
                ARG(0) = c->r[16];
                c->r[31] = 0x089EE7D8u; f_089F1C68(c);
            }
            ARG(1) = *W(c->r[18] + 16512);
next_voice:
            c->r[16]++;
            ARG(0) = (int32_t)c->r[16] < (int32_t)ARG(1);
            if (ARG(0) == 0) break;
        }
        goto find;
    }
    sp = c->r[29];
    if (from == 2) goto next_voice;
find:
    ARG(0) = *W(sp + 88);
    c->r[31] = 0x089EE738u; f_089F1628(c);
    c->r[16] = c->r[2];
    c->r[2] = 0x80540000u;
    c->r[3] = 0x80540010u;
    if ((int32_t)c->r[16] >= 0) {
        ARG(0) = c->r[16];
        ARG(1) = sp + 0x10;
        c->r[31] = 0x089EE754u; f_089F1808(c);
        c->r[3] = c->r[2];
        if (c->r[2] == 0) c->r[3] = c->r[16];
    }
done:
    c->r[31] = *W(sp + 124);
    c->r[18] = *W(sp + 120);
    c->r[17] = *W(sp + 116);
    c->r[16] = *W(sp + 112);
    c->r[2] = c->r[3];
    c->r[29] += 0x80;
}

SFA_FUNC(089EE6FC, note_start_whole) { note_start(c, 0); }
SFA_FUNC(089EE730, note_start_find) { note_start(c, 1); }
SFA_FUNC(089EE7B0, note_start_next_voice) { note_start(c, 2); }

/* A note played on a bank's program, in two parts the original enters separately. (bank,
   program's place, -, note, -, three more settings): the bank in the slot (0 to 128) has the
   program looked up; each of the program's layers whose range of notes (+16 to +20 of the
   layer's record) takes the note starts a note of its own; the voices started come back as a
   mask (0 when the engine is not ready, the bank is not there or the program is not found).
   A 0x40 frame keeping s0..s8, with the program at +0, the layer at +4 and the slot at +8,
   which the routine's end closes. `from` 1 enters after a layer is done (0x089ED6DC). */
static void program_note(Cpu *c, int from) {
    gaddr sp;
    if (from == 0) {
        c->r[11] = *W(SEQ_READY);
        c->r[29] -= 0x40;
        sp = c->r[29];
        *W(sp + 16) = c->r[16];
        c->r[16] = ARG(0);
        *W(sp + 48) = c->r[30];
        c->r[30] = ARG(1);
        ARG(1) = ARG(2);
        *W(sp + 44) = c->r[23];
        c->r[6] = 0;
        c->r[23] = c->r[8];
        *W(sp + 40) = c->r[22];
        c->r[22] = c->r[9];
        *W(sp + 36) = c->r[21];
        c->r[21] = c->r[10];
        *W(sp + 32) = c->r[20];
        c->r[20] = 0;
        *W(sp + 24) = c->r[18];
        c->r[18] = ARG(3);
        *W(sp + 52) = c->r[31];
        *W(sp + 28) = c->r[19];
        *W(sp + 20) = c->r[17];
        if (c->r[11] == 0xFFFFFFFFu || c->r[16] >= 0x81) { f_089ED620(c); return; }
        c->r[17] = BANKS + (c->r[16] << 2);
        ARG(0) = *W(c->r[17]);
        if (ARG(0) == 0) { f_089ED620(c); return; }
        ARG(0) = *W(ARG(0));
        c->r[6] = sp;
        c->r[31] = 0x089ED65Cu; f_089EEF64(c);
        c->r[6] = 0;
        if (c->r[2] != 0) { f_089ED620(c); return; }
        ARG(0) = *W(sp);
        c->r[19] = 0;
        if (*W(ARG(0)) == 0) { c->r[6] = c->r[20]; f_089ED620(c); return; }
        *W(sp + 8) = c->r[17];
    } else {
        sp = c->r[29];
        goto layer_done;
    }
    for (;;) {
        c->r[12] = *W(sp + 8);
        c->r[17] = c->r[19] << 2;
        c->r[10] = *W(c->r[12]);
        ARG(1) = *W(c->r[17] + *W(sp) + 16);
        c->r[6] = sp + 4;
        ARG(0) = *W(c->r[10]);
        c->r[19]++;
        c->r[31] = 0x089ED69Cu; f_089EEEC4(c);
        c->r[3] = *W(sp + 4);
        if ((int32_t)c->r[2] >= 0 && c->r[18] >= *W(c->r[3] + 16)) {
            c->r[24] = *W(c->r[3] + 20);
            ARG(0) = c->r[16];
            c->r[6] = c->r[30];
            ARG(3) = c->r[18];
            c->r[8] = 0;
            c->r[9] = c->r[23];
            c->r[10] = c->r[21];
            c->r[11] = c->r[22];
            if (c->r[24] >= c->r[18]) {
                ARG(1) = *W(c->r[17] + *W(sp) + 16);
                c->r[17] = 1;
                c->r[31] = 0x089ED70Cu; f_089EE6FC(c);
                if ((int32_t)c->r[2] >= 0) c->r[20] |= 1u << (c->r[2] & 31);
            }
        }
layer_done:
        ARG(0) = *W(sp);
        c->r[7] = *W(ARG(0));
        if (c->r[19] >= c->r[7]) break;
    }
    c->r[6] = c->r[20];
    f_089ED620(c);
}

SFA_FUNC(089ED598, program_note_play) { program_note(c, 0); }
SFA_FUNC(089ED6DC, program_note_layer_done) { program_note(c, 1); }
