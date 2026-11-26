/* Small functions the scenarios reach, 27 - 34 instructions: fighter objects' starts (one shape, the
   start values as numbers) and two effects' steps. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_08821424(Cpu *c);
void f_089DB868(Cpu *c); void f_0887684C(Cpu *c); void f_088768F0(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (object): when 0x0886BF84 allows it, on (+4 = 1, +6 = 7, +235 = 1, +236 = k236; +5 / +7 (and with
   `b196` +196) cleared, +11 = +697) and `then`; 1 then, else 0. */
static void start_s1(Cpu *c, uint32_t k236, int b196, void (*then)(Cpu *), uint32_t back1, uint32_t back2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    c->r[31] = back1; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = (uint8_t)k236; *B(o + 235) = 1;
        if (b196) *B(o + 196) = 0;
        *B(o + 11) = (uint8_t)S8(o + 697);
        c->r[17] = 1;
        call_on(c, o, back2, then);
        c->r[2] = 1;
    } else c->r[2] = 0;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0892630C, object_start_0892630c) { start_s1(c, 3, 0, f_0886DBE8, 0x08926324u, 0x08926360u); }
SFA_FUNC(089300C8, object_start_089300c8) { start_s1(c, 4, 0, f_0886DB94, 0x089300E0u, 0x0893011Cu); }
SFA_FUNC(0895C654, object_start_0895c654) { start_s1(c, 2, 0, f_0886DBE8, 0x0895C66Cu, 0x0895C6A8u); }
SFA_FUNC(0895C7A8, object_start_0895c7a8) { start_s1(c, 3, 0, f_0886DBE8, 0x0895C7C0u, 0x0895C7FCu); }
SFA_FUNC(0895C8BC, object_start_0895c8bc) { start_s1(c, 5, 0, f_0886DB94, 0x0895C8D4u, 0x0895C910u); }
SFA_FUNC(089676BC, object_start_089676bc) { start_s1(c, 4, 0, f_0886DB94, 0x089676D4u, 0x08967710u); }
SFA_FUNC(08967740, object_start_08967740) { start_s1(c, 5, 0, f_0886DBE8, 0x08967758u, 0x08967794u); }
SFA_FUNC(089677C4, object_start_089677c4) { start_s1(c, 6, 0, f_0886DB94, 0x089677DCu, 0x08967818u); }
SFA_FUNC(089678D0, object_start_089678d0) { start_s1(c, 8, 0, f_0886DB94, 0x089678E8u, 0x08967924u); }
SFA_FUNC(0898EB44, object_start_0898eb44) { start_s1(c, 2, 0, f_0886DBE8, 0x0898EB5Cu, 0x0898EB98u); }
SFA_FUNC(0898EBC8, object_start_0898ebc8) { start_s1(c, 4, 0, f_0886DB94, 0x0898EBE0u, 0x0898EC1Cu); }
SFA_FUNC(0898EC9C, object_start_0898ec9c) { start_s1(c, 12, 0, f_0886DBE8, 0x0898ECB4u, 0x0898ECF0u); }
SFA_FUNC(08994290, object_start_08994290) { start_s1(c, 2, 0, f_0886DBE8, 0x089942A8u, 0x089942E4u); }
SFA_FUNC(089943C4, object_start_089943c4) { start_s1(c, 5, 0, f_0886DB94, 0x089943DCu, 0x08994418u); }
SFA_FUNC(0897C1DC, object_start_0897c1dc) { start_s1(c, 6, 1, f_0886DBE8, 0x0897C1F4u, 0x0897C234u); }
SFA_FUNC(089A5E18, object_start_089a5e18) { start_s1(c, 4, 1, f_0886DB94, 0x089A5E30u, 0x089A5E70u); }
SFA_FUNC(089A61C4, object_start_089a61c4) { start_s1(c, 6, 1, f_0886DB94, 0x089A61DCu, 0x089A621Cu); }

/* (object, row): on (+5, then 0x089DB868) once the row's +2 reaches +120; until then drawn. */
SFA_FUNC(089DB7FC, object_wait_120) {
    gaddr o = ARG(0), p = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S16(p + 2) < (int32_t)(int16_t)*B(o + 120)) { ARG(0) = o; c->r[31] = 0x089DB85Cu; f_08821424(c); }
    else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        call2(c, o, p, 0x089DB848u, f_089DB868);
    }
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): first frame +0 = 2 and 0x0887684C (0xB); on its second step, step 3 once +75 goes
   negative, 0x088768F0; drawn. */
SFA_FUNC(0891CC30, effect_play_b) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    uint32_t k = *B(o + 5);
    if (k == 0) {
        *B(o + 5) = 1; *B(o) = 2;
        call2(c, o, 0xB, 0x0891CC68u, f_0887684C);
    } else if (k == 1) {
        if (S8(o + 75) < 0) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        call_on(c, o, 0x0891CC9Cu, f_088768F0);
    }
    call_on(c, o, 0x0891CCA4u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}