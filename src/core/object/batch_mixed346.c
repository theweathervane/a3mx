/* Four more: a sound request carried out by its kind, the two fighters of a dramatic fight made
   partners, the sound thread's restart from the game's side, and one bit of three of the game's
   bytes. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u

void f_08809F48(Cpu *c); void f_089E2480(Cpu *c); void f_089E2590(Cpu *c); void f_089E25E8(Cpu *c);
void f_089E2644(Cpu *c); void f_089E26A0(Cpu *c); void f_089E6458(Cpu *c);

/* (request): a sound request of four words carried out by the low byte of its first: 1 a record
   taken by the entry its second word names (0x08809F48), with the third's two bytes and the
   fourth as the record and the priority; 2 the entry's callback entered again; 8, 9 and 10 the
   three of 0x089E25E8, 0x089E2644 and 0x089E26A0 with the third word. The answer is 0x0A000001
   for the other kinds and where the entry is missing (0 for the kind 2); with the answer 0 the
   byte at 0x092D9EEC moves on, 0x3F at most. A frame keeping s0, s1. */
SFA_FUNC(08809D78, sound_request_run) {
    gaddr p = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0;
    c->r[16] = p;
    uint32_t kind = (uint32_t)(int32_t)S16(p) & 0xFF;
    *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[17] = 0x0A000001u;
    static const uint32_t back[11] = { 0, 0x08809DC8u, 0x08809E14u, 0, 0, 0, 0, 0, 0x08809E3Cu, 0x08809E6Cu, 0x08809E9Cu };
    if (kind == 1 || kind == 2 || (kind >= 8 && kind <= 10)) {
        if (kind == 2) c->r[17] = 0;
        ARG(0) = (uint32_t)(int32_t)S16(p + 2); c->r[31] = back[kind]; f_08809F48(c);
        gaddr e = c->r[2];
        ARG(0) = e;
        if (e != 0) {
            int32_t third = S16(p + 4);
            if (kind == 1) {
                ARG(1) = (uint32_t)(int32_t)(int8_t)(third >> 8);
                ARG(2) = (uint32_t)(int32_t)(int8_t)third;
                ARG(3) = (uint32_t)(int32_t)(int8_t)S16(p + 6);
                c->r[31] = 0x08809E00u; f_089E2480(c);
            } else if (kind == 2) { c->r[31] = 0x08809E28u; f_089E2590(c); }
            else if (kind == 8) {
                ARG(1) = (uint32_t)(int32_t)(int8_t)third; ARG(2) = 0;
                c->r[31] = 0x08809E5Cu; f_089E25E8(c);
            } else if (kind == 9) {
                ARG(1) = (uint32_t)(int32_t)(int8_t)third; ARG(2) = 0;
                c->r[31] = 0x08809E8Cu; f_089E2644(c);
            } else {
                ARG(1) = (uint32_t)third; ARG(2) = 0;
                c->r[31] = 0x08809EB4u; f_089E26A0(c);
            }
            c->r[17] = c->r[2];
        }
    }
    if (c->r[17] == 0) {
        *B(0x092D9EECu) = (uint8_t)(*B(0x092D9EECu) + 1);
        *B(0x092D9EECu) = (uint8_t)(*B(0x092D9EECu) & 0x3F);
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* The two of a pair know each other (+1168); `cpu` gets the cpu's mark (+373) and its first step
   (+4 at 1, +5 at 9, +6 and +7 cleared), `player` has the mark cleared. */
static void partners(gaddr a, gaddr b, gaddr player, gaddr cpu) {
    *W(a + 1168) = b;
    *W(b + 1168) = a;
    if (player == a) { *B(a + 373) = 0; *B(b + 373) = 1; }
    else { *B(a + 373) = 1; *B(b + 373) = 0; }
    *B(cpu + 4) = 1;
    *B(cpu + 5) = 9;
    *B(cpu + 6) = 0;
    *B(cpu + 7) = 0;
}

/* The partners of a fight of two against one, by its kind (BATTLE + 56): 1 and 2 fighters 3 and
   4, with 4 the cpu's; 3 fighters 1 and 2, the cpu's being 1 when GAME's +53 is 2 and 2
   otherwise. */
SFA_FUNC(08825588, pair_partners_set) {
    int32_t kind = S16(BATTLE + 56);
    if (kind == 1 || kind == 2) partners(FIGHTER_3, FIGHTER_4, FIGHTER_3, FIGHTER_4);
    else if (kind == 3) {
        if (S8(GAME + 53) == 2) partners(FIGHTER_1, FIGHTER_2, FIGHTER_2, FIGHTER_1);
        else partners(FIGHTER_1, FIGHTER_2, FIGHTER_1, FIGHTER_2);
    }
}

/* The sound thread's restart behind a 0x10 frame. */
SFA_FUNC(0887C708, sound_thread_restart_framed) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = ra;
    c->r[31] = 0x0887C718u; f_089E6458(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* Bit 4 of GAME's +41, +53 and +57 together. */
SFA_FUNC(088A05B4, game_bit_4_any) {
    c->r[2] = (uint32_t)((int32_t)S8(GAME + 57) | S8(GAME + 41) | S8(GAME + 53)) & 4;
}
