/* The sound engine's sequences: one event of a track read and carried out, and a controller
   change. A sequence's record is described in batch_seq420.c; besides, +8 is its last status
   byte, +16 its tempo, +28 the place it reads from, +32 where its loop begins, +36 whether the
   next data entry is the loop's count, +37 that count.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089F0244(Cpu *c); void f_089F0248(Cpu *c); void f_089F029C(Cpu *c); void f_089F0354(Cpu *c);
void f_089F0358(Cpu *c); void f_089F035C(Cpu *c); void f_089ED71C(Cpu *c); void f_089F0684(Cpu *c);
void f_089F0D78(Cpu *c); void f_089F092C(Cpu *c); void f_089F0930(Cpu *c); void f_089EDE14(Cpu *c);
void f_089EDC1C(Cpu *c); void f_089EDF94(Cpu *c); void f_089EE070(Cpu *c); void f_089EDFD0(Cpu *c);
void f_089EE0AC(Cpu *c); void f_089F0CB0(Cpu *c); void f_089F08D0(Cpu *c);

/* The two data bytes of an event are read and the routine called with (status, first, second,
   track); the status is then kept. */
static void two_byte_event(Cpu *c, void (*fn)(Cpu *), uint32_t back) {
    gaddr t = c->r[16];
    gaddr p = *W(t + 28);
    ARG(0) = c->r[17];
    ARG(3) = t;
    ARG(1) = *B(p);
    *W(t + 28) = p + 1;
    ARG(2) = *B(p + 1);
    *W(t + 28) = p + 2;
    c->r[31] = back; fn(c);
    *B(t + 8) = (uint8_t)c->r[17];
    f_089F0248(c);
}

/* (track): the track's next event is read (a byte below 0x80 means the status before is used
   again) and carried out: 0x80 note off, 0x90 note on, 0xB0 controller, 0xC0 program (kept for
   the channel), 0xE0 pitch bend; 0xA0 and 0xD0 are passed over; 0xF0 to 0xFE skip the length
   that follows (seven bits a byte); 0xFF is a meta event: 0x2F the track's end, 0x51 the tempo
   (three bytes, kept at +16), anything else skipped by its length. The ends keep the status and
   close the 0x10 frame that kept s0, s1. */
SFA_FUNC(089F01CC, track_event) {
    gaddr t = ARG(0);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    c->r[16] = t;
    c->r[8] = 0;
    *W(sp + 8) = c->r[31];
    *W(sp + 4) = c->r[17];
    gaddr p = *W(t + 28);
    c->r[17] = *B(p);
    *W(t + 28) = p + 1;
    c->r[6] = 0;
    if ((int8_t)c->r[17] >= 0) {
        *W(t + 28) = p;
        c->r[17] = *B(t + 8);
    }
    uint32_t status = c->r[17];
    uint32_t kind = status & 0xF0;
    c->r[3] = kind;
    if (kind == 0xB0) { two_byte_event(c, f_089F08D0, 0x089F04FCu); return; }
    if (kind == 0x90) { two_byte_event(c, f_089F0684, 0x089F02D0u); return; }
    if (kind == 0xE0) { two_byte_event(c, f_089F0D78, 0x089F04BCu); return; }
    if (kind == 0x80) {
        p = *W(t + 28);
        ARG(0) = status & 0xF;
        *W(t + 28) = p + 2;
        ARG(1) = *B(p);
        ARG(2) = *W(t + (ARG(0) << 4) + 72);
        c->r[31] = 0x089F0284u; f_089ED71C(c);
        *B(t + 8) = (uint8_t)status;
        f_089F0248(c);
        return;
    }
    if (kind == 0xA0) { c->r[2] = *W(t + 28) + 2; f_089F029C(c); return; }
    if (kind == 0xD0) { c->r[2] = *W(t + 28) + 1; f_089F029C(c); return; }
    if (kind == 0xC0) {
        p = *W(t + 28);
        c->r[24] = *B(p);
        *W(t + 28) = p + 1;
        *B(t + ((status & 0xF) << 4) + 64) = (uint8_t)c->r[24];
        f_089F0244(c);
        return;
    }
    if (kind != 0xF0) {
        if (kind < 0x80) { f_089F0244(c); return; }
        *B(t + 8) = (uint8_t)status;
        f_089F0248(c);
        return;
    }
    if (status != 0xFF) {
        p = *W(t + 28);
        uint32_t b = *B(p);
        *W(t + 28) = p + 1;
        uint32_t length = b;
        if ((int8_t)b < 0) {
            length = b & 0x7F;
            do {
                p = *W(t + 28);
                b = *B(p);
                *W(t + 28) = p + 1;
                length = (length << 7) + (b & 0x7F);
            } while ((int8_t)b < 0);
        }
        ARG(0) = length;
        ARG(1) = *W(t + 28);
        f_089F0354(c);
        return;
    }
    p = *W(t + 28);
    uint32_t type = *B(p);
    c->r[8] = type;
    *W(t + 28) = p + 1;
    uint32_t length = *B(p + 1);
    c->r[6] = length;
    *W(t + 28) = p + 2;
    if (type == 0x51) {
        *W(t + 28) = p + 3;
        *W(t + 28) = p + 4;
        uint32_t tempo = ((uint32_t)*B(p + 2) << 16) | ((uint32_t)*B(p + 3) << 8) | *B(p + 4);
        *W(t + 28) = p + 5;
        *W(t + 16) = tempo;
        f_089F035C(c);
        return;
    }
    c->r[2] = *W(t + 28) + length;
    f_089F0358(c);
}

/* A number over 127 the way the original works it out, without and with the step that makes
   it right for numbers below 0. */
static uint32_t over127(int32_t n) {
    return (uint32_t)(((int32_t)(((int64_t)n * (int32_t)0x81020409u) >> 32) + n) >> 6);
}

/* A controller's change given to the voices of the channel's group (the mask in s3) from the
   one in s1 on. With `pan` each voice's two pans are set again from the sequence's and the
   channel's (s4 the channel); otherwise each voice's two levels: its own level (from the
   routine at 0x089EDC1C) times the sequence's, the channel's volume and its expression (s5 the
   channel), each over 127. Then the routine's end. */
static void controller_voices_from(Cpu *c, int pan, uint32_t back_level, uint32_t back_a, uint32_t back_b) {
    gaddr t = c->r[18];
    for (; c->r[17] < 0x20; c->r[17]++) {
        if (((c->r[19] >> (c->r[17] & 31)) & 1) == 0) continue;
        ARG(0) = c->r[17];
        uint32_t a, b;
        if (pan) {
            uint32_t ch_pan = *B(c->r[20] + 3);
            a = (uint32_t)MIN(MAX((int32_t)(*B(t + 41) + ch_pan) - 0x40, (int32_t)c->r[22]), (int32_t)c->r[21]);
            b = (uint32_t)MIN(MAX((int32_t)(*B(t + 43) + ch_pan) - 0x40, (int32_t)c->r[22]), (int32_t)c->r[21]);
            c->r[16] = b;
        } else {
            c->r[31] = back_level; f_089EDC1C(c);
            uint32_t level = c->r[2] & 0xFF;
            uint32_t volume = *B(c->r[21] + 1), expression = *B(c->r[21] + 2);
            int32_t xa = (int32_t)((over127((int32_t)(over127((int32_t)(level * *B(t + 40))) & 0xFF) * (int32_t)volume) & 0xFF) * expression);
            int32_t xb = (int32_t)((over127((int32_t)(over127((int32_t)(level * *B(t + 42))) & 0xFF) * (int32_t)volume) & 0xFF) * expression);
            a = (over127(xa) - (uint32_t)(xa >> 31)) & 0xFF;
            b = (over127(xb) - (uint32_t)(xb >> 31)) & 0xFF;
            c->r[22] = b;
            ARG(0) = c->r[17];
        }
        ARG(1) = a;
        c->r[31] = back_a;
        if (pan) f_089EDFD0(c); else f_089EDF94(c);
        ARG(0) = c->r[17];
        ARG(1) = b;
        c->r[31] = back_b;
        if (pan) f_089EE0AC(c); else f_089EE070(c);
    }
    c->r[31] = *W(c->r[29] + 28);
    f_089F0930(c);
}

SFA_FUNC(089F0998, controller_volume_rest) { controller_voices_from(c, 0, 0x089F09B4u, 0x089F0A88u, 0x089F0A94u); }
SFA_FUNC(089F0B14, controller_expression_rest) { controller_voices_from(c, 0, 0x089F0B30u, 0x089F0C04u, 0x089F0C10u); }
SFA_FUNC(089F0D20, controller_pan_rest) { controller_voices_from(c, 1, 0, 0x089F0D64u, 0x089F0D70u); }

/* (status, controller, value, track): a controller change for the status's channel:
     7 volume, 11 expression, 10 pan: kept for the channel and given to its group's voices.
     64 the channel's fifth setting (1 for a value of 127, else 0), passed to the routine at
       0x089ED808 with the group.
     99 the loop: value 0 marks its start here and makes the next data entry its count; value 1
       goes back to the start, a count of 0 always, a count of -1 with no start never, else once
       for each of the count (the start is forgotten when it runs out).
     6 data entry: the loop's count, when one is awaited.
   Returns 0. A 0x20 frame keeping s0..s6, which the ends close. */
SFA_FUNC(089F08D0, sequence_controller) {
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    uint32_t control = ARG(1) & 0xFF;
    ARG(1) = control;
    *W(sp + 8) = c->r[18];
    uint32_t value = ARG(2) & 0xFF;
    c->r[6] = value;
    gaddr t = ARG(3);
    c->r[18] = t;
    *W(sp + 28) = c->r[31];
    uint32_t channel = ARG(0) & 0xF;
    ARG(0) = channel;
    *W(sp + 24) = c->r[22]; *W(sp + 20) = c->r[21]; *W(sp + 16) = c->r[20]; *W(sp + 12) = c->r[19];
    *W(sp + 4) = c->r[17]; *W(sp) = c->r[16];
    gaddr ch = t + (channel << 4);
    if (control == 7 || control == 11 || control == 10) {
        c->r[16] = ch + 0x40;
        *B(ch + 0x40 + (control == 7 ? 1 : control == 11 ? 2 : 3)) = (uint8_t)value;
        ARG(1) = *W(ch + 72);
        c->r[31] = control == 7 ? 0x089F096Cu : control == 11 ? 0x089F0AE8u : 0x089F0CF4u;
        f_089EDE14(c);
        c->r[19] = c->r[2];
        if (c->r[2] == 0) { f_089F092C(c); return; }
        c->r[17] = 0;
        if (control == 10) {
            c->r[20] = c->r[16];
            c->r[22] = 0;
            c->r[21] = 0x7F;
            controller_voices_from(c, 1, 0, 0x089F0D64u, 0x089F0D70u);
        } else {
            c->r[21] = c->r[16];
            c->r[20] = 0x81020409u;
            if (control == 7) controller_voices_from(c, 0, 0x089F09B4u, 0x089F0A88u, 0x089F0A94u);
            else controller_voices_from(c, 0, 0x089F0B30u, 0x089F0C04u, 0x089F0C10u);
        }
        return;
    }
    if (control == 6) {
        c->r[31] = *W(sp + 28);
        if (*B(t + 36) != 0) {
            *B(t + 37) = (uint8_t)value;
            *B(t + 36) = 0;
        }
        f_089F0930(c);
        return;
    }
    if (control == 0x40) {
        if (value == 0x7F) {
            c->r[18] = 1;
            ARG(1) = 1;
            *B(ch + 68) = 1;
        } else {
            *B(ch + 68) = 0;
            ARG(1) = 0;
        }
        c->r[6] = *W(ch + 72);
        f_089F0CB0(c);
        return;
    }
    if (control == 0x63) {
        c->r[31] = *W(sp + 28);
        if (value == 0) {
            *B(t + 36) = 1;
            *W(t + 32) = *W(t + 28);
            f_089F0930(c);
            return;
        }
        if (value == 1) {
            gaddr start = *W(t + 32);
            uint32_t count = *B(t + 37);
            if (start == 0 && (int8_t)count == -1) { f_089F0930(c); return; }
            if ((int8_t)count == 0) *W(t + 28) = start;
            else {
                *B(t + 37) = (uint8_t)(count - 1);
                if ((int8_t)(count - 1) == 0) {
                    *B(t + 37) = 0xFF;
                    *W(t + 32) = 0;
                } else *W(t + 28) = start;
            }
            f_089F092C(c);
            return;
        }
        f_089F0930(c);
        return;
    }
    if ((int32_t)control < 11) { f_089F092C(c); return; }
    c->r[31] = *W(sp + 28);
    f_089F0930(c);
}
