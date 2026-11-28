/* The library's formatted printing: sprintf, the choice between the printing with and without
   floating point, the one without, and what it writes through. Offsets stay numbers until
   something shows what they are.

   A stream is the library's own record: +0 the next place to write, +8 the room left, +12 its
   flags (0x200: it writes to memory), +14 its file, +84 the library's state.
 */
#include "step.h"
#include "../guest_mem.h"

#define REENT    0x08AD84DCu /* points at the library's state */
#define LINE     0x08E75858u /* the 128 bytes a file's output is gathered in */
#define LINE_N   0x08AD8560u /* how many are in it */
#define LINE_AT  0x08AD8564u /* the next place in it */
#define BLANKS   0x08AD8568u /* sixteen spaces */
#define ZEROES   0x08AD8578u /* sixteen zeroes */

void f_089F3430(Cpu *c); void f_089F351C(Cpu *c); void f_089F8E84(Cpu *c); void f_089F54CC(Cpu *c);
void f_089F40E0(Cpu *c);
void hle_sceKernelStdout(Cpu *c); void hle_sceKernelStderr(Cpu *c); void hle_sceKernelStdin(Cpu *c);
void hle_sceIoWrite(Cpu *c);

/* (p, ch, n): the first of the n bytes at p that is ch, or 0. */
SFA_FUNC(089F9488, lib_memchr) {
    gaddr p = ARG(0);
    uint32_t n = ARG(2);
    c->r[2] = p;
    c->r[7] = n;
    ARG(0) = n - 1;
    ARG(1) &= 0xFF;
    c->r[6] = n - 1;
    if (n != 0) {
        for (;;) {
            c->r[7] = *B(c->r[2]);
            if (c->r[7] == ARG(1)) { c->r[7] = ARG(0); return; }
            c->r[7] = ARG(0);
            ARG(0) = c->r[6] - 1;
            c->r[2]++;
            if (c->r[7] == 0) break;
            c->r[6] = ARG(0);
        }
    }
    c->r[2] = 0;
}

/* The end of the write to a file: the bytes (s0, s1 of them) go to the system's file in r2
   unless that is below 0 (the answer is then -9); the frame of 0x10 that kept s0, s1 closes. */
SFA_FUNC(088047EC, tail_file_write) {
    gaddr sp = c->r[29];
    c->r[3] = 0xFFFFFFF7u;
    if ((int32_t)c->r[2] >= 0) {
        ARG(0) = c->r[2]; ARG(1) = c->r[16]; ARG(2) = c->r[17];
        c->r[31] = 0x08804804u; hle_sceIoWrite(c);
        c->r[3] = c->r[2];
    }
    c->r[31] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = c->r[3];
    c->r[29] += 0x10;
}

/* (file, bytes, n): n bytes written to a file: 0, 1 and 2 are the system's input, output and
   error files, anything else the system's own number. A 0x10 frame keeping s0, s1, which the
   end closes. */
SFA_FUNC(088047B4, file_write) {
    int32_t file = (int32_t)ARG(0);
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    c->r[3] = 1;
    *W(sp + 4) = c->r[17];
    c->r[17] = ARG(2);
    *W(sp) = c->r[16];
    c->r[16] = ARG(1);
    *W(sp + 8) = c->r[31];
    if (file == 1) { c->r[31] = 0x08804850u; hle_sceKernelStdout(c); }
    else if (file == 2) { c->r[3] = 0; ARG(1) = 2; c->r[31] = 0x08804828u; hle_sceKernelStderr(c); }
    else if (file == 0) { c->r[3] = 1; ARG(1) = 2; c->r[31] = 0x08804840u; hle_sceKernelStdin(c); }
    else { c->r[3] = file < 2; c->r[2] = (uint32_t)file; }
    f_088047EC(c);
}

/* (file, bytes, n, flush): output to a file through the line of 128 bytes: each byte goes into
   the line, which is written out when it is full (0 is returned if that write answers 0, else
   n). With flush at 1 the line is written out as it is, and the write's answer returned. A 0x30
   frame keeping s0..s6. */
SFA_FUNC(089F569C, file_put) {
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2];
    c->r[8] = 1;
    c->r[18] = ARG(0);
    c->r[17] = ARG(1);
    *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4]; *W(sp + 36) = sv[5];
    *W(sp + 40) = sv[6]; *W(sp + 44) = ra;
    c->r[16] = ARG(2);
    uint32_t answer;
    if (ARG(3) == 1) {
        c->r[16] = 0x08AE0000u;
        c->r[17] = LINE;
        ARG(2) = *W(LINE_N);
        ARG(0) = c->r[18]; ARG(1) = LINE;
        c->r[31] = 0x089F5710u; f_088047B4(c);
        *W(LINE_N) = 0;
        *W(LINE_AT) = LINE;
        answer = c->r[2];
    } else {
        gaddr bytes = c->r[17];
        uint32_t n = c->r[16];
        c->r[19] = 0;
        c->r[22] = 0x08E70000u;
        answer = 0;
        if (n != 0) {
            c->r[22] = LINE;
            c->r[20] = 0x08AE0000u; c->r[21] = 0x08AE0000u;
            int stopped = 0;
            do {
                *B(*W(LINE_AT)) = *B(bytes + c->r[19]);
                *W(LINE_AT) = *W(LINE_AT) + 1;
                uint32_t held = *W(LINE_N) + 1;
                *W(LINE_N) = held;
                if ((int32_t)held >= 128) {
                    ARG(2) = held; ARG(0) = c->r[18]; ARG(1) = LINE;
                    c->r[31] = 0x089F5784u; f_088047B4(c);
                    *W(LINE_AT) = LINE;
                    *W(LINE_N) = 0;
                    if (c->r[2] == 0) { stopped = 1; break; }
                }
                c->r[19]++;
            } while (c->r[19] < n);
            answer = stopped ? 0 : c->r[19];
        }
    }
    c->r[2] = answer;
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* n bytes go to the stream: into its memory as far as there is room, or to its file (file 0 of a
   stream with no flags becomes 1 first). The three return addresses are the three calls this is
   in the original at each place it is used. */
static void emit(Cpu *c, gaddr fp, gaddr p, int32_t n, uint32_t full, uint32_t part, uint32_t file) {
    if ((S16(fp + 12) & 0x200) != 0) {
        int32_t room = (int32_t)*W(fp + 8);
        if (n < room) {
            ARG(0) = *W(fp); ARG(1) = p; ARG(2) = (uint32_t)n;
            c->r[31] = full; f_089F3430(c);
            *W(fp) = *W(fp) + (uint32_t)n;
            *W(fp + 8) = *W(fp + 8) - (uint32_t)n;
        } else {
            ARG(0) = *W(fp); ARG(2) = (uint32_t)room; ARG(1) = p;
            c->r[31] = part; f_089F3430(c);
            uint32_t at = *W(fp), left = *W(fp + 8);
            *W(fp + 8) = 0;
            *W(fp) = at + left;
        }
    } else {
        int32_t to = S16(fp + 14);
        if (to == 0 && S16(fp + 12) == 0) {
            *H(fp + 14) = 1;
            to = S16(fp + 14);
        }
        ARG(0) = (uint32_t)to; ARG(1) = p; ARG(2) = (uint32_t)n; ARG(3) = 0;
        c->r[31] = file; f_089F569C(c);
    }
}

/* The count in the register `reg` (above 0) of the sixteen bytes at `with` go to the stream,
   sixteen at a time; the register keeps what the last piece was. */
static void pad(Cpu *c, gaddr fp, gaddr with, int reg, const uint32_t back[6]) {
    while ((int32_t)c->r[reg] >= 17) {
        emit(c, fp, with, 16, back[0], back[1], back[2]);
        c->r[reg] -= 16;
    }
    emit(c, fp, with, (int32_t)c->r[reg], back[3], back[4], back[5]);
}

static int is_digit(int32_t ch) { return (uint32_t)(ch - 0x30) < 10; }

/* (-, stream, format, arguments): the printing without floating point. The format's plain text
   is copied, a character at a time in the locale's encoding, and each % item printed:
     flags  space and + (a sign), # (0 before octal, 0x before hex), - (left), 0 (zeroes), '
            (digits in groups, by the routine at 0x089F54CC)
     width and .precision, each a number or * for an argument
     sizes  h, l, ll and q (the last two are read as one word all the same)
     items  d i u o x X (and D U O as long), p, c, s (no string prints "(null)"), n, and any
            other character as itself
   The frame of 0xB0 holds the item's parts where the original keeps them: the sign at +32, the
   character read at +34, the text's length at +36, the digits built backwards from +80, the
   two letters of a 0x at +80, the encoding's state at +84, the tables at +88 to +108, the
   left-or-zero flags at +112, the item's whole size at +116, the hex digits at +120, the left
   flag at +124, the precision at +128, and the count returned at +132. Keeps s0..s8, and the
   registers hold what the original's do across every call. */
SFA_FUNC(089F5820, lib_vfiprintf) {
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0xB0;
    gaddr sp = c->r[29];
    *W(sp + 140) = sv[1];
    *W(sp + 84) = 0;
    *W(sp + 132) = 0;
    *W(sp + 88) = BLANKS;
    *W(sp + 108) = ZEROES;
    *W(sp + 100) = 0x08A02BA4u; /* "(null)" */
    *W(sp + 104) = 0x08A02B90u; /* "0123456789abcdef" */
    *W(sp + 96) = 0x08A02BACu;  /* "0123456789ABCDEF" */
    *W(sp + 144) = sv[2]; *W(sp + 164) = sv[7]; *W(sp + 168) = sv[8];
    *W(sp + 92) = 0x08A02BC0u;  /* "bug in vfprintf: bad base" */
    *W(sp + 136) = sv[0]; *W(sp + 148) = sv[3]; *W(sp + 152) = sv[4]; *W(sp + 156) = sv[5];
    *W(sp + 160) = sv[6]; *W(sp + 172) = sv[9];
    gaddr fp = ARG(1);
    c->r[17] = ARG(2);
    c->r[23] = 0x2A;
    c->r[18] = fp;
    c->r[30] = ARG(3);
#define FMT   c->r[17]
#define AP    c->r[30]
#define FLAGS c->r[20]
#define WIDTH c->r[22]
#define PREC  c->r[19]
    for (;;) {
        /* The plain text up to the next %. */
        c->r[20] = FMT;
        c->r[21] = 0x25;
        c->r[22] = sp + 0x54;
        c->r[19] = 0x08AE0000u;
        for (;;) {
            ARG(0) = *W(REENT);
            ARG(3) = *W(0x08AD89D0u);
            ARG(1) = sp + 0x22;
            ARG(2) = FMT;
            c->r[8] = c->r[22];
            c->r[31] = 0x089F58DCu; f_089F8E84(c);
            c->r[16] = c->r[2];
            if ((int32_t)c->r[16] <= 0) break;
            FMT += c->r[16];
            if (*H(sp + 34) == 0x25) { FMT -= 1; break; }
        }
        c->r[19] = FMT - c->r[20];
        if (c->r[19] != 0) {
            emit(c, fp, c->r[20], (int32_t)c->r[19], 0x089F593Cu, 0x089F5968u, 0x089F59B0u);
            *W(sp + 132) = c->r[19] + *W(sp + 132);
        }
        if ((int32_t)c->r[16] <= 0) break;
        if ((int32_t)*W(fp + 8) <= 0 && (S16(fp + 12) & 0x200) != 0) break;

        /* The item's flags, width, precision and size. */
        FMT++;
        *W(sp + 128) = 0;
        FLAGS = 0;
        WIDTH = 0;
        PREC = 0xFFFFFFFFu;
        *B(sp + 32) = 0;
        int32_t ch = 0;
        int again = 0;
        for (;;) {
            if (!again) { ch = S8(FMT); FMT++; }
            again = 0;
            c->r[16] = (uint32_t)ch;
            if (ch == ' ') { if (S8(sp + 32) == 0) *B(sp + 32) = 0x20; }
            else if (ch == '#') FLAGS |= 1;
            else if (ch == '\'') FLAGS |= 0x200;
            else if (ch == '*') {
                AP += 4;
                WIDTH = *W(AP - 4);
                if ((int32_t)WIDTH < 0) { WIDTH = 0 - WIDTH; FLAGS |= 4; }
            }
            else if (ch == '-') FLAGS |= 4;
            else if (ch == '+') *B(sp + 32) = 0x2B;
            else if (ch == '.') {
                ch = S8(FMT); FMT++;
                c->r[16] = (uint32_t)ch;
                if (ch == '*') {
                    AP += 4;
                    int32_t v = (int32_t)*W(AP - 4);
                    PREC = v < 0 ? 0xFFFFFFFFu : (uint32_t)v;
                } else {
                    int32_t n = 0;
                    while (is_digit(ch)) { n = n * 10 + ch - 0x30; ch = S8(FMT); FMT++; }
                    PREC = n < 0 ? 0xFFFFFFFFu : (uint32_t)n;
                    again = 1;
                }
            }
            else if (ch == '0') FLAGS |= 0x80;
            else if (ch >= '1' && ch <= '9') {
                int32_t n = 0;
                do { n = n * 10 + ch - 0x30; ch = S8(FMT); FMT++; } while (is_digit(ch));
                WIDTH = (uint32_t)n;
                again = 1;
            }
            else if (ch == 'h') FLAGS |= 0x40;
            else if (ch == 'l') {
                if (S8(FMT) == 'l') { FMT++; FLAGS |= 0x20; }
                else FLAGS |= 0x10;
            }
            else if (ch == 'q') FLAGS |= 0x20;
            else break;
        }

        /* The item itself: its text at `text`, its length at +36. */
        if (ch == 'n') {
            gaddr to = *W(AP);
            uint32_t count = *W(sp + 132);
            AP += 4;
            if ((FLAGS & 0x20) == 0 && (FLAGS & 0x10) == 0 && (FLAGS & 0x40) != 0) *H(to) = (uint16_t)count;
            else *W(to) = count;
            continue;
        }
        gaddr text;
        int by_precision;
        if (ch == 'c' || (ch != 's' && ch != 'd' && ch != 'i' && ch != 'D' && ch != 'o' && ch != 'O'
                          && ch != 'u' && ch != 'U' && ch != 'x' && ch != 'X' && ch != 'p')) {
            if (ch == 0) break;
            text = sp + 0x28;
            if (ch == 'c') { AP += 4; *B(text) = (uint8_t)*W(AP - 4); }
            else *B(text) = (uint8_t)ch;
            *B(sp + 32) = 0;
            *W(sp + 36) = 1;
            *W(sp + 112) = FLAGS & 0x84;
            *W(sp + 124) = FLAGS & 4;
            by_precision = 0;
        } else if (ch == 's') {
            AP += 4;
            text = *W(AP - 4);
            *W(sp + 112) = FLAGS & 0x84;
            *W(sp + 124) = FLAGS & 4;
            if (text == 0) text = *W(sp + 100);
            c->r[21] = text;
            int32_t length;
            if ((int32_t)PREC >= 0) {
                ARG(0) = text; ARG(1) = 0; ARG(2) = PREC;
                c->r[31] = 0x089F5D9Cu; f_089F9488(c);
                length = (int32_t)PREC;
                if (c->r[2] != 0 && (int32_t)(c->r[2] - text) <= (int32_t)PREC) length = (int32_t)(c->r[2] - text);
            } else {
                ARG(0) = text;
                c->r[31] = 0x089F5DE0u; f_089F351C(c);
                length = (int32_t)c->r[2];
            }
            *W(sp + 36) = (uint32_t)length;
            *B(sp + 32) = 0;
            by_precision = length < 0;
        } else {
            uint32_t value;
            int base; /* 0 octal, 1 decimal, 2 hex */
            if (ch == 'D' || ch == 'O' || ch == 'U') FLAGS |= 0x10;
            if (ch == 'p') {
                AP += 4;
                value = *W(AP - 4);
                base = 2;
                FLAGS |= 2;
                *W(sp + 120) = *W(sp + 104);
                ch = 'x';
                c->r[16] = 0x78;
                *B(sp + 32) = 0;
            } else {
                AP += 4;
                value = *W(AP - 4);
                int is_short = (FLAGS & 0x20) == 0 && (FLAGS & 0x10) == 0 && (FLAGS & 0x40) != 0;
                if (ch == 'd' || ch == 'i' || ch == 'D') {
                    if (is_short) value = (uint32_t)(int16_t)value;
                    base = 1;
                    if ((int32_t)value < 0) { *B(sp + 32) = 0x2D; value = 0 - value; }
                } else {
                    if (is_short) value &= 0xFFFF;
                    if (ch == 'o' || ch == 'O') base = 0;
                    else if (ch == 'u' || ch == 'U') base = 1;
                    else {
                        base = 2;
                        *W(sp + 120) = ch == 'X' ? *W(sp + 96) : *W(sp + 104);
                        if ((FLAGS & 1) != 0 && value != 0) FLAGS |= 2;
                    }
                    *B(sp + 32) = 0;
                }
            }
            gaddr digits = *W(sp + 120);
            c->r[15] = digits;
            *W(sp + 128) = PREC;
            if ((int32_t)PREC >= 0) FLAGS &= 0xFFFFFF7Fu;
            *W(sp + 112) = FLAGS & 0x84;
            *W(sp + 124) = FLAGS & 4;
            text = sp + 0x50;
            if ((value | PREC) != 0) {
                if (base == 0) {
                    do { text--; *B(text) = (uint8_t)((value & 7) + 0x30); value >>= 3; } while (value != 0);
                    if ((FLAGS & 1) != 0 && S8(text) != 0x30) { text--; *B(text) = 0x30; }
                } else if (base == 1) {
                    while (value >= 10) {
                        DIVU(c, value, 10);
                        text--;
                        *B(text) = (uint8_t)(c->hi + 0x30);
                        value = c->lo;
                    }
                    text--;
                    *B(text) = (uint8_t)(value + 0x30);
                } else {
                    do { text--; *B(text) = *B(digits + (value & 0xF)); value >>= 4; } while (value != 0);
                }
            }
            *W(sp + 120) = digits;
            c->r[21] = text;
            if (base == 1 && (FLAGS & 0x200) != 0) {
                ARG(0) = sp + 0x28; ARG(3) = sp + 0x24; ARG(1) = text; ARG(2) = sp + 0x50;
                c->r[31] = 0x089F6078u; f_089F54CC(c);
                text = c->r[2];
            } else {
                *W(sp + 36) = sp + 0x50 - text;
            }
            by_precision = (int32_t)*W(sp + 36) < (int32_t)PREC;
        }
        c->r[21] = text;

        /* Out it goes: blanks, the sign or 0x, zeroes, the text, blanks. */
        int32_t sign = S8(sp + 32);
        uint32_t whole = by_precision ? *W(sp + 128) : *W(sp + 36);
        if (sign != 0) whole += 1;
        else if ((FLAGS & 2) != 0) whole += 2;
        *W(sp + 116) = whole;
        if (*W(sp + 112) == 0) {
            static const uint32_t back[6] = { 0x089F6160u, 0x089F6194u, 0x089F61DCu, 0x089F621Cu, 0x089F6250u, 0x089F62A8u };
            c->r[19] = WIDTH - whole;
            if ((int32_t)c->r[19] > 0) pad(c, fp, *W(sp + 88), 19, back);
        }
        c->r[19] = *W(sp + 108);
        if (S8(sp + 32) != 0) {
            emit(c, fp, sp + 0x20, 1, 0x089F62ECu, 0x089F631Cu, 0x089F636Cu);
        } else if ((FLAGS & 2) != 0) {
            *B(sp + 80) = 0x30;
            *B(sp + 81) = (uint8_t)c->r[16];
            emit(c, fp, sp + 0x50, 2, 0x089F63B0u, 0x089F63E0u, 0x089F6434u);
        }
        c->r[20] = 1;
        if (*W(sp + 112) == 0x80) {
            static const uint32_t back[6] = { 0x089F648Cu, 0x089F64C0u, 0x089F6508u, 0x089F6548u, 0x089F6578u, 0x089F65C4u };
            c->r[16] = WIDTH - *W(sp + 116);
            if ((int32_t)c->r[16] > 0) pad(c, fp, c->r[19], 16, back);
        }
        {
            static const uint32_t back[6] = { 0x089F6610u, 0x089F6644u, 0x089F668Cu, 0x089F66CCu, 0x089F6700u, 0x089F6750u };
            c->r[16] = *W(sp + 128) - *W(sp + 36);
            if ((int32_t)c->r[16] > 0) pad(c, fp, c->r[19], 16, back);
        }
        emit(c, fp, text, (int32_t)*W(sp + 36), 0x089F6784u, 0x089F67B4u, 0x089F67F8u);
        if (*W(sp + 124) != 0) {
            static const uint32_t back[6] = { 0x089F6850u, 0x089F6884u, 0x089F68CCu, 0x089F690Cu, 0x089F6938u, 0x089F697Cu };
            c->r[16] = WIDTH - *W(sp + 116);
            if ((int32_t)c->r[16] > 0) {
                c->r[19] = *W(sp + 88);
                pad(c, fp, c->r[19], 16, back);
            }
        }
        uint32_t counted = *W(sp + 116);
        if ((int32_t)counted < (int32_t)WIDTH) counted = WIDTH;
        *W(sp + 132) = *W(sp + 132) + counted;
    }
#undef FMT
#undef AP
#undef FLAGS
#undef WIDTH
#undef PREC
    if ((S16(fp + 12) & 0x200) == 0) {
        ARG(0) = (uint32_t)S16(fp + 14); ARG(1) = 0; ARG(2) = 0; ARG(3) = 1;
        c->r[31] = 0x089F69C0u; f_089F569C(c);
    }
    c->r[2] = *W(sp + 132);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9];
    c->r[29] += 0xB0;
}

/* (stream, format, arguments): the format is looked through for an item that needs floating
   point (e, f, g, E, G or L after a %'s flags and sizes): with one the full printing runs, else
   the one without. A 0x20 frame. */
SFA_FUNC(089F3FF0, lib_vfprintf_pick) {
    gaddr stream = ARG(0), format = ARG(1);
    uint32_t args = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = format;
    int32_t ch = S8(p);
    c->r[7] = *W(stream + 84);
    *W(c->r[29] + 16) = ra;
    int floats = 0;
    while (ch != 0) {
        if (ch == 0x25 && S8(p + 1) != 0) {
            p++;
            ch = S8(p);
            while (ch < 65 && S8(p + 1) != 0) { p++; ch = S8(p); }
            if (ch == 'E' || ch == 'G' || ch == 'L' || ch == 'e' || ch == 'f' || ch == 'g') { floats = 1; break; }
        }
        p++;
        ch = S8(p);
    }
    c->r[8] = format; c->r[9] = args;
    ARG(0) = c->r[7]; ARG(1) = stream; ARG(2) = format; ARG(3) = args;
    if (floats) { c->r[31] = 0x089F409Cu; f_089F40E0(c); }
    else { c->r[31] = 0x089F40D4u; f_089F5820(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (to, format, ...): sprintf: a stream that writes to memory at `to` with no end is made in the
   frame (at +20), the arguments given in registers are put where the rest follow (+120), the
   printing runs, and the text is closed with a 0. A 0x90 frame. */
SFA_FUNC(089F31AC, lib_sprintf) {
    gaddr to = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x90;
    gaddr sp = c->r[29];
    *W(sp + 120) = ARG(2); *W(sp + 124) = ARG(3); *W(sp + 128) = c->r[8]; *W(sp + 132) = c->r[9];
    *W(sp + 136) = c->r[10]; *W(sp + 140) = c->r[11];
    *W(sp + 20) = to;
    *H(sp + 32) = 0x208;
    *W(sp + 36) = to;
    *W(sp + 28) = 0x7FFFFFFFu;
    *W(sp + 40) = 0x7FFFFFFFu;
    *W(sp + 104) = *W(REENT);
    *W(sp + 108) = ra;
    ARG(2) = sp + 0x78;
    ARG(0) = sp + 0x14;
    c->r[31] = 0x089F3210u; f_089F3FF0(c);
    ARG(0) = *W(sp + 20);
    *B(ARG(0)) = 0;
    c->r[31] = ra; c->r[29] += 0x90;
}
