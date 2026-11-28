/* Two more of the library: the system's font opened, and the end of the sound core's start, where
   every voice gets a neutral pitch, envelope and volume. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define FONT_LIB 0x08E65C68u /* the font library's handle */
#define FONT     0x08E65C6Cu /* the open font's handle */

void f_089F3470(Cpu *c);
void hle_sceFontNewLib(Cpu *c); void hle_sceFontGetNumFontList(Cpu *c); void hle_sceFontGetFontList(Cpu *c);
void hle_sceFontFindOptimumFont(Cpu *c); void hle_sceFontOpen(Cpu *c); void hle_sceFontGetFontInfo(Cpu *c);
void hle___sceSasSetPitch(Cpu *c); void hle___sceSasSetSimpleADSR(Cpu *c); void hle___sceSasSetVolume(Cpu *c);

static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* The system's font is opened: a font library (its parameters at 0x08AD7ED0), the list of fonts
   (to 0x08E65C70), the font that best fits a height of 10.0, opened (once more with 2 at the
   style's +26 when the first try fails), and its information read to 0x08E67020; the word at
   0x08AD7EFC is then set. Any failure leaves it clear. A 0xD0 frame keeping s0..s3, with the
   error word at +16 and the style wanted from +20. */
SFA_FUNC(089E4C0C, font_open) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0xD0;
    gaddr sp = c->r[29];
    gaddr err = sp + 0x10, style = sp + 0x14;
    *W(sp + 192) = s1; *W(sp + 188) = s0; *W(sp + 196) = s2; *W(sp + 200) = s3; *W(sp + 204) = ra;
    c->r[17] = err;
    call2(c, 0x08AD7ED0u, err, hle_sceFontNewLib, 0x089E4C38u);
    c->r[16] = 0x08E60000u;
    *W(FONT_LIB) = c->r[2];
    do {
        if (*W(err) != 0) break;
        call2(c, *W(FONT_LIB), err, hle_sceFontGetNumFontList, 0x089E4C54u);
        if (*W(err) != 0) break;
        call3(c, *W(FONT_LIB), 0x08E65C70u, c->r[2], hle_sceFontGetFontList, 0x089E4C74u);
        c->r[18] = style;
        if (*W(err) != 0) break;
        call3(c, style, 0, 0xA8, f_089F3470, 0x089E4C90u);
        *H(sp + 46) = 0;
        *H(sp + 40) = 0;
        *H(sp + 42) = 0;
        c->f[12] = 0x41200000u;
        *W(sp + 20) = 0x41200000u;
        call3(c, *W(FONT_LIB), style, err, hle_sceFontFindOptimumFont, 0x089E4CB8u);
        uint32_t index = c->r[2];
        c->r[19] = index;
        if (*W(err) != 0) break;
        ARG(3) = err;
        call3(c, *W(FONT_LIB), index, 0, hle_sceFontOpen, 0x089E4CD8u);
        c->r[18] = 0x08E60000u;
        *W(FONT) = c->r[2];
        if (*W(err) != 0) {
            *H(sp + 46) = 2;
            ARG(3) = err;
            call3(c, *W(FONT_LIB), index, 0, hle_sceFontOpen, 0x089E4D08u);
            *W(FONT) = c->r[2];
            if (*W(err) != 0) break;
        }
        call2(c, *W(FONT), 0x08E67020u, hle_sceFontGetFontInfo, 0x089E4D24u);
        *W(err) = c->r[2];
        if (c->r[2] != 0) break;
        *W(0x08AD7EFCu) = 1;
    } while (0);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0xD0;
}

/* The rest of the sound core's start (s0 the voice, r4 saying whether to go on; s1 and s3 point
   at the page the started flag is on, s2 at the core's): each of the 32 voices gets the pitch
   0x1000, the short envelope (0xF, 0x5FC0) and silence, each call only while the flag is set;
   then the flag is set and 0 returned from a 0x20 frame that kept s0..s3. */
SFA_FUNC(089ECA14, sound_core_start_rest) {
    for (;;) {
        int more = ARG(0) != 0;
        c->r[3] = *W(c->r[17] + 12288);
        if (!more) break;
        ARG(1) = c->r[16];
        ARG(0) = c->r[18] + 0x3040;
        ARG(2) = 0x1000;
        if (c->r[3] != 0) {
            c->r[31] = 0x089ECA50u; hle___sceSasSetPitch(c);
            c->r[8] = *W(c->r[17] + 12288);
            ARG(1) = c->r[16];
            ARG(0) = c->r[18] + 0x3040;
            ARG(2) = 0xF;
            ARG(3) = 0x5FC0;
            if (c->r[8] != 0) {
                c->r[31] = 0x089ECA70u; hle___sceSasSetSimpleADSR(c);
                c->r[10] = *W(c->r[19] + 12288);
                ARG(1) = c->r[16];
                ARG(0) = c->r[18] + 0x3040;
                ARG(2) = 0; ARG(3) = 0; c->r[8] = 0;
                c->r[9] = 0;
                if (c->r[10] != 0) {
                    c->r[16] = c->r[16] + 1;
                    c->r[31] = 0x089ECA98u; hle___sceSasSetVolume(c);
                    ARG(0) = (int32_t)c->r[16] < 32;
                    continue;
                }
            }
        }
        c->r[16] = c->r[16] + 1;
        ARG(0) = (int32_t)c->r[16] < 32;
    }
    gaddr sp = c->r[29];
    ARG(1) = 1;
    *W(c->r[17] + 12288) = 1;
    c->r[3] = 0;
    c->r[31] = *W(sp + 16);
    c->r[19] = *W(sp + 12);
    c->r[18] = *W(sp + 8);
    c->r[17] = *W(sp + 4);
    c->r[16] = *W(sp);
    c->r[2] = 0;
    c->r[29] += 0x20;
}
