/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, 7 - 8
   instructions: one-call wrappers (a frame, the return address kept, one call), places moved by their
   speeds, and fields set from tables. Offsets stay numbers until something shows what they are.
 */
#include "../decomp.h"
#include "../guest_mem.h"

#define BATTLE  0x092E1228u
#define SESSION 0x09523820u

void f_0881F6A0(Cpu *c); void f_08820B7C(Cpu *c); void f_08831B34(Cpu *c); void f_08821424(Cpu *c);
void f_088219B0(Cpu *c); void f_0882319C(Cpu *c); void f_0886DC3C(Cpu *c); void f_08863168(Cpu *c);
void f_0887A10C(Cpu *c); void f_088231C8(Cpu *c); void f_08880B84(Cpu *c); void f_088215C4(Cpu *c);
void f_08823278(Cpu *c); void f_088233BC(Cpu *c); void f_088AA398(Cpu *c); void f_088AD470(Cpu *c);
void f_088AD4DC(Cpu *c); void f_088AE240(Cpu *c); void f_08823220(Cpu *c); void f_088BCB9C(Cpu *c);
void f_0886A2D8(Cpu *c); void f_08876998(Cpu *c); void f_088A0664(Cpu *c); void f_088764A8(Cpu *c);
void f_088230F4(Cpu *c); void f_089CEF04(Cpu *c); void f_089E0054(Cpu *c); void f_089E00CC(Cpu *c);
void f_089E0144(Cpu *c); void f_089F33C0(Cpu *c); void f_089F3430(Cpu *c); void f_08871A84(Cpu *c);
void f_088A055C(Cpu *c); void f_08822600(Cpu *c);

/* One call inside a frame of `frame` bytes with the return address kept at `slot` (+16 in a 0x20
   frame, +0 in a 0x10 one), as the original wrappers do; the callee gets every argument register as
   it came in, and `back` as its return address. */
static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WRAP20(back, fn) wrap(c, 0x20, 16, back, fn)
#define WRAP10(back, fn) wrap(c, 0x10, 0, back, fn)

/* ---------------------------------------------------------------- wrappers: the same call, another address */
/* 0x088231C8 (an object's routine run, then the next) */
SFA_FUNC(0887EB88, call_088231C8_a) { WRAP20(0x0887EB98u, f_088231C8); }
SFA_FUNC(088988E0, call_088231C8_b) { WRAP20(0x088988F0u, f_088231C8); }
SFA_FUNC(0889CDAC, call_088231C8_c) { WRAP20(0x0889CDBCu, f_088231C8); }
SFA_FUNC(0889DB2C, call_088231C8_d) { WRAP20(0x0889DB3Cu, f_088231C8); }
SFA_FUNC(088AD2A0, call_088231C8_e) { WRAP20(0x088AD2B0u, f_088231C8); }
SFA_FUNC(088ADCB4, call_088231C8_f) { WRAP20(0x088ADCC4u, f_088231C8); }
SFA_FUNC(088AE2B0, call_088231C8_g) { WRAP20(0x088AE2C0u, f_088231C8); }
SFA_FUNC(088AEA6C, call_088231C8_h) { WRAP20(0x088AEA7Cu, f_088231C8); }
SFA_FUNC(088AED24, call_088231C8_i) { WRAP20(0x088AED34u, f_088231C8); }
SFA_FUNC(088B0210, call_088231C8_j) { WRAP20(0x088B0220u, f_088231C8); }
SFA_FUNC(088B53BC, call_088231C8_k) { WRAP20(0x088B53CCu, f_088231C8); }
SFA_FUNC(088EC058, call_088231C8_l) { WRAP20(0x088EC068u, f_088231C8); }
SFA_FUNC(088F1DA0, call_088231C8_m) { WRAP20(0x088F1DB0u, f_088231C8); }
SFA_FUNC(08908F4C, call_088231C8_n) { WRAP20(0x08908F5Cu, f_088231C8); }
SFA_FUNC(089133B0, call_088231C8_o) { WRAP20(0x089133C0u, f_088231C8); }
SFA_FUNC(08917CB4, call_088231C8_p) { WRAP20(0x08917CC4u, f_088231C8); }
SFA_FUNC(08961A50, call_088231C8_q) { WRAP20(0x08961A60u, f_088231C8); }
SFA_FUNC(0896A518, call_088231C8_r) { WRAP20(0x0896A528u, f_088231C8); }
SFA_FUNC(08979D64, call_088231C8_s) { WRAP20(0x08979D74u, f_088231C8); }
SFA_FUNC(08981AAC, call_088231C8_t) { WRAP20(0x08981ABCu, f_088231C8); }
SFA_FUNC(08981AC8, call_088231C8_u) { WRAP20(0x08981AD8u, f_088231C8); }
SFA_FUNC(089A2530, call_088231C8_v) { WRAP20(0x089A2540u, f_088231C8); }
SFA_FUNC(089A9414, call_088231C8_w) { WRAP20(0x089A9424u, f_088231C8); }
SFA_FUNC(089CD280, call_088231C8_x) { WRAP20(0x089CD290u, f_088231C8); }
SFA_FUNC(089D77EC, call_088231C8_y) { WRAP20(0x089D77FCu, f_088231C8); }
SFA_FUNC(089D99CC, call_088231C8_z) { WRAP20(0x089D99DCu, f_088231C8); }
/* 0x088233BC */
SFA_FUNC(08887ECC, call_088233BC_a) { WRAP20(0x08887EDCu, f_088233BC); }
SFA_FUNC(08888EE4, call_088233BC_b) { WRAP20(0x08888EF4u, f_088233BC); }
SFA_FUNC(08889574, call_088233BC_c) { WRAP20(0x08889584u, f_088233BC); }
SFA_FUNC(088B2A40, call_088233BC_d) { WRAP20(0x088B2A50u, f_088233BC); }
SFA_FUNC(088B5B94, call_088233BC_e) { WRAP20(0x088B5BA4u, f_088233BC); }
SFA_FUNC(088EB8DC, call_088233BC_f) { WRAP20(0x088EB8ECu, f_088233BC); }
/* 0x088219B0 (animate) */
SFA_FUNC(08858460, animate_a) { WRAP20(0x08858470u, f_088219B0); }
SFA_FUNC(08858B34, animate_b) { WRAP20(0x08858B44u, f_088219B0); }
SFA_FUNC(08858D38, animate_c) { WRAP20(0x08858D48u, f_088219B0); }
SFA_FUNC(088B1488, animate_d) { WRAP20(0x088B1498u, f_088219B0); }
SFA_FUNC(0893ACE0, animate_e) { WRAP20(0x0893ACF0u, f_088219B0); }
/* 0x088215C4 (object_draw) */
SFA_FUNC(0888143C, draw_a) { WRAP20(0x0888144Cu, f_088215C4); }
SFA_FUNC(088816B8, draw_b) { WRAP20(0x088816C8u, f_088215C4); }
/* 0x08821424 */
SFA_FUNC(08853E94, call_08821424_a) { WRAP20(0x08853EA4u, f_08821424); }
SFA_FUNC(089DB8E8, call_08821424_b) { WRAP20(0x089DB8F8u, f_08821424); }
/* 0x08823220 */
SFA_FUNC(088B1154, call_08823220_a) { WRAP20(0x088B1164u, f_08823220); }
SFA_FUNC(088B1DA8, call_08823220_b) { WRAP20(0x088B1DB8u, f_08823220); }
/* 0x088764A8 */
SFA_FUNC(088F150C, call_088764A8_a) { WRAP20(0x088F151Cu, f_088764A8); }
SFA_FUNC(08965D8C, call_088764A8_b) { WRAP20(0x08965D9Cu, f_088764A8); }
SFA_FUNC(089A54C4, call_088764A8_c) { WRAP20(0x089A54D4u, f_088764A8); }
/* 0x088AE240 */
SFA_FUNC(088ADE9C, call_088AE240_a) { WRAP20(0x088ADEACu, f_088AE240); }
SFA_FUNC(088ADF60, call_088AE240_b) { WRAP20(0x088ADF70u, f_088AE240); }
/* one each */
SFA_FUNC(088250D4, call_08831B34) { WRAP20(0x088250E4u, f_08831B34); }
SFA_FUNC(08858920, call_0882319C) { WRAP20(0x08858930u, f_0882319C); }
SFA_FUNC(088870BC, call_08823278) { WRAP20(0x088870CCu, f_08823278); }
SFA_FUNC(088AA37C, call_088AA398) { WRAP20(0x088AA38Cu, f_088AA398); }
SFA_FUNC(088BCAE0, call_088BCB9C) { WRAP20(0x088BCAF0u, f_088BCB9C); }
SFA_FUNC(088D30D8, call_0886A2D8) { WRAP20(0x088D30E8u, f_0886A2D8); }
SFA_FUNC(088D3E9C, call_08876998) { WRAP20(0x088D3EACu, f_08876998); }
SFA_FUNC(08947EC0, call_088230F4) { WRAP20(0x08947ED0u, f_088230F4); }
SFA_FUNC(089E0540, call_089F33C0) { WRAP20(0x089E0550u, f_089F33C0); }
SFA_FUNC(089E0DC4, call_089F3430) { WRAP20(0x089E0DD4u, f_089F3430); }
SFA_FUNC(0881F684, call_0881F6A0) { WRAP10(0x0881F694u, f_0881F6A0); }
SFA_FUNC(088D4354, call_088A0664) { WRAP10(0x088D4364u, f_088A0664); }
SFA_FUNC(089E0038, call_089E0054) { WRAP10(0x089E0048u, f_089E0054); }
SFA_FUNC(089E00B0, call_089E00CC) { WRAP10(0x089E00C0u, f_089E00CC); }
SFA_FUNC(089E0128, call_089E0144) { WRAP10(0x089E0138u, f_089E0144); }

/* ---------------------------------------------------------------- wrappers with something set first */
/* (n): 0x08820B7C with n's low half-word. */
SFA_FUNC(0882134C, call_08820B7C_u16) { ARG(0) &= 0xFFFF; WRAP20(0x0882135Cu, f_08820B7C); }
/* (fighter): +123 cleared, then 0x0886DC3C. */
SFA_FUNC(08863114, clear_123_then_0886DC3C) { *B(ARG(0) + 123) = 0; WRAP20(0x08863124u, f_0886DC3C); }
/* (x): 0x08863168 (x, 0) / (x, 1). */
SFA_FUNC(08863130, call_08863168_0) { ARG(1) = 0; WRAP20(0x08863140u, f_08863168); }
SFA_FUNC(0886314C, call_08863168_1) { ARG(1) = 1; WRAP20(0x0886315Cu, f_08863168); }
/* (fighter): +550 cleared, then 0x0887A10C. */
SFA_FUNC(0887A078, clear_550_then_0887A10C) { *B(ARG(0) + 550) = 0; WRAP20(0x0887A088u, f_0887A10C); }
/* (x): 0x08880B84 (x, 0) / (x, 2). */
SFA_FUNC(08880C38, call_08880B84_0) { ARG(1) = 0; WRAP20(0x08880C48u, f_08880B84); }
SFA_FUNC(08880C54, call_08880B84_2) { ARG(1) = 2; WRAP20(0x08880C64u, f_08880B84); }
/* (x): 0x088AD470 / 0x088AD4DC with (x, 0). */
SFA_FUNC(088AD30C, call_088AD470_0) { ARG(1) = 0; WRAP20(0x088AD31Cu, f_088AD470); }
SFA_FUNC(088AD39C, call_088AD4DC_0) { ARG(1) = 0; WRAP20(0x088AD3ACu, f_088AD4DC); }
/* (object): +93 cleared, then 0x088233BC. */
SFA_FUNC(088BDB9C, clear_93_then_088233BC) { *B(ARG(0) + 93) = 0; WRAP20(0x088BDBACu, f_088233BC); }
/* (object): 0x089CEF04 (object, its +116). */
SFA_FUNC(089CD330, call_089CEF04_116) { ARG(1) = *B(ARG(0) + 116); WRAP20(0x089CD340u, f_089CEF04); }
/* (dst, src, n): 0x089F3430 (the game's memcpy) with n half-words. */
SFA_FUNC(089E11C8, copy_halfwords) { ARG(2) += ARG(2); WRAP20(0x089E11D8u, f_089F3430); }
/* (fighter): +117 = 9 / 10 / 11, then 0x08871A84 (fighter, -, the same number). */
static void set_117_then_08871A84(Cpu *c, uint32_t v, uint32_t back) {
    ARG(2) = v;
    *B(ARG(0) + 117) = (uint8_t)v;
    WRAP20(back, f_08871A84);
}
SFA_FUNC(088717E8, set_117_9) { set_117_then_08871A84(c, 9, 0x088717FCu); }
SFA_FUNC(08871808, set_117_10) { set_117_then_08871A84(c, 0xA, 0x0887181Cu); }
SFA_FUNC(08871828, set_117_11) { set_117_then_08871A84(c, 0xB, 0x0887183Cu); }
/* (object, v): +36 = v, then 0x088A055C (object, v's low half-word). */
SFA_FUNC(088A053C, set_36_then_088A055C) {
    *H(ARG(0) + 36) = (uint16_t)ARG(1);
    ARG(1) &= 0xFFFF;
    WRAP20(0x088A0550u, f_088A055C);
}
/* (record): +49 and +20 cleared, then 0x08822600. */
SFA_FUNC(088E30DC, clear_49_20_then_08822600) {
    gaddr r = ARG(0);
    *B(r + 49) = 0; *W(r + 20) = 0;
    WRAP10(0x088E30F0u, f_08822600);
}

/* ---------------------------------------------------------------- places moved by their speeds */
/* (object): x (+16) moved by its speed (+96), which changes by +104. */
static void move_x(gaddr o) {
    uint32_t speed = *W(o + 96), accel = *W(o + 104);
    *W(o + 16) += speed;
    *W(o + 96) = speed + accel;
}
/* (object): y (+20) moved by its speed (+100), which changes by +108; the new speed back. */
static uint32_t move_y(gaddr o) {
    uint32_t speed = *W(o + 100), accel = *W(o + 108);
    *W(o + 20) += speed;
    *W(o + 100) = speed + accel;
    return speed + accel;
}
SFA_FUNC(088D31AC, move_x_a) { move_x(ARG(0)); }
SFA_FUNC(08999408, move_x_b) { move_x(ARG(0)); }
SFA_FUNC(088D31CC, move_y_a) { move_y(ARG(0)); }
SFA_FUNC(088EF7F8, move_y_b) { move_y(ARG(0)); }
SFA_FUNC(0890CC44, move_y_c) { move_y(ARG(0)); }
SFA_FUNC(08913018, move_y_d) { move_y(ARG(0)); }
SFA_FUNC(0892B168, move_y_e) { move_y(ARG(0)); }
SFA_FUNC(08963984, move_y_f) { move_y(ARG(0)); }
SFA_FUNC(0896F658, move_y_g) { move_y(ARG(0)); }
SFA_FUNC(089B29F8, move_y_h) { move_y(ARG(0)); }
/* these copies leave the new speed in v0 */
SFA_FUNC(08908C04, move_y_answer_a) { RETURN(move_y(ARG(0))); }
SFA_FUNC(08961428, move_y_answer_b) { RETURN(move_y(ARG(0))); }
SFA_FUNC(0899CF4C, move_y_answer_c) { RETURN(move_y(ARG(0))); }

/* ---------------------------------------------------------------- fields */
/* (record): +108 = 1, +158 / +160 = +68 / +70. */
SFA_FUNC(089DFD40, copy_68_70_to_158) {
    gaddr r = ARG(0);
    int16_t a = S16(r + 68), b = S16(r + 70);
    *B(r + 108) = 1;
    *H(r + 158) = (uint16_t)a;
    *H(r + 160) = (uint16_t)b;
}
/* The word at 0x08B014D8 = the pad words at 0x08E75BAC and 0x08E75BD8 together. */
SFA_FUNC(08818CB8, pads_together) { *W(0x08B014D8u) = *W(0x08E75BA0u + 12) | *W(0x08E75BA0u + 56); }
/* (object): +5 on, speed +100 = 0x140000, +108 = -1.0. */
SFA_FUNC(08854E58, step_on_5_rise) {
    gaddr o = ARG(0);
    *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    *W(o + 100) = 0x140000u;
    *W(o + 108) = 0xFFFF0000u;
}
/* (object): +4 on, BATTLE +44 (the intro's stage) = 6. */
SFA_FUNC(08855248, step_on_4_intro_6) {
    *B(ARG(0) + 4) = (uint8_t)(*B(ARG(0) + 4) + 1);
    *B(BATTLE + 44) = 6;
}
/* (fighter): +506 counted down; 1 once it is 0 (then left at 0), else 0. */
SFA_FUNC(08876B5C, count_down_506) {
    gaddr f = ARG(0);
    int16_t v = S16(f + 506);
    if (v == 0) { RETURN(1); return; }
    *H(f + 506) = (uint16_t)(v - 1);
    RETURN(0);
}
/* (object, n): +79 = n, +22 = the s16 at 0x08A2AB84 + 2 n. */
SFA_FUNC(0889CD8C, set_79_y_from_table) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    *B(o + 79) = (uint8_t)n;
    *H(o + 22) = (uint16_t)S16(0x08A2AB84u + n * 2);
}
/* (object): +96 = the word at 0x08A2B35C by its +3. */
SFA_FUNC(088A0594, speed_by_3) { gaddr o = ARG(0); *W(o + 96) = *W(0x08A2B35Cu + (uint32_t)*B(o + 3) * 4); }
/* (object): the word at 0x08B027E0 counted down by 2, its +84 on by 1. */
SFA_FUNC(088A9A20, count_08B027E0_and_84) {
    *W(0x08B027E0u) -= 2;
    *W(ARG(0) + 84) += 1;
}
/* (record): +2 on by 1, +4 - +10 cleared. */
SFA_FUNC(088BC144, count_2_clear_4_to_10) {
    gaddr r = ARG(0);
    uint16_t n = *H(r + 2);
    *H(r + 4) = 0;
    *H(r + 2) = (uint16_t)(n + 1);
    *H(r + 6) = 0; *H(r + 8) = 0; *H(r + 10) = 0;
}
/* (object): at its owner's (+88) place moved by (0x50, 0x10) / (0x70, -10). */
static void at_owner(gaddr o, int dx, int dy) {
    gaddr w = *W(o + 88);
    *H(o + 18) = (uint16_t)(S16(w + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(w + 22) + dy);
}
SFA_FUNC(088BC650, at_owner_50_10) { at_owner(ARG(0), 0x50, 0x10); }
SFA_FUNC(088BDA40, at_owner_70_m10) { at_owner(ARG(0), 0x70, -10); }
/* (record): the byte at 0x08A3E960 + its +42 + its +43. */
SFA_FUNC(088D5A54, byte_by_42_43) { gaddr r = ARG(0); RETURN(*B(0x08A3E960u + *B(r + 42) + *B(r + 43))); }
/* (record or 0): its +8, or 0 for none or when +0 is negative. */
SFA_FUNC(088DF690, word_8_if_valid) {
    gaddr r = ARG(0);
    RETURN(r != 0 && (int32_t)*W(r) >= 0 ? *W(r + 8) : 0);
}
/* (list, out or 0): the list (+0x1C)'s +8; *out = its first entry + 0xC when there is one. */
SFA_FUNC(088E1074, list_first) {
    gaddr list = ARG(0) + 0x1C, out = ARG(1);
    ARG(0) = list;
    if (out != 0) {
        uint32_t first = *W(list);
        if (first != 0) *W(out) = first + 0xC;
    }
    RETURN(*W(list + 8));
}
/* (fighter): +96 = the word at 0x08A5837C by its +194. */
SFA_FUNC(0890AB14, speed_by_194) { gaddr f = ARG(0); *W(f + 96) = *W(0x08A5837Cu + (uint32_t)(int32_t)S8(f + 194) * 4); }
/* (object): step 2 (+4) unless its owner's (+88) record (+296) has +1 set. */
SFA_FUNC(08947E94, step_2_unless_owner) {
    gaddr o = ARG(0);
    if (*B(*W(*W(o + 88) + 296) + 1) == 0) *B(o + 4) = 2;
}
/* (object): +5 on, +0x53 = ' '. */
SFA_FUNC(089703FC, step_on_5_space) { gaddr o = ARG(0); *B(o + 5) = (uint8_t)(*B(o + 5) + 1); *B(o + 0x53) = 0x20; }
/* (object, x): speed +96 = (x - its x (+16)) * 12 / 256. */
SFA_FUNC(08987300, speed_towards) {
    gaddr o = ARG(0);
    int32_t d = (int32_t)(ARG(1) - *W(o + 16));
    *W(o + 96) = (uint32_t)((int32_t)((uint32_t)d * 12u) >> 8);
}
/* (object): +824 = 1 if +76 is set, else 0. */
SFA_FUNC(0898A874, flag_824_from_76) { gaddr o = ARG(0); *W(o + 824) = S8(o + 76) != 0 ? 1u : 0u; }

/* ---------------------------------------------------------------- more 7-instruction leaves */
/* (v): the byte at 0x08AD8FE4 = v != 0. */
SFA_FUNC(088051A0, set_08AD8FE4_flag) { *B(0x08AD8FE4u) = ARG(0) != 0; }
/* (object): +84 = the s8 at 0x09523C64. */
SFA_FUNC(08818E5C, take_09523C64) { *H(ARG(0) + 84) = (uint16_t)(int16_t)S8(0x09523C60u + 4); }
/* (record): the word at 0x08A0E264 by its +56. */
SFA_FUNC(08826184, word_by_56) { RETURN(*W(0x08A0E264u + (uint32_t)(int32_t)S16(ARG(0) + 56) * 4)); }
/* (n): the byte at 0x08A0F604 + n's low byte. */
SFA_FUNC(08831798, byte_08A0F604) { RETURN(*B(0x08A0F604u + (ARG(0) & 0xFF))); }
/* BATTLE +46 = the s16 at 0x09524A68 / 8. */
SFA_FUNC(088369AC, battle_46_from_09524A68) { *H(BATTLE + 46) = (uint16_t)(S16(0x09524A68u) >> 3); }
/* (fighter): 1 while its held keys (+752) have bit 2. */
SFA_FUNC(08862430, held_bit_2) { RETURN((*H(ARG(0) + 752) & 4) != 0); }
/* (object): +4 = +5 = 1, +16 = its +114 as a 16.16 place. */
static void start_16_from_114(gaddr o) {
    int16_t x = S16(o + 114);
    *B(o + 5) = 1; *B(o + 4) = 1;
    *W(o + 16) = (uint32_t)x << 16;
}
SFA_FUNC(0889EF00, start_16_from_114_a) { start_16_from_114(ARG(0)); }
SFA_FUNC(0889F7F4, start_16_from_114_b) { start_16_from_114(ARG(0)); }
/* (object): +64 counted on; 1 once it reaches 13. */
SFA_FUNC(088AC360, count_64_to_13) {
    gaddr o = ARG(0);
    uint16_t n = (uint16_t)(*H(o + 64) + 1);
    *H(o + 64) = n;
    RETURN(n >= 13);
}
/* (object): +13 = the byte at 0x08A302A8 by its +78. */
SFA_FUNC(088B1AB4, set_13_by_78) { gaddr o = ARG(0); *B(o + 13) = *B(0x08A302A8u + (uint32_t)(int32_t)S8(o + 78)); }
/* (v): the session's +228 = v, and its partner's (+232) +228 = v ^ 1. */
SFA_FUNC(088DD070, session_228_pair) {
    uint32_t v = ARG(0);
    gaddr other = *W(SESSION + 232);
    *W(SESSION + 228) = v;
    *W(other + 228) = v ^ 1;
}