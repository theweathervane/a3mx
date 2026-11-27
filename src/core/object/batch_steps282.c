/* Small fighter and effect steps: stand-up waits, a dash that ends in a spark, and effect script steps
   that follow their fighter. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088622CC(Cpu *c); void f_088622E8(Cpu *c); void f_08864740(Cpu *c); void f_0886469C(Cpu *c);
void f_08861848(Cpu *c); void f_088F3720(Cpu *c); void f_088F5DBC(Cpu *c); void f_089D8C40(Cpu *c);
void f_08821AA8(Cpu *c); void f_088616EC(Cpu *c);

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

/* A lying-down wait: once 0x088622CC allows it the stand-up choice (0x088622E8); else at the
   animation's end the given hand-over, otherwise ticked. */
static void down_wait(Cpu *c, void (*end)(Cpu *), const uint32_t back[4]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_088622CC, back[0]);
    if (c->r[2] != 0) call1(c, o, f_088622E8, back[1]);
    else if (S8(o + 75) & 0x80) call1(c, o, end, back[2]);
    else call1(c, o, f_08861848, back[3]);
    leave(c, sv);
}

/* (fighter): lying down, ending in the crouch hand-over 0x08864740. */
SFA_FUNC(0885A458, down_wait_crouch) {
    static const uint32_t b[4] = { 0x0885A46Cu, 0x0885A47Cu, 0x0885A498u, 0x0885A4A8u };
    down_wait(c, f_08864740, b);
}

/* (fighter): lying down, ending in the stand hand-over 0x0886469C. */
SFA_FUNC(0885A66C, down_wait_stand) {
    static const uint32_t b[4] = { 0x0885A680u, 0x0885A690u, 0x0885A6ACu, 0x0885A6BCu };
    down_wait(c, f_0886469C, b);
}

/* (fighter): a dash step: 0x088F3720; going backwards stops it (step 3); heavy ones throw a spark of kind
   7 (0x088F5DBC); ticked. */
SFA_FUNC(088F3548, dash_spark) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_088F3720, 0x088F355Cu);
    if ((int32_t)*W(o + 96) < 0) { *B(o + 7) = 3; *W(o + 104) = 0; *W(o + 96) = 0; }
    if (S8(o + 194) == 2) call2(c, o, 7, f_088F5DBC, 0x088F358Cu);
    call1(c, o, f_08861848, 0x088F3594u);
    leave(c, sv);
}

/* (effect, script): an effect script step: before the frame count +120 runs the script (0x08821AA8);
   then +5 on and the follow step 2, or at the round's end +5 on twice, +78 = 2 and the follow step 4. */
SFA_FUNC(089D31F0, effect_step_timed) {
    gaddr e = ARG(0), sc = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t n = (int16_t)*B(e + 120);
    int lt = S16(sc + 2) < n;
    c->r[5] = (uint32_t)lt;
    *W(c->r[29] + 16) = ra;
    if (lt) { c->r[31] = 0x089D3278u; f_08821AA8(c); }
    else {
        *B(e + 5) = (uint8_t)(*B(e + 5) + 1);
        if (S8(0x092E1228u + 13) == 0) call2(c, e, 2, f_089D8C40, 0x089D3240u);
        else {
            *B(e + 5) = (uint8_t)(*B(e + 5) + 1);
            *B(e + 78) = 2;
            call2(c, e, 4, f_089D8C40, 0x089D3264u);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (effect): runs its script (0x08821AA8) until the animation ends, then +5 on, +78 = 2 and the follow
   step 4. */
SFA_FUNC(089D3284, effect_step_end) {
    gaddr e = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(e + 75);
    *W(c->r[29] + 16) = ra;
    if ((int32_t)c->r[5] < 0) {
        *B(e + 5) = (uint8_t)(*B(e + 5) + 1);
        *B(e + 78) = 2;
        call2(c, e, 4, f_089D8C40, 0x089D32B4u);
    } else { c->r[31] = 0x089D32C8u; f_08821AA8(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_0887C614(Cpu *c); void f_0886C8F4(Cpu *c); void f_0886D2EC(Cpu *c); void f_089D7808(Cpu *c);
void f_089CF674(Cpu *c);

/* (fighter): an air dash step: heavy ones throw a spark (kind 7 rising, 8 falling); 0x088F3720; landed
   step 4, +73 / +262 / +235 cleared, set down, 0x0887C614 and ticked; in the air ticked unless +77. */
SFA_FUNC(088F35A4, air_dash_spark) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (S8(o + 194) == 2) call2(c, o, (int32_t)*W(o + 100) >= 0 ? 7 : 8, f_088F5DBC, 0x088F35E0u);
    call1(c, o, f_088F3720, 0x088F35E8u);
    int32_t y = S16(o + 132);
    if (!(y < S16(o + 22))) {
        *B(o + 7) = 4; *B(o + 73) = 0; *B(o + 262) = 0; *B(o + 235) = 0; *H(o + 22) = (uint16_t)y;
        call1(c, o, f_0887C614, 0x088F3634u);
        call1(c, o, f_08861848, 0x088F363Cu);
    } else if (S8(o + 77) == 0) call1(c, o, f_08861848, 0x088F3610u);
    leave(c, sv);
}

/* (fighter): an air special's start: in the air, without a choice made, with at least 48 of stun gauge
   (+366) and +536 clear, when the input check passes (0x0886C8F4): +81 = +564, state 1 / 0 / 8 / 0, +236
   = 9, facing the opponent, +235 = 1 and 0x0886D2EC: 1, else 0. A frame keeping s0, s1. */
SFA_FUNC(088F77C4, air_special_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 20) = s1; c->r[17] = 0;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if (S8(o + 386) == 0 && !(S16(o + 366) < 48) && *B(o + 536) == 0 && S8(o + 73) != 0) {
        call1(c, o, f_0886C8F4, 0x088F7814u);
        if (c->r[2] != 0) {
            *B(o + 81) = (uint8_t)S8(o + 564);
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            *B(o + 236) = 9; *B(o + 11) = (uint8_t)S8(o + 697); *B(o + 235) = 1;
            c->r[17] = 1;
            call1(c, o, f_0886D2EC, 0x088F7860u);
        }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (effect, script): once the script's frame count reaches +118: +38 / +14 cleared, +4 on, 0x089D7808,
   the follow step by +116 (0x089D8C40) and the script's next step 0x089CF674. A frame keeping s0, s1. */
SFA_FUNC(089CF5AC, effect_step_next) {
    gaddr e = ARG(0), sc = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t n = (int16_t)*B(e + 118);
    *W(c->r[29] + 20) = s1; c->r[17] = e;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = sc;
    if (!(S16(sc + 2) < n)) {
        *H(e + 38) = 0;
        *B(e + 4) = (uint8_t)(*B(e + 4) + 1);
        *B(e + 14) = 0;
        call1(c, e, f_089D7808, 0x089CF5FCu);
        uint32_t k = *B(e + 116);
        uint32_t back = S8(0x08E75910u + 114) == 9 ? 0x089CF654u : (*B(e + 117) == 0 ? 0x089CF640u : 0x089CF62Cu);
        call2(c, e, k, f_089D8C40, back);
        ARG(0) = e; ARG(1) = sc; c->r[31] = 0x089CF660u; f_089CF674(c);
    } else c->r[6] = 1;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08905438(Cpu *c); void f_088219B0(Cpu *c); void f_08821384(Cpu *c); void f_08821424(Cpu *c);
void f_088D304C(Cpu *c); void f_088D32C8(Cpu *c); void f_088D3278(Cpu *c); void f_088D35B8(Cpu *c);

/* (object): a held piece (+3 = 2, 0x08905438); else, unless paused, it slides along +96 and ends (+4 on)
   once 0x20 past its mark +112 the way it faces; 0x088219B0, 0x08821384 and drawn (0x08821424). */
SFA_FUNC(0890537C, slide_piece) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (*B(o + 3) == 2) call1(c, o, f_08905438, 0x08905428u);
    else {
        if (S8(0x092E1228u + 37) == 0) {
            *W(o + 16) += *W(o + 96);
            int16_t d = (int16_t)(S16(o + 18) - S16(o + 112));
            if (*B(o + 11) == 0) d = (int16_t)-d;
            if (!(d + 0x20 < 64)) *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
        }
        call1(c, o, f_088219B0, 0x08905408u);
        call1(c, o, f_08821384, 0x08905410u);
        call1(c, o, f_08821424, 0x08905418u);
    }
    leave(c, sv);
}

/* (fighter): an air super's start: 0x088D304C (with the part at +0x398) and the other checks (in the
   air, no choice made, not a light standing move in state 5, +536 clear, 0x088D32C8 and the input check
   0x0886C8F4 both 1); then 0x088D3278, +236 = 9, +235 = 1, +552 / +194 = 2, 0x088D35B8 and +78 from the
   stick (+764 bit 8: 1, bit 9: 2): 1, else 0. A frame keeping s0..s2. */
SFA_FUNC(089C0C08, air_super_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    ARG(1) = o + 0x398; c->r[31] = 0x089C0C28u; f_088D304C(c);
    c->r[18] = 1;
    uint32_t r = 0;
    int go = c->r[2] == 1 && S8(o + 73) != 0 && S8(o + 386) == 0;
    if (go && *B(o + 6) == 5 && S8(o + 193) == 0 && S8(o + 194) == 0) go = 0;
    if (go && *B(o + 536) != 0) go = 0;
    if (go) { call1(c, o, f_088D32C8, 0x089C0C88u); go = c->r[2] == 1; }
    if (go) { call1(c, o, f_0886C8F4, 0x089C0C98u); go = c->r[2] == 1; }
    if (go) {
        call1(c, o, f_088D3278, 0x089C0CA8u);
        *B(o + 236) = 9; c->r[17] = 2;
        *B(o + 235) = 1; *B(o + 552) = 2; *B(o + 194) = 2;
        call1(c, o, f_088D35B8, 0x089C0CC8u);
        uint32_t h = *H(o + 764);
        *B(o + 78) = 0;
        if (h & 0x100) *B(o + 78) = 1;
        if (h & 0x200) *B(o + 78) = 2;
        r = 1;
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0887DF88(Cpu *c); void f_0882196C(Cpu *c);

/* (piece): a piece's start (+4 on): kind 2 is held by its owner (+88): its side, script, step from the
   owner's +338, +12..+14 / +24 copied (+13 two lower), +9 = 2, timers cleared, then the held step;
   otherwise +73 cleared, +72 = 1, kind 0 a big one (+148 cleared, side 5, 0x0887DF88 0xF, set 0x80 in
   front) or the owner's (pose from 0x08A4C1EC); it slides at 1.0 along its facing, from +112 = x. A
   frame keeping s0, s1. */
SFA_FUNC(08905234, piece_start) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    gaddr own = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    uint32_t k = *B(o + 3);
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    if (k == 2) {
        *H(o + 86) = *H(own + 86); *W(o + 164) = *W(own + 164);
        *B(o + 9) = 2; *B(o + 7) = (uint8_t)S8(own + 338);
        *B(o + 12) = (uint8_t)S8(own + 12); *B(o + 13) = *B(own + 13);
        uint8_t b14 = *B(own + 14);
        *B(o + 15) = 0; *B(o + 14) = b14;
        *H(o + 24) = *H(own + 24);
        *H(o + 38) = 0; *H(o + 82) = 0;
        *B(o + 13) = (uint8_t)(*B(o + 13) - 2);
        *B(o + 78) = 0;
        call1(c, o, f_08905438, 0x08905368u);
    } else {
        *B(o + 73) = 0; *B(o + 72) = 1;
        int16_t off;
        if (k == 0) {
            *W(o + 148) = 0; *H(o + 86) = 5; off = 0x80;
            c->r[17] = 0x80;
            call2(c, o, 0xF, f_0887DF88, 0x08905294u);
        } else {
            *H(o + 86) = *H(own + 86); off = 0; c->r[17] = 0;
            *W(o + 164) = *W(own + 164);
            call2(c, o, *W(0x08A4C1ECu), f_0882196C, 0x089052C4u);
        }
        uint8_t f = *B(o + 11);
        int32_t x = S16(o + 18);
        *W(o + 96) = 0x10000;
        if (f == 0) { off = (int16_t)-off; *W(o + 96) = 0u - *W(o + 96); }
        c->r[17] = (uint32_t)(int32_t)off;
        *H(o + 18) = (uint16_t)(x + off);
        *H(o + 112) = *H(o + 18);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08821368(Cpu *c);

/* (fighter): step 4 and a backward slide (x -4.0, brake 0.125 the other way). */
SFA_FUNC(08968A1C, slide_back_4) {
    gaddr o = ARG(0);
    c->r[5] = 0xFFFC0000u; c->r[6] = 0x2000;
    *B(o + 7) = 4;
    *W(o + 96) = 0xFFFC0000u; *W(o + 104) = 0x2000; *W(o + 100) = 0; *W(o + 108) = 0;
}

/* (object): the next state (+4 on). */
SFA_FUNC(08987D2C, next_state_4) {
    gaddr o = ARG(0);
    c->r[5] = (uint32_t)*B(o + 4) + 1;
    *B(o + 4) = (uint8_t)c->r[5];
}

/* (fighter): a random strength (0x08821368's low two bits through 0x08A957F0). A 0x10 frame keeping s0. */
SFA_FUNC(0898C268, random_strength) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    c->r[31] = 0x0898C27Cu; f_08821368(c);
    c->r[5] = 0x08A957F0u;
    c->r[4] = *B(0x08A957F0u + (c->r[2] & 3));
    *B(o + 194) = (uint8_t)c->r[4];
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x10;
}


/* (fighter, distance): the cpu's random timings: +828 / +832 / +836 cleared; on three rolls in four,
   either fixed (+828 = 1, +836 = 0x46) when the opponent's +536 is set or the opponent is no nearer than
   the distance, or three random picks (0x08A762C0 / 0x08A762B8 / 0x08A762BC) into +836 / +828 / +832. A
   0x10 frame keeping s0, s1. */
SFA_FUNC(08943F04, cpu_random_timing) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x10;
    gaddr sp = c->r[29];
    *W(sp) = s0; c->r[16] = o;
    *W(sp + 4) = s1;
    *W(o + 836) = 0;
    int32_t d = (int16_t)ARG(1); c->r[17] = (uint32_t)d;
    *W(o + 832) = 0;
    *W(sp + 8) = ra;
    *W(o + 828) = 0;
    c->r[31] = 0x08943F30u; f_08821368(c);
    if ((c->r[2] & 3) != 0) {
        if (*B(*W(o + 92) + 536) != 0 || !(d < S16(o + 508))) {
            c->r[5] = 0x46;
            *W(o + 828) = 1; *W(o + 836) = 0x46;
        } else {
            c->r[31] = 0x08943F64u; f_08821368(c);
            c->r[5] = 0x08A762C0u; c->r[4] = (uint32_t)S8(0x08A762C0u + (c->r[2] & 3));
            *W(o + 836) = c->r[4];
            c->r[31] = 0x08943F80u; f_08821368(c);
            c->r[5] = 0x08A762B8u; c->r[4] = (uint32_t)S8(0x08A762B8u + (c->r[2] & 3));
            *W(o + 828) = c->r[4];
            c->r[31] = 0x08943F9Cu; f_08821368(c);
            c->r[5] = 0x08A762BCu; c->r[4] = (uint32_t)S8(0x08A762BCu + (c->r[2] & 3));
            *W(o + 832) = c->r[4];
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x10;
}


void f_0886CA2C(Cpu *c); void f_08837098(Cpu *c); void f_08943F04(Cpu *c); void f_08968A1C(Cpu *c);
void f_08987BCC(Cpu *c); void f_08987FA4(Cpu *c); void f_08987D68(Cpu *c); void f_08987D8C(Cpu *c);
void f_089880E4(Cpu *c); void f_08987D2C(Cpu *c);

/* A back-step start: +640 / +262 / step 1, the meter by 6, 0x08837098, backwards at speed, the move,
   and for the cpu its random timings (0x08943F04) with the distance. */
static void back_step(Cpu *c, uint32_t vx, uint32_t move, uint32_t dist, const uint32_t back[4]) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 640) = 1; *B(o + 262) = 1; *B(o + 7) = 1;
    call2(c, o, 6, f_0886CA2C, back[0]);
    call1(c, o, f_08837098, back[1]);
    *W(o + 96) = vx;
    call2(c, o, move, f_088616EC, back[2]);
    if (S8(o + 373) != 0) call2(c, o, dist, f_08943F04, back[3]);
    leave(c, sv);
}

/* (fighter): a back-step (x -1.5, move 0x2F, cpu distance 0x68). */
SFA_FUNC(08943DF4, back_step_2f) {
    static const uint32_t b[4] = { 0x08943E20u, 0x08943E28u, 0x08943E40u, 0x08943E54u };
    back_step(c, 0xFFFE8000u, 0x2F, 0x68, b);
}

/* (fighter): a back-step (x -1.0, move 0x30, cpu distance 0x98). */
SFA_FUNC(08943E98, back_step_30) {
    static const uint32_t b[4] = { 0x08943EC4u, 0x08943ECCu, 0x08943EE0u, 0x08943EF4u };
    back_step(c, 0xFFFF0000u, 0x30, 0x98, b);
}

/* (fighter): with +123 the back slide (0x08968A1C); else on +77 +7 on, +83 = 0xA, the rise stopped and +73
   = 1; ticked. */
SFA_FUNC(089687E8, hop_or_slide) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(o + 123);
    *W(c->r[29] + 16) = ra;
    if (c->r[5] != 0) { c->r[31] = 0x08968804u; f_08968A1C(c); }
    else {
        c->r[5] = (uint32_t)S8(o + 77);
        if (c->r[5] != 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 0x52 + 1) = 0xA;
            c->r[5] = 1;
            *W(o + 100) = 0; *B(o + 73) = 1;
        }
        c->r[31] = 0x08968844u; f_08861848(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* An arm piece's frame (piece, by side 0 / 1): +1 cleared; while its side of the owner's arm (+0x33C)
   is out and the owner's timer word is set, it is cleared and the piece's +0x66 counts down to the
   reset (first). Gone (+4 on) once the owner's +338 leaves the piece's +3; with no hit (+296's +10..11)
   its +0x64 is cleared; else the arm's pose from 0x08A93D8C (0x08987D68, +1 = 1 while out) and the
   change (last) when it differs from +0x64. A 0x30 frame keeping s0..s4. */
static void arm_piece(Cpu *c, uint32_t side, uint32_t timer, void (*first)(Cpu *), void (*last)(Cpu *),
                      const uint32_t back[4]) {
    gaddr x = ARG(0);
    uint32_t sv[5], ra = c->r[31];
    for (int i = 0; i < 5; i++) sv[i] = c->r[16 + i];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = sv[3];
    gaddr own = *W(x + 88); c->r[19] = own;
    *W(sp + 24) = sv[2];
    *B(x + 1) = 0;
    gaddr arm = own + 0x33C; c->r[18] = arm;
    int32_t out = S8(arm + side);
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 32) = sv[4]; *W(sp + 36) = ra;
    c->r[16] = x;
    if (out != 0 && *W(own + timer) != 0) {
        *W(own + timer) = 0;
        gaddr q = x + 0x60; c->r[17] = q;
        int32_t k = S8(q + 6);
        if (k == 0) { ARG(0) = x; ARG(1) = own; c->r[31] = back[0]; first(c); }
        else *B(q + 6) = (uint8_t)(k - 1);
    }
    if ((int32_t)*B(x + 3) != S8(own + 338)) { ARG(0) = x; c->r[31] = back[1]; f_08987D2C(c); }
    else {
        gaddr h = *W(own + 296);
        uint32_t v = ((uint32_t)*B(h + 10) << 8) | *B(h + 11);
        gaddr q = x + 0x60; c->r[17] = q;
        if (v == 0) *H(q + 4) = 0;
        else {
            c->r[20] = arm;
            uint32_t want = *H(0x08A93D8Cu + v * 4 + (side ? 0 : 2));   /* side 1's pose comes first */
            c->r[18] = want;
            ARG(0) = x; ARG(1) = own; c->r[31] = back[2]; f_08987D68(c);
            if (S8(arm + side) != 0) *B(x + 1) = 1;
            if (*H(q + 4) != want) { ARG(0) = x; ARG(1) = want; c->r[31] = back[3]; last(c); }
        }
    }
    for (int i = 0; i < 5; i++) c->r[16 + i] = sv[i];
    c->r[31] = ra; c->r[29] += 0x30;
}

/* (piece): the arm piece on side 1 (timer +820). */
SFA_FUNC(08987AB0, arm_piece_1) {
    static const uint32_t b[4] = { 0x08987B10u, 0x08987B38u, 0x08987B88u, 0x08987BACu };
    arm_piece(c, 1, 820, f_08987BCC, f_08987D8C, b);
}

/* (piece): the arm piece on side 0 (timer +824). */
SFA_FUNC(08987E88, arm_piece_0) {
    static const uint32_t b[4] = { 0x08987EE8u, 0x08987F10u, 0x08987F60u, 0x08987F84u };
    arm_piece(c, 0, 824, f_08987FA4, f_089880E4, b);
}


void f_0896A34C(Cpu *c); void f_08877258(Cpu *c); void f_0898A874(Cpu *c); void f_0886DB94(Cpu *c);

/* (fighter): with +123 the back slide; else meeting a wall on its facing side (+238) +7 by 2 and
   0x0896A34C; with +648 the back slide; else +83 counts down (+7 by 2 at 0) and 0x0896A34C. */
SFA_FUNC(089688CC, rush_wall) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(o + 123);
    *W(c->r[29] + 16) = ra;
    if (c->r[5] != 0) { c->r[31] = 0x089688E8u; f_08968A1C(c); }
    else if (S8(o + 238) & (int32_t)(*B(o + 11) + 1)) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 2);
        c->r[31] = 0x08968918u; f_0896A34C(c);
    } else if (S8(o + 648) != 0) { c->r[31] = 0x08968934u; f_08968A1C(c); }
    else {
        int8_t t = (int8_t)(S8(o + 0x52 + 1) - 1);
        *B(o + 0x52 + 1) = (uint8_t)t;
        if (t == 0) *B(o + 7) = (uint8_t)(*B(o + 7) + 2);
        c->r[31] = 0x08968970u; f_0896A34C(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a charge wait until +828: the cpu every +82 frames (a random 0..15 next) tries the
   pattern at 0x08A952D4 (0x08877258) and on a hit sets +828 = 2, a random strength and 0x0898A874; a
   player sets +828 = 2 on a punch (+760 bits 4..6), then 0x0898A874 and 0x0886DB94. */
SFA_FUNC(0898C1B4, charge_wait) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (*W(o + 828) == 0) {
        if (S8(o + 373) != 0) {
            int16_t h = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)h;
            if (h == 0) {
                c->r[31] = 0x0898C1FCu; f_08821368(c);
                *H(o + 82) = (uint16_t)(c->r[2] & 0xF);
                call2(c, o, 0x08A952D4u, f_08877258, 0x0898C214u);
                if (c->r[2] != 0) {
                    *W(o + 828) = 2;
                    call1(c, o, f_0898C268, 0x0898C228u);
                    call1(c, o, f_0898A874, 0x0898C230u);
                }
            }
        } else {
            if (*H(o + 760) & 0x70) *W(o + 828) = 2;
            call1(c, o, f_0898A874, 0x0898C250u);
            call1(c, o, f_0886DB94, 0x0898C258u);
        }
    }
    leave(c, sv);
}


void f_0886DBE8(Cpu *c);

/* (fighter): a charge move's choice until +840 is set: the cpu every +82 frames (then a random 0..15)
   tries the pattern at 0x08A952D4 and on a hit picks +840 from 0x08A956C8 by a random roll, a random
   strength and 0x0898A874; a player picks it from the buttons: punches by +290's direction (1: 4, 6: 6,
   2: 2, else 1) with 0x0898A874 and 0x0886DB94, kicks (1: 5, bit 3: 7, bit 2: 8, else 3) with
   0x0886DBE8. */
SFA_FUNC(0898ACA8, charge_choice) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    if (*W(o + 840) == 0) {
        if (S8(o + 373) != 0) {
            int16_t h = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)h;
            if (h == 0) {
                c->r[31] = 0x0898ACF0u; f_08821368(c);
                *H(o + 82) = (uint16_t)(c->r[2] & 0xF);
                call2(c, o, 0x08A952D4u, f_08877258, 0x0898AD08u);
                if (c->r[2] != 0) {
                    c->r[31] = 0x0898AD18u; f_08821368(c);
                    c->r[5] = *B(0x08A956C8u + (c->r[2] & 0x1F));
                    *W(o + 840) = c->r[5];
                    call1(c, o, f_0898C268, 0x0898AD38u);
                    call1(c, o, f_0898A874, 0x0898AD40u);
                }
            }
        } else {
            uint32_t h = *H(o + 760);
            if (h & 0x3F0) {
                uint32_t s = *H(o + 290), d = s & 0xF;
                if (h & 0x70) {
                    *W(o + 840) = d == 1 ? 4 : d == 6 ? 6 : d == 2 ? 2 : 1;
                    call1(c, o, f_0898A874, 0x0898ADA4u);
                    call1(c, o, f_0886DB94, 0x0898ADACu);
                } else {
                    *W(o + 840) = d == 1 ? 5 : (s & 8) ? 7 : (s & 4) ? 8 : 3;
                    call1(c, o, f_0886DBE8, 0x0898ADFCu);
                }
            }
        }
    }
    leave(c, sv);
}


void f_0898AE0C(Cpu *c); void f_0898B0C8(Cpu *c); void f_0898B0F0(Cpu *c); void f_0898C640(Cpu *c);

/* (fighter): the charge choice, ticked; at the animation's end 0x0886469C, else 0x0898AE0C on +77 and
   0x0898B0C8. */
SFA_FUNC(0898ABF4, charge_step) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_0898ACA8, 0x0898AC08u);
    call1(c, o, f_08861848, 0x0898AC10u);
    if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0898AC28u);
    else {
        if (S8(o + 77) != 0) call1(c, o, f_0898AE0C, 0x0898AC40u);
        call1(c, o, f_0898B0C8, 0x0898AC48u);
    }
    leave(c, sv);
}

/* An arm piece's start: +4 on, its timer +0x66, the owner's (+88) +24 / +338 / +12..+14 copied, +0x64 /
   +0x60 cleared, the pose change with 0 and its first frame. */
static void arm_start(Cpu *c, uint32_t timer, void (*pose)(Cpu *), void (*frame)(Cpu *), const uint32_t back[2]) {
    gaddr x = ARG(0); uint32_t sv[2]; enter(c, x, sv);
    gaddr own = *W(x + 88);
    *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
    gaddr q = x + 0x60; c->r[6] = q;
    *B(q + 6) = (uint8_t)timer;
    *H(x + 24) = *H(own + 24);
    *B(x + 3) = (uint8_t)S8(own + 338);
    *B(x + 12) = (uint8_t)S8(own + 12);
    *B(x + 13) = *B(own + 13);
    *B(x + 14) = *B(own + 14);
    *H(q + 4) = 0; *W(q) = 0;
    call2(c, x, 0, pose, back[0]);
    call1(c, x, frame, back[1]);
    leave(c, sv);
}

/* (piece): the arm piece's start on side 1 (timer 8). */
SFA_FUNC(08987A30, arm_start_1) {
    static const uint32_t b[2] = { 0x08987A98u, 0x08987AA0u };
    arm_start(c, 8, f_08987D8C, f_08987AB0, b);
}

/* (piece): the arm piece's start on side 0 (timer 0x20). */
SFA_FUNC(08987E08, arm_start_0) {
    static const uint32_t b[2] = { 0x08987E70u, 0x08987E78u };
    arm_start(c, 0x20, f_089880E4, f_08987E88, b);
}

/* (fighter): a rush's start: +7 on, +262 = 1, the meter by 6, 0x08837098, +123 / +20 cleared, at the
   speed by strength from 0x08A8B8BC, move strength + 8 and its first frame (0x089687E8). */
SFA_FUNC(08968758, rush_start) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    *B(o + 262) = 1;
    call2(c, o, 6, f_0886CA2C, 0x08968788u);
    call1(c, o, f_08837098, 0x08968790u);
    *B(o + 123) = 0;
    int32_t s = S8(o + 194);
    *H(o + 20) = 0; *W(o + 100) = 0; *W(o + 104) = 0; *W(o + 108) = 0;
    *W(o + 96) = *W(0x08A8B8BCu + ((uint32_t)s << 2));
    call2(c, o, (uint32_t)(s + 8) & 0xFFFF, f_088616EC, 0x089687D0u);
    call1(c, o, f_089687E8, 0x089687D8u);
    leave(c, sv);
}

/* (fighter): the charge wait and 0x0898B0F0; once charged (+828) and +77 set (bit 7 only with +824) the
   release: +7 on, +262 = 1, 0x0898C640, +828 cleared and move strength + 0x2D; else 0x0886469C at the
   animation's end, or ticked. */
SFA_FUNC(0898C098, charge_release) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    call1(c, o, f_0898C1B4, 0x0898C0ACu);
    call1(c, o, f_0898B0F0, 0x0898C0B4u);
    int go = 0;
    if (*W(o + 828) != 0) {
        int32_t k = S8(o + 77);
        if (k != 0) go = !(k & 0x80) || *W(o + 824) != 0;
    }
    if (go) {
        *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
        *B(o + 262) = 1;
        call1(c, o, f_0898C640, 0x0898C118u);
        int32_t s = S8(o + 194);
        *W(o + 828) = 0;
        call2(c, o, (uint32_t)(s + 0x2D) & 0xFFFF, f_088616EC, 0x0898C130u);
    } else if (S8(o + 75) & 0x80) call1(c, o, f_0886469C, 0x0898C0F8u);
    else call1(c, o, f_08861848, 0x0898C148u);
    leave(c, sv);
}
