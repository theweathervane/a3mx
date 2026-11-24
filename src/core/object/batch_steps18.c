/* Small steps (0.147.0): the 2 functions of 9 - 100 instructions made ready by the batch before and the
   next 2 of 101 - 200. A row object's step by type, the network matching for a thread's record, the
   pause menu's texts, and a text scroller. Offsets stay numbers until something shows what they are.
   What the original leaves in a0 - t2 at a call is set as the original sets it.
 */
#include "step.h"
#include "../psp_os.h"

#include "../guest_mem.h"

void f_089DDA68(Cpu *c);   /* panel_by_type_6f20 */
void f_089DDB68(Cpu *c);   /* turn_then_free_with_parent */
void f_089DDBD0(Cpu *c);   /* row_start_by_92 */
void f_089DDCD4(Cpu *c);   /* banner_fade_step */
void f_088DE734(Cpu *c);   /* net_matching_start */
void f_088DF0B8(Cpu *c);   /* adhoc_matching_end */
void f_0880A6CC(Cpu *c);   /* game_text */

/* (object): by its type (+92 below 16 or not) and whether it has started (+128): a panel, then turning
   away; or a row's start, then its fade. */
SFA_FUNC(089DD9F0, row_object_step) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t started = *B(o + 128);
    c->r[5] = started;
    if (*B(o + 92) < 16) { if (started == 0) f_089DDA68(c); else f_089DDB68(c); }
    else { if (started == 0) f_089DDBD0(c); else f_089DDCD4(c); }
    c->r[29] += 0x20;
}

/* (thread record, data): ad hoc matching in the record's mode (+24) with the data's +0x88 (4 bytes);
   then the ad hoc control parameters into +0x1C. On a failure the matching ended. The result. */
SFA_FUNC(088DF10C, net_matching_for_record) {
    gaddr t = ARG(0), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = t;
    c->r[4] = *W(t + 24); c->r[5] = ARG(1) + 0x88; c->r[6] = 4;
    f_088DE734(c);
    int32_t r = (int32_t)c->r[2];
    if (r >= 0) r = os_net_adhocctl_get_parameter(c, t + 0x1C);
    c->r[17] = (uint32_t)r;
    if (r < 0) { c->r[4] = t; f_088DF0B8(c); }
    RETURN((uint32_t)r);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (game, _, record, confirm): the pause menu's texts: the title by the first side paused (the game's
   +89 bits 0 - 2: 0x089FE360 / 0x089FE36C / 0x089FE378 at 0x90, 0x58); then the four rows of
   0x08A0F96C (text, font, x, y; 12 bytes) with the chosen one (+8) lit (3, else 9); or, to confirm, the
   question 0x08A0F960 (its byte 9 set to 0x9C) and its two answers 0x089FE384 / 0x089FE388 lit by +8. */
SFA_FUNC(08834FF0, pause_menu_texts) {
    gaddr g = ARG(0), rec = ARG(2);
    uint32_t confirm = ARG(3);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x30;
    c->r[16] = rec; c->r[17] = confirm;
    uint32_t m = (uint32_t)(int32_t)S8(g + 89), i = 0;
    while (!(m & (1u << i))) { i = (i + 1) & 0xFF; if (!((int32_t)i < 3)) break; }
    c->r[4] = i == 0 ? 0x089FE360u : i == 1 ? 0x089FE36Cu : 0x089FE378u;
    c->r[5] = 3; c->r[6] = 0x90; c->r[7] = 0x58; c->r[8] = 2;
    f_0880A6CC(c);
    if (confirm == 0) {
        gaddr p = 0x08A0F96Cu;
        for (uint32_t j = 0; j < 4; j++, p += 12) {
            c->r[18] = j; c->r[17] = p;
            uint32_t txt = *W(p);
            if (txt == 0) continue;
            c->r[8] = j == *B(rec + 8) ? 3 : 9;
            c->r[4] = txt; c->r[5] = *B(p + 4);
            c->r[6] = (uint32_t)(int32_t)S16(p + 6);
            c->r[7] = c->r[9] = (uint32_t)(int32_t)S16(p + 8);
            f_0880A6CC(c);
        }
    } else {
        *B(0x08A0F969u) = 0x9C;
        c->r[4] = 0x08A0F960u; c->r[5] = 3; c->r[6] = 0x84; c->r[7] = 0x68; c->r[8] = 9;
        f_0880A6CC(c);
        c->r[17] = 0x089FE388u;
        c->r[4] = 0x089FE384u; c->r[5] = 3; c->r[6] = 0xAE; c->r[7] = 0x78; c->r[8] = *B(rec + 8) != 0 ? 9 : 3;
        f_0880A6CC(c);
        uint32_t sel = *B(rec + 8);
        c->r[16] = sel;
        c->r[4] = 0x089FE388u; c->r[5] = 3; c->r[6] = 0xB4; c->r[7] = 0x88; c->r[8] = sel != 0 ? 3 : 9;
        f_0880A6CC(c);
    }
    c->r[29] += 0x30;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}

/* The top 16 bits of a 16.16 value, rounded toward 0. */
static int32_t whole(uint32_t v) {
    uint32_t round = (uint32_t)((int32_t)v >> 16) >> 16;
    return (int32_t)(v + round) >> 16;
}

/* (scroller): a text scroller: +24 (x, 16.16) and +28 (y) moved by the speed +20; past y -23 the next
   line (+40) comes in 16 lower. Up to 18 lines from 0x08AB7378 (by +40; 0xFFFF ends the scroller, +4 on;
   0 a blank line) drawn from the line table 0x08AB6F60 (8 bytes: colour, x, text) at 16-pixel steps,
   each nearer line further right by the speed's slope. */
SFA_FUNC(089DDEEC, text_scroller) {
    gaddr o = ARG(0);
    gaddr s16 = c->r[16], s17 = c->r[17], s18 = c->r[18], s19 = c->r[19], s20 = c->r[20], s21 = c->r[21], s22 = c->r[22];
    c->r[29] -= 0x40;
    c->r[16] = o;
    uint32_t speed = *W(o + 20);
    *W(o + 28) = *W(o + 28) - speed;
    uint32_t acc = *W(o + 24) + speed;
    *W(o + 24) = acc;
    int32_t y = S16(o + 30);
    if (y < -23) {
        DIV(c, speed << 12, speed);
        *H(o + 40) = (uint16_t)(S16(o + 40) + 1);
        *H(o + 30) = (uint16_t)(y + 0x10);
        y = S16(o + 30);
        acc -= c->lo << 8;
        *W(o + 24) = acc;
    }
    int32_t x = (int16_t)whole(acc);
    c->r[20] = 0x08AB7378u; c->r[22] = 0x08AB6F60u; c->r[21] = 0xFFFF;
    for (int32_t i = 0; i < 18; i = (int16_t)(i + 1)) {
        c->r[19] = (uint32_t)i; c->r[18] = (uint32_t)y; c->r[17] = acc;
        uint32_t idx = (uint32_t)(S16(o + 40) + i);
        uint32_t line = *H(0x08AB7378u + idx * 2);
        if (line == 0xFFFF) {
            *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
            goto done;
        }
        if (line != 0) {
            DIV(c, speed << 12, speed);
            gaddr e = 0x08AB6F60u + line * 8;
            int32_t off = whole(c->lo * 4352);
            c->r[9] = (uint32_t)off;
            c->r[4] = *W(e + 4); c->r[5] = 7;
            c->r[6] = (uint32_t)(int32_t)(int16_t)(x + S16(e + 2) - off + 0x110);
            c->r[7] = (uint32_t)y;
            c->r[8] = *H(e) & 0xFF;
            f_0880A6CC(c);
            speed = *W(o + 20);
        }
        DIV(c, speed << 12, speed);
        y = (int16_t)(y + 0x10);
        acc -= c->lo << 8;
        x = (int16_t)whole(acc);
    }
done:
    c->r[29] += 0x40;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18; c->r[19] = s19; c->r[20] = s20; c->r[21] = s21; c->r[22] = s22;
}
