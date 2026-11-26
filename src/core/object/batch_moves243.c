/* A hop-and-land special in seven steps (0x08988D54), and an effect spawner after the cue 0x0887C65C
   (0x0887FD2C): with no choice made one effect (object 7,
   kind 3) placed from the 0x08A24798 offsets by +338, otherwise one effect per strength level. Offsets
   stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887C65C(Cpu *c); void f_08822EAC(Cpu *c); void f_08837098(Cpu *c); void f_0887C408(Cpu *c);
void f_088616EC(Cpu *c); void f_08861848(Cpu *c); void f_08987440(Cpu *c); void f_0887C614(Cpu *c);
void f_0886469C(Cpu *c); void f_0886DB2C(Cpu *c);

static gaddr spawn_fx(Cpu *c, uint32_t kind, uint32_t sub, uint32_t back) {
    ARG(0) = kind; ARG(1) = sub; c->r[31] = back; f_08822EAC(c);
    return c->r[2];
}

/* (fighter, a, b): the cue 0x0887C65C (a, b); with no choice made one effect (kind 3: +78 0x46 for +338
   0x1C, else 0x45; facing and x / y offset by +338 from 0x08A24798) and 1, failing that 0; otherwise one
   effect (kind strength) for each level from the strength down (+78 the level, +112 / +114 = a / b) and
   1. A 0x30 frame keeping s0..s6. */
SFA_FUNC(0887FD2C, cue_effects) {
    gaddr o = ARG(0);
    uint32_t a0 = ARG(1), b0 = ARG(2);
    uint32_t sv[7] = { c->r[16], c->r[17], c->r[18], c->r[19], c->r[20], c->r[21], c->r[22] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 7; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 44) = ra;
    int32_t a = (int16_t)a0, b = (int16_t)b0;
    c->r[17] = (uint32_t)a; c->r[18] = (uint32_t)b; c->r[16] = o;
    ARG(0) = o; ARG(1) = a0; ARG(2) = b0; c->r[31] = 0x0887FD68u; f_0887C65C(c);
    uint32_t ret;
    if (S8(o + 386) == -1) {
        gaddr p = spawn_fx(c, 7, 3, 0x0887FD80u);
        c->r[8] = p;
        if (p == 0) ret = 0;
        else {
            *B(p + 9) = 0; *B(p + 12) = 0; *B(p + 13) = 0; *B(p + 73) = 0;
            *B(p + 14) = *B(o + 14); *H(p + 86) = 5; *H(p + 24) = *H(o + 24); *W(p + 88) = o;
            const gaddr tab = 0x08A24798u;
            *B(p + 78) = S8(o + 338) == 0x1C ? 0x46 : 0x45;
            uint32_t f = *B(o + 11);
            *B(p + 11) = (uint8_t)f;
            gaddr e = tab + (uint32_t)(S8(o + 338) * 4);
            int32_t dx = S16(e);
            *H(p + 18) = (uint16_t)(f == 0 ? S16(o + 18) + dx : S16(o + 18) - dx);
            int32_t dy = S16(tab + (uint32_t)(S8(o + 338) * 4) + 2);
            *H(p + 22) = (uint16_t)(S16(o + 22) + dy);
            /* callers read the scratch registers this leaves (0x089072BC ticks whatever a0 holds) */
            c->r[4] = (uint32_t)dy; c->r[5] = tab; c->r[6] = (uint32_t)dx; c->r[7] = 1; c->r[9] = f;
            ret = 1;
        }
    } else {
        int32_t lv = S8(o + 194);
        c->r[20] = 1; c->r[21] = (uint32_t)lv;
        c->r[4] = 7; c->r[5] = 0xFFFFFFFFu;
        if (lv >= 0) {
            c->r[22] = 1; c->r[19] = 5;
            while (lv >= 0) {
                gaddr p = spawn_fx(c, 7, (uint32_t)S8(o + 194) & 0xFF, 0x0887FEB0u);
                if (p != 0) {
                    *B(p + 9) = 1; *B(p + 12) = *B(o + 12); *B(p + 13) = 0; *B(p + 73) = 0;
                    *B(p + 78) = (uint8_t)lv; *B(p + 14) = *B(o + 14);
                    *H(p + 86) = 5; *H(p + 24) = *H(o + 24); *W(p + 88) = o;
                    *H(p + 112) = (uint16_t)a; *H(p + 114) = (uint16_t)b;
                    c->r[5] = *H(o + 24);
                }
                c->r[4] = p;
                lv--;
                c->r[21] = (uint32_t)lv;
            }
        }
        ret = 1;
    }
    c->r[2] = ret;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[20] = sv[4]; c->r[21] = sv[5]; c->r[22] = sv[6];
    c->r[31] = ra; c->r[29] += 0x30;
}

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
/* landed on the floor (+132), +73 / +20 cleared and the landing cue */
static void land(Cpu *c, gaddr o, uint32_t back) {
    *H(o + 22) = *H(o + 132); *B(o + 73) = 0; *H(o + 20) = 0;
    call1(c, o, f_0887C614, back);
}
static int above_floor(gaddr o) { return S16(o + 132) < S16(o + 22); }

/* (fighter): step 0: step 1, +262 = 1, +235 / +696 cleared, 0x08837098, cue 0x0887C408 (+337, 0x208), a
   backward jump (x -3.0, y 10.0, fall -0.625) and move 0xB; 1: ticked, in the air at the end of the frame
   flag; 2: ticked, on with a frame flag, 0x08987440 and landed (step 3); 3: 0x08987440 and landed (step 4);
   4: ticked, on frame flag 2 step 5 with y 5.0; 5: 0x08987440, ticked while above the floor until a frame
   flag, then +262 cleared, step 6, +74 = 1 and ticked; 6: standing once ended, else 0x0886DB2C and ticked. */
SFA_FUNC(08988D54, hop_land_special) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    uint32_t st = *B(o + 7);
    c->r[18] = 1;
    if (st == 0) {
        *B(o + 7) = 1; *B(o + 262) = 1; *B(o + 235) = 0; *B(o + 696) = 0;
        call1(c, o, f_08837098, 0x08988D94u);
        ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337); ARG(2) = 0x208; c->r[31] = 0x08988DA4u; f_0887C408(c);
        *W(o + 96) = 0xFFFD0000u; *W(o + 100) = 0xA0000; *W(o + 104) = 0; *W(o + 108) = 0xFFFF6000u;
        ARG(0) = o; ARG(1) = 0xB; c->r[31] = 0x08988DD0u; f_088616EC(c);
        goto out;
    }
    c->r[17] = 2;
    switch (st) {
    case 1:
        call1(c, o, f_08861848, 0x08988DE8u);
        if (S8(o + 77) == 0) { *B(o + 73) = 1; *B(o + 7) = (uint8_t)(*B(o + 7) + 1); }
        break;
    case 2:
        call1(c, o, f_08861848, 0x08988E18u);
        if (S8(o + 77) != 0) *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call1(c, o, f_08987440, 0x08988E38u);
        if (!above_floor(o)) { *B(o + 7) = 3; land(c, o, 0x08988E6Cu); }
        break;
    case 3:
        call1(c, o, f_08987440, 0x08988E84u);
        if (!above_floor(o)) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); land(c, o, 0x08988EC0u); }
        break;
    case 4:
        call1(c, o, f_08861848, 0x08988ED8u);
        if (S8(o + 77) == 2) { *B(o + 7) = (uint8_t)(*B(o + 7) + 1); *W(o + 100) = 0x50000; *B(o + 77) = 0; }
        break;
    case 5:
        call1(c, o, f_08987440, 0x08988F10u);
        if (above_floor(o)) { if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08988F58u); }
        else {
            *B(o + 262) = 0; *B(o + 7) = (uint8_t)(*B(o + 7) + 1); *B(o + 74) = 1;
            call1(c, o, f_08861848, 0x08988F40u);
        }
        break;
    case 6:
        if (S8(o + 75) < 0) call1(c, o, f_0886469C, 0x08988F7Cu);
        else { call1(c, o, f_0886DB2C, 0x08988F8Cu); call1(c, o, f_08861848, 0x08988F94u); }
        break;
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
