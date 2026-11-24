/* Steps (0.155.0): the next ready function of 201 - 400 instructions: a network menu item's step (the
   menu's state copied for the status line, the item shown and run only on the chosen row when its
   option is allowed). Offsets stay numbers until something shows what they are. What the original
   leaves in a0 - a3 at a call is set as the original sets it.
 */
#include "step.h"

#include "../guest_mem.h"

#define GAME      0x08E75910u
#define NET       0x09523980u
#define NET_PEER  0x09523C00u   /* the other side's settings */
#define STATUS    0x08B03F8Au   /* the network menu's status bytes */
#define ROW_OPTS  0x08A3EA34u   /* 11 bytes a row: which options it allows */

void rt_call_indirect(Cpu *c, uint32_t addr);
void f_088215C4(Cpu *c);   /* object_draw */

/* (item): the status bytes (+0 - +8) from this side's settings (GAME +46 / +77 / +47 / +119 / +156,
   +118 / +48) or, with NET +263, the other side's (0x09523C00 +2 - +8); then, off the chosen row
   (NET +47), the item steps away (+4 down on a later row, up on row 0) hidden; on it, unless its type
   (+3, table 0x08A016F0) names an option the row doesn't allow (0x08A3EA34) or is 20 / 25, its routine
   from 0x08A3F7A8 runs, and it is drawn while +1 is set. */
SFA_FUNC(088ED1C8, net_menu_item_step) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(NET + 263) == 0) {
        *B(STATUS + 2) = (uint8_t)S8(GAME + 46);
        *B(STATUS + 1) = (uint8_t)S8(GAME + 77);
        *B(STATUS) = (uint8_t)S8(GAME + 47);
        *B(STATUS + 4) = S8(NET_PEER) == 0;
        *B(STATUS + 5) = (uint8_t)S8(GAME + 119);
        *B(STATUS + 6) = (uint8_t)S8(GAME + 156);
        *B(STATUS + 7) = (uint8_t)S8(NET + 265);
        *B(STATUS + 8) = (uint8_t)S8(NET + 266);
        int32_t k = S8(GAME + 118);
        *B(STATUS + 3) = k == 1 ? 0x81 : k == 2 ? 0x82 : (uint8_t)(S8(GAME + 48) & 0xF);
    } else {
        *B(STATUS + 2) = (uint8_t)S8(NET_PEER + 4);
        *B(STATUS + 1) = (uint8_t)S8(NET_PEER + 3);
        *B(STATUS) = (uint8_t)S8(NET_PEER + 2);
        *B(STATUS + 4) = S8(NET_PEER) == 0;
        *B(STATUS + 5) = (uint8_t)S8(NET_PEER + 7);
        *B(STATUS + 6) = (uint8_t)S8(NET_PEER + 8);
        *B(STATUS + 7) = (uint8_t)S8(NET + 265);
        *B(STATUS + 8) = (uint8_t)S8(NET + 266);
        int32_t k = S8(NET_PEER + 6);
        *B(STATUS + 3) = k == 1 ? 0x81 : k == 2 ? 0x82 : (uint8_t)(S8(NET_PEER + 5) & 0xF);
    }
    int32_t sel = S8(NET + 47);
    if ((int32_t)*B(o + 10) != sel) {
        *B(o + 1) = 0;
        *B(o + 4) = (uint8_t)(*B(o + 4) + (sel != 0 ? -1 : 1));
    } else {
        uint32_t t = *B(o + 3);
        int run = 1;
        c->r[7] = t < 0x27u;
        if (t >= 0x27u) run = 0;
        else {
            uint32_t to = *W(0x08A016F0u + t * 4);
            int off = -1;
            switch (to) {
            case 0x088ED388u: off = 1; break;
            case 0x088ED3B8u: off = 2; break;
            case 0x088ED3E8u: off = 3; break;
            case 0x088ED418u: off = 4; break;
            case 0x088ED448u: off = 7; break;
            case 0x088ED478u: off = 8; break;
            case 0x088ED4A8u: off = 0; break;
            case 0x088ED4D8u: off = 5; break;
            case 0x088ED508u: off = 6; break;
            case 0x088ED538u: off = 9; break;
            case 0x088ED568u: off = 10; break;
            case 0x088ED594u: break;
            case 0x088ED59Cu: run = 0; break;
            default: rt_bad_jump(c, to, 0x088ED380u); return;
            }
            if (off >= 0) {
                c->r[7] = ROW_OPTS;
                if (*B(ROW_OPTS + (uint32_t)sel * 11 + (uint32_t)off) == 0) run = 0;
            }
        }
        if (t == 20 || t == 25) run = 0;
        if (run) {
            c->r[4] = o; c->r[5] = t; c->r[6] = 1;
            rt_call_indirect(c, *W(0x08A3F7A8u + t * 4));
        } else *B(o + 1) = 0;
    }
    if (*B(o + 1) != 0) { c->r[4] = o; f_088215C4(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
