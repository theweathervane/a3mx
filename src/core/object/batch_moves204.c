/* Steps of 83 - 84 instructions: a projectile pair with a voice by strength (0x089307E4), the down
   step that ends a knock-down in a get-up or a timed rise (0x0885DE60, 6,845 calls) and a jump that
   lands into move 0x22 by strength (0x088FE41C). Offsets stay numbers until something shows what they
   are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_088616EC(Cpu *c); void f_08861724(Cpu *c); void f_08822D2C(Cpu *c);
void f_08930930(Cpu *c); void f_0887C408(Cpu *c); void f_08821368(Cpu *c); void f_0887C614(Cpu *c);
void f_088FE670(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void cue(Cpu *c, gaddr o, uint32_t n, uint32_t back) {
    ARG(0) = o; ARG(1) = (uint32_t)(int32_t)S8(o + 337); ARG(2) = n; c->r[31] = back; f_0887C408(c);
}

/* (fighter): ticked; a frame flag steps +7 on (+82 = 5) with voice 0x220 (0x221 with strength) and
   spawns projectile 0xD (set up by 0x08930930; +260 from 0x08A6DB12 by strength, +311 = 2, +312 = 8);
   with +251 and the meter (+286) at +662 or more a second one, its +280 = +662. */
SFA_FUNC(089307E4, move_flag_projectiles_d) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s[5];
    c->r[29] -= 0x30;
    for (int i = 0; i < 5; i++) { s[i] = c->r[16 + i]; *W(c->r[29] + 16 + 4 * i) = s[i]; }
    *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    call1(c, o, f_08861848, 0x08930808u);
    if (S8(o + 77) != 0) {
        uint32_t st = *B(o + 7);
        *B(o + 7) = (uint8_t)(st + 1);
        int32_t sg = S8(o + 194);
        *H(o + 82) = 5;
        if (sg == 0) cue(c, o, 0x220, 0x08930844u);
        else cue(c, o, 0x221, 0x0893085Cu);
        ARG(0) = o; ARG(1) = 0xD; ARG(2) = 0; c->r[31] = 0x0893086Cu; f_08822D2C(c);
        gaddr e = c->r[2];
        c->r[20] = e;
        if (e != 0) {
            ARG(0) = o; ARG(1) = e; c->r[31] = 0x08930880u; f_08930930(c);
            gaddr tab = 0x08A6DB12u;
            c->r[19] = tab; c->r[18] = 2; c->r[17] = 8;
            *B(e + 260) = (uint8_t)S16(tab + (uint32_t)(S8(o + 194) * 2));
            *B(e + 311) = 2; *B(e + 312) = 8;
            if (S8(o + 251) != 0 && !(S16(o + 286) < S16(o + 662))) {
                ARG(0) = o; ARG(1) = 0xD; ARG(2) = 0; c->r[31] = 0x089308D8u; f_08822D2C(c);
                gaddr e2 = c->r[2];
                c->r[20] = e2;
                if (e2 != 0) {
                    ARG(0) = o; ARG(1) = e2; c->r[31] = 0x089308ECu; f_08930930(c);
                    *B(e2 + 280) = (uint8_t)S16(o + 662);
                    uint32_t v = (uint32_t)S16(tab + (uint32_t)(S8(o + 194) * 2));
                    *B(e2 + 311) = 2; *B(e2 + 260) = (uint8_t)v; *B(e2 + 312) = 8;
                }
            }
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = s[i];
    c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (fighter): down; once the move ends: with +702 a recovery roll (+116 = 6, hit and hold marks
   cleared, +701 by 5 unless +810 = 8 and by 16 with +308, voice 0x204 + +414; character 0x24 gets up at
   once with voice 0x2E, others rise after a random wait from 0x08A192E8 with move 0x1A of list 2);
   without it the get-up (state 1 / 1 / 2 / 0, marks cleared). Else ticked. */
SFA_FUNC(0885DE60, fighter_down_end) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t e = S8(o + 75);
    enter(c, o, sv);
    if (!(e & 0x80)) { call1(c, o, f_08861848, 0x0885DFA0u); leave(c, sv); return; }
    int getup = 1;
    if (S8(o + 702) != 0) {
        *B(o + 116) = 6; *B(o + 7) = 0;
        *B(o + 595) = 0; *B(o + 588) = 0; *B(o + 770) = 0; *B(o + 592) = 0; *B(o + 270) = 0;
        *B(o + 696) = 0; *B(o + 625) = 0; *B(o + 624) = 0; *B(o + 702) = 0; *B(o + 700) = 0;
        int32_t k = S8(o + 810);
        *B(o + 699) = 0;
        if (k != 8) *B(o + 701) = (uint8_t)(*B(o + 701) + 5);
        if (S8(o + 308) != 0) *B(o + 701) = (uint8_t)(*B(o + 701) + 0x10);
        cue(c, o, (uint32_t)(S8(o + 414) + 0x204), 0x0885DF08u);
        if (S8(o + 338) == 0x24) cue(c, o, 0x2E, 0x0885DF28u);
        else {
            getup = 0;
            call1(c, o, f_08821368, 0x0885DF68u);
            *H(o + 82) = (uint16_t)S16(0x08A192E8u + ((c->r[2] & 0xF) << 1));
            call2(c, o, 0x1A, f_08861724, 0x0885DF90u);
        }
    }
    if (getup) {
        *B(o + 4) = 1; *B(o + 5) = 1; *B(o + 6) = 2; *B(o + 7) = 0;
        *B(o + 696) = 0; *B(o + 770) = 0; *B(o + 592) = 0; *B(o + 270) = 0;
        *B(o + 539) = 0; *B(o + 541) = 0; *B(o + 698) = 0;
    }
    leave(c, sv);
}

/* (fighter): below strength 2 a fall (+73 = 1, 0x088FE670, +20 by +100 by +108) that at the floor lands
   (+73 cleared) and is ticked on frame flags; from strength 2 ticked, and ended a hop (+7 on, x 12.0 /
   -0.5, y 9.0 / -0.375) into move 0x21, else 0x088FE670 until a frame flag. Landing: cue 0x22 / 0x23,
   step 6 and move 0x22 + strength. */
SFA_FUNC(088FE41C, move_jump_land_22_b) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t s = S8(o + 194);
    enter(c, o, sv);
    int land = 0;
    if (s < 2) {
        if (S8(o + 77) != 0) call1(c, o, f_08861848, 0x088FE55Cu);
        else {
            *B(o + 73) = 1;
            call1(c, o, f_088FE670, 0x088FE500u);
            uint32_t vy = *W(o + 100);
            *W(o + 20) = *W(o + 20) + vy;
            int32_t floor = S16(o + 132);
            *W(o + 100) = vy + *W(o + 108);
            if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x088FE544u);
            else { *H(o + 22) = (uint16_t)floor; *B(o + 73) = 0; land = 1; }
        }
    } else {
        call1(c, o, f_08861848, 0x088FE444u);
        if (S8(o + 75) < 0) {
            if (S8(o + 194) != 0) {
                uint32_t st = *B(o + 7);
                *B(o + 7) = (uint8_t)(st + 1);
                *W(o + 96) = 0xC0000; *W(o + 104) = 0xFFFF8000u; *W(o + 100) = 0x90000; *W(o + 108) = 0xFFFFA000u;
                call2(c, o, 0x21, f_088616EC, 0x088FE4DCu);
            } else land = 1;
        } else if (S8(o + 77) == 0) call1(c, o, f_088FE670, 0x088FE464u);
    }
    if (land) {
        call1(c, o, f_0887C614, 0x088FE484u);
        int32_t sg = S8(o + 194);
        *B(o + 7) = 6;
        call2(c, o, (uint32_t)(sg + 0x22) & 0xFFFF, f_088616EC, 0x088FE4A0u);
    }
    leave(c, sv);
}
