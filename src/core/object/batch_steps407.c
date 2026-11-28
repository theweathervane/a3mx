/* Seven step routines that start a piece of music, and the modules loaded at the game's start.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u
#define F1     0x092E3EA0u
#define F2     0x092E4338u
#define F3     0x092E47D0u
#define F4     0x092E4C68u

void f_08821F58(Cpu *c); void f_08808424(Cpu *c); void f_0887C6E4(Cpu *c); void f_08881CF0(Cpu *c);
void f_0882B4E4(Cpu *c); void f_08881BEC(Cpu *c); void f_088082D0(Cpu *c); void f_08809934(Cpu *c);
void f_08823338(Cpu *c); void f_08855AA4(Cpu *c); void f_0887C724(Cpu *c); void f_089E6050(Cpu *c);
void f_08824AA0(Cpu *c);
void hle_sceUtilityLoadNetModule(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* (a, b): a screen's start: a's step (+4) goes on with 0x1F at +16, b's +81 and +15 cleared and
   +37 set; the screen's set-up, the routine at 0x08808424 with 4, the music 0x103 and the
   screen's own start. A 0x20 frame. */
SFA_FUNC(0882B674, screen_start_music_103) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = *H(a + 4) + 1u;
    c->r[7] = 0x1F;
    *H(a + 4) = (uint16_t)c->r[6];
    *H(a + 16) = 0x1F;
    *B(b + 81) = 0;
    ARG(0) = 1;
    *B(b + 15) = 0;
    *W(c->r[29] + 16) = ra;
    *B(b + 37) = 1;
    c->r[31] = 0x0882B6A4u; f_08821F58(c);
    call1(c, 4, f_08808424, 0x0882B6ACu);
    call1(c, 0x103, f_0887C6E4, 0x0882B6B4u);
    c->r[31] = 0x0882B6BCu; f_08881CF0(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): another screen's start: a's step goes on with 3 at +16; the routine at 0x0882B4E4
   gives what is kept at b's +92; with b's +31 set the music 0x101 starts and the routine at
   0x08881BEC runs. A 0x20 frame keeping s0. */
SFA_FUNC(0882B304, screen_start_music_101) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = 3;
    *W(c->r[29] + 16) = s0;
    c->r[16] = b;
    *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
    *H(a + 16) = 3;
    *W(c->r[29] + 20) = ra;
    call2(c, a, b, f_0882B4E4, 0x0882B330u);
    ARG(0) = (uint32_t)S8(b + 31);
    *W(b + 92) = c->r[2];
    if (ARG(0) != 0) {
        call1(c, 0x101, f_0887C6E4, 0x0882B344u);
        c->r[31] = 0x0882B34Cu; f_08881BEC(c);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* The modules the game needs are loaded from the disc and started (the four named from
   0x089FD080, the first two with 1 and the others with 2), then the system's two network
   modules, the second only if the first loads. A 0x20 frame. */
SFA_FUNC(08804A08, modules_load) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    call2(c, 0x089FD080u, 1, f_088082D0, 0x08804A20u);
    call2(c, 0x089FD090u, 1, f_088082D0, 0x08804A30u);
    call2(c, 0x089FD0A0u, 2, f_088082D0, 0x08804A40u);
    call2(c, 0x089FD0B4u, 2, f_088082D0, 0x08804A50u);
    call1(c, 1, hle_sceUtilityLoadNetModule, 0x08804A58u);
    if ((int32_t)c->r[2] >= 0) call1(c, 2, hle_sceUtilityLoadNetModule, 0x08804A68u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): a third screen's start, like the first, with the sound bank "SOSB" asked for (kind 2,
   its list at 0x08A01D10) in place of the routine at 0x08808424, and b's +4 set to -1 at the
   end. A 0x20 frame keeping s0. */
SFA_FUNC(089DEF90, screen_start_with_bank) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = b;
    ARG(1) = *H(a + 4) + 1u;
    c->r[6] = 0x1F;
    *H(a + 4) = (uint16_t)ARG(1);
    *H(a + 16) = 0x1F;
    *B(b + 81) = 0;
    ARG(0) = 1;
    *B(b + 15) = 0;
    *W(c->r[29] + 20) = ra;
    *B(b + 37) = 1;
    c->r[31] = 0x089DEFC8u; f_08821F58(c);
    ARG(0) = 2; ARG(1) = 0x42534F53u; ARG(2) = 0x08A01D10u;
    c->r[31] = 0x089DEFE0u; f_08809934(c);
    call1(c, 0x103, f_0887C6E4, 0x089DEFE8u);
    c->r[31] = 0x089DEFF0u; f_08881CF0(c);
    ARG(0) = 0xFFFFFFFFu;
    *B(b + 4) = 0xFF;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a): the music 0x100 starts, a's step goes on, and twelve pieces are asked for from the
   routine at 0x08823338 (six of kind 6, six of kind 0x1C); the byte at 0x08B01F34 is set. A
   0x20 frame keeping s0. */
SFA_FUNC(0883042C, step_music_100_and_pieces) {
    gaddr a = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = a;
    *W(c->r[29] + 20) = ra;
    call1(c, 0x100, f_0887C6E4, 0x08830444u);
    *H(a + 4) = (uint16_t)(*H(a + 4) + 1);
    static const uint8_t pieces[12][2] = {
        { 6, 0x10 }, { 6, 0x11 }, { 6, 0x12 }, { 6, 0x13 }, { 6, 0x14 }, { 6, 0x19 },
        { 0x1C, 0 }, { 0x1C, 5 }, { 0x1C, 2 }, { 0x1C, 1 }, { 0x1C, 3 }, { 0x1C, 4 },
    };
    for (uint32_t i = 0; i < 12; i++) call2(c, pieces[i][0], pieces[i][1], f_08823338, 0x0883045Cu + 0xC * i);
    ARG(0) = 1;
    ARG(1) = 0x08B00000u;
    *B(0x08B01F34u) = 1;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): a wait until none of the four fighters is in state 1, step 7; then a's count (+8)
   goes on with +72 and +43 cleared, b's +15 and +44 are cleared, the routine at 0x08855AA4
   runs, and the next piece of music (the one for the battle's first number) is read ahead: in
   the first round (b's +2 at 0) always, later only with b's +155 set and +63 clear. A 0x20
   frame keeping s0. */
SFA_FUNC(08825404, wait_fighters_then_read_music) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = b;
    *W(c->r[29] + 20) = ra;
    static const uint32_t fighters[4] = { F1, F2, F3, F4 };
    int busy = 0;
    for (int i = 0; i < 4 && !busy; i++) busy = *B(fighters[i] + 4) == 1 && *B(fighters[i] + 5) == 7;
    if (!busy) {
        ARG(1) = *H(a + 8) + 1u;
        *B(a + 72) = 0;
        *H(a + 8) = (uint16_t)ARG(1);
        *B(a + 43) = 0;
        *B(b + 15) = 0;
        *B(b + 44) = 0;
        c->r[31] = 0x088254C4u; f_08855AA4(c);
        if (S16(b + 2) == 0) {
            call1(c, (uint32_t)S16(BATTLE), f_0887C724, 0x088254D8u);
            call1(c, c->r[2] & 0xFF, f_089E6050, 0x088254E0u);
        } else if (S8(b + 155) != 0 && S8(b + 63) == 0) {
            call1(c, (uint32_t)S16(BATTLE), f_0887C724, 0x08825508u);
            call1(c, c->r[2] & 0xFF, f_089E6050, 0x08825510u);
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): a stage's turn in a run of fights: the stage (b's +59) has a bit (the table at
   0x08A0E200) in the player's fighter's +411 (the first fighter when a's +57 is odd, else the
   second). Set already, the routine at 0x08824AA0 runs. Else it is set, both fighters' +415
   become -1, a's count (+8) goes on with +72 and +43 cleared, b's +15 cleared, and b's +66 taken
   from the table at 0x08A0F48A (42 bytes a stage, one for each character): -1 runs the routine
   at 0x08824AA0, anything else starts the music 0x10A, or 0x10B on stage 6. A 0x20 frame. */
SFA_FUNC(08824F5C, run_stage_turn) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t bit = S8(0x08A0E200u + (uint32_t)S8(b + 59));
    gaddr f = (S8(a + 57) & 1) != 0 ? F1 : F2;
    c->r[8] = f;
    *W(c->r[29] + 16) = ra;
    int32_t seen = S8(f + 411);
    c->r[7] = (uint32_t)seen;
    c->r[9] = (uint32_t)(seen & bit);
    c->r[6] = (uint32_t)(seen | bit);
    if ((seen & bit) != 0) {
        c->r[31] = 0x08824FB8u; f_08824AA0(c);
    } else {
        *B(f + 411) = (uint8_t)(seen | bit);
        c->r[6] = 0xFFFFFFFFu;
        *B(F2 + 415) = 0xFF;
        *B(F1 + 415) = 0xFF;
        c->r[7] = *H(a + 8) + 1u;
        *B(a + 72) = 0;
        *H(a + 8) = (uint16_t)c->r[7];
        *B(a + 43) = 0;
        int32_t stage = S8(b + 59);
        c->r[7] = (uint32_t)stage;
        *B(b + 15) = 0;
        int32_t next = S8(0x08A0F532u + (uint32_t)(stage * 42 + S8(f + 338)) - 168);
        c->r[8] = (uint32_t)next;
        *B(b + 66) = (uint8_t)next;
        if (next == -1) { c->r[31] = 0x08825070u; f_08824AA0(c); }
        else if (stage == 6) call1(c, 0x10B, f_0887C6E4, 0x08825048u);
        else call1(c, 0x10A, f_0887C6E4, 0x0882505Cu);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}
