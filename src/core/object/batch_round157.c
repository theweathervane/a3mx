/* More end-of-move steps, in eight shapes that repeat across the characters: when the animation has
   ended the fighter stands again (0x0886469C), some after 0x0886A530 or a flag; otherwise 0x0886DB2C
   (some) and the animation ticks. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_0886A530(Cpu *c); void f_0886DB2C(Cpu *c);
void f_088D3EC0(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static int ended(gaddr o) { return S8(o + 75) < 0; }

/* Ended: 0x0886A530 and standing; else ticked. */
static void end_a530(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { call1(c, o, f_0886A530, b1); call1(c, o, f_0886469C, b2); }
    else call1(c, o, f_08861848, b3);
    leave(c, sv);
}
/* Ended: 0x0886A530 and standing; else 0x0886DB2C and ticked. */
static void end_a530_db2c(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) { call1(c, o, f_0886A530, b1); call1(c, o, f_0886469C, b2); }
    else { call1(c, o, f_0886DB2C, b3); call1(c, o, f_08861848, b4); }
    leave(c, sv);
}
/* Ended: standing; else 0x0886DB2C and the part ticked. */
static void end_db2c_part(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, b1);
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_088D3EC0, b3); }
    leave(c, sv);
}
/* Ended (bit 7): standing; else ticked (only the return address kept). */
static void end80_round(Cpu *c, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 75) & 0x80) { c->r[31] = b1; f_0886469C(c); }
    else { c->r[31] = b2; f_08861848(c); }
    c->r[31] = ra;
    c->r[29] += 0x20;
}
/* Ended: standing; else 0x0886DB2C and ticked. */
static void end_db2c(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, b1);
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
/* The same with the test the other way round. */
static void db2c_or_end(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (!ended(o)) { call1(c, o, f_0886DB2C, b1); call1(c, o, f_08861848, b2); }
    else call1(c, o, f_0886469C, b3);
    leave(c, sv);
}
/* Ended (bit 7): the opponent's (+92) +543 = 5 and standing; else 0x0886DB2C and ticked. */
static void end80_543(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 75) & 0x80) { *B(*W(o + 92) + 543) = 5; call1(c, o, f_0886469C, b1); }
    else { call1(c, o, f_0886DB2C, b2); call1(c, o, f_08861848, b3); }
    leave(c, sv);
}
/* Ended: standing. Else the +82 timer (when set) runs down, clearing +262 as it runs out; 0x0886DB2C when
   it is out (or was not set), and ticked. */
static void timer_db2c(Cpu *c, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4) {
    gaddr o = ARG(0); uint32_t sv[2];
    enter(c, o, sv);
    if (ended(o)) call1(c, o, f_0886469C, b4);
    else {
        int32_t t = S16(o + 82);
        if (t == 0) call1(c, o, f_0886DB2C, b2);
        else {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) { *B(o + 262) = 0; call1(c, o, f_0886DB2C, b1); }
        }
        call1(c, o, f_08861848, b3);
    }
    leave(c, sv);
}
SFA_FUNC(088EE84C, move_end_a530_round_e84c) { end_a530(c, 0x088EE86Cu, 0x088EE874u, 0x088EE884u); }
SFA_FUNC(088EFBAC, move_end_a530_round_fbac) { end_a530(c, 0x088EFBCCu, 0x088EFBD4u, 0x088EFBE4u); }
SFA_FUNC(088F0520, move_end_a530_round_0520) { end_a530(c, 0x088F0540u, 0x088F0548u, 0x088F0558u); }
SFA_FUNC(088F3BF4, move_end_a530_round_3bf4) { end_a530(c, 0x088F3C14u, 0x088F3C1Cu, 0x088F3C2Cu); }
SFA_FUNC(088F4FD4, move_end_a530_round_4fd4) { end_a530(c, 0x088F4FF4u, 0x088F4FFCu, 0x088F500Cu); }
SFA_FUNC(088F87E8, move_end_a530_round_87e8) { end_a530(c, 0x088F8808u, 0x088F8810u, 0x088F8820u); }
SFA_FUNC(088FE628, move_end_a530_round_e628) { end_a530(c, 0x088FE648u, 0x088FE650u, 0x088FE660u); }
SFA_FUNC(08902BDC, move_end_a530_round_2bdc) { end_a530(c, 0x08902BFCu, 0x08902C04u, 0x08902C14u); }
SFA_FUNC(08903E50, move_end_a530_round_3e50) { end_a530(c, 0x08903E70u, 0x08903E78u, 0x08903E88u); }
SFA_FUNC(08904200, move_end_a530_round_4200) { end_a530(c, 0x08904220u, 0x08904228u, 0x08904238u); }
SFA_FUNC(08907A4C, move_end_a530_round_7a4c) { end_a530(c, 0x08907A6Cu, 0x08907A74u, 0x08907A84u); }
SFA_FUNC(08963EEC, move_end_a530_round_3eec) { end_a530(c, 0x08963F0Cu, 0x08963F14u, 0x08963F24u); }
SFA_FUNC(08965790, move_end_a530_round_5790) { end_a530(c, 0x089657B0u, 0x089657B8u, 0x089657C8u); }
SFA_FUNC(0899C184, move_end_a530_round_c184) { end_a530(c, 0x0899C1A4u, 0x0899C1ACu, 0x0899C1BCu); }
SFA_FUNC(088EF724, move_end_a530_or_db2c_f724) { end_a530_db2c(c, 0x088EF744u, 0x088EF74Cu, 0x088EF75Cu, 0x088EF764u); }
SFA_FUNC(088F364C, move_end_a530_or_db2c_364c) { end_a530_db2c(c, 0x088F366Cu, 0x088F3674u, 0x088F3684u, 0x088F368Cu); }
SFA_FUNC(088F8274, move_end_a530_or_db2c_8274) { end_a530_db2c(c, 0x088F8294u, 0x088F829Cu, 0x088F82ACu, 0x088F82B4u); }
SFA_FUNC(089029DC, move_end_a530_or_db2c_29dc) { end_a530_db2c(c, 0x089029FCu, 0x08902A04u, 0x08902A14u, 0x08902A1Cu); }
SFA_FUNC(08903904, move_end_a530_or_db2c_3904) { end_a530_db2c(c, 0x08903924u, 0x0890392Cu, 0x0890393Cu, 0x08903944u); }
SFA_FUNC(08906ADC, move_end_a530_or_db2c_6adc) { end_a530_db2c(c, 0x08906AFCu, 0x08906B04u, 0x08906B14u, 0x08906B1Cu); }
SFA_FUNC(08906CF4, move_end_a530_or_db2c_6cf4) { end_a530_db2c(c, 0x08906D14u, 0x08906D1Cu, 0x08906D2Cu, 0x08906D34u); }
SFA_FUNC(089638AC, move_end_a530_or_db2c_38ac) { end_a530_db2c(c, 0x089638CCu, 0x089638D4u, 0x089638E4u, 0x089638ECu); }
SFA_FUNC(089B5AAC, part_end_or_db2c_5aac) { end_db2c_part(c, 0x089B5ACCu, 0x089B5ADCu, 0x089B5AE4u); }
SFA_FUNC(089B5FC4, part_end_or_db2c_5fc4) { end_db2c_part(c, 0x089B5FE4u, 0x089B5FF4u, 0x089B5FFCu); }
SFA_FUNC(089B6124, part_end_or_db2c_6124) { end_db2c_part(c, 0x089B6144u, 0x089B6154u, 0x089B615Cu); }
SFA_FUNC(089BB9DC, part_end_or_db2c_b9dc) { end_db2c_part(c, 0x089BB9FCu, 0x089BBA0Cu, 0x089BBA14u); }
SFA_FUNC(089C1664, part_end_or_db2c_1664) { end_db2c_part(c, 0x089C1684u, 0x089C1694u, 0x089C169Cu); }
SFA_FUNC(089C8250, part_end_or_db2c_8250) { end_db2c_part(c, 0x089C8270u, 0x089C8280u, 0x089C8288u); }
SFA_FUNC(089C94FC, part_end_or_db2c_94fc) { end_db2c_part(c, 0x089C951Cu, 0x089C952Cu, 0x089C9534u); }
SFA_FUNC(0890E3C0, move_end80_round_e3c0) { end80_round(c, 0x0890E3E0u, 0x0890E3F4u); }
SFA_FUNC(089125B8, move_end80_round_25b8) { end80_round(c, 0x089125D8u, 0x089125ECu); }
SFA_FUNC(08916358, move_end80_round_6358) { end80_round(c, 0x08916378u, 0x0891638Cu); }
SFA_FUNC(08979D00, move_end80_round_9d00) { end80_round(c, 0x08979D20u, 0x08979D34u); }
SFA_FUNC(0898B380, move_end80_round_b380) { end80_round(c, 0x0898B3A0u, 0x0898B3B4u); }
SFA_FUNC(0899014C, move_end80_round_014c) { end80_round(c, 0x0899016Cu, 0x08990180u); }
SFA_FUNC(0894410C, move_end_or_db2c_410c) { end_db2c(c, 0x0894412Cu, 0x0894413Cu, 0x08944144u); }
SFA_FUNC(08944394, move_end_or_db2c_4394) { end_db2c(c, 0x089443B4u, 0x089443C4u, 0x089443CCu); }
SFA_FUNC(089689D4, move_end_or_db2c_89d4) { end_db2c(c, 0x089689F4u, 0x08968A04u, 0x08968A0Cu); }
SFA_FUNC(0899BD20, move_end_or_db2c_bd20) { end_db2c(c, 0x0899BD40u, 0x0899BD50u, 0x0899BD58u); }
SFA_FUNC(089A415C, move_end_or_db2c_415c) { end_db2c(c, 0x089A417Cu, 0x089A418Cu, 0x089A4194u); }
SFA_FUNC(089A4348, move_end_or_db2c_4348) { end_db2c(c, 0x089A4368u, 0x089A4378u, 0x089A4380u); }
SFA_FUNC(08921224, move_db2c_or_end_1224) { db2c_or_end(c, 0x08921244u, 0x0892124Cu, 0x0892125Cu); }
SFA_FUNC(08935EBC, move_db2c_or_end_5ebc) { db2c_or_end(c, 0x08935EDCu, 0x08935EE4u, 0x08935EF4u); }
SFA_FUNC(0893E500, move_db2c_or_end_e500) { db2c_or_end(c, 0x0893E520u, 0x0893E528u, 0x0893E538u); }
SFA_FUNC(0898F918, move_end80_543_f918) { end80_543(c, 0x0898F948u, 0x0898F958u, 0x0898F960u); }
SFA_FUNC(08990A44, move_end80_543_0a44) { end80_543(c, 0x08990A74u, 0x08990A84u, 0x08990A8Cu); }
SFA_FUNC(08995070, move_end80_543_5070) { end80_543(c, 0x089950A0u, 0x089950B0u, 0x089950B8u); }
SFA_FUNC(088F3960, move_timer_db2c_3960) { timer_db2c(c, 0x088F39A4u, 0x088F39B4u, 0x088F39BCu, 0x088F39CCu); }
SFA_FUNC(088F8538, move_timer_db2c_8538) { timer_db2c(c, 0x088F857Cu, 0x088F858Cu, 0x088F8594u, 0x088F85A4u); }
SFA_FUNC(088F9E00, move_timer_db2c_9e00) { timer_db2c(c, 0x088F9E44u, 0x088F9E54u, 0x088F9E5Cu, 0x088F9E6Cu); }
