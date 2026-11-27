/* Command throws and the specials that start with a throw check (0x0886A6D4). Offsets stay numbers until
   something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886A2F4(Cpu *c); void f_0886A9FC(Cpu *c); void f_0886A6D4(Cpu *c); void f_08861848(Cpu *c);
void f_088616EC(Cpu *c); void f_0886DDCC(Cpu *c); void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c);
void f_08944DF8(Cpu *c);

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

/* (fighter, throw n): a throw whatever the opponent's state: the opponent picked (mode 0), unless its
   +296 entry's +5 is set or it is held (+573), thrown by 0x0886A9FC; else 0. */
SFA_FUNC(0886A66C, throw_any_state) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = n; c->r[17] = o;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 0; c->r[31] = 0x0886A68Cu; f_0886A2F4(c);
    gaddr p = *W(o + 92);
    if (*B(*W(p + 296) + 5) != 0 || S8(p + 573) != 0) c->r[2] = 0;
    else { ARG(0) = o; ARG(1) = p; ARG(2) = n; c->r[31] = 0x0886A6C0u; f_0886A9FC(c); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked until a frame flag; then throw 0x60: caught +7 on, move 0x2A and 0x0886DDCC, missed
   step 7. */
SFA_FUNC(0891225C, grab_60) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x0891227Cu);
    else {
        call2(c, o, 0x60, f_0886A6D4, 0x08912290u);
        if (c->r[2] == 0) *B(o + 7) = 7;
        else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x2A, f_088616EC, 0x089122B8u);
            call1(c, o, f_0886DDCC, 0x089122C0u);
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; at the end of the frame flag +237 cleared, +7 on and throw 0x53: caught step 4, +82
   cleared, +240 by strength from 0x08A90284 and move 0x41. */
SFA_FUNC(0897713C, grab_53) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08977150u);
    if (S8(o + 77) == 0) {
        *B(o + 237) = 0;
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, 0x53, f_0886A6D4, 0x08977178u);
        if (c->r[2] != 0) {
            int32_t s = S8(o + 194);
            *B(o + 7) = 4; *H(o + 82) = 0;
            *H(o + 240) = (uint16_t)S16(0x08A90284u + (uint32_t)(s + s));
            call2(c, o, 0x41, f_088616EC, 0x089771B0u);
        }
    }
    leave(c, sv);
}

/* (fighter): throw 0x50 (0x51 with +543): caught step 2, the meter by 0x14, 0x08837098, move 0xA and
   0x0886DDCC; missed special 9 from step 0 through 0x08944DF8. */
SFA_FUNC(0894496C, grab_50) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, o, S8(o + 543) != 0 ? 0x51 : 0x50, f_0886A6D4, 0x08944998u);
    if (c->r[2] != 0) {
        *B(o + 7) = 2;
        call2(c, o, 0x14, f_0886CA2C, 0x089449B4u);
        call1(c, o, f_08837098, 0x089449BCu);
        call2(c, o, 0xA, f_088616EC, 0x089449C8u);
        call1(c, o, f_0886DDCC, 0x089449D0u);
    } else {
        *B(o + 236) = 9; *B(o + 7) = 0;
        call1(c, o, f_08944DF8, 0x089449E8u);
    }
    leave(c, sv);
}


void f_0887CD7C(Cpu *c); void f_088D3EC0(Cpu *c); void f_088D34F4(Cpu *c); void f_088D2E04(Cpu *c);
void f_0886DEC8(Cpu *c); void f_0886469C(Cpu *c); void f_08862004(Cpu *c); void f_0887C614(Cpu *c);
void f_089178BC(Cpu *c);

/* (fighter): ticked until its animation ends; then throw 0x46: caught step 5, 0x0887CD7C, move 0x1E and
   0x0886DDCC; missed +7 on and move 4. */
SFA_FUNC(08969830, grab_46) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x089698ACu);
    else {
        call2(c, o, 0x46, f_0886A6D4, 0x08969854u);
        if (c->r[2] != 0) {
            *B(o + 7) = 5;
            call1(c, o, f_0887CD7C, 0x08969868u);
            call2(c, o, 0x1E, f_088616EC, 0x08969874u);
            call1(c, o, f_0886DDCC, 0x0896987Cu);
        } else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 4, f_088616EC, 0x0896989Cu);
        }
    }
    leave(c, sv);
}

/* (fighter): 0x088D3EC0; on a frame flag (cleared, +235 cleared) throw 0x2F: caught +7 on, the meter
   entry 4, 0x088D2E04 (0x23) and 0x0886DEC8; missed step 3 and move 0x1E. */
SFA_FUNC(089C1CAC, grab_2f) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_088D3EC0, 0x089C1CC0u);
    if (S8(o + 77) != 0) {
        *B(o + 235) = 0; *B(o + 77) = 0;
        call2(c, o, 0x2F, f_0886A6D4, 0x089C1CE0u);
        if (c->r[2] == 1) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 4, f_088D34F4, 0x089C1D04u);
            call2(c, o, 0x23, f_088D2E04, 0x089C1D10u);
            call1(c, o, f_0886DEC8, 0x089C1D18u);
        } else {
            *B(o + 7) = 3;
            call2(c, o, 0x1E, f_088616EC, 0x089C1D30u);
        }
    }
    leave(c, sv);
}

/* (fighter): standing once ended; ticked without a frame flag; with one, throw 0x44 + strength: caught
   +7 on, the meter by 0x14 and move 0x1A; missed ticked. */
SFA_FUNC(08915B68, grab_44s) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x08915B90u);
    else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08915BA8u);
    else {
        call2(c, o, (uint32_t)(S8(o + 194) + 0x44), f_0886A6D4, 0x08915BBCu);
        if (c->r[2] != 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x14, f_0886CA2C, 0x08915BE8u);
            call2(c, o, 0x1A, f_088616EC, 0x08915BF4u);
        } else call1(c, o, f_08861848, 0x08915BCCu);
    }
    leave(c, sv);
}

/* (fighter): ticked until it lands (0x08862004); then on the floor (+73 cleared, +20 at +132), the landing
   cue and throw 0x4A: caught +82 cleared, +7 on, 0x0887CD7C and 0x089178BC; missed step 8 and move 0x1B. */
SFA_FUNC(08916048, land_grab_4a) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08862004, 0x0891605Cu);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x0891606Cu);
    else {
        *B(o + 73) = 0;
        *W(o + 20) = (uint32_t)S16(o + 132) << 16;
        call1(c, o, f_0887C614, 0x08916088u);
        call2(c, o, 0x4A, f_0886A6D4, 0x08916094u);
        if (c->r[2] == 0) { *B(o + 7) = 8; call2(c, o, 0x1B, f_088616EC, 0x089160ACu); }
        else {
            *H(o + 82) = 0;
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call1(c, o, f_0887CD7C, 0x089160CCu);
            call1(c, o, f_089178BC, 0x089160D4u);
        }
    }
    leave(c, sv);
}


void f_0894496C(Cpu *c); void f_0886A66C(Cpu *c);

/* (fighter): +262 = 1, step 1, move strength + 0x21, the meter by 5, 0x08837098, +502 = 1 in the air, then
   the throw 0x0894496C. */
SFA_FUNC(089448FC, grab_50_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    int32_t s = S8(o + 194);
    *W(c->r[29] + 20) = s1; c->r[17] = 1;
    *B(o + 262) = 1; *B(o + 7) = 1;
    *W(c->r[29] + 24) = ra;
    call2(c, o, (uint32_t)(s + 0x21) & 0xFFFF, f_088616EC, 0x08944930u);
    call2(c, o, 5, f_0886CA2C, 0x0894493Cu);
    call1(c, o, f_08837098, 0x08944944u);
    if (S8(o + 80) != 0) *B(o + 502) = 1;
    call1(c, o, f_0894496C, 0x08944958u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): throw 0x4B whatever the opponent's state: caught +7 on, a lift (x 10.0, brake -0.875), +825 by
   strength from 0x08A58444 and move 0x13; missed step 6 and move strength + 0x17. */
SFA_FUNC(0890BBD4, grab_4b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, o, 0x4B, f_0886A66C, 0x0890BBECu);
    int32_t s = S8(o + 194);
    if (c->r[2] == 0) {
        *B(o + 7) = 6;
        call2(c, o, (uint32_t)(s + 0x17) & 0xFFFF, f_088616EC, 0x0890BC0Cu);
    } else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0xA0000; *W(o + 104) = 0xFFFF2000u;
        *B(o + 0x334 + 5) = (uint8_t)S8(0x08A58444u + (uint32_t)s);
        call2(c, o, 0x13, f_088616EC, 0x0890BC58u);
    }
    leave(c, sv);
}

/* (fighter): throw 0x3E + strength (0x4A + with a choice, +3 with +543): caught step 2, the meter by 0x14,
   0x08837098, move 5 and 0x0886DDCC; missed step 7 and ticked. */
SFA_FUNC(08944480, grab_3e) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t n = S8(o + 386) >= 0 ? 0x4A : 0x3E;
    n += (uint32_t)S8(o + 194);
    if (S8(o + 543) != 0) n += 3;
    call2(c, o, n, f_0886A6D4, 0x089444C0u);
    if (c->r[2] != 0) {
        *B(o + 7) = 2;
        call2(c, o, 0x14, f_0886CA2C, 0x089444DCu);
        call1(c, o, f_08837098, 0x089444E4u);
        call2(c, o, 5, f_088616EC, 0x089444F0u);
        call1(c, o, f_0886DDCC, 0x089444F8u);
    } else { *B(o + 7) = 7; call1(c, o, f_08861848, 0x0894450Cu); }
    leave(c, sv);
}

/* (fighter): ticked until its animation ends; then throw 0x55 + strength: caught step 3, the meter by 6,
   0x08837098, move 7 and 0x0886DDCC; missed +7 on and move 4. */
SFA_FUNC(089692D4, grab_55s) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x08969360u);
    else {
        call2(c, o, (uint32_t)(S8(o + 194) + 0x55), f_0886A6D4, 0x089692FCu);
        if (c->r[2] != 0) {
            *B(o + 7) = 3;
            call2(c, o, 6, f_0886CA2C, 0x08969314u);
            call1(c, o, f_08837098, 0x0896931Cu);
            call2(c, o, 7, f_088616EC, 0x08969328u);
            call1(c, o, f_0886DDCC, 0x08969330u);
        } else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 4, f_088616EC, 0x08969350u);
        }
    }
    leave(c, sv);
}


void f_0887C408(Cpu *c); void f_08821330(Cpu *c); void f_0892EB34(Cpu *c); void f_0886163C(Cpu *c);
void f_08944480(Cpu *c);

/* (fighter): +262 = 1, step 1, +640 cleared, the meter by 5, 0x08837098, move strength + 0x28, +502 = 1 in
   the air, then the throw 0x08944480. */
SFA_FUNC(08944410, grab_3e_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; c->r[16] = o; c->r[17] = 1;
    *B(o + 262) = 1; *B(o + 7) = 1; *B(o + 640) = 0;
    *W(c->r[29] + 24) = ra;
    call2(c, o, 5, f_0886CA2C, 0x0894443Cu);
    call1(c, o, f_08837098, 0x08944444u);
    call2(c, o, (uint32_t)(S8(o + 194) + 0x28) & 0xFFFF, f_088616EC, 0x08944458u);
    if (S8(o + 80) != 0) *B(o + 502) = 1;
    call1(c, o, f_08944480, 0x0894446Cu);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked until its animation ends; then the throw by strength from 0x08A9E7D8: caught +7 on,
   the meter by 0x14, move 4 and 0x0886DDCC; missed step 8 and move 1. */
SFA_FUNC(089A6D4C, grab_tab_e7d8) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x089A6DDCu);
    else {
        call2(c, o, (uint32_t)S8(0x08A9E7D8u + (uint32_t)S8(o + 194)), f_0886A6D4, 0x089A6D80u);
        if (c->r[2] != 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x14, f_0886CA2C, 0x089A6DA0u);
            call2(c, o, 4, f_088616EC, 0x089A6DACu);
            call1(c, o, f_0886DDCC, 0x089A6DB4u);
        } else { *B(o + 7) = 8; call2(c, o, 1, f_088616EC, 0x089A6DCCu); }
    }
    leave(c, sv);
}

/* (fighter): +75 bit 7 hands over to 0x0886469C; without +77 ticked; else throw 0x54 + strength: caught +7
   on, +82 = strength + 1, the meter by 0xF and move 0x20; missed ticked. */
SFA_FUNC(08916F98, grab_54) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x08916FC0u);
    else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x08916FD8u);
    else {
        call2(c, o, (uint32_t)(S8(o + 194) + 0x54), f_0886A6D4, 0x08916FECu);
        if (c->r[2] == 0) call1(c, o, f_08861848, 0x08916FFCu);
        else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *H(o + 82) = (uint16_t)(S8(o + 194) + 1);
            call2(c, o, 0xF, f_0886CA2C, 0x08917024u);
            call2(c, o, 0x20, f_088616EC, 0x08917030u);
        }
    }
    leave(c, sv);
}

/* (fighter): throw 0x62 (0x44 with no choice made): caught step 4, 0x0887C408 (+337, the value 7 or 0xB
   by strength from 0x08A749E4), 0x0887CD7C and move 0x12; missed step 0xA and ticked. */
SFA_FUNC(08945D94, grab_62) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call2(c, o, S8(o + 386) < 0 ? 0x44 : 0x62, f_0886A6D4, 0x08945DC0u);
    if (c->r[2] != 0) {
        *B(o + 7) = 4;
        uint32_t k = S8(o + 194) == 0 ? 7 : 0xB;
        ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337); ARG(2) = *H(0x08A749E4u + k + k);
        c->r[31] = 0x08945E04u; f_0887C408(c);
        call1(c, o, f_0887CD7C, 0x08945E0Cu);
        call2(c, o, 0x12, f_088616EC, 0x08945E18u);
    } else { *B(o + 7) = 0xA; call1(c, o, f_08861848, 0x08945E2Cu); }
    leave(c, sv);
}

/* (fighter): throw by strength from 0x08A58320 (0x44 in state 0x10): caught +7 by 2, +22 = +132, +235 = 1,
   0x0887CD7C in state 8, move 6; missed +7 on and move 0xE. */
SFA_FUNC(0890A340, grab_tab_8320) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t n = *H(0x08A58320u + (uint32_t)(S8(o + 194) * 2));
    if (*B(o + 6) == 0x10) n = 0x44;
    call2(c, o, n, f_0886A6D4, 0x0890A384u);
    if (c->r[2] != 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 2);
        *H(o + 22) = *H(o + 132);
        *B(o + 235) = 1;
        if (*B(o + 6) == 8) call1(c, o, f_0887CD7C, 0x0890A3BCu);
        call2(c, o, 6, f_088616EC, 0x0890A3C8u);
    } else {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        call2(c, o, 0xE, f_088616EC, 0x0890A3E4u);
    }
    leave(c, sv);
}

/* (fighter): ticked until its animation ends; then throw 0x3F (0x40 with +193): missed +7 on and move 0x30;
   caught it faces away when the opponent (0x08821330) is 0xC0 behind on the cpu side, else by the stick
   (+362: forward faces 1, back 0), step 3 and move 0. */
SFA_FUNC(0890C718, grab_3f) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0890C738u);
    else {
        call2(c, o, S8(o + 193) != 0 ? 0x40 : 0x3F, f_0886A6D4, 0x0890C75Cu);
        if (c->r[2] == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 0x30, f_088616EC, 0x0890C77Cu);
        } else {
            if (S8(o + 373) != 0) {
                call1(c, o, f_08821330, 0x0890C794u);
                *B(o + 11) = (int32_t)(S16(c->r[2] + 18) + 0xC0) < S16(o + 18) ? 1 : 0;
            } else {
                uint32_t h = *H(o + 362);
                if (h & 3) *B(o + 11) = (h & 2) ? 0 : 1;
            }
            *B(o + 7) = 3;
            call2(c, o, 0, f_088616EC, 0x0890C7F0u);
        }
    }
    leave(c, sv);
}

/* (fighter): ticked until its animation ends; then throw 0x57: caught step 3, facing 1 when 0x0892EB34 says
   so on the cpu side, else by the stick (+362: back faces 1, forward 0), and move 0; missed step 2 and
   move 0x19. */
SFA_FUNC(0892E53C, grab_57) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x0892E5E8u);
    else {
        call2(c, o, 0x57, f_0886A6D4, 0x0892E560u);
        if (c->r[2] != 0) {
            *B(o + 7) = 3;
            if (S8(o + 373) != 0) {
                call1(c, o, f_0892EB34, 0x0892E5A4u);
                if (c->r[2] != 0) *B(o + 11) = 1;
            } else {
                uint32_t h = *H(o + 362);
                if (h & 3) *B(o + 11) = (h & 2) ? 1 : 0;
            }
            call2(c, o, 0, f_088616EC, 0x0892E5C0u);
        } else { *B(o + 7) = 2; call2(c, o, 0x19, f_088616EC, 0x0892E5D8u); }
    }
    leave(c, sv);
}

/* (fighter): a timed dive: +832 counted down; at 0, or once +16 (moved by +96) meets +238's bit for the
   facing, step 7, +82 = 8 and 0x0886163C (0); otherwise throw 0x5B: caught +7 on, +832 = 0x10, +82
   cleared and move 0x33; missed ticked. */
SFA_FUNC(0896511C, grab_dive_5b) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t t = *W(o + 832) - 1;
    *W(o + 832) = t;
    int land;
    uint32_t back;
    if (t == 0) { land = 1; back = 0x08965158u; }
    else {
        *W(o + 16) += *W(o + 96);
        land = (S8(o + 238) & (1u << (*B(o + 11) & 31))) != 0; back = 0x089651A8u;
    }
    if (land) {
        *B(o + 7) = 7; *H(o + 82) = 8;
        call2(c, o, 0, f_0886163C, back);
    } else {
        call2(c, o, 0x5B, f_0886A6D4, 0x089651B8u);
        if (c->r[2] != 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *W(o + 832) = 0x10; *H(o + 82) = 0;
            call2(c, o, 0x33, f_088616EC, 0x089651E4u);
        } else call1(c, o, f_08861848, 0x089651F4u);
    }
    leave(c, sv);
}


void f_089452F8(Cpu *c);

/* (fighter): ticked until its animation ends; then throw 0x4D (0x4E with +193): missed +7 on and move 3;
   caught step 3, facing away when the opponent (0x08821330) is 0xC0 behind on the cpu side, else by the
   stick (+362: back faces 1, forward 0), and move 0 (1 with +193). */
SFA_FUNC(088F1000, grab_4d) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 75) >= 0) call1(c, o, f_08861848, 0x088F1020u);
    else {
        call2(c, o, S8(o + 193) == 0 ? 0x4D : 0x4E, f_0886A6D4, 0x088F1044u);
        if (c->r[2] == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call2(c, o, 3, f_088616EC, 0x088F1064u);
        } else {
            *B(o + 7) = 3;
            if (S8(o + 373) != 0) {
                call1(c, o, f_08821330, 0x088F1080u);
                *B(o + 11) = (int32_t)(S16(c->r[2] + 18) + 0xC0) < S16(o + 18) ? 1 : 0;
            } else {
                uint32_t h = *H(o + 362);
                if (h & 3) *B(o + 11) = (h & 2) ? 1 : 0;
            }
            if (S8(o + 193) != 0) call2(c, o, 1, f_088616EC, 0x088F10F8u);
            else call2(c, o, 0, f_088616EC, 0x088F10E8u);
        }
    }
    leave(c, sv);
}

/* (fighter): a timed dive: +825 counted down; at 0, or once +16 (moved by +96) meets +238's bit for +78,
   step 8, +82 = 8 and 0x0886163C (0); otherwise throw 0x5D: caught +7 on, +825 = 0x10 and move 0x2A;
   missed ticked. A frame keeping s0, s1 (fighter + 0x334). */
SFA_FUNC(088FA058, grab_dive_5d) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1;
    gaddr q = o + 0x334; c->r[17] = q;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    uint8_t t = (uint8_t)(*B(q + 5) - 1);
    *B(q + 5) = t;
    *W(c->r[29] + 24) = ra;
    int land; uint32_t back;
    if ((int8_t)t == 0) { land = 1; back = 0x088FA0A4u; }
    else {
        *W(o + 16) += *W(o + 96);
        land = (S8(o + 238) & (1u << ((uint32_t)S8(o + 78) & 31))) != 0; back = 0x088FA0F4u;
    }
    if (land) {
        *B(o + 7) = 8; *H(o + 82) = 8;
        call2(c, o, 0, f_0886163C, back);
    } else {
        call2(c, o, 0x5D, f_0886A6D4, 0x088FA104u);
        if (c->r[2] != 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(q + 5) = 0x10;
            call2(c, o, 0x2A, f_088616EC, 0x088FA12Cu);
        } else call1(c, o, f_08861848, 0x088FA13Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x089452F8, then while rising slower than 2.0 throw 0x58: caught step 3, the meter by 0xC,
   0x08837098, 0x0887C408 (+337, the value at 0x08A749FA), move 0xE and 0x0886DDCC; otherwise +82 counted
   down: at 0 step 9 and move strength + 0x24, else ticked. */
SFA_FUNC(08944EA8, grab_58_air) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_089452F8, 0x08944EBCu);
    int caught = 0;
    if ((int32_t)*W(o + 96) < 0x20000) {
        call2(c, o, 0x58, f_0886A6D4, 0x08944ED8u);
        caught = c->r[2] != 0;
    }
    if (caught) {
        *B(o + 7) = 3;
        call2(c, o, 0xC, f_0886CA2C, 0x08944F1Cu);
        call1(c, o, f_08837098, 0x08944F24u);
        ARG(0) = o; ARG(1) = (uint32_t)S8(o + 337); ARG(2) = *H(0x08A749E4u + 22);
        c->r[31] = 0x08944F3Cu; f_0887C408(c);
        call2(c, o, 0xE, f_088616EC, 0x08944F48u);
        call1(c, o, f_0886DDCC, 0x08944F50u);
    } else {
        int16_t k = (int16_t)(S16(o + 82) - 1);
        *H(o + 82) = (uint16_t)k;
        if (k == 0) {
            *B(o + 7) = 9;
            call2(c, o, (uint32_t)(S8(o + 194) + 0x24) & 0xFFFF, f_088616EC, 0x08944F78u);
        } else call1(c, o, f_08861848, 0x08944F04u);
    }
    leave(c, sv);
}


void f_0886AFB0(Cpu *c); void f_0886A540(Cpu *c);

/* (fighter): unless its animation is flagged ticked; then throw 0x4A (0x4B with +193): missed +7 on and
   ticked; caught it faces the opponent (0x08821330, 0xC0 behind flips it) on the cpu side, else +752 bit 0
   turns it round; then step 3 and move 0x1E, or with +193 step 7, turned round again and move 0x1D;
   0x0886DEC8. */
SFA_FUNC(089982A0, grab_4a) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x089982C0u);
    else {
        call2(c, o, S8(o + 193) == 0 ? 0x4A : 0x4B, f_0886A6D4, 0x089982E4u);
        if (c->r[2] == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            call1(c, o, f_08861848, 0x08998398u);
        } else {
            if (S8(o + 373) != 0) {
                *B(o + 11) = 0;
                call1(c, o, f_08821330, 0x08998304u);
                if (!((int32_t)(S16(c->r[2] + 18) + 0xC0) < S16(o + 18))) *B(o + 11) = 1;
            } else if (*H(o + 752) & 1) *B(o + 11) ^= 1;
            uint32_t n;
            if (S8(o + 193) == 0) { *B(o + 7) = 3; n = 0x1E; }
            else { *B(o + 7) = 7; *B(o + 11) ^= 1; n = 0x1D; }
            call2(c, o, n, f_088616EC, 0x08998378u);
            call1(c, o, f_0886DEC8, 0x08998380u);
        }
    }
    leave(c, sv);
}

/* (fighter): while +73 is set 0x08862004 waits (ticked unless +77 bit 7) and then clears it with +22 =
   +132; then with +77 set (bit 7 clear) it is cleared and throw 0x37: caught step 4, ticked and
   0x0886DEC8; missed +7 on, +73 = 1, a hop (y 5.0, brake -0.375, x by strength from 0x08A901F4 towards
   the facing) and ticked. Otherwise ticked. */
SFA_FUNC(089768D8, grab_37_hop) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 73) != 0) {
        call1(c, o, f_08862004, 0x089768F8u);
        if (c->r[2] == 0) {
            if ((S8(o + 77) & 0x80) == 0) call1(c, o, f_08861848, 0x08976918u);
            leave(c, sv); return;
        }
        *B(o + 73) = 0; *H(o + 22) = *H(o + 132);
    }
    int32_t k = S8(o + 77);
    if ((k & 0x80) || k == 0) call1(c, o, f_08861848, 0x08976948u);
    else {
        *B(o + 77) = 0;
        call2(c, o, 0x37, f_0886A6D4, 0x0897695Cu);
        if (c->r[2] != 0) {
            *B(o + 7) = 4;
            call1(c, o, f_08861848, 0x089769D8u);
            call1(c, o, f_0886DEC8, 0x089769E0u);
        } else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 73) = 1;
            *W(o + 100) = 0x50000; *W(o + 108) = 0xFFFFA000u; *W(o + 104) = 0;
            *W(o + 96) = *W(0x08A901F4u + ((uint32_t)S8(o + 194) << 2));
            if (*B(o + 11) == 0) *W(o + 96) = 0u - *W(o + 96);
            call1(c, o, f_08861848, 0x089769C4u);
        }
    }
    leave(c, sv);
}

/* (fighter, opponent, a2): can a throw land? In the air with the opponent's +816 clear, only standing
   still in state 1 / 0 / 7, then the air throw check 0x0886A540 (fighter, a2); otherwise not while the
   opponent has +543 / +702 / +573 / +270, nor in state 1 outside substates 0 / 1 / 2 / 9, nor with all
   three of its first bytes at +256 clear; then 0x0886AFB0 (fighter, opponent, a2). */
SFA_FUNC(0886AE58, throw_allowed) {
    gaddr o = ARG(0), p = ARG(1), x = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 80) != 0 && *W(p + 816) == 0) {
        if (*B(o + 4) != 1 || *B(o + 5) != 0 || *B(o + 6) != 7) c->r[2] = 0;
        else { ARG(0) = o; ARG(1) = x; ARG(2) = x; c->r[31] = 0x0886AFA4u; f_0886A540(c); }
        c->r[31] = ra; c->r[29] += 0x20; return;
    }
    int ok = 1;
    if ((S8(p + 543) | S8(p + 702) | S8(p + 573) | S8(p + 270)) != 0) ok = 0;
    else if (*B(p + 4) == 1) {
        uint32_t ss = *B(p + 5);
        if (!(ss == 0 || ss == 1 || ss == 2 || ss == 9)) ok = 0;
    }
    if (ok) {
        gaddr q = *W(p + 256);
        if (*B(q) == 0 && *B(q + 1) == 0 && *B(q + 2) == 0) ok = 0;
    }
    if (ok) { ARG(0) = o; ARG(1) = p; ARG(2) = x; c->r[7] = x; c->r[31] = 0x0886AF50u; f_0886AFB0(c); }
    else c->r[2] = 0;
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886E294(Cpu *c); void f_08945D94(Cpu *c);

/* (fighter, a1): the opponent picked (0x0886A2F4, 1); a throw only while its +73 is set, by the
   throw-allowed check (fighter, opponent, a1). */
SFA_FUNC(0886AE04, throw_allowed_73) {
    gaddr o = ARG(0), x = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = x; c->r[17] = o;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 1; c->r[31] = 0x0886AE24u; f_0886A2F4(c);
    gaddr p = *W(o + 92);
    c->r[2] = 0;
    if (S8(p + 73) != 0) { ARG(0) = o; ARG(1) = p; ARG(2) = x; c->r[31] = 0x0886AE44u; f_0886AE58(c); }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, a1): the opponent picked (0x0886A2F4, 2), then the throw-allowed check (fighter, opponent,
   a1). */
SFA_FUNC(0886B100, throw_allowed_2) {
    gaddr o = ARG(0), x = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = x; c->r[17] = o;
    *W(c->r[29] + 24) = ra;
    ARG(0) = o; ARG(1) = 2; c->r[31] = 0x0886B120u; f_0886A2F4(c);
    ARG(0) = o; ARG(1) = *W(o + 92); ARG(2) = x; c->r[31] = 0x0886B130u; f_0886AE58(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): ticked; once +76 clears, step 3, +237 cleared, 0x0886E294, move 0x27 and the throw
   0x08945D94. */
SFA_FUNC(08945D40, grab_62_start) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x08945D54u);
    if (S8(o + 76) == 0) {
        *B(o + 7) = 3; *B(o + 237) = 0;
        call1(c, o, f_0886E294, 0x08945D70u);
        call2(c, o, 0x27, f_088616EC, 0x08945D7Cu);
        call1(c, o, f_08945D94, 0x08945D84u);
    }
    leave(c, sv);
}


void f_088D35D0(Cpu *c); void f_088D2DBC(Cpu *c); void f_088D36D0(Cpu *c); void f_088D3960(Cpu *c);
void f_0891B994(Cpu *c); void f_0891DD70(Cpu *c); void f_0893F1D0(Cpu *c); void f_08999370(Cpu *c);
void f_089958E4(Cpu *c); void f_08867C30(Cpu *c);

/* (fighter): 0x0891B994 and 0x0891DD70; falling: step 7, stopped and move 0xB; rising with +77 clear,
   throw 0x37 on a +73 opponent: caught +7 on, +828 = 0xFF, +82 = 4, the meter by 7 (unless +540) and
   move 9; otherwise ticked. */
SFA_FUNC(08919A2C, grab_37_air) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_0891B994, 0x08919A40u);
    call1(c, o, f_0891DD70, 0x08919A48u);
    int32_t v = (int32_t)*W(o + 100);
    if (v < 0) {
        *B(o + 7) = 7; *W(o + 104) = 0; *W(o + 96) = 0;
        call2(c, o, 0xB, f_088616EC, 0x08919AE4u);
    } else {
        int caught = 0;
        if (v != 0 && S8(o + 77) == 0) {
            call2(c, o, 0x37, f_0886AE04, 0x08919A70u);
            caught = c->r[2] != 0;
        }
        if (!caught) call1(c, o, f_08861848, 0x08919A80u);
        else {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 0x334 + 8) = 0xFF; *H(o + 82) = 4;
            if (S8(o + 540) == 0) call2(c, o, 7, f_0886CA2C, 0x08919AB8u);
            call2(c, o, 9, f_088616EC, 0x08919AC4u);
        }
    }
    leave(c, sv);
}

/* (fighter): ticked; falling: +7 on, a bounce (x -1.605, y -0.1875, brakes 0.0195 / -0.1875) and move
   0x1F; else 0x0893F1D0 and with +77 bit 7 throw 5 on a +73 opponent: caught step 7, 0x0887CD7C, move
   0x20 and 0x0886DEC8. */
SFA_FUNC(0893F120, grab_5_air) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893F134u);
    if ((int32_t)*W(o + 100) < 0) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *W(o + 96) = 0xFFFE6500u; *W(o + 100) = 0xFFFFD000u;
        *W(o + 104) = 0x500; *W(o + 108) = 0xFFFFD000u;
        call2(c, o, 0x1F, f_088616EC, 0x0893F1C0u);
    } else {
        call1(c, o, f_0893F1D0, 0x0893F148u);
        if (S8(o + 77) < 0) {
            call2(c, o, 5, f_0886AE04, 0x0893F15Cu);
            if (c->r[2] != 0) {
                *B(o + 7) = 7;
                call1(c, o, f_0887CD7C, 0x0893F170u);
                call2(c, o, 0x20, f_088616EC, 0x0893F17Cu);
                call1(c, o, f_0886DEC8, 0x0893F184u);
            }
        }
    }
    leave(c, sv);
}

/* (fighter): landing (0x08999370) once +132 reaches +22: step 6, +20 = +132 (16.16), +73 cleared, the
   meter by 3 and move 0x1A. Otherwise while 0x089958E4 holds, the throw 0x4E (by stick and buttons for a
   player, 0x08867C30 0xE unless +760 bits 7..9; no stick on +752 or no catch gives step 7, the meter by 4
   and move 0x1B): caught step 5 and move 0x19 on an airborne opponent or one in range by +820, step 4
   and move 0x18 on a crouching one or beyond; else ticked. A frame keeping s0, s1 (the opponent). */
SFA_FUNC(0899575C, grab_4e) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    gaddr p = *W(o + 92); c->r[17] = p;
    call1(c, o, f_08999370, 0x08995778u);
    int32_t a = S16(o + 132);
    if (c->r[2] != 0 && !(a < S16(o + 22))) {
        *B(o + 7) = 6; *W(o + 20) = (uint32_t)a << 16; *B(o + 73) = 0;
        call2(c, o, 3, f_0886CA2C, 0x089957B0u);
        call2(c, o, 0x1A, f_088616EC, 0x089957BCu);
    } else {
        call1(c, o, f_089958E4, 0x089957CCu);
        int r = 0;  /* 0 ticked, 1 caught, 2 missed */
        if (c->r[2] != 0) {
            if (S8(o + 373) != 0) {
                call2(c, o, 0x4E, f_0886B100, 0x08995838u);
                r = c->r[2] != 0 ? 1 : 0;
            } else {
                int go = 1;
                if ((*H(o + 760) & 0x380) == 0) {
                    call2(c, o, 0xE, f_08867C30, 0x089957F8u);
                    go = c->r[2] != 0;
                }
                if (go) {
                    if ((*H(o + 752) & 3) == 0) r = 2;
                    else {
                        call2(c, o, 0x4E, f_0886B100, 0x0899581Cu);
                        r = c->r[2] != 0 ? 1 : 2;
                    }
                }
            }
        }
        if (r == 0) call1(c, o, f_08861848, 0x08995848u);
        else if (r == 2) {
            *B(o + 7) = 7;
            call2(c, o, 4, f_0886CA2C, 0x089958C4u);
            call2(c, o, 0x1B, f_088616EC, 0x089958D0u);
        } else {
            int near;
            if (S8(p + 73) != 0) near = 1;
            else if (S8(p + 696) != 0) near = 0;
            else near = (int32_t)(S16(o + 22) - S16(p + 22)) < (int32_t)*W(o + 820);
            if (near) { *B(o + 7) = 5; call2(c, o, 0x19, f_088616EC, 0x089958ACu); }
            else { *B(o + 7) = 4; call2(c, o, 0x18, f_088616EC, 0x08995894u); }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x088D3EC0; finished (+75 bit 7, not +73) hands over to 0x0886469C. With +73 and +77,
   0x08862004 and 0x088D35D0: a 1 clears +73, then 0x088D2DBC, 0x088D36D0, 0x088D3960 (0, 0) and
   0x0886469C. Else on +76 (cleared): on the ground throw 0x4A + strength: caught +7 on, move 0x1F,
   0x0887CD7C and 0x0886DEC8; in the air it steps 8 / 0xC / 4 by strength along its facing and tries the
   air throw 0x30 + strength: caught +7 on, move 0x27, 0x0887CD7C and 0x0886DDCC. A frame keeping s0, s1. */
SFA_FUNC(089BBE34, grab_4a_air) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    call1(c, o, f_088D3EC0, 0x089BBE4Cu);
    c->r[5] = (uint32_t)S8(o + 75);
    int32_t k73 = S8(o + 73);
    if ((int32_t)c->r[5] < 0 && k73 == 0) {
        call1(c, o, f_0886469C, 0x089BBE68u);
        c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20; return;
    }
    if (k73 != 0 && S8(o + 77) != 0) {
        call1(c, o, f_08862004, 0x089BBE98u);
        call1(c, o, f_088D35D0, 0x089BBEA0u);
        if (c->r[2] == 1) {
            *B(o + 73) = 0;
            call1(c, o, f_088D2DBC, 0x089BBECCu);
            call1(c, o, f_088D36D0, 0x089BBED4u);
            ARG(0) = o; ARG(1) = 0; ARG(2) = 0; c->r[31] = 0x089BBEE4u; f_088D3960(c);
            call1(c, o, f_0886469C, 0x089BBEECu);
            c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20; return;
        }
    }
    if (S8(o + 76) != 0) {
        int32_t air = S8(o + 73);
        *B(o + 76) = 0; c->r[17] = 1;
        int32_t s = S8(o + 194);
        if (air == 0) {
            call2(c, o, (uint32_t)(s + 0x4A), f_0886A6D4, 0x089BBF1Cu);
            if (c->r[2] == 1) {
                *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
                call2(c, o, 0x1F, f_088616EC, 0x089BBF3Cu);
                call1(c, o, f_0887CD7C, 0x089BBF44u);
                call1(c, o, f_0886DEC8, 0x089BBF4Cu);
            }
        } else {
            uint32_t x = (uint32_t)S16(o + 18), d;
            if (s > 0) d = s < 2 ? 8 : (s < 3 ? 0xC : 0);
            else if (s == 0) d = 4;
            else d = c->r[5];
            *H(o + 18) = (uint16_t)(*B(o + 11) == 0 ? x - d : x + d);
            call2(c, o, (uint32_t)(s + 0x30), f_0886AE04, 0x089BBFB8u);
            if (c->r[2] == 1) {
                *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
                call2(c, o, 0x27, f_088616EC, 0x089BBFD8u);
                call1(c, o, f_0887CD7C, 0x089BBFE0u);
                call1(c, o, f_0886DDCC, 0x089BBFE8u);
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
