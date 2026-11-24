/* Small steps (0.149.0): the next 3 ready functions of 101 - 200 instructions. A network menu row's
   step (its panel growing when chosen), a critter wandering a stage, and the select grid's cells
   barred or shown (with the entry rows). Offsets stay numbers until something shows what they are.
   What the original leaves in a0 - t1 at a call is set as the original sets it.
 */
#include "step.h"

static inline uint8_t  *B(gaddr a) { return guest_ptr(a); }
static inline uint16_t *H(gaddr a) { return guest_ptr(a); }
static inline uint32_t *W(gaddr a) { return guest_ptr(a); }
static inline int8_t  S8(gaddr a)  { return *(int8_t *)guest_ptr(a); }
static inline int16_t S16(gaddr a) { return *(int16_t *)guest_ptr(a); }

#define GAME      0x08E75910u
#define BATTLE    0x092E1228u
#define NET       0x09523980u
#define ROWS      0x08A3F650u   /* the network menu's row records, 12 bytes */
#define CELLS     0x08A28418u   /* the select grid's cells, 6 bytes */
#define CELL_OF   0x08A26F60u   /* bytes: what each grid slot shows */

void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_088231C8(Cpu *c);   /* object_free_32 */
void f_088EB790(Cpu *c);   /* net_menu_on_row */
void f_08821044(Cpu *c);   /* object_move */
void f_08821368(Cpu *c);   /* game_random */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088219B0(Cpu *c);   /* object_animate */
void f_0889472C(Cpu *c);   /* game_select_barred */

/* (row): a network menu row's step: rows 0 - 3, when the chosen row (NET +46) changes, their panel
   (+132 = 0x08A3F650 + 12 * row) set from the tables 0x08A3F6DC / 0x08A3F6FA / 0x08A3F5F0 /
   0x08A3F6E6 / 0x08A3F6F0, shade 0x60, and the chosen one 2 bigger each way (shade 0xFF); row 5 shows
   its marker (kind 0x82 / 1 at +124) once, with the chosen row. Then, once net_menu_on_row (2) says
   so, +129 set, +93 cleared and row 5's marker freed. */
SFA_FUNC(088EBC84, net_menu_row_step) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o; c->r[18] = NET;
    uint32_t t = *B(o + 92);
    if (t <= 3) {
        c->r[17] = 1;
        int8_t sel = S8(NET + 46);
        if ((uint32_t)*B(o + 130) != (uint32_t)(int32_t)sel) {
            *B(o + 130) = (uint8_t)sel;
            gaddr rec = ROWS + t * 12;
            *W(o + 132) = rec;
            *H(rec) = *H(0x08A3F6DCu + t * 2);
            *H(*W(o + 132) + 2) = *H(0x08A3F6FAu + *B(o + 92) * 2);
            *W(*W(o + 132) + 4) = 0x08A3F5F0u + *B(o + 92) * 16;
            *H(*W(*W(o + 132) + 4)) = *H(0x08A3F6E6u + *B(o + 92) * 2);
            *H(*W(*W(o + 132) + 4) + 2) = *H(0x08A3F6F0u + *B(o + 92) * 2);
            *B(o + 137) = 1; *B(o + 136) = 0x60; *B(o + 56) = 1;
            if ((int32_t)*B(o + 92) == S8(NET + 46)) {
                gaddr r = *W(o + 132), sub = *W(r + 4);
                *H(r) = (uint16_t)(S16(r) - 2);
                *H(r + 2) = (uint16_t)(S16(r + 2) - 2);
                *H(sub) = (uint16_t)(*H(sub) + 4);
                *H(sub + 2) = (uint16_t)(*H(sub + 2) + 4);
                *B(o + 136) = 0xFF; *B(o + 56) = 0;
            }
        }
        *B(o + 93) = 1;
    } else if (t == 5) {
        c->r[17] = 1;
        if (*B(o + 130) != 0) {
            *B(o + 130) = 0;
            *W(o + 132) = ROWS + 5 * 12;
            *H(o + 6) = 0x13; *H(o + 10) = 0x178;
            *B(o + 137) = 1; *B(o + 136) = 0xC0;
            c->r[4] = 0x82; c->r[5] = 1; f_08822EAC(c);
            *W(o + 124) = c->r[2];
        }
        *B(*W(o + 124) + 10) = (uint8_t)S8(NET + 46);
        *B(o + 93) = 1;
    }
    c->r[4] = 2; f_088EB790(c);
    if (c->r[2] == 0) {
        *B(o + 129) = 1;
        *B(o + 93) = 0;
        if (*B(o + 92) == 5) { c->r[4] = *W(o + 124); f_088231C8(c); }
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (critter): unless BATTLE +37 / +52, a critter on the ground: walking (+116 0) it moves and animates
   down to y 64; there, with +76, one time in four it stops for 60 - 123 frames and turns its pace by
   its direction (+112); now and then (random with GAME +32) it hops (+100 0x7000, a pull by random);
   with +77 its pace from the direction; at the edges (x 1120 going right, 160 going left) it turns
   (animations 0x08A165A8 / 0x08A165AC). Stopped (+116), it waits, or with +76 clear eases its pace
   toward the direction every frame. Drawn if on screen. */
SFA_FUNC(0885414C, critter_step) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if ((S8(BATTLE + 37) | S8(BATTLE + 52)) == 0) {
        uint32_t w = *B(o + 116);
        c->r[17] = 1;
        if (w == 0) {
            c->r[4] = o; f_08821044(c);
            c->r[4] = o; f_088219B0(c);
            if (S16(o + 22) < 64) *H(o + 22) = 64;
            if (S16(o + 22) == 64) {
                if (S8(o + 76) != 0) {
                    f_08821368(c);
                    if ((c->r[2] & 3) == 0) {
                        f_08821368(c);
                        *B(o + 116) = (uint8_t)((c->r[2] & 0x3F) + 0x3C);
                        *B(o + 76) = 0;
                        *H(o + 82) = 0;
                        if (S16(o + 112) == 1) { *W(o + 96) = 0xA000; *W(o + 104) = 0xFFFFFE00u; }
                        else { *W(o + 96) = 0xFFFF6000u; *W(o + 104) = 0x200; }
                    }
                }
                f_08821368(c);
                if (((c->r[2] + *B(GAME + 32)) & 0x1F) != 0) { *W(o + 100) = 0; *W(o + 108) = 0; }
                else {
                    *W(o + 100) = 0x7000;
                    f_08821368(c);
                    *W(o + 108) = 0xFFFFFC00u - ((c->r[2] & 3) << 8);
                }
            }
            if (S8(o + 77) != 0) {
                uint32_t pace = (uint32_t)((int64_t)S16(o + 112) * 0x15000);
                c->r[18] = pace;
                f_08821368(c);
                *W(o + 96) = pace + ((c->r[2] & 7) << 12);
            }
            int32_t x = S16(o + 18);
            if (S16(o + 112) == 1) {
                if (!(x < 1121)) {
                    *W(o + 96) = 0xFFFEB000u;
                    *H(o + 112) = 0xFFFF;
                    c->r[4] = o; c->r[5] = *W(0x08A165A8u); f_0882196C(c);
                }
            } else if (!(x < 160)) {
            } else {
                *W(o + 96) = 0x15000;
                *H(o + 112) = 1;
                c->r[4] = o; c->r[5] = *W(0x08A165ACu); f_0882196C(c);
            }
        } else {
            if (w == 1) {
                *W(o + 96) = S16(o + 112) == 1 ? 0x15000u : 0xFFFEB000u;
                *W(o + 104) = 0;
            }
            if (S8(o + 76) != 0) *B(o + 116) = (uint8_t)(w - 1);
            else {
                if (S16(o + 82) & 1) { c->r[4] = o; f_088219B0(c); }
                uint32_t v = *W(o + 96) + *W(o + 104);
                if (S16(o + 112) == 1) { if ((int32_t)v < 0) v = 1; }
                else { if ((int32_t)v >= 0) v = 0xFFFFFFFFu; }
                *W(o + 96) = v;
                *W(o + 16) = *W(o + 16) + v;
                *H(o + 82) = (uint16_t)(S16(o + 82) + 1);
            }
        }
    }
    c->r[4] = o; f_08821424(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* (screen): the select grid's 36 cells: a barred character's cell emptied (0xFF in 0x08A26F60 by
   0x08A26FCE) with its picture moved to the second layer (+688); others shown on the first (+4)
   with the picture from 0x08A27020. Then, unless NET +35 or the screen's +184, the entry rows: for
   each side and its three entries (0x09523454, 0x3C apart, 0xD0 a side) the slot (0x08A26FC2) shows
   the entry (+1 * 2 | 0x80 | side, picture by +1, +286 0x27) or none (0xFF, 0x4000, +286 from
   0x08A27080 by side); else the two saved entries cleared (0x354 bytes at 0x09358528 / 0x0935887C). */
SFA_FUNC(08896B8C, select_grid_cells) {
    gaddr o = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20];
    gaddr s21 = c->r[21], s22 = c->r[22], s23 = c->r[23], s30 = c->r[30];
    c->r[29] -= 0x40;
    c->r[16] = o; c->r[17] = 0x08A26FCEu; c->r[18] = 0xFFFFFFFFu; c->r[19] = CELL_OF;
    c->r[20] = 0x08A2708Cu; c->r[21] = CELLS; c->r[22] = 0x08A27020u;
    for (uint32_t i = 0; i < 36; i++) {
        c->r[30] = i; c->r[23] = 0x08A27020u + i * 2;
        c->r[4] = i; f_0889472C(c);
        int32_t k = S8(0x08A2708Cu + i);
        uint16_t pic = *H(0x08A27020u + i * 2);
        gaddr e = CELLS + (uint32_t)k * 6;
        if (c->r[2] != 0) {
            *B(CELL_OF + (uint32_t)(int32_t)S8(0x08A26FCEu + i)) = 0xFF;
            *H(e + 4) = (uint16_t)((*H(e + 4) & 0x8000) | 0x4000);
            *H(e + 688) = (uint16_t)((*H(e + 688) & 0x8000) | pic);
        } else {
            *H(e + 688) = (uint16_t)((*H(e + 688) & 0x8000) | 0x4000);
            *H(e + 4) = (uint16_t)((*H(e + 4) & 0x8000) | pic);
        }
    }
    if (*B(NET + 35) == 0 && S8(o + 184) == 0) {
        gaddr row = 0x09523454u, tail = 0x08A27074u;
        for (uint32_t side = 0; side < 2; side++, row += 0xD0, tail += 6) {
            gaddr ent = row;
            for (uint32_t j = 0; j < 3; j++, ent += 0x3C) {
                uint32_t n = j * 2 + side;
                gaddr slot = CELL_OF + (uint32_t)(int32_t)S8(0x08A26FC2u + n);
                gaddr e = CELLS + n * 6;
                if (S8(ent) != 0) {
                    int32_t ch = S8(ent + 1);
                    *B(slot) = (uint8_t)((uint32_t)(ch << 1) | 0x80 | side);
                    *H(e + 286) = 0x27;
                    *H(e + 4) = *H(0x08A27020u + (uint32_t)ch * 2);
                } else {
                    *B(slot) = 0xFF;
                    *H(e + 4) = 0x4000;
                    *H(e + 286) = *H(tail + 12);
                }
            }
        }
    } else {
        for (uint32_t i = 0; i < 0x354; i++) *B(0x09358528u + i) = 0;
        for (uint32_t i = 0; i < 0x354; i++) *B(0x0935887Cu + i) = 0;
    }
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20;
    c->r[21] = s21; c->r[22] = s22; c->r[23] = s23; c->r[30] = s30;
}
