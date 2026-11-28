/* Names drawn with the system's font: a name is read from UTF-8, drawn into a strip of 256 by 32,
   kept in a list, and shown; with the link's lists of players that use them. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_089E4A08(Cpu *c); void f_089E4A18(Cpu *c); void f_089F3470(Cpu *c); void f_089E4AE4(Cpu *c);
void f_089E4D8C(Cpu *c); void f_088DC18C(Cpu *c); void f_089E4F28(Cpu *c); void f_088DD84C(Cpu *c);
void f_088DD9BC(Cpu *c); void f_088DDAEC(Cpu *c); void f_088DDCBC(Cpu *c); void f_088DD114(Cpu *c);
void f_088E4B08(Cpu *c); void f_088DDD30(Cpu *c); void f_088DD23C(Cpu *c); void f_088DDC30(Cpu *c);
void f_088DD22C(Cpu *c); void f_0880E788(Cpu *c); void f_0880A6CC(Cpu *c);
void hle_sceFontGetCharInfo(Cpu *c); void hle_sceFontGetCharGlyphImage_Clip(Cpu *c);
void hle_sceKernelDcacheWritebackRange(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (v): 1 when v fits sixteen bits. */
SFA_FUNC(089E4A08, fits_16_bits) {
    c->r[5] = 0xFFFF;
    c->r[2] = (0xFFFFu < ARG(0)) ^ 1;
}

/* (place): the next character of the UTF-8 text the place points at, and the place moved past
   it. A byte that cannot start a character gives the word at 0x08AD7CE4; a character past
   sixteen bits gives 0xFF1F. A frame keeping s0, s1. */
SFA_FUNC(089E4A18, utf8_next) {
    gaddr place = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = place;
    gaddr p = *W(place);
    *W(sp + 16) = s0;
    uint32_t ch = *B(p);
    *W(sp + 24) = ra;
    p++;
    if (ch < 0x80) *W(place) = p;
    else {
        if (ch < 0xC2 || !(ch < 0xF5)) ch = *W(0x08AD7CE4u);
        else {
            ch &= 0x3F;
            uint32_t limit = 0x800;
            for (;;) {
                ch = (ch << 6) | (*B(p) & 0x3F);
                p++;
                if (ch < limit) break;
                ch &= limit - 1;
                limit <<= 5;
            }
        }
        *W(place) = p;
    }
    c->r[16] = ch;
    c->r[17] = 0xFF1F;
    call1(c, ch, f_089E4A08, 0x089E4AC4u);
    if (c->r[2] != 0) c->r[17] = ch & 0xFFFF;
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (to, text): at most nine characters of the text as sixteen-bit words with a 0 after them; a
   longer one ends in 0x2026, and none at all (or no text) gives three of 0xFF1F. A 0x30 frame
   keeping s0..s3, with the text's place at sp + 16. */
SFA_FUNC(089E4AE4, name_to_words) {
    gaddr to = ARG(0);
    gaddr text = ARG(1);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 24) = sv[1];
    *W(sp + 16) = text;
    c->r[17] = 0;
    *W(sp + 20) = sv[0]; *W(sp + 28) = sv[2]; *W(sp + 32) = sv[3]; *W(sp + 36) = sv[4];
    c->r[16] = to;
    uint32_t n = 0;
    if (text != 0) {
        c->r[19] = sp + 0x10; c->r[18] = 9;
        while (n != 9) {
            call1(c, sp + 0x10, f_089E4A18, 0x089E4B1Cu);
            if ((c->r[2] & 0xFFFF) == 0) break;
            *H(to) = (uint16_t)c->r[2];
            n++; to += 2;
            c->r[17] = n; c->r[16] = to;
        }
    }
    if (text != 0 && n == 9) {
        *H(to - 4) = 0x2026;
        *H(to - 2) = 0;
    } else if (n == 0) {
        *H(to) = 0xFF1F; *H(to + 2) = 0xFF1F; *H(to + 4) = 0xFF1F;
        *H(to + 6) = 0;
    } else *H(to) = 0;
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (buffer, words, w, h): the words drawn into a buffer of w by h, four bits a dot (half of w * h
   bytes, cleared first): each character's place comes from the font (the handle at 0x08E65C6C),
   from the left edge and the line whose height is the word at 0x08E67028 plus 0x80; a 0xD, 0xA
   pair starts a new line 0x578 lower. The buffer is flushed at the end; a failing font call
   leaves at once. A 0xB0 frame keeping s0..s7 and fp. */
SFA_FUNC(089E4D8C, words_draw) {
    gaddr buf = ARG(0), text = ARG(1);
    uint32_t w = ARG(2), h = ARG(3);
    uint32_t sv[10];
    for (int i = 0; i < 8; i++) sv[i] = c->r[16 + i];
    sv[8] = c->r[30]; sv[9] = c->r[31];
    c->r[29] -= 0xB0;
    gaddr sp = c->r[29];
    int64_t m = (int64_t)(int32_t)h * (int32_t)w;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    uint32_t half = (uint32_t)((int32_t)c->lo / 2);
    for (int i = 0; i < 8; i++) *W(sp + 124 + 4 * i) = sv[i];
    *W(sp + 116) = buf;
    *W(sp + 120) = half;
    *W(sp + 156) = sv[8]; *W(sp + 160) = sv[9];
    c->r[20] = h; c->r[19] = w; c->r[16] = text; c->r[17] = buf;
    call3(c, buf, 0, half, f_089F3470, 0x089E4DF0u);
    *W(sp + 32) = 0;
    *H(sp + 44) = (uint16_t)w;
    *H(sp + 46) = (uint16_t)h;
    *H(sp + 48) = (uint16_t)((int32_t)w / 2);
    *H(sp + 50) = 0;
    *W(sp + 52) = buf;
    uint32_t line = *W(0x08E67020u + 8) + 0x80;
    uint32_t x = 0;
    uint32_t ch = *H(text);
    int failed = 0;
    c->r[18] = line; c->r[17] = 0;
    if (ch != 0) {
        c->r[21] = w - 2; c->r[30] = 0xA; c->r[23] = sp + 0x38; c->r[22] = sp + 0x20;
        c->r[20] = h - 2; c->r[19] = 0x08E60000u;
        while (ch != 0) {
            if (ch == 0xD && *H(text + 2) == 0xA) {
                text += 4;
                x = 0;
                line += 0x578;
            } else {
                c->r[16] = text; c->r[17] = x; c->r[18] = line;
                call3(c, *W(0x08E65C6Cu), ch, sp + 0x38, hle_sceFontGetCharInfo, 0x089E4E74u);
                if (c->r[2] != 0) { failed = 1; break; }
                *W(sp + 36) = x + (*W(sp + 64) << 6);
                *W(sp + 40) = line - (*W(sp + 68) << 6);
                ARG(3) = 0; c->r[8] = 0; c->r[9] = w - 2; c->r[10] = h - 2;
                call3(c, *W(0x08E65C6Cu), *H(text), sp + 0x20, hle_sceFontGetCharGlyphImage_Clip, 0x089E4EBCu);
                if (c->r[2] != 0) { failed = 1; break; }
                text += 2;
                x += *W(sp + 104);
            }
            ch = *H(text);
        }
    }
    if (!failed) call2(c, *W(sp + 116), *W(sp + 120), hle_sceKernelDcacheWritebackRange, 0x089E4EF8u);
    for (int i = 0; i < 8; i++) c->r[16 + i] = sv[i];
    c->r[30] = sv[8]; c->r[31] = sv[9]; c->r[29] += 0xB0;
}

/* (buffer, text): the text's name drawn into the buffer as a strip of 256 by 32. Nothing without
   a buffer. A 0x220 frame keeping s0..s2, with the name's words at sp + 16. */
SFA_FUNC(089E4F28, name_draw) {
    gaddr buf = ARG(0), text = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x220;
    gaddr sp = c->r[29];
    *W(sp + 532) = s1; c->r[17] = buf;
    *W(sp + 528) = s0; *W(sp + 536) = s2; *W(sp + 540) = ra;
    c->r[16] = text;
    if (buf != 0) {
        c->r[18] = sp + 0x10;
        call3(c, sp + 0x10, 0, 0x200, f_089F3470, 0x089E4F5Cu);
        call2(c, sp + 0x10, text, f_089E4AE4, 0x089E4F68u);
        ARG(3) = 0x20;
        call3(c, buf, sp + 0x10, 0x100, f_089E4D8C, 0x089E4F7Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x220;
}

/* (text): a name's record: 0x10 bytes from 0x088DC18C, with a buffer of 0x1020 at +4 that the
   name is drawn into (from its first place of 32), the text at +8 and 1 at +13; it goes to the
   front of the list at 0x08B02EA8. Returns it, or 0 without a text or without room. A frame
   keeping s0, s1. */
SFA_FUNC(088DD9BC, name_record_new) {
    gaddr text = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = text;
    uint32_t rec = 0;
    if (text != 0) {
        call1(c, 0x10, f_088DC18C, 0x088DD9DCu);
        rec = c->r[2];
        c->r[17] = rec;
        if (rec != 0) {
            call3(c, rec, 0, 0x10, f_089F3470, 0x088DD9F4u);
            call1(c, 0x1020, f_088DC18C, 0x088DD9FCu);
            gaddr buf = c->r[2];
            *W(rec + 4) = buf;
            if (buf != 0) {
                call2(c, (buf & 0xFFFFFFE0u) + 0x20, text, f_089E4F28, 0x088DDA1Cu);
                gaddr head = *W(0x08B02EA8u);
                if (head != 0) *W(rec) = head;
                *W(0x08B02EA8u) = rec;
                *B(rec + 13) = 1;
                *W(rec + 8) = text;
                *B(rec + 14) = 0;
                *B(rec + 15) = 0;
            } else {
                call1(c, rec, f_088DD84C, 0x088DDA58u);
                rec = 0;
            }
        }
    }
    c->r[2] = rec;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object, x, y, a, b): the object's name (its text at +8, the record kept at +144 and made the
   first time) shown at (x, y) with a and b. A 0x40 frame keeping s0..s4. */
SFA_FUNC(088DDC30, object_name_show) {
    gaddr o = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 5; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 52) = sv[5];
    c->r[20] = (uint32_t)(int32_t)(int16_t)ARG(1);
    c->r[19] = (uint32_t)(int32_t)(int16_t)ARG(2);
    c->r[18] = ARG(3) & 0xFF;
    c->r[17] = c->r[8] & 0xFF;
    c->r[16] = o;
    if (o != 0) {
        if (*W(o + 144) == 0) {
            call1(c, o + 8, f_088DD9BC, 0x088DDC80u);
            *W(o + 144) = c->r[2];
        }
        ARG(3) = c->r[18]; c->r[8] = c->r[17];
        call3(c, *W(o + 144), c->r[20], c->r[19], f_088DDAEC, 0x088DDC9Cu);
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x40;
}

/* (i, text): the i-th of the sixteen kept names at 0x09523C20, made from the text the first time
   (its +15 is then i + 1). Returns its record, 0 past the sixteenth. A frame keeping s0, s1. */
SFA_FUNC(088DDCBC, kept_name) {
    uint32_t i = ARG(0);
    gaddr text = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1;
    gaddr at = 0x09523C20u + (i << 2);
    c->r[17] = at;
    *W(sp + 16) = s0;
    c->r[16] = i;
    uint32_t rec = *W(at);
    *W(sp + 24) = ra;
    ARG(0) = text;
    if (!((int32_t)i < 16)) rec = 0;
    else if (rec == 0) {
        call0(c, f_088DD9BC, 0x088DDD08u);
        rec = c->r[2];
        if (rec != 0) {
            *W(at) = rec;
            *B(rec + 15) = (uint8_t)(i + 1);
        }
    }
    c->r[2] = rec;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (i, text, x, y, a, b): the i-th kept name shown at (x, y) with a and b. A 0x40 frame keeping
   s0..s3. */
SFA_FUNC(088DDD30, kept_name_show) {
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 48) = sv[4];
    c->r[19] = (uint32_t)(int32_t)(int16_t)ARG(2);
    c->r[18] = (uint32_t)(int32_t)(int16_t)ARG(3);
    c->r[17] = c->r[8] & 0xFF;
    c->r[16] = c->r[9] & 0xFF;
    ARG(0) = ARG(0) & 0xFF;
    call0(c, f_088DDCBC, 0x088DDD68u);
    ARG(1) = c->r[19];
    if (c->r[2] != 0) {
        ARG(3) = c->r[17]; c->r[8] = c->r[16];
        call3(c, c->r[2], c->r[19], c->r[18], f_088DDAEC, 0x088DDD84u);
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x40;
}

/* (p, i): the text of the i-th entry of p: one of the 42 of 20 bytes at 0x08A3ED44 by the byte
   at +64 of its 24-byte record, and the one at 0x08A3ED40 when the entry is not in use (+254) or
   the byte is past them. */
SFA_FUNC(088E4B08, entry_text) {
    gaddr p = ARG(0);
    uint32_t i = ARG(1);
    c->r[2] = 0x08A3ED40u;
    if (S8(p + i + 254) != 0) {
        uint32_t v = *B(p + i * 24 + 64);
        if ((int32_t)v < 42) c->r[2] = v * 20 + 0x08A3ED44u;
    }
}

/* (p, n, x, y, a, b): the n-th line of a list of players at (x, y + 5): past the players there
   are (0x088DD114) a kept name, the entry's text when p's +45 is set and one of the fixed texts
   of 0x08A3F0D0 otherwise; among them the player's own name (0x088DD23C). A 0x40 frame keeping
   s0..s6. */
SFA_FUNC(088E4B5C, player_line_show) {
    gaddr p = ARG(0);
    uint32_t n = ARG(1);
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    for (int i = 0; i < 7; i++) *W(sp + 32 + 4 * i) = sv[i];
    *W(sp + 60) = sv[7];
    uint32_t x = (uint32_t)(int32_t)(int16_t)ARG(2);
    uint32_t y = (uint32_t)(int32_t)(int16_t)(ARG(3) + 5);
    uint32_t a = c->r[8] & 0xFF, b = c->r[9] & 0xFF;
    c->r[19] = c->r[9]; c->r[18] = c->r[8]; c->r[20] = ARG(3); c->r[21] = ARG(2);
    c->r[16] = n; c->r[17] = p;
    call0(c, f_088DD114, 0x088E4B9Cu);
    int32_t players = (int32_t)c->r[2];
    c->r[21] = x; c->r[20] = y; c->r[19] = b; c->r[18] = a;
    if (players < (int32_t)n) {
        uint32_t text;
        uint32_t back;
        c->r[22] = n & 0xFF;
        if (*B(p + 45) != 0) {
            call2(c, p, n, f_088E4B08, 0x088E4BDCu);
            text = c->r[2];
            back = 0x088E4BF8u;
        } else {
            text = *W(0x08A3F0D0u + ((n - (uint32_t)players) << 2) - 4);
            back = 0x088E4C30u;
        }
        ARG(3) = y; c->r[8] = b; c->r[9] = a;
        call3(c, n & 0xFF, text, x, f_088DDD30, back);
    } else {
        call1(c, n, f_088DD23C, 0x088E4C40u);
        ARG(3) = b; c->r[8] = a;
        call3(c, c->r[2], x, y, f_088DDC30, 0x088E4C58u);
    }
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x40;
}

/* (object): the link's own line: the name of 0x088DD22C's object at (5, 0xD1), a box
   (0x0880E788), and the text the object's +44 names in the table 0x08A3EA28. A 0x30 frame keeping
   s0. */
SFA_FUNC(088D68C8, link_own_line) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    call0(c, f_088DD22C, 0x088D68DCu);
    ARG(3) = 0; c->r[8] = 0x10;
    call3(c, c->r[2], 5, 0xD1, f_088DDC30, 0x088D68F4u);
    ARG(3) = 0xDE; c->r[8] = 0xC0000026u; c->r[9] = 0xE;
    call3(c, 2, 0xC9, 0xA4, f_0880E788, 0x088D6914u);
    ARG(3) = 0xB8; c->r[8] = 9;
    call3(c, *W(0x08A3EA28u + ((uint32_t)*B(o + 44) << 2)), 1, 4, f_0880A6CC, 0x088D6940u);
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}
