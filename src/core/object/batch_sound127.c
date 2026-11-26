/* A sound cue queued (68 instructions, 0x0880A0BC, called for every sound effect and voice): the cue's
   chain of 16-byte entries walked and each turned into commands on the sound ring (0x08809A08). Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08809A08(Cpu *c);

#define CUE_TABLES 0x092DA0F0u   /* +160: a cue list per (group + bank) */

static void put(Cpu *c, uint32_t cmd, uint32_t vol, uint32_t a, uint32_t b, uint32_t back) {
    ARG(0) = cmd; ARG(1) = vol; ARG(2) = a; ARG(3) = b;
    c->r[31] = back; f_08809A08(c);
}

static gaddr cue_entry(uint32_t bank, uint32_t id) {
    return *W(CUE_TABLES + ((((id & 0xF00) >> 8) + bank) << 2) + 160) + ((id & 0xFF) << 4);
}

/* (bank, volume offset, cue id, pan): ids with any of bits 12 - 15 do nothing. Each entry (+0 command,
   +2 volume, +4 / +6 / +8 / +12 values, +10 pan, +14 next) gives a volume (+2 plus the offset, at most
   7) and a pan (+10, or when that is -0x8000 the caller's pan / 6 - 32 held to -31 .. 31, kept for the
   next entries). By the command's low byte: 0 a full start (commands 2, 8 with +8, 9 with the pan, 10
   with +12, then 1 with +4, whose high byte is offset by the bank for group 3 or for a non-zero high
   byte in command group 4, and +6); 8 with +8; 9 with the pan; anything else (and -1 as a command, then
   skipped) on its own. The next entry (+14, low byte in the same group) follows until it is 0 or -1. */
SFA_FUNC(0880A0BC, sound_cue_queue) {
    uint32_t bank = ARG(0);
    uint32_t ra = c->r[31];
    uint32_t sv[9] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22], c->r[23], c->r[30] };
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    uint32_t id = ARG(2) & 0xFFFF;
    int32_t pan = (int16_t)ARG(3);
    *W(sp + 28) = sv[2]; *W(sp + 32) = sv[3];
    *W(sp + 40) = sv[5];
    *W(sp + 20) = sv[0]; *W(sp + 24) = sv[1]; *W(sp + 36) = sv[4];
    *W(sp + 44) = sv[6]; *W(sp + 48) = sv[7]; *W(sp + 52) = sv[8]; *W(sp + 56) = ra;
    *W(sp + 16) = ARG(1);
    c->r[21] = bank; c->r[19] = id; c->r[18] = (uint32_t)pan;
    ARG(2) = id & 0xF000;
    if ((id & 0xF000) == 0) {
        c->r[22] = CUE_TABLES; c->r[20] = 0xFFFFFFFFu; c->r[30] = 3; c->r[23] = 0x400;
        gaddr e = cue_entry(bank, id);
        for (;;) {
            c->r[16] = e;
            int32_t vol = (int16_t)(S16(e + 2) + *W(sp + 16));
            if (vol >= 8) vol = 7;
            c->r[17] = (uint32_t)vol;
            int32_t p10 = S16(e + 10);
            int32_t cmd = S16(e);
            if (p10 != -0x8000) pan = p10;
            else {
                DIV(c, (uint32_t)pan, 6);
                pan = (int16_t)((int32_t)c->lo - 32);
                if (pan >= 32) pan = 31;
                if (pan < -31) pan = -31;
            }
            c->r[18] = (uint32_t)pan;
            uint32_t v = (uint32_t)vol, kind = (uint32_t)cmd & 0xFF;
            if (cmd != -1) {
                if (kind == 0) {
                    uint32_t grp = (uint32_t)cmd & 0xFF00;
                    put(c, (grp | 2) & 0xFFFF, v, 0, 0, 0x0880A218u);
                    put(c, (((uint32_t)S16(e) & 0xFF00) | 8) & 0xFFFF, v, (uint32_t)S16(e + 8), 0, 0x0880A238u);
                    put(c, (((uint32_t)S16(e) & 0xFF00) | 9) & 0xFFFF, v, (uint32_t)pan, 0, 0x0880A258u);
                    put(c, (((uint32_t)S16(e) & 0xFF00) | 0xA) & 0xFFFF, v, (uint32_t)S16(e + 12), 0, 0x0880A278u);
                    uint32_t w4 = (uint32_t)S16(e + 4);
                    int32_t hi = (int16_t)((w4 & 0xFF00) >> 8);
                    uint32_t c0 = (uint32_t)S16(e);
                    int32_t shifted;
                    if (hi == 3 || ((c0 & 0xF00) == 0x400 && hi != 0)) shifted = (int16_t)((uint32_t)(hi + (int32_t)bank) << 8);
                    else shifted = (int16_t)((uint32_t)hi << 8);
                    put(c, ((c0 & 0xFF00) | 1) & 0xFFFF, v, (uint32_t)(int32_t)(int16_t)((w4 & 0xFF) | (uint32_t)shifted),
                        (uint32_t)S16(e + 6), 0x0880A2F0u);
                } else if (kind == 8) {
                    put(c, (uint32_t)cmd & 0xFFFF, v, (uint32_t)S16(e + 8), 0, 0x0880A30Cu);
                } else if (kind == 9) {
                    put(c, (uint32_t)cmd & 0xFFFF, v, (uint32_t)pan, 0, 0x0880A1F4u);
                } else {
                    put(c, (uint32_t)cmd & 0xFFFF, v, 0, 0, 0x0880A328u);
                }
            }
            int32_t next = S16(e + 14);
            c->r[16] = (uint32_t)next;
            if (next == -1 || next == 0) break;
            id = ((id & 0xFF00) | (uint32_t)next) & 0xFFFF;
            c->r[19] = id;
            e = cue_entry(bank, id);
        }
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3]; c->r[20] = sv[4];
    c->r[21] = sv[5]; c->r[22] = sv[6]; c->r[23] = sv[7]; c->r[30] = sv[8];
    c->r[31] = ra;
    c->r[29] += 0x40;
}
