/* Three more that the last ones set free: the callback tables' reset, a round's last frames for the
   side they belong to, and a piece of the screen's start by its kind. Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_089E1F78(Cpu *c); void f_089E2004(Cpu *c); void f_088268D4(Cpu *c); void f_0887EDD8(Cpu *c);
void f_08823338(Cpu *c); void f_0887C4CC(Cpu *c); void f_0882196C(Cpu *c); void f_088215C4(Cpu *c);
void f_08809BE4(Cpu *c); void f_0887EC58(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* Both callback tables are entered afresh (0x089E1F78, 0x089E2004) and the two bytes at 0x092D9EEC
   are cleared. A 0x10 frame. */
SFA_FUNC(088099D4, callbacks_reset) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x088099E4u; f_089E1F78(c);
    c->r[31] = 0x088099ECu; f_089E2004(c);
    *B(0x092D9EECu) = 0;
    *B(0x092D9EEDu) = 0;
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (x, y): a round's last frames (0x088268D4) once y's +44 is 4 and its +11 is the one the table
   0x08A0E280 names for the fight's kind (y + 56). */
SFA_FUNC(0882687C, round_last_frames_for_kind) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(y + 44) == 4 && S8(y + 11) == S8(0x08A0E280u + (uint32_t)(int32_t)S16(y + 56)))
        call2(c, x, y, f_088268D4, 0x088268C8u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): a piece of the screen starts. Outside a fight (BATTLE + 36 clear) its +4 goes to 2.
   Otherwise +4 moves on, its drawing fields are set, and 0x0887EDD8 runs (for the kind 12,
   0x08823338 with 0x1E). By the kind (+3): from 13 on a place from the table 0x08A24526, the way
   out (0x0887EC58 with a byte from 0x08A24541) and the picture it names in 0x08A16CD0; 7 the
   sound 0x3B, a place and its owner's (+92) colours and picture 8 (0x1C for the character 0x17)
   of the owner's own; 8 a place, the table's second picture and the sound 0x3A; the others the
   sound 0x3A alone. Below 13 the music's tempo goes to 0xA. A frame keeping s0..s2. */
SFA_FUNC(0887F00C, screen_piece_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    int32_t fight = S8(BATTLE + 36);
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[16] = o;
    if (fight == 0) *B(o + 4) = 2;
    else {
        *B(o + 81) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        *H(o + 82) = 0;
        *B(o + 127) = 0;
        *B(o + 9) = 3;
        *B(o + 11) = 0;
        c->r[17] = 0x3F;
        *B(o + 12) = 1;
        *B(o + 13) = 0x3F;
        *B(o + 14) = 0;
        *B(o + 15) = 0;
        *H(o + 24) = 7;
        *B(o + 72) = 1;
        c->r[18] = 7;
        *H(o + 86) = 0x11;
        *W(o + 148) = 7;
        uint32_t kind = *B(o + 3);
        *W(o + 164) = 1;
        if (kind == 0xC) call2(c, 0x1E, 0, f_08823338, 0x0887F0CCu);
        else { ARG(1) = kind; call1(c, o, f_0887EDD8, 0x0887F0B8u); }
        kind = *B(o + 3);
        if (!(kind < 13)) {
            gaddr place = 0x08A2455Au + (kind << 2);
            *H(o + 18) = (uint16_t)S16(place - 52);
            *H(o + 22) = (uint16_t)S16(place - 50);
            call2(c, o, *B(0x08A2454Eu + kind - 13), f_0887EC58, 0x0887F210u);
            call2(c, o, *W(0x08A16CD0u + ((c->r[2] & 0xFFFF) << 2)), f_0882196C, 0x0887F230u);
            call1(c, o, f_088215C4, 0x0887F238u);
        } else {
            if (kind == 7) {
                call3(c, 0, 0x3B, 0xC0, f_0887C4CC, 0x0887F0F4u);
                gaddr owner = *W(o + 92);
                *W(o + 148) = 0;
                *H(o + 86) = (uint16_t)(int16_t)S8(owner + 337);
                *W(o + 164) = *W(owner + 164);
                *B(o + 160) = 0;
                *H(o + 18) = 0xD8;
                *H(o + 22) = 0xF0;
                *B(o + 13) = 0x14;
                *B(o + 116) = 0x3F;
                uint32_t picture = S8(owner + 338) == 0x17 ? 0x1C : 8;
                c->r[17] = picture;
                call2(c, o, *W(*W(*W(owner + 416) + 140) + (picture << 2)), f_0882196C, 0x0887F164u);
                call1(c, o, f_088215C4, 0x0887F16Cu);
            } else if (kind == 8) {
                *H(o + 18) = 0xC0;
                *H(o + 22) = 0xB0;
                call2(c, o, *W(0x08A16CD0u + 4), f_0882196C, 0x0887F1A0u);
                call1(c, o, f_088215C4, 0x0887F1A8u);
                call3(c, 0, 0x3A, 0xC0, f_0887C4CC, 0x0887F1B8u);
            } else call3(c, 0, 0x3A, 0xC0, f_0887C4CC, 0x0887F1CCu);
            call1(c, 0xA, f_08809BE4, 0x0887F1D4u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
