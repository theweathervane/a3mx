/* Move steps of 52 - 54 instructions: a part character's arrival by strength, an end that spawns a copy
   of the fighter (0x08903FE0), a slow hop with a guard pick, a fall that lands on a held direction or a
   wait, a flight with a cue, a charge that counts by the buttons held (0x088FF2E8), a ground check
   that flips a picture (0x0891F938), and a drift that steps on. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861674(Cpu *c);
void f_088D3EC0(Cpu *c); void f_088D31EC(Cpu *c); void f_089CA064(Cpu *c); void f_088D35D0(Cpu *c);
void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c); void f_088D3180(Cpu *c); void f_08822D2C(Cpu *c);
void f_0886E294(Cpu *c); void f_0896F678(Cpu *c); void f_0896F5F8(Cpu *c); void f_0887C408(Cpu *c);
void f_0887C614(Cpu *c); void f_08867BF0(Cpu *c); void f_08881F94(Cpu *c); void f_08822EAC(Cpu *c);
void f_08864D40(Cpu *c); void f_089B14B0(Cpu *c); void f_089B2A18(Cpu *c); void f_089B298C(Cpu *c);

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
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

/* (character): the part ticked, 0x088D31EC, 0x089CA064; when 0x088D35D0 gives 1, 0x088D36D0, 0x088D3960
   (0, 0), then below strength 2 step 9 (+73 / +235 cleared) into move 0x2D + strength, else +7 on, a
   hop (x 1.5, y 4.0 / -0.125), 0x088D3180 and move 0x30. */
SFA_FUNC(089C8B04, part_arrive_by_strength) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C8B18u);
    call1(c, o, f_088D31EC, 0x089C8B20u);
    call1(c, o, f_089CA064, 0x089C8B28u);
    call1(c, o, f_088D35D0, 0x089C8B30u);
    if (c->r[2] == 1) {
        call1(c, o, f_088D36D0, 0x089C8B44u);
        ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089C8B54u; f_088D3960(c);
        int32_t s = S8(o + 194);
        if (s < 2) {
            *B(o + 7) = 9; *B(o + 73) = 0; *B(o + 235) = 0;
            call2(c, o, (uint32_t)(s + 0x2D), f_088616EC, 0x089C8BC4u);
        } else {
            bump(o + 7);
            *W(o + 96) = 0x18000; *W(o + 100) = 0x40000; *W(o + 104) = 0; *W(o + 108) = 0xFFFFE000u;
            call1(c, o, f_088D3180, 0x089C8B98u);
            call2(c, o, 0x30, f_088616EC, 0x089C8BA4u);
        }
    }
    leave(c, sv);
}

/* (fighter): ended: a copy (0x08822D2C 0x15 / 0) takes its picture, facing, place and floor (+255 = 3,
   +347 = 1, tied to it) with step 4; move 0x23 + 2 x strength; else ticked. */
SFA_FUNC(08903FE0, move_end_spawn_copy) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t e75 = S8(o + 75);
    enter(c, o, sv);
    if (e75 < 0) {
        ARG(0) = o; ARG(1) = 0x15; ARG(2) = 0; c->r[31] = 0x08904008u; f_08822D2C(c);
        gaddr e = c->r[2];
        if (e != 0) {
            *B(o + 7) = 4;
            *B(e + 255) = 3; *B(e + 347) = 1;
            *B(e + 14) = *B(o + 14); *B(e + 11) = *B(o + 11);
            *B(e + 122) = *B(o + 122); *B(e + 121) = *B(o + 121);
            *H(e + 38) = *H(o + 38);
            *B(e + 12) = *B(o + 12); *B(e + 13) = *B(o + 13);
            *W(e + 16) = *W(o + 16); *W(e + 20) = *W(o + 20);
            uint32_t fl = *H(o + 132);
            *W(e + 88) = o; *H(e + 132) = (uint16_t)fl;
        }
        call2(c, o, (uint32_t)((S8(o + 194) << 1) + 0x23) & 0xFFFF, f_088616EC, 0x08904094u);
    } else call1(c, o, f_08861848, 0x089040A4u);
    leave(c, sv);
}

/* (fighter): +543 = 2, ticked; without a negative frame flag +7 steps on, +237 cleared, +573 from
   0x08A85B8C (by strength, or by 0x0886E294 with +81) and a slow hop (x by strength from 0x08A85B90 /
   -0.125, y 2.0 / -0.1875). */
SFA_FUNC(0895E774, move_tick_slow_hop) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 543) = 2;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    call1(c, o, f_08861848, 0x0895E79Cu);
    if (S8(o + 77) >= 0) {
        uint32_t st = *B(o + 7);
        int32_t k = S8(o + 81);
        *B(o + 7) = (uint8_t)(st + 1);
        c->r[18] = 0x08A85B8Cu; c->r[17] = 0x08A85B90u;
        *B(o + 237) = 0;
        uint32_t i;
        if (k != 0) { call1(c, o, f_0886E294, 0x0895E7D8u); i = c->r[2]; }
        else i = (uint32_t)S8(o + 194);
        *B(o + 573) = *B(0x08A85B8Cu + i);
        uint32_t vx = *W(0x08A85B90u + ((uint32_t)S8(o + 194) << 2));
        c->r[18] = vx;
        *W(o + 96) = vx; *W(o + 104) = 0xFFFFE000u; *W(o + 100) = 0x20000; *W(o + 108) = 0xFFFFD000u;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (fighter): with +123 step 3 and 0x0896F678; else it lands (step 4, at the floor, +20 / +73 cleared,
   move 7 + 2 x strength) when +238 holds its facing's bit, with +648, or once the +82 wait runs out;
   otherwise 0x0896F5F8 and ticked. */
SFA_FUNC(0896D6F8, move_wait_land_7) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f != 0) { *B(o + 7) = 3; call1(c, o, f_0896F678, 0x0896D720u); }
    else {
        int land = (S8(o + 238) & (1 << (*B(o + 11) & 31))) != 0 || S8(o + 648) != 0;
        if (!land) {
            int8_t t = (int8_t)(S8(o + 0x52) - 1);
            *B(o + 0x52) = (uint8_t)t;
            land = t == 0;
        }
        if (land) {
            int32_t floor = S16(o + 132);
            *B(o + 7) = 4;
            int32_t s = S8(o + 194);
            *H(o + 22) = (uint16_t)floor; *H(o + 20) = 0; *B(o + 73) = 0;
            call2(c, o, (uint32_t)(s + s + 7) & 0xFFFF, f_088616EC, 0x0896D7A4u);
        } else {
            call1(c, o, f_0896F5F8, 0x0896D7B4u);
            call1(c, o, f_08861848, 0x0896D7BCu);
        }
    }
    leave(c, sv);
}

/* (fighter): a pending +123 (cleared) cues 0x20D; ticked on a frame flag; else it flies (+73 = 1) and at
   or below the floor +7 steps on, at the floor, +235 / +73 cleared, cue 0x22 / 0x23 and move 0x20 of
   its list; else ticked. */
SFA_FUNC(088EE774, move_cue_fly_land_20) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t f = S8(o + 123);
    enter(c, o, sv);
    if (f != 0) { *B(o + 123) = 0; cue(c, o, 0x20D, 0x088EE7A0u); }
    if (S8(o + 77) != 0) call1(c, o, f_08861848, 0x088EE7B4u);
    else {
        uint32_t vy = *W(o + 100);
        *B(o + 73) = 1;
        *W(o + 20) = *W(o + 20) + vy;
        *W(o + 16) = *W(o + 16) + *W(o + 96);
        *W(o + 100) = vy + *W(o + 108);
        int32_t floor = S16(o + 132);
        if (floor < S16(o + 22)) call1(c, o, f_08861848, 0x088EE808u);
        else {
            uint32_t st = *B(o + 7);
            *B(o + 7) = (uint8_t)(st + 1);
            *W(o + 20) = (uint32_t)floor << 16;
            *B(o + 235) = 0; *B(o + 73) = 0;
            call1(c, o, f_0887C614, 0x088EE830u);
            call2(c, o, 0x20, f_08861674, 0x088EE83Cu);
        }
    }
    leave(c, sv);
}

/* (fighter): a frame flag sets step 7, 0x08881F94 and cue 0x30; else it slides (+16 by +96 by +104),
   +82 cleared, and unlocked the buttons (0x08867BF0 (0x80) | +760) step it on: bit 0x80 at once, 0x100
   with +82 = 2, 0x200 with +82 = 4; ticked. */
SFA_FUNC(088FF2E8, move_charge_by_buttons) {
    gaddr o = ARG(0); uint32_t sv[2];
    int32_t k = S8(o + 77);
    enter(c, o, sv);
    if (k != 0) {
        *B(o + 7) = 7;
        call1(c, o, f_08881F94, 0x088FF398u);
        cue(c, o, 0x30, 0x088FF3A8u);
    } else {
        uint32_t v = *W(o + 96);
        *W(o + 16) = *W(o + 16) + v;
        *W(o + 96) = v + *W(o + 104);
        int32_t lock = S8(o + 373);
        *H(o + 82) = 0;
        if (lock == 0) {
            call2(c, o, 0x80, f_08867BF0, 0x088FF334u);
            uint32_t b = (*H(o + 760) | (c->r[2] & 0xFFFF)) & 0xFFFF;
            int add = -1;
            if (b & 0x80) add = 0;
            else if (b & 0x100) add = 1;
            else if (b & 0x200) add = 2;
            if (add >= 0) {
                for (int i = 0; i < add; i++) *H(o + 82) = (uint16_t)(S16(o + 82) + 2);
                bump(o + 7);
            }
        }
    }
    call1(c, o, f_08861848, 0x088FF3B0u);
    leave(c, sv);
}

/* (fighter): +20 by +100; within 40 of the floor +6 steps on, +78 = 1 and (with +430) effect 0x1E / 2
   tied to it, 0x08864D40 and move 0x34; above that with +430 0x08864D40 and the picture flipped (+10 ^ 1
   into +13); ticked. */
SFA_FUNC(0891F938, move_near_floor_34) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    *W(o + 20) = *W(o + 20) + *W(o + 100);
    int32_t floor = S16(o + 132);
    int32_t y = (int32_t)(((uint32_t)S16(o + 22) - 0x28) & 0xFFFF);
    int32_t k = S8(o + 430);
    if (y < (floor & 0xFFFF)) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *B(o + 78) = 1;
        if (k != 0) {
            ARG(0) = 0x1E; ARG(1) = 2; c->r[31] = 0x0891F99Cu; f_08822EAC(c);
            gaddr e = c->r[2];
            if (e != 0) *W(e + 88) = o;
            call1(c, o, f_08864D40, 0x0891F9B0u);
            call2(c, o, 0x34, f_0886163C, 0x0891F9BCu);
            leave(c, sv);
            return;
        }
    } else if (k != 0) {
        call1(c, o, f_08864D40, 0x0891F9DCu);
        *B(o + 10) = (uint8_t)(*B(o + 10) ^ 1);
        *B(o + 13) = (uint8_t)(*B(o + 13) + *B(o + 10));
    }
    call1(c, o, f_08861848, 0x0891FA00u);
    leave(c, sv);
}

/* (fighter): +100's low half drops by +98; it steps on (+74 = 1, +7 on, x brake -0.625) once that is
   negative, with +648, when 0x089B14B0 says so or +238 holds its facing's bit; else 0x089B2A18,
   0x089B298C and ticked until a frame flag. */
SFA_FUNC(089B1330, move_drift_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    gaddr p = o + 100;
    *H(p) = (uint16_t)(*H(p) - (uint32_t)S16(o + 98));
    int16_t v = (int16_t)*H(p);
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int go = v < 0 || S8(o + 648) != 0;
    if (!go) {
        call1(c, o, f_089B14B0, 0x089B1384u);
        go = c->r[2] != 0 || (S8(o + 238) & (1 << (*B(o + 11) & 31))) != 0;
    }
    if (go) {
        uint32_t st = *B(o + 7);
        *B(o + 74) = 1; *B(o + 7) = (uint8_t)(st + 1);
        *W(o + 104) = 0xFFFF6000u;
        call1(c, o, f_08861848, 0x089B13F4u);
    } else {
        call1(c, o, f_089B2A18, 0x089B13ACu);
        call1(c, o, f_089B298C, 0x089B13B4u);
        if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089B13C8u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}
