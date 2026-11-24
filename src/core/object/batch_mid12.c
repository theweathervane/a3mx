/* More steps (0.121.0): objects that bounce or count down and then animate, a fighter's script steps
 * and the CPU's choice of a throw or an attack on the opponent, the camera lines' starts, the fight's
 * objects reset, particles and panels set up. Offsets stay numbers until something shows what they
 * are. Arguments of callees, and what the original leaves in a0 - t3 at a call, are set as the
 * original sets them; s0 - s3 as the original sets them around calls.
 */
#include "../decomp.h"

#define BATTLE  0x092E1228u
#define P3      0x092E47D0u
#define P4      0x092E4C68u
#define CAMERA_TABLES 0x0951F950u   /* three of 152 bytes (+2 / +154 / +306 a flag, +10 / +14 the place) */
#define LAYER_0 0x092E8800u
#define LAYER_1 0x092E88A0u

#include "../guest_mem.h"

void f_08821AA8(Cpu *c);   /* object_next_frame */
void f_089CEFC8(Cpu *c);   /* bounce_by_88 */
void f_088B4C74(Cpu *c);   /* particle_start_by_78 */
void f_089E0178(Cpu *c);   /* camera_line_start */
void f_088AADD8(Cpu *c);   /* next_2810_entry */
void f_088AE240(Cpu *c);   /* draw_until_16_changes */
void f_088230F4(Cpu *c);   /* object_clear_for_use */
void f_08869AC8(Cpu *c);   /* fighter_press_three */
void f_0887E7AC(Cpu *c);   /* make_from_description */
void f_08822144(Cpu *c);   /* fight_objects_reset */
void f_08822808(Cpu *c);   /* pools_168_reset */
void f_08822934(Cpu *c);   /* pool_4_reset */
void f_088229CC(Cpu *c);   /* pool_7_reset */
void f_08822AA8(Cpu *c);   /* fighters_reset_steps */
void f_08823304(Cpu *c);   /* game_objects_3_clear */
void f_08822444(Cpu *c);   /* pool_5_reset */
void f_08822B60(Cpu *c);   /* ticker_clear */
void f_0885F050(Cpu *c);   /* fighter_routine_772 */
void f_088B4FD4(Cpu *c);   /* wrap_108_156 */
void f_08879C8C(Cpu *c);   /* fighter_script_leave_block */
void f_0887A10C(Cpu *c);   /* fighter_script_read */
void f_0887BBC8(Cpu *c);   /* fighter_free_to_act */
void f_088215C4(Cpu *c);   /* object_draw */
void f_088BD1D4(Cpu *c);   /* make_23_pair_then_drop */
void f_088BD268(Cpu *c);   /* drop_to_routine_y */
void f_0880AA74(Cpu *c);   /* game_start_fade_out */
void f_089DEBE8(Cpu *c);   /* backdrop_three_texts */
void f_0886B468(Cpu *c);   /* battle_136_or_140 */
void f_0886DC3C(Cpu *c);   /* fighter_clear_644 */
void f_0887B504(Cpu *c);   /* fighter_run_own_routine */
void f_0887B528(Cpu *c);   /* fighter_cpu_state_from_word */
void f_0887B9EC(Cpu *c);   /* fighter_can_throw */
void f_0887BA84(Cpu *c);   /* fighter_in_plain_step */
void f_0887BAD8(Cpu *c);   /* fighter_asks_of_opponent */
void f_089E3364(Cpu *c);   /* value_by_low_4 */
void f_088774A8(Cpu *c);   /* fighter_script_step */
void f_0887A400(Cpu *c);   /* step_0887A400 */
void f_0887B418(Cpu *c);   /* fighter_script_start_510 */
void f_088B4514(Cpu *c);   /* trail_record_point */
void f_088ABD4C(Cpu *c);   /* move_and_count_kind */
void f_0882196C(Cpu *c);   /* object_set_animation */
void f_088B5334(Cpu *c);   /* particles_start */
void f_088B54FC(Cpu *c);   /* clear_5c_7f */

/* ---------------------------------------------------------------- objects */

/* (object): bounces (+88), then its next frame. */
SFA_FUNC(089CFFC4, bounce_then_next_frame) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_089CEFC8(c);
    c->r[4] = o;
    f_08821AA8(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +73 counts down; at 0 the particle starts (by +78). */
SFA_FUNC(088B4C40, count_73_then_particle) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t n = (uint32_t)(S8(o + 73) - 1);
    *B(o + 73) = (uint8_t)n;
    c->r[6] = n;
    c->r[7] = n << 24;
    c->r[5] = (uint32_t)(int32_t)(int8_t)n;
    if (c->r[5] == 0) f_088B4C74(c);
    c->r[29] += 0x20;
}

/* (object): +9 on, +13 = 0x2A, +14 = 4, +15 = 0, then the next entry of 0x08B02810. */
SFA_FUNC(088AAD0C, entry_2810_restart) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    *B(o + 9) = (uint8_t)(*B(o + 9) + 1);
    *B(o + 13) = 0x2A;
    *B(o + 14) = 4;
    *B(o + 15) = 0;
    c->r[5] = 4; c->r[6] = 0x2A;
    f_088AADD8(c);
    c->r[29] += 0x20;
}

/* (object, leader): x moves by its speed (+96), y and facing taken from the leader; then drawn until
   +16 changes. */
SFA_FUNC(088ADEF0, follow_y_and_draw) {
    gaddr o = ARG(0), p = ARG(1);
    c->r[29] -= 0x20;
    uint32_t speed = *W(o + 96);
    *H(o + 18) = (uint16_t)((uint32_t)(int32_t)S16(o + 18) + speed);
    uint32_t y = (uint32_t)(int32_t)S16(p + 22);
    *H(o + 22) = (uint16_t)y;
    uint32_t facing = *B(p + 11);
    *B(o + 11) = (uint8_t)facing;
    c->r[5] = facing; c->r[6] = y; c->r[7] = speed;
    f_088AE240(c);
    c->r[29] += 0x20;
}

/* (object): its owner's (+88) +536 counts down, at 0 clearing the owner's +228; the object is freed. */
SFA_FUNC(0892EEE8, owner_count_then_free) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    gaddr p = *W(o + 88);
    uint32_t n = (uint32_t)*B(p + 536) - 1;
    *B(p + 536) = (uint8_t)n;
    n &= 0xFF;
    if (n == 0) *W(p + 228) = 0;
    c->r[5] = p; c->r[6] = n;
    f_088230F4(c);
    c->r[29] += 0x20;
}

/* (fighter, record): the record's +0 counted, +2 / +4 / +6 set to 1 and +3 / +5 / +7 cleared, then the
   three presses (a2 as the caller left it). */
SFA_FUNC(08869A88, press_three_reset) {
    gaddr r = ARG(1);
    c->r[29] -= 0x20;
    uint32_t n = (uint32_t)*B(r) + 1;
    *B(r) = (uint8_t)n;
    *B(r + 2) = 1; *B(r + 3) = 0;
    *B(r + 4) = 1; *B(r + 5) = 0;
    *B(r + 6) = 1; *B(r + 7) = 0;
    c->r[7] = n; c->r[8] = 1;
    f_08869AC8(c);
    c->r[29] += 0x20;
}

/* (a0, x, y): the description at 0x08B026A4 (+0 = 0, +1 = 1, +2 = 0xF, +4 x, +6 y) made into a0. */
SFA_FUNC(0887E420, make_from_26a4) {
    gaddr d = 0x08B026A4u;
    c->r[29] -= 0x20;
    *B(d) = 0;
    *B(d + 1) = 1;
    *H(d + 4) = (uint16_t)ARG(1);
    *B(d + 2) = 0xF;
    *H(d + 6) = (uint16_t)ARG(2);
    c->r[5] = d; c->r[7] = 0xF; c->r[8] = d; c->r[9] = 1;
    f_0887E7AC(c);
    c->r[29] += 0x20;
}

/* (): the fighters' steps, the pools and the game's objects reset. */
SFA_FUNC(08821F58, fight_reset_short) {
    c->r[29] -= 0x10;
    f_08822AA8(c);
    f_08822808(c);
    f_08822934(c);
    f_088229CC(c);
    f_08823304(c);
    f_08822144(c);
    c->r[29] += 0x10;
}

/* (): as 0x08821F58 for a new fight: also fighters 3 and 4 off (+8), pool 5 and the ticker, the twenty
   bytes at 0x095177C8 cleared and carried into the seven records after them, and +0 of the twelve
   0x18-byte records at 0x09517868 cleared. */
SFA_FUNC(08822040, fight_reset) {
    c->r[29] -= 0x10;
    f_08822AA8(c);
    *B(P3 + 8) = 0;
    c->r[4] = P4;
    *B(P4 + 8) = 0;
    f_08822808(c);
    f_08822934(c);
    f_088229CC(c);
    f_08822444(c);
    f_08822B60(c);
    gaddr first = 0x095177C8u, recs = 0x09517868u, end = recs + 0x120;
    for (uint32_t i = 0; i < 0x14; i++) *B(first + i) = 0;
    uint32_t w[5] = { 0 };
    for (uint32_t k = 0; k < 7; k++) {
        gaddr from = first + k * 0x14, to = from + 0x14;
        for (int i = 0; i < 5; i++) w[i] = *W(from + 4 * i);
        for (int i = 0; i < 5; i++) *W(to + 4 * i) = w[i];
    }
    gaddr r = recs;
    for (; r < end; r += 0x18) *B(r) = 0;
    c->r[4] = end; c->r[5] = r; c->r[6] = 0;
    c->r[7] = first + 0x14 * 8; c->r[8] = 8; c->r[9] = 0;
    c->r[10] = w[4]; c->r[11] = w[2];
    f_08822144(c);
    c->r[29] += 0x10;
}

/* (object): moved by its speeds (+96 / +98, 8.8 into +16 / +108), then wrapped. */
SFA_FUNC(088B4F2C, move_then_wrap) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    uint32_t dx = (uint32_t)(int32_t)S16(o + 96) << 8;
    uint32_t x = *W(o + 16) + dx;
    uint32_t y0 = *W(o + 108);
    *W(o + 16) = x;
    uint32_t y = y0 + ((uint32_t)(int32_t)S16(o + 98) << 8);
    *W(o + 108) = y;
    c->r[5] = y; c->r[6] = x; c->r[7] = dx; c->r[8] = y0;
    f_088B4FD4(c);
    c->r[29] += 0x20;
}

/* (object): +5 = 0 makes the 0x23 pair, 1 drops it to its routine's y; then drawn. */
SFA_FUNC(088BD180, pair_23_step_then_draw) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    uint32_t step = *B(o + 5);
    c->r[5] = 1;
    if (step == 0) { c->r[4] = o; f_088BD1D4(c); }
    else if (step == 1) { c->r[4] = o; f_088BD268(c); }
    else c->r[4] = step;
    c->r[4] = o;
    f_088215C4(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): unless +93, +18 counts down and past 0 starts the fade out, +4 on; then the backdrop's
   three texts (a0 as the fade left it). */
SFA_FUNC(089DEC88, count_then_fade_out) {
    gaddr o = ARG(0);
    c->r[29] -= 0x10;
    c->r[5] = (uint32_t)(int32_t)S8(o + 93);
    if (c->r[5] == 0) {
        uint32_t n = (uint32_t)(int32_t)S16(o + 18) - 1;
        c->r[6] = n << 16;
        *H(o + 18) = (uint16_t)n;
        c->r[5] = (uint32_t)(int32_t)(int16_t)n;
        if ((int32_t)c->r[5] < 0) {
            *H(o + 18) = 0;
            c->r[5] = (uint32_t)*H(o + 4) + 1;
            *H(o + 4) = (uint16_t)c->r[5];
            f_0880AA74(c);
        }
    }
    f_089DEBE8(c);
    c->r[29] += 0x10;
}

/* (object, value): while +2's value (a1 + 2) is under +119 the object animates and is drawn;
   otherwise +4 = 2. */
SFA_FUNC(089CF6A8, animate_while_under_119) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    c->r[6] = *B(o + 119);
    c->r[5] = (int32_t)S16(ARG(1) + 2) < (int32_t)c->r[6];
    if (c->r[5]) {
        c->r[4] = o; f_08821AA8(c);
        c->r[4] = o; f_088215C4(c);
    } else {
        *B(o + 4) = 2;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (): camera line 0 / 1 / 2 started for the stage (BATTLE + 0) unless its table's flag is set; lines 0
   and 1 then put their layer (+10 / +18 x, +14 / +22 y) at the table's place. */
static void camera_line(Cpu *c, uint32_t line) {
    gaddr t = CAMERA_TABLES + line * 152, s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = t;
    if (*B(t + 2) == 0) {
        c->r[5] = (uint32_t)(int32_t)S16(BATTLE);
        c->r[4] = line;
        f_089E0178(c);
        if (line < 2) {
            gaddr l = line ? LAYER_1 : LAYER_0;
            uint32_t x = (uint32_t)(int32_t)S16(t + 10), y = (uint32_t)(int32_t)S16(t + 14);
            *H(l + 10) = (uint16_t)x; *H(l + 18) = (uint16_t)x;
            *H(l + 14) = (uint16_t)y; *H(l + 22) = (uint16_t)y;
            c->r[4] = x; c->r[5] = l; c->r[6] = y;
        }
    } else {
        c->r[4] = 0x092E0000u;
    }
    c->r[29] += 0x20;
    c->r[16] = s16;
}
SFA_FUNC(089E0054, camera_line_0) { camera_line(c, 0); }
SFA_FUNC(089E00CC, camera_line_1) { camera_line(c, 1); }

/* (): line 2 (no layer to place); s0 is not used. */
SFA_FUNC(089E0144, camera_line_2) {
    c->r[29] -= 0x20;
    gaddr t = CAMERA_TABLES + 2 * 152;
    c->r[4] = *B(t + 2);
    if (c->r[4] == 0) {
        c->r[5] = (uint32_t)(int32_t)S16(BATTLE);
        c->r[4] = 2;
        f_089E0178(c);
    } else {
        c->r[4] = 0x092E0000u;
    }
    c->r[29] += 0x20;
}

/* (object): the value for +28 (0 none; 2: the one at +1484; otherwise +28 itself) by its low four bits. */
SFA_FUNC(089E3400, value_of_28) {
    gaddr o = ARG(0);
    c->r[29] -= 0x20;
    int32_t v = (int32_t)*W(o + 28);
    c->r[5] = (uint32_t)v;
    c->r[6] = 2;
    if (v == 0) { RETURN(0); c->r[29] += 0x20; return; }
    c->r[4] = v == 2 ? *W(o + 1484) : (uint32_t)v;
    f_089E3364(c);
    c->r[29] += 0x20;
}

/* (object): its step by +5 (0x08A37398), the trail's point, and drawn while +5 is under 5. */
SFA_FUNC(088B40E0, step_trail_draw) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    gaddr fn = *W(0x08A37398u + (uint32_t)*B(o + 5) * 4);
    c->r[5] = fn;
    c->r[4] = o;
    rt_call_indirect(c, fn);
    c->r[4] = o;
    f_088B4514(c);
    c->r[4] = *B(o + 5);
    if ((int32_t)c->r[4] < 5) { c->r[4] = o; f_088215C4(c); }
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): +80 / +78 on; each of +21's three bits not yet set is set once its kind's objects have
   moved (0x08B027C8 / F8 / 0x08B02828, 0x140 / 0x1A0 / 0x280); with all three, +17 on. */
SFA_FUNC(088ABC3C, three_kinds_moved) {
    gaddr o = ARG(0), s16 = c->r[16];
    static const struct { gaddr list; uint32_t n; } K[3] = {
        { 0x08B027C8u, 0x140 }, { 0x08B027F8u, 0x1A0 }, { 0x08B02828u, 0x280 } };
    c->r[29] -= 0x20;
    c->r[16] = o;
    *B(o + 80) = 1;
    *B(o + 78) = 1;
    uint32_t v = *B(o + 21);
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t bit = 1u << i;
        c->r[5] = v & bit;
        if (v & bit) continue;
        c->r[4] = K[i].list; c->r[5] = K[i].n; c->r[6] = i;
        f_088ABD4C(c);
        int moved = (int32_t)c->r[2] > 0;
        v = *B(o + 21);
        if (moved && !(v & bit)) {
            *B(o + 21) = (uint8_t)(v | bit);
            v = *B(o + 21);
        }
    }
    c->r[4] = v;
    c->r[5] = 7;
    if (v == 7) *B(o + 17) = (uint8_t)(*B(o + 17) + 1);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* (object): the 66 particles at 0x08B0285C cleared and given their places (0x08B02C7C, from the
   table 0x08A3709E, kept at 0x092E3E98), the object set up (kind 0x7D's place, animation 0x08A37310
   + 8) and the particles started. */
SFA_FUNC(088B5074, particles_setup) {
    gaddr o = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = o;
    f_088B54FC(c);
    gaddr part = 0x08B0285Cu, place = 0x08B02C7Cu, table = 0x08A3709Eu, from = table;
    uint32_t last = 0;
    for (uint32_t k = 0; k < 66; k++, part += 16, place += 6, from += 6) {
        for (uint32_t i = 0; i < 16; i++) *B(part + i) = 0;
        *H(place) = *H(from);
        *H(place + 2) = *H(from + 2);
        last = *H(from + 4);
        *H(place + 4) = (uint16_t)last;
    }
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    *W(o + 148) = 1;
    *H(o + 86) = 10;
    *W(0x092E3E98u) = table;
    *B(o + 9) = 3;
    *H(o + 38) = 0;
    *B(o + 15) = 0; *B(o + 14) = 0; *B(o + 11) = 0;
    *B(o + 12) = 0xFF;
    *B(o + 13) = 1;
    *H(o + 24) = 7;
    *H(o + 26) = 3;
    *H(o + 82) = 0x10;
    *H(o + 18) = 0xB8;
    *H(o + 22) = 0x7D;
    c->r[4] = o; c->r[5] = 0x7D; c->r[6] = 7; c->r[7] = part;
    c->r[8] = 66; c->r[9] = 0; c->r[10] = part; c->r[11] = last;
    f_088B5334(c);
    c->r[4] = o; c->r[5] = *W(0x08A37310u + 8);
    f_0882196C(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}

/* ---------------------------------------------------------------- fighters */

/* (fighter): +772 set runs its routine; otherwise its character's routine +72 (a1 the routine). */
SFA_FUNC(0885EE74, fighter_routine_or_72) {
    gaddr f = ARG(0);
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)(int32_t)S8(f + 772);
    if (c->r[5] != 0) {
        f_0885F050(c);
    } else {
        gaddr fn = *W(*W(f + 416) + 72);
        c->r[5] = fn;
        rt_call_indirect(c, fn);
    }
    c->r[29] += 0x20;
}

/* (fighter, script): free to act, the script is read on; otherwise its block is left. */
SFA_FUNC(088790C4, fighter_script_read_if_free) {
    gaddr f = ARG(0), p = ARG(1), s16 = c->r[16], s17 = c->r[17];
    c->r[29] -= 0x20;
    c->r[17] = f; c->r[16] = p;
    f_0887BBC8(c);
    int free = c->r[2] != 0;
    c->r[4] = f; c->r[5] = p;
    if (free) f_0887A10C(c);
    else f_08879C8C(c);
    c->r[29] += 0x20;
    c->r[16] = s16; c->r[17] = s17;
}

/* (fighter, script): the script's word (+490) sets the CPU's state; a fighter running its own routine
   reads the script on. Otherwise its routine goes back to 1 / 0 / 7 / 0 with +482 = 4 (from 1 / 1 / 2
   / 0 its pair record's +356 = 2 first), facing its opponent's side, +644 cleared; returns the script
   after the word. */
SFA_FUNC(088788D4, fighter_script_cpu_word) {
    gaddr f = ARG(0), p = ARG(1);
    uint32_t s[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    c->r[29] -= 0x30;
    c->r[6] = (uint32_t)(int32_t)S16(p);
    c->r[17] = f;
    *H(f + 490) = (uint16_t)c->r[6];
    c->r[16] = p + 2;
    c->r[5] = (uint32_t)(int32_t)S16(f + 490) & 0xFFFF;
    f_0887B528(c);
    c->r[4] = f;
    f_0887B504(c);
    if (c->r[2] == 0) {
        c->r[4] = f; c->r[5] = c->r[16];
        f_0887A10C(c);
    } else {
        c->r[19] = 1;
        if (*B(f + 4) == 1 && *B(f + 5) == 1) {
            c->r[18] = 2;
            if (*B(f + 6) == 2 && *B(f + 7) == 0) {
                c->r[4] = f;
                f_0886B468(c);
                *B(c->r[2] + 356) = 2;
            }
        }
        *B(f + 482) = 4;
        *B(f + 483) = 0;
        *B(f + 4) = 1;
        *B(f + 6) = 7;
        *B(f + 7) = 0;
        *B(f + 5) = 0;
        uint8_t side = *B(f + 697);
        *B(f + 262) = 1;
        *B(f + 235) = 1;
        *B(f + 11) = side;
        *B(f + 696) = 0;
        *B(f + 123) = 0;
        c->r[4] = f; c->r[5] = 7;
        f_0886DC3C(c);
        RETURN(c->r[16]);
    }
    c->r[29] += 0x30;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2]; c->r[19] = s[3];
}

/* (fighter): the CPU's script bits (+544) asking for an attack, unless at the deepest nesting (+484 =
   3): bit 0 a throw it can make (+510 = 0); bit 1 from routine 1 / 1 / 2 / 0 with +508 under 64 and
   the opponent's +296 record set (+510 = 1: its speeds, +244 / +778 and +125 / +126 cleared, +543 = 5,
   facing its opponent's side); bit 2 with +508 under 96 what it asks of its opponent (+510 = 2); bit 3
   from a plain step against an opponent in 1 / 1 with +116 = 6 (+510 = 3). Returns whether one was. */
SFA_FUNC(0887B7FC, fighter_cpu_attack_asked) {
    gaddr f = ARG(0);
    uint32_t s[3] = { c->r[16], c->r[17], c->r[18] };
    c->r[29] -= 0x20;
    uint32_t bits = (uint32_t)(int32_t)S16(f + 544);
    c->r[18] = bits;
    c->r[16] = f;
    uint32_t r = 0;
    if (bits == 0) goto out;
    c->r[17] = 3;
    if (*B(f + 484) == 3) goto out;
    if (bits & 1) {
        c->r[4] = f;
        f_0887B9EC(c);
        if (c->r[2] != 0) { *B(f + 510) = 0; r = 1; goto out; }
    }
    if (bits & 2) {
        if (*B(f + 4) == 1 && *B(f + 5) == 1) {
            c->r[5] = 2;
            if (*B(f + 6) == 2 && *B(f + 7) == 0 && S16(f + 508) < 64 &&
                *B(*W(*W(f + 92) + 296)) != 0) {
                *B(f + 270) = 0;
                *W(f + 244) = 0;
                *H(f + 778) = 0;
                *B(f + 126) = 0;
                *B(f + 125) = 0;
                *B(f + 543) = 5;
                *B(f + 11) = (uint8_t)S8(f + 697);
                *B(f + 510) = 1;
                r = 1; goto out;
            }
        }
    }
    if ((bits & 4) && S16(f + 508) < 96) {
        c->r[4] = f;
        f_0887BAD8(c);
        if (c->r[2] != 0) { *B(f + 510) = 2; r = 1; goto out; }
    }
    if (bits & 8) {
        c->r[4] = f;
        f_0887BA84(c);
        if (c->r[2] != 0) {
            gaddr opp = *W(f + 92);
            if (*B(opp + 4) == 1 && *B(opp + 5) == 1 && *B(opp + 116) == 6) {
                *B(f + 510) = 3; r = 1; goto out;
            }
        }
    }
out:
    RETURN(r);
    c->r[29] += 0x20;
    c->r[16] = s[0]; c->r[17] = s[1]; c->r[18] = s[2];
}

/* (fighter): the CPU's first script picked (+480) and its first command run (+481, from +524 into
   +528) once each; then its step by +482. */
SFA_FUNC(08877448, fighter_script_first_then_step) {
    gaddr f = ARG(0), s16 = c->r[16];
    c->r[29] -= 0x20;
    c->r[16] = f;
    if (*B(f + 480) == 0) {
        *B(f + 480) = 1;
        c->r[4] = f;
        f_0887B418(c);
    }
    if (*B(f + 481) == 0) {
        c->r[5] = *W(f + 524);
        *B(f + 481) = 1;
        *W(f + 528) = c->r[5];
        c->r[4] = f;
        f_088774A8(c);
    }
    c->r[4] = f;
    f_0887A400(c);
    c->r[29] += 0x20;
    c->r[16] = s16;
}
