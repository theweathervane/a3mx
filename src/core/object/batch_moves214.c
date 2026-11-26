/* Steps of 113 instructions: the hit sparks spawned at an offset (0x08880034: one big spark for
   character 0's strong attack with no choice made, else one per strength level), and a summoned
   helper that rides in beside its target, waits, hops and lands (0x08972670). Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_08862004(Cpu *c); void f_0887C614(Cpu *c);
void f_0887C65C(Cpu *c); void f_08822EAC(Cpu *c); void f_08821330(Cpu *c); void f_089789AC(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}

/* (fighter, x offset, y offset): 0x0887C65C, then for character 0 at strength 2 with no choice made one
   big spark (0x08822EAC 7 / 4: +9 = 1, +78 = 0x51, +86 = 5, its picture, at the offset toward its
   facing; gives 0); otherwise one spark 7 / strength for each level down from the strength (+73 = -1,
   +78 = the level, +112 / +114 the offset, +4 = 2 at strength 0; gives 1). */
SFA_FUNC(08880034, hit_sparks_at) {
    gaddr o = ARG(0);
    int32_t dx = (int16_t)ARG(1), dy = (int16_t)ARG(2);
    uint32_t ra = c->r[31], s[8];
    c->r[29] -= 0x40;
    for (int i = 0; i < 8; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 48) = ra;
    c->r[16] = o; c->r[17] = (uint32_t)dx; c->r[18] = (uint32_t)dy;
    c->r[31] = 0x08880074u; f_0887C65C(c);
    int32_t lv = S8(o + 194);
    c->r[22] = (uint32_t)lv;
    uint32_t ret;
    if (lv == 2 && S8(o + 338) == 0 && S8(o + 386) == -1) {
        ARG(0) = 7; ARG(1) = 4; c->r[31] = 0x088800A8u; f_08822EAC(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(e + 9) = 1; *B(e + 12) = 0; *B(e + 13) = 0; *B(e + 78) = 0x51;
            *B(e + 14) = *B(o + 14);
            uint32_t pic = *H(o + 24);
            *H(e + 86) = 5; *H(e + 24) = (uint16_t)pic;
            *W(e + 88) = o;
            int32_t x = S16(o + 18);
            *H(e + 18) = (uint16_t)(*B(o + 11) == 0 ? x + dx : x - dx);
            *H(e + 22) = (uint16_t)(S16(o + 22) + dy);
        }
        ret = 0;
    } else {
        c->r[20] = 1;
        if (lv >= 0) {
            c->r[19] = 0xFFFFFFFFu; c->r[21] = 5; c->r[23] = 2;
            while (lv >= 0) {
                ARG(0) = 7; ARG(1) = (uint32_t)S8(o + 194) & 0xFF; c->r[31] = 0x0888016Cu; f_08822EAC(c);
                gaddr e = c->r[2];
                if (e != 0) {
                    *B(e + 9) = 1; *B(e + 12) = 0; *B(e + 13) = 0; *B(e + 73) = 0xFF; *B(e + 78) = (uint8_t)lv;
                    *B(e + 14) = *B(o + 14);
                    uint32_t pic = *H(o + 24);
                    *H(e + 86) = 5; *H(e + 24) = (uint16_t)pic;
                    *W(e + 88) = o;
                    *H(e + 112) = (uint16_t)dx; *H(e + 114) = (uint16_t)dy;
                    if (S8(o + 194) == 0) *B(e + 4) = 2;
                }
                lv--;
                c->r[22] = (uint32_t)lv;
            }
        }
        ret = 1;
    }
    c->r[2] = ret;
    for (int i = 0; i < 8; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x40;
}

/* (helper, owner +88): step 0: 0x089789AC, +82 = 18, placed 480 / -96 from 0x08821330's object by its
   side and 38 up, ticked; 1: the +82 wait; 2: a frame flag (cleared) sets the owner's +9 = 2, and once
   ended a hop (+73 = 1, 48 up, y 4.0 / -0.375, x 0.5 and 22 toward its facing) into move 0x13, else
   ticked; later standing on landing (0x08862004: at the floor, cue 0x22 / 0x23, move 0), else
   ticked. */
SFA_FUNC(08972670, helper_ride_in) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint32_t st = *B(o + 7);
    *W(c->r[29] + 20) = ra;
    gaddr own = *W(o + 88);
    if (st == 0) {
        *B(o + 7) = 1;
        call1(c, o, f_089789AC, 0x0897269Cu);
        *H(o + 82) = 0x12;
        call1(c, o, f_08821330, 0x089726ACu);
        *H(o + 18) = *H(c->r[2] + 18);
        int32_t x = S16(o + 18);
        *H(o + 18) = (uint16_t)(S8(o + 337) != 0 ? x + 0x1E0 : x - 0x60);
        *H(o + 22) = (uint16_t)(S16(o + 22) + 0x26);
        call1(c, o, f_08861848, 0x089726E8u);
    } else if (st == 1) {
        uint32_t t = (uint32_t)S16(o + 82) - 1;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t == 0) *B(o + 7) = 2;
    } else if (st == 2) {
        if (S8(o + 77) != 0) { *B(o + 77) = 0; *B(own + 9) = 2; }
        if (S8(o + 75) & 0x80) {
            *B(o + 7) = 3; *B(o + 73) = 1;
            *H(o + 22) = (uint16_t)(S16(o + 22) + 0x30);
            *W(o + 100) = 0x40000; *W(o + 108) = 0xFFFFA000u;
            uint32_t f = *B(o + 11);
            *W(o + 104) = 0;
            int32_t x = S16(o + 18);
            if (f != 0) { *W(o + 96) = 0x8000; *H(o + 18) = (uint16_t)(x + 0x16); }
            else { *W(o + 96) = 0xFFFF8000u; *H(o + 18) = (uint16_t)(x - 0x16); }
            call2(c, o, 0x13, f_0886163C, 0x089727C8u);
        } else call1(c, o, f_08861848, 0x08972758u);
    } else {
        call1(c, o, f_08862004, 0x089727D8u);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0;
            int32_t floor = S16(o + 132);
            *B(o + 7) = 0; *B(o + 73) = 0; *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0;
            call1(c, o, f_0887C614, 0x08972818u);
            call2(c, o, 0, f_0886163C, 0x08972824u);
        } else call1(c, o, f_08861848, 0x089727E8u);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
