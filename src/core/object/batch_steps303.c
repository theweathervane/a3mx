/* More of what the new leaves set free: two characters' helper fighters' starts (one shape), and a
   forward dash that can turn into a move. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define BATTLE 0x092E1228u

void f_08876998(Cpu *c); void f_0887684C(Cpu *c); void f_088F16D4(Cpu *c); void f_088F1AD4(Cpu *c);
void f_088F14D8(Cpu *c); void f_08965F58(Cpu *c); void f_08966370(Cpu *c); void f_08965D58(Cpu *c);
void f_088616EC(Cpu *c); void f_08974620(Cpu *c); void f_08974578(Cpu *c); void f_089746E4(Cpu *c);
void f_089745AC(Cpu *c); void f_08861848(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}

/* A helper fighter's start (the helper's kind is +346): the next state, its owner's (+88) +24, its
   own tables (+352, +356, +128, +200) and 0x08876998; kinds 3 to 10 run `prepare` and, for an owner
   whose choice is -1, `pieces`; kinds 11 and up take the next colour (+13); +344 from the byte table
   by kind (16 further on for the choice -1), the speed from the word table, by its facing, placed
   0x59 in front of where it is and 0x38 up; 0x0887684C with the kind (17 more for the choice -1) and
   `first`. A 0x30 frame keeping s0..s6. */
typedef struct {
    uint32_t t352, t356, t128, t200, bytes, speeds;
    void (*prepare)(Cpu *), (*pieces)(Cpu *), (*first)(Cpu *);
    uint32_t back[5];
} Helper;

static void helper_start(Cpu *c, const Helper *h) {
    gaddr x = ARG(0);
    uint32_t sv[8];
    for (int i = 0; i < 7; i++) sv[i] = c->r[16 + i];
    sv[7] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = x;
    *W(sp + 36) = sv[5];
    gaddr own = *W(x + 88);
    c->r[21] = own;
    uint32_t kind = (uint32_t)(int32_t)S8(x + 346) & 0xFFFF;
    *B(x + 4) = (uint8_t)(*B(x + 4) + 1);
    *W(sp + 32) = sv[4];
    *B(x + 9) = 0;
    *H(x + 24) = *H(own + 24);
    *W(x + 352) = h->t352; *W(x + 356) = h->t356; *W(x + 128) = h->t128; *W(x + 200) = h->t200;
    c->r[20] = kind;
    *W(sp + 20) = sv[1]; *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 40) = sv[6]; *W(sp + 44) = sv[7];
    call1(c, x, f_08876998, h->back[0]);
    int small = (int32_t)kind < 11;
    gaddr byte_at = h->bytes + kind, speed_at = h->speeds + (kind << 2);
    c->r[18] = byte_at; c->r[17] = speed_at; c->r[22] = (uint32_t)small; c->r[19] = 0xFFFFFFFFu;
    if (small && !((int32_t)kind < 3)) {
        call1(c, x, h->prepare, h->back[1]);
        if (S8(own + 386) == -1) call1(c, x, h->pieces, h->back[2]);
    }
    *B(x + 73) = 0;
    *W(x + 100) = 0;
    int32_t px = S16(x + 18);
    if (!small) *B(x + 13) = (uint8_t)(*B(x + 13) + 1);
    *B(x + 344) = (uint8_t)(S8(own + 386) == -1 ? S8(byte_at + 16) : S8(byte_at));
    uint32_t speed = *W(speed_at);
    c->r[18] = speed;
    if (*B(x + 11) != 0) { *W(x + 96) = speed; *H(x + 18) = (uint16_t)(px + 0x59); }
    else { *W(x + 96) = 0 - speed; *H(x + 18) = (uint16_t)(px - 0x59); }
    *H(x + 22) = (uint16_t)(S16(x + 22) + 0x38);
    if (S8(own + 386) == -1) kind = (kind + 0x11) & 0xFFFF;
    c->r[20] = kind;
    call2(c, x, kind, f_0887684C, h->back[3]);
    call1(c, x, h->first, h->back[4]);
    for (int i = 0; i < 7; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[7]; c->r[29] += 0x30;
}

SFA_FUNC(088F133C, helper_start_f133c) {
    static const Helper h = { 0x08A40768u, 0x08A407F0u, 0x08A40AD4u, 0x08A408BCu, 0x08A43BD8u, 0x08A43BF8u,
                              f_088F16D4, f_088F1AD4, f_088F14D8,
                              { 0x088F13C8u, 0x088F1404u, 0x088F1418u, 0x088F14A8u, 0x088F14B0u } };
    helper_start(c, &h);
}
SFA_FUNC(08965BBC, helper_start_65bbc) {
    static const Helper h = { 0x08A86F3Cu, 0x08A86FC4u, 0x08A872ACu, 0x08A87092u, 0x08A8AA3Cu, 0x08A8AA5Cu,
                              f_08965F58, f_08966370, f_08965D58,
                              { 0x08965C48u, 0x08965C84u, 0x08965C98u, 0x08965D28u, 0x08965D30u } };
    helper_start(c, &h);
}

/* (fighter): a forward dash: it ends (step 5, animation 3) while the fight is held, when the opponent
   has no vitality left, or when its time (+82) runs out. At a wall on its facing side (+238), or for a
   player holding back, it stops short: step 4, the time down by 8, animation 4. Before that the move
   0x08974620 picks (0x08974578 with its byte), or the leap 0x089745AC when 0x089746E4 says so or +648
   is set; else it moves on by its speed by hand and is ticked. A 0x20 frame keeping s0 at +20, the
   byte at +16. */
SFA_FUNC(089743B8, dash_forward) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    int held = S8(BATTLE + 14) | S8(BATTLE + 13);
    *W(sp + 20) = s0; c->r[16] = o;
    *W(sp + 24) = ra;
    int end = held != 0 || S16(*W(o + 92) + 112) < 0;
    if (!end) {
        int32_t t = S16(o + 82);
        if (t == 0) end = 1;
        else {
            *H(o + 82) = (uint16_t)(t - 1);
            if ((int16_t)(t - 1) == 0) end = 1;
        }
    }
    if (end) {
        *B(o + 7) = 5;
        call2(c, o, 3, f_088616EC, 0x08974568u);
        goto out;
    }
    if (!(S8(o + 238) & (int32_t)(1u << (*B(o + 11) & 31)))) {
        call2(c, o, sp + 0x10, f_08974620, 0x08974434u);
        if (c->r[2] != 0) { call2(c, o, (uint32_t)(int32_t)S8(sp + 16), f_08974578, 0x089744A8u); goto out; }
        call1(c, o, f_089746E4, 0x08974444u);
        if (c->r[2] != 0) { call1(c, o, f_089745AC, 0x08974490u); goto out; }
        if (S8(o + 648) != 0) { call1(c, o, f_089745AC, 0x08974478u); goto out; }
        uint8_t facing = *B(o + 11);
        uint32_t x = *W(o + 16), v = *W(o + 96);
        int32_t cpu = S8(o + 373);
        *W(o + 16) = facing != 0 ? x + v : x - v;
        if (cpu != 0) { call1(c, o, f_08861848, 0x08974528u); goto out; }
        if (!(*H(o + 752) & 1)) { call1(c, o, f_08861848, 0x08974510u); goto out; }
    }
    {
        int32_t t = S16(o + 82) - 8;
        *B(o + 7) = 4;
        *H(o + 82) = (uint16_t)t;
        if ((int16_t)t < 0) *H(o + 82) = 0;
        call2(c, o, 4, f_088616EC, 0x08974544u);
    }
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
