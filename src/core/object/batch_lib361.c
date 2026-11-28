/* The library's sound voices behind their guard, more commands of the drawing list, and more ends
   of longer routines. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND    0x08E69200u /* the sound driver's state */
#define LIST     0x08E69348u /* points at the drawing list being written; its +8 is the next place */
#define SAS      0x08E73040u /* the sound core the voices belong to */
#define SAS_UP   0x08E73000u /* set once the sound core is started */
#define SAS_DOWN 0x80420100u /* the answer while it is not */

void f_089E5ED0(Cpu *c); void f_089F3470(Cpu *c); void f_089E5B30(Cpu *c); void f_089E7B1C(Cpu *c);
void f_089EAB00(Cpu *c); void f_089EAB30(Cpu *c); void f_089EA538(Cpu *c); void f_089EA5E8(Cpu *c);
void f_089ECEF0(Cpu *c); void f_089F0248(Cpu *c); void f_089F0930(Cpu *c); void f_089F1898(Cpu *c);
void f_089F1CE0(Cpu *c); void f_089F1F48(Cpu *c);
void hle_sceAudioGetChannelRestLength(Cpu *c); void hle___sceSasSetKeyOn(Cpu *c);
void hle___sceSasSetKeyOff(Cpu *c); void hle___sceSasSetPitch(Cpu *c); void hle___sceSasGetEndFlag(Cpu *c);
void hle___sceSasGetEnvelopeHeight(Cpu *c); void hle___sceSasRevVON(Cpu *c); void hle___sceSasRevEVOL(Cpu *c);
void hle___sceSasRevType(Cpu *c); void hle___sceSasRevParam(Cpu *c);

/* (a): one call of the sound core with a, when it is started. A 0x10 frame. */
static void sas_one(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    uint32_t a = ARG(0);
    c->r[6] = *W(SAS_UP);
    c->r[29] -= 0x10;
    c->r[3] = 0x80420000u;
    ARG(1) = a;
    *W(c->r[29]) = ra;
    ARG(0) = SAS;
    c->r[2] = SAS_DOWN;
    if (c->r[6] != 0) { c->r[31] = back; fn(c); }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (a, b): the same with two. */
static void sas_two(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    uint32_t ra = c->r[31];
    uint32_t a = ARG(0), b = ARG(1);
    c->r[8] = *W(SAS_UP);
    c->r[29] -= 0x10;
    c->r[3] = 0x80420000u;
    ARG(2) = b;
    *W(c->r[29]) = ra;
    ARG(1) = a;
    ARG(0) = SAS;
    c->r[7] = SAS_DOWN;
    if (c->r[8] != 0) { c->r[31] = back; fn(c); c->r[7] = c->r[2]; }
    c->r[2] = c->r[7];
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The sound's buffer is taken and, when there is one, cleared and kept at SOUND + 128. */
SFA_FUNC(089E5F04, sound_buffer_ready) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[31] = 0x089E5F18u; f_089E5ED0(c);
    c->r[16] = 0x08AD0000u;
    gaddr buf = *W(0x08AD7F24u);
    ARG(2) = 0xD0000;
    if (buf != 0) {
        ARG(0) = buf; ARG(1) = 0; ARG(2) = 0xC8002;
        c->r[31] = 0x089E5F34u; f_089F3470(c);
        *W(SOUND + 128) = *W(0x08AD7F24u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* Unless the byte at SOUND + 164 is set, the sound thread's bit 8 and 0xFF at SOUND + 166. */
SFA_FUNC(089E64BC, sound_flag_8_unless_164) {
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = SOUND;
    uint32_t held = *B(SOUND + 164);
    *W(c->r[29] + 20) = ra;
    if (held == 0) {
        ARG(0) = *W(SOUND + 148); ARG(1) = 8;
        c->r[31] = 0x089E64E8u; f_089E5B30(c);
        *B(SOUND + 166) = 0xFF;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The end of a 0x10 frame with its return address at its start. */
SFA_FUNC(089E7B18, tail_e7b18) {
    c->r[31] = *W(c->r[29]);
    f_089E7B1C(c);
}

/* The end of a routine that swaps the buffers: the word at 0x08E69C38 is turned over, the record
   in s0 gives its +44 and +40 to 0x08E71C4C, and its +44 is returned from a 0x10 frame that kept
   s0. */
SFA_FUNC(089E7C88, tail_buffers_swap) {
    c->r[31] = 0x089E7C90u; f_089EAB00(c);
    gaddr rec = c->r[16];
    ARG(0) = *W(rec + 36); ARG(1) = *W(rec + 44); ARG(2) = *W(rec + 40);
    c->r[31] = 0x089E7CA0u; f_089EAB30(c);
    c->r[2] = *W(rec + 44);
    c->r[31] = *W(c->r[29] + 4);
    c->r[16] = *W(c->r[29]);
    c->r[29] += 0x10;
}

/* (a, b): the drawing list's commands 0x4C with sixteen times a and 0x4D with sixteen times b. */
SFA_FUNC(089E843C, list_cmd_4c_4d) {
    c->r[13] = 0x08E70000u;
    c->r[9] = *W(LIST);
    c->r[11] = ARG(0) << 4;
    c->r[12] = 0x4C000000u;
    c->r[10] = *W(c->r[9] + 8);
    c->r[3] = ARG(1) << 4;
    c->r[7] = 0x4D000000u;
    ARG(0) = c->r[10] + 4;
    c->r[6] = c->r[11] | c->r[12];
    c->r[8] = c->r[3] | c->r[7];
    c->r[2] = ARG(0) + 4;
    *W(c->r[10]) = c->r[6];
    *W(c->r[9] + 8) = c->r[2];
    *W(ARG(0)) = c->r[8];
}

/* (a, b): the drawing list's command 0xC9 with a, b above it and the list's +96 above that; a and
   b are kept at the list's +100 and +104. */
SFA_FUNC(089E87A8, list_cmd_c9) {
    c->r[14] = 0x08E70000u;
    c->r[8] = *W(LIST);
    c->r[13] = ARG(1) << 8;
    c->r[10] = 0xC9000000u;
    c->r[3] = *W(c->r[8] + 96);
    c->r[7] = *W(c->r[8] + 8);
    *W(c->r[8] + 104) = ARG(1);
    c->r[12] = c->r[3] << 16;
    c->r[11] = c->r[12] | c->r[13];
    c->r[9] = c->r[11] | ARG(0);
    c->r[6] = c->r[9] | c->r[10];
    c->r[2] = c->r[7] + 4;
    *W(c->r[8] + 100) = ARG(0);
    *W(c->r[8] + 8) = c->r[2];
    *W(c->r[7]) = c->r[6];
}

/* (a, b, d, e): the drawing list's command 0xC5 with a, four times b, d and e. */
SFA_FUNC(089E89EC, list_cmd_c5) {
    c->r[14] = 0x08E70000u;
    c->r[9] = *W(LIST);
    c->r[13] = ARG(2) << 8;
    c->r[12] = ARG(3) << 16;
    c->r[11] = ARG(1) << 2;
    c->r[10] = c->r[12] | c->r[13];
    ARG(1) = *W(c->r[9] + 8);
    c->r[8] = c->r[10] | c->r[11];
    ARG(2) = c->r[8] | ARG(0);
    ARG(0) = 0xC5000000u;
    c->r[3] = ARG(2) | ARG(0);
    c->r[2] = ARG(1) + 4;
    *W(ARG(1)) = c->r[3];
    *W(c->r[9] + 8) = c->r[2];
}

/* (a, b, d): the drawing list's command 0xDB with a and the bytes b and d. */
SFA_FUNC(089E8AF8, list_cmd_db) {
    c->r[14] = 0x08E70000u;
    c->r[8] = *W(LIST);
    c->r[12] = ARG(1) & 0xFF;
    c->r[13] = ARG(2) & 0xFF;
    ARG(1) = *W(c->r[8] + 8);
    c->r[10] = c->r[13] << 16;
    c->r[11] = c->r[12] << 8;
    c->r[9] = c->r[10] | c->r[11];
    c->r[7] = c->r[9] | ARG(0);
    ARG(0) = 0xDB000000u;
    c->r[3] = c->r[7] | ARG(0);
    c->r[2] = ARG(1) + 4;
    *W(ARG(1)) = c->r[3];
    *W(c->r[8] + 8) = c->r[2];
}

/* (list, b, d, e, f): three commands on the list given: 0xC2 with f, e and d, 0xC3 with b, and
   0xCB. */
SFA_FUNC(089EA460, list_cmd_c2_c3_cb) {
    gaddr list = ARG(0);
    c->r[10] = *W(list + 8);
    c->r[14] = ARG(2) << 16;
    c->r[15] = ARG(3) << 8;
    c->r[13] = c->r[14] | c->r[15];
    ARG(2) = c->r[10] + 4;
    c->r[11] = c->r[13] | c->r[8];
    c->r[12] = 0xC2000000u;
    c->r[9] = 0xC3000000u;
    ARG(3) = c->r[11] | c->r[12];
    c->r[8] = ARG(1) | c->r[9];
    c->r[2] = ARG(2) + 8;
    c->r[3] = 0xCB000000u;
    *W(c->r[10]) = ARG(3);
    *W(list + 8) = c->r[2];
    *W(ARG(2)) = c->r[8];
    *W(ARG(2) + 4) = c->r[3];
}

/* (list, on): the command 0x50 with 1 or 0 on the list given. */
SFA_FUNC(089EA524, list_cmd_50) {
    uint32_t on = ARG(1);
    c->r[3] = 0x50000001u;
    c->r[2] = *W(ARG(0) + 8);
    if (on == 0) c->r[3] = 0x50000000u;
    f_089EA538(c);
}

/* (a, record): the record's routine (+4), when it has one, runs with the low sixteen bits of
   a. A 0x10 frame. */
SFA_FUNC(089EA5D0, record_routine_run) {
    uint32_t fn = *W(ARG(1) + 4);
    c->r[2] = fn;
    c->r[29] -= 0x10;
    *W(c->r[29]) = c->r[31];
    ARG(0) = ARG(0) & 0xFFFF;
    if (fn != 0) { c->r[31] = 0x089EA5F8u; rt_call_indirect(c, fn); }
    c->r[31] = *W(c->r[29]);
    f_089EA5E8(c);
}

/* (i): how much the i-th of the four sound channels (handles at 0x08E72FB8) still has to play, or
   0x80440010 past them. A 0x10 frame. */
SFA_FUNC(089EB878, channel_rest) {
    uint32_t i = ARG(0);
    uint32_t ra = c->r[31];
    c->r[7] = 0x08E72FB8u;
    c->r[29] -= 0x10;
    c->r[3] = (i << 2) + 0x08E72FB8u;
    c->r[6] = 0x80440000u;
    ARG(1) = i < 4;
    *W(c->r[29]) = ra;
    c->r[2] = 0x80440010u;
    if (i < 4) {
        ARG(0) = *W(c->r[3]);
        c->r[31] = 0x089EB8A8u; hle_sceAudioGetChannelRestLength(c);
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (voice): the voice starts. */
SFA_FUNC(089ECAA0, voice_key_on) {
    sas_one(c, hle___sceSasSetKeyOn, 0x089ECAD0u);
}

/* (voice): the voice stops. */
SFA_FUNC(089ECADC, voice_key_off) {
    sas_one(c, hle___sceSasSetKeyOff, 0x089ECB0Cu);
}

/* (voice, pitch): the voice's pitch; 0x80420012 for a pitch of 0. A 0x10 frame. */
SFA_FUNC(089ECC5C, voice_pitch) {
    uint32_t voice = ARG(0), pitch = ARG(1);
    uint32_t ra = c->r[31];
    c->r[6] = *W(SAS_UP);
    c->r[29] -= 0x10;
    c->r[3] = 0x80420000u;
    *W(c->r[29]) = ra;
    c->r[7] = pitch;
    c->r[2] = SAS_DOWN;
    if (c->r[6] != 0) {
        c->r[8] = 0x80420000u;
        ARG(1) = voice; ARG(2) = pitch; ARG(0) = SAS;
        c->r[2] = 0x80420012u;
        if (pitch != 0) { c->r[31] = 0x089ECCA0u; hle___sceSasSetPitch(c); }
    }
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The voices that have ended, one bit each; 0 while the sound core is not started. */
SFA_FUNC(089ECECC, voices_ended) {
    ARG(1) = *W(SAS_UP);
    c->r[3] = 0x08E70000u;
    c->r[29] -= 0x10;
    ARG(0) = SAS;
    *W(c->r[29]) = c->r[31];
    c->r[2] = 0;
    if (ARG(1) != 0) { c->r[31] = 0x089ECF00u; hle___sceSasGetEndFlag(c); }
    c->r[31] = *W(c->r[29]);
    f_089ECEF0(c);
}

/* (voice): the height of the voice's envelope. */
SFA_FUNC(089ECF08, voice_envelope) {
    sas_one(c, hle___sceSasGetEnvelopeHeight, 0x089ECF38u);
}

/* (a, b): the reverb's two switches. */
SFA_FUNC(089ECF44, reverb_von) {
    sas_two(c, hle___sceSasRevVON, 0x089ECF7Cu);
}

/* (a, b): the reverb's two volumes. */
SFA_FUNC(089ECF90, reverb_evol) {
    sas_two(c, hle___sceSasRevEVOL, 0x089ECFC8u);
}

/* (type): the reverb's kind. */
SFA_FUNC(089ECFDC, reverb_type) {
    sas_one(c, hle___sceSasRevType, 0x089ED00Cu);
}

/* (a, b): the reverb's delay and feedback. */
SFA_FUNC(089ED018, reverb_param) {
    sas_two(c, hle___sceSasRevParam, 0x089ED050u);
}

/* (p): p is kept at 0x08E74304 and the 0x80 words listed at 0x08E74308 are cleared. Returns 0, or
   -1 without p. */
SFA_FUNC(089EFE9C, records_list_start) {
    uint32_t p = ARG(0);
    c->r[2] = 0xFFFFFFFFu;
    if (p != 0) {
        *W(0x08E74304u) = p;
        for (uint32_t i = 0; i < 0x80; i++) *W(0x08E74308u + 4 * i) = 0;
        c->r[3] = 0x08E74308u + 0x200;
        ARG(0) = 0x80; ARG(1) = 0;
        c->r[2] = 0;
    }
}

/* The end of a routine: s1 goes to +8 of the record in s0, then the frame's end. */
SFA_FUNC(089F0244, tail_f0244) {
    *B(c->r[16] + 8) = (uint8_t)c->r[17];
    f_089F0248(c);
}

/* The end of a 0x20 frame with its return address at +28. */
SFA_FUNC(089F092C, tail_f092c) {
    c->r[31] = *W(c->r[29] + 28);
    f_089F0930(c);
}

/* The end of a routine: two words are copied from the place in r6 to the one in r7 and 0 is
   returned. */
SFA_FUNC(089F1910, tail_copy_two_words) {
    c->r[2] = *W(c->r[6]);
    c->r[3] = *W(c->r[6] + 4);
    *W(c->r[7]) = c->r[2];
    *W(c->r[7] + 4) = c->r[3];
    c->r[6] = 0;
    f_089F1898(c);
}

/* The end of a routine: r2 goes to +1 of the place in r3 and 0 is returned. */
SFA_FUNC(089F1D3C, tail_f1d3c) {
    *B(c->r[3] + 1) = (uint8_t)c->r[2];
    f_089F1CE0(c);
}

/* The end of a routine that returns 0. */
SFA_FUNC(089F1F44, tail_f1f44) {
    c->r[6] = 0;
    f_089F1F48(c);
}
