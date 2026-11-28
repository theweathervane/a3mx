/* Two of the library's routines that wait: the thread that plays a music stream, and a file read
   whole. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND 0x08E69200u /* the sound's state: +148 its event flag, +156 the stream's volume,
                             +160 the fade's step, +166 set when the stream is stopped */

void f_089E5B68(Cpu *c); void f_089E5B4C(Cpu *c); void f_089EB3E8(Cpu *c); void f_089EB528(Cpu *c);
void f_089F3430(Cpu *c); void f_089F3470(Cpu *c); void f_089EB878(Cpu *c); void f_089E5BC4(Cpu *c);
void f_08808418(Cpu *c);
void hle_sceAtracGetRemainFrame(Cpu *c); void hle_sceAtracDecodeData(Cpu *c);
void hle_sceAtracGetInternalErrorInfo(Cpu *c); void hle_sceKernelDelayThread(Cpu *c);
void hle_sceUmdGetDriveStat(Cpu *c); void hle_sceIoOpen(Cpu *c); void hle_sceIoReadAsync(Cpu *c);
void hle_sceIoClose(Cpu *c); void hle_sceIoPollAsync(Cpu *c); void hle_sceKernelDcacheWritebackRange(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* Whether one of the stream's requests (bits of the flag at +4 of the stream) is waiting. */
static int requested(Cpu *c, gaddr stream, uint32_t bits, uint32_t back) {
    call2(c, *W(stream + 4), bits, f_089E5B68, back);
    return c->r[2] != 0;
}

/* The block at `from` (in samples from the buffer in s4) goes to the stream's output channel
   (+8 of the stream), n samples of it, at the stream's volume times the music volume (the byte
   at 0x08AD80C1) over 15, the same on both sides. */
static void send_block(Cpu *c, gaddr stream, uint32_t n, uint32_t divisor, uint32_t back, uint32_t back2) {
    call2(c, *W(stream + 8), n, f_089EB3E8, back);
    int64_t m = (int64_t)(int32_t)*W(SOUND + 156) * (int32_t)*B(0x08AD80C1u);
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    DIV(c, c->lo, divisor);
    c->r[7] = c->r[20] + (c->r[19] << 2);
    ARG(0) = *W(stream + 8);
    ARG(1) = c->lo;
    ARG(2) = ARG(1);
    c->r[31] = back2; f_089EB528(c);
}

/* (stream): the thread that plays a music stream (+0 its decoder, +4 its requests, +8 its
   channel). Each turn: a fade out (0x40) or in (0x20) moves the volume by the step, between 0
   and 0x8000, and lets go of its request at the end; unless paused (0x10, which waits 1667
   microseconds while no fade runs), more is decoded after what is waiting in the buffer, and
   once 0x800 samples or more wait, whole blocks of 64 are sent; past 0x1800 samples the rest
   moves back to the buffer's start. A stop (8) clears the volume and marks the stream stopped;
   otherwise the turns go on until the decoder says the stream has ended, when what is left is
   sent with silence after it up to 0x800 samples (unless the volume is 0). Then the channel is
   waited for, and the stream closed. A 0x9A20 frame keeping s0..s8, with the buffer inside it
   on a boundary of 64 bytes. */
SFA_FUNC(089E68A4, music_stream_thread) {
    gaddr stream = ARG(0);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[1] = 0x9A20;
    c->r[29] -= 0x9A20;
    gaddr sp = c->r[29];
    *W(sp + 40) = sv[1]; *W(sp + 52) = sv[4]; *W(sp + 64) = sv[7];
    c->r[17] = stream;
    gaddr buf = (sp + 0x1A1C) & 0xFFFFFFC0u;
    c->r[23] = 0xFFFFFFC0u;
    *W(sp + 44) = sv[2]; *W(sp + 48) = sv[3]; *W(sp + 60) = sv[6];
    gaddr remain = sp + 0x19D4;
    c->r[22] = remain;
    c->r[18] = 0; c->r[19] = 0;
    c->r[20] = buf;
    *W(sp + 36) = sv[0]; *W(sp + 56) = sv[5]; *W(sp + 68) = sv[8]; *W(sp + 72) = sv[9];
    call2(c, *W(stream), remain, hle_sceAtracGetRemainFrame, 0x089E68FCu);
    c->r[16] = SOUND;
    c->r[30] = 0x80630023u;
    c->r[21] = 0x8000;
    uint32_t volume;
    for (;;) {
        if (requested(c, stream, 0x40, 0x089E691Cu)) {
            int32_t v = (int32_t)(*W(SOUND + 156) - *W(SOUND + 160));
            *W(SOUND + 156) = (uint32_t)v;
            if (v < 0) {
                *W(SOUND + 156) = 0;
                call2(c, *W(SOUND + 148), 0x40, f_089E5B4C, 0x089E6948u);
            }
        }
        if (requested(c, stream, 0x20, 0x089E6954u)) {
            int32_t v = (int32_t)(*W(SOUND + 156) + *W(SOUND + 160));
            *W(SOUND + 156) = (uint32_t)v;
            if (0x8000 < v) {
                *W(SOUND + 156) = 0x8000;
                call2(c, *W(SOUND + 148), 0x20, f_089E5B4C, 0x089E6988u);
            }
        }
        if (!requested(c, stream, 0x20, 0x089E6994u) && !requested(c, stream, 0x40, 0x089E69A8u)
            && requested(c, stream, 0x10, 0x089E69BCu)) {
            call1(c, 0x683, hle_sceKernelDelayThread, 0x089E6AACu);
        } else {
            ARG(0) = *W(stream);
            ARG(1) = buf + (c->r[18] << 2);
            ARG(2) = sp + 0x19D8;
            ARG(3) = sp + 0x19D0;
            c->r[8] = remain;
            c->r[31] = 0x089E69E0u; hle_sceAtracDecodeData(c);
            if ((int32_t)c->r[2] < 0 && c->r[2] != c->r[30]) {
                call2(c, *W(stream), sp + 0x19DC, hle_sceAtracGetInternalErrorInfo, 0x089E6A00u);
                volume = *W(SOUND + 156);
                break;
            }
            c->r[18] += *W(sp + 0x19D8);
            uint32_t waiting = c->r[18] - c->r[19];
            if (waiting >= 0x800) {
                *W(sp + 32) = c->r[30];
                c->r[30] = waiting & 0xFFFFFFC0u;
                send_block(c, stream, c->r[30], 0xF, 0x089E6A30u, 0x089E6A6Cu);
                c->r[19] += c->r[30];
                c->r[30] = *W(sp + 32);
            }
            if (c->r[18] >= 0x1800) {
                c->r[18] -= c->r[19];
                ARG(1) = buf + (c->r[19] << 2);
                ARG(2) = c->r[18] << 2;
                ARG(0) = buf;
                c->r[31] = 0x089E6A98u; f_089F3430(c);
                c->r[19] = 0;
            }
        }
        if (requested(c, stream, 8, 0x089E6AB8u)) {
            *B(SOUND + 166) = 0xFF;
            *W(SOUND + 156) = 0;
            volume = 0;
            break;
        }
        if (*W(sp + 0x19D0) != 0) { volume = *W(SOUND + 156); break; }
    }
    if (volume != 0 && c->r[19] < c->r[18]) {
        uint32_t waiting = c->r[18] - c->r[19];
        c->r[21] = 0xF;
        ARG(0) = buf + (c->r[18] << 2);
        ARG(2) = (0x800 - waiting) << 2;
        ARG(1) = 0;
        c->r[31] = 0x089E6B10u; f_089F3470(c);
        send_block(c, stream, 0x800, c->r[21], 0x089E6B1Cu, 0x089E6B54u);
    }
    for (;;) {
        ARG(0) = *W(stream + 8);
        c->r[31] = 0x089E6B60u; f_089EB878(c);
        if ((int32_t)c->r[2] <= 0) break;
        call1(c, 0x3E8, hle_sceKernelDelayThread, 0x089E6B70u);
    }
    call1(c, stream, f_089E5BC4, 0x089E6B80u);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[1] = 0x9A20;
    c->r[29] += 0x9A20;
}

/* (path, to, size): a file read whole, waiting for the disc as long as it takes: every failure
   (the disc not ready, the file not opening, the read not starting or failing, or fewer bytes
   read than the 64-bit count at 0x08AD80C8) starts again after a millisecond. 0 when read, the
   memory's cache written back; 1 when the byte at 0x08AD80C2 is set in the meantime. The
   routine at 0x08808418 is told 1 while a read is under way and 0 otherwise. A 0x50 frame
   keeping s0..s8, with the path at +24 and the read's result at +16. */
SFA_FUNC(089E5D04, file_read_blocking) {
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 24) = ARG(0);
    *W(sp + 40) = sv[3]; *W(sp + 48) = sv[5];
    c->r[21] = *W(0x08AD80CCu);
    *W(sp + 32) = sv[1]; *W(sp + 36) = sv[2]; *W(sp + 44) = sv[4]; *W(sp + 52) = sv[6];
    *W(sp + 56) = sv[7]; *W(sp + 60) = sv[8];
    c->r[17] = sp + 0x10;
    c->r[18] = 1;
    c->r[30] = ARG(1);
    c->r[23] = ARG(2);
    c->r[19] = 0x020F0001u;
    c->r[20] = *W(0x08AD80C8u);
    c->r[22] = 0x08AE0000u;
    *W(sp + 28) = sv[0]; *W(sp + 64) = sv[9];
    uint32_t answer;
    for (;;) {
        call1(c, 0, f_08808418, 0x089E5D64u);
        if (*B(0x08AD80C2u) != 0) { answer = c->r[18]; break; }
        call1(c, 0x3E8, hle_sceKernelDelayThread, 0x089E5D78u);
        c->r[31] = 0x089E5D80u; hle_sceUmdGetDriveStat(c);
        if ((c->r[2] & 0x20) == 0) continue;
        ARG(2) = 0;
        call2(c, *W(sp + 24), c->r[19], hle_sceIoOpen, 0x089E5DD4u);
        c->r[16] = c->r[2];
        if ((int32_t)c->r[16] < 0) continue;
        call1(c, 1, f_08808418, 0x089E5DE8u);
        ARG(2) = c->r[23];
        call2(c, c->r[16], c->r[30], hle_sceIoReadAsync, 0x089E5DF8u);
        if (c->r[2] != 0) { call1(c, c->r[16], hle_sceIoClose, 0x089E5E08u); continue; }
        for (;;) {
            call2(c, c->r[16], c->r[17], hle_sceIoPollAsync, 0x089E5E18u);
            if (c->r[2] != c->r[18]) break;
            call1(c, 0x3E8, hle_sceKernelDelayThread, 0x089E5E2Cu);
        }
        if (c->r[2] != 0) { call1(c, c->r[16], hle_sceIoClose, 0x089E5EC8u); continue; }
        uint32_t hi = *W(sp + 20), lo = *W(sp + 16);
        if ((hi == c->r[21] && lo < c->r[20]) || (int32_t)hi < (int32_t)c->r[21]) {
            call1(c, c->r[16], hle_sceIoClose, 0x089E5E6Cu);
            continue;
        }
        call2(c, c->r[30], c->r[23], hle_sceKernelDcacheWritebackRange, 0x089E5E7Cu);
        call1(c, c->r[16], hle_sceIoClose, 0x089E5E84u);
        call1(c, 0, f_08808418, 0x089E5E8Cu);
        answer = 0;
        break;
    }
    c->r[2] = answer;
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[29] += 0x50;
}
