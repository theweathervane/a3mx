/* The sound's loader callback: what happens when a piece of a sound bank has been read. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define TAG_SMPB 0x42504D53u /* the four letters as the code compares them, as one number */
#define TAG_RMPB 0x42504D52u
#define TAG_SOSB 0x42534F53u
#define TAG_PPHD 0x44485050u

void f_089ED490(Cpu *c); void f_089EDAC0(Cpu *c);

/* A music bank's record (0xA414 bytes at 0x08D52380 for each bank) with both its pieces (+0 and
   +4) there is entered (the number kept at +8), and each of its 128 sequences that has data
   (+20 of its 0x148 bytes) is loaded for it (the answer at +24). The two callers differ only in
   the registers they keep things in and the places they return to. */
static void music_bank_enter(Cpu *c, gaddr bank, uint32_t back_enter, uint32_t back_load, int list_in_s3) {
    ARG(0) = bank + 0xC; ARG(1) = *W(bank); ARG(2) = *W(bank + 4);
    c->r[31] = back_enter; f_089ED490(c);
    *W(bank + 8) = c->r[2];
    gaddr seq = bank;
    for (uint32_t i = 0; i < 128; i++, seq += 0x148) {
        c->r[17] = i; c->r[18] = seq;
        if (list_in_s3) c->r[19] = seq + 0x1C; else c->r[20] = seq + 0x1C;
        uint32_t data = *W(seq + 20);
        if (data == 0) continue;
        ARG(0) = seq + 0x1C; ARG(1) = data; ARG(2) = *W(bank + 8);
        c->r[31] = back_load; f_089EDAC0(c);
        *W(seq + 24) = c->r[2];
    }
}

/* (request, tag, bank): a piece of a bank has been read. For the tag "SMPB" (the samples), unless
   the piece itself starts with those letters, the music bank is entered once both its pieces are
   there. For "SOSB" with a piece that starts "PPHD" and a size (request + 8) below 4096, the
   effects bank (0x1204 bytes each from 0x08ACCEE0) gets its table: the count from the chunk the
   bank's +24 leads to (its +20 less +16, plus one) at +4096, and each effect's place (the chunk's
   +8 bytes apart, from its +32) from +4100. Last, the request's own routine (+12, unless it is 0
   or -1) is called with its +16. Returns 0. A 0x30 frame keeping s0..s5. */
SFA_FUNC(089E2B60, bank_piece_loaded) {
    gaddr req = ARG(0);
    int32_t tag = (int32_t)ARG(1);
    int32_t bank_n = (int8_t)ARG(2);
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 32) = sv[4]; *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    *W(sp + 28) = sv[3]; *W(sp + 36) = sv[5]; *W(sp + 40) = sv[6];
    c->r[16] = req;
    c->r[20] = (uint32_t)bank_n;
    if (tag == (int32_t)TAG_SMPB) {
        int32_t head = (int32_t)*W(*W(req + 4));
        if (head != (int32_t)TAG_SMPB) {
            int64_t m = (int64_t)bank_n * 0xA414;
            c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            gaddr bank = 0x08D52380u + c->lo;
            if (head == (int32_t)TAG_PPHD) {
                c->r[20] = bank;
                if (*W(bank) != 0 && *W(bank + 4) != 0) music_bank_enter(c, bank, 0x089E2C58u, 0x089E2C84u, 1);
            } else {
                c->r[19] = bank;
                c->r[20] = *W(bank + 4);
                if (*W(bank + 4) != 0 && *W(bank) != 0) {
                    c->r[21] = c->lo;
                    music_bank_enter(c, bank, 0x089E2CE8u, 0x089E2D14u, 0);
                }
            }
        }
    } else if (tag == (int32_t)TAG_SOSB) {
        int32_t head = (int32_t)*W(*W(req + 4));
        if (head == (int32_t)TAG_PPHD && (int32_t)*W(req + 8) < 4096) {
            gaddr bank = 0x08ACCEE0u + (uint32_t)bank_n * 0x1204;
            gaddr chunk = bank + *W(bank + 24);
            int32_t count = (int32_t)(*W(chunk + 20) - *W(chunk + 16) + 1);
            *W(bank + 4096) = (uint32_t)count;
            for (int32_t i = 0; i < count; i++) {
                int64_t m = (int64_t)(int32_t)*W(chunk + 8) * i;
                c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
                *W(bank + 4 * (uint32_t)i + 4100) = chunk + 0x20 + c->lo;
            }
        }
    }
    uint32_t fn = *W(req + 12);
    if (fn != 0 && fn != 0xFFFFFFFFu) {
        ARG(1) = fn;
        ARG(0) = *W(req + 16); c->r[31] = 0x089E2E00u;
        rt_call_indirect(c, fn);
    }
    c->r[2] = 0;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}
