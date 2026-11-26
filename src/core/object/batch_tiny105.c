/* A projectile's frame (98 instructions): its build-up by strength, then its flight, the owner's
   cancel, its trail sparks and push boxes. Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08821384(Cpu *c); void f_08821424(Cpu *c); void f_08874934(Cpu *c); void f_088768F0(Cpu *c);
void f_0891C6C4(Cpu *c); void f_0891CCEC(Cpu *c); void f_0891CD58(Cpu *c); void f_0891ED28(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (projectile): unless halted (BATTLE +37 / +78), step 0 builds up (0x0891CD58) for its strength's
   frames (0x08A64544 by +346, +3 with +347, counted at +0x53), then flies (+6 = 1, 0x0891C6C4,
   0x0891CCEC); meanwhile its owner's (+88) +539 ends it (step 3), the round's end (BATTLE +7) dims it
   (+0 = 2), every other frame (+0x6E) a trail spark (0x0891ED28), 0x088768F0 and, without +126, its
   push boxes (0x08874934). Step 1 flies (0x0891CCEC, its steps, 0x088768F0, 0x08821384). Drawn. */
SFA_FUNC(0891C790, projectile_frame) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    c->r[17] = BATTLE;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(BATTLE + 37) == 0 && S8(BATTLE + 78) == 0) {
        uint32_t st = *B(o + 6);
        if (st == 0) {
            call_on(c, o, 0x0891C7D4u, f_0891CD58);
            int32_t idx = (S8(o + 347) != 0 ? 3 : 0) + S8(o + 346);
            *B(o + 0x53) = (uint8_t)(*B(o + 0x53) + 1);
            int32_t n = *B(o + 0x53);
            if (!(n < (int32_t)*B(0x08A64544u + (uint32_t)idx))) {
                *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
                call_on(c, o, 0x0891C830u, f_0891C6C4);
                call_on(c, o, 0x0891C838u, f_0891CCEC);
            } else {
                gaddr p = *W(o + 88);
                if (S8(p + 539) != 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
                if (S8(BATTLE + 7) != 0) *B(o) = 2;
                gaddr q = o + 0x6C;
                int8_t v = (int8_t)(S8(q + 2) - 1);
                *B(q + 2) = (uint8_t)v;
                if (v < 0) {
                    *B(q + 2) = 1;
                    call_on(c, o, 0x0891C898u, f_0891ED28);
                }
                call_on(c, o, 0x0891C8A0u, f_088768F0);
                if (S8(o + 126) == 0) call_on(c, o, 0x0891C8B4u, f_08874934);
            }
        } else if (st == 1) {
            call_on(c, o, 0x0891C8CCu, f_0891CCEC);
            *W(o + 16) += *W(o + 96);
            *W(o + 20) += *W(o + 100);
            call_on(c, o, 0x0891C8F4u, f_088768F0);
            call_on(c, o, 0x0891C8FCu, f_08821384);
        }
    }
    call_on(c, o, 0x0891C904u, f_08821424);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
