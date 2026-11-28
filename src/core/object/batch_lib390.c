/* The sound's start and the music stream's loading: the output channels reserved and let go, the
   sequences shut down, the whole sound system started, and a music file read and handed to the
   decoder. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND      0x08E69200u /* the sound's state; it begins with the music file's path */
#define OUT_READY  0x08E71C5Cu /* 1 once the output channels are reserved */
#define OUT_MAIN   0x08E72FA8u /* the main output channel's number */
#define OUT_EXTRA  0x08E72FB8u /* three more channels' numbers */
#define SEQ_READY  0x08AD80D0u /* -1 until the sequences are made ready */

void f_089EAC54(Cpu *c); void f_089E5F04(Cpu *c); void f_089E5D04(Cpu *c); void f_089F3470(Cpu *c);
void f_089E575C(Cpu *c); void f_089E5968(Cpu *c); void f_089E5ED0(Cpu *c); void f_089EB3E8(Cpu *c);
void f_089ED320(Cpu *c); void f_089EDF0C(Cpu *c); void f_089ECFDC(Cpu *c); void f_089ECF44(Cpu *c);
void f_089ECF90(Cpu *c); void f_089ED9EC(Cpu *c);
void hle_sceAudioChReserve(Cpu *c); void hle_sceAudioChRelease(Cpu *c);
void hle_sceKernelExitThread(Cpu *c); void hle_sceAtracSetDataAndGetID(Cpu *c);
void hle_sceAtracGetSecondBufferInfo(Cpu *c); void hle_sceAtracGetSoundSample(Cpu *c);
void hle_sceAtracReleaseAtracID(Cpu *c); void hle_sceAtracSetLoopNum(Cpu *c);
void hle_sceKernelCreateEventFlag(Cpu *c); void hle_sceKernelCreateThread(Cpu *c);
void hle_sceKernelStartThread(Cpu *c); void hle_sceKernelDeleteEventFlag(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* The end of the channels' reserving when one could not be had: the channel in r4 is let go, and
   0x80440001 returned. */
SFA_FUNC(089EAC88, tail_channel_release) {
    c->r[31] = 0x089EAC90u; hle_sceAudioChRelease(c);
    c->r[15] = 0x80440000u;
    c->r[3] = 0x80440001u;
    f_089EAC54(c);
}

/* The output channels are reserved, once: the two buffers' places noted (0x08E72FB0 and
   0x08E72FB4), the two blocks of 0x260 bytes at 0x08E72AE4 and 0x08E72D44 cleared, and four
   channels reserved, the main one of 0x1C0 samples and three of 0x100. When one cannot be had
   those reserved so far are let go. 0, or 0x80440001. A 0x20 frame keeping s0..s3, which the
   end closes. */
SFA_FUNC(089EAB44, output_channels_reserve) {
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 12) = c->r[19];
    c->r[19] = 0x08E70000u;
    c->r[2] = 0x80440000u;
    ARG(0) = *W(OUT_READY);
    *W(sp + 16) = c->r[31];
    c->r[3] = 0x80440001u;
    *W(sp + 8) = c->r[18]; *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
    if (ARG(0) != 0) { f_089EAC54(c); return; }
    *W(0x08E72FB4u) = 0x08E71C64u;
    *W(0x08E72FB0u) = 0x08E71CD4u;
    for (uint32_t i = 0; i < 0x260; i++) *B(0x08E72AE4u + i) = 0;
    for (uint32_t i = 0; i < 0x260; i++) *B(0x08E72D44u + i) = 0;
    c->r[17] = OUT_MAIN - 4;
    call3(c, 0xFFFFFFFFu, 0x1C0, 0, hle_sceAudioChReserve, 0x089EABE8u);
    *W(OUT_MAIN) = c->r[2];
    c->r[3] = 0x80440001u;
    if ((int32_t)c->r[2] < 0) { f_089EAC54(c); return; }
    c->r[16] = 0x08E70000u;
    call3(c, 0xFFFFFFFFu, 0x100, 0, hle_sceAudioChReserve, 0x089EAC0Cu);
    *W(OUT_EXTRA) = c->r[2];
    c->r[18] = OUT_EXTRA;
    if ((int32_t)c->r[2] < 0) { ARG(0) = *W(OUT_MAIN); f_089EAC88(c); return; }
    call3(c, 0xFFFFFFFFu, 0x100, 0, hle_sceAudioChReserve, 0x089EAC28u);
    *W(OUT_EXTRA + 4) = c->r[2];
    if ((int32_t)c->r[2] < 0) {
        call1(c, *W(OUT_MAIN), hle_sceAudioChRelease, 0x089EACA4u);
        ARG(0) = *W(OUT_EXTRA);
        f_089EAC88(c);
        return;
    }
    call3(c, 0xFFFFFFFFu, 0x100, 0, hle_sceAudioChReserve, 0x089EAC40u);
    *W(OUT_EXTRA + 8) = c->r[2];
    if ((int32_t)c->r[2] < 0) {
        call1(c, *W(OUT_MAIN), hle_sceAudioChRelease, 0x089EAC7Cu);
        call1(c, *W(OUT_EXTRA), hle_sceAudioChRelease, 0x089EAC84u);
        ARG(0) = *W(OUT_EXTRA + 4);
        f_089EAC88(c);
        return;
    }
    c->r[16] = 1;
    *W(OUT_READY) = 1;
    c->r[3] = 0;
    f_089EAC54(c);
}

/* The output channels are let go: 0, or 0x80440002 when they are not reserved. A 0x10 frame
   keeping s0, s1. */
SFA_FUNC(089EB67C, output_channels_release) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = 0x08E70000u;
    c->r[3] = 0x80440000u;
    ARG(1) = 1;
    *W(sp + 8) = ra;
    ARG(0) = 0x80440002u;
    *W(sp) = s0;
    if (*W(OUT_READY) == 1) {
        call1(c, *W(OUT_MAIN), hle_sceAudioChRelease, 0x089EB6B0u);
        c->r[16] = OUT_EXTRA;
        call1(c, *W(OUT_EXTRA), hle_sceAudioChRelease, 0x089EB6C0u);
        call1(c, *W(OUT_EXTRA + 4), hle_sceAudioChRelease, 0x089EB6C8u);
        call1(c, *W(OUT_EXTRA + 8), hle_sceAudioChRelease, 0x089EB6D0u);
        *W(0x08E72FB4u) = 0;
        *W(OUT_READY) = 0;
        *W(0x08E72FB0u) = 0;
        ARG(0) = 0;
    }
    c->r[2] = ARG(0);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}

/* The sound core's started flag is cleared: 0, or 0x80420100 when it was not started. */
SFA_FUNC(089ED080, sound_core_stop) {
    ARG(1) = 0x08E70000u;
    c->r[3] = 0x80420000u;
    c->r[6] = 1;
    ARG(0) = 0x80420100u;
    if (*W(0x08E73000u) == 1) {
        *W(0x08E73000u) = 0;
        ARG(0) = 0;
    }
    c->r[2] = ARG(0);
}

/* The sequences are shut down: 0x80450001 when they were never made ready, else the sound core
   is stopped (its error returned), and they are marked not ready. A 0x10 frame keeping s0,
   s1. */
SFA_FUNC(089EDC34, sequences_shutdown) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = 0x08AE0000u;
    ARG(0) = *W(SEQ_READY);
    *W(sp) = s0;
    c->r[16] = 0xFFFFFFFFu;
    c->r[3] = 0x80450001u;
    *W(sp + 8) = ra;
    if (ARG(0) != 0xFFFFFFFFu) {
        c->r[31] = 0x089EDC64u; f_089ED080(c);
        c->r[3] = c->r[2];
        if (c->r[2] == 0) {
            *W(SEQ_READY) = 0xFFFFFFFFu;
            c->r[3] = 0;
        }
    }
    c->r[2] = c->r[3];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}

/* What the sound's start undoes when it fails after the sequences are ready. */
static void start_undo(Cpu *c, uint32_t back) {
    c->r[31] = back; f_089EDC34(c);
    c->r[31] = back + 8; f_089EB67C(c);
}

/* The sound system is started: its tables cleared, the output channels reserved (the main one
   set to 0x400), the sequences made ready for 32 voices, voices 0 to 15 and 24 to 31 set up, the
   core's settings given, and the music stream's event flag (kept at 0x08E692B4) and its thread
   made and started. 0, or -1 when something fails, with what was made undone. The word at
   0x08E692B0 is 1 while the sound runs. A 0x50 frame keeping s0..s2, with a block of eight words
   for the core at +32. */
SFA_FUNC(089E6628, sound_start) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 68) = s1;
    c->r[17] = 0x08E70000u;
    *W(0x08E692B0u) = 1;
    *W(sp + 64) = s0; *W(sp + 72) = s2; *W(sp + 76) = ra;
    call3(c, 0x08E67200u, 0, 0x2000, f_089F3470, 0x089E665Cu);
    c->r[31] = 0x089E6664u; f_089E575C(c);
    c->r[31] = 0x089E666Cu; f_089E5968(c);
    call3(c, SOUND, 0, 0xB0, f_089F3470, 0x089E6680u);
    ARG(0) = 0x08AD0000u;
    *W(0x08AD7F24u) = 0;
    c->r[31] = 0x089E668Cu; f_089E5ED0(c);
    c->r[31] = 0x089E6694u; f_089EAB44(c);
    call2(c, 0, 0x400, f_089EB3E8, 0x089E66A0u);
    call1(c, 0x20, f_089ED320, 0x089E66A8u);
    c->r[18] = 0;
    uint32_t answer = 0xFFFFFFFFu;
    if (c->r[2] == 0) {
        c->r[16] = 0x08E70000u;
        for (; (int32_t)c->r[18] < 16; c->r[18]++) call2(c, c->r[18], 0, f_089EDF0C, 0x089E66E0u);
        for (c->r[18] = 0x18; (int32_t)c->r[18] < 32; c->r[18]++) call2(c, c->r[18], 0, f_089EDF0C, 0x089E6700u);
        call1(c, 4, f_089ECFDC, 0x089E6718u);
        call2(c, 1, 0, f_089ECF44, 0x089E6724u);
        call2(c, 0xC00, 0xC00, f_089ECF90, 0x089E6730u);
        *W(sp + 32) = 0xFFFFFFFFu;
        *W(sp + 36) = 4;
        *W(sp + 40) = 0;
        *W(sp + 44) = 0;
        *W(sp + 48) = 0xC00;
        *W(sp + 52) = 0xC00;
        *W(sp + 56) = 1;
        *W(sp + 60) = 0;
        call1(c, sp + 0x20, f_089ED9EC, 0x089E6768u);
        ARG(3) = 0;
        call3(c, 0x08A020A4u, 0x200, 0, hle_sceKernelCreateEventFlag, 0x089E6780u);
        *W(0x08E692B4u) = c->r[2];
        if ((int32_t)c->r[2] < 0) {
            *W(0x08E692B0u) = 0;
            start_undo(c, 0x089E6840u);
        } else {
            c->r[8] = 0; c->r[9] = 0;
            ARG(3) = 0x10000;
            call3(c, 0x08A020B4u, 0x089E5BECu, 0x10, hle_sceKernelCreateThread, 0x089E67ACu);
            ARG(0) = c->r[2];
            ARG(1) = 0;
            if ((int32_t)ARG(0) < 0) {
                ARG(0) = *W(0x08E692B4u);
                *W(0x08E692B0u) = 0;
                c->r[31] = 0x089E680Cu; hle_sceKernelDeleteEventFlag(c);
                start_undo(c, 0x089E6814u);
            } else {
                ARG(2) = 0;
                c->r[31] = 0x089E67C0u; hle_sceKernelStartThread(c);
                if (c->r[2] == 0) answer = 0;
                else {
                    ARG(0) = *W(0x08E692B4u);
                    *W(0x08E692B0u) = 0;
                    c->r[31] = 0x089E67D4u; hle_sceKernelDeleteEventFlag(c);
                    start_undo(c, 0x089E67DCu);
                }
            }
        }
    }
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x50;
}

/* The thread that reads a music file ahead of time: the file named at the sound's start is read
   to the stream's buffer (+128, of +132 bytes), and +165 becomes 2 when it is there, else 0;
   then the thread ends. A 0x20 frame. */
SFA_FUNC(089E5FB8, music_preload_thread) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = ra;
    c->r[31] = 0x089E5FC8u; f_089E5F04(c);
    *W(sp + 16) = SOUND;
    call3(c, SOUND, *W(SOUND + 128), *W(SOUND + 132), f_089E5D04, 0x089E5FE0u);
    ARG(0) = *W(sp + 16);
    if ((int32_t)c->r[2] >= 0) { ARG(1) = 2; *B(ARG(0) + 165) = 2; }
    else *B(ARG(0) + 165) = 0;
    call1(c, 0, hle_sceKernelExitThread, 0x089E6000u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (stream): a music stream is opened: its decoder's number (+0, -1 until there is one), the
   sound's event flag (+4) and channel (+8). Unless the file was read ahead (+165 of the sound's
   state) it is read now. The data goes to the decoder, which must say it has no second buffer
   (0x80630022) and give the length in samples (to +12); it is then set to loop for ever. 0, or
   -1 with the decoder let go. A 0x30 frame keeping s0, s1. */
SFA_FUNC(089E6130, music_stream_open) {
    gaddr stream = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s0;
    c->r[16] = stream;
    *W(sp + 40) = s1;
    c->r[17] = 0xFFFFFFFFu;
    *W(stream) = 0xFFFFFFFFu;
    *W(stream + 4) = *W(SOUND + 148);
    *W(stream + 8) = *W(SOUND + 144);
    ARG(1) = *B(SOUND + 165);
    *W(sp + 44) = ra;
    uint32_t answer = 0xFFFFFFFFu;
    do {
        ARG(0) = SOUND;
        if (ARG(1) == 0) {
            *W(sp + 32) = SOUND;
            c->r[31] = 0x089E6178u; f_089E5F04(c);
            gaddr s = *W(sp + 32);
            call3(c, s, *W(s + 128), *W(s + 132), f_089E5D04, 0x089E6188u);
            ARG(0) = *W(sp + 32);
            if (c->r[2] != 0) break;
        }
        gaddr s = ARG(0);
        uint32_t data = *W(s + 128), size = *W(s + 132);
        ARG(2) = size;
        *B(s + 165) = 0;
        call2(c, data, size, hle_sceAtracSetDataAndGetID, 0x089E61A8u);
        *W(stream) = c->r[2];
        call3(c, c->r[2], sp + 0x18, sp + 0x1C, hle_sceAtracGetSecondBufferInfo, 0x089E61BCu);
        ARG(1) = 0x80630022u;
        ARG(0) = *W(stream);
        if (c->r[2] != 0x80630022u) {
            c->r[31] = 0x089E620Cu; hle_sceAtracReleaseAtracID(c);
            break;
        }
        ARG(3) = sp + 0x14;
        call3(c, ARG(0), stream + 0xC, sp + 0x10, hle_sceAtracGetSoundSample, 0x089E61DCu);
        ARG(0) = *W(stream);
        if ((int32_t)c->r[2] < 0) {
            c->r[31] = 0x089E61ECu; hle_sceAtracReleaseAtracID(c);
            break;
        }
        ARG(1) = 0xFFFFFFFFu;
        c->r[31] = 0x089E622Cu; hle_sceAtracSetLoopNum(c);
        answer = 0;
    } while (0);
    c->r[2] = answer;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}
