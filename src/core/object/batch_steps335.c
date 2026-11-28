/* What the library's small pieces set free: the pause that ends a fight, a round's last frames,
   a countdown with the music's tempo, a thrown piece's way out, and four of the library's own
   callers of the callback tables. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08821F9C(Cpu *c); void f_08822F8C(Cpu *c); void f_0888D5B0(Cpu *c); void f_08819A20(Cpu *c);
void f_0884AE40(Cpu *c); void f_0881DF10(Cpu *c); void f_0880AEE4(Cpu *c); void f_08809BE4(Cpu *c);
void f_08837690(Cpu *c); void f_0880B6CC(Cpu *c); void f_08824AA0(Cpu *c); void f_08826A64(Cpu *c);
void f_08826A14(Cpu *c); void f_088D5E44(Cpu *c); void f_08821368(Cpu *c); void f_089E5A00(Cpu *c);
void f_089E5804(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (x, y): a fight's frame while it can be left: once the top bit of the byte at 0x092E510F is set
   (and x's +145 is clear), x's +6 moves on, y's +37 is -1, 0x08821F9C runs, outside mode 2
   0x08822F8C (2, or 3 past the first mode), the five clean-ups, and unless y's +104 is set the
   music's tempo 0xF0 and the byte at 0x092DA22B. Then 0x08837690 unless x's +145 is set, and
   0x0880B6CC. A frame keeping s0, s1. */
SFA_FUNC(08824438, fight_leave) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    uint32_t done = *B(0x092E5100u + 15) & 0x80;
    enter2(c, sv);
    c->r[17] = x; c->r[16] = y;
    if (done && S8(x + 145) == 0) {
        uint32_t phase = (uint32_t)*H(x + 6) + 1;
        *H(x + 6) = (uint16_t)phase;
        *B(y + 37) = 0xFF;
        call2(c, phase, 0xFFFFFFFFu, f_08821F9C, 0x08824488u);
        if (S8(x + 114) != 2) {
            if (S8(y + 79) != 0) call2(c, 3, 0, f_08822F8C, 0x088244C0u);
            else call2(c, 2, 0, f_08822F8C, 0x088244B0u);
        }
        call0(c, f_0888D5B0, 0x088244C8u);
        call0(c, f_08819A20, 0x088244D0u);
        call0(c, f_0884AE40, 0x088244D8u);
        call0(c, f_0881DF10, 0x088244E0u);
        call0(c, f_0880AEE4, 0x088244E8u);
        if (S8(y + 104) == 0) {
            call1(c, 0xF0, f_08809BE4, 0x088244FCu);
            *B(0x092DA1B0u + 123) = 1;
        }
    }
    if (S8(x + 145) == 0) call0(c, f_08837690, 0x08824520u);
    call0(c, f_0880B6CC, 0x08824528u);
    leave2(c, sv);
}

/* (x, y): the music's tempo 0x96, then 0x08824AA0. A frame keeping s0, s1. */
SFA_FUNC(088250F0, tempo_96_then_24aa0) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = x; c->r[17] = y;
    call1(c, 0x96, f_08809BE4, 0x08825110u);
    call2(c, x, y, f_08824AA0, 0x0882511Cu);
    leave2(c, sv);
}

/* (x, y): a round's last frames. On the deciding round (y's +2 against twice +8 less one), with
   the fight over (y + 5) or in the fights through lists (y + 155), the music's tempo goes to
   0x12C. Unless y's +70 or x's +50 or +69 is set, x's +43 and +72 are cleared. Then one of two
   endings: 0x08826A64 after a draw (y + 76), for the far side's winner (y + 112) in the kind 3,
   and for a cpu winner when 0x088D5E44 says 0; 0x08826A14 for the others. A frame keeping
   s0..s2. */
SFA_FUNC(088268D4, round_last_frames) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2;
    gaddr winner = *W(y + 112);
    c->r[18] = winner;
    int32_t last = (S8(y + 8) << 1) - 1;
    int32_t round = S16(y + 2);
    *W(sp + 20) = s1; c->r[17] = x;
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = y;
    if (round == last || S8(y + 5) != 0 || S8(y + 155) != 0) call1(c, 0x12C, f_08809BE4, 0x08826928u);
    if (S8(y + 70) == 0 && S8(x + 50) == 0 && S8(x + 69) == 0) { *B(x + 43) = 0; *B(x + 72) = 0; }
    if (S8(y + 76) != 0) call2(c, x, y, f_08826A64, 0x08826968u);
    else if (S16(y + 56) == 3) {
        if (S8(winner + 337) & 2) call2(c, x, y, f_08826A64, 0x0882699Cu);
        else call2(c, x, y, f_08826A14, 0x088269ACu);
    } else {
        call2(c, (uint32_t)(int32_t)S16(y + 56), 3, f_088D5E44, 0x088269BCu);
        if (c->r[2] != 0) call2(c, x, y, f_08826A14, 0x088269CCu);
        else if (S8(winner + 373) != 0) call2(c, x, y, f_08826A64, 0x088269ECu);
        else call2(c, x, y, f_08826A14, 0x088269FCu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object, n): a piece flies off by its kind (+3; nothing below 19): 19 and 20 up and to one side
   (20 faces right), 21 and 22 straight up or down, the others across; the speeds are doubled by
   its +73, mirrored by its +78, and its place is shaken by a random amount. It gets 0x30 frames
   (+82). Returns n plus its +73 (n for the kinds below 19). A 0x10 frame keeping s0..s2. */
SFA_FUNC(0887EC58, piece_fly_off) {
    gaddr o = ARG(0);
    uint32_t n = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp + 4) = s1; c->r[17] = n;
    uint32_t kind = *B(o + 3);
    *W(sp) = s0; *W(sp + 8) = s2; *W(sp + 12) = ra;
    c->r[16] = o;
    if (!(kind < 19)) {
        uint32_t shift = (uint32_t)S8(o + 73) & 31;
        if (kind == 0x13 || kind == 0x14) {
            int32_t x = S16(o + 18);
            uint32_t vy;
            int32_t shake;
            if (kind == 0x13) { vy = 0x48000u << shift; shake = -0x80; }
            else {
                *B(o + 11) = 1;
                vy = 0xFFFB8000u << shift; shake = 0x80;
            }
            uint32_t vx = 0x90000u << shift;
            int32_t dx = -0x100;
            if (S8(o + 78) != 0) { vx = 0 - vx; vy = 0 - vy; dx = 0x100; shake = -shake; }
            c->r[18] = (uint32_t)shake;
            *W(o + 96) = vx;
            *W(o + 100) = vy;
            *H(o + 18) = (uint16_t)(x + dx);
            call1(c, vy, f_08821368, 0x0887ED0Cu);
            *H(o + 22) = (uint16_t)(((uint32_t)shake + c->r[2]) & 0xFF);
        } else if (kind == 0x15 || kind == 0x16) {
            int32_t x = S16(o + 18);
            c->r[18] = (uint32_t)x;
            uint32_t vy = (kind == 0x15 ? 0xFFFA0000u : 0x60000u) << shift;
            *W(o + 96) = 0;
            *W(o + 100) = vy;
            call1(c, vy, f_08821368, 0x0887ED5Cu);
            *H(o + 18) = (uint16_t)((uint32_t)x + (c->r[2] & 0xFF));
        } else {
            *W(o + 100) = 0;
            uint32_t vx = 0x90000u << shift;
            int32_t x = -0x100;
            if (S8(o + 78) != 0) { vx = 0 - vx; x = 0x100; }
            *W(o + 96) = vx;
            *H(o + 18) = (uint16_t)x;
            int32_t y = S16(o + 22);
            c->r[18] = (uint32_t)y;
            call1(c, vx, f_08821368, 0x0887EDA0u);
            *H(o + 22) = (uint16_t)((uint32_t)y + (c->r[2] & 0x7F));
        }
        n += (uint32_t)(int32_t)S8(o + 73);
        *H(o + 82) = 0x30;
    }
    c->r[2] = n;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x10;
}

/* (x): a countdown at x + 16: the music's tempo goes to 0x1E when it stands at 0x3C, it counts
   down, and below 0 x's +4 moves on with +6 cleared. */
SFA_FUNC(088BC0E4, countdown_with_tempo) {
    gaddr x = ARG(0);
    uint32_t sv[2];
    int32_t at = S16(x + 16);
    enter(c, x, sv);
    if (at == 0x3C) call1(c, 0x1E, f_08809BE4, 0x088BC108u);
    int32_t t = S16(x + 16) - 1;
    *H(x + 16) = (uint16_t)t;
    if ((int16_t)t < 0) {
        uint16_t step = *H(x + 4);
        *H(x + 6) = 0;
        *H(x + 4) = (uint16_t)(step + 1);
    }
    leave(c, sv);
}

/* The 128 records of 0x148 bytes at 0x08D52380 are each entered in the second callback table by
   their +24. Returns 0. A 0x30 frame with its counters kept on the stack. */
SFA_FUNC(089E1F78, enter_records_128) {
    const gaddr base = 0x08D52380u;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 36) = ra;
    for (uint32_t i = 0; i < 128; i++) {
        gaddr p = base + i * 0x148;
        *W(sp + 32) = 0; *W(sp + 28) = i; *W(sp + 24) = 0; *W(sp + 20) = base;
        uint32_t w = *W(p + 24);
        *W(sp + 16) = p;
        ARG(1) = w;
        call1(c, w, f_089E5A00, 0x089E1FBCu);
    }
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x30;
}

/* The numbers 0 to 15 are each entered in the first callback table. Returns 0. */
SFA_FUNC(089E2004, enter_sixteen) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = ra;
    for (uint32_t i = 0; i < 16; i++) {
        *W(sp + 16) = i;
        call1(c, i, f_089E5804, 0x089E2018u);
    }
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (entry): one of the sixteen entries of 0x14 bytes at 0x08AD3AF8 is entered in the first callback
   table by its number. Returns 0. */
SFA_FUNC(089E22E8, enter_by_number) {
    gaddr entry = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    for (uint32_t i = 0; i < 16; i++) {
        if (0x08AD3AF8u + i * 0x14 == entry) { call1(c, i, f_089E5804, 0x089E2330u); break; }
    }
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (entry): one of the eight entries of 0x18 bytes at 0x08AD3C38 is entered in the second callback
   table by its +16. Returns 0. */
SFA_FUNC(089E2590, enter_by_word) {
    gaddr entry = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    for (uint32_t i = 0; i < 8; i++) {
        gaddr at = 0x08AD3C38u + i * 0x18;
        if (at == entry) { ARG(1) = at; call1(c, *W(at + 16), f_089E5A00, 0x089E25D8u); break; }
    }
    c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}
