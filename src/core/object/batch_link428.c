/* The link play's lobby: the screen where a match is asked for and agreed. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define PADS 0x08E75BA0u

void f_088DD070(Cpu *c); void f_088DD124(Cpu *c); void f_088DF754(Cpu *c); void f_088DF828(Cpu *c);
void f_088D5680(Cpu *c); void f_088E135C(Cpu *c); void f_088E3ED8(Cpu *c); void f_088E448C(Cpu *c);
void f_088E4324(Cpu *c); void f_088DD040(Cpu *c); void f_088DD23C(Cpu *c); void f_089F3430(Cpu *c);
void f_088E43B0(Cpu *c); void f_088DCB00(Cpu *c); void f_088DF208(Cpu *c); void f_088E3F98(Cpu *c);
void f_088E45F0(Cpu *c); void f_0887C4CC(Cpu *c); void f_088E3FD0(Cpu *c); void f_0880E788(Cpu *c);
void f_088E40D8(Cpu *c); void f_088DDC30(Cpu *c); void f_088E3FC0(Cpu *c); void f_088E450C(Cpu *c);
void f_088E456C(Cpu *c); void f_088E3F44(Cpu *c); void f_088D6564(Cpu *c); void f_088E1444(Cpu *c);
void f_088DCD40(Cpu *c); void f_0880A6CC(Cpu *c); void f_088D68C8(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void sound(Cpu *c, uint32_t which, uint32_t back) {
    ARG(0) = 0; ARG(1) = which; ARG(2) = 0xC0; c->r[31] = back; f_0887C4CC(c);
}
static void step_on(gaddr o) { *H(o) = (uint16_t)(*H(o) + 1); }
/* A count of frames at +at goes down by one; whether it has reached 0. */
static int count_down(gaddr o, uint32_t at) {
    int32_t left = S16(o + at) - 1;
    *H(o + at) = (uint16_t)left;
    return (int16_t)left == 0;
}

/* (o): the screen's step (o's +0), with the player picked at +28 and what this side is at +44
   (1 the one who asked):
     0  the one who asked, or the one asked (+44 at 2), goes on to 1; a side that is neither
        (+44 at 0) answers the request (+32) and the screen's script goes on.
     1, 2  the link is told, then waited on: an error ends the screen with it, an answer goes
        on.
     3, 4  the match's message and the wait's display are put up; the one who asked gathers the
        players' addresses (as many as answered, at most +54) into the request it sends, and
        waits 0xB4 frames.
     5  a request from someone else while waiting is turned down. The one who asked waits for
        the routine at 0x088E45F0, giving up (error 0x13) after ten waits; the other side can
        go on (step 0x14 with a longer wait when the picked player's +144 is 4, on bit 8, or on
        an answer), and sees a box with the player's name while the routine at 0x088E3FD0 says
        so.
     6, 7  a wait of 0x78 frames, then up to 0x4B0 more for the routine at 0x088E45F0; when
        they run out the screen gives up (step 0x14).
     8, 9  the same for the second part of the agreement, ending with error 0x81001003.
     10  a last wait, 11 the message gone and the match made ready, 12 the script goes on.
     0x14, 0x15  giving up: the link is told, the picked player let go, the list refreshed.
     0x16  a wait (cut short by bit 8) for the message to go, 0x17 the script goes back two.
   Any other step tells the link, refreshes the list and sets the script at 0x08A3EB14. Then
   the screen's title and its own drawing. A 0x40 frame keeping s0..s6. */
SFA_FUNC(088D9CD0, lobby_match_screen) {
    gaddr o = ARG(0);
    uint32_t sv[7], ra = c->r[31];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x40;
    gaddr sp = c->r[29];
    *W(sp + 32) = sv[0];
    c->r[16] = o;
    *W(sp + 36) = sv[1];
    gaddr picked = *W(o + 28);
    c->r[17] = picked;
    uint32_t step = *H(o);
    *W(sp + 40) = sv[2]; *W(sp + 44) = sv[3]; *W(sp + 48) = sv[4]; *W(sp + 52) = sv[5];
    *W(sp + 56) = sv[6]; *W(sp + 60) = ra;
    c->r[22] = 4;
    uint32_t side = *B(o + 44);
    switch (step) {
    case 0:
        if (side == 0) {
            *B(o + 54) = 1;
            call1(c, *B(o + 32), f_088DD070, 0x088D9D48u);
            *H(o + 2) = 0;
            *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
            *H(o) = 0;
            *B(o + 47) = 0;
        } else if (side < 3) {
            *B(o + 54) = 0;
            *H(o) = 1;
        }
        break;
    case 1:
        call0(c, f_088DD124, 0x088D9D80u);
        call1(c, c->r[2], f_088DF754, 0x088D9D88u);
        step_on(o);
        /* fall through */
    case 2:
        call0(c, f_088DD124, 0x088D9D9Cu);
        call1(c, c->r[2], f_088DF828, 0x088D9DA4u);
        if ((int32_t)c->r[2] < 0) call2(c, o, c->r[2], f_088D5680, 0x088D9DB8u);
        else if ((int32_t)c->r[2] > 0) step_on(o);
        break;
    case 3:
        call0(c, f_088E135C, 0x088D9DE0u);
        c->r[18] = 1;
        if (*B(o + 44) == 1) {
            call1(c, 0x14, f_088E3ED8, 0x088D9DF8u);
            call1(c, 7, f_088E448C, 0x088D9E00u);
        } else {
            call1(c, 2, f_088E3ED8, 0x088D9E18u);
            call1(c, 1, f_088E448C, 0x088D9E20u);
        }
        *H(o + 4) = 0;
        *H(o + 6) = 0xA;
        step_on(o);
        /* fall through */
    case 4:
        c->r[18] = 1;
        if (*B(o + 44) == 1) {
            call2(c, 1, 0xFFFFFFFFu, f_088E4324, 0x088D9E58u);
            gaddr request = c->r[2];
            c->r[19] = request;
            call0(c, f_088DD040, 0x088D9E60u);
            int32_t answered = (int32_t)c->r[2];
            c->r[20] = (uint32_t)answered;
            if (answered <= 0) {
                call2(c, o, 0x81001003u, f_088D5680, 0x088D9F00u);
                break;
            }
            if ((int32_t)*B(o + 54) < answered) *B(o + 54) = (uint8_t)answered;
            *B(request + 76) = (uint8_t)answered;
            c->r[21] = request + ((uint32_t)answered << 3) + 0xC;
            for (; (int32_t)c->r[20] >= 0; c->r[20]--, c->r[21] -= 8) {
                call1(c, c->r[20], f_088DD23C, 0x088D9E98u);
                if (c->r[2] != 0) {
                    *W(c->r[21]) = *W(c->r[2]);
                    *W(c->r[21] + 4) = *W(c->r[2] + 4);
                }
            }
            ARG(2) = 9;
            call2(c, request, 0x09523C00u, f_089F3430, 0x088D9ED4u);
            call1(c, request, f_088E43B0, 0x088D9EDCu);
            *H(o + 4) = 0xB4;
        }
        step_on(o);
        /* fall through */
    case 5: {
        c->r[18] = 1;
        call0(c, f_088DCB00, 0x088D9F14u);
        gaddr other = c->r[2];
        c->r[19] = other;
        if (other != 0 && other != picked) {
            call1(c, other + 4, f_088DF208, 0x088D9F30u);
            *B(other + 154) = 0;
        }
        if (*B(o + 44) == 1) {
            call1(c, 0, f_088E3F98, 0x088D9F48u);
            call1(c, 7, f_088E45F0, 0x088D9F50u);
            if (c->r[2] != 0) step_on(o);
            else if (count_down(o, 4)) {
                if (count_down(o, 6)) call2(c, o, 0x13, f_088D5680, 0x088D9FA4u);
                else *H(o) = (uint16_t)(*H(o) - 1);
            }
            break;
        }
        call1(c, 1, f_088E3F98, 0x088D9FC4u);
        if (*W(picked + 144) == 4) {
            call1(c, 5, f_088E3ED8, 0x088D9FD8u);
            call1(c, 0, f_088E3F98, 0x088D9FE0u);
            *H(o) = 0x14;
            *H(o + 4) = 0x1E0;
        } else if (((*W(PADS + 12) | *W(PADS + 56) | *W(PADS + 100)) & 0x100) != 0) {
            sound(c, 0x2F, 0x088DA024u);
            *H(o) = 0x14;
            *H(o + 4) = 0;
        } else {
            call0(c, f_088DD124, 0x088DA03Cu);
            if ((int32_t)c->r[2] > 0) {
                call1(c, 0x10, f_088E3ED8, 0x088DA04Cu);
                call1(c, 0, f_088E3F98, 0x088DA054u);
                sound(c, 0x39, 0x088DA064u);
                step_on(o);
            }
        }
        call0(c, f_088E3FD0, 0x088DA078u);
        if (c->r[2] != 0) {
            ARG(0) = 0x6D; ARG(1) = 0x7C; ARG(2) = 0x113; ARG(3) = 0x92;
            c->r[8] = 0xA0800080u; c->r[9] = 0x13;
            c->r[31] = 0x088DA09Cu; f_0880E788(c);
            ARG(0) = 0x08A3E9B3u; ARG(1) = 1; ARG(2) = 0xB0; ARG(3) = 0x6B; c->r[8] = 4;
            c->r[31] = 0x088DA0B8u; f_088E40D8(c);
            ARG(0) = picked + 4; ARG(1) = 0x70; ARG(2) = 0x84; ARG(3) = 0; c->r[8] = 0x13;
            c->r[31] = 0x088DA0D0u; f_088DDC30(c);
        }
        break;
    }
    case 6:
        call0(c, f_088E3FC0, 0x088DA0E0u);
        call1(c, 7, f_088E448C, 0x088DA0E8u);
        *H(o + 4) = 0x78;
        *H(o + 6) = 0x4B0;
        step_on(o);
        /* fall through */
    case 7:
        if (S16(o + 4) != 0) { *H(o + 4) = (uint16_t)(S16(o + 4) - 1); break; }
        call1(c, 7, f_088E45F0, 0x088DA128u);
        if (c->r[2] != 0) step_on(o);
        else if (count_down(o, 6)) {
            call1(c, 5, f_088E3ED8, 0x088DA160u);
            call1(c, 0, f_088E3F98, 0x088DA168u);
            sound(c, 0x2F, 0x088DA178u);
            *H(o) = 0x14;
        }
        break;
    case 8:
        call0(c, f_088E450C, 0x088DA18Cu);
        *H(o + 4) = 0x78;
        *H(o + 6) = 0x4B0;
        step_on(o);
        break;
    case 9: {
        if (S16(o + 4) != 0) { *H(o + 4) = (uint16_t)(S16(o + 4) - 1); break; }
        int agreed = 0;
        call1(c, 7, f_088E45F0, 0x088DA1C8u);
        if (c->r[2] != 0) {
            call0(c, f_088E456C, 0x088DA1D8u);
            agreed = c->r[2] != 0;
        }
        if (agreed) {
            *H(o + 4) = 0x5A;
            step_on(o);
        } else if (count_down(o, 6)) {
            call2(c, o, 0x81001003u, f_088D5680, 0x088DA21Cu);
        }
        break;
    }
    case 10:
        if (count_down(o, 4)) *H(o) = 11;
        break;
    case 11:
        call0(c, f_088E3F44, 0x088DA250u);
        if (c->r[2] != 0) {
            call1(c, o, f_088D6564, 0x088DA260u);
            step_on(o);
        }
        break;
    case 12:
        *H(o + 2) = 0;
        *H(o + 24) = (uint16_t)(*H(o + 24) + 1);
        *H(o) = 0;
        *B(o + 47) = 0;
        break;
    case 20:
        call0(c, f_088E3FC0, 0x088DA294u);
        call0(c, f_088E1444, 0x088DA29Cu);
        call0(c, f_088DD124, 0x088DA2A4u);
        call1(c, c->r[2], f_088DF754, 0x088DA2ACu);
        step_on(o);
        /* fall through */
    case 21:
        if (picked != 0) {
            call1(c, picked + 4, f_088DF208, 0x088DA2C8u);
            *B(picked + 154) = 0;
        }
        *W(o + 28) = 0;
        call0(c, f_088DCD40, 0x088DA2D4u);
        step_on(o);
        break;
    case 22:
        if (S16(o + 4) != 0) {
            *H(o + 4) = (uint16_t)(S16(o + 4) - 1);
            call1(c, 1, f_088E3F98, 0x088DA2FCu);
            if (((*W(PADS + 12) | *W(PADS + 56) | *W(PADS + 100)) & 0x100) != 0) *H(o + 4) = 0;
        } else {
            call0(c, f_088E3F44, 0x088DA334u);
            if (c->r[2] != 0) step_on(o);
        }
        break;
    case 23:
        *H(o + 2) = 0;
        *H(o + 24) = (uint16_t)(*H(o + 24) - 1);
        *H(o) = 0;
        *B(o + 47) = 0;
        *H(o + 24) = (uint16_t)(*H(o + 24) - 1);
        break;
    default:
        call0(c, f_088E1444, 0x088DA37Cu);
        call0(c, f_088DD124, 0x088DA384u);
        call1(c, c->r[2], f_088DF754, 0x088DA38Cu);
        call0(c, f_088DCD40, 0x088DA394u);
        *W(o + 20) = 0x08A3EB14u;
        *H(o + 24) = 0;
        *H(o + 2) = 0;
        *H(o) = 0;
        *B(o + 47) = 0;
        break;
    }
    ARG(0) = *W(0x08A3EA04u + (((uint32_t)*B(o + 42) + *B(o + 43)) << 2));
    ARG(1) = 2; ARG(2) = 0x30; ARG(3) = 4; c->r[8] = 9;
    c->r[31] = 0x088DA3E4u; f_0880A6CC(c);
    call1(c, o, f_088D68C8, 0x088DA3ECu);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x40;
}
