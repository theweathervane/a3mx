/* More of what the new leaves set free: a held projectile's release by distance for the cpu, and a
   rolling attack's step with its sparks. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME   0x08E75910u
#define BATTLE 0x092E1228u

void f_089BAA50(Cpu *c); void f_089BAAA8(Cpu *c); void f_089BAB0C(Cpu *c); void f_088D31AC(Cpu *c);
void f_088D3EC0(Cpu *c); void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_0886B68C(Cpu *c);
void f_0887C408(Cpu *c); void f_0890CC04(Cpu *c); void f_0890D498(Cpu *c); void f_08821368(Cpu *c);
void f_0887E460(Cpu *c); void f_0886E3CC(Cpu *c); void f_0887E4A0(Cpu *c); void f_0887E4E0(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}

/* (fighter): a second held projectile's release. A player lets go by kick: light 0x089BAA50, middle
   0x089BAAA8, heavy 0x089BAB0C. The cpu by its plan (+490, low four bits) and the distance (+508): 0
   and 1 the light one within 48, 2 the middle within 112, 3 the heavy within 96, 4 to 6 within 64
   another move altogether (move 10, +78 = 1, step 0, the plan less 3). Otherwise the wait +82 counts
   down: at 0 the light one, and for the cpu also once it faces the way the opponent does from 128 or
   further; before that 0x088D31AC and 0x088D3EC0. */
SFA_FUNC(089BA0F8, held_shot_release_2) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int32_t cpu = S8(o + 373);
    *W(c->r[29] + 20) = ra;
    gaddr other = *W(o + 92);
    void (*fire)(Cpu *) = 0; uint32_t back = 0;
    if (cpu == 0) {
        uint32_t b = *H(o + 760);
        if (b & 0x80) { fire = f_089BAA50; back = 0x089BA184u; }
        else if (b & 0x100) { fire = f_089BAAA8; back = 0x089BA16Cu; }
        else if (b & 0x200) { fire = f_089BAB0C; back = 0x089BA154u; }
    } else {
        int32_t plan_word = S16(o + 490);
        uint32_t plan = (uint32_t)plan_word & 0xF;
        int32_t dist = S16(o + 508);
        if (plan <= 1) { if (dist < 48) { fire = f_089BAA50; back = 0x089BA1D4u; } }
        else if (plan == 2) { if (dist < 112) { fire = f_089BAAA8; back = 0x089BA1F8u; } }
        else if (plan == 3) { if (dist < 96) { fire = f_089BAB0C; back = 0x089BA21Cu; } }
        else if (plan < 7 && dist < 64) {
            *B(o + 236) = 0xA; *B(o + 235) = 0; *B(o + 78) = 1; *B(o + 7) = 0;
            *H(o + 490) = (uint16_t)(plan_word - 3);
            goto out;
        }
    }
    if (fire) call1(c, o, fire, back);
    else {
        int32_t t = S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        int now = (int16_t)t == 0;
        if (cpu == 0) {
            if (now) call1(c, o, f_089BAA50, 0x089BA284u);
            else {
                call1(c, o, f_088D31AC, 0x089BA294u);
                call1(c, o, f_088D3EC0, 0x089BA29Cu);
            }
        } else {
            if (!now && *B(o + 11) == *B(other + 11) && !(S16(o + 508) < 128)) now = 1;
            if (now) call1(c, o, f_089BAA50, 0x089BA2DCu);
            else {
                call1(c, o, f_088D31AC, 0x089BA2ECu);
                call1(c, o, f_088D3EC0, 0x089BA2F4u);
            }
        }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a rolling attack's step, by +77: 0 ticked; 2 turns it to face the middle from either end
   of the stage (below 513 or from 769) and is ticked. Otherwise: going backwards, the count at
   +0x339 comes down: at 0 the next step with the animation 0x14, else off again (0x0886B68C 0x4A, 10.0
   across slowing by 0xE000, BATTLE + 35 = 8, the cue 0x225, the animation from 0x08A585CC by the
   count). Going forwards: 0x0890CC04; on even frames (GAME + 32) 0x0890D498 while the count is more
   than the strength + 3; and every fourth frame a spark at a random place: for the last count
   0x0887E460, else one time in sixteen 0x0887E4A0 and otherwise 0x0887E4E0, each with a shake for both
   fighters (0x0886E3CC, from the table 0x08A58184 by strength); ticked. (The original also has a
   branch for +77 = 2 here that it can never reach.) A 0x30 frame keeping s0..s4. */
SFA_FUNC(0890DBA4, roll_step) {
    gaddr o = ARG(0);
    uint32_t sv[6];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    sv[5] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = o;
    int32_t k = S8(o + 77);
    for (uint32_t i = 1; i < 5; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 36) = sv[5];
    gaddr other = *W(o + 92);
    c->r[17] = other;
    if (k == 2) {
        int32_t x = S16(o + 18);
        if (x < 513) { *B(o + 11) = 0; *B(o + 11) = (uint8_t)(*B(o + 11) + 1); }
        else if (!(x < 769)) *B(o + 11) = 0;
        call1(c, o, f_08861848, 0x0890DEA4u);
    } else if (k == 0) call1(c, o, f_08861848, 0x0890DBE4u);
    else if ((int32_t)*W(o + 96) < 0) {
        c->r[17] = o + 0x334;
        int32_t n = S8(o + 0x334 + 5) - 1;
        *B(o + 0x334 + 5) = (uint8_t)n;
        if ((int8_t)n == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x14, f_088616EC, 0x0890DC88u);
        } else {
            call2(c, o, 0x4A, f_0886B68C, 0x0890DC98u);
            *W(o + 96) = 0xA0000; *W(o + 104) = 0xFFFF2000u;
            *B(BATTLE + 35) = 8;
            call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x225, f_0887C408, 0x0890DCCCu);
            call2(c, o, *H(0x08A585CCu + (uint32_t)(S8(o + 0x334 + 5) * 2)), f_088616EC, 0x0890DCECu);
        }
    } else {
        const gaddr shakes = 0x08A58184u;
        call1(c, o, f_0890CC04, 0x0890DCFCu);
        c->r[18] = GAME;
        if (!(*B(GAME + 32) & 1) && S8(o + 194) + 3 < S8(o + 0x334 + 5)) call1(c, o, f_0890D498, 0x0890DD38u);
        if ((*B(GAME + 32) & 3) == 0) {
            c->r[31] = 0x0890DD50u; f_08821368(c);
            uint32_t at = (c->r[2] & 0xF) + 0x68;
            c->r[18] = at;
            c->r[31] = 0x0890DD5Cu; f_08821368(c);
            int32_t px = (int16_t)at, py = (int16_t)(c->r[2] & 0xF);
            c->r[19] = (uint32_t)px; c->r[20] = (uint32_t)py; c->r[18] = shakes;
            if (S8(o + 0x334 + 5) == 1) {
                call3(c, o, (uint32_t)px, (uint32_t)py, f_0887E460, 0x0890DD98u);
                call3(c, o, *B(shakes + (uint32_t)(int32_t)S8(o + 194)), 4, f_0886E3CC, 0x0890DDB0u);
                call3(c, other, *B(shakes + (uint32_t)(int32_t)S8(o + 194) + 1), 4, f_0886E3CC, 0x0890DDC8u);
            } else {
                c->r[31] = 0x0890DDD8u; f_08821368(c);
                if ((c->r[2] & 0xF) == 0) call3(c, o, (uint32_t)px, (uint32_t)py, f_0887E4A0, 0x0890DDF4u);
                else call3(c, o, (uint32_t)px, (uint32_t)py, f_0887E4E0, 0x0890DE10u);
                uint32_t shake = *B(shakes + (uint32_t)(int32_t)S8(o + 194));
                c->r[19] = shake;
                call3(c, o, shake, 4, f_0886E3CC, 0x0890DE2Cu);
                call3(c, other, *B(shakes + (uint32_t)(int32_t)S8(o + 194) + 1), 4, f_0886E3CC, 0x0890DE44u);
            }
        }
        call1(c, o, f_08861848, 0x0890DE4Cu);
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[5]; c->r[29] += 0x30;
}
