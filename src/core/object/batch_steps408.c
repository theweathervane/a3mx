/* Two set-up routines that end by starting music: a fight's, and the link play's lobby. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define LOBBY_SEEN 0x08A3E8F8u /* set once the lobby has been set up */

void f_08821FEC(Cpu *c); void f_0880A39C(Cpu *c); void f_08836538(Cpu *c); void f_088BE39C(Cpu *c);
void f_088BE178(Cpu *c); void f_08812DDC(Cpu *c); void f_088288F0(Cpu *c); void f_08819984(Cpu *c);
void f_0880AEE4(Cpu *c); void f_0884AE40(Cpu *c); void f_08819A20(Cpu *c); void f_0887C6E4(Cpu *c);
void f_0881DF10(Cpu *c); void f_088379B8(Cpu *c); void f_08804A74(Cpu *c); void f_08814320(Cpu *c);
void f_08814574(Cpu *c); void f_08813770(Cpu *c); void f_08822BD8(Cpu *c); void f_0882E0D0(Cpu *c);
void f_0880B57C(Cpu *c); void f_088051A0(Cpu *c); void f_08805564(Cpu *c); void f_088054F0(Cpu *c);
void f_088DC080(Cpu *c); void f_088DEB10(Cpu *c); void f_088E0A58(Cpu *c); void f_088E3EB0(Cpu *c);
void f_088DBF14(Cpu *c); void f_088E3294(Cpu *c); void f_088DD81C(Cpu *c); void f_089F3470(Cpu *c);
void f_088DD8F4(Cpu *c); void f_088DC0E0(Cpu *c); void f_08814ED8(Cpu *c); void f_08814260(Cpu *c);
void f_08815348(Cpu *c); void f_088157A0(Cpu *c); void f_08823338(Cpu *c); void f_0880ABD4(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): a fight is set up: a's step (+6) goes on and a run of its and b's fields are cleared,
   the screen and the fight's pieces are made ready, both first fighters' +432 set, three
   objects switched on, b's +2, +9 and +10 set (3, 0x13, 0x3B), and the fight's music (the
   battle's first number) started. A 0x20 frame keeping s0..s2. */
SFA_FUNC(088BDE40, fight_setup_b) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = a;
    *H(a + 6) = (uint16_t)(*H(a + 6) + 1);
    *W(sp + 20) = s1;
    c->r[17] = b;
    *B(a + 114) = 0;
    *B(b + 48) = 0;
    *B(b + 90) = 0;
    *H(b + 56) = 0;
    *B(b + 45) = 0;
    *B(a + 72) = 0;
    *B(b + 4) = 0;
    static const uint8_t cleared[9] = { 56, 71, 69, 59, 93, 39, 38, 43, 57 };
    for (int i = 0; i < 9; i++) *B(a + cleared[i]) = 0;
    *H(a + 32) = 0;
    *B(a + 138) = 0;
    *B(a + 89) = 0;
    *W(a + 60) = 0;
    *W(sp + 24) = s2;
    *B(a + 128) = 0;
    c->r[18] = 0x092F0000u;
    ARG(0) = 0x092E88A0u;
    ARG(1) = 0;
    *W(sp + 28) = ra;
    *B(0x092E88A0u + 123) = 0;
    call0(c, f_08821FEC, 0x088BDEE4u);
    call0(c, f_0880A39C, 0x088BDEECu);
    *B(a + 73) = 7;
    call1(c, 7, f_08836538, 0x088BDEF8u);
    *B(F2 + 432) = 1;
    ARG(0) = 1; ARG(1) = F1;
    *B(F1 + 432) = 1;
    call0(c, f_088BE39C, 0x088BDF18u);
    call0(c, f_088BE178, 0x088BDF20u);
    *B(0x092E8940u) = 1;
    *B(0x092E88A0u) = 1;
    *B(0x092E8800u) = 1;
    *H(b + 2) = 3;
    *B(b + 9) = 0x13;
    *B(b + 10) = 0x3B;
    ARG(1) = 0x13;
    call1(c, 0x13, f_08812DDC, 0x088BDF58u);
    call1(c, 4, f_08812DDC, 0x088BDF60u);
    call0(c, f_088288F0, 0x088BDF68u);
    call1(c, 1, f_08819984, 0x088BDF70u);
    call0(c, f_0880AEE4, 0x088BDF78u);
    call0(c, f_0884AE40, 0x088BDF80u);
    call0(c, f_08819A20, 0x088BDF88u);
    call1(c, (uint32_t)S16(BATTLE), f_0887C6E4, 0x088BDF94u);
    call0(c, f_0881DF10, 0x088BDF9Cu);
    call0(c, f_088379B8, 0x088BDFA4u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b, d): the link play's lobby is set up: everything from the screen before is put away and
   a frame passes, b's +64 and +114 kept across it; the first time the lobby's own pieces are
   made and its record (a, 0x10C bytes) cleared, later two routines bring them back; then the
   lobby's screen is loaded, six pieces asked for, the record's script set (the first time),
   and the music 0x101 started. A 0x30 frame keeping s0..s4. */
SFA_FUNC(088D5704, lobby_setup) {
    gaddr a = ARG(0), b = ARG(1), d = ARG(2);
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 20) = sv[1];
    c->r[17] = b;
    *W(sp + 24) = sv[2];
    c->r[18] = (uint32_t)S16(b + 64);
    *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    c->r[16] = a;
    c->r[19] = d;
    c->r[20] = (uint32_t)S8(b + 114);
    *W(sp + 36) = ra;
    call1(c, 0, f_08804A74, 0x088D573Cu);
    call0(c, f_08814320, 0x088D5744u);
    call0(c, f_08814574, 0x088D574Cu);
    call0(c, f_08813770, 0x088D5754u);
    call0(c, f_08822BD8, 0x088D575Cu);
    call0(c, f_0882E0D0, 0x088D5764u);
    call0(c, f_0880A39C, 0x088D576Cu);
    call0(c, f_08821FEC, 0x088D5774u);
    call0(c, f_0880B57C, 0x088D577Cu);
    call1(c, 0, f_088051A0, 0x088D5784u);
    call1(c, 3, f_08805564, 0x088D578Cu);
    call1(c, 1, f_088054F0, 0x088D5794u);
    *B(a + 267) = 0;
    *B(b + 146) = 1;
    *H(b + 64) = (uint16_t)c->r[18];
    *B(b + 114) = (uint8_t)c->r[20];
    *B(b + 145) = 0;
    *B(b + 138) = 0;
    *B(b + 89) = 0;
    *B(d + 23) = 0;
    c->r[17] = 0x08A40000u;
    if (*B(LOBBY_SEEN) == 0) {
        call0(c, f_088DC080, 0x088D57D0u);
        call0(c, f_088DEB10, 0x088D57D8u);
        call0(c, f_088E0A58, 0x088D57E0u);
        call0(c, f_088E3EB0, 0x088D57E8u);
        call0(c, f_088DBF14, 0x088D57F0u);
        call0(c, f_088E3294, 0x088D57F8u);
        call0(c, f_088DD81C, 0x088D5800u);
        ARG(2) = 0x10C;
        call2(c, a, 0, f_089F3470, 0x088D5810u);
        *H(a + 50) = (uint16_t)c->r[18];
    } else {
        call0(c, f_088DD8F4, 0x088D5820u);
        call0(c, f_088DC0E0, 0x088D5828u);
    }
    call0(c, f_08814ED8, 0x088D5830u);
    call0(c, f_08814260, 0x088D5838u);
    call0(c, f_08815348, 0x088D5840u);
    call1(c, 1, f_088054F0, 0x088D5848u);
    call0(c, f_088157A0, 0x088D5850u);
    static const uint8_t pieces[6] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x19 };
    for (uint32_t i = 0; i < 6; i++) call2(c, 6, pieces[i], f_08823338, 0x088D585Cu + 0xC * i);
    if (*B(LOBBY_SEEN) == 0) {
        *W(a + 20) = 0x08A3EABCu;
        *H(a + 24) = 0;
        *H(a + 2) = 0;
        *H(a) = 0;
        *B(a + 47) = 0;
    }
    call0(c, f_0880ABD4, 0x088D58C4u);
    call1(c, 0x101, f_0887C6E4, 0x088D58CCu);
    *B(LOBBY_SEEN) = 1;
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}
