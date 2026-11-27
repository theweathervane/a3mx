/* Larger steps of objects and fighters (0.104.0): object drawing and registration, hit boxes, the
 * fighters pushed apart, a pool's make, set-ups by screen, and a particle field. Offsets stay numbers
 * until something shows what they are. Arguments of native callees are set as the original passes them;
 * s0 - s4 as the original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u
#define DRAW    0x08AD9320u     /* the sprite drawer's state */

#include "../guest_mem.h"

void f_08855D80(Cpu *c);   /* intro_place */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_0880E434(Cpu *c);   /* screen_draw_shape */
void f_089E0CF4(Cpu *c);   /* tex_use_list */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_08821330(Cpu *c);   /* object_routine_by_14 */
void f_08821368(Cpu *c);   /* game_random */
void f_088BE334(Cpu *c);   /* random_of_36 */
void f_088BE36C(Cpu *c);   /* random_of_eight */
void f_0880C308(Cpu *c);   /* game_draw_sprite */
void f_0880C4C8(Cpu *c);   /* sprite_drawer_one */
void f_0880E6E8(Cpu *c);   /* game_sprite_anchor */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }

/* ---------------------------------------------------------------- drawing */

/* (object): drawn: its sprites before (+52, +48), itself at its place (x + its +268, 240 - y) with its
   facing (+11), colour (+160 - +163), +168 and +24, then its sprites after (+56, +44, +40). Nothing while
   +0 or +1 is clear. */
SFA_FUNC(0880BA94, object_draw_with_parts) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17];
    if (*B(o) == 0 || *B(o + 1) == 0) return;
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[17] = DRAW;
    gaddr p;
    if ((p = *W(o + 52)) != 0) { c->r[4] = p; f_0880C308(c); }
    if ((p = *W(o + 48)) != 0) { c->r[4] = p; f_0880C308(c); }
    *H(DRAW + 80) = (uint16_t)S16(o + 18);
    *H(DRAW + 82) = (uint16_t)(0xF0 - S16(o + 22));
    c->r[4] = DRAW; c->r[5] = o;
    f_0880E6E8(c);
    *H(DRAW) = (uint16_t)(S16(DRAW + 80) + S16(o + 268));
    *H(DRAW + 2) = (uint16_t)S16(DRAW + 82);
    *W(DRAW + 56) = *B(o + 11) & 1u;
    *W(DRAW + 60) = (uint32_t)*B(o + 11) >> 1;
    *W(DRAW + 48) = ((uint32_t)*B(o + 160) << 24) | ((uint32_t)*B(o + 161) << 16) | ((uint32_t)*B(o + 162) << 8) | *B(o + 163);
    *B(DRAW + 85) = (uint8_t)S8(o + 168);
    *W(DRAW + 44) = *H(o + 24);
    c->r[4] = DRAW; c->r[5] = o;
    f_0880C4C8(c);
    gaddr after = *W(o + 56);
    c->r[17] = after;
    if (after != 0) { c->r[4] = after; f_0880C308(c); }
    if ((p = *W(o + 44)) != 0) { c->r[4] = p; f_0880C308(c); }
    gaddr last = *W(o + 40);
    c->r[16] = last;
    if (last != 0) { c->r[4] = last; f_0880C308(c); }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (place, record): the fading shape: the record's +12 picks shape 3 * (n / 24) + (n % 24) / 8 and its
   alpha n % 8 (n the low 12 bits of the word at its +4), drawn 64 left and 16 up of the record's place
   (+2, +4), and again 256 higher unless the record's +1 is set. */
SFA_FUNC_WRAPPED(0880DA78, draw_fading_shape, ism_presentation_fading_shape) {
    gaddr place = ARG(0), r = ARG(1);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    *H(place) = (uint16_t)(S16(r + 2) - 64);
    *H(place + 2) = (uint16_t)(S16(r + 4) - 16);
    c->r[16] = r; c->r[17] = place;
    c->r[4] = *W(0x08E75A18u + 28); c->r[5] = 0x08E70000u;
    f_089E0CF4(c);
    uint32_t n = *H(*W(r + 12) + 4) & 0xFFF;
    DIV(c, n, 0x18);
    uint32_t q = c->lo, rem = c->hi;
    uint32_t alpha = rem & 7, shape = q * 3 + (uint32_t)((int32_t)rem >> 3);
    *W(place + 44) = 7;
    c->r[16] = alpha; c->r[17] = place; c->r[18] = shape; c->r[19] = r;
    c->r[4] = place; c->r[5] = shape; c->r[6] = alpha; c->r[7] = q * 2;
    f_0880E434(c);
    if (*B(r + 1) == 0) {
        *H(place + 2) = (uint16_t)(S16(place + 2) - 0x100);
        c->r[4] = place; c->r[5] = shape; c->r[6] = alpha;
        f_0880E434(c);
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (object): put on its lists: by +121 on 0x09517700 (count 0x095177C0) or 0x09517760 (0x095177C1), and by
   +9 on 0x092E2300 and 0x092E23C8 (count 0x092E22F8, copied to 0x092E23C0) or 0x092E2360 / 0x092E2428
   (0x092E22F9, 0x092E23C1). */
SFA_FUNC(08821600, object_put_on_lists) {
    gaddr o = ARG(0);
    gaddr list = S8(o + 121) == 0 ? 0x09517700u : 0x09517760u;
    gaddr count = S8(o + 121) == 0 ? 0x095177C0u : 0x095177C1u;
    *W(list + (uint32_t)*B(count) * 4) = o;
    *B(count) = (uint8_t)(*B(count) + 1);
    uint32_t side = *B(o + 9) != 0;
    gaddr n = 0x092E22F8u + side;
    *W(0x092E2300u + side * 0x60 + (uint32_t)*B(n) * 4) = o;
    *W(0x092E23C8u + side * 0x60 + (uint32_t)*B(n) * 4) = o;
    uint8_t k = (uint8_t)(*B(n) + 1);
    *B(n) = k;
    *B(0x092E23C0u + side) = k;
}

/* ---------------------------------------------------------------- making objects */

/* (owner, type, sub): an object made from the twelve 0x168-byte slots by the owner's +121 (0x09510F28
   down, free count 0x095176F0; or 0x09512008 down, 0x095176F1, and +345 = 1): in use, its type, +86 from
   the owner, white, still. The answer is it, or 0 when all twelve are taken. */
SFA_FUNC(08822D2C, object_make_12) {
    gaddr o = ARG(0);
    uint32_t type = ARG(1) & 0xFF, sub = ARG(2) & 0xFF;
    int second = S8(o + 121) != 0;
    gaddr at = second ? 0x09512008u : 0x09510F28u;
    int n;
    for (n = 12; n > 0; n--, at -= 0x168) if (*B(at) == 0) break;
    if (n == 0) { RETURN(0); return; }
    gaddr cnt = second ? 0x095176F1u : 0x095176F0u;
    *B(cnt) = (uint8_t)(S8(cnt) - 1);
    *B(at) = 1;
    *B(at + 2) = (uint8_t)type;
    *B(at + 3) = (uint8_t)sub;
    *B(at + 345) = (uint8_t)second;
    *W(at + 148) = 0;
    *H(at + 86) = *H(o + 86);
    *W(at + 164) = 0;
    *B(at + 163) = 0xFF; *B(at + 162) = 0xFF; *B(at + 161) = 0xFF; *B(at + 160) = 0xFF;
    *B(at + 168) = 0; *B(at + 169) = 0;
    *W(at + 108) = 0; *W(at + 104) = 0; *W(at + 100) = 0; *W(at + 96) = 0;
    *W(at + 20) = 0; *W(at + 16) = 0;
    RETURN(at);
}

/* (): when pool 4 has 5 or more free, five kind-0x23 objects (types 3, 4, 6, 7, 10) with their places and
   speeds; the last follows the camera layer 0x092E8800. */
SFA_FUNC(088BD334, make_five_23) {
    if (S8(0x095176F3u) < 5) return;
    gaddr s16 = c->r[16];
    c->r[29] -= 0x20;
    gaddr x;
    c->r[4] = 0x23; c->r[5] = 3; f_08822EAC(c); x = c->r[2];
    *W(x + 16) = 0xFFC00000u; *W(x + 20) = 0x800000u; *W(x + 96) = 0x40000u; *W(x + 104) = 0xFFFFF000u; *H(x + 132) = 0x30;
    c->r[16] = 0x800000u;
    c->r[4] = 0x23; c->r[5] = 4; f_08822EAC(c); x = c->r[2];
    *W(x + 16) = 0x1B00000u; *W(x + 20) = 0x800000u; *W(x + 96) = 0xFFFC0000u; *W(x + 104) = 0x1000; *H(x + 132) = 0x140;
    c->r[4] = 0x23; c->r[5] = 6; f_08822EAC(c); x = c->r[2];
    *H(x + 18) = 0; *H(x + 22) = 0x18; *W(x + 96) = 0x20000u;
    c->r[4] = 0x23; c->r[5] = 7; f_08822EAC(c); x = c->r[2];
    *H(x + 18) = S8(*W(0x09525DE8u) + 14) < 3 ? 0x1A0 : 0x160;
    *H(x + 22) = 0xE8; *W(x + 96) = 0xFFFE0000u;
    c->r[4] = 0x23; c->r[5] = 0xA; f_08822EAC(c); x = c->r[2];
    *B(x + 10) = 0x25; *W(x + 88) = 0x092E8800u;
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- set-ups */

/* (object): placed by the intro (layer 5, -0x90, 0x78), +4 on, kind 6; +73 by the fight's state (BATTLE +
   16, + 5, + 32 and GAME + 137: 2 - 5), the matching run of 0x08A16F34 copied into 0x08A16750's entries up
   to the first value not above 0; animation 0x08A16B18. */
SFA_FUNC(08856B38, intro_setup_73) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = 5; c->r[6] = 0xFFFFFF70u; c->r[7] = 0x78;
    f_08855D80(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 1;
    *H(o + 86) = 6;
    *W(o + 164) = 1;
    uint32_t mode, k;
    if (S8(BATTLE + 16) != 0) { mode = 2; k = 0; }
    else if (S8(BATTLE + 5) == 0) { mode = 3; k = 2; }
    else if (S8(BATTLE + 32) != 0 && S8(GAME + 137) == 0) { mode = 5; k = 5; }
    else { mode = 4; k = 6; }
    *B(o + 73) = (uint8_t)mode;
    gaddr to = 0x08A16750u;
    int16_t v;
    do {
        v = S16(0x08A16F34u + k * 2);
        k = (k + 1) & 0xFFFF;
        *H(to + 4) = (uint16_t)v;
        to += 6;
    } while (v > 0);
    set_anim(c, o, *W(0x08A16B0Cu + 12));
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): set up as kind 0x14 at (0xC0, 0x80) with +32 = GAME + 16's low byte, +72 = 0x1B; its
   animation by +3 (6: 0x1A, 8: 0x1B, 9: 0x1C, else 0) from 0x08A2ED04, or 0x08A2EBA0 when 0x09525DE3 is set. */
SFA_FUNC(088AF9DC, setup_14_anim_by_3) {
    gaddr o = ARG(0);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 32) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *W(o + 148) = 0;
    *H(o + 86) = 0x14;
    *W(o + 164) = 1;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 82) = 0;
    *B(o + 12) = 0; *B(o + 13) = 0;
    *H(o + 24) = 1; *H(o + 26) = 2;
    *H(o + 18) = 0xC0; *H(o + 22) = 0x80;
    *B(o + 72) = 0x1B;
    *H(o + 34) = 0;
    uint32_t t = *B(o + 3), k = 0;
    if (t == 6) k = 0x1A; else if (t == 8) k = 0x1B; else if (t == 9) k = 0x1C;
    gaddr table = S8(0x09525DE3u) == 0 ? 0x08A2ED04u : 0x08A2EBA0u;
    c->r[29] -= 0x20;
    set_anim(c, o, *W(table + k * 4));
    c->r[29] += 0x20;
}

/* (object): its place by GAME + 6 (0 - 4) and, by GAME + 4 and GAME + 6, shown (+1) with an animation from
   0x08A3C87C and a place, as the owner's (+88) +1 / +2 say. */
SFA_FUNC(088CF388, setup_by_screen_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    uint32_t s6 = *H(GAME + 6);
    gaddr w = *W(o + 88);
    static const uint16_t XY[5][2] = { { 0xD8, 0x71 }, { 0xB0, 0x6A }, { 0xB0, 0x70 }, { 0xCE, 0x71 }, { 0xC0, 0x6A } };
    if (s6 < 5) { *H(o + 18) = XY[s6][0]; *H(o + 22) = XY[s6][1]; }
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 1) = 0;
    const gaddr T = 0x08A3C87Cu;
    if (*H(GAME + 4) == 0) {
        *B(o + 1) = 1;
        set_anim(c, o, *W(T + (uint32_t)*H(GAME + 6) * 4 + 36));
    } else {
        int32_t s = (int32_t)*H(GAME + 6);
        if (s == 0) {
            if (S8(w + 1) == 0) { *B(o + 1) = 1; set_anim(c, o, *W(T + 72)); *H(o + 18) = 0xA8; *H(o + 22) = 0x58; }
        } else if (s == 2) {
            if (S8(w + 2) == 1) { *B(o + 1) = 1; set_anim(c, o, *W(T + 68)); *H(o + 18) = 0xB0; *H(o + 22) = 0x71; }
        } else if (s == 3) {
            if (S8(w + 1) == 1) { *B(o + 1) = 1; set_anim(c, o, *W(T + 64)); *H(o + 18) = 0xA8; *H(o + 22) = 0x71; }
        }
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): shown (+93) on the first screen, or on screen 2 / 3 when the record (+124) says so; place,
   size and colours (0x80, 0, 0x80); +26 = 0x56 on the first screen's state 2, else 0x28. The answer is 1. */
SFA_FUNC(088D0354, setup_panel_by_screen) {
    gaddr o = ARG(0), r = *W(o + 124);
    RETURN(1);
    uint8_t shown = *H(GAME + 4) == 0;
    uint32_t s6 = *H(GAME + 6);
    if (s6 == 2 && S8(r + 2) == 1) shown = 1;
    else if (s6 == 3 && S8(r + 1) == 1) shown = 1;
    *B(o + 93) = shown;
    int first = *H(GAME + 4) == 0 && *H(GAME + 6) == 2;
    *B(o + 136) = 0x80;
    *H(o + 6) = 0x2C; *H(o + 10) = 0x86;
    for (int k = 36; k <= 39; k++) *B(o + k) = 0x80;
    for (int k = 42; k <= 45; k++) *B(o + k) = 0;
    for (int k = 48; k <= 51; k++) *B(o + k) = 0x80;
    *H(o + 14) = 0x128;
    *H(o + 26) = first ? 0x56 : 0x28;
}

/* (object): +128 on, shown (+93), colours by +92 (0xB: 0x1D / 0x11 / 0xF7 and +55 = 1; 0xC: 0xFA / 0x20 /
   0x0E; 0xD: white), +94 = GAME + 16's low byte, size 0x180 x 0xF0. */
SFA_FUNC(088B2A5C, setup_band_by_92) {
    gaddr o = ARG(0);
    *H(o + 94) = (uint16_t)(S16(GAME + 16) & 0xFF);
    *B(o + 128) = (uint8_t)(*B(o + 128) + 1);
    *B(o + 93) = 1;
    *B(o + 55) = 0; *B(o + 56) = 0; *B(o + 1) = 0; *B(o + 3) = 0;
    *W(o + 4) = 0;
    *W(o + 8) = 0xFF0000u;
    *B(o + 136) = 0xFF;
    uint32_t k = *B(o + 92), a = 0, b = 0, d = 0;
    if (k == 0xB) { *B(o + 55) = 1; a = 0x1D; b = 0x11; d = 0xF7; }
    else if (k == 0xC) { a = 0xFA; b = 0x20; d = 0x0E; }
    else if (k == 0xD) { a = b = d = 0xFF; }
    if (k >= 0xB && k <= 0xD) {
        for (int i = 36; i <= 39; i++) *B(o + i) = (uint8_t)a;
        for (int i = 42; i <= 45; i++) *B(o + i) = (uint8_t)b;
        for (int i = 48; i <= 51; i++) *B(o + i) = (uint8_t)d;
    }
    *H(o + 12) = 0;
    *H(o + 24) = 0;
    *H(o + 14) = 0x180;
    *H(o + 26) = 0xF0;
}

/* (object): 66 particles (16-byte records at 0x08B0285C) move by their speeds, which move by their
   accelerations, and carry their places (0x08A3709E, 6 bytes each) by 1/256 of that; after 8 frames
   (+82) +4 moves on, GAME + 10's high byte is set and the places are put back from 0x08B02C7C. */
SFA_FUNC(088B520C, particles_66) {
    gaddr o = ARG(0);
    for (uint32_t k = 0; k < 66; k++) {
        gaddr r = 0x08B0285Cu + k * 16, p = 0x08A3709Eu + k * 6;
        int16_t s4 = S16(r + 4), s12 = S16(r + 12);
        *W(r) += (uint32_t)(int32_t)s4;
        *H(r + 4) = (uint16_t)(s4 + S16(r + 6));
        *W(r + 8) += (uint32_t)(int32_t)s12;
        *H(r + 12) = (uint16_t)(s12 + S16(r + 14));
        *H(p) = (uint16_t)(S16(p) + ((int32_t)*W(r) >> 8));
        *H(p + 2) = (uint16_t)(S16(p + 2) + ((int32_t)*W(r + 8) >> 8));
    }
    *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
    if (S16(o + 82) < 8) return;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(GAME + 10) = (uint16_t)(*H(GAME + 10) | 0xFF00);
    for (uint32_t k = 0; k < 66; k++) {
        gaddr from = 0x08B02C7Cu + k * 6, to = 0x08A3709Eu + k * 6;
        *H(to) = (uint16_t)S16(from);
        *H(to + 2) = (uint16_t)S16(from + 2);
        *H(to + 4) = *H(from + 4);
    }
}

/* ---------------------------------------------------------------- fighters */

/* (other, fighter): 1 while the other is in range of the fighter's boxes (its frame record +28's chain
   by +6, boxes from +224 by each entry's second byte of +416's table +16): horizontally within the box's
   half width + 0x58; a chain entry with +1 bits 6 - 7 ends it with the distance under 160 either way. 0 when
   +307 bit 7 is set, or out of range at the end. */
SFA_FUNC(088627D0, fighter_in_box_range) {
    gaddr a = ARG(0), f = ARG(1);
    gaddr rec = *W(f + 28);
    gaddr tab = *W(*W(f + 416) + 16);
    gaddr e = tab + (uint32_t)*H(rec + 8) * 12;
    if (*B(f + 307) & 0x80) { RETURN(0); return; }
    gaddr boxes = *W(f + 224);
    int facing = *B(f + 11) != 0;
    int16_t xf = S16(f + 18), xa = S16(a + 18);
    RETURN(1);
    uint32_t box = *B(e + 1);
    for (;;) {
        if (box != 0) {
            gaddr bx = boxes + (box << 5);
            int16_t off = S16(bx);
            if (facing) off = (int16_t)-off;
            int16_t d = (int16_t)((int16_t)(off + xf) - xa);
            if (d < 0) d = (int16_t)-d;
            int32_t reach = (S16(bx + 4) + 0x58) & 0xFFFF;
            if (!(reach < (int32_t)(uint16_t)d)) return;
        }
        if (S8(rec + 1) & 0xC0) {
            int32_t dx = (int16_t)(xf - xa + 0xA0) & 0xFFFF;
            if (dx < 321) return;
            RETURN(0);
            return;
        }
        rec = rec + (uint32_t)(S16(rec + 6) * 10);
        box = *B(tab + (uint32_t)*H(rec + 8) * 12 + 1);
    }
}

/* (fighter): unless +671, the fighters pushed apart: +238 of the opponent says how (1: they overlapped,
   both moved by the overlap; 2: past the screen's width, both moved back), the margin 0x18 when
   0x08A0588A is set. The answer is the routine pointer 0x08821330 gave. */
SFA_FUNC(08862FF0, fighters_push_apart) {
    gaddr f = ARG(0), opp = *W(f + 92);
    gaddr s[5] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20] };
    int32_t margin = S8(0x08A0588Au) != 0 ? 0x18 : 0;
    c->r[29] -= 0x30;
    c->r[16] = f; c->r[18] = (uint32_t)margin; c->r[19] = opp;
    if (S8(f + 671) == 0) {
        *B(opp + 238) = 0;
        gaddr left, right;
        if (S16(f + 18) < S16(opp + 18)) { left = f; right = opp; } else { left = opp; right = f; }
        c->r[17] = right; c->r[20] = left;
        c->r[4] = f;
        f_08821330(c);
        int32_t edge = S16(c->r[2] + 18);
        c->r[20] = (uint32_t)edge;
        int16_t d = (int16_t)(S16(left + 18) - S16(left + 232) - edge - margin);
        if (d < 0) {
            *H(f + 18) = (uint16_t)(S16(f + 18) - d);
            *H(opp + 18) = (uint16_t)(S16(opp + 18) - d);
            *B(opp + 238) = 1;
        } else {
            int16_t over = (int16_t)(S16(right + 18) - edge + S16(right + 232) + margin - 0x180);
            if (over >= 0) {
                *H(f + 18) = (uint16_t)(S16(f + 18) - over);
                *H(opp + 18) = (uint16_t)(S16(opp + 18) - over);
                *B(opp + 238) = 2;
            }
        }
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3]; c->r[20] = s[4];
}

/* (a, b, box a, box b): whether the boxes meet: their x (+0, mirrored by each one's facing) and y (+2)
   from each one's place, within the sum of half widths (+4, less 2) and half heights (+6). b's +714 -
   +720 keep the four places. */
SFA_FUNC(08873728, boxes_meet) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    int16_t xa = *B(a + 11) ? (int16_t)(S16(a + 18) - S16(ba)) : (int16_t)(S16(ba) + S16(a + 18));
    int16_t xb = *B(b + 11) ? (int16_t)(S16(b + 18) - S16(bb)) : (int16_t)(S16(bb) + S16(b + 18));
    *H(b + 714) = (uint16_t)xa;
    *H(b + 716) = (uint16_t)xb;
    int16_t d = (int16_t)(xb - xa);
    if (d < 0) d = (int16_t)-d;
    int16_t w = (int16_t)(S16(ba + 4) + S16(bb + 4) - 2);
    if (w < 0) w = 0;
    if (w < d) { RETURN(0); return; }
    uint32_t ya = (uint32_t)(S16(ba + 2) + S16(a + 22)), yb = (uint32_t)(S16(bb + 2) + S16(b + 22));
    *H(b + 718) = (uint16_t)ya;
    *H(b + 720) = (uint16_t)yb;
    int16_t dy = (int16_t)((int16_t)yb - (int16_t)ya);
    if (dy < 0) dy = (int16_t)-dy;
    int16_t h = (int16_t)(S16(ba + 6) + S16(bb + 6));
    RETURN(h < dy ? 0 : 1);
}

/* (a, b, box a, box b): as 0x08873728, the x offsets negated by facing, both sums 4 larger when b's +386
   is -1; +718 / +720 keep the y places. */
SFA_FUNC(08873BF0, boxes_meet_386) {
    gaddr a = ARG(0), b = ARG(1), ba = ARG(2), bb = ARG(3);
    int wide = S8(b + 386) == -1;
    int16_t oa = S16(ba), ob = S16(bb);
    if (*B(a + 11)) oa = (int16_t)-oa;
    if (*B(b + 11)) ob = (int16_t)-ob;
    int16_t xa = (int16_t)(oa + S16(a + 18)), xb = (int16_t)(ob + S16(b + 18));
    int16_t d = (int16_t)(xb - xa);
    if (d < 0) d = (int16_t)-d;
    int16_t w = (int16_t)(S16(ba + 4) + S16(bb + 4));
    if (wide) w = (int16_t)(w + 4);
    if (w < d) { RETURN(0); return; }
    uint32_t ya = (uint32_t)(S16(ba + 2) + S16(a + 22)), yb = (uint32_t)(S16(bb + 2) + S16(b + 22));
    *H(b + 718) = (uint16_t)ya;
    *H(b + 720) = (uint16_t)yb;
    int16_t dy = (int16_t)((int16_t)yb - (int16_t)ya);
    if (dy < 0) dy = (int16_t)-dy;
    int16_t h = (int16_t)(S16(ba + 6) + S16(bb + 6));
    if (wide) h = (int16_t)(h + 4);
    RETURN(h < dy ? 0 : 1);
}

/* (fighter): a random character (0x088BE334, not the one last picked for its side at 0x09525DEC) and
   its ISM (+386: at random for 4 / 6, -1 for 0x12 / 0x14, else one of eight not the side's last at
   0x09525DEE); +376 / +449 = 2 * (ISM + 1). */
SFA_FUNC(088BE210, random_character_and_ism) {
    gaddr f = ARG(0);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    int32_t side = S8(f + 337);
    *B(f + 376) = 2;
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[19] = 0x09525DECu;
    f_088BE334(c);
    gaddr last_ch = 0x09525DECu + (uint32_t)side, last_ism = 0x09525DEEu + (uint32_t)side;
    c->r[18] = last_ch; c->r[17] = last_ism;
    uint32_t ch = c->r[2];
    if ((uint32_t)(int32_t)S8(last_ch) == ch) { f_088BE334(c); ch = c->r[2]; }
    *B(f + 338) = (uint8_t)ch;
    *B(last_ch) = (uint8_t)ch;
    int32_t k = S8(f + 338);
    if (k == 4 || k == 6) {
        f_08821368(c);
        uint32_t v = c->r[2] & 1;
        *B(last_ism) = (uint8_t)v;
        *B(f + 386) = (uint8_t)v;
    } else if (k == 0x12 || k == 0x14) {
        *B(last_ism) = 0xFF;
        *B(f + 386) = 0xFF;
    } else {
        f_088BE36C(c);
        uint32_t v = c->r[2];
        if ((uint32_t)(int32_t)S8(last_ism) == v) { f_088BE36C(c); v = c->r[2]; }
        *B(f + 386) = (uint8_t)v;
        *B(last_ism) = (uint8_t)v;
    }
    uint32_t mark = ((uint32_t)(S8(f + 386) + 1)) << 1;
    *B(f + 376) = (uint8_t)mark;
    *B(f + 449) = (uint8_t)mark;
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* ---------------------------------------------------------------- more objects and records */

void f_08881F70(Cpu *c);   /* make_beside_15 (small_steps.c) */

/* (object): moving towards its target (+92, +94) at speeds +96 / +98 changing by +100 / +102, on the axes
   +104 / +105 switch on; an axis that reaches or passes it sets its bit in +6 and clears its fraction. Then
   with any bit set, +5 moves on, +6 = 0, +73 = 2 and the bit for +3 is set in GAME + 10's low byte. */
SFA_FUNC(088B42D0, move_to_target) {
    gaddr o = ARG(0);
    for (int axis = 0; axis < 2; axis++) {
        if (S8(o + 104 + axis) == 0) continue;
        gaddr sp = o + 96 + axis * 2;
        *H(sp) = (uint16_t)(S16(sp) + S16(sp + 4));
        int32_t v = S16(sp);
        *W(o + 16 + axis * 4) += (uint32_t)v << 8;
        int16_t diff = (int16_t)(S16(o + 18 + axis * 4) - S16(o + 92 + axis * 2));
        if (diff != 0 && (int16_t)(diff ^ v) < 0) continue;
        uint8_t bits = *B(o + 6), bit = (uint8_t)(1 << axis);
        if ((bits & bit) == 0) { *B(o + 6) = (uint8_t)(bits | bit); *H(o + 16 + axis * 4) = 0; }
    }
    if (*B(o + 6) == 0) return;
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *B(o + 6) = 0;
    *B(o + 73) = 2;
    uint32_t bit = 1u << (*B(o + 3) & 31), now = *H(GAME + 10);
    if (((now & 0xFF) & bit) == 0) *H(GAME + 10) = (uint16_t)(now | bit);
}

/* (record): +66 - +84 unpacked (+99 = 1, +103 the top four bits of +66, +112 - +132, +120 the flags of
   +74, +308); three slots at 0x0951F950 (0x98 apart) take a value from 0x08AB7A20 by two bits each
   (+120 bits 4 - 5, 2 - 3, 0 - 1) and are on unless +120 bits 10 / 9 / 8 are set. */
SFA_FUNC(089DFA54, unpack_record_66) {
    gaddr r = ARG(0);
    *B(r + 99) = 1;
    *B(r + 103) = (uint8_t)((S16(r + 66) >> 12) & 0xF);
    *H(r + 112) = (uint16_t)S16(r + 68);
    *H(r + 114) = (uint16_t)S16(r + 70);
    *H(r + 116) = (uint16_t)S16(r + 72);
    *H(r + 126) = (uint16_t)S16(r + 76);
    *H(r + 128) = (uint16_t)S16(r + 78);
    *H(r + 130) = (uint16_t)S16(r + 80);
    *H(r + 132) = (uint16_t)S16(r + 82);
    *W(r + 120) = (uint32_t)(int32_t)S16(r + 74);
    *W(r + 308) = (uint32_t)(int32_t)S16(r + 84);
    const gaddr T = 0x08AB7A20u, S = 0x0951F950u;
    uint32_t f = *W(r + 120);
    *B(S + 28) = (uint8_t)*H(T + (uint32_t)(((int32_t)(f & 0x30) >> 4) * 2));
    *B(S) = (*W(r + 120) & 0x400) ? 0 : 1;
    *B(S + 0x98 + 28) = (uint8_t)*H(T + (uint32_t)(((int32_t)(*W(r + 120) & 0xC) >> 2) * 2));
    *B(S + 0x98) = (*W(r + 120) & 0x200) ? 0 : 1;
    *B(S + 0x130 + 28) = (uint8_t)*H(T + (*W(r + 120) & 3) * 2);
    *B(S + 0x130) = (*W(r + 120) & 0x100) ? 0 : 1;
}

/* (fighter, k): a step of its walk: the step's move from the table by +119 and k (0x08A18D04), read at
   the count +584 from 0x08A18C8C (0x08A18EC4 with +588); a negative entry ends it (the answer 1).
   Otherwise the count moves on, +541 = 1, and x (+18) moves by it (backwards unless +134, doubled when
   +112 is negative); the answer 0. On odd counts, unless +238 or +73, a kind-15 object is made beside it. */
SFA_FUNC(08863E1C, fighter_walk_step) {
    gaddr f = ARG(0);
    uint32_t k = ARG(1);
    gaddr s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = 1; c->r[18] = k;
    int32_t n = S16(f + 584);
    *B(f + 541) = 0;
    if ((n & 1) && S8(f + 238) == 0 && S8(f + 73) == 0) {
        c->r[4] = f;
        f_08881F70(c);
        n = S16(f + 584);
    }
    int32_t idx = S8(0x08A18D04u + (uint32_t)*B(f + 119) * 2 + k);
    c->r[18] = (uint32_t)idx << 2;
    gaddr tab = S8(f + 588) == 0 ? 0x08A18C8Cu : 0x08A18EC4u;
    int32_t v = S8(*W(tab + (uint32_t)(idx * 4)) + (uint32_t)n);
    if (v < 0) {
        RETURN(1);
    } else {
        *H(f + 584) = (uint16_t)(n + 1);
        *B(f + 541) = 1;
        int16_t move = S8(f + 134) == 0 ? (int16_t)-v : (int16_t)v;
        if (S16(f + 112) < 0) move = (int16_t)(move * 2);
        *H(f + 18) = (uint16_t)(S16(f + 18) + move);
        RETURN(0);
    }
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (record): the objects of its list (+268's entry for +4, 32 bytes each, the last with bit 7 of its
   first byte): each made (kind = byte 0 & 0x7F, type byte 1) with its facing, place, speeds and fields
   from the entry, kind 0x19, +12 = 0xFF, and +88 the one made before (the first: what s0 held). */
SFA_FUNC(089DFED4, make_objects_from_list) {
    gaddr r = ARG(0);
    gaddr s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    gaddr e = *W(*W(r + 268) + (uint32_t)(S16(r + 4) * 4) - 4);
    gaddr prev = c->r[16];
    c->r[29] -= 0x30;
    c->r[17] = 0x19; c->r[18] = 0xFFFFFFFFu;
    for (;;) {
        c->r[19] = e;
        uint32_t kind = (uint32_t)S8(e) & 0x7F, type = *B(e + 1);
        c->r[4] = kind; c->r[5] = type; c->r[6] = type;
        f_08822EAC(c);
        gaddr x = c->r[2];
        *B(x + 11) = (uint8_t)S8(e + 2);
        *B(x + 116) = *B(e + 3);
        *H(x + 24) = (uint16_t)S8(e + 4);
        *B(x + 9) = (uint8_t)(S8(e + 5) >> 1);
        *B(x + 13) = (uint8_t)S8(e + 6);
        *B(x + 117) = (uint8_t)S8(e + 7);
        *H(x + 18) = (uint16_t)S16(e + 8);
        *H(x + 22) = (uint16_t)S16(e + 10);
        *H(x + 112) = (uint16_t)S16(e + 12);
        *H(x + 114) = (uint16_t)S16(e + 14);
        *W(x + 96) = (uint32_t)(int32_t)S16(e + 16);
        *W(x + 100) = (uint32_t)(int32_t)S16(e + 18);
        *W(x + 104) = (uint32_t)(int32_t)S16(e + 20);
        *W(x + 108) = (uint32_t)(int32_t)S16(e + 22);
        *B(x + 118) = (uint8_t)S8(e + 24);
        *B(x + 119) = (uint8_t)S8(e + 25);
        *B(x + 120) = (uint8_t)S8(e + 26);
        *B(x + 80) = (uint8_t)S8(e + 27);
        *B(x + 136) = (uint8_t)S8(e + 28);
        *B(x + 137) = (uint8_t)S8(e + 29);
        *B(x + 138) = (uint8_t)S8(e + 30);
        *B(x + 12) = 0xFF;
        *B(x + 73) = (uint8_t)S8(e + 31);
        *H(x + 86) = 0x19;
        *W(x + 148) = 0;
        *W(x + 88) = prev;
        prev = x;
        c->r[16] = x;
        if (S8(e) & 0x80) break;
        e += 0x20;
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}
