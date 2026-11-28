/* Ten more of the library: the free room on the memory stick, the screen's buffer handed to the
   display, the sound core's volumes and envelopes, and more ends of longer routines. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SAVE     0x08D5C7C0u /* the save dialog's parameters */
#define LIST     0x08E69348u /* points at the drawing list being written; its +8 is the next place */
#define GE       0x08E69300u /* the drawing state */
#define SAS      0x08E73040u /* the sound core the voices belong to */
#define SAS_UP   0x08E73000u /* set once the sound core is started */
#define SAS_DOWN 0x80420100u /* the answer while it is not */

void f_089E3400(Cpu *c); void f_089E7064(Cpu *c); void f_089F0248(Cpu *c);
void hle_sceKernelCpuSuspendIntr(Cpu *c); void hle_sceDisplaySetFrameBuf(Cpu *c);
void hle_sceAudioSetChannelDataLen(Cpu *c); void hle___sceSasSetVolume(Cpu *c);
void hle___sceSasSetADSR(Cpu *c); void hle___sceSasSetADSRmode(Cpu *c);

/* The free room on the memory stick once the dialog's list mode (8) has answered: the two words
   of its block (+1488) multiplied; -1 otherwise. A frame keeping s0, s1. */
SFA_FUNC(089E358C, stick_free_room) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1;
    c->r[16] = SAVE; c->r[17] = 0xFFFFFFFFu;
    *W(sp + 24) = ra;
    ARG(0) = SAVE; c->r[31] = 0x089E35B0u; f_089E3400(c);
    uint32_t room = 0xFFFFFFFFu;
    if (c->r[2] == 0 && *W(SAVE + 48) == 8) {
        gaddr block = *W(SAVE + 1488);
        int64_t m = (int64_t)(int32_t)*W(block + 4) * (int32_t)*W(block);
        c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
        room = c->lo;
    }
    c->r[2] = room;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The end of a routine that finishes a drawing list: how long it got is kept in s1, and with
   interrupts off the list (number at the state's +12, 252 bytes each from +76) hands over to the
   one it names at +324, or to none. */
SFA_FUNC(089E7008, tail_list_finish) {
    c->r[16] = c->r[16] + 0xFFFF9300u;
    gaddr st = c->r[16];
    c->r[14] = *W(st + 72);
    ARG(0) = *W(c->r[14] + 8);
    c->r[13] = *W(c->r[14] + 4);
    c->r[17] = ARG(0) - c->r[13];
    c->r[31] = 0x089E7020u; hle_sceKernelCpuSuspendIntr(c);
    c->r[12] = *W(st + 12);
    c->r[10] = 0xFFFFFFFFu;
    ARG(0) = c->r[2];
    c->r[7] = c->r[12] << 6;
    c->r[11] = c->r[7] - c->r[12];
    c->r[3] = c->r[11] << 2;
    c->r[9] = c->r[3] + st;
    ARG(1) = *W(c->r[9] + 324);
    *W(c->r[9] + 324) = 0xFFFFFFFFu;
    *W(st + 12) = ARG(1);
    if ((int32_t)ARG(1) < 0) *W(st + 72) = 0;
    else {
        c->r[8] = ARG(1) << 6;
        c->r[6] = c->r[8] - ARG(1);
        c->r[24] = c->r[6] << 2;
        c->r[25] = st + 0x4C;
        c->r[15] = c->r[24] + c->r[25];
        *W(st + 72) = c->r[15];
    }
    f_089E7064(c);
}

/* (on): the screen's buffer is shown (the drawing state's +20 and +48 together, its width and
   format from +40 and +36) or, for 0, nothing is. The state's +8 keeps the new setting and the
   old one is returned. A 0x10 frame keeping s0, s1. */
SFA_FUNC(089E7B6C, display_buffer_set) {
    uint32_t on = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1;
    c->r[17] = 0x08E70000u;
    c->r[3] = GE;
    *W(sp) = s0;
    ARG(1) = 0; ARG(2) = 0;
    *W(sp + 8) = ra;
    ARG(3) = 1;
    c->r[16] = on;
    if (on != 0) {
        c->r[8] = *W(GE + 48);
        ARG(2) = *W(GE + 36);
        ARG(1) = *W(GE + 40);
        ARG(0) = *W(GE + 20) + c->r[8];
    }
    c->r[31] = 0x089E7BB8u; hle_sceDisplaySetFrameBuf(c);
    ARG(0) = GE;
    c->r[2] = *W(GE + 8);
    *W(GE + 8) = on;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}

/* (a, b, d, e, f): the drawing list's commands 0xDF with b, sixteen times d and a above them,
   0xE0 with the low 24 bits of e, and 0xE1 with those of f. */
SFA_FUNC(089E8C8C, list_cmd_df_e0_e1) {
    c->r[11] = *W(LIST);
    c->r[3] = ARG(0) << 8;
    c->r[9] = ARG(2) << 4;
    c->r[14] = *W(c->r[11] + 8);
    c->r[10] = c->r[3] | c->r[9];
    ARG(2) = 0x00FFFFFFu;
    c->r[25] = c->r[10] | ARG(1);
    ARG(0) = c->r[14] + 4;
    c->r[9] = c->r[8] & 0x00FFFFFFu;
    c->r[12] = ARG(3) & 0x00FFFFFFu;
    c->r[13] = 0xE0000000u;
    c->r[2] = 0xE1000000u;
    c->r[15] = c->r[25] | 0xDF000000u;
    c->r[10] = c->r[12] | 0xE0000000u;
    ARG(1) = c->r[9] | 0xE1000000u;
    c->r[3] = ARG(0) + 8;
    *W(c->r[14]) = c->r[15];
    *W(c->r[11] + 8) = c->r[3];
    *W(ARG(0)) = c->r[10];
    *W(ARG(0) + 4) = ARG(1);
}

/* (i, length): the i-th of the four sound channels plays `length` samples at a time, a multiple
   of 64 between 64 and 0xFFC0 (else 0x80440011; 0x80440010 past the channels). A 0x10 frame. */
SFA_FUNC(089EB3E8, channel_length_set) {
    uint32_t i = ARG(0), len = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    uint32_t answer = 0x80440010u;
    c->r[8] = i;
    if (i < 4) {
        answer = 0x80440011u;
        ARG(0) = len & 0x3F;
        if (!(0xFF80u < len - 0x40)) {
            c->r[3] = (i << 2) + 0x08E72FB8u;
            if ((len & 0x3F) == 0) {
                ARG(0) = *W(c->r[3]);
                c->r[31] = 0x089EB444u; hle_sceAudioSetChannelDataLen(c);
                answer = c->r[2];
            }
        }
    }
    ARG(2) = answer;
    c->r[2] = answer;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice, left, right, a, b): the voice's volumes, with the two for its effect. A 0x10 frame. */
SFA_FUNC(089ECBF8, voice_volume) {
    uint32_t voice = ARG(0), left = ARG(1), right = ARG(2), a = ARG(3), b = c->r[8];
    uint32_t ra = c->r[31];
    c->r[13] = *W(SAS_UP);
    c->r[10] = voice; c->r[3] = left; c->r[2] = a;
    c->r[29] -= 0x10;
    c->r[11] = 0x80420000u;
    *W(c->r[29]) = ra;
    ARG(0) = SAS; ARG(1) = voice; ARG(2) = left; ARG(3) = right;
    c->r[8] = a; c->r[9] = b;
    c->r[12] = SAS_DOWN;
    if (c->r[13] != 0) {
        c->r[31] = 0x089ECC48u; hle___sceSasSetVolume(c);
        c->r[12] = c->r[2];
    }
    c->r[2] = c->r[12];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice and five words): one of the two calls that set a voice's envelope. A 0x10 frame. */
static void sas_six(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t voice = ARG(0), a = ARG(1), b = ARG(2), d = ARG(3), e = c->r[8], f = c->r[9];
    uint32_t ra = c->r[31];
    c->r[13] = *W(SAS_UP);
    c->r[14] = voice; c->r[2] = d; c->r[3] = e;
    c->r[29] -= 0x10;
    c->r[11] = 0x80420000u; c->r[15] = 0x08E70000u;
    *W(c->r[29]) = ra;
    ARG(0) = SAS; ARG(1) = voice; ARG(2) = a; ARG(3) = b;
    c->r[8] = d; c->r[9] = e; c->r[10] = f;
    c->r[12] = SAS_DOWN;
    if (c->r[13] != 0) {
        c->r[31] = back; fn(c);
        c->r[12] = c->r[2];
    }
    c->r[2] = c->r[12];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice, which, attack, decay, sustain, release): the voice's envelope. */
SFA_FUNC(089ECD54, voice_adsr) {
    sas_six(c, hle___sceSasSetADSR, 0x089ECDACu);
}

/* (voice, which and four modes): how each part of the voice's envelope runs. */
SFA_FUNC(089ECDC0, voice_adsr_mode) {
    sas_six(c, hle___sceSasSetADSRmode, 0x089ECE18u);
}

/* The end of a routine that reads a track's event (the record in s0, the event in s1, its kind
   in r8): the event is kept at +8; but the end of the track (0xFF, 0x2F) with r6 clear starts it
   again from 0x16 past its data (+4), counting its repeats (+56) down unless they are endless
   (+52 at -1), and stops it (+0) when none are left. */
SFA_FUNC(089F035C, tail_track_event) {
    gaddr t = c->r[16];
    c->r[9] = c->r[8] ^ 0x2F;
    c->r[3] = c->r[17] ^ 0xFF;
    ARG(0) = c->r[3] < 1;
    ARG(3) = c->r[9] < 1;
    c->r[8] = ARG(0) & ARG(3);
    if (c->r[8] == 0 || ARG(2) != 0) *B(t + 8) = (uint8_t)c->r[17];
    else {
        c->r[17] = *W(t + 52);
        ARG(0) = 0xFFFFFFFFu;
        if (c->r[17] != 0xFFFFFFFFu) {
            c->r[10] = *W(t + 56);
            ARG(2) = c->r[10] - 1;
            *W(t + 56) = ARG(2);
        }
        c->r[14] = *W(t + 4);
        c->r[11] = *W(t + 56);
        c->r[12] = 1;
        c->r[13] = c->r[14] + 0x16;
        *W(t + 28) = c->r[13];
        *B(t + 8) = 0xFF;
        *B(t + 12) = 1;
        *W(t + 48) = 0;
        *W(t + 60) = 0;
        if (c->r[11] == 0) *B(t) = 0;
    }
    f_089F0248(c);
}

/* The end of a routine that builds a double: the top word from its sign (r12), exponent (r9) and
   the high bits of its fraction (r11), the low word from r10; both are returned from the 0x10
   frame they are put together in. */
SFA_FUNC(089FBCFC, tail_double_pack) {
    gaddr sp = c->r[29];
    ARG(2) = c->r[11] & 0x000FFFFFu;
    c->r[2] = (c->r[2] & 0xFFF00000u) | ARG(2);
    ARG(1) = (c->r[9] & 0x7FF) << 20;
    c->r[2] = (c->r[2] & 0x800FFFFFu) | ARG(1);
    c->r[3] = 0x7FFFFFFFu;
    ARG(0) = c->r[12] << 31;
    c->r[2] = (c->r[2] & 0x7FFFFFFFu) | ARG(0);
    *W(sp + 4) = c->r[2];
    *W(sp) = c->r[10];
    c->r[2] = *W(sp);
    c->r[3] = *W(sp + 4);
    c->r[29] += 0x10;
}
