/* Eight more of the library: the drawing hardware set up, the drawing list finished, voices let
   go or held by a mask, the reverb set from a record, a sequence loaded, and a note's pitch and
   volume worked out. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE       0x08E69300u /* the drawing state */
#define VOICES   0x08E74534u /* the sound's voices, 0x68 bytes each */
#define VOICE_N  0x08E74080u /* how many voices the masks below go over */

void f_089E6F70(Cpu *c); void f_089F1C68(Cpu *c); void f_089ED778(Cpu *c); void f_089F1E98(Cpu *c);
void f_089ED860(Cpu *c); void f_089ECFDC(Cpu *c); void f_089EDA4C(Cpu *c); void f_089EDA24(Cpu *c);
void f_089F0104(Cpu *c); void f_089F0554(Cpu *c); void f_089EF2B0(Cpu *c); void f_089EEC08(Cpu *c);
void f_089F2ABC(Cpu *c); void f_089EA600(Cpu *c); void f_089EAAD4(Cpu *c); void f_089E6CA4(Cpu *c);
void hle_sceGeEdramGetAddr(Cpu *c); void hle_sceKernelCreateEventFlag(Cpu *c);
void hle_sceKernelDeleteEventFlag(Cpu *c); void hle_sceGeSetCallback(Cpu *c);
void hle_sceGeUnsetCallback(Cpu *c); void hle_sceGeListEnQueue(Cpu *c); void hle_sceGeListSync(Cpu *c);
void hle_sceGeDrawSync(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }

/* The drawing hardware is set up, once (the drawing state's first word says so; 0x80000020 the
   second time): the video memory's address is kept (+20), the state reset, an event flag made
   (kept at 0x08E692FC), the two callbacks 0x089EA550 and 0x089EA5D0 registered (the number at
   +32), and the empty list at 0x08A020E8 queued (its number at +24) and waited for. A failure
   undoes what was made and is returned. A 0x30 frame keeping s0..s3. */
SFA_FUNC(089E6BB8, draw_hardware_start) {
    const gaddr own = 0x08E692D0u;
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = c->r[18];
    c->r[18] = 0x08E70000u;
    ARG(0) = *W(GE);
    *W(sp + 28) = c->r[19];
    c->r[3] = 0x80000020u;
    c->r[19] = GE;
    *W(sp + 32) = c->r[31];
    *W(sp + 20) = c->r[17];
    *W(sp + 16) = c->r[16];
    if (ARG(0) == 0) {
        call0(c, hle_sceGeEdramGetAddr, 0x089E6BF0u);
        *W(GE + 20) = c->r[2];
        call0(c, f_089EA600, 0x089E6BF8u);
        call0(c, f_089EAAD4, 0x089E6C00u);
        ARG(1) = 0x200; ARG(2) = 1; ARG(3) = 0;
        call1(c, 0x08A02470u, hle_sceKernelCreateEventFlag, 0x089E6C18u);
        c->r[17] = own;
        c->r[3] = c->r[2];
        if ((int32_t)c->r[2] >= 0) {
            *W(sp + 8) = 0x089EA5D0u;
            *W(own + 44) = c->r[2];
            *W(sp) = 0x089EA550u;
            *W(sp + 4) = own;
            *W(sp + 12) = own;
            call1(c, sp, hle_sceGeSetCallback, 0x089E6C54u);
            c->r[16] = c->r[2];
            if ((int32_t)c->r[2] < 0) {
                call1(c, *W(own + 44), hle_sceKernelDeleteEventFlag, 0x089E6CE4u);
                c->r[3] = c->r[16];
            } else {
                *W(GE + 32) = c->r[2];
                ARG(1) = 0; ARG(2) = 0xFFFFFFFFu; ARG(3) = 0;
                call1(c, 0x08A020E8u, hle_sceGeListEnQueue, 0x089E6C78u);
                c->r[16] = c->r[2];
                if ((int32_t)c->r[2] < 0) {
                    call1(c, *W(own + 44), hle_sceKernelDeleteEventFlag, 0x089E6CCCu);
                    call1(c, *W(GE + 32), hle_sceGeUnsetCallback, 0x089E6CD4u);
                    c->r[3] = c->r[16];
                } else {
                    *W(GE + 24) = c->r[2];
                    ARG(1) = 0;
                    call1(c, c->r[2], hle_sceGeListSync, 0x089E6C90u);
                    call1(c, 0, hle_sceGeDrawSync, 0x089E6C98u);
                    *W(GE) = 1;
                    c->r[3] = 0;
                }
            }
        }
    }
    f_089E6CA4(c);
}

/* The drawing list in use is finished, with 0. A 0x10 frame. */
SFA_FUNC(089E7894, list_finish_0) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    call1(c, 0, f_089E6F70, 0x089E78A4u);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* The rest of a loop over the voices (s0 the one before the next; the count in the word at
   s3 + 0x4080): each whose bit is set in s1 is let go, and the bits (s4 shifted) of those that
   answer 0 are gathered in s2; the frame's end returns them. */
SFA_FUNC(089ED7D4, voices_let_go_by_mask) {
    for (;;) {
        c->r[13] = *W(c->r[19] + 16512);
        c->r[16] = c->r[16] + 1;
        c->r[8] = c->r[16] < c->r[13];
        if (c->r[8] == 0) break;
        c->r[7] = c->r[17] >> (c->r[16] & 31);
        ARG(1) = c->r[7] & 1;
        ARG(0) = c->r[16];
        if (ARG(1) == 0) continue;
        c->r[31] = 0x089ED7F8u; f_089F1C68(c);
        c->r[3] = c->r[20] << (c->r[16] & 31);
        if (c->r[2] == 0) c->r[18] |= c->r[3];
    }
    ARG(0) = c->r[18];
    f_089ED778(c);
}

/* The same with the hold of each voice whose bit is set in s3 set to s1 (the count at
   s4 + 0x4080, the bit from s5). */
SFA_FUNC(089ED8C0, voices_hold_by_mask) {
    for (;;) {
        c->r[11] = *W(c->r[20] + 16512);
        c->r[16] = c->r[16] + 1;
        ARG(1) = c->r[16] < c->r[11];
        c->r[10] = c->r[19] >> (c->r[16] & 31);
        if (ARG(1) == 0) break;
        ARG(2) = c->r[10] & 1;
        ARG(0) = c->r[16];
        ARG(1) = c->r[17];
        if (ARG(2) == 0) continue;
        c->r[31] = 0x089ED8E4u; f_089F1E98(c);
        c->r[3] = c->r[21] << (c->r[16] & 31);
        if (c->r[2] == 0) c->r[18] |= c->r[3];
    }
    ARG(2) = c->r[18];
    f_089ED860(c);
}

/* The same as the first, with the frame's end its own: a 0x20 frame that kept s0..s4. */
SFA_FUNC(089ED95C, voices_let_go_by_mask_b) {
    for (;;) {
        c->r[9] = *W(c->r[19] + 16512);
        c->r[16] = c->r[16] + 1;
        c->r[8] = c->r[16] < c->r[9];
        ARG(2) = c->r[17] >> (c->r[16] & 31);
        if (c->r[8] == 0) break;
        ARG(1) = ARG(2) & 1;
        ARG(0) = c->r[16];
        if (ARG(1) == 0) continue;
        c->r[31] = 0x089ED9A0u; f_089F1C68(c);
        c->r[3] = c->r[20] << (c->r[16] & 31);
        if (c->r[2] == 0) c->r[18] |= c->r[3];
    }
    gaddr sp = c->r[29];
    ARG(1) = c->r[18];
    c->r[31] = *W(sp + 20);
    c->r[20] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = ARG(1);
    c->r[29] += 0x20;
}

/* (record): the reverb set from a record by the bits of its first word: 1 the kind (+4), then
   the delay and feedback, the volumes and the switches in their own ends. 0x80450001 before the
   sound is started; a failing call is the answer. A 0x10 frame keeping s0. */
SFA_FUNC(089ED9EC, reverb_set) {
    gaddr rec = ARG(0);
    ARG(2) = *W(0x08AD80D0u);
    c->r[29] -= 0x10;
    *W(c->r[29]) = c->r[16];
    c->r[16] = rec;
    *W(c->r[29] + 4) = c->r[31];
    ARG(1) = 0x80450001u;
    if (ARG(2) == 0xFFFFFFFFu) { f_089EDA4C(c); return; }
    c->r[3] = *W(rec);
    if (c->r[3] & 1) {
        call1(c, *W(rec + 4), f_089ECFDC, 0x089EDAB0u);
        ARG(1) = c->r[2];
        if (c->r[2] != 0) { f_089EDA4C(c); return; }
        c->r[3] = *W(rec);
    }
    f_089EDA24(c);
}

/* (state, data): a sequence is loaded into the first free place of the 0x80 at 0x08E74308: its
   state is reset with the place's number and the file opened; a file that does not open leaves
   the state marked 3 and the place free. Returns the number; 0x80450001 before the list exists,
   0x80450003 without data, 0x80540004 without a state, 0x80540006 with no room, or the file's
   error. A 0x10 frame keeping s0..s2. */
SFA_FUNC(089EF238, sequence_load) {
    gaddr state = ARG(0), data = ARG(1);
    c->r[8] = *W(0x08E74304u);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 8) = c->r[18];
    c->r[7] = state;
    c->r[18] = data;
    *W(sp + 12) = c->r[31];
    *W(sp + 4) = c->r[17];
    *W(sp) = c->r[16];
    if (c->r[8] == 0) ARG(2) = 0x80450001u;
    else if (data == 0) ARG(2) = 0x80450003u;
    else if (state == 0) ARG(2) = 0x80540004u;
    else {
        gaddr slot = 0x08E74308u;
        uint32_t i = 0;
        while (i < 0x80 && *W(slot) != 0) { i++; slot += 4; }
        c->r[16] = slot; c->r[17] = i;
        if (i == 0x80) ARG(2) = 0x80540006u;
        else {
            *W(slot) = state;
            ARG(1) = i;
            call1(c, state, f_089F0104, 0x089EF2D8u);
            ARG(1) = *W(slot);
            call1(c, data, f_089F0554, 0x089EF2E4u);
            ARG(2) = i;
            if (c->r[2] != 0) {
                c->r[7] = *W(slot);
                ARG(2) = c->r[2];
                *B(c->r[7]) = 3;
                *W(slot) = 0;
            }
        }
    }
    f_089EF2B0(c);
}

/* The end of a routine that starts a note (the track's note in s0, the voice's number in s2). A
   voice without an instrument (+93 at 0xFF) takes the note's own pitch (+48). Otherwise the
   pitch comes from the instrument's fine tunings (+76 and +78, as 127 hundredths) and its key
   (+77) through the pitch routine, is multiplied by the voice's +44 and scaled (a division by
   44100 in the original's way), has the voice's +100 added, and is kept between 1 and 0x4000.
   The note's start follows. */
SFA_FUNC(089F29FC, tail_note_pitch) {
    ARG(2) = 0x68;
    c->r[19] = 0x08E70000u;
    int64_t m = (int64_t)(int32_t)c->r[18] * 0x68;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    gaddr voice = c->lo + VOICES;
    c->r[17] = voice;
    c->r[3] = 0xFF;
    c->r[9] = *B(voice + 93);
    c->r[14] = voice + 0x40;
    if (c->r[9] == 0xFF) {
        ARG(1) = *W(c->r[16] + 48);
        ARG(0) = c->r[18];
        f_089F2ABC(c);
        return;
    }
    int32_t a = S8(voice + 76) * 127, b = S8(voice + 78) * 127;
    int64_t ma = (int64_t)a * 0x51EB851F, mb = (int64_t)b * 0x51EB851F;
    c->lo = (uint32_t)mb; c->hi = (uint32_t)(mb >> 32);
    int32_t fine_a = ((int32_t)(ma >> 32) >> 5) - (a >> 31);
    int32_t fine_b = ((int32_t)(mb >> 32) >> 5) - (b >> 31);
    ARG(0) = *B(voice + 77);
    ARG(2) = c->r[9];
    ARG(3) = (uint32_t)(int32_t)(int16_t)fine_a;
    ARG(1) = (uint32_t)(int32_t)(int16_t)fine_b;
    c->r[31] = 0x089F2A80u; f_089EEC08(c);
    c->r[15] = *W(voice + 44);
    c->r[13] = 0xBE37C63Bu;
    m = (int64_t)(int32_t)c->r[15] * (int32_t)c->r[2];
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    c->r[11] = *W(voice + 100);
    c->r[9] = 0x4000;
    ARG(0) = *B(c->r[16]);
    c->r[2] = 1;
    c->r[14] = c->lo;
    uint64_t u = (uint64_t)c->r[14] * 0xBE37C63Bu;
    c->lo = (uint32_t)u; c->hi = (uint32_t)(u >> 32);
    c->r[12] = c->hi;
    c->r[10] = c->r[12] >> 15;
    c->r[8] = c->r[10] + c->r[11];
    ARG(3) = (uint32_t)MIN((int32_t)c->r[8], 0x4000);
    ARG(1) = (uint32_t)MAX((int32_t)ARG(3), 1);
    f_089F2ABC(c);
}

void f_089F2E50(Cpu *c); void f_089ECBF8(Cpu *c); void f_089EDB58(Cpu *c); void f_089ECC5C(Cpu *c);

/* A voice's pitch when its bend (+100) has changed since it was last set (+12): worked out as at
   a note's start and set, and the bend remembered. */
static void voice_pitch_update(Cpu *c, gaddr v) {
    int32_t b = S8(v + 78) * 127, a = S8(v + 76) * 127;
    int64_t mb = (int64_t)b * (int32_t)c->r[18], ma = (int64_t)a * (int32_t)c->r[18];
    c->lo = (uint32_t)ma; c->hi = (uint32_t)(ma >> 32);
    ARG(0) = *B(v + 77);
    ARG(2) = *B(v + 93);
    ARG(1) = (uint32_t)(int32_t)(int16_t)(((int32_t)(mb >> 32) >> 5) - (b >> 31));
    ARG(3) = (uint32_t)(int32_t)(int16_t)(((int32_t)(ma >> 32) >> 5) - (a >> 31));
    c->r[31] = 0x089F2D08u; f_089EEC08(c);
    int64_t m = (int64_t)(int32_t)*W(v + 44) * (int32_t)c->r[2];
    uint64_t u = (uint64_t)(uint32_t)m * 0xBE37C63Bu;
    c->lo = (uint32_t)u; c->hi = (uint32_t)(u >> 32);
    int32_t pitch = (int32_t)((c->hi >> 15) + *W(v + 100));
    pitch = MIN(pitch, 0x4000);
    ARG(1) = (uint32_t)MAX(pitch, 1);
    ARG(0) = *B(v);
    c->r[31] = 0x089F2D48u; f_089ECC5C(c);
    *W(v + 12) = *W(v + 100);
}

/* The rest of a loop over the voices (s0 the voice, s1 its number, r4 the pitch it was last set
   to): each voice in use whose four pan bytes (+96 to +99) differ from the ones last used (+8 to
   +11) has its two pairs of volumes split again and set, and each whose bend has changed has its
   pitch set again. The frame's end is a 0x30 one that kept s0..s3 at +16, with the four volumes
   at its start. */
SFA_FUNC(089F2CA8, voices_update_rest) {
    gaddr sp = c->r[29];
    for (;;) {
        gaddr v = c->r[16];
        if (ARG(0) != *W(v + 100)) voice_pitch_update(c, v);
        for (;;) {
            c->r[17] = c->r[17] + 1;
            int more = c->r[17] < *W(c->r[19] + 17672);
            c->r[16] += 0x68;
            if (!more) {
                c->r[31] = *W(sp + 32);
                c->r[19] = *W(sp + 28);
                c->r[18] = *W(sp + 24);
                c->r[17] = *W(sp + 20);
                c->r[16] = *W(sp + 16);
                c->r[29] += 0x30;
                return;
            }
            if (S8(c->r[16] + 1) != 0) break;
        }
        v = c->r[16];
        if (*B(v + 8) != *B(v + 96) || *B(v + 9) != *B(v + 97) || *B(v + 10) != *B(v + 98)
            || *B(v + 11) != *B(v + 99)) {
            ARG(0) = *W(v + 52); ARG(1) = *B(v + 96); ARG(2) = *B(v + 56); ARG(3) = *B(v + 97);
            c->r[8] = sp; c->r[9] = sp + 4;
            c->r[31] = 0x089F2C38u; f_089F2E50(c);
            ARG(0) = *W(v + 60); ARG(1) = *B(v + 98); ARG(2) = *B(v + 64); ARG(3) = *B(v + 99);
            c->r[9] = sp + 0xC; c->r[8] = sp + 8;
            c->r[31] = 0x089F2C54u; f_089F2E50(c);
            ARG(0) = *B(v); ARG(1) = *W(sp); ARG(2) = *W(sp + 4); ARG(3) = *W(sp + 8); c->r[8] = *W(sp + 12);
            c->r[31] = 0x089F2C6Cu; f_089ECBF8(c);
            ARG(1) = *W(sp); ARG(2) = *W(sp + 4); ARG(3) = *W(sp + 8); c->r[8] = *W(sp + 12);
            ARG(0) = c->r[17];
            c->r[31] = 0x089F2C84u; f_089EDB58(c);
            *B(v + 8) = *B(v + 96);
            *B(v + 9) = *B(v + 97);
            *B(v + 10) = *B(v + 98);
            *B(v + 11) = *B(v + 99);
        }
        ARG(0) = *W(v + 12);
    }
}
