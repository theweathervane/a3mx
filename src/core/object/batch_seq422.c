/* The sound engine's tick: the sequences played forward, then the voices moved on. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SEQ_READY    0x08AD80D0u
#define VOICES_READY 0x08AD81ECu
#define SEQUENCES    0x08E74308u /* the 128 sequences' slots */
#define VOICES       0x08E74534u /* 0x68 bytes a voice: +0 its number, +1 its state, +4 its
                                    envelope's height */
#define VOICE_COUNT  0x08E74508u

void f_089ECD08(Cpu *c); void f_089ECCAC(Cpu *c); void f_089F29FC(Cpu *c); void f_089ECADC(Cpu *c);
void f_089F2BC8(Cpu *c); void f_089EC8E4(Cpu *c); void f_089ECECC(Cpu *c); void f_089ECF08(Cpu *c);
void f_089F2928(Cpu *c); void f_089F2898(Cpu *c); void f_089EF3F4(Cpu *c); void f_089F0504(Cpu *c);
void f_089F01CC(Cpu *c);

/* (voice's record): the voice's sound is set: a noise (its kind at +40) when +28 is set, else
   the sample at +32 of the size at +36. The routine's other part goes on from there. A 0x30
   frame keeping s0..s3. */
SFA_FUNC(089F29C0, voice_sound_set) {
    gaddr v = ARG(0);
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = c->r[16];
    *W(sp + 32) = c->r[31];
    *W(sp + 28) = c->r[19]; *W(sp + 24) = c->r[18]; *W(sp + 20) = c->r[17];
    c->r[3] = *W(v + 28);
    c->r[16] = v;
    if (c->r[3] != 0) {
        ARG(0) = *B(v);
        ARG(1) = *B(v + 40);
        c->r[31] = 0x089F2BC0u; f_089ECD08(c);
    } else {
        ARG(0) = *B(v);
        ARG(1) = *W(v + 32);
        ARG(2) = *W(v + 36);
        ARG(3) = 1;
        c->r[31] = 0x089F29F8u; f_089ECCAC(c);
    }
    c->r[18] = *B(c->r[16]);
    f_089F29FC(c);
}

/* Every voice in one list goes to the end of another in a new state, after being let go
   (`off`) and given its sound (`sound`) as asked; the first list is then empty. The registers
   hold what the original's loops do. */
static void voices_move(Cpu *c, gaddr from_n, gaddr from, gaddr to_n, gaddr to, uint8_t state,
                        int off, int sound, uint32_t back_off, uint32_t back_sound) {
    c->r[17] = 0;
    if (*W(from_n) != 0) {
        c->r[21] = to;
        c->r[16] = from;
        c->r[20] = state;
        do {
            gaddr v = *W(c->r[16]);
            c->r[17]++;
            if (off) { ARG(0) = *B(v); c->r[31] = back_off; f_089ECADC(c); }
            if (sound) { ARG(0) = *W(c->r[16]); c->r[31] = back_sound; f_089F29C0(c); }
            uint32_t n = *W(to_n);
            v = *W(c->r[16]);
            *W(to + (n << 2)) = v;
            *W(to_n) = n + 1;
            *B(v + 1) = state;
            c->r[16] += 4;
        } while (c->r[17] < *W(from_n));
    }
    *W(from_n) = 0;
}

/* The voices' tick, in three parts the original enters separately. (output): the voices asked
   to stop are let go, those with a new note are let go and given their sound, and each moves to
   the list of its next state; the engine's own update runs, and the core produces the output
   (-1 if it fails). Then every voice has its envelope's height read, and the ones the core
   says have ended (those in states 3, 5 and 7) go back to the free list. Returns 0, or -1
   before the voices are ready. A 0x30 frame keeping s0..s8. `from`: 0 the whole routine, 1
   from the next voice (0x089F11D8), 2 from a voice's freeing with its list in r4 and r5
   (0x089F122C). */
static void voices_tick(Cpu *c, int from) {
    gaddr sp;
    if (from == 0) {
        c->r[6] = *W(VOICES_READY);
        c->r[29] -= 0x30;
        sp = c->r[29];
        *W(sp + 24) = c->r[22];
        c->r[22] = ARG(0);
        ARG(0) = 0xFFFFFFFFu;
        *W(sp + 36) = c->r[31]; *W(sp + 32) = c->r[30]; *W(sp + 28) = c->r[23];
        *W(sp + 20) = c->r[21]; *W(sp + 16) = c->r[20]; *W(sp + 12) = c->r[19];
        *W(sp + 8) = c->r[18]; *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
        if (c->r[6] == 0xFFFFFFFFu) goto done;
        voices_move(c, 0x08E7451Cu, 0x08E75434u, 0x08E74520u, 0x08E754B4u, 5, 1, 0, 0x089F0F68u, 0);
        voices_move(c, 0x08E74524u, 0x08E75534u, 0x08E74518u, 0x08E753B4u, 3, 1, 1, 0x089F0FD4u, 0x089F0FDCu);
        voices_move(c, 0x08E7452Cu, 0x08E75634u, 0x08E74528u, 0x08E755B4u, 7, 1, 1, 0x089F1048u, 0x089F1050u);
        voices_move(c, 0x08E74514u, 0x08E75334u, 0x08E74518u, 0x08E753B4u, 3, 0, 1, 0, 0x089F10B8u);
        voices_move(c, 0x08E74530u, 0x08E756B4u, 0x08E74528u, 0x08E755B4u, 7, 0, 1, 0, 0x089F1120u);
        c->r[31] = 0x089F1158u; f_089F2BC8(c);
        ARG(0) = c->r[22];
        c->r[31] = 0x089F1160u; f_089EC8E4(c);
        ARG(0) = 0xFFFFFFFFu;
        if ((int32_t)c->r[2] < 0) goto done;
        c->r[19] = 0x08E70000u;
        c->r[31] = 0x089F1170u; f_089ECECC(c);
        c->r[22] = *W(VOICE_COUNT);
        c->r[18] = c->r[2];
        c->r[17] = 0;
        ARG(0) = 0;
        if (c->r[22] == 0) goto done;
        c->r[16] = VOICES;
        c->r[23] = 0x08E70000u;
        c->r[20] = 5;
        c->r[22] = 0x08E70000u;
        c->r[21] = 1;
        c->r[30] = 7;
    } else {
        sp = c->r[29];
        if (from == 1) goto next;
        goto release;
    }
    for (;;) {
        ARG(0) = *B(c->r[16]);
        c->r[31] = 0x089F11A4u; f_089ECF08(c);
        *W(c->r[16] + 4) = c->r[2];
        if (((c->r[18] >> (*B(c->r[16]) & 31)) & 1) != 0) {
            int32_t state = S8(c->r[16] + 1);
            c->r[3] = (uint32_t)state;
            if (state == 5) { ARG(0) = 0x08E74520u; ARG(1) = 0x08E754B4u; goto release; }
            if (state == 3) { ARG(0) = 0x08E74518u; ARG(1) = 0x08E753B4u; goto release; }
            if (state == 7) { ARG(0) = 0x08E74528u; ARG(1) = 0x08E755B4u; goto release; }
        }
        c->r[11] = *W(c->r[19] + 17672);
        goto next;
release:
        c->r[6] = c->r[23] + 0x4510;
        c->r[7] = c->r[22] + 0x52B4;
        c->r[8] = c->r[17];
        c->r[31] = 0x089F123Cu; f_089F2928(c);
        *B(c->r[2] + 1) = (uint8_t)c->r[21];
        ARG(0) = c->r[2];
        c->r[31] = 0x089F1248u; f_089F2898(c);
        c->r[11] = *W(c->r[19] + 17672);
next:
        c->r[17]++;
        ARG(0) = c->r[17] < c->r[11];
        if (ARG(0) == 0) break;
        c->r[16] += 0x68;
    }
    ARG(0) = 0;
done:
    c->r[31] = *W(sp + 36);
    c->r[30] = *W(sp + 32);
    c->r[23] = *W(sp + 28); c->r[22] = *W(sp + 24); c->r[21] = *W(sp + 20); c->r[20] = *W(sp + 16);
    c->r[19] = *W(sp + 12); c->r[18] = *W(sp + 8); c->r[17] = *W(sp + 4); c->r[16] = *W(sp);
    c->r[2] = ARG(0);
    c->r[29] += 0x30;
}

SFA_FUNC(089F0EEC, voices_tick_whole) { voices_tick(c, 0); }
SFA_FUNC(089F11D8, voices_tick_next) { voices_tick(c, 1); }
SFA_FUNC(089F122C, voices_tick_release) { voices_tick(c, 2); }

/* The sequences' tick, in three parts the original enters separately. The time one tick takes
   (the engine's samples a tick at 0x08E74304, as millionths of a second at 44100 a second) is
   taken off every playing sequence (its first byte at 1): one whose time to its next event
   (+48) is longer just waits; otherwise its events are carried out one after another, each
   followed by reading the time to the next (the track's delta, kept above 0, over the
   sequence's division at +10, times its tempo's step at +60), until one lies ahead, the
   sequence stops, or an event answers -1, which ends the tick. A 0x30 frame keeping s0..s8,
   which the end closes. `from`: 0 the whole routine, 1 from the next sequence (0x089EF3E0), 2
   from a sequence's next event (0x089EF448). */
static void sequences_tick(Cpu *c, int from) {
    gaddr sp;
    if (from == 0) {
        int64_t m = (int64_t)(int32_t)*W(0x08E74304u) * 1000000;
        c->r[29] -= 0x30;
        sp = c->r[29];
        *W(sp + 36) = c->r[31];
        uint64_t u = (uint64_t)(uint32_t)m * 0xBE37C63Bu;
        c->lo = (uint32_t)u; c->hi = (uint32_t)(u >> 32);
        *W(sp + 28) = c->r[23]; *W(sp + 24) = c->r[22]; *W(sp + 20) = c->r[21];
        *W(sp + 12) = c->r[19]; *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
        *W(sp + 32) = c->r[30];
        c->r[30] = SEQUENCES;
        *W(sp + 16) = c->r[20];
        c->r[20] = c->hi >> 15;
        *W(sp + 8) = c->r[18];
        c->r[18] = 0;
        c->r[16] = 0;
    } else {
        sp = c->r[29];
        if (from == 1) goto next;
        goto event;
    }
    for (;;) {
        c->r[10] = c->r[16] + c->r[30];
        c->r[2] = *W(c->r[10]);
        if (c->r[2] != 0 && *B(c->r[2]) == 1) {
            c->r[13] = 0x08E70000u;
            c->r[21] = SEQUENCES;
            c->r[19] = c->r[16] + SEQUENCES;
            c->r[23] = 1;
            c->r[22] = 0xFFFFFFFFu;
            c->r[15] = *W(c->r[19]);
            for (;;) {
                c->r[17] = c->r[16] + c->r[21];
                c->r[14] = *B(c->r[15] + 12);
                ARG(0) = c->r[15];
                if (c->r[14] != 0) {
                    c->r[31] = 0x089EF49Cu; f_089F0504(c);
                    gaddr seq = *W(c->r[19]);
                    c->r[24] = seq;
                    c->r[7] = *H(seq + 10);
                    c->r[8] = (uint32_t)MAX((int32_t)(*W(seq + 16) + *W(seq + 20)), 0);
                    DIV(c, c->r[8], c->r[7]);
                    if (c->r[7] == 0) rt_break(c, 0x089EF4C0u);
                    int64_t step = (int64_t)(int32_t)*W(seq + 60) * (int32_t)c->lo;
                    c->lo = (uint32_t)step; c->hi = (uint32_t)(step >> 32);
                    *W(seq + 48) = *W(seq + 48) + c->lo;
                }
event:
                c->r[3] = *W(c->r[17]);
                ARG(1) = *W(c->r[3] + 48);
                ARG(0) = c->r[3];
                if (ARG(1) >= c->r[20]) {
                    c->r[16] = ARG(1) - c->r[20];
                    *W(c->r[3] + 48) = c->r[16];
                    *B(c->r[3] + 12) = 0;
                    break;
                }
                c->r[31] = 0x089EF464u; f_089F01CC(c);
                c->r[3] = *W(c->r[17]);
                c->r[17] = *B(c->r[3]);
                *B(c->r[3] + 12) = (uint8_t)c->r[23];
                if (c->r[17] == 0) break;
                if (c->r[2] == c->r[22]) {
                    c->r[31] = *W(sp + 36);
                    f_089EF3F4(c);
                    return;
                }
                c->r[15] = *W(c->r[19]);
            }
        }
next:
        c->r[18]++;
        c->r[19] = c->r[18] < 0x80;
        c->r[16] = c->r[18] << 2;
        if (c->r[19] == 0) break;
    }
    c->r[31] = *W(sp + 36);
    f_089EF3F4(c);
}

SFA_FUNC(089EF358, sequences_tick_whole) { sequences_tick(c, 0); }
SFA_FUNC(089EF3E0, sequences_tick_next) { sequences_tick(c, 1); }
SFA_FUNC(089EF448, sequences_tick_event) { sequences_tick(c, 2); }

/* (output): one tick of the sound engine: the sequences, then the voices into the output.
   0x80450001 before the sequences are ready, else 0. A 0x10 frame keeping s0. */
SFA_FUNC(089EDC8C, sound_engine_tick) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[6] = *W(SEQ_READY);
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0;
    c->r[16] = ARG(0);
    ARG(0) = 0xFFFFFFFFu;
    *W(c->r[29] + 4) = ra;
    ARG(1) = 0x80450001u;
    if (c->r[6] != 0xFFFFFFFFu) {
        c->r[31] = 0x089EDCBCu; f_089EF358(c);
        ARG(0) = c->r[16];
        c->r[31] = 0x089EDCC4u; f_089F0EEC(c);
        ARG(1) = 0;
    }
    c->r[2] = ARG(1);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}
