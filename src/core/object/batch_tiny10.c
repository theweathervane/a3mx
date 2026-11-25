/* Small functions the fighter scenarios (tests/scenarios/fighters_*.txt, 0.164.0) reached, about 20
   instructions, ready once batch_tiny9.c was done: a move's strength from the button pressed, palettes
   by colour, owners' links cleared before an object is freed, the CPU's script steps, script reading,
   network session checks. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D3C88(Cpu *c); void f_0882324C(Cpu *c); void f_089E1234(Cpu *c); void f_0886DC3C(Cpu *c);
void f_0880AEA8(Cpu *c); void f_0887B6B8(Cpu *c); void f_0887B7CC(Cpu *c); void f_0887BBC8(Cpu *c);
void f_0887B1A8(Cpu *c); void f_088DD114(Cpu *c); void f_088E3B18(Cpu *c); void f_08822F54(Cpu *c);
void f_089DA93C(Cpu *c); void f_089E00B0(Cpu *c);

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
static uint8_t inc8(gaddr a) { uint8_t v = (uint8_t)(*B(a) + 1); *B(a) = v; return v; }
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- objects and owners */
SFA_FUNC(089C5AEC, call_088D3C88_a) { WRAP20(0x089C5AFCu, f_088D3C88); }
SFA_FUNC(089CBDCC, call_088D3C88_b) { WRAP20(0x089CBDDCu, f_088D3C88); }
SFA_FUNC(089CC544, call_088D3C88_c) { WRAP20(0x089CC554u, f_088D3C88); }
/* (object): the first of its owner's (+88) three links that is this object cleared, then freed. */
static void unlink_first_then_free(Cpu *c, const unsigned at[3], uint32_t back) {
    gaddr o = ARG(0), w = *W(o + 88);
    uint32_t v = 0;
    for (int k = 0; k < 3; k++) {
        v = *W(w + at[k]);
        if (v == o) { *W(w + at[k]) = 0; break; }
    }
    ARG(1) = w; ARG(2) = v;
    WRAP20(back, f_0882324C);
}
SFA_FUNC(089BE6C4, unlink_40_44_48_free) { static const unsigned at[3] = { 40, 44, 48 }; unlink_first_then_free(c, at, 0x089BE708u); }
SFA_FUNC(089C4760, unlink_40_48_52_free) { static const unsigned at[3] = { 40, 48, 52 }; unlink_first_then_free(c, at, 0x089C47A4u); }
/* (owner): a new object (0x08822F54: list 0x30 / 0x31, 0) with the owner at +88, +139 and +14 the
   owner's, +0 = 1; the owner's +40 / +48 = it. */
static void new_linked(Cpu *c, uint32_t list, unsigned link, uint32_t back) {
    gaddr owner = ARG(0); WITH_S0(owner);
    ARG(0) = list; ARG(1) = 0;
    c->r[31] = back; f_08822F54(c);
    gaddr n = c->r[2];
    if (n != 0) {
        *W(n + 88) = owner;
        *B(n + 139) = (uint8_t)S8(owner + 139);
        ARG(0) = *B(owner + 14);
        *B(n + 14) = (uint8_t)ARG(0);
        *W(owner + link) = n;
        ARG(1) = 1;
        *B(n) = 1;
    }
    END_S0();
}
SFA_FUNC(089BE2DC, new_object_30_linked) { new_linked(c, 0x30, 40, 0x089BE2F8u); }
SFA_FUNC(089BE330, new_object_31_linked) { new_linked(c, 0x31, 48, 0x089BE34Cu); }
/* (object): as its owner's +12, +13 (also +139), +24, side (+337, at +86) and +164; +4 on, +78 and
   +140 cleared. */
SFA_FUNC(089245D0, setup_from_owner_139) {
    gaddr o = ARG(0), w = *W(o + 88);
    *B(o + 12) = (uint8_t)S8(w + 12);
    inc8(o + 4);
    *B(o + 13) = *B(w + 13);
    *H(o + 24) = *H(w + 24);
    *B(o + 78) = 0;
    *B(o + 139) = *B(o + 13);
    *B(o + 140) = 0;
    *H(o + 86) = (uint16_t)(int16_t)S8(w + 337);
    *W(o + 164) = *W(w + 164);
}
/* (object): x moved the way it faces, stopped once the speed goes negative; while the y speed is
   negative its change +108 = -0.5. */
SFA_FUNC(088FE670, move_x_facing_stop_fall) {
    gaddr o = ARG(0);
    uint32_t v = *W(o + 96);
    if ((int32_t)*W(o + 100) < 0) *W(o + 108) = 0xFFFF8000u;
    *W(o + 16) += *B(o + 11) == 0 ? 0u - v : v;
    uint32_t nv = v + *W(o + 104);
    *W(o + 96) = nv;
    if ((int32_t)nv < 0) { *W(o + 104) = 0; *W(o + 96) = 0; }
}
/* (object): 1 while its x is within +0x6C in front of its fighter's (+92) (by facing). */
SFA_FUNC(089B14B0, within_6C_of_fighter) {
    gaddr o = ARG(0), f = *W(o + 92);
    int16_t x = S16(o + 18), fx = S16(f + 18);
    int16_t d = (int16_t)(*B(o + 11) != 0 ? fx - x : x - fx);
    RETURN((int32_t)(d & 0xFFFF) < (int32_t)*H(o + 0x6C));
}
/* (object, record): unless the record's +2 is below +80, +5 on and 0x089DA93C (object, record). */
SFA_FUNC(089DA8E8, step_on_5_by_80) {
    gaddr o = ARG(0), r = ARG(1);
    int32_t lim = S8(o + 80);
    ARG(0) = r;
    if (S16(r + 2) < lim) return;
    inc8(o + 5);
    ARG(0) = o; ARG(1) = r; ARG(2) = r;
    WRAP20(0x089DA930u, f_089DA93C);
}
/* (object): when +37 is 1, 0x089E00B0 (1), then the camera's (0x092E88A0) y moved by +160. */
SFA_FUNC(089DE700, camera_y_by_160) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    c->r[16] = o;
    int8_t v = S8(o + 37);
    ARG(0) = v < 2;
    if (v == 1) {
        c->r[31] = 0x089DE72Cu; f_089E00B0(c);
        *H(0x092E88A0u + 22) = (uint16_t)(S16(0x092E88A0u + 22) + S16(o + 160));
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}
/* (object): its next routine from 0x08A956E8 by +840 (cleared), unless +824 is 0 and +77 negative. */
SFA_FUNC(0898AE0C, routine_by_840) {
    gaddr o = ARG(0);
    if (*W(o + 824) == 0 && (S8(o + 77) & 0x80)) return;
    uint32_t n = *W(o + 840);
    *W(o + 840) = 0;
    gaddr h = *W(0x08A956E8u + n * 4);
    ARG(2) = 0x08A956E8u; ARG(1) = h;
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20; *W(c->r[29] + 16) = ra;
    c->r[31] = 0x0898AE54u;
    rt_call_indirect(c, h);
    c->r[31] = ra; c->r[29] += 0x20;
}
/* (-, reader): the reader's entry (+0, 8 bytes) moved on by its +6 entries and its first four bytes
   copied to +4 - +7; at an entry with bit 6 of +1, +5 = -1 instead. */
SFA_FUNC(0891DD1C, script_next) {
    gaddr r = ARG(1), p = *W(r);
    if (S8(p + 1) & 0x40) { *B(r + 5) = 0xFF; return; }
    p += (uint32_t)(int32_t)S16(p + 6) << 3;
    *W(r) = p;
    *B(r + 4) = *B(p);
    *B(r + 5) = (uint8_t)S8(p + 1);
    *B(r + 6) = (uint8_t)S8(p + 2);
    *B(r + 7) = (uint8_t)S8(p + 3);
}
/* (pairs): for each pair (a, b) up to a = 0xFF, 0x089E1234 (a + 0xA0, b): a texture's palette. */
SFA_FUNC(088199CC, texture_palettes) {
    gaddr p = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0xFF;
    for (uint8_t a = *B(p); a != 0xFF; a = *B(p)) {
        c->r[16] = p;
        ARG(1) = *B(p + 1); ARG(0) = a + 0xA0u;
        c->r[31] = 0x088199FCu; f_089E1234(c);
        p += 2;
    }
    ARG(0) = 0xFF;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters */
/* (fighter): 0x0886DC3C, then the move's strength (+194): 0 light, 1 medium, 2 heavy, by the punch /
   kick buttons pressed or released (+760 | +762). */
static void strength_from(Cpu *c, uint32_t light, uint32_t medium, uint32_t back) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, back, f_0886DC3C);
    uint32_t k = (*H(f + 760) | *H(f + 762)) & 0xFFFF;
    *B(f + 194) = (k & light) ? 0 : (k & medium) ? 1 : 2;
    END_S0();
}
SFA_FUNC(0886DB94, punch_strength) { strength_from(c, 0x10, 0x20, 0x0886DBA8u); }
SFA_FUNC(0886DBE8, kick_strength) { strength_from(c, 0x80, 0x100, 0x0886DBFCu); }
/* (fighter, n): unless +657 is n already, palettes for its colour (+376) into its slot (+13): from its
   routine table's +132 (4 a colour, one), or +136 (2 a colour, two from slot +3). */
static void palette_by_657(Cpu *c, unsigned at, unsigned per, unsigned add, unsigned count, uint32_t back) {
    gaddr f = ARG(0);
    int8_t n = (int8_t)ARG(1);
    uint32_t base = *W(*W(f + 416) + at);
    ARG(1) = base;
    if (S8(f + 657) == n) return;
    ARG(1) = base + ((((uint32_t)(int32_t)S8(f + 376) * per) & 0xFFFF) << 5);
    ARG(3) = *B(f + 13) + add;
    ARG(0) = 0; ARG(2) = 0; c->r[8] = count;
    wrap(c, 0x30, 32, back, f_0880AEA8);
}
SFA_FUNC(0886F9A4, palette_by_657_4) { palette_by_657(c, 132, 4, 0, 1, 0x0886F9ECu); }
SFA_FUNC(0886FADC, palette_by_657_136) { palette_by_657(c, 136, 2, 3, 2, 0x0886FB28u); }
/* (fighter): the CPU's script step: 0x0887B7CC when 0x0887B6B8 says so, else its first script and
   command cleared when 0x0887BBC8 says so. */
SFA_FUNC(0887A8B0, cpu_script_step) {
    gaddr f = ARG(0); WITH_S0(f);
    call_on(c, f, 0x0887A8C4u, f_0887B6B8);
    if (c->r[2] != 0) call_on(c, f, 0x0887A8D4u, f_0887B7CC);
    else {
        call_on(c, f, 0x0887A8E4u, f_0887BBC8);
        if (c->r[2] != 0) call_on(c, f, 0x0887A8F4u, f_0887B1A8);
    }
    END_S0();
}

/* ---------------------------------------------------------------- network session */
/* (record): 1 when its +42 is 2 or 3 and the session's +236 is 2 or more. */
SFA_FUNC(088D59A0, session_236_two) {
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x10;
    *W(c->r[29]) = s0; *W(c->r[29] + 4) = ra;
    uint8_t k = *B(ARG(0) + 42);
    uint32_t res = 0;
    if (k == 3 || k == 2) {
        c->r[31] = 0x088D59D0u; f_088DD114(c);
        if (!((int32_t)c->r[2] < 2)) res = 1;
    }
    RETURN(res);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x10;
}
/* 1 with no record at 0x09523960; else 0, after counting its +22 on (0x088E3B18) when it is 2. */
SFA_FUNC(088E3F44, net_record_check) {
    gaddr s = 0x09523960u;
    ARG(0) = s;
    if (*W(s) == 0) { RETURN(1); return; }
    ARG(1) = *B(s + 22); ARG(2) = 2;
    if (ARG(1) == 2) WRAP20(0x088E3F78u, f_088E3B18);
    RETURN(0);
}
