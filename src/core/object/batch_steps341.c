/* Three larger steps: a round's winner marked and rewarded, the pause menu's frame, and the link's
   fighters set up again. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define NETP      0x09523980u
#define FIGHTER_1 0x092E3EA0u
#define FIGHTER_2 0x092E4338u
#define FIGHTER_3 0x092E47D0u
#define FIGHTER_4 0x092E4C68u
#define LIST_1    0x09517A28u
#define LIST_2    0x09517A50u

void f_088288B8(Cpu *c); void f_089E0B64(Cpu *c); void f_08855CB0(Cpu *c); void f_088283B0(Cpu *c);
void f_08828674(Cpu *c); void f_08834FA4(Cpu *c); void f_088B8E80(Cpu *c); void f_0887C4CC(Cpu *c);
void f_08834FF0(Cpu *c); void f_088325DC(Cpu *c); void f_08820C94(Cpu *c); void f_088E938C(Cpu *c);
void f_088E9380(Cpu *c);

static void call1(Cpu *c, uint32_t a, void (*fn)(Cpu *), uint32_t back) { ARG(0) = a; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c);
}
static void call3(Cpu *c, uint32_t a, uint32_t b, uint32_t d, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = a; ARG(1) = b; ARG(2) = d; c->r[31] = back; fn(c);
}
static void mark(gaddr f) { *B(f + 248) = (uint8_t)(S8(f + 248) | 0x80); }

/* (x, y): while y's +36 counts (0x088288B8) nothing else. Then x's +8 moves on, its +20 goes to
   0x97, the screen black, 0x08855CB0; without a draw (y + 76) or y's +13 the winner (y + 112) is
   marked (bit 0x80 of +248) by the fight's kind (y + 56): 0 the winner; 1 and 2 fighters 3 and 4
   when the winner's +337 has bit 2; 3 fighters 1 and 2 when it has not. Last the winner's gauge:
   0x088283B0 past the first mode (y + 79), 0x08828674 in the fights through lists (y + 155). A
   frame keeping s0..s2. */
SFA_FUNC(088266B8, round_winner_marked) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2;
    gaddr winner = *W(y + 112);
    c->r[18] = winner;
    int32_t wait = S8(y + 36);
    *W(sp + 20) = s1; c->r[17] = x;
    *W(sp + 16) = s0; *W(sp + 28) = ra;
    c->r[16] = y;
    if (wait != 0) call2(c, x, y, f_088288B8, 0x088266ECu);
    else {
        *H(x + 8) = (uint16_t)(*H(x + 8) + 1);
        *H(x + 20) = 0x97;
        call3(c, 0xFF000000u, 0xFF000000u, 0xFF000000u, f_089E0B64, 0x08826718u);
        c->r[31] = 0x08826720u; f_08855CB0(c);
        if (S8(y + 76) == 0 && S8(y + 13) == 0) {
            int32_t kind = S16(y + 56);
            if (kind == 0) mark(winner);
            else if (kind == 1 || kind == 2) {
                if (S8(winner + 337) & 2) { mark(FIGHTER_3); mark(FIGHTER_4); }
                else mark(winner);
            } else if (kind == 3) {
                if (S8(winner + 337) & 2) mark(winner);
                else { mark(FIGHTER_1); mark(FIGHTER_2); }
            }
        }
        if (S8(y + 79) != 0) call2(c, x, y, f_088283B0, 0x08826810u);
        else if (S8(y + 155) != 0) call2(c, x, y, f_08828674, 0x0882682Cu);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* The menu's choice is taken back: p's first byte, +1 cleared, +8 from +9, and the menu drawn
   without its cursor (0x08834FF0 with 0). */
static void menu_back(Cpu *c, gaddr a, gaddr b, gaddr p, uint32_t back) {
    *B(p) = (uint8_t)(*B(p) - 1);
    *B(p + 1) = 0;
    *B(p + 8) = *B(p + 9);
    ARG(3) = 0;
    call3(c, a, b, p, f_08834FF0, back);
}

/* (a, b, p): the pause menu's frame. The pad that paused (the first bit of a's +89, 44 bytes each
   from 0x08E75BA0) is copied to the stack. By p's +9: 0 the pause ends; 1 0x088B8E80, and +8 set
   unless p's first byte is 2; the others the menu itself: the first frame only marks +1; then
   0x100 takes the choice back (sound 0x2F), 0x8080 chooses (sound 0x39: the second entry takes it
   back, the others leave by 0x088325DC), 8 and 4 move the cursor (+8) between two entries (sound
   0x38); and the menu is drawn (0x08834FF0 with 1). A 0x50 frame keeping s0..s3. */
SFA_FUNC(08834D00, pause_menu_frame) {
    gaddr a = ARG(0), b = ARG(1), p = ARG(2);
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x50;
    gaddr sp = c->r[29];
    *W(sp + 60) = sv[0]; *W(sp + 64) = sv[1];
    c->r[16] = a; c->r[17] = b;
    *W(sp + 68) = sv[2];
    c->r[18] = p;
    uint32_t mask = (uint32_t)(int32_t)S8(a + 89);
    *W(sp + 72) = sv[3];
    c->r[19] = 1;
    uint32_t state = *B(p + 9);
    *W(sp + 76) = sv[4];
    uint32_t pad = 0;
    while (pad < 3 && !(mask & (1u << pad))) pad++;
    gaddr src = 0x08E75BA0u + pad * 44;
    for (uint32_t k = 0; k < 11; k++) *W(sp + 16 + 4 * k) = *W(src + 4 * k);
    c->r[5] = *W(src + 36); c->r[7] = *W(src + 28); c->r[8] = *W(src + 32);
    if (state == 0) call3(c, a, b, p, f_08834FA4, 0x08834DDCu);
    else if (state == 1) {
        ARG(2) = state;
        call1(c, *W(src + 40), f_088B8E80, 0x08834DF4u);
        if (*B(p) != 2) *B(p + 8) = 1;
    } else {
        int drawn = 0;
        if (*B(p + 1) == 0) *B(p + 1) = 1;
        else {
            uint32_t held = *W(sp + 28);
            if (held & 0x100) {
                call3(c, 6, 0x2F, 0xC0, f_0887C4CC, 0x08834E4Cu);
                menu_back(c, a, b, p, 0x08834E78u);
                drawn = 1;
            } else if (held & 0x8080) {
                call3(c, 6, 0x39, 0xC0, f_0887C4CC, 0x08834EA0u);
                if (*B(p + 8) != 1) call2(c, a, b, f_088325DC, 0x08834EB4u);
                else { menu_back(c, a, b, p, 0x08834EE8u); drawn = 1; }
            } else if (held & 8) {
                call3(c, 6, 0x38, 0xC0, f_0887C4CC, 0x08834F1Cu);
                uint32_t at = *B(p + 8);
                *B(p + 8) = at != 0 ? (uint8_t)(at - 1) : 1;
            } else if (held & 4) {
                call3(c, 6, 0x38, 0xC0, f_0887C4CC, 0x08834F54u);
                uint32_t at = (uint32_t)*B(p + 8) + 1;
                *B(p + 8) = (uint8_t)at;
                if ((at & 0xFF) == 2) *B(p + 8) = 0;
            }
        }
        if (!drawn) { ARG(3) = 1; call3(c, a, b, p, f_08834FF0, 0x08834F88u); }
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x50;
}

/* (x, y): the link's fighters set up again. The fight's kind (y + 56) comes from NETP + 52 and
   0x08820C94 runs; the three taking part go to the stack by the kind (0: 1 and 2; 1: 1, 3, 4;
   2: 2, 3, 4; 3: 1, 2, 3; the others leave what was there) and each is set up, last first, with
   the flag set in the modes 0xD and 0x13 unless x's +53 has bit 4. Then both lists' +1 and +2
   are cleared. A 0x30 frame keeping s0..s2. */
SFA_FUNC(088E9458, link_fighters_again) {
    gaddr x = ARG(0), y = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 32) = s1; c->r[17] = y;
    *W(sp + 28) = s0; c->r[16] = x;
    int32_t kind = S16(NETP + 52);
    *W(sp + 36) = s2; *W(sp + 40) = ra;
    *H(y + 56) = (uint16_t)kind;
    call2(c, (uint32_t)kind, NETP, f_08820C94, 0x088E9488u);
    kind = S16(y + 56);
    int32_t mode = S8(x + 114);
    if (kind == 0) { *W(sp + 16) = FIGHTER_1; *W(sp + 20) = FIGHTER_2; *W(sp + 24) = 0; }
    else if (kind == 1) { *W(sp + 16) = FIGHTER_1; *W(sp + 20) = FIGHTER_3; *W(sp + 24) = FIGHTER_4; }
    else if (kind == 2) { *W(sp + 16) = FIGHTER_2; *W(sp + 20) = FIGHTER_3; *W(sp + 24) = FIGHTER_4; }
    else if (kind == 3) { *W(sp + 16) = FIGHTER_1; *W(sp + 20) = FIGHTER_2; *W(sp + 24) = FIGHTER_3; }
    uint32_t flag = 0;
    if ((mode == 0x13 || mode == 0xD) && !(S8(x + 53) & 4)) flag = 1;
    c->r[18] = flag;
    for (int32_t i = 2; i >= 0; i--) {
        gaddr at = sp + 8 - 4 * (uint32_t)(2 - i);
        c->r[16] = at; c->r[17] = (uint32_t)i;
        gaddr f = *W(at + 16);
        if (f != 0) call2(c, f, flag, f_088E938C, 0x088E95A0u);
    }
    call1(c, LIST_1, f_088E9380, 0x088E95B8u);
    call1(c, LIST_2, f_088E9380, 0x088E95C4u);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x30;
}

void f_08823338(Cpu *c); void f_0882DD5C(Cpu *c); void f_08809BE4(Cpu *c); void f_08809FE8(Cpu *c);
void f_088099D4(Cpu *c); void f_088884C8(Cpu *c); void f_088891CC(Cpu *c);

#define GAME 0x08E75910u

/* A held direction repeats: the first frame of a group (0xC is 1, 3 is 2) waits 0x14 frames, then
   it counts as pressed again every seventh. The group is kept at dir and the count at dir + 1. */
static uint32_t held_repeats(uint32_t pressed, uint32_t held, gaddr dir) {
    uint32_t bits = held & 0xC;
    uint32_t group = 1;
    if (bits == 0) { bits = held & 3; group = 2; }
    if (bits == 0) { *B(dir) = 0; return pressed; }
    if ((uint32_t)(int32_t)S8(dir) != group) {
        *B(dir + 1) = 0x14;
        *B(dir) = (uint8_t)group;
        return pressed;
    }
    int32_t t = S8(dir + 1) - 1;
    *B(dir + 1) = (uint8_t)t;
    if ((int8_t)t >= 0) return pressed;
    *B(dir + 1) = 6;
    return (pressed | bits) & 0xFFFF;
}

/* A menu's frame, its record at 0x0950FF60. The first time it is marked, with 0x08823338 (3, 4)
   agreeing the pause is set up (0x0882DD5C, the music's tempo 0x1E, the sound thread's bit 0x10,
   the callback tables' reset) and without it GAME's +89 and +90 are cleared; the record's +5 goes
   to the word at 0x08B02648. Afterwards the pad of the side GAME's +53 names (bit 0: the first)
   with its directions repeating goes to the step the record's +1 names in the table 0x08A1379C.
   Then, of the 32 objects from 0x092E2B88, those of the kind 3 run 0x088884C8 and those of the
   kind 4 with +92 at 3 run 0x088891CC. A 0x30 frame keeping s0..s3. */
SFA_FUNC(088480EC, menu_frame_ff60) {
    const gaddr rec = 0x0950FF60u;
    uint32_t sv[5];
    for (int i = 0; i < 4; i++) sv[i] = c->r[16 + i];
    sv[4] = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    for (int i = 0; i < 4; i++) *W(sp + 16 + 4 * i) = sv[i];
    *W(sp + 32) = sv[4];
    c->r[16] = rec;
    if (*B(rec) == 0) {
        *B(rec) = 1;
        *B(rec + 23) = 0;
        call2(c, 3, 4, f_08823338, 0x0884812Cu);
        if (c->r[2] != 0) {
            c->r[31] = 0x0884813Cu; f_0882DD5C(c);
            call1(c, 0x1E, f_08809BE4, 0x08848144u);
            c->r[31] = 0x0884814Cu; f_08809FE8(c);
            c->r[31] = 0x08848154u; f_088099D4(c);
        } else {
            *B(GAME + 89) = 0;
            *B(GAME + 90) = 0;
        }
        *W(0x08B00000u + 9800) = *B(rec + 5);
    } else {
        uint32_t pressed;
        if (S8(GAME + 53) & 1)
            pressed = held_repeats(*W(0x08E75BA0u + 12) & 0xFFFF, *W(0x08E75BA0u + 4), 0x08A10000u + 14276);
        else
            pressed = held_repeats(*W(0x08E75BA0u + 56) & 0xFFFF, *W(0x08E75BA0u + 48), 0x08A10000u + 14278);
        uint32_t fn = *W(0x08A1379Cu + ((uint32_t)*B(rec + 1) << 2));
        c->r[6] = fn;
        ARG(0) = rec; ARG(1) = pressed; c->r[31] = 0x0884833Cu;
        rt_call_indirect(c, fn);
    }
    c->r[18] = 3; c->r[19] = 4;
    gaddr o = 0x092E2B88u;
    for (uint32_t i = 0; i < 32; i++, o += 0x94) {
        c->r[16] = o; c->r[17] = i;
        if (*B(o + 90) == 0) continue;
        if (*B(o + 91) == 3) call1(c, o, f_088884C8, 0x08848370u);
        if (*B(o + 91) == 4 && *B(o + 92) == 3) call1(c, o, f_088891CC, 0x08848390u);
    }
    for (int i = 0; i < 4; i++) c->r[16 + i] = sv[i];
    c->r[31] = sv[4]; c->r[29] += 0x30;
}
