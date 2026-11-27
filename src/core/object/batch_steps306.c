/* What the last batches set free in turn: starts for a following piece, a move's object and the side
   mark, the taunt's wait, the block's first step, a chooser by the animation's flags, and the link's
   entry for an address. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0890D2E0(Cpu *c); void f_089078C0(Cpu *c); void f_08907914(Cpu *c); void f_08907700(Cpu *c);
void f_0885D3F8(Cpu *c); void f_0887C408(Cpu *c); void f_08861724(Cpu *c); void f_0885D5D4(Cpu *c);
void f_089F33E4(Cpu *c); void f_088DCC54(Cpu *c); void f_08876998(Cpu *c); void f_0887684C(Cpu *c);
void f_0891F2B0(Cpu *c); void f_088A0594(Cpu *c); void f_088A0588(Cpu *c); void f_088A0104(Cpu *c);
void f_0889FFF4(Cpu *c); void f_0885D164(Cpu *c); void f_0886E3CC(Cpu *c); void f_0885CF5C(Cpu *c);
void f_08863F80(Cpu *c); void f_08861848(Cpu *c); void f_088619EC(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
/* A 0x20 frame with only the return address, at +16. */
static uint32_t enter0(Cpu *c) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    return ra;
}
static void leave0(Cpu *c, uint32_t ra) { c->r[31] = ra; c->r[29] += 0x20; }

/* (piece): a following piece's start: the next state, its owner's (+88) +24, character (into +3) and
   +12..+14, +78 cleared, and its first frame (0x0890D2E0). */
SFA_FUNC(0890D28C, piece_start_53df0) {
    gaddr x = ARG(0), own = *W(x + 88);
    *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
    *H(x + 24) = *H(own + 24);
    *B(x + 3) = (uint8_t)S8(own + 338);
    *B(x + 12) = (uint8_t)S8(own + 12);
    *B(x + 13) = *B(own + 13);
    uint8_t b14 = *B(own + 14);
    *B(x + 78) = 0;
    uint32_t ra = enter0(c);
    *B(x + 14) = b14;
    c->r[6] = own;
    c->r[31] = 0x0890D2D4u; f_0890D2E0(c);
    leave0(c, ra);
}

/* (fighter): by the animation's flags: at its end (+75) 0x089078C0; on its second flag (+76) the
   follow-up by button (0x08907914); else 0x08907700. */
SFA_FUNC(089076A8, step_by_anim_flags) {
    gaddr o = ARG(0);
    int32_t end = S8(o + 75);
    uint32_t ra = enter0(c);
    if (end < 0) { c->r[31] = 0x089076C4u; f_089078C0(c); }
    else if (S8(o + 76) < 0) { c->r[31] = 0x089076E0u; f_08907914(c); }
    else { c->r[31] = 0x089076F4u; f_08907700(c); }
    leave0(c, ra);
}

/* (fighter): the block's first step: the next step and its start (0x0885D3F8); with something stored
   (+816) step 3 instead, the cue 0x205, the animation 0x2B (0x2C crouching) and the wait for the store
   (0x0885D5D4). */
SFA_FUNC(0885D36C, block_first) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    uint8_t step = *B(o + 7);
    uint32_t store = *W(o + 816);
    *B(o + 7) = (uint8_t)(step + 1);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (store == 0) call1(c, o, f_0885D3F8, 0x0885D398u);
    else {
        int8_t side = S8(o + 337);
        *B(o + 7) = 3;
        call3(c, o, (uint32_t)(int32_t)side, 0x205, f_0887C408, 0x0885D3B8u);
        if (S8(o + 696) != 0) call2(c, o, 0x2C, f_08861724, 0x0885D3D0u);
        else call2(c, o, 0x2B, f_08861724, 0x0885D3E0u);
        call1(c, o, f_0885D5D4, 0x0885D3E8u);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (record): the link's entry for a record's address (its first six bytes, compared by 0x089F33E4 with
   each entry's +4 down the list from 0x09523908): the one found, or a new one added after the last
   (0x088DCC54). A frame keeping s0..s2. */
SFA_FUNC(088DCE6C, net_entry_for) {
    gaddr rec = ARG(0);
    const gaddr head = 0x09523820u;
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; c->r[18] = head;
    *W(c->r[29] + 20) = s1;
    gaddr e = *W(head + 232);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 28) = ra;
    c->r[16] = rec;
    int found = 0;
    while (e != 0) {
        c->r[17] = e;
        call3(c, rec, e + 4, 6, f_089F33E4, 0x088DCEA4u);
        if (c->r[2] == 0) { found = 1; break; }
        gaddr next = *W(e);
        if (next == 0) break;
        e = next;
    }
    c->r[17] = e;
    if (found) c->r[2] = e;
    else call3(c, head, e, rec, f_088DCC54, 0x088DCEF4u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): a move's object's start: the next state, its owner's (+88) +24, its own tables (+128,
   +200, +352, +356) and 0x08876998; no vitality, on the ground at the owner's place and facing with
   its +292; 0x0887684C with its kind (+346) + 8, and its first frame (0x0891F2B0). A frame keeping
   s0, s1. */
SFA_FUNC(0891CF54, move_object_start) {
    gaddr x = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = x;
    uint8_t st = *B(x + 4);
    *W(c->r[29] + 20) = s1;
    gaddr own = *W(x + 88);
    c->r[17] = own;
    *B(x + 4) = (uint8_t)(st + 1);
    *B(x + 9) = 0;
    *H(x + 24) = *H(own + 24);
    *W(x + 128) = 0x08A619B4u; *W(x + 200) = 0x08A61964u; *W(x + 352) = 0x08A60564u; *W(x + 356) = 0x08A60594u;
    *W(c->r[29] + 24) = ra;
    call1(c, x, f_08876998, 0x0891CFBCu);
    *H(x + 112) = 0; *B(x + 73) = 0;
    *H(x + 18) = (uint16_t)S16(own + 18);
    int16_t y = S16(own + 22);
    int32_t kind = S8(x + 346);
    *H(x + 22) = (uint16_t)y;
    *B(x + 11) = *B(own + 11);
    *W(x + 292) = *W(own + 292);
    call2(c, x, (uint32_t)(kind + 8) & 0xFFFF, f_0887684C, 0x0891CFF8u);
    call1(c, x, f_0891F2B0, 0x0891D000u);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (mark, fighter): the side mark's start: the next state, the fighter's facing and looks (+11, +14,
   +15, +24, +38), +82 / +148 cleared, +86 = 8; kind 3 (+3) takes the opponent's side into +13 and -1
   or 0 by it into +12; 0x088A0594, then hidden while the fighter's +76 / +77 pair is 0, else started
   (0x088A0104) and placed (0x0889FFF4). A frame keeping s0, s1. */
SFA_FUNC(0889FF0C, side_mark_start) {
    gaddr x = ARG(0), f = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
    *B(x + 11) = *B(f + 11);
    *B(x + 14) = *B(f + 14);
    *B(x + 15) = *B(f + 15);
    *H(x + 24) = *H(f + 24);
    int16_t h38 = S16(f + 38);
    *H(x + 82) = 0;
    *H(x + 38) = (uint16_t)h38;
    *W(x + 148) = 0;
    uint32_t kind = *B(x + 3);
    *H(x + 86) = 8;
    *W(c->r[29] + 20) = s1; c->r[17] = x;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = f;
    if (kind == 3) {
        int32_t side = S8(*W(f + 92) + 337);
        *B(x + 13) = (uint8_t)side;
        *B(x + 12) = ((uint32_t)side & 0xFF) != 0 ? 0xFF : 0;
    }
    call1(c, x, f_088A0594, 0x0889FF9Cu);
    uint32_t pair = (uint32_t)((S8(f + 76) << 8) + S8(f + 77)) & 0xFFFF;
    if (pair == 0) call1(c, x, f_088A0588, 0x0889FFC0u);
    else {
        c->r[6] = pair;
        call2(c, x, f, f_088A0104, 0x0889FFD4u);
        call2(c, x, f, f_0889FFF4, 0x0889FFE0u);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the taunt's wait (+127): while it runs, the throwing taunt may start (0x0885CF5C), and
   with the opponent's +251 set 0x08863F80; ticked once it has (+691), else it shakes (+268 from the
   table 0x08A17FC8 by the count, turned by +134) and 0x088619EC. When it runs out: the next step
   (+6): with +691 step 4 and a shake for both fighters (0x0886E3CC 9 / 16), else step 0 and
   0x0885D164. A frame keeping s0, s1. */
SFA_FUNC(0885CE44, taunt_wait) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t t = S8(o + 127) - 1;
    *W(c->r[29] + 20) = s1;
    gaddr other = *W(o + 92);
    c->r[17] = other;
    *B(o + 127) = (uint8_t)t;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    if ((int8_t)t < 0) {
        uint32_t step = (uint32_t)*B(o + 6) + 1;
        if (S8(o + 691) == 0) {
            *B(o + 6) = (uint8_t)step; *B(o + 7) = 0; *B(o + 127) = 0;
            call1(c, o, f_0885D164, 0x0885CE98u);
        } else {
            *B(o + 6) = (uint8_t)step; *B(o + 7) = 4; *B(o + 691) = 0; *B(o + 127) = 0;
            call3(c, o, 9, 0x10, f_0886E3CC, 0x0885CEC4u);
            call3(c, other, 9, 0x10, f_0886E3CC, 0x0885CED4u);
        }
    } else {
        call1(c, o, f_0885CF5C, 0x0885CEE4u);
        if (S8(other + 251) != 0) call1(c, o, f_08863F80, 0x0885CEF8u);
        if (S8(o + 691) != 0) call1(c, o, f_08861848, 0x0885CF0Cu);
        else {
            int32_t v = S16(0x08A17FC8u + (uint32_t)(S8(o + 127) * 2));
            *H(o + 268) = (uint16_t)(S8(o + 134) != 0 ? -v : v);
            call1(c, o, f_088619EC, 0x0885CF48u);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
