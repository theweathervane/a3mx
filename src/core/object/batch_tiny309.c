/* More small ones whose callees are all done: moves' starts behind their checks, a few steps, the
   objects they make, and a link message's dispatch. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_08823220(Cpu *c); void f_08880A04(Cpu *c); void f_0887DF88(Cpu *c); void f_08880AB8(Cpu *c);
void f_088219B0(Cpu *c); void f_088215C4(Cpu *c); void f_088415B8(Cpu *c); void f_08842690(Cpu *c);
void f_088E426C(Cpu *c); void f_08822EAC(Cpu *c); void f_0886BF84(Cpu *c); void f_08828C04(Cpu *c);
void f_0886BB1C(Cpu *c); void f_08861848(Cpu *c); void f_0886469C(Cpu *c); void f_088616EC(Cpu *c);
void f_08921850(Cpu *c); void f_08821368(Cpu *c); void f_089218B8(Cpu *c); void f_08921940(Cpu *c);
void f_0886C6EC(Cpu *c); void f_08935708(Cpu *c); void f_0886C2B4(Cpu *c); void f_0893D8AC(Cpu *c);
void f_0886C9F0(Cpu *c); void f_0886C7E4(Cpu *c); void f_0886DD68(Cpu *c); void f_08943448(Cpu *c);
void f_0887C408(Cpu *c); void f_0896F514(Cpu *c); void f_089844E0(Cpu *c);

static void call0(Cpu *c, void (*fn)(Cpu *), uint32_t back) { c->r[31] = back; fn(c); }
static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
static void enter2(Cpu *c, uint32_t sv[3]) {
    sv[0] = c->r[16]; sv[1] = c->r[17]; sv[2] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1]; *W(c->r[29] + 24) = sv[2];
}
static void leave2(Cpu *c, const uint32_t sv[3]) {
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[31] = sv[2]; c->r[29] += 0x20;
}

/* (object): a mark beside its owner (+88): shown only in game mode 2 with GAME + 4 set and the byte
   +58 of the owner's record (0x354 each from 0x09524AF0, by +78) set, 0x2D to the right of the
   owner and 15 below; never while the byte at 0x09525881 is set. With GAME + 4 clear 0x08823220. */
SFA_FUNC(08847E80, owner_mark) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t idx = S8(o + 78);
    gaddr own = *W(o + 88);
    *B(o + 1) = 0;
    gaddr rec = 0x09524AF0u + (uint32_t)(idx * 0x354);
    *W(c->r[29] + 16) = ra;
    if (*H(GAME + 6) == 2 && *H(GAME + 4) != 0 && S8(rec + 58) != 0) {
        *B(o + 1) = 1;
        *H(o + 18) = (uint16_t)(S16(own + 18) + 0x2D);
        *H(o + 22) = (uint16_t)(S16(own + 22) - 15);
    }
    int32_t off = S8(0x09525881u);
    if (off != 0) *B(o + 1) = 0;
    uint32_t on = *H(GAME + 4);
    c->r[5] = on; c->r[6] = (uint32_t)off; c->r[7] = own;
    if (on == 0) call0(c, f_08823220, 0x08847F20u);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): state 2 once 0x08880A04 says so. Before that, the first time (+5) the cue from the
   byte table 0x08A24A00 by +82 plus a quarter of +3 (0x0887DF88), afterwards 0x088219B0; then
   0x08880AB8 and 0x088215C4. */
SFA_FUNC(08880898, object_880898) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_08880A04, 0x088808ACu);
    if (c->r[2] != 0) *B(o + 4) = 2;
    else {
        if (*B(o + 5) == 0) {
            int32_t at = S16(o + 82);
            *B(o + 5) = 1;
            uint32_t cue = (uint32_t)(S8(0x08A24A00u + (uint32_t)at) + (*B(o + 3) >> 2)) & 0xFFFF;
            call2(c, o, cue, f_0887DF88, 0x08880900u);
            call1(c, o, f_08880AB8, 0x08880908u);
        } else {
            call1(c, o, f_088219B0, 0x08880918u);
            call1(c, o, f_08880AB8, 0x08880920u);
        }
        call1(c, o, f_088215C4, 0x08880928u);
    }
    leave(c, sv);
}

/* Of the 32 objects from 0x09512770 (0xC0 each) every live one of kind 0x7E is passed to
   0x088415B8, and of the 32 from 0x092E2B88 (0x94 each) every live one (+90) of kind (+91) 0x13 to
   0x08842690. A frame keeping s0..s2. */
SFA_FUNC(088CC690, clear_kinds_7e_13) {
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2; *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 28) = ra;
    c->r[17] = 0x7E;
    for (uint32_t i = 0; i < 32; i++) {
        gaddr at = 0x09512770u + i * 0xC0;
        c->r[16] = i; c->r[18] = at;
        if (*B(at) != 0 && *B(at + 2) == 0x7E) call1(c, at, f_088415B8, 0x088CC6D4u);
    }
    c->r[17] = 0x13;
    for (uint32_t i = 0; i < 32; i++) {
        gaddr at = 0x092E2B88u + i * 0x94;
        c->r[16] = i; c->r[18] = at;
        if (*B(at + 90) != 0 && *B(at + 91) == 0x13) call1(c, at, f_08842690, 0x088CC714u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, message, body): a link message that starts with 0xAC is passed on by the low four bits of its
   second byte: when its word at +2 is what 0x088E426C makes of the body with the word from the
   table 0x08A3EC50, the handler from the table 0x08A3EC90, if there is one, gets a and the body.
   A 0x30 frame keeping s0..s3. */
SFA_FUNC(088E428C, link_message_pass) {
    gaddr a = ARG(0), msg = ARG(1), body = ARG(2);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t kind = *B(msg + 1) & 0xF;
    *W(sp + 20) = sv[1]; c->r[17] = a;
    *W(sp + 24) = sv[2]; c->r[18] = msg;
    *W(sp + 16) = sv[0]; *W(sp + 28) = sv[3]; *W(sp + 32) = sv[4];
    c->r[16] = body;
    if (*B(msg) == 0xAC) {
        uint32_t word = *H(msg + 2);
        c->r[19] = word; c->r[18] = kind << 2;
        call2(c, body, *W(0x08A3EC50u + (kind << 2)), f_088E426C, 0x088E42E4u);
        if (word == c->r[2]) {
            uint32_t fn = *W(0x08A3EC90u + (kind << 2));
            c->r[18] = fn;
            if (fn != 0) {
                ARG(0) = a; ARG(1) = body; c->r[31] = 0x088E4308u;
                rt_call_indirect(c, fn);
            }
        }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (owner): a pair of objects (0x08822EAC 8 / 1) at its height, 0x20 to either side, the second
   facing the other way; both take its +14 and it as their owner. */
SFA_FUNC(088F1E74, make_pair_f1e74) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    for (uint32_t i = 0; i < 2; i++) {
        call2(c, 8, 1, f_08822EAC, i ? 0x088F1ED4u : 0x088F1E90u);
        gaddr n = c->r[2];
        if (n == 0) break;
        *B(n + 12) = 0; *B(n + 13) = 0;
        *B(n + 11) = (uint8_t)i;
        *B(n + 14) = *B(o + 14);
        *H(n + 18) = (uint16_t)(S16(o + 18) + (i ? -0x20 : 0x20));
        *W(n + 88) = o;
        *H(n + 22) = (uint16_t)S16(o + 22);
    }
    leave(c, sv);
}

/* (fighter): with +251 or +254 set and 0x0886BF84 allowing, a move from state 1 / 0 / 7 / 0, facing
   as +697 says (some set +235 too), and 0x08828C04. Returns whether. */
static void move_if_flag(Cpu *c, uint8_t move, int with235, uint32_t back0, uint32_t back1) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[17] = 0; c->r[16] = o;
    if (S8(o + 251) == 0 && S8(o + 254) == 0) { c->r[2] = 0; leave2(c, sv); return; }
    call1(c, o, f_0886BF84, back0);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = move;
        int8_t facing = (int8_t)S8(o + 697);
        if (with235) *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)facing;
        c->r[5] = move == 9 ? 1u : (uint32_t)(int32_t)facing;
        c->r[17] = 1;
        call1(c, o, f_08828C04, back1);
    }
    c->r[2] = c->r[17];
    leave2(c, sv);
}
SFA_FUNC(088F2C48, move7_if_flag_f2c48) { move_if_flag(c, 7, 0, 0x088F2C7Cu, 0x088F2CD4u); }
SFA_FUNC(088F75E0, move12_if_flag) { move_if_flag(c, 0xC, 0, 0x088F7614u, 0x088F766Cu); }
SFA_FUNC(0890203C, move9_if_flag) { move_if_flag(c, 9, 1, 0x08902070u, 0x089020D0u); }
SFA_FUNC(08909D28, move7_if_flag_9d28) { move_if_flag(c, 7, 1, 0x08909D5Cu, 0x08909DBCu); }

/* (fighter): ticked until 0x0886BB1C says so; then the strength's bit is set at +0x334, +567 = -1,
   the step back to 0 and, unless +251 is set, 0x17 forward; ticked. */
SFA_FUNC(0890F2D4, roll_again) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_0886BB1C, 0x0890F2E8u);
    if (c->r[2] == 0) call1(c, o, f_08861848, 0x0890F2F8u);
    else {
        *B(o + 0x334) = (uint8_t)(S8(o + 0x334) | (int32_t)(1u << (S8(o + 194) & 31)));
        if (S8(o + 251) != 0) {
            *B(o + 567) = 0xFF; *B(o + 7) = 0;
            call1(c, o, f_08861848, 0x0890F334u);
        } else {
            int32_t d = *B(o + 11) != 0 ? 0x17 : 0xFFE8;
            int32_t x = S16(o + 18);
            *B(o + 567) = 0xFF;
            *H(o + 18) = (uint16_t)(x + d);
            *B(o + 7) = 0;
            call1(c, o, f_08861848, 0x0890F368u);
        }
    }
    leave(c, sv);
}

/* (fighter): a move's repeat: until the count +0x338 is 2 it moves on by its speed, slowing by hand.
   Ticked to the animation's end; then the count goes up: at 3 0x0886469C, else step 1 with the
   animation 0x2E + strength. */
SFA_FUNC(089166B0, repeat_step_166b0) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr p = o + 0x334;
    c->r[5] = p;
    *W(c->r[29] + 16) = ra;
    if (S8(p + 4) != 2) {
        uint32_t x = *W(o + 16), v = *W(o + 96), a = *W(o + 104);
        *W(o + 16) = x + v;
        *W(o + 96) = v + a;
    }
    if (!(S8(o + 75) & 0x80)) call0(c, f_08861848, 0x08916700u);
    else {
        int32_t n = S8(p + 4) + 1;
        *B(p + 4) = (uint8_t)n;
        c->r[5] = (uint32_t)(int32_t)(int8_t)n; c->r[6] = (uint32_t)n; c->r[7] = 3;
        if ((int8_t)n == 3) call0(c, f_0886469C, 0x08916730u);
        else {
            int32_t strength = S8(o + 194);
            *B(o + 7) = 1;
            c->r[6] = 1;
            call2(c, o, (uint32_t)(strength + 0x2E) & 0xFFFF, f_088616EC, 0x08916750u);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): 0x08921940 once 0x08921850 says so. Else, on its way down, one time in sixteen it turns
   to one of three others (0x089218B8 with 0 to 2; a fourth draw does nothing); otherwise ticked. */
SFA_FUNC(089216B4, object_9216b4) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    call0(c, f_08921850, 0x089216C8u);
    if (c->r[2] != 0) call1(c, o, f_08921940, 0x08921738u);
    else {
        int turned = 0;
        if ((int32_t)*W(o + 100) < 0) {
            call1(c, *W(o + 100), f_08821368, 0x089216E4u);
            if ((c->r[2] & 0xF) == 0) {
                call1(c, 0, f_08821368, 0x089216F8u);
                uint32_t v = c->r[2] & 3;
                if (v != 3) { call2(c, o, v, f_089218B8, 0x08921720u); turned = 1; }
            }
        }
        if (!turned) call1(c, o, f_08861848, 0x08921710u);
    }
    leave(c, sv);
}

/* (fighter): with the choice 0, a full meter (0x90) and 0x0886C6EC allowing: the move 0xE from
   state 1 / 0 / 8 / 2 at strength 2, facing as +697 says, and 0x08828C04. Returns whether. */
SFA_FUNC(0892A634, super_e_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok = S8(o + 386) == 0 && !(S16(o + 366) < 144);
    if (ok) {
        call1(c, o, f_0886C6EC, 0x0892A664u);
        ok = c->r[2] != 0;
    }
    if (ok) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 2;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 236) = 0xE;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 552) = 2; *B(o + 194) = 2; *B(o + 196) = 0; *B(o + 235) = 0;
        c->r[5] = (uint32_t)(int32_t)facing;
        call1(c, o, f_08828C04, 0x0892A6B0u);
    }
    c->r[2] = (uint32_t)ok;
    leave(c, sv);
}

/* (fighter): for the choice 0 the move 9 / 1 from state 1 / 0 / 7 / 0, facing as +697 says,
   +196..+199 cleared, its +127 cleared and the opponent's 0x18, +573 = 0x1C, +777 set; for the
   others 0x08935708. */
SFA_FUNC(0893567C, move9_start_3567c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t choice = S8(o + 386);
    *W(c->r[29] + 16) = ra;
    c->r[5] = 9;
    if (choice != 0) call0(c, f_08935708, 0x089356FCu);
    else {
        *B(o + 236) = 9;
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 235) = 1;
        *B(o + 11) = (uint8_t)facing;
        *B(o + 196) = 0; *B(o + 197) = 0; *B(o + 198) = 0; *B(o + 199) = 0;
        gaddr other = *W(o + 92);
        *B(o + 127) = 0;
        *B(other + 127) = 0x18;
        *B(o + 573) = 0x1C;
        *B(o + 777) = 1;
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): 0x0893D8AC when allowed: on the ground by 0x0886BF84; in the air by 0x0886C2B4, and
   never for the choice -1 or with +0x3B8 set. Returns whether. */
SFA_FUNC(0893D80C, start_3d8ac_if) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    int ok;
    if (S8(o + 73) == 0) {
        call1(c, o, f_0886BF84, 0x0893D874u);
        ok = c->r[2] != 0;
        if (ok) call1(c, o, f_0893D8AC, 0x0893D884u);
    } else {
        ok = S8(o + 386) != -1 && S8(o + 0x3B8) == 0;
        if (ok) {
            call1(c, o, f_0886C2B4, 0x0893D848u);
            ok = c->r[2] != 0;
        }
        if (ok) call1(c, o, f_0893D8AC, 0x0893D858u);
    }
    c->r[2] = (uint32_t)ok;
    leave(c, sv);
}

/* (fighter): never with +634 set. In the air what 0x08943448 says. On the ground, when both
   0x0886C9F0 and 0x0886C7E4 allow: the move 0 from state 1 / 0 / 8 / 0, facing as +697 says, and
   0x0886DD68. Returns whether. */
SFA_FUNC(0894339C, super_0_start) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (S8(o + 634) != 0) c->r[2] = 0;
    else if (S8(o + 73) != 0) call1(c, o, f_08943448, 0x08943438u);
    else {
        call1(c, o, f_0886C9F0, 0x089433C8u);
        int ok = c->r[2] != 0;
        if (ok) {
            call1(c, o, f_0886C7E4, 0x089433D8u);
            ok = c->r[2] != 0;
        }
        if (ok) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8;
            int8_t facing = (int8_t)S8(o + 697);
            *B(o + 7) = 0; *B(o + 236) = 0;
            *B(o + 11) = (uint8_t)facing;
            call1(c, o, f_0886DD68, 0x08943408u);
        }
        c->r[2] = (uint32_t)ok;
    }
    leave(c, sv);
}

/* (fighter): step 0x13: it turns round and appears 0x160 behind where it was and 0x40 up, in the
   air, with +83 = 0x48, +656 = 0x1E, the cue 0x210 and 0x0896F514. */
SFA_FUNC(0896B800, reappear_behind) {
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    uint8_t facing = *B(o + 11) ^ 1;
    *B(o + 7) = 0x13;
    *B(o + 11) = facing;
    *B(o + 73) = 1;
    int32_t x = S16(o + 18);
    *H(o + 18) = (uint16_t)(facing != 0 ? x - 0x160 : x + 0x160);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x40);
    *B(o + 83) = 0x48;
    *B(o + 656) = 0x1E;
    call3(c, o, (uint32_t)(int32_t)S8(o + 337), 0x210, f_0887C408, 0x0896B880u);
    call1(c, o, f_0896F514, 0x0896B888u);
    leave(c, sv);
}

/* (owner, vx, vy, time): an object (0x08822EAC 0x1C / 4) of kind 0x1C / 4 for the owner, with its
   +337 and +164, the speeds (vx's bits turned over when the owner faces right) and the time. A 0x30
   frame keeping s0..s3. */
SFA_FUNC(0897B4FC, make_object_1c_4) {
    gaddr o = ARG(0);
    uint32_t vx = ARG(1), vy = ARG(2), time = ARG(3);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (uint32_t i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 32) = sv[4];
    c->r[19] = o; c->r[18] = vx; c->r[16] = time; c->r[17] = vy;
    call2(c, 0x1C, 4, f_08822EAC, 0x0897B530u);
    gaddr n = c->r[2];
    if (n != 0) {
        *B(n) = 1; *B(n + 1) = 0; *B(n + 2) = 0x1C; *B(n + 3) = 4;
        *W(n + 88) = o;
        *H(n + 86) = (uint16_t)S8(o + 337);
        *W(n + 164) = *W(o + 164);
        if (*B(o + 11) != 0) vx = ~vx;
        *W(n + 96) = vx; *W(n + 100) = vy;
        *H(n + 82) = (uint16_t)time;
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}

/* (fighter): for the choice 0 the move 6 / 1 from state 1 / 0 / 7 / 0 at strength 2, +696 cleared,
   facing as +697 says, its +127 cleared and the opponent's 0x1A with +573 = 0x1E, the animation 7
   and +777 set; for the others 0x089844E0. */
SFA_FUNC(08984440, move6_start_84440) {
    gaddr o = ARG(0);
    uint32_t sv[3];
    enter2(c, sv);
    c->r[16] = o;
    if (S8(o + 386) != 0) call1(c, o, f_089844E0, 0x089844CCu);
    else {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 236) = 6;
        c->r[17] = 1;
        *B(o + 696) = 0;
        *B(o + 235) = 1;
        int8_t facing = (int8_t)S8(o + 697);
        *B(o + 194) = 2;
        *B(o + 127) = 0;
        gaddr other = *W(o + 92);
        *B(o + 11) = (uint8_t)facing;
        *B(other + 127) = 0x1A;
        *B(other + 573) = 0x1E;
        c->r[6] = 0x1E;
        call2(c, o, 7, f_088616EC, 0x089844BCu);
        *B(o + 777) = 1;
    }
    leave2(c, sv);
}
