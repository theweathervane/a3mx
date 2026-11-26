/* Small functions the scenarios reach, 74 - 76 instructions: a menu item's press with its sounds,
   a projectile's start, a palette animation step, a select-screen portrait's colours. Offsets stay
   numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

#define GAME 0x08E75910u

void f_0880AEA8(Cpu *c); void f_08823338(Cpu *c); void f_0887684C(Cpu *c); void f_08876998(Cpu *c);
void f_08900A64(Cpu *c); void f_088231C8(Cpu *c); void f_089E11C8(Cpu *c);

static void call_on(Cpu *c, gaddr o, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, uint32_t a, uint32_t b, uint32_t back, void (*fn)(Cpu *)) { ARG(0) = a; ARG(1) = b; c->r[31] = back; fn(c); }

/* (item): once the press (GAME +16's low byte) reaches its number (+13) + 0x36: counted (+9), its bit
   added to GAME +10's high byte; item 3 loads the palette 0x08A310B8; the others set +68 = -0x20,
   play 7 / 0x10 (items 0 / 1) and item 2 plays 7 / 0x11 and, with it, sets three objects' +122 (3 / 1 /
   2) and plays 0x1D. */
SFA_FUNC(088AC0E8, menu_item_press) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16], s1 = c->r[17];
    c->r[29] -= 0x30;
    *W(c->r[29] + 32) = s0;
    c->r[16] = o;
    *W(c->r[29] + 36) = s1;
    uint32_t k = *B(o + 13);
    c->r[17] = k;
    *W(c->r[29] + 40) = ra;
    if (!((int32_t)((uint32_t)S16(GAME + 16) & 0xFF) < (int32_t)(k + 0x36))) {
        *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
        uint32_t h = *H(GAME + 10);
        if (((h & 0xFF00) & (k << 8)) == 0) *H(GAME + 10) = (uint16_t)(h | (k << 8));
        c->r[17] = 2;
        if (k == 3) {
            ARG(0) = 2; ARG(1) = 0x08A310B8u; ARG(2) = 0; ARG(3) = 1; c->r[8] = 1;
            c->r[31] = 0x088AC17Cu; f_0880AEA8(c);
        } else {
            *H(o + 68) = (uint16_t)-0x20;
            int go = 1;
            if (*B(o + 13) < 2) {
                call2(c, 7, 0x10, 0x088AC1A0u, f_08823338);
                go = c->r[2] != 0;
            }
            if (go && *B(o + 13) == 2) {
                call2(c, 7, 0x11, 0x088AC1BCu, f_08823338);
                if (c->r[2] != 0) {
                    *B(0x092E8800u + 122) = 3;
                    *B(0x092E88A0u + 122) = 1;
                    *B(0x092E8940u + 122) = 2;
                    call2(c, 0x1D, 0, 0x088AC1FCu, f_08823338);
                }
            }
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra;
    c->r[29] += 0x30;
}

/* (projectile): on (+4, +9 / +73 / y step cleared), +344 = -1, its owner's (+88) +24, pictures
   0x08A4A064 / 0x08A49E4C (+128 / +200, 0x08876998); its strength's (+346) speed (0x08A4BC60; 0x50000
   with +3) its way, moved by its ground's (+132) offset (0x08A4BC6C) and 0x44 up, +352 / +356 =
   0x08A49C84 / 0x08A49CA0, 0x0887684C (strength, + 3 with +3) and 0x08900A64. */
SFA_FUNC(0890093C, projectile_start_6) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *B(o + 344) = 0xFF;
    *H(o + 24) = *H(*W(o + 88) + 24);
    *W(o + 128) = 0x08A4A064u;
    *W(o + 200) = 0x08A49E4Cu;
    *W(c->r[29] + 20) = ra;
    call_on(c, o, 0x08900990u, f_08876998);
    *W(o + 100) = 0;
    *B(o + 73) = 0;
    uint32_t k3 = *B(o + 3);
    int32_t k = S8(o + 346);
    uint32_t sp = k3 != 0 ? 0x50000u : *W(0x08A4BC60u + ((uint32_t)k << 2));
    int32_t dx = S16(0x08A4BC6Cu + 2 * (uint32_t)S16(o + 132));
    if (*B(o + 11) == 0) { dx = (int16_t)-dx; sp = (uint32_t)-(int32_t)sp; }
    *W(o + 96) = sp;
    *H(o + 18) = (uint16_t)(S16(o + 18) + dx);
    *H(o + 22) = (uint16_t)(S16(o + 22) + 0x44);
    uint32_t arg = k3 != 0 ? ((uint32_t)k + 3) & 0xFFFF : (uint32_t)k & 0xFFFF;
    *W(o + 352) = 0x08A49C84u;
    *W(o + 356) = 0x08A49CA0u;
    call2(c, o, arg, 0x08900A4Cu, f_0887684C);
    call_on(c, o, 0x08900A54u, f_08900A64);
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (animation, timer): until the timer's +2 reaches the animation's +119, every +82 frames (not while
   GAME +93 / +94) it steps through its list (+92: frames, next) of 32-byte colour rows (+88) and
   copies the row into palette line +73 (0x089E11C8, 0x10 colours) marking them changed; after that
   its last colours (+117 of the timer's +292 list, unless 0xFF) go into line 0x3F and it is freed
   (0x088231C8). */
SFA_FUNC(089CD120, palette_anim_step) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    int32_t lim = (int16_t)*B(o + 119);
    *W(c->r[29] + 16) = s0;
    c->r[16] = o;
    *W(c->r[29] + 20) = ra;
    if (S16(t + 2) < lim) {
        if (S8(GAME + 93) == 0 && S8(GAME + 94) == 0) {
            gaddr rows = *W(o + 88);
            gaddr list = *W(o + 92);
            int16_t h = (int16_t)(S16(o + 82) - 1);
            *H(o + 82) = (uint16_t)h;
            if (h == 0) {
                int32_t step = S8(list + 1);
                *W(o + 88) = rows + ((uint32_t)step << 5);
                list += (uint32_t)(step * 2);
                *W(o + 92) = list;
                *H(o + 82) = (uint16_t)(int32_t)S8(list);
            }
            ARG(0) = ((uint32_t)S8(o + 73) << 5) + 0x092DF980u; ARG(1) = rows; ARG(2) = 0x10;
            c->r[31] = 0x089CD1E0u; f_089E11C8(c);
            *W(0x092E0980u) = 1;
        }
    } else {
        uint32_t idx = *B(o + 117);
        if (idx != 0xFF) {
            ARG(0) = 0x092E0160u; ARG(1) = *W(*W(t + 292) + (idx << 2)); ARG(2) = 0x10;
            c->r[31] = 0x089CD224u; f_089E11C8(c);
            *W(0x092E0980u) = 1;
        }
        call_on(c, o, 0x089CD238u, f_088231C8);
    }
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}

/* (portrait): on (+4, +9 cleared), its owner's (+88) +24 / +14 / side, 0x40 / 0x14 from it, +12 = 1,
   +86 = 0x16, +164 = 1, +13 = slot (+10 % 6) + 0xE; from its entry (0x09525DE8, 20 bytes each)
   +73 and its picture (+84, 0x08ACCC36 + 2), +148 = 3; the colours for its +73 / entry +9 row
   (0x09335A00) copied into palette line 0xE + slot (0x089E11C8) and marked changed. */
SFA_FUNC(088BC520, portrait_colours) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31], s0 = c->r[16];
    c->r[29] -= 0x20;
    uint32_t v = *B(o + 10);
    DIV(c, v, 6);
    gaddr p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 9) = 0;
    *H(o + 24) = *H(p + 24);
    *B(o + 14) = *B(p + 14);
    *B(o + 11) = *B(p + 11);
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0x40);
    *H(o + 22) = (uint16_t)(S16(p + 22) + 0x14);
    *B(o + 12) = 1;
    *W(c->r[29] + 16) = s0;
    c->r[16] = 1;
    *H(o + 86) = 0x16;
    *W(o + 164) = 1;
    uint32_t slot = c->hi;
    *B(o + 13) = (uint8_t)(slot + 0xE);
    gaddr e = *W(0x09525DE8u) + (v << 4) + (v << 2);
    *B(o + 73) = (uint8_t)S8(e + 8);
    int32_t a73 = S8(o + 73);
    *W(o + 148) = 3;
    *H(o + 84) = (uint16_t)(*B(0x08ACCC36u + (uint32_t)a73) + 2);
    int32_t a = S8(e + 9) >> 1;
    uint32_t r = ((uint32_t)(42 * a + a73 + 0xD) & 0xFFFF) << 5;
    *W(c->r[29] + 20) = ra;
    ARG(0) = (slot << 5) + 0x092DFB40u; ARG(1) = 0x09335A00u + r; ARG(2) = 0x10;
    c->r[31] = 0x088BC638u; f_089E11C8(c);
    *W(0x092E0980u) = 1;
    c->r[16] = s0; c->r[31] = ra;
    c->r[29] += 0x20;
}
