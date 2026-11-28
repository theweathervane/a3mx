/* The end of a frame's drawing in the library: the screens swapped, and the plain text that was
   asked for during the frame drawn straight into the picture. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GE       0x08E69300u /* the drawing state: +8 whether the screen is shown, +20 the video
                                memory, +36 and +40 the screen's format and width, +44 and +48
                                the two screens */
#define TEXT_SET 0x08E69C38u /* which of the two sets of text is being filled */
#define TEXT     0x08E69C3Cu /* the sets: 0x4000 bytes each, 16 bytes a letter (x, y, colour,
                                letter) */
#define TEXT_N   0x08E71C3Cu /* how many letters each set holds */
#define FONT     0x08A02560u /* eight bytes a letter, a row each, the left dot the low bit */

void f_089E7C88(Cpu *c);
void hle_sceDisplaySetFrameBuf(Cpu *c);

/* The text's drawing: every letter of the set being filled that lies wholly inside the picture
   is drawn dot by dot (8 by 8) in its colour, as 16-bit dots or, in format 3, 32-bit ones; then
   the set is emptied. The picture's format, its two addresses, its width in memory and its size
   are read from 0x08E71C44 on and kept at the bottom of the 0x50 frame, which keeps s0..s8.
   `from` says where the routine is entered: 0 at its start, 1 where a row of a letter begins
   (the original's 0x089EA9D4), 2 after a dot (0x089EAA30); the registers hold the rest. */
static void text_draw(Cpu *c, int from) {
    gaddr sp;
    if (from == 1) { sp = c->r[29]; goto row_begin; }
    if (from == 2) { sp = c->r[29]; goto dot_done; }
    c->r[9] = 0x08E70000u;
    c->r[7] = *W(TEXT_SET);
    c->r[29] -= 0x50;
    sp = c->r[29];
    *W(sp + 64) = c->r[30]; *W(sp + 60) = c->r[23]; *W(sp + 56) = c->r[22]; *W(sp + 52) = c->r[21];
    *W(sp + 48) = c->r[20]; *W(sp + 44) = c->r[19]; *W(sp + 40) = c->r[18]; *W(sp + 36) = c->r[17];
    *W(sp + 32) = c->r[16];
    c->r[18] = *W(TEXT_N + (c->r[7] << 2));
    c->r[25] = TEXT + (c->r[7] << 14);
    if (c->r[18] == 0) goto done;
    c->r[19] = *W(0x08E71C54u);
    c->r[16] = *W(0x08E71C48u);
    c->r[14] = *W(0x08E71C4Cu);
    c->r[12] = *W(0x08E71C44u);
    c->r[22] = *W(0x08E71C58u);
    *W(sp + 16) = c->r[19];
    c->r[7] = 0;
    *W(sp) = c->r[16];
    *W(sp + 4) = c->r[14];
    *W(sp + 8) = c->r[12];
    *W(sp + 12) = *W(0x08E71C50u);
    c->r[10] = 0;
    for (;;) {
        c->r[15] = *W(c->r[25] + 4);
        c->r[30] = c->r[15] + 7;
        c->r[23] = (int32_t)c->r[30] < (int32_t)c->r[22];
        c->r[24] = *W(c->r[25]);
        if (c->r[23] == 0) goto letter_done;
        if (!((int32_t)(c->r[24] + 7) < (int32_t)*W(sp + 16) && (int32_t)c->r[15] >= 0)) goto letter_done;
        c->r[23] = 4;
        if ((int32_t)c->r[24] < 0) goto letter_done;
        c->r[14] = (uint32_t)S8(c->r[25] + 12);
        c->r[9] = *W(c->r[25] + 8);
        c->r[20] = *W(sp);
        c->r[17] = *W(sp + 4);
        c->r[16] = *W(sp + 8);
        c->r[19] = *W(sp + 12);
        c->r[13] = 0;
        c->r[21] = FONT + (c->r[14] << 3);
        for (;;) {
            /* A row: the letter's rows come four to a word, the mask starting again with each. */
            if (c->r[13] == 0) c->r[10] = *W(c->r[21]);
            else if (c->r[13] == c->r[23]) c->r[10] = *W(c->r[21] + 4);
            else { c->r[8] = c->r[15] + c->r[13]; goto row_place; }
row_begin:
            c->r[7] = 1;
            c->r[8] = c->r[15] + c->r[13];
row_place:
            {
                int64_t m = (int64_t)(int32_t)c->r[8] * (int32_t)c->r[19];
                c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
            }
            c->r[12] = 3;
            c->r[8] = c->r[20];
            c->r[11] = 0x40000000u;
            c->r[14] = (uint32_t)((int32_t)c->r[9] >> 16);
            ARG(0) = c->r[24] + c->lo;
            c->r[2] = (ARG(0) << 1) + c->r[17] + c->r[16];
            c->r[3] = (ARG(0) << 2) + c->r[17] + c->r[16];
            c->r[30] = (ARG(0) << 2) + c->r[17];
            c->r[6] = 7;
            for (;;) {
                ARG(1) = c->r[10] & c->r[7];
                if (c->r[8] == 3) {
                    gaddr dot = 0x40000000u | (c->r[3] & 0x1FFFFFFFu);
                    if (ARG(1) != 0) {
                        *H(dot + 2) = (uint16_t)c->r[14];
                        *H(dot) = (uint16_t)c->r[9];
                    }
                } else {
                    gaddr dot = 0x40000000u | (c->r[2] & 0x1FFFFFFFu);
                    if (ARG(1) != 0) *H(dot) = (uint16_t)c->r[9];
                }
dot_done:
                c->r[6]--;
                c->r[7] <<= 1;
                c->r[3] += 4;
                c->r[2] += 2;
                if ((int32_t)c->r[6] < 0) break;
            }
            c->r[13]++;
            c->r[11] = (int32_t)c->r[13] < 8;
            if (c->r[11] == 0) break;
        }
letter_done:
        c->r[18]--;
        c->r[25] += 0x10;
        if (c->r[18] == 0) break;
    }
    c->r[10] = 0x08E70000u;
    c->r[25] = *W(TEXT_SET);
    c->r[7] = 0x08E70000u;
    c->r[24] = TEXT_N;
    c->r[22] = c->r[25] << 2;
    c->r[18] = c->r[22] + TEXT_N;
    *W(c->r[18]) = 0;
done:
    c->r[30] = *W(sp + 64); c->r[23] = *W(sp + 60); c->r[22] = *W(sp + 56); c->r[21] = *W(sp + 52);
    c->r[20] = *W(sp + 48); c->r[19] = *W(sp + 44); c->r[18] = *W(sp + 40); c->r[17] = *W(sp + 36);
    c->r[16] = *W(sp + 32);
    c->r[29] += 0x50;
}

SFA_FUNC(089EA8BC, text_flush) { text_draw(c, 0); }
SFA_FUNC(089EA9D4, text_flush_from_row) { text_draw(c, 1); }
SFA_FUNC(089EAA30, text_flush_from_dot) { text_draw(c, 2); }

/* The frame's drawing ends: the text is drawn, the two screens change places and, when the
   screen is shown (+8 at 1), the one just drawn is put up; the routine's end follows. A 0x10
   frame keeping s0. */
SFA_FUNC(089E7C4C, screens_swap) {
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = c->r[16];
    *W(sp + 4) = c->r[31];
    c->r[31] = 0x089E7C60u; f_089EA8BC(c);
    ARG(0) = 0x08E70000u;
    c->r[16] = GE;
    c->r[8] = *W(GE + 44);
    c->r[3] = *W(GE + 48);
    ARG(1) = *W(GE + 8);
    ARG(0) = 1;
    c->r[7] = 0;
    *W(GE + 44) = c->r[3];
    *W(GE + 48) = c->r[8];
    if (ARG(1) == 1) {
        c->r[2] = *W(GE + 20);
        ARG(1) = *W(GE + 40);
        ARG(2) = *W(GE + 36);
        ARG(0) = c->r[2] + c->r[8];
        c->r[31] = 0x089E7CC8u; hle_sceDisplaySetFrameBuf(c);
    }
    f_089E7C88(c);
}

/* The frame's drawing ends, and the screen now being drawn to is kept at 0x08E65C4C. A 0x10
   frame. */
SFA_FUNC(089E48B4, frame_draw_end) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x089E48C4u; f_089E7C4C(c);
    ARG(0) = 0x08E60000u;
    *W(0x08E65C4Cu) = c->r[2];
    c->r[31] = ra; c->r[29] += 0x10;
}
