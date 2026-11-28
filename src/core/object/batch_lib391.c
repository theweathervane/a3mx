/* Three more routines around the library: the game's sound started, the music stream's thread,
   and the library's reading of one character from a string in the locale's encoding. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define SOUND 0x08E69200u

void f_0880A0B4(Cpu *c); void f_0880A028(Cpu *c); void f_089E6628(Cpu *c); void f_089E6130(Cpu *c);
void f_089E68A4(Cpu *c); void f_089F351C(Cpu *c); void f_089F34A4(Cpu *c);
void hle_sceKernelExitDeleteThread(Cpu *c);

/* The game's sound is started: its two routines of its own (the first with 0), then the sound
   system; the settings' +71 is set. Returns 0. A 0x20 frame. */
SFA_FUNC(0880986C, game_sound_start) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(0) = 0; c->r[31] = 0x0880987Cu; f_0880A0B4(c);
    c->r[31] = 0x08809884u; f_0880A028(c);
    c->r[31] = 0x0880988Cu; f_089E6628(c);
    ARG(0) = 1;
    ARG(1) = 0x08A058A0u;
    *B(0x08A058A0u + 71) = 1;
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* The music stream's thread: the stream (kept in the 0x30 frame at +16) is opened and, when that
   works, played to its end, with the sound's +167 set to 1 (or 0xFF when it does not open);
   then the sound's +168 is cleared and the thread ends and is deleted. Keeps s0, s1. */
SFA_FUNC(089E6288, music_thread_entry) {
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = s1;
    c->r[17] = sp + 0x10;
    *W(sp + 32) = s0; *W(sp + 40) = ra;
    ARG(0) = c->r[17]; c->r[31] = 0x089E62A4u; f_089E6130(c);
    c->r[16] = SOUND;
    if ((int32_t)c->r[2] >= 0) {
        *B(SOUND + 167) = 1;
        ARG(0) = c->r[17]; c->r[31] = 0x089E62CCu; f_089E68A4(c);
    } else {
        ARG(0) = 0xFF;
        *B(SOUND + 167) = 0xFF;
    }
    *W(SOUND + 168) = 0;
    ARG(0) = 0; c->r[31] = 0x089E62D8u; hle_sceKernelExitDeleteThread(c);
    c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x30;
}

/* (r, to, s, n, state): one character is read from the string s (of n bytes) to the halfword at
   `to` (a place in the frame when 0), in the encoding the locale at r's +52 names. Returns the
   bytes used, 0 for the string's end or no string, -1 for what is not a character.
     none, a name under two letters, or any name not below: each byte is a character.
     "C-SJIS": a first byte of 0x81..0x9F or 0xE0..0xEF takes a second of 0x40..0x7E or
       0x80..0xFC; anything else is one byte.
     "C-EUCJP": a first byte of 0xA1..0xFE takes a second of the same; anything else is one byte.
     "C-JIS": the bytes go through the two tables at 0x08AD8C80 (what to do) and 0x08AD8AD0 (the
       state after), nine kinds of byte for each of the states, from the state `state` points
       at; with no string that state is cleared and 1 returned.
   A 0x30 frame keeping s0..s5. */
SFA_FUNC(089F8E84, mbtowc_reent) {
    uint32_t sv[7];
    for (int i = 0; i < 6; i++) sv[i] = c->r[16 + i];
    sv[6] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[0]; *W(sp + 24) = sv[1]; *W(sp + 28) = sv[2]; *W(sp + 32) = sv[3];
    *W(sp + 36) = sv[4]; *W(sp + 40) = sv[5];
    gaddr r = ARG(0), to = ARG(1), s = ARG(2), state = c->r[8];
    uint32_t n = ARG(3);
    c->r[21] = s; c->r[16] = r; c->r[17] = to; c->r[18] = s; c->r[19] = n; c->r[20] = state;
    *W(sp + 44) = sv[6];
    if (to == 0) { to = sp + 0x10; c->r[17] = to; }
    uint32_t answer;
    do {
        if (s != 0 && n == 0) { answer = 0xFFFFFFFFu; break; }
        gaddr locale = *W(r + 52);
        ARG(0) = locale;
        int plain = locale == 0;
        if (!plain) {
            c->r[31] = 0x089F8F18u; f_089F351C(c);
            ARG(0) = (int32_t)c->r[2] < 2;
            ARG(1) = 0x08A00000u;
            plain = (int32_t)c->r[2] < 2;
        }
        if (!plain) {
            ARG(0) = *W(r + 52); ARG(1) = 0x08A02D70u;
            c->r[31] = 0x089F8F30u; f_089F34A4(c);
            ARG(1) = 0x08A00000u;
            if (c->r[2] == 0) {
                if (s == 0) { answer = 0; break; }
                int32_t ch = *B(s);
                ARG(0) = (uint32_t)ch;
                if ((ch >= 129 && ch < 160) || (ch >= 224 && ch < 240)) {
                    int32_t second = *B(s + 1);
                    c->r[21] = (uint32_t)second;
                    if (n < 2) { answer = 0xFFFFFFFFu; break; }
                    if ((second >= 64 && second < 127) || (second >= 128 && second < 253)) {
                        ARG(0) = (uint32_t)((ch << 8) + second);
                        *H(to) = (uint16_t)ARG(0);
                        answer = 2;
                    } else answer = 0xFFFFFFFFu;
                    break;
                }
                plain = 1;
            }
        }
        if (!plain) {
            ARG(0) = *W(r + 52); ARG(1) = 0x08A02D78u;
            c->r[31] = 0x089F9038u; f_089F34A4(c);
            ARG(1) = 0x08A00000u;
            if (c->r[2] == 0) {
                if (s == 0) { answer = 0; break; }
                int32_t ch = *B(s);
                ARG(0) = (uint32_t)ch;
                if (ch >= 161 && ch < 255) {
                    int32_t second = *B(s + 1);
                    c->r[21] = (uint32_t)second;
                    if (n < 2) { answer = 0xFFFFFFFFu; break; }
                    if (second >= 161 && second < 255) {
                        ARG(0) = (uint32_t)((ch << 8) + second);
                        *H(to) = (uint16_t)ARG(0);
                        answer = 2;
                    } else answer = 0xFFFFFFFFu;
                    break;
                }
                plain = 1;
            }
        }
        if (!plain) {
            ARG(0) = *W(r + 52); ARG(1) = 0x08A02D80u;
            c->r[31] = 0x089F9104u; f_089F34A4(c);
            if (c->r[2] == 0) {
                if (s == 0) { *W(state) = 0; answer = 1; break; }
                uint32_t st = *W(state) == 0 ? 0 : 3;
                uint32_t i = 0;
                gaddr p = s, from = s;
                answer = 0xFFFFFFFFu;
                int done = 0;
                while (!done && i < n) {
                    uint32_t ch = *B(p), kind;
                    switch (ch) {
                    case 0x1B: kind = 0; break;
                    case '$': kind = 1; break;
                    case '(': kind = 2; break;
                    case '@': kind = 3; break;
                    case 'B': kind = 4; break;
                    case 'J': kind = 5; break;
                    case 0: kind = 6; break;
                    default: kind = (ch >= 33 && ch < 127) ? 7 : 8; break;
                    }
                    uint32_t action = *W(0x08AD8C80u + st * 36 + kind * 4);
                    st = *W(0x08AD8AD0u + st * 36 + kind * 4);
                    done = 1;
                    switch (action) {
                    case 0:
                        *W(state) = 0;
                        *H(to) = *B(from);
                        answer = i + 1;
                        break;
                    case 1:
                        *W(state) = 0;
                        *H(to) = (uint16_t)((*B(from) << 8) + *B(from + 1));
                        answer = i + 1;
                        break;
                    case 2:
                        *W(state) = 1;
                        *H(to) = (uint16_t)((*B(from) << 8) + *B(from + 1));
                        answer = from - s + 2;
                        break;
                    case 3: case 4:
                        from = p + 1;
                        i++; p++;
                        done = 0;
                        break;
                    case 5:
                        i++; p++;
                        done = 0;
                        break;
                    case 6:
                        *W(state) = 0;
                        *H(to) = 0;
                        answer = i;
                        break;
                    default:
                        answer = 0xFFFFFFFFu;
                        break;
                    }
                }
                break;
            }
        }
        if (s == 0) { answer = 0; break; }
        ARG(0) = *B(s);
        *H(to) = (uint16_t)ARG(0);
        answer = *B(s) != 0;
    } while (0);
    c->r[2] = answer;
    for (int i = 0; i < 6; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[6]; c->r[29] += 0x30;
}
