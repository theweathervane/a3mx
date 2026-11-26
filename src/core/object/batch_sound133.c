/* Sound-cue callers, 33 - 39 instructions: a voice looked up by key, effects spawned with their cue,
   menu toggles and a voice per stage step. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C408(Cpu *c); void f_0887C4CC(Cpu *c); void f_089F3430(Cpu *c); void f_08822EAC(Cpu *c);
void f_088230F4(Cpu *c); void f_088D2E04(Cpu *c); void f_0882196C(Cpu *c); void f_0898820C(Cpu *c);
void f_0898825C(Cpu *c); void f_08881D9C(Cpu *c);

#define BATTLE 0x092E1228u

static void side_cue(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = n;
    c->r[31] = back; f_0887C408(c);
}
static void plain_cue(Cpu *c, uint32_t n, uint32_t back) {
    ARG(0) = 0; ARG(1) = n; ARG(2) = 0xC0;
    c->r[31] = back; f_0887C4CC(c);
}

/* (object, key): the 15 (key, cue) pairs at 0x08A3E798 copied to the stack and searched from the last;
   the first with the key's low byte and a cue other than -1 plays from the object's side. */
SFA_FUNC(088D2E54, keyed_voice) {
    gaddr o = ARG(0);
    uint32_t key = ARG(1) & 0xFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0xA0;
    gaddr sp = c->r[29];
    *W(sp + 136) = s0; *W(sp + 140) = s1;
    c->r[17] = key; c->r[16] = o;
    *W(sp + 144) = ra;
    ARG(0) = sp + 0x10; ARG(1) = 0x08A3E798u; ARG(2) = 0x78;
    c->r[31] = 0x088D2E80u; f_089F3430(c);
    for (int i = 14; i >= 0; i--) {
        gaddr e = sp + 0x10 + 8 * (uint32_t)i;
        if (*W(e) == key && *W(e + 4) != 0xFFFFFFFFu) { side_cue(c, o, *W(e + 4), 0x088D2EB4u); break; }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0xA0;
}

/* (owner): effect 0x3B spawned (0x08822EAC) at the owner's position (+16 up), tied to it (+88), +9 = 3,
   +78 = 0x51, +72 = 1, +86 / +164 copied, with cue 0x220 from the owner's fighter's side. */
SFA_FUNC(0893A760, effect_3b_cue) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    ARG(0) = 0x3B; ARG(1) = 1;
    c->r[31] = 0x0893A77Cu; f_08822EAC(c);
    gaddr e = c->r[2];
    ARG(1) = 3;
    if (e != 0) {
        *B(e + 9) = 3;
        *H(e + 18) = (uint16_t)S16(o + 18);
        *B(e + 78) = 0x51;
        *H(e + 22) = (uint16_t)(S16(o + 22) + 0x10);
        *W(e + 88) = o;
        *B(e + 72) = 1;
        *H(e + 86) = *H(o + 86);
        *W(e + 164) = *W(o + 164);
        ARG(0) = e; ARG(1) = (uint32_t)(int32_t)S8(*W(o + 88) + 337); ARG(2) = 0x220;
        c->r[31] = 0x0893A7D4u; f_0887C408(c);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): with +3 set, its fighter's (+88) mark (+0x334) placed at the effect and cue 0x225; the
   fighter's +536 count runs down (at 0: +228 and the effect's +260 cleared), then 0x088230F4. */
SFA_FUNC(08980C24, effect_mark_cue) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr f = *W(o + 88);
    c->r[17] = f;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (*B(o + 3) != 0) {
        gaddr m = f + 0x334;
        *B(m + 4) = 0;
        *H(m) = (uint16_t)S16(o + 18);
        *H(m + 2) = (uint16_t)S16(o + 22);
        side_cue(c, f, 0x225, 0x08980C6Cu);
    }
    uint32_t n = (uint32_t)*B(f + 536) - 1;
    *B(f + 536) = (uint8_t)n;
    if ((n & 0xFF) == 0) { *W(f + 228) = 0; *B(o + 260) = 0; }
    ARG(0) = o; c->r[31] = 0x08980C94u; f_088230F4(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter, n): entry n of the fighter's state (+338) voice list (0x08A3E6D8): its bank cue (low byte;
   none for -1) and its +2 returned, else 0. */
SFA_FUNC(088D2C8C, state_voice) {
    gaddr f = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    gaddr e = *W(0x08A3E6D8u + ((uint32_t)S8(f + 338) << 2)) + (ARG(1) << 2);
    *W(c->r[29] + 16) = s0;
    c->r[16] = e;
    uint32_t h = *H(e);
    *W(c->r[29] + 20) = ra;
    uint32_t r = 0;
    if ((int16_t)h != -1 && (int8_t)(h & 0xFF) != -1) {
        ARG(0) = f; ARG(1) = h & 0xFF;
        c->r[31] = 0x088D2CECu; f_088D2E04(c);
        r = *H(e + 2);
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (item, keys): cue 0x38 and the item's three-way choice (+386: -1, 0, 1) stepped left (bit 3) or
   right, wrapping. */
SFA_FUNC(088A3AE0, menu_toggle3) {
    gaddr o = ARG(0);
    uint32_t keys = ARG(1) & 0xFFFF;
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = o; c->r[17] = keys;
    *W(c->r[29] + 24) = ra;
    plain_cue(c, 0x38, 0x088A3B08u);
    int32_t v = S8(o + 386);
    if (keys & 8) { v = (int16_t)(v - 1); if (v < -1) v = 1; }
    else { v = (int16_t)(v + 1); if (v >= 2) v = -1; }
    *B(o + 386) = (uint8_t)v;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (effect): launched: +4 steps on, speeds +100 = 4.0 up, +108 = -1/8, +96 = +-1.0 by 0x0898820C; cue
   0x225 from its fighter's (+88) side, its animation (0x08A92E1C +8) and 0x0898825C. */
SFA_FUNC(08988178, effect_launch_cue) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 24) = s2;
    gaddr f = *W(o + 88);
    c->r[18] = f;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFE000u;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    ARG(0) = o; c->r[31] = 0x089881BCu; f_0898820C(c);
    c->r[17] = c->r[2] != 0 ? 0xFFFF0000u : 0x10000u;
    *W(o + 96) = c->r[17];
    side_cue(c, f, 0x225, 0x089881D8u);
    ARG(0) = o; ARG(1) = *W(0x08A92E1Cu + 8);
    c->r[31] = 0x089881ECu; f_0882196C(c);
    ARG(0) = o; c->r[31] = 0x089881F4u; f_0898825C(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (task): 0x08881D9C (0, 0xE0, 0x80), the step (+4) moves on, +82 = 16, the stage step (the battle side
   +92's +348) kept at +78 picks the animation (0x08A299FC +36 list), and step 9 plays cue 0x12F. */
SFA_FUNC(08880ED4, stage_step_voice) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr sd = *W(BATTLE + 92);
    c->r[17] = sd;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 24) = ra;
    ARG(1) = 0; ARG(2) = 0xE0; ARG(3) = 0x80;
    c->r[31] = 0x08880F04u; f_08881D9C(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = 0x10;
    *B(o + 78) = (uint8_t)*H(sd + 348);
    ARG(0) = o; ARG(1) = *W(0x08A299FCu + 36) + ((uint32_t)(int32_t)S8(o + 78) << 3);
    c->r[31] = 0x08880F40u; f_0882196C(c);
    uint32_t st = *H(sd + 348);
    if (st == 9) plain_cue(c, st + 0x126, 0x08880F5Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
