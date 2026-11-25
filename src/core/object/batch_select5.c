/* Functions the select, VS and menu scenarios reach (the areas the Hyper Alpha spike works in): the ISM
   box's steps, a set of textures freed, the mode's title panel, a throw's start. Offsets stay numbers
   until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_089E06BC(Cpu *c); void f_0889CE64(Cpu *c); void f_0889AD44(Cpu *c); void f_088215C4(Cpu *c);
void f_08871828(Cpu *c); void f_08871A84(Cpu *c); void f_088748FC(Cpu *c); void f_0880AEA8(Cpu *c);
void f_08821424(Cpu *c);

static void wrap(Cpu *c, uint32_t frame, uint32_t slot, uint32_t back, void (*fn)(Cpu *)) {
    uint32_t ra = c->r[31];
    c->r[29] -= frame;
    *W(c->r[29] + slot) = ra;
    c->r[31] = back;
    fn(c);
    c->r[31] = ra;
    c->r[29] += frame;
}
#define WITH_S0(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = ra_; c->r[16] = (o)
#define END_S0() do { c->r[16] = s0_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
#define WITH_S01(o) \
    uint32_t ra_ = c->r[31], s0_ = c->r[16], s1_ = c->r[17]; \
    c->r[29] -= 0x20; *W(c->r[29] + 16) = s0_; *W(c->r[29] + 20) = s1_; *W(c->r[29] + 24) = ra_; c->r[16] = (o)
#define END_S01() do { c->r[16] = s0_; c->r[17] = s1_; c->r[31] = ra_; c->r[29] += 0x20; } while (0)
static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* ---------------------------------------------------------------- the ISM box's steps */
/* (box): when it has arrived (0x0889CE64), +6 on (and x from +114); then the box (0x0889AD44) and drawn. */
static void ism_box_step(Cpu *c, int set_x, const uint32_t backs[3]) {
    gaddr o = ARG(0); WITH_S0(o);
    call_on(c, o, backs[0], f_0889CE64);
    if (c->r[2] != 0) {
        int16_t x = S16(o + 114);
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        if (set_x) *W(o + 16) = (uint32_t)x << 16;
    }
    call_on(c, o, backs[1], f_0889AD44);
    call_on(c, o, backs[2], f_088215C4);
    END_S0();
}
SFA_FUNC(0889ACFC, ism_box_wait) { static const uint32_t b[3] = { 0x0889AD10u, 0x0889AD2Cu, 0x0889AD34u }; ism_box_step(c, 0, b); }
SFA_FUNC(0889AC50, ism_box_arrive) { static const uint32_t b[3] = { 0x0889AC64u, 0x0889AC8Cu, 0x0889AC94u }; ism_box_step(c, 1, b); }
/* (box, row): when the row's +78 has bit 1 (confirmed), +6 on, +114 = +112 and the speed cleared; then
   the box and drawn. */
SFA_FUNC(0889ACA4, ism_box_confirmed) {
    gaddr o = ARG(0);
    int8_t v = S8(ARG(1) + 78);
    WITH_S0(o);
    if (v & 2) {
        int16_t y = S16(o + 112);
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *H(o + 114) = (uint16_t)y;
        *W(o + 96) = 0;
    }
    call_on(c, o, 0x0889ACE4u, f_0889AD44);
    call_on(c, o, 0x0889ACECu, f_088215C4);
    END_S0();
}

/* ---------------------------------------------------------------- textures */
/* (n): texture record n freed (0x089E06BC); 0 back. */
SFA_FUNC(089E0C24, texture_free_answer) { wrap(c, 0x20, 16, 0x089E0C34u, f_089E06BC); RETURN(0); }
/* (set): each of its +4 textures (12-byte entries from +0, the record at +8) freed; 0 back. */
SFA_FUNC(089E157C, texture_set_free) {
    gaddr set = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 28) = ra;
    c->r[16] = set;
    c->r[17] = 0; c->r[18] = 0;
    while (c->r[18] < *W(set + 4)) {
        ARG(0) = *W(*W(set) + c->r[17] + 8);
        c->r[31] = 0x089E15B8u; f_089E06BC(c);
        c->r[18]++;
        c->r[17] += 0xC;
    }
    RETURN(0);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- panels */
/* (panel): the mode's title panel: +4 on, picture 0x18 + the byte at 0x08A37790 by the mode (GAME
   +114), at (0xC0, 0x24), +12 = 1, +24 = 7, +86 = 0x12, +148 = 3, +169 = 1, others cleared; one palette
   from the side-2 palettes (0x092D9E00 +72) into slot 0; then 0x08821424. */
SFA_FUNC(088B58D8, mode_title_panel) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0; *W(c->r[29] + 36) = ra;
    c->r[16] = o;
    *B(o + 9) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 14) = 0; *B(o + 12) = 1; *B(o + 13) = 0;
    *H(o + 24) = 7;
    *B(o + 11) = 0;
    *H(o + 84) = (uint16_t)(*B(0x08A37790u + (uint32_t)(int32_t)S8(GAME + 114)) + 0x18);
    *H(o + 18) = 0xC0; *H(o + 22) = 0x24;
    *B(o + 1) = 0;
    *H(o + 86) = 0x12;
    *W(o + 148) = 3;
    *W(o + 164) = 0;
    *B(o + 169) = 1;
    *B(o + 10) = 0;
    ARG(1) = *W(0x092D9E00u + 72);
    ARG(0) = 0; ARG(2) = 0; ARG(3) = 0; c->r[8] = 1;
    c->r[31] = 0x088B5988u; f_0880AEA8(c);
    call_on(c, o, 0x088B5990u, f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* ---------------------------------------------------------------- hits */
/* (fighter, hit): a hit taken: with +73 clear and the hit record's (0x09358410) +72 set, it is
   blocked: +588 cleared, guarding (+696) 0x08871828, else +117 = 5 and 0x08871A84. Otherwise +698 /
   +539 = 1, +590 cleared, +117 = the hit's +13, 0x088748FC, and the hit kind's routine (0x08A21DCC by
   the hit's +13; fighter, hit). */
SFA_FUNC(08871358, take_hit) {
    gaddr f = ARG(0), x = ARG(1);
    WITH_S01(x);
    c->r[17] = f;
    int react = S8(f + 73) != 0 || *B(0x09358410u + 72) == 0;
    if (!react) {
        *B(f + 588) = 0;
        if (S8(f + 696) != 0) call2(c, f, x, 0x088713A4u, f_08871828);
        else { *B(f + 117) = 5; call2(c, f, x, 0x088713C0u, f_08871A84); }
    } else {
        *B(f + 698) = 1; *B(f + 539) = 1; *B(f + 590) = 0;
        uint8_t kind = *B(x + 13);
        *B(f + 117) = kind;
        call2(c, f, kind, 0x088713E8u, f_088748FC);
        gaddr h = *W(0x08A21DCCu + (uint32_t)*B(x + 13) * 4);
        ARG(0) = f; ARG(1) = x; ARG(2) = h;
        c->r[31] = 0x0887140Cu;
        rt_call_indirect(c, h);
    }
    END_S01();
}
