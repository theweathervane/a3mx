/* More middle-sized steps (0.108.0): set-ups from tables, objects made beside others, the CPU's script
 * commands that test its opponent, fighters' press counters and the particle field's start. Offsets
 * stay numbers until something shows what they are. Arguments of native callees, and what the original
 * leaves in a0 - a3 / t0 at a call through a table, are set as the original sets them; s0 - s4 as the
 * original sets them around calls.
 */
#include "../decomp.h"

#define GAME    0x08E75910u
#define BATTLE  0x092E1228u

#include "../guest_mem.h"

void f_0887BBC8(Cpu *c);   /* fighter_free_to_act */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_08877F44(Cpu *c);   /* fighter_to_routine_1 */
void f_088347B4(Cpu *c);   /* fighter_set_412 */
void f_08821424(Cpu *c);   /* object_draw_if_on_screen */
void f_08823338(Cpu *c);   /* object_make_list_3 */
void f_08822EAC(Cpu *c);   /* object_make_32 */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088215C4(Cpu *c);   /* object_draw */
void f_08855D80(Cpu *c);   /* intro_place */
void f_08881F70(Cpu *c);   /* make_beside_15 */
void f_0887D990(Cpu *c);   /* object_set_animation_by_type */
void f_0889C0D4(Cpu *c);   /* row_by_width */
void f_0889CFE8(Cpu *c);   /* record_setup_layer */
void f_088AB1D8(Cpu *c);   /* set_27b0_2810 */
void f_088AC37C(Cpu *c);   /* count_88_to_21 */
void f_08821330(Cpu *c);   /* object_routine_by_14 */
void f_088BAC18(Cpu *c);   /* game_option_pulse */

static void set_anim(Cpu *c, gaddr o, uint32_t anim) { c->r[4] = o; c->r[5] = anim; f_0882196C(c); }
static gaddr side_record(Cpu *c, int32_t n) {
    int64_t m = (int64_t)n * 0x354;
    c->lo = (uint32_t)m; c->hi = (uint32_t)(m >> 32);
    return 0x09519588u + (uint32_t)m;
}

/* ---------------------------------------------------------------- the CPU's script commands */

/* (fighter, script): read on (0x0887A10C) while the fighter is free to act (0x0887BBC8); otherwise
   +482 = 0, +483 = 10 and back to routine 1 (0x08877F44). */
SFA_FUNC(08877B58, fighter_script_cmd_free) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = p; c->r[17] = f;
    f_0887BBC8(c);
    if (c->r[2] != 0) {
        c->r[4] = f; c->r[5] = p;
        f_0887A10C(c);
    } else {
        *B(f + 482) = 0;
        *B(f + 483) = 10;
        c->r[4] = f; c->r[5] = p;
        f_08877F44(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, script): script command: +490 and +506 from the script; at or past the level (+508) read
   on, otherwise the script ends (state 1, step 2, +483 = 5). */
SFA_FUNC(088781C4, fighter_script_cmd_490_506) {
    gaddr f = ARG(0), p = ARG(1);
    int32_t v = S16(p), w = S16(p + 2), lv = S16(f + 508);
    *H(f + 506) = (uint16_t)w;
    *H(f + 490) = (uint16_t)v;
    int32_t d = (int16_t)(v - lv);
    c->r[2] = p + 4;
    c->r[29] -= 0x20;
    if (d >= 0) {
        c->r[5] = p + 4; c->r[6] = (uint32_t)d; c->r[7] = (uint32_t)(v - lv); c->r[8] = (uint32_t)lv;
        f_0887A10C(c);
    } else {
        *B(f + 482) = 1; *B(f + 483) = 5;
        *B(f + 4) = 1; *B(f + 6) = 2; *B(f + 7) = 0; *B(f + 5) = 0;
        *B(f + 78) = 0; *B(f + 497) = 0;
        c->r[5] = 1; c->r[6] = 2;
    }
    c->r[29] += 0x20;
}

/* (fighter, script): read on when the opponent (+92) has +73 = 1 and +78 = 1; otherwise the block is left. */
SFA_FUNC(08878F40, fighter_script_cmd_73_78) {
    gaddr f = ARG(0), p = ARG(1), q = *W(f + 92);
    int read = S8(q + 73) == 1 && S8(q + 78) == 1;
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (read) f_0887A10C(c); else f_08879C8C(c);
    c->r[29] += 0x20;
}

/* (fighter, script): the block is left when the opponent (+92) has +73 = 1 and +100 not negative;
   otherwise read on. */
SFA_FUNC(088793A8, fighter_script_cmd_73_100) {
    gaddr f = ARG(0), p = ARG(1), q = *W(f + 92);
    int leave = S8(q + 73) == 1 && (int32_t)*W(q + 100) >= 0;
    c->r[29] -= 0x20;
    c->r[4] = f; c->r[5] = p; c->r[6] = p;
    if (leave) f_08879C8C(c); else f_0887A10C(c);
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- fighters' press counters */

/* (a, b, fighter): +347 counts down: in between +412 = 10 (0x088347B4); at 0 +341 moves on, +361 and
   +352 cleared, the side's bit (+337) cleared in a's +53, and +412 = 0. */
SFA_FUNC(08834558, fighter_count_347) {
    gaddr a = ARG(0), f = ARG(2);
    c->r[29] -= 0x20;
    uint8_t n = (uint8_t)(*B(f + 347) - 1);
    *B(f + 347) = n;
    if (n != 0) {
        c->r[4] = f; c->r[5] = 10;
        f_088347B4(c);
    } else {
        *B(f + 341) = (uint8_t)(S8(f + 341) + 1);
        *B(f + 361) = 0;
        *W(f + 352) = 0;
        *B(a + 53) = (uint8_t)(S8(a + 53) & ~(1 << (S8(f + 337) & 31)));
        *B(f + 412) = 0;
        c->r[5] = f; c->r[8] = 1;
    }
    c->r[29] += 0x20;
}

/* (a, b, fighter): unless b's +6 is clear and (a's +43 is clear or b's +4 is set), +412 = 0 and +347
   counts down to 0; otherwise the press step +341 from 0x08A0F8F0 with (a, b, fighter). */
SFA_FUNC(08834430, fighter_press_step) {
    gaddr a = ARG(0), b = ARG(1), f = ARG(2);
    c->r[29] -= 0x20;
    if (S8(b + 6) == 0 && (S8(a + 43) == 0 || S8(b + 4) != 0)) {
        gaddr fn = *W(0x08A0F8F0u + (uint32_t)(int32_t)S8(f + 341) * 4);
        c->r[4] = a; c->r[5] = b; c->r[6] = f; c->r[7] = f; c->r[8] = fn;
        rt_call_indirect(c, fn);
    } else {
        *B(f + 412) = 0;
        int32_t n = *B(f + 347);
        if (n > 0) *B(f + 347) = (uint8_t)(n - 1);
        c->r[4] = f; c->r[5] = (uint32_t)n;
    }
    c->r[29] += 0x20;
}

/* ---------------------------------------------------------------- objects */

/* (object): +82 counts down and it is drawn if on screen; past 0, +5 moves on, battle state 6 and a
   list-3 object (kind 5, type 1) owned by it. */
SFA_FUNC(08855470, count_82_battle_6) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n >= 0) {
        c->r[4] = o;
        f_08821424(c);
    } else {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *B(BATTLE + 44) = 6;
        c->r[4] = 5; c->r[5] = 1;
        f_08823338(c);
        if (c->r[2] != 0) *W(c->r[2] + 124) = o;
        c->r[4] = c->r[2];
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): placed (0x08855D80), +4 moves on and it starts moving: +82 = 16, +112 and a speed of
   +96 / +100, draw state 0x08A16962 and animation +60 / +64 of 0x08A16A90. */
static void intro_start(Cpu *c, uint32_t layer, uint32_t x, uint32_t y, uint16_t at112, uint32_t speed,
                        uint32_t anim_at) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[5] = layer; c->r[6] = x; c->r[7] = y;
    f_08855D80(c);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *H(o + 82) = 0x10;
    *H(o + 112) = at112;
    *W(o + 100) = speed;
    *W(o + 96) = speed;
    *W(o + 148) = 0xB;
    *W(o + 156) = 0x08A16962u;
    set_anim(c, o, *W(0x08A16A90u + anim_at));
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(08856C60, intro_start_left)  { intro_start(c, 4, 0x9A, 0x160, 0x8C, 0xFFF00000u, 60); }
SFA_FUNC(088572E4, intro_start_right) { intro_start(c, 5, 0xE6, 0xFFFFFFA0u, 0x74, 0x00100000u, 64); }

/* (object): +80 set, the camera layer 0x092E8940 +122 = 2, and 0x08B02810 / 14 from entry +15 of
   0x08A2C6C4 (six, round again), +15 moving on. */
SFA_FUNC(088AADD8, next_2810_entry) {
    gaddr o = ARG(0);
    *B(o + 80) = 1;
    if (*B(o + 15) >= 6) *B(o + 15) = 0;
    *B(0x092E8940u + 122) = 2;
    gaddr e = 0x08A2C6C4u + (uint32_t)*B(o + 15) * 4;
    *W(0x08B02810u) = (uint32_t)(int32_t)S16(e) << 16;
    *W(0x08B02814u) = (uint32_t)(int32_t)S16(e + 2) << 16;
    *B(o + 15) = (uint8_t)(*B(o + 15) + 1);
}

/* (object): y from 0x08A3CAF2 by its parent's (+124) +13 unless +94; shown (+93) in mode 2 when the
   parent's +2 is 1, or mode 3 when its +1 is 1. */
SFA_FUNC(088CFB7C, option_y_show_by_mode) {
    gaddr o = ARG(0), p = *W(o + 124);
    if (S16(o + 94) == 0) *H(o + 10) = (uint16_t)S16(0x08A3CAF2u + (uint32_t)(int32_t)S8(p + 13) * 2);
    *B(o + 93) = 0;
    uint32_t m = *H(GAME + 6);
    if ((m == 2 && S8(p + 2) == 1) || (m == 3 && S8(p + 1) == 1)) *B(o + 93) = 1;
}

/* (object): while +78 has bit 2 and it moves (+100 or +108), y (+22) is held at +114 once it gets
   there (from either side by +100's sign) and bit 2 cleared; answers 0 while still on its way. */
SFA_FUNC(089CF0A4, stop_at_114) {
    gaddr o = ARG(0);
    int32_t fl = S8(o + 78);
    if ((fl & 2) == 0) { RETURN(1); return; }
    uint32_t a = *W(o + 100);
    if ((a | *W(o + 108)) != 0) {
        int32_t y = S16(o + 22), lim = S16(o + 114);
        int32_t ny = y, nl = lim;
        if ((int32_t)a < 0) { ny = (int16_t)(-y); nl = (int16_t)(-lim); }
        if (ny < nl) { RETURN(0); return; }
        *H(o + 22) = (uint16_t)lim;
    }
    *B(o + 78) = (uint8_t)(fl & ~2);
    RETURN(1);
}

/* (object): four kind-15 objects beside it (0x08881F70), each moved along x by the table at 0x08A195C0
   / 0x08A195C8 (last first; mirrored by +11), stopping when none is made. */
static void four_beside(Cpu *c, gaddr table) {
    gaddr o = ARG(0), s16 = c->r[16], s17 = c->r[17], s18 = c->r[18];
    c->r[29] -= 0x20;
    c->r[16] = o;
    gaddr t = table + 6;
    for (int32_t n = 3; n >= 0; n--, t -= 2) {
        c->r[17] = t; c->r[18] = (uint32_t)n;
        c->r[4] = o;
        f_08881F70(c);
        gaddr r = c->r[2];
        if (r == 0) break;
        int32_t x = S16(r + 18), dx = S16(t);
        *H(r + 18) = (uint16_t)(*B(o + 11) != 0 ? x - dx : x + dx);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17; c->r[18] = s18;
}
SFA_FUNC(08863748, four_beside_95c0) { four_beside(c, 0x08A195C0u); }
SFA_FUNC(088637CC, four_beside_95c8) { four_beside(c, 0x08A195C8u); }

/* (object): hidden, +4 moves on, set up at (0, 0) as picture 0xD with animation GAME +6 of
   0x08A3C8A0's list. */
SFA_FUNC(088CE97C, setup_by_mode_hidden) {
    gaddr o = ARG(0);
    *B(o + 1) = 0;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 0;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *B(o + 12) = 1;
    *B(o + 13) = 0xD;
    *H(o + 24) = 6;
    *B(o + 9) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 18) = 0;
    *H(o + 22) = 0;
    gaddr e = (uint32_t)*H(GAME + 6) * 4 + 0x08A3C87Cu;
    c->r[29] -= 0x20;
    c->r[6] = 0x08A3C87Cu;
    set_anim(c, o, *W(e + 36));
    c->r[29] += 0x20;
}

/* (object): +82 counts down; at 0 +6 moves on and it comes in from x -0x60 at 0x18000 with the
   animation 0x08A16B10, drawn. */
SFA_FUNC(08855774, count_82_come_in) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    int32_t n = (int16_t)(S16(o + 82) - 1);
    *H(o + 82) = (uint16_t)n;
    if (n == 0) {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        *W(o + 148) = 1;
        *H(o + 82) = 0x79;
        *H(o + 18) = 0xFFA0;
        *W(o + 96) = 0x18000;
        set_anim(c, o, *W(0x08A16B10u));
        c->r[4] = o;
        f_088215C4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): y from its owner's (+88) +132, and a kind-4 object (step 0x14) made on it, its own +92
   pointing back; then the animation by type. */
SFA_FUNC(0887D738, make_shadow_on_owner) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 9) = 0;
    *H(o + 22) = (uint16_t)S16(*W(o + 88) + 132);
    c->r[4] = 4; c->r[5] = 0x14;
    f_08822EAC(c);
    gaddr r = c->r[2];
    if (r != 0) {
        *H(r + 18) = (uint16_t)S16(o + 18);
        *H(r + 22) = (uint16_t)S16(o + 22);
        *B(r + 121) = (uint8_t)S8(o + 121);
        *B(r + 135) = 0xFF;
        *W(r + 88) = *W(o + 88);
        *H(r + 82) = 0;
        *W(r + 92) = o;
        *B(r + 119) = 0xFF;
        c->r[5] = 0xFF; c->r[6] = 0xFFFFFFFFu;
    }
    c->r[4] = o;
    f_0887D990(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object, fighter): with the fighter's +424 bit 0 +5 moves on; otherwise +6 moves on, the record set
   up on layer 1, a speed of 1.5 towards the fighter's side (+337), and the row (0x0889C0D4). */
SFA_FUNC(0889C04C, row_start_by_side) {
    gaddr o = ARG(0), f = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[16] = f; c->r[17] = o;
    if (S16(f + 424) & 1) {
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
    } else {
        *B(o + 6) = (uint8_t)(*B(o + 6) + 1);
        c->r[4] = o; c->r[5] = 1;
        f_0889CFE8(c);
        *W(o + 96) = S8(f + 337) == 0 ? 0x00018000u : 0xFFFE8000u;
        c->r[4] = o; c->r[5] = f;
        f_0889C0D4(c);
    }
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (object): +13 counts down: at 0 GAME +22 gets bit 4, +9 moves on and 0x08B027B0 / 2810 take entry 8;
   under 8 GAME +22 gets bit 2 and the entry flashes 0 / 8; otherwise entry 0. */
SFA_FUNC(088AB150, flash_13_entry) {
    gaddr o = ARG(0);
    uint8_t n = (uint8_t)(*B(o + 13) - 1);
    *B(o + 13) = n;
    uint32_t e = 0;
    if (n == 0) {
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 4);
        *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
        e = 8;
    }
    if (*B(o + 13) < 8) {
        *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 2);
        e = ((uint32_t)*B(o + 13) & 1) << 3;
    }
    c->r[29] -= 0x20;
    c->r[5] = e;
    f_088AB1D8(c);
    c->r[29] += 0x20;
}

/* (object): each time +88 runs to 21 (0x088AC37C): +13 moves on to 3 with +12 = 0xB0 and GAME +22's low
   byte 0xF; at 3 +9 = 4. +9 then counts down. */
SFA_FUNC(088AC210, count_13_to_3) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088AC37C(c);
    if (c->r[2] == 0) {
        uint32_t k = *B(o + 13);
        if (k != 3) {
            *B(o + 13) = (uint8_t)(k + 1);
            *B(o + 12) = 0xB0;
            *H(GAME + 22) = (uint16_t)(S16(GAME + 22) & 0xFF00);
            *H(GAME + 22) = (uint16_t)(S16(GAME + 22) | 0xF);
        } else {
            *B(o + 9) = 4;
        }
        *B(o + 9) = (uint8_t)(*B(o + 9) - 1);
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): the particle field's 66 entries (0x08B0285C, 16 bytes each): +6 from the eleven at 0x08A37628 in
   turn, +14 from 0x08A3763E's list, both x 32. */
SFA_FUNC(088B5334, particles_start) {
    (void)c;
    gaddr e = 0x08B0285Cu;
    for (uint32_t k = 0; k < 66; k++, e += 16) {
        *H(e + 6) = (uint16_t)(S16(0x08A37628u + (k % 11) * 2) << 5);
        *H(e + 14) = (uint16_t)(S16(0x08A3763Eu + k * 2) << 5);
    }
}

/* (object): +13 flips with +121; once y reaches its routine entry's +22 + 0x30, it stops there, +5 moves
   on, GAME +11 = 1 and +13 = 0xB. */
SFA_FUNC(088BD268, drop_to_routine_y) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    if (S8(o + 121) != 0) *B(o + 13) = (uint8_t)(*B(o + 13) ^ 1);
    c->r[4] = o;
    f_08821330(c);
    uint32_t y = (uint32_t)(S16(c->r[2] + 22) + 0x30);
    if ((int16_t)y >= S16(o + 22)) {
        *H(o + 22) = (uint16_t)y;
        *B(o + 5) = (uint8_t)(*B(o + 5) + 1);
        *B(GAME + 11) = 1;
        *B(o + 13) = 0xB;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): shown, +4 moves on, set up as picture 0xE at its parent's (+88) place + (0x72, -8) with
   the animation 0x08A3C8B8. */
SFA_FUNC(088CEA34, setup_beside_parent_e) {
    gaddr o = ARG(0), p = *W(o + 88);
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *W(o + 148) = 0;
    *H(o + 86) = 0x12;
    *W(o + 164) = 0;
    *B(o + 12) = 1;
    *B(o + 13) = 0xE;
    *H(o + 24) = 6;
    *B(o + 9) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *H(o + 18) = (uint16_t)(S16(p + 18) + 0x72);
    *H(o + 22) = (uint16_t)(S16(p + 22) - 8);
    c->r[29] -= 0x20;
    c->r[6] = 0x08A40000u; c->r[7] = 0xE;
    set_anim(c, o, *W(0x08A3C8B8u));
    c->r[29] += 0x20;
}

/* (object): in mode 2, the option pulse and shown (+93) while GAME +4 is set; +136 cleared when its
   parent's (+124) +1 is 2. */
SFA_FUNC(088CF880, option_pulse_mode_2) {
    gaddr o = ARG(0), s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    gaddr p = *W(o + 124);
    c->r[29] -= 0x30;
    c->r[16] = o; c->r[17] = 2; c->r[18] = p; c->r[19] = GAME;
    *B(o + 93) = 0;
    if (*H(GAME + 6) == 2) {
        c->r[4] = o;
        f_088BAC18(c);
        *B(o + 93) = *H(GAME + 4) != 0 ? 1 : 0;
        c->r[18] = (uint32_t)(int32_t)S8(p + 1);
        if ((int32_t)c->r[18] == 2) *B(o + 136) = 0;
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (object): shown, +4 moves on, set up as picture 3 with +84 from 0x08ACCB10 by its parent's (+88) side
   record. */
SFA_FUNC(088D06B0, setup_by_side_record) {
    gaddr o = ARG(0);
    gaddr r = side_record(c, S8(*W(o + 88) + 78));
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *B(o + 1) = 1;
    *B(o + 14) = 0;
    *B(o + 9) = 0;
    *H(o + 24) = 6;
    *B(o + 11) = 0;
    *B(o + 12) = 1;
    *B(o + 13) = 3;
    *H(o + 86) = 0x12;
    *W(o + 148) = 3;
    *H(o + 84) = (uint16_t)(*B(0x08ACCB10u + (uint32_t)(int32_t)S8(r + 1)) + 0x51);
}
