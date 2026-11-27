/* Super-combo starts (super_start.h): each one's choice / distance tests, its super number and how it
   starts. Offsets stay numbers until something shows what they are.
 */
#include "super_start.h"

void f_089439DC(Cpu *c); void f_08902314(Cpu *c); void f_089A536C(Cpu *c); void f_088FDA68(Cpu *c);
void f_0899B624(Cpu *c); void f_089238A0(Cpu *c);

#define SUPER(...) static const Super k = { __VA_ARGS__ }; super_try(c, &k)

SFA_FUNC(08983EFC, super_13) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .b236 = 0xD, .b235 = -1, .d3e0 = 1,
          .b_gauge = 0x08983F24u, .b_start = 0x08983F5Cu);
}
SFA_FUNC(08901EB0, super_6) {
    SUPER(.frame = FR_S1Z, .test = T_GT0, .b236 = 6, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x08901ED8u, .b_start = 0x08901F18u);
}
SFA_FUNC(08901FBC, super_8) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .b236 = 8, .b235 = 1,
          .b_gauge = 0x08901FE4u, .b_start = 0x08902024u);
}
SFA_FUNC(089739F8, super_14) {
    SUPER(.frame = FR_S0, .test = T_NE0, .ret = R_ONE, .b236 = 0xE, .b235 = -1, .clr696 = 1,
          .b_gauge = 0x08973A18u, .b_start = 0x08973A50u);
}
SFA_FUNC(08973A78, super_15) {
    SUPER(.frame = FR_S0, .test = T_NE0, .ret = R_ONE, .b236 = 0xF, .b235 = -1, .clr696 = 1,
          .b_gauge = 0x08973A98u, .b_start = 0x08973AD0u);
}
SFA_FUNC(08909998, super_3_near) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .near = N_48, .b236 = 3, .b235 = -1,
          .b_gauge = 0x089099D0u, .b_start = 0x08909A08u);
}
SFA_FUNC(08943550, super_39dc_near) {
    SUPER(.frame = FR_S0, .test = T_NE0, .near = N_48, .ret = R_ONE, .b236 = -1, .b235 = -1, .d3e0 = 1,
          .mid = f_089439DC, .mid_a1 = 0xA, .b_gauge = 0x08943580u, .b_pre = 0x089435A8u, .b_start = 0x089435B0u);
}
SFA_FUNC(08901F30, super_7_2314) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .b236 = 7, .b235 = 1, .d3e0 = 1, .pre = f_08902314,
          .b_gauge = 0x08901F58u, .b_pre = 0x08901F6Cu, .b_start = 0x08901FA4u);
}
SFA_FUNC(089A34FC, super_9_536c) {
    SUPER(.frame = FR_S0, .test = T_NE0, .ret = R_LEFT, .b236 = 9, .b235 = 1, .d3e0 = 1, .pre = f_089A536C,
          .b_gauge = 0x089A351Cu, .b_pre = 0x089A352Cu, .b_start = 0x089A3564u);
}
SFA_FUNC(089A3588, super_a_536c) {
    SUPER(.frame = FR_S0, .test = T_NE0, .ret = R_LEFT, .b236 = 0xA, .b235 = 1, .pre = f_089A536C,
          .b_gauge = 0x089A35A8u, .b_pre = 0x089A35B8u, .b_start = 0x089A35F0u);
}
SFA_FUNC(088EF1F8, super_5) {
    SUPER(.frame = FR_S1, .test = T_NE0, .b236 = 5, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x088EF21Cu, .b_start = 0x088EF258u);
}
SFA_FUNC(088F2F60, super_4_near) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .near = N_48, .b236 = 4, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x088F2F98u, .b_start = 0x088F2FD8u);
}

SFA_FUNC(088F3104, super_5_gt_near) {
    SUPER(.frame = FR_S1Z, .test = T_GT0, .near = N_48, .b236 = 5, .b235 = 1,
          .b_gauge = 0x088F313Cu, .b_start = 0x088F317Cu);
}
SFA_FUNC(0893D478, super_8_nearu) {
    SUPER(.frame = FR_S0, .test = T_NE0, .near = N_48U, .ret = R_ONE, .b236 = 8, .b235 = -1, .d3e0 = 1,
          .b_gauge = 0x0893D4ACu, .b_start = 0x0893D4E0u);
}
SFA_FUNC(089737F0, super_4_not1) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .b236 = 4, .b235 = -1, .clr696 = 1, .d3e0 = 1,
          .b_gauge = 0x08973818u, .b_start = 0x08973850u);
}
SFA_FUNC(0899EF58, super_8_near_left) {
    SUPER(.frame = FR_S0, .test = T_NE0, .near = N_48, .ret = R_LEFT, .b236 = 8, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0899EF88u, .b_start = 0x0899EFC4u);
}
SFA_FUNC(0898A0D0, super_9_836) {
    SUPER(.frame = FR_S1, .test = T_NE0, .b236 = 9, .b235 = 1, .clr836 = 1, .d3e0 = 1,
          .b_gauge = 0x0898A0F4u, .b_start = 0x0898A134u);
}
SFA_FUNC(0898A164, super_8_not1_836) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .b236 = 8, .b235 = 1, .clr836 = 1,
          .b_gauge = 0x0898A18Cu, .b_start = 0x0898A1C8u);
}
SFA_FUNC(089A624C, super_8_not1_196) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .b236 = 8, .b235 = 1, .clr196 = 1,
          .b_gauge = 0x089A6274u, .b_start = 0x089A62B0u);
}
SFA_FUNC(089A62E0, super_9_196) {
    SUPER(.frame = FR_S1, .test = T_NE0, .b236 = 9, .b235 = 1, .clr196 = 1,
          .b_gauge = 0x089A6304u, .b_start = 0x089A6344u);
}
SFA_FUNC(089434B8, super_39dc_b) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = -1, .b235 = -1, .mid = f_089439DC, .mid_a1 = 0xB,
          .b_gauge = 0x089434F0u, .b_pre = 0x08943518u, .b_start = 0x08943520u);
}
SFA_FUNC(089A3464, super_8_536c_left) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .ret = R_LEFT, .b236 = 8, .b235 = 1, .d3e0 = 1, .pre = f_089A536C,
          .b_gauge = 0x089A348Cu, .b_pre = 0x089A349Cu, .b_start = 0x089A34D0u);
}
SFA_FUNC(088EF15C, super_4_free) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .busy536 = 1, .b236 = 4, .b235 = 1,
          .b_gauge = 0x088EF190u, .b_start = 0x088EF1C8u);
}
SFA_FUNC(088F7B94, super_5_near_free) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 5, .b235 = 1,
          .b_gauge = 0x088F7BD8u, .b_start = 0x088F7C18u);
}
SFA_FUNC(08914F88, super_4_clr4_left) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .ret = R_LEFT, .b236 = 4, .b235 = 1, .clr196 = 4,
          .b_gauge = 0x08914FB0u, .b_start = 0x08914FF8u);
}
SFA_FUNC(0896309C, super_4_free_b) {
    SUPER(.frame = FR_S1, .test = T_NE0, .busy536 = 1, .b236 = 4, .b235 = 1,
          .b_gauge = 0x089630CCu, .b_start = 0x08963108u);
}
SFA_FUNC(0896739C, super_1_near) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 1, .b235 = 1,
          .b_gauge = 0x089673D4u, .b_start = 0x08967408u);
}

/* (fighter): s2 is the result here: unless the choice is 1 or the opponent is closer than 48, with the
   gauge the super state, super 1, +235 = 1, facing the opponent, strength 2 and the start 0x0886D2EC. */
SFA_FUNC(08909888, super_1_s2) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = 0; c->r[17] = 1; c->r[16] = o;
    if (S8(o + 386) != 1 && S16(o + 366) >= 48) {
        ARG(0) = o; c->r[31] = 0x089098C8u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 1; *B(o + 235) = 1; *B(o + 11) = *B(o + 697);
            c->r[18] = 1;
            *B(o + 194) = 2;
            ARG(0) = o; c->r[31] = 0x08909908u; f_0886D2EC(c);
        }
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* generated from the bodies by scratch supgen.py, then checked by parity */
SFA_FUNC(0899EFE8, super_efe8) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .ret = R_LEFT, .b236 = 7, .b235 = 1,
          .b_gauge = 0x0899F020u, .b_start = 0x0899F058u);
}
SFA_FUNC(089064E0, super_64e0) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 4, .b235 = 1, .d3e0 = 1, .b_gauge = 0x08906514u,
          .b_start = 0x08906550u);
}
SFA_FUNC(08906580, super_6580) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 5, .b235 = 1, .b_gauge = 0x089065B8u,
          .b_start = 0x089065F0u);
}
SFA_FUNC(0890FF9C, super_ff9c) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 2, .b235 = 1, .d3e0 = 1, .b_gauge = 0x0890FFD0u,
          .b_start = 0x0891000Cu);
}
SFA_FUNC(08926390, super_6390) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 4, .b235 = 1, .d3e0 = 1, .b_gauge = 0x089263C4u,
          .b_start = 0x08926400u);
}
SFA_FUNC(08926430, super_6430) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 5, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x08926468u, .b_start = 0x089264A0u);
}
SFA_FUNC(0892A934, super_a934) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 0xb, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0892A96Cu, .b_start = 0x0892A9A4u);
}
SFA_FUNC(0892AA80, super_aa80) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1, .b_gauge = 0x0892AAB4u,
          .b_start = 0x0892AAF0u);
}
SFA_FUNC(089302B0, super_02b0) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 7, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x089302E8u, .b_start = 0x08930320u);
}
SFA_FUNC(08930350, super_0350) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 8, .b235 = 1, .d3e0 = 1, .b_gauge = 0x08930384u,
          .b_start = 0x089303C0u);
}
SFA_FUNC(0895CA08, super_ca08) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0895CA40u, .b_start = 0x0895CA78u);
}
SFA_FUNC(0895CAA8, super_caa8) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 8, .b235 = 1, .d3e0 = 1, .b_gauge = 0x0895CADCu,
          .b_start = 0x0895CB18u);
}
SFA_FUNC(0896331C, super_331c) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 0xe, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x08963350u, .b_start = 0x0896338Cu);
}
SFA_FUNC(08967438, super_7438) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 2, .b235 = 1, .d3e0 = 1, .b_gauge = 0x0896746Cu,
          .b_start = 0x089674A8u);
}
SFA_FUNC(0897BFE8, super_bfe8) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 0xd, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0897C01Cu, .b_start = 0x0897C058u);
}
SFA_FUNC(0898ED20, super_ed20) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1, .b_gauge = 0x0898ED54u,
          .b_start = 0x0898ED90u);
}
SFA_FUNC(0898EE5C, super_ee5c) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 0xf, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0898EE94u, .b_start = 0x0898EECCu);
}
SFA_FUNC(08994630, super_4630) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1, .b_gauge = 0x08994664u,
          .b_start = 0x089946A0u);
}
SFA_FUNC(0896C620, super_c620) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48U, .b236 = 8, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x0896C658u, .b_start = 0x0896C694u);
}
SFA_FUNC(089B108C, super_108c) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48U, .b236 = 9, .b235 = 1, .b_gauge = 0x089B10C4u,
          .b_start = 0x089B1100u);
}
SFA_FUNC(088FD6C4, super_d6c4) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 3, .b235 = 1, .d3e0 = 1, .pre = f_088FDA68,
          .b_pre = 0x088FD70Cu, .b_gauge = 0x088FD6FCu, .b_start = 0x088FD740u);
}
SFA_FUNC(088FD770, super_d770) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 4, .b235 = 1,
          .b_gauge = 0x088FD7B0u, .b_start = 0x088FD7ECu);
}
SFA_FUNC(08919110, super_9110) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 5, .b235 = 1, .clr196 = 4,
          .b_gauge = 0x08919154u, .b_start = 0x089191A4u);
}
SFA_FUNC(0892A9D4, super_a9d4) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 4, .b235 = 1,
          .b_gauge = 0x0892AA14u, .b_start = 0x0892AA50u);
}
SFA_FUNC(0899B4CC, super_b4cc) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1, .pre = f_0899B624,
          .b_pre = 0x0899B514u, .b_gauge = 0x0899B504u, .b_start = 0x0899B548u);
}
SFA_FUNC(0899B578, super_b578) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 7, .b235 = 1,
          .b_gauge = 0x0899B5B8u, .b_start = 0x0899B5F4u);
}
SFA_FUNC(088FD614, super_d614) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 2, .b235 = 1, .clr196 = 4, .d3e0 = 1,
          .b_gauge = 0x088FD648u, .b_start = 0x088FD694u);
}
SFA_FUNC(0893D314, super_d314) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48U, .busy536 = 1, .b236 = 7, .b235 = 1,
          .b_gauge = 0x0893D358u, .b_start = 0x0893D394u);
}
SFA_FUNC(0893537C, super_537c) {
    SUPER(.frame = FR_S1ONE, .test = T_NE0, .near = N_48U, .b236 = 5, .b235 = 1, .clr196 = 4,
          .b_gauge = 0x089353B8u, .b_start = 0x08935400u);
}
SFA_FUNC(08935430, super_5430) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48U, .b236 = 6, .b235 = 1, .clr196 = 4,
          .b_gauge = 0x0893546Cu, .b_start = 0x089354B4u);
}
SFA_FUNC(089354E4, super_54e4) {
    SUPER(.frame = FR_S1ONE, .test = T_NE0, .near = N_48U, .b236 = 4, .b235 = 1, .clr196 = 4, .d3e0 = 1,
          .b_gauge = 0x08935520u, .b_start = 0x08935568u);
}

/* by hand: the bodies with something the generator does not take */
SFA_FUNC(0899489C, super_13_49) {
    SUPER(.frame = FR_S1ONE, .test = T_EQ1, .near = N_48, .lim = 49, .b236 = 0x13, .b235 = 1,
          .b_gauge = 0x089948D4u, .b_start = 0x0899490Cu);
}
SFA_FUNC(089191BC, super_6_free540) {
    SUPER(.frame = FR_S1Z, .test = T_NE0, .near = N_48, .busy_b = 540, .b236 = 6, .b235 = 0, .clr196 = 4,
          .d3e0 = 1, .b_gauge = 0x08919200u, .b_start = 0x0891924Cu);
}
SFA_FUNC(0891003C, super_5_823) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .b236 = 5, .b235 = 1, .set823 = 1,
          .b_gauge = 0x08910070u, .b_start = 0x089100B8u);
}
SFA_FUNC(08930204, super_6_noshot) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .busy_w = 228, .b236 = 6, .b235 = 1,
          .b_gauge = 0x08930244u, .b_start = 0x08930280u);
}

/* (fighter): s2 is the result: unless the choice is 1 or the opponent is closer than 48, with the gauge
   super 4, +235 = 1, the super state, facing the opponent, +196..+199 cleared and the start 0x0886D2EC. */
SFA_FUNC(08919068, super_4_s2) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = 0; c->r[17] = 1; c->r[16] = o;
    if (S8(o + 386) != 1 && S16(o + 366) >= 48) {
        ARG(0) = o; c->r[31] = 0x089190A8u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 236) = 4; *B(o + 235) = 1;
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 11) = *B(o + 697);
            for (int i = 196; i < 200; i++) *B(o + i) = 0;
            c->r[18] = 1;
            ARG(0) = o; c->r[31] = 0x089190F4u; f_0886D2EC(c);
        }
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(089264D0, super_6_clr127) {
    SUPER(.frame = FR_S1, .test = T_NE0, .near = N_48, .busy536 = 1, .b236 = 6, .b235 = 1, .clr_b = 127,
          .b_gauge = 0x08926510u, .b_start = 0x08926550u);
}
SFA_FUNC(08949568, super_4_char11) {
    SUPER(.frame = FR_S1, .need338 = 0x11, .test = T_NE0, .near = N_48, .b236 = 4, .b235 = 1,
          .b_gauge = 0x089495ACu, .b_start = 0x089495E8u);
}
SFA_FUNC(08949618, super_5_char11) {
    SUPER(.frame = FR_S1ONE, .need338 = 0x11, .test = T_EQ1, .near = N_48, .b236 = 5, .b235 = 1,
          .b_gauge = 0x08949660u, .b_start = 0x08949698u);
}
SFA_FUNC(089496C8, super_6_char13) {
    SUPER(.frame = FR_S1ONE, .need338 = 0x13, .test = T_EQ1, .near = N_48, .b236 = 6, .b235 = 1, .d3e0 = 1,
          .b_gauge = 0x08949710u, .b_start = 0x08949748u);
}
SFA_FUNC(0893D3C4, super_c_nochoice) {
    SUPER(.frame = FR_S1, .test = T_NOT_M1, .busy536 = 1, .near = N_48U, .b236 = 0xC, .b235 = 1,
          .b_gauge = 0x0893D40Cu, .b_start = 0x0893D448u);
}

/* (fighter): with a choice made below zero and the opponent 48 or more away (16 bits), with the gauge the
   super state, facing the opponent, +235 = 1 and super 0xB through 0x0886D3E0 when +760 bits 7..9 are
   held, else super 9 through 0x0886D2EC. */
SFA_FUNC(089B0F2C, super_9_or_b) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = o;
    uint32_t ret = 0;
    if (S8(o + 386) < 0 && *H(o + 366) >= 48) {
        ARG(0) = o; c->r[31] = 0x089B0F64u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 11) = *B(o + 697);
            c->r[17] = 1;
            *B(o + 235) = 1;
            ARG(0) = o;
            if (*H(o + 760) & 0x380) { *B(o + 236) = 0xB; c->r[31] = 0x089B0FC4u; f_0886D3E0(c); }
            else { *B(o + 236) = 9; c->r[31] = 0x089B0FACu; f_0886D2EC(c); }
            ret = c->r[17];
        }
    }
    c->r[2] = ret;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* the start that measures the distance against +430's entry in `tab`, calls 0x089238A0 after the super
   number and sets strength 2 (+194 / +552) when +430 is set */
static void super_430(Cpu *c, int s1one, gaddr tab, uint32_t num, int d3e0, const uint32_t b[3]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    if (s1one) c->r[17] = 1;
    c->r[16] = o;
    uint32_t ret = 0;
    int32_t ch = S8(o + 386);
    if (s1one ? ch != 1 : ch == 0) {
        int32_t lim = *H(tab + (uint32_t)(S8(o + 430) * 2));
        if (!((int32_t)*H(o + 366) < lim)) {
            ARG(0) = o; c->r[31] = b[0]; f_0886C6EC(c);
            if (c->r[2] != 0) {
                *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
                *B(o + 236) = (uint8_t)num;
                ARG(0) = o; c->r[31] = b[1]; f_089238A0(c);
                c->r[17] = 1;
                *B(o + 235) = 1; *B(o + 11) = *B(o + 697);
                ARG(0) = o; c->r[31] = b[2];
                if (d3e0) f_0886D3E0(c); else f_0886D2EC(c);
                if (S8(o + 430) != 0) { *B(o + 194) = 2; *B(o + 552) = 2; }
                ret = c->r[17];
            }
        }
    }
    c->r[2] = ret;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(08920BB4, super_4_430) {
    static const uint32_t b[3] = { 0x08920C04u, 0x08920C30u, 0x08920C48u };
    super_430(c, 0, 0x08A66D08u, 4, 0, b);
}
SFA_FUNC(08920C8C, super_5_430) {
    static const uint32_t b[3] = { 0x08920CE0u, 0x08920D0Cu, 0x08920D20u };
    super_430(c, 1, 0x08A66D0Cu, 5, 1, b);
}
/* (fighter): unless the choice is 1 or the opponent is closer than 48 (16 bits), with the gauge super 5:
   the super state, facing the opponent, +235 = 1, +916 the strength's button held (+760 / +762 bits 6 /
   5, else 4) and the start 0x0886D2EC. */
SFA_FUNC(0896C6C4, super_5_button) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[17] = 1; c->r[16] = o;
    uint32_t ret = 0;
    if (S8(o + 386) != 1 && *H(o + 366) >= 48) {
        ARG(0) = o; c->r[31] = 0x0896C700u; f_0886C6EC(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 5; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
            uint32_t h = (*H(o + 760) | *H(o + 762)) & 0x70;
            *H(o + 0x390 + 4) = (h & 0x40) ? 0x40 : (h & 0x20) ? 0x20 : 0x10;
            ARG(0) = o; c->r[31] = 0x0896C770u; f_0886D2EC(c);
            ret = c->r[17];
        }
    }
    c->r[2] = ret;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with no choice made and the gauge super 3: the super state, +235 = 1, facing the opponent,
   +196..+199 cleared; then by the buttons held (+760 | +762): with +564 clear any of bits 4..6 starts it
   through 0x0886D2EC (+193 cleared), else 0x0886D3E0 (+193 = 1); with +564 set bits 5 and 9 together
   give 0x0886D2EC (+193 cleared), else 0x0886D3E0 (+193 = 1). The result is the start's. */
SFA_FUNC(08914E84, super_3_buttons) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(o + 386) != 0) { c->r[2] = 0; goto out; }
    ARG(0) = o; c->r[31] = 0x08914EA4u; f_0886C6EC(c);
    if (c->r[2] == 0) { c->r[2] = 0; goto out; }
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
    *B(o + 236) = 3; *B(o + 235) = 1; *B(o + 11) = *B(o + 697);
    for (int i = 196; i < 200; i++) *B(o + i) = 0;
    {
        int32_t h = (int16_t)(*H(o + 760) | *H(o + 762));
        ARG(0) = o;
        if (S8(o + 564) == 0) {
            if (h & 0x70) { *B(o + 193) = 0; c->r[31] = 0x08914F48u; f_0886D2EC(c); }
            else { *B(o + 193) = 1; c->r[31] = 0x08914F38u; f_0886D3E0(c); }
        } else if ((h & 0x220) == 0x220) { *B(o + 193) = 0; c->r[31] = 0x08914F68u; f_0886D2EC(c); }
        else { *B(o + 193) = 1; c->r[31] = 0x08914F78u; f_0886D3E0(c); }
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): not with choice 1. Locked choices (below zero) at 144 or more away with the gauge take super
   0xF without a start: step 2 of the super state, +235 / +196..+199 cleared, facing, strength 2, and the
   gauge's answer is the result. Otherwise at 48 or more with the gauge super 0xC (+235 = 1, the state,
   facing) through 0x0886D2EC, with the start's result. */
SFA_FUNC(0897BEDC, super_c_or_f) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; c->r[17] = 1; *W(sp + 24) = ra;
    int32_t k = S8(o + 386);
    int32_t d = S16(o + 366);
    if (k == 1) { c->r[2] = 0; goto out; }
    if (k & 0x80) {
        if (d < 144) { c->r[2] = 0; goto out; }
        ARG(0) = o; c->r[31] = 0x0897BF7Cu; f_0886C6EC(c);
        if (c->r[2] == 0) { c->r[2] = 0; goto out; }
        *B(o + 236) = 0xF; *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 2;
        *B(o + 235) = 0;
        for (int i = 196; i < 200; i++) *B(o + i) = 0;
        *B(o + 11) = *B(o + 697); *B(o + 552) = 2; *B(o + 194) = 2;
    } else {
        if (d < 48) { c->r[2] = 0; goto out; }
        ARG(0) = o; c->r[31] = 0x0897BF1Cu; f_0886C6EC(c);
        if (c->r[2] == 0) { c->r[2] = 0; goto out; }
        *B(o + 236) = 0xC; *B(o + 235) = 1;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
        *B(o + 11) = *B(o + 697);
        ARG(0) = o; c->r[31] = 0x0897BF54u; f_0886D2EC(c);
    }
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}