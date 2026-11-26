/* Move steps of 20 - 24 instructions: landings and knock-downs, the fighter step dispatcher's wait
   (0x0885D164, 33,453 calls), moves picked by strength or a threshold, and recoveries. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08861848(Cpu *c); void f_0886163C(Cpu *c); void f_088616EC(Cpu *c); void f_08861674(Cpu *c);
void f_08908BD8(Cpu *c); void f_089070C4(Cpu *c); void f_0893E0B0(Cpu *c); void f_0893E040(Cpu *c);
void f_088D2D14(Cpu *c); void f_0886E294(Cpu *c); void f_088F0C48(Cpu *c); void f_0890CC04(Cpu *c);
void f_0890CC44(Cpu *c); void f_08862004(Cpu *c); void f_08862068(Cpu *c); void f_0885C350(Cpu *c);
void f_0885C374(Cpu *c); void f_088619EC(Cpu *c); void f_088F504C(Cpu *c); void f_08821330(Cpu *c);

#define BATTLE 0x092E1228u

static void bump(gaddr a) { *B(a) = (uint8_t)(*B(a) + 1); }
static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void frame_in(Cpu *c, uint32_t *ra) { *ra = c->r[31]; c->r[29] -= 0x20; *W(c->r[29] + 16) = *ra; }
static void frame_out(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void stand(gaddr o) { *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }

/* (fighter): 0x08908BD8; above the floor ticked, else landed (0x089070C4). */
SFA_FUNC(08907074, move_fall_or_land_b) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08908BD8, 0x08907088u);
    if (S16(o + 132) < S16(o + 22)) call1(c, o, f_08861848, 0x089070A4u);
    else call1(c, o, f_089070C4, 0x089070B4u);
    leave(c, sv);
}

/* (fighter): ticked, 0x0893E0B0; when the animation ends +7 steps on, +82 = 16 and 0x0893E040. */
SFA_FUNC(0893DE58, move_tick_end_helper) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x0893DE6Cu);
    call1(c, o, f_0893E0B0, 0x0893DE74u);
    if (S8(o + 75) < 0) { bump(o + 7); *B(o + 0x52) = 0x10; call1(c, o, f_0893E040, 0x0893DEA0u); }
    leave(c, sv);
}

/* (fighter): when the animation ends, standing again facing the other way; else ticked. */
SFA_FUNC(08961E30, move_end_turn) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 75) < 0) {
        uint32_t f = *B(o + 11);
        stand(o);
        *B(o + 11) = (uint8_t)(f ^ 1);
        call2(c, o, 0, f_0886163C, 0x08961E68u);
    } else call1(c, o, f_08861848, 0x08961E7Cu);
    frame_out(c, ra);
}

/* (fighter): step 3, a push (+104 = 1/2, away from the facing +11), move 0x3F (list 1) and cue 0x30. */
SFA_FUNC(089BD30C, move_push_3f) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    *B(o + 7) = 3;
    uint32_t f = *B(o + 11);
    *W(o + 104) = 0x8000;
    enter(c, o, sv);
    if (f == 0) *W(o + 104) = 0u - *W(o + 104);
    call2(c, o, 0x3F, f_088616EC, 0x089BD34Cu);
    call1(c, o, f_088D2D14, 0x089BD354u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +237 cleared, +7 steps on and +573 from the table at `tab`
   by 0x0886E294. */
static void tick_573(Cpu *c, gaddr tab, int sgn, uint32_t b1, uint32_t b2) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, b1);
    if (S8(o + 77) == 0) {
        uint32_t st = *B(o + 7);
        *B(o + 237) = 0;
        *B(o + 7) = (uint8_t)(st + 1);
        call1(c, o, f_0886E294, b2);
        gaddr a = c->r[2] + tab;
        *B(o + 573) = sgn ? (uint8_t)S8(a) : *B(a);
    }
    leave(c, sv);
}
SFA_FUNC(088F0324, move_tick_573) { tick_573(c, 0x08A43B54u, 1, 0x088F0338u, 0x088F035Cu); }
SFA_FUNC(0897435C, move_tick_573_b) { tick_573(c, 0x08A900C2u, 0, 0x08974370u, 0x08974394u); }

/* (fighter): on a frame with +1 set and no +123 the +82 count goes up and +260 takes 0x08A43BA0's entry;
   then it slides (0x088F0C48). */
SFA_FUNC(088F0BEC, move_count_slide) {
    gaddr o = ARG(0);
    uint32_t ra;
    gaddr fr = *W(o + 296);
    frame_in(c, &ra);
    if (*B(fr + 1) != 0 && S8(o + 123) == 0) {
        *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
        *B(o + 260) = (uint8_t)S8(0x08A43BA0u + (uint32_t)S16(o + 82));
    }
    c->r[31] = 0x088F0C3Cu; f_088F0C48(c);
    frame_out(c, ra);
}

/* (fighter): 0x0890CC04, 0x0890CC44; when the mark's timer (+0x337) runs out, or once on the floor, +7
   steps on with +73 cleared and the height set to the floor, and ticked. */
SFA_FUNC(0890B9A0, move_air_timer_land) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_0890CC04, 0x0890B9B4u);
    call1(c, o, f_0890CC44, 0x0890B9BCu);
    gaddr m = o + 0x334;
    int32_t t = S8(m + 3);
    int out = 0;
    if (t != 0) { *B(m + 3) = (uint8_t)(t - 1); out = (int8_t)(t - 1) == 0; }
    if (out) {
        int32_t y = S16(o + 132);
        bump(o + 7); *B(o + 73) = 0; *H(o + 22) = (uint16_t)y;
        call1(c, o, f_08861848, 0x0890BA1Cu);
    } else if (!(S16(o + 132) < S16(o + 22))) {
        int32_t y = S16(o + 132);
        *B(o + 73) = 0; bump(o + 7); *H(o + 22) = (uint16_t)y;
        call1(c, o, f_08861848, 0x0890BA44u);
    }
    leave(c, sv);
}

/* (fighter): once moving back (+96 < 0) step 6 with move 0x17 + its +194; else 0x0890CC04 and ticked. */
SFA_FUNC(0890BB78, move_back_17) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t v = (int32_t)*W(o + 96);
    enter(c, o, sv);
    if (v < 0) {
        int32_t k = S8(o + 194);
        *B(o + 7) = 6;
        call2(c, o, (uint32_t)(k + 0x17) & 0xFFFF, f_088616EC, 0x0890BBACu);
    } else {
        call1(c, o, f_0890CC04, 0x0890BBBCu);
        call1(c, o, f_08861848, 0x0890BBC4u);
    }
    leave(c, sv);
}

/* (fighter): move 2k or 2k + 1 (k its +194) by its +508 against the threshold at 0x08A5AD18. */
SFA_FUNC(0890EFE0, move_by_508) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t k = S8(o + 194);
    uint32_t thr = *H(0x08A5AD18u + 2 * (uint32_t)k);
    frame_in(c, &ra);
    uint32_t n = (uint32_t)k << 1;
    if (S16(o + 508) < (int32_t)thr) call2(c, o, n & 0xFFFF, f_08861674, 0x0890F018u);
    else call2(c, o, (n + 1) & 0xFFFF, f_08861674, 0x0890F030u);
    frame_out(c, ra);
}

/* (fighter): move 2k + 7 or 2k + 6 by the threshold at 0x08A5AD1E against its +508. */
SFA_FUNC(0890F5EC, move_by_508_b) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t k = S8(o + 194);
    uint32_t thr = *H(0x08A5AD1Eu + 2 * (uint32_t)k);
    frame_in(c, &ra);
    uint32_t n = (uint32_t)k << 1;
    if ((int32_t)thr < S16(o + 508)) call2(c, o, (n + 7) & 0xFFFF, f_08861674, 0x0890F628u);
    else call2(c, o, (n + 6) & 0xFFFF, f_08861674, 0x0890F640u);
    frame_out(c, ra);
}

/* (fighter): ticked; when the +82 timer runs out, standing again. */
SFA_FUNC(08929DBC, move_tick_timer_stand) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08929DD0u);
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t == 0) { stand(o); call2(c, o, 0, f_0886163C, 0x08929E08u); }
    leave(c, sv);
}

/* (fighter): without a frame flag +73 = 1 and it falls (+20 by +100, +100 by +108; once falling +7
   steps on); ticked. */
SFA_FUNC(0892BBDC, move_tick_fall_77) {
    gaddr o = ARG(0);
    uint32_t ra;
    frame_in(c, &ra);
    if (S8(o + 77) == 0) {
        uint32_t v = *W(o + 100);
        *B(o + 73) = 1;
        *W(o + 20) = *W(o + 20) + v;
        v += *W(o + 108);
        *W(o + 100) = v;
        if ((int32_t)v < 0) bump(o + 7);
    }
    call1(c, o, f_08861848, 0x0892BC2Cu);
    frame_out(c, ra);
}

/* (fighter): 0x08862004; once falling +7 steps on with move 0x21 + its +194; else ticked. */
SFA_FUNC(0898B2A8, move_rise_then_21) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08862004, 0x0898B2BCu);
    if ((int32_t)*W(o + 100) < 0) {
        int32_t k = S8(o + 194);
        bump(o + 7);
        call2(c, o, (uint32_t)(k + 0x21) & 0xFFFF, f_088616EC, 0x0898B2F4u);
    } else call1(c, o, f_08861848, 0x0898B2D0u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 steps on, and facing left (+11 clear) the x speeds turn. */
SFA_FUNC(08996A84, move_tick_turn_speed) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08996A98u);
    if (S8(o + 77) == 0) {
        bump(o + 7);
        if (*B(o + 11) == 0) { *W(o + 96) = 0u - *W(o + 96); *W(o + 104) = 0u - *W(o + 104); }
    }
    leave(c, sv);
}

/* (fighter): step 1 / 0 / 3 / 0, facing +697, +503 cleared, +78 = +497, 0x08862068, move 0x10. */
SFA_FUNC(0885B364, move_guard_10) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 3; *B(o + 7) = 0;
    *B(o + 11) = *B(o + 697); *B(o + 503) = 0; *B(o + 78) = *B(o + 497);
    call1(c, o, f_08862068, 0x0885B3A8u);
    call2(c, o, 0x10, f_0886163C, 0x0885B3B4u);
    leave(c, sv);
}

/* (fighter): knocked down with kind `k` (+116): alive (+112) with +538 and no +698, a heavy landing
   (+655 = 0x16, +117 = 12, 0x0885C350); else +631 cleared and the landing (0x0885C374). */
static void knockdown(Cpu *c, uint8_t k, uint32_t b_heavy, uint32_t b_land) {
    gaddr o = ARG(0);
    uint32_t ra;
    int32_t hp = S16(o + 112);
    frame_in(c, &ra);
    *B(o + 116) = k;
    if (hp >= 0 && S8(o + 538) != 0 && S8(o + 698) == 0) {
        *B(o + 655) = 0x16; *B(o + 117) = 0xC;
        call1(c, o, f_0885C350, b_heavy);
    } else { *B(o + 631) = 0; call1(c, o, f_0885C374, b_land); }
    frame_out(c, ra);
}
SFA_FUNC(0885C454, move_knockdown_4) { knockdown(c, 4, 0x0885C494u, 0x0885C4A8u); }
SFA_FUNC(0885C4B4, move_knockdown_8) { knockdown(c, 8, 0x0885C4F4u, 0x0885C508u); }

/* (fighter): while its +127 wait lasts it only ticks (0x088619EC) and counts it down; else the step of
   its kind (+116) from 0x08A1926C runs. */
SFA_FUNC(0885D164, fighter_kind_step) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    int32_t w = S8(o + 127);
    enter(c, o, sv);
    if (w != 0) {
        call1(c, o, f_088619EC, 0x0885D184u);
        *B(o + 127) = (uint8_t)(S8(o + 127) - 1);
    } else {
        ARG(0) = o; c->r[31] = 0x0885D1B4u;
        rt_call_indirect(c, *W(0x08A1926Cu + ((uint32_t)*B(o + 116) << 2)));
    }
    leave(c, sv);
}

/* (fighter): 0x088F504C; once moving back step 7, speeds cleared but a fall (+108), move 0x33; else ticked. */
SFA_FUNC(088F4F04, move_back_33) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_088F504C, 0x088F4F18u);
    if ((int32_t)*W(o + 96) < 0) {
        *B(o + 7) = 7; *W(o + 104) = 0; *W(o + 96) = 0; *W(o + 108) = 0xFFFFC800u;
        call2(c, o, 0x33, f_088616EC, 0x088F4F44u);
    } else call1(c, o, f_08861848, 0x088F4F54u);
    leave(c, sv);
}

/* (fighter): ticked; without a frame flag +7 steps on, +73 = 1, +78 = -1 and the mark (+0x334) takes
   0x08821330's +16. */
SFA_FUNC(08906E74, move_tick_mark) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call1(c, o, f_08861848, 0x08906E88u);
    if (S8(o + 77) == 0) {
        bump(o + 7); *B(o + 73) = 1; *B(o + 78) = 0xFF;
        call1(c, o, f_08821330, 0x08906EB8u);
        *W(o + 0x334) = *W(c->r[2] + 16);
    }
    leave(c, sv);
}

/* (fighter): the +82 timer runs down; past 0 +7 steps on and its side's bit (+337) set in the
   battle's +11; ticked. */
SFA_FUNC(089088E4, move_timer_side_bit) {
    gaddr o = ARG(0);
    uint32_t ra;
    uint32_t t = (uint32_t)S16(o + 82) - 1;
    frame_in(c, &ra);
    *H(o + 82) = (uint16_t)t;
    if ((int16_t)t < 0) {
        bump(o + 7);
        *B(BATTLE + 11) = (uint8_t)((uint32_t)S8(BATTLE + 11) | (1u << (S8(o + 337) & 31)));
    }
    call1(c, o, f_08861848, 0x08908938u);
    frame_out(c, ra);
}
