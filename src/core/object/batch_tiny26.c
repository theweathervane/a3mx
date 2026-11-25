/* Small functions the scenarios reach, 31 instructions: list entry lookups, effects taking their
   owner's place, effects playing once, a countdown step, six effects made at random. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088DD124(Cpu *c); void f_088DD314(Cpu *c); void f_088E09AC(Cpu *c); void f_088E11C0(Cpu *c);
void f_088DD3DC(Cpu *c); void f_088DD418(Cpu *c); void f_088E32BC(Cpu *c); void f_08821424(Cpu *c);
void f_0887684C(Cpu *c); void f_088768F0(Cpu *c); void f_0896A34C(Cpu *c); void f_08821368(Cpu *c);
void f_08822EAC(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c); }
static void call0(Cpu *c, uint32_t back, void (*fn)(Cpu *)) { c->r[31] = back; fn(c); }

/* (n, a, b): 0x088E09AC (a) when n is the current entry (0x088DD124), else 0x088E11C0 (entry n, a, b). */
SFA_FUNC(088E1280, entry_call) {
    uint32_t n = ARG(0), a = ARG(1), b = ARG(2);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1;
    c->r[16] = b; c->r[17] = a;
    *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[18] = n;
    call0(c, 0x088E12A4u, f_088DD124);
    if (n == c->r[2]) call_on(c, a, 0x088E12E4u, f_088E09AC);
    else {
        call_on(c, n, 0x088E12B4u, f_088DD314);
        call3(c, c->r[2], a, b, 0x088E12C4u, f_088E11C0);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (value): each list entry's +222 = whether the current entry (0x088DD124) is under its +228; then
   0x09523938 = value and 0x088E32BC. */
SFA_FUNC(088E32F0, entries_mark_after) {
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 28) = s2;
    c->r[18] = (uint32_t)(int32_t)(int16_t)ARG(0);
    *W(sp + 20) = s0; *W(sp + 24) = s1; *W(sp + 32) = ra;
    call0(c, 0x088E3310u, f_088DD124);
    c->r[17] = c->r[2];
    call0(c, 0x088E3318u, f_088DD3DC);
    *W(sp + 16) = c->r[2];
    c->r[16] = sp + 0x10;
    for (;;) {
        call_on(c, sp + 0x10, 0x088E3328u, f_088DD418);
        gaddr e = c->r[2];
        if (e == 0) break;
        *B(e + 0xD4 + 10) = (int32_t)c->r[17] < (int32_t)*W(e + 228) ? 1 : 0;
    }
    *H(0x09523920u + 24) = (uint16_t)c->r[18];
    call_on(c, 0x09523920u, 0x088E3354u, f_088E32BC);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (owner, effect): the effect takes the owner's +121 / +122 / +80 / +11 - +14 / +38 / x / y and +194 (as
   its +346), +255 / +347 / +112 cleared, and points back at it (+88); the owner's +228 = it and +536
   on. */
static void take_owner_xy(gaddr a, gaddr b, Cpu *c) {
    *B(b + 122) = (uint8_t)S8(a + 122);
    *B(b + 121) = (uint8_t)S8(a + 121);
    *B(b + 346) = (uint8_t)S8(a + 194);
    *B(b + 347) = 0;
    *B(b + 80) = (uint8_t)S8(a + 80);
    *B(b + 255) = 0;
    *B(b + 14) = *B(a + 14);
    *B(b + 11) = *B(a + 11);
    *B(b + 12) = (uint8_t)S8(a + 12);
    *B(b + 13) = *B(a + 13);
    *H(b + 38) = (uint16_t)S16(a + 38);
    *W(b + 16) = *W(a + 16);
    *H(b + 112) = 0;
    *W(b + 20) = *W(a + 20);
    *W(b + 88) = a;
    uint32_t n = *B(a + 536);
    *W(a + 228) = b;
    *B(a + 536) = (uint8_t)(n + 1);
    ARG(1) = n + 1; ARG(2) = n;
}
SFA_FUNC(088F38E4, effect_take_owner_xy)   { take_owner_xy(ARG(0), ARG(1), c); }
SFA_FUNC(08930930, effect_take_owner_xy_2) { take_owner_xy(ARG(0), ARG(1), c); }

/* (effect): first frame its owner's (+88) +228 let go and 0x0887684C (`anim`); then step 3 once +75 has
   bit 7, 0x088768F0; drawn. */
static void play_then_3(Cpu *c, uint32_t anim, const uint32_t backs[3]) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (*B(o + 5) == 0) {
        *B(o + 5) = 1;
        *W(*W(o + 88) + 228) = 0;
        call2(c, o, anim, backs[0], f_0887684C);
    } else {
        if (S8(o + 75) & 0x80) { *B(o + 4) = 3; *B(o + 5) = 0; *B(o + 6) = 0; *B(o + 7) = 0; }
        call_on(c, o, backs[1], f_088768F0);
    }
    call_on(c, o, backs[2], f_08821424);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
SFA_FUNC(0892897C, effect_play_9) {
    static const uint32_t b[3] = { 0x089289B0u, 0x089289E0u, 0x089289E8u };
    play_then_3(c, 9, b);
}
SFA_FUNC(08965EA4, effect_play_6) {
    static const uint32_t b[3] = { 0x08965ED8u, 0x08965F08u, 0x08965F10u };
    play_then_3(c, 6, b);
}

/* (object): +7 on at once with +123 set, else when +83 counts down to 0 (then +83 again from
   0x08A8B8C8 by +194 and +123 cleared); then 0x0896A34C. */
SFA_FUNC(08968850, object_countdown_step) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (S8(o + 123) != 0) *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
    else {
        int8_t t = (int8_t)(S8(o + 83) - 1);
        *B(o + 83) = (uint8_t)t;
        if (t == 0) {
            *B(o + 7) = (uint8_t)(*B(o + 7) + 1);
            *B(o + 83) = *B(0x08A8B8C8u + (uint32_t)(int32_t)S8(o + 194));
            *B(o + 123) = 0;
        }
    }
    c->r[31] = 0x089688C0u; f_0896A34C(c);
    c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (owner): six list-0x27 type-0xD effects (0x08822EAC) for the owner, all on one side picked at
   random (0x08A8E014's 8 bytes). */
SFA_FUNC(0896F498, effects_make_six) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17], s2 = c->r[18];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = s2; *W(c->r[29] + 28) = ra;
    c->r[16] = o;
    c->r[31] = 0x0896F4B4u; f_08821368(c);
    c->r[18] = (uint32_t)(int32_t)S8(0x08A8E014u + (c->r[2] & 7));
    for (c->r[17] = 0; (int32_t)c->r[17] < 6;) {
        call2(c, 0x27, 0xD, 0x0896F4D8u, f_08822EAC);
        gaddr n = c->r[2];
        if (n != 0) { *W(n + 88) = o; *B(n + 139) = (uint8_t)c->r[18]; }
        c->r[17]++;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra;
    c->r[29] += 0x20;
}
