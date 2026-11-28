/* The sound's thread, and the memory a sound file is read into by its kind. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define MUSIC_BANKS 0x08D52380u /* 0xA414 bytes a bank: +0 and +4 its two pieces, +8 its number
                                   in the engine, then 128 sequences of 0x148 bytes from +0,
                                   each with its data at +20 and its number at +24 */

void f_089E5838(Cpu *c); void f_089E5A2C(Cpu *c); void f_089EDC8C(Cpu *c); void f_089EB528(Cpu *c);
void f_089EB878(Cpu *c); void f_089EE210(Cpu *c); void f_089E0540(Cpu *c); void f_089E04EC(Cpu *c);
void f_089ED540(Cpu *c);
void hle_sceKernelDelayThread(Cpu *c); void hle_sceKernelExitDeleteThread(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }

/* The sound's thread: while the sound runs (the word at 0x08E692B0), each turn the requests are
   carried out, the voices kept up, and one tick of the engine written to a quarter of one of
   the two output buffers (0x1000 bytes each from 0x08E67200); every fourth turn the buffer goes
   out at the effects' volume (the byte at 0x08AD80C0 as a part of 0x8000 over 15, both sides)
   and the other buffer is used. When the sound stops the output is waited for and the thread
   ends and is deleted. A 0x40 frame keeping s0..s7. */
SFA_FUNC(089E5BEC, sound_thread) {
    uint32_t sv[8], ra = c->r[31];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 36) = sv[5];
    c->r[21] = 0x08E70000u;
    *W(sp + 44) = sv[7];
    c->r[23] = 0;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3];
    *W(sp + 32) = sv[4]; *W(sp + 40) = sv[6]; *W(sp + 48) = ra;
    c->r[22] = 0;
    if (*W(0x08E692B0u) != 0) {
        c->r[19] = 0x08E67200u;
        c->r[18] = 4;
        c->r[16] = 0xF;
        c->r[17] = 0x08AE0000u;
        do {
            c->r[31] = 0x089E5C44u; f_089E5838(c);
            c->r[31] = 0x089E5C4Cu; f_089E5A2C(c);
            c->r[20] = c->r[22] << 12;
            call1(c, c->r[20] + (c->r[23] << 10) + c->r[19], f_089EDC8C, 0x089E5C68u);
            c->r[23]++;
            if (c->r[23] == c->r[18]) {
                DIV(c, (uint32_t)*B(0x08AD80C0u) << 15, c->r[16]);
                c->r[23] = 0;
                ARG(3) = c->r[20] + c->r[19];
                ARG(0) = 0;
                ARG(1) = c->lo;
                ARG(2) = c->lo;
                c->r[31] = 0x089E5C9Cu; f_089EB528(c);
                c->r[22] ^= 1;
            }
        } while (*W(0x08E692B0u) != 0);
    }
    for (;;) {
        call1(c, 0, f_089EB878, 0x089E5CB4u);
        if ((int32_t)c->r[2] <= 0) break;
        call1(c, 0x3E8, hle_sceKernelDelayThread, 0x089E5CC4u);
    }
    call1(c, 0, hle_sceKernelExitDeleteThread, 0x089E5CD4u);
    c->r[2] = 0;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}

/* (bank, sequence, size): memory for a sequence's file: the sequence's slot (0 to 127) in the
   music bank has what it held let go (its number in the engine, then its data), and memory of
   the size is taken for it. The memory is returned, or 0 for a sequence past 127. A 0x20 frame
   keeping s0, s1. */
SFA_FUNC(089E2AA0, sequence_memory_take) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    int32_t seq_n = (int8_t)ARG(1);
    int32_t bank_n = (int8_t)ARG(0);
    c->r[17] = (uint32_t)seq_n;
    *W(sp + 16) = s0; *W(sp + 24) = ra;
    c->r[16] = ARG(2);
    c->r[2] = 0;
    if (seq_n < 128) {
        int64_t m = (int64_t)bank_n * 0xA414;
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        gaddr seq = MUSIC_BANKS + c->lo + (uint32_t)seq_n * 0x148;
        c->r[17] = seq;
        if (*W(seq + 20) != 0) {
            call1(c, *W(seq + 24), f_089EE210, 0x089E2B30u);
            ARG(0) = *W(seq + 20);
            *W(seq + 24) = 0;
            c->r[31] = 0x089E2B3Cu; f_089E0540(c);
            *W(seq + 20) = 0;
        }
        call1(c, c->r[16], f_089E04EC, 0x089E2B48u);
        *W(seq + 20) = c->r[2];
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* One of a music bank's two pieces is given new memory: when the piece is there already the
   whole bank is let go first (its number in the engine, both pieces, and every sequence's
   number). */
static void bank_piece_take(Cpu *c, uint32_t piece, uint32_t back) {
    int64_t m = (int64_t)(int32_t)c->r[19] * 0xA414;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    c->r[18] = MUSIC_BANKS;
    c->r[17] = c->lo;
    gaddr bank = c->lo + MUSIC_BANKS;
    c->r[19] = bank;
    if (*W(bank + piece) != 0) {
        c->r[20] = c->r[17];
        call1(c, *W(bank + 8), f_089ED540, back);
        ARG(0) = *W(bank);
        *W(bank + 8) = 0;
        c->r[31] = back + 0xC; f_089E0540(c);
        ARG(0) = *W(bank + 4);
        *W(bank) = 0;
        c->r[31] = back + 0x18; f_089E0540(c);
        *W(bank + 4) = 0;
        c->r[18] = bank;
        for (c->r[17] = 0; (int32_t)c->r[17] < 128; c->r[17]++, c->r[18] += 0x148) {
            if (*W(c->r[18] + 20) == 0) continue;
            call1(c, *W(c->r[18] + 24), f_089EE210, back + 0x38);
            *W(c->r[18] + 24) = 0;
        }
    }
    call1(c, c->r[16], f_089E04EC, back + 0x54);
    *W(bank + piece) = c->r[2];
}

/* (kind, number, size, piece): where a sound file's piece is to be read to, 0 for nowhere. For
   "SMPB" (a music bank's samples) piece 0 or 1 of the bank gets memory of the size. For "SOSB"
   (the effects) piece 0 is the effects bank's own record (0x1204 bytes each from 0x08ACCEE0)
   when the size is under 4096, and piece 1 the place the table at 0x08ACCEB0 gives, when the
   size is under what it allows. A 0x30 frame keeping s0..s4. */
SFA_FUNC(089E2E28, sound_file_place) {
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    int32_t tag = (int32_t)ARG(0);
    int32_t number = (int8_t)ARG(1);
    int32_t piece = (int8_t)ARG(3);
    c->r[19] = (uint32_t)number;
    ARG(1) = (uint32_t)tag;
    ARG(0) = (uint32_t)piece;
    c->r[2] = 0;
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 32) = sv[4];
    *W(sp + 36) = ra;
    int32_t size = (int32_t)ARG(2);
    c->r[16] = (uint32_t)size;
    if (tag == 0x42504D53) {
        if (piece == 0) bank_piece_take(c, 0, 0x089E2F00u);
        else if (piece == 1) bank_piece_take(c, 4, 0x089E2F88u);
    } else if (tag == 0x42534F53) {
        if (piece == 0) {
            if (size < 4096) c->r[2] = 0x08ACCEE0u + (uint32_t)number * 0x1204;
        } else if (piece == 1) {
            gaddr entry = 0x08ACCEB0u + ((uint32_t)number << 3);
            c->r[19] = entry;
            if (size < (int32_t)*W(entry + 4)) c->r[2] = *W(entry);
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}
