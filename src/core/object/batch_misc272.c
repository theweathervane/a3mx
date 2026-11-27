/* Small dispatchers and the leaves under them: a step picked by a count, a button reader by table, and a
   crouching attack start. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0887F2C4(Cpu *c); void f_0887DF88(Cpu *c); void f_0895C18C(Cpu *c); void f_08863114(Cpu *c);
void f_0886CA2C(Cpu *c); void f_08861674(Cpu *c); void f_0886D678(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }
static void call2(Cpu *c, gaddr o, uint32_t n, void (*fn)(Cpu *), uint32_t back) {
    ARG(0) = o; ARG(1) = n; c->r[31] = back; fn(c);
}
static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }

/* (object): +4 on, +9 cleared and 0x0887DF88 (+78). */
SFA_FUNC(0887F348, next_state_78) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *B(o + 4) = (uint8_t)(*B(o + 4) + 1);
    c->r[6] = (uint32_t)S8(o + 78);
    *B(o + 9) = 0;
    *W(c->r[29] + 16) = ra;
    ARG(1) = c->r[6] & 0xFFFF; c->r[31] = 0x0887F374u; f_0887DF88(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (object): 0x0887F2C4 while +3 is under 3, else the next state (0x0887F348). */
SFA_FUNC(0887F284, count_or_next) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    if (*B(o + 3) < 3) { c->r[5] = 1; c->r[31] = 0x0887F2A4u; f_0887F2C4(c); }
    else { c->r[5] = 0; c->r[31] = 0x0887F2B8u; f_0887F348(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the cpu's buttons by +290's low four bits, through the handlers at 0x08A70A00. */
SFA_FUNC(08939A98, cpu_buttons_tab) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = 0x08A70A00u;
    uint32_t fn = *W(0x08A70A00u + ((*H(o + 290) & 0xF) << 2));
    c->r[5] = fn;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x08939AC4u; rt_call_indirect(c, fn);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): the buttons +760 bits 4..6 for a player, else the cpu's (0x08939A98) as a halfword. */
SFA_FUNC(08938718, buttons_456) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(o + 373);
    *W(c->r[29] + 16) = ra;
    if (c->r[5] == 0) c->r[2] = *H(o + 760) & 0x70;
    else { c->r[31] = 0x08938748u; f_08939A98(c); c->r[2] &= 0xFFFF; }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 0: step 1, +235 = 1, facing the opponent, 0x08863114, the meter by strength and move
   strength + 0xC (0xF with +193); later the attack step. */
SFA_FUNC(0895C0F4, attack_crouch_c) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    uint32_t st = *B(o + 7);
    if (st != 0) call1(c, o, f_0886D678, 0x0895C17Cu);
    else {
        *B(o + 7) = 1; *B(o + 235) = 1; *B(o + 11) = (uint8_t)S8(o + 697);
        call1(c, o, f_08863114, 0x0895C12Cu);
        call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x0895C138u);
        int k = S8(o + 193) != 0;
        uint32_t n = (uint32_t)(S8(o + 194) + (k ? 0xF : 0xC)) & 0xFFFF;
        call2(c, o, n, f_08861674, k ? 0x0895C154u : 0x0895C16Cu);
    }
    leave(c, sv);
}

/* (fighter): standing: +696 cleared and 0x0895C18C; crouching: +696 = 1 and 0x0895C0F4. */
SFA_FUNC(0895C0B8, attack_by_stance_c) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[5] = 1;
    if (S8(o + 192) == 0) { *B(o + 696) = 0; c->r[31] = 0x0895C0D4u; f_0895C18C(c); }
    else { *B(o + 696) = 1; c->r[31] = 0x0895C0E8u; f_0895C0F4(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_089093FC(Cpu *c); void f_08864740(Cpu *c); void f_0890CC04(Cpu *c); void f_08861848(Cpu *c);

/* (fighter): by strength: light / medium step 0 faces the opponent (step 1, +235 = 1, 0x08863114), the
   meter by strength and move strength + 0xF, step 1 the attack step; heavy step 0 the same with a lunge
   (x 10.0, brake -0.75), the meter by 2 and move 0x11, step 1 hands over to 0x08864740 when its animation
   ends, else 0x0890CC04 on +77 while moving forward, and ticked. */
SFA_FUNC(0890947C, attack_193_lunge) {
    gaddr o = ARG(0); uint32_t sv[2]; enter(c, o, sv);
    int32_t s = S8(o + 194);
    uint32_t st = *B(o + 7);
    if (s == 0 || s == 1) {
        if (st == 0) {
            *B(o + 7) = 1; *B(o + 11) = (uint8_t)S8(o + 697); *B(o + 235) = 1;
            call1(c, o, f_08863114, 0x089094C8u);
            call2(c, o, (uint32_t)S8(o + 194), f_0886CA2C, 0x089094D4u);
            call2(c, o, (uint32_t)(S8(o + 194) + 0xF) & 0xFFFF, f_08861674, 0x089094E8u);
        } else if (st == 1) call1(c, o, f_0886D678, 0x08909500u);
    } else if (s == 2) {
        if (st == 0) {
            *B(o + 7) = 1; *B(o + 11) = (uint8_t)S8(o + 697); *B(o + 235) = 1;
            *W(o + 96) = 0xA0000; *W(o + 104) = 0xFFFF4000u;
            call1(c, o, f_08863114, 0x0890954Cu);
            call2(c, o, 2, f_0886CA2C, 0x08909558u);
            call2(c, o, 0x11, f_08861674, 0x08909564u);
        } else if (st == 1) {
            if (S8(o + 75) < 0) call1(c, o, f_08864740, 0x08909588u);
            else {
                if (S8(o + 77) != 0 && (int32_t)*W(o + 96) >= 0) call1(c, o, f_0890CC04, 0x089095ACu);
                call1(c, o, f_08861848, 0x089095B4u);
            }
        }
    }
    leave(c, sv);
}

/* (fighter): +696 = 1, then 0x089093FC, or 0x0890947C with +193. */
SFA_FUNC(089093BC, attack_by_193) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = (uint32_t)S8(o + 193); c->r[5] = 1;
    *W(c->r[29] + 16) = ra;
    *B(o + 696) = 1;
    if (c->r[6] == 0) { c->r[31] = 0x089093DCu; f_089093FC(c); }
    else { c->r[31] = 0x089093F0u; f_0890947C(c); }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_0882B894(Cpu *c); void f_08831FF4(Cpu *c); void f_088D5E44(Cpu *c); void f_08822F8C(Cpu *c);

/* (owner, script): a list-driven spawn: while 0x0882B894 holds, 0x08831FF4 (owner, script). Else the
   owner's +10 on, +22 = 0xE0; the entry +65 of list +66 (from the side's fighter +416, or 0x09517E24 in
   the alternate mode, by 0x088D5E44) goes to the script's +18. A zero entry ends it (0x08831FF4); else
   a new object (0x08822F8C 1, 0) gets kind 0xD, its frame (0x08A05838 x 0x80, or the alternate table),
   +32 = 1, +30 = 0x98 / 0x18 by the entry's bit 15 and +26 = 0x60 unless that bit and the fighter's
   +337 differ; the script's +65 on. A 0x30 frame keeping s0..s3. */
SFA_FUNC(08831CC0, list_spawn) {
    gaddr x = ARG(0), q = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], s3 = c->r[19], ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = q;
    *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = s3; *W(sp + 32) = ra;
    c->r[19] = x;
    ARG(0) = x; c->r[31] = 0x08831CE4u; f_0882B894(c);
    if (c->r[2] != 0) { ARG(0) = x; ARG(1) = q; c->r[31] = 0x08831CF4u; f_08831FF4(c); }
    else {
        *H(x + 10) = (uint16_t)(*H(x + 10) + 1);
        ARG(0) = (uint32_t)S8(x + 57) & 1; ARG(1) = 0xE0;
        *H(x + 22) = 0xE0;
        gaddr f = ARG(0) ? 0x092E3EA0u : 0x092E4338u; c->r[17] = f;
        c->r[31] = 0x08831D38u; f_088D5E44(c);
        uint32_t k = (uint32_t)S8(q + 66) << 2;
        gaddr base = c->r[2] == 0 ? *W(*W(*W(f + 416) + 152)) : *W(*W(0x09517A78u + 940));
        gaddr lst = *W(base + k);
        int32_t n = S16(lst + (uint32_t)(S8(q + 65) * 2));
        c->r[18] = (uint32_t)n;
        *H(q + 18) = (uint16_t)n;
        if (n == 0) {
            ARG(0) = x; ARG(1) = q; c->r[31] = 0x08831DB8u; f_08831FF4(c);
        } else {
            ARG(0) = 1; ARG(1) = 0; c->r[31] = 0x08831D98u; f_08822F8C(c);
            gaddr e = c->r[2]; c->r[19] = e;
            if (e != 0) {
                *B(e + 42) = 0xD; *B(e) = 0xD;
                ARG(0) = 0xD; c->r[31] = 0x08831DE0u; f_088D5E44(c);
                uint32_t idx = (uint32_t)n & 0x3FF;
                if (c->r[2] == 0) *W(e + 44) = *W(0x08A05834u + 4) + (idx << 7) - 0x80;
                else *W(e + 44) = *W(*W(*W(0x09517A78u + 940) + 4) + (idx << 2) - 4);
                *H(e + 32) = 1;
                *H(e + 30) = (n & 0x8000) ? 0x98 : 0x18;
                int32_t s = S8(f + 337); c->r[17] = (uint32_t)s;
                uint32_t v = (uint32_t)(int16_t)((((uint32_t)n >> 15) & 1) + (uint32_t)s) & 1;
                *H(e + 26) = v == 0 ? 0x60 : 0;
            }
            *B(q + 65) = (uint8_t)(S8(q + 65) + 1);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[19] = s3; c->r[31] = ra; c->r[29] += 0x30;
}


void f_08821F58(Cpu *c); void f_08863A9C(Cpu *c); void f_088616EC(Cpu *c);

/* (fighter): the side's bit (+337) set in 0x08E75997. */
SFA_FUNC(08828C04, mark_side_135) {
    gaddr o = ARG(0);
    c->r[4] = 1u << ((uint32_t)S8(o + 337) & 31);
    c->r[5] = 0x08E75910u;
    c->r[6] = (uint32_t)S8(0x08E75910u + 135);
    c->r[4] |= c->r[6];
    *B(0x08E75910u + 135) = (uint8_t)c->r[4];
}

/* (a, b): a's +38 / +72 and b's +14 set to -1, b's +15 cleared, then 0x08821F58 (a, b). */
SFA_FUNC(088249A0, link_reset) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x10;
    c->r[6] = 0xFFFFFFFFu;
    *B(a + 38) = 0xFF; *B(b + 14) = 0xFF; *B(a + 72) = 0xFF;
    *W(c->r[29]) = ra;
    *B(b + 15) = 0;
    c->r[31] = 0x088249C0u; f_08821F58(c);
    c->r[31] = ra; c->r[29] += 0x10;
}

/* (fighter): the part at +0x3A0 gets +32 = -1; a player's stick from 0x08863A9C, the cpu's from the
   part's +30 / +31, as a word. */
SFA_FUNC(08939A44, part_stick) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    c->r[6] = 0xFFFFFFFFu;
    gaddr q = o + 0x3A0; c->r[5] = q;
    *B(q + 32) = 0xFF;
    c->r[6] = (uint32_t)S8(o + 373);
    *W(sp + 20) = ra;
    if (c->r[6] == 0) {
        ARG(1) = sp + 16; c->r[31] = 0x08939A6Cu; f_08863A9C(c);
    } else {
        *B(sp + 16) = (uint8_t)S8(q + 30);
        c->r[4] = (uint32_t)S8(q + 31);
        *B(sp + 17) = (uint8_t)c->r[4];
    }
    c->r[2] = *W(sp + 16);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): step 4 / 7 / 8 by +236 (2 / 0xC / else), a lift (x -0.75, y 10.0, brake -0.5) and move
   strength + 9. */
SFA_FUNC(0895D3E0, lift_by_236) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t k = S8(o + 236);
    *W(c->r[29] + 16) = ra;
    *B(o + 7) = k == 2 ? 4 : (k == 0xC ? 7 : 8);
    *W(o + 96) = 0xFFFF4000u; *W(o + 104) = 0; *W(o + 100) = 0xA0000;
    c->r[6] = (uint32_t)S8(o + 194); c->r[7] = 0xFFFF8000u;
    *W(o + 108) = 0xFFFF8000u;
    ARG(1) = (c->r[6] + 9) & 0xFFFF; c->r[31] = 0x0895D44Cu; f_088616EC(c);
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_088249A0(Cpu *c); void f_08939A44(Cpu *c); void f_08939A98(Cpu *c);

/* (a, b): a link step: with a's +38 = 1 the link reset (0x088249A0), else unless +39 the handler for +8
   from the table (a, b). */
static void link_step(Cpu *c, gaddr tab, uint32_t back_reset, uint32_t back_call) {
    gaddr a = ARG(0), b = ARG(1);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[6] = b; c->r[7] = (uint32_t)S8(a + 38); c->r[8] = 1;
    *W(c->r[29] + 16) = ra;
    if (c->r[7] == 1) { c->r[31] = back_reset; f_088249A0(c); }
    else {
        c->r[6] = (uint32_t)S8(a + 39);
        if (c->r[6] == 0) {
            uint32_t fn = *W(tab + ((uint32_t)*H(a + 8) << 2));
            c->r[7] = fn; c->r[6] = b;
            c->r[31] = back_call; rt_call_indirect(c, fn);
        } else c->r[6] = b;
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (a, b): a link step through the handlers at 0x08A0E218. */
SFA_FUNC(0882538C, link_step_e218) { link_step(c, 0x08A0E218u, 0x088253BCu, 0x088253F8u); }

/* (a, b): a link step through the handlers at 0x08A0E284. */
SFA_FUNC(08826ADC, link_step_e284) { link_step(c, 0x08A0E284u, 0x08826B0Cu, 0x08826B48u); }

/* The stick word from the part (0x08939A44) when the cpu's buttons (0x08939A98) or a player's +760 bits
   are set, kept at slot (cpu 20, player 24) and its low half copied to 16; 1 when read. */
static int read_part_stick(Cpu *c, gaddr o, uint32_t mask, uint32_t b98, uint32_t b44c, uint32_t b44p) {
    gaddr sp = c->r[29];
    uint32_t slot;
    if (S8(o + 373) != 0) {
        ARG(0) = o; c->r[31] = b98; f_08939A98(c);
        if (c->r[2] == 0) return 0;
        ARG(0) = o; c->r[31] = b44c; f_08939A44(c);
        slot = 20;
    } else {
        if ((*H(o + 760) & mask) == 0) return 0;
        ARG(0) = o; c->r[31] = b44p; f_08939A44(c);
        slot = 24;
    }
    *W(sp + slot) = c->r[2];
    c->r[4] = (uint32_t)S16(sp + slot);
    *H(sp + 16) = (uint16_t)c->r[4];
    return 1;
}

/* (fighter): the part's stick (buttons bits 4..9) into +78. A 0x30 frame keeping s0. */
SFA_FUNC(08935F04, part_stick_78) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    c->r[5] = (uint32_t)S8(o + 373);
    *W(c->r[29] + 28) = s0; *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    if (read_part_stick(c, o, 0x3F0, 0x08935F24u, 0x08935F34u, 0x08935F64u)) {
        c->r[4] = *B(c->r[29] + 16);
        *B(o + 78) = (uint8_t)c->r[4];
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): the part's stick (buttons bits 4..9) into the strength +194 and +193. A 0x30 frame keeping
   s0. */
SFA_FUNC(08936470, part_stick_strength) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    c->r[5] = (uint32_t)S8(o + 373);
    *W(c->r[29] + 28) = s0; *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    if (read_part_stick(c, o, 0x3F0, 0x08936490u, 0x089364A0u, 0x089364D8u)) {
        c->r[4] = *B(c->r[29] + 16); c->r[5] = *B(c->r[29] + 17);
        *B(o + 194) = (uint8_t)c->r[4]; *B(o + 193) = (uint8_t)c->r[5];
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}

/* (fighter): the part's stick (buttons bits 4..6) into +78 and the part's +32 = 1. A 0x30 frame keeping
   s0. */
SFA_FUNC(08938D40, part_stick_78p) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x30;
    c->r[5] = (uint32_t)S8(o + 373);
    *W(c->r[29] + 28) = s0; *W(c->r[29] + 32) = ra;
    c->r[16] = o;
    if (read_part_stick(c, o, 0x70, 0x08938D60u, 0x08938D70u, 0x08938DACu)) {
        c->r[5] = 1;
        c->r[4] = *B(c->r[29] + 16);
        c->r[6] = o + 0x3A0;
        *B(o + 78) = (uint8_t)c->r[4]; *B(o + 0x3A0 + 32) = 1;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x30;
}


void f_0886BF84(Cpu *c); void f_08828C04(Cpu *c); void f_088D30D8(Cpu *c); void f_088D3488(Cpu *c);

/* (fighter): a cancel into a special: with +251 or +254 and 0x0886BF84, state 1 / 0 / 7 / 0, +236 = 6,
   facing the opponent and the side marked (0x08828C04): 1, else 0. */
SFA_FUNC(088EED14, cancel_special) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    c->r[5] = (uint32_t)S8(o + 251);
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    uint32_t r = 0;
    if (c->r[5] != 0 || S8(o + 254) != 0) {
        ARG(0) = o; c->r[31] = 0x088EED40u; f_0886BF84(c);
        if (c->r[2] != 0) {
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
            c->r[5] = (uint32_t)S8(o + 697);
            *B(o + 236) = 6; *B(o + 11) = (uint8_t)c->r[5];
            ARG(0) = o; c->r[31] = 0x088EED74u; f_08828C04(c);
            r = 1;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, kind, return addresses): a cancel into a super: when 0x088D30D8 says 1, +251 or +254 is set
   and 0x0886BF84 says 1, 0x088D3488, +236 = kind, +235 cleared, facing the opponent and the side marked
   (0x08828C04): 1, else 0. A frame keeping s0, s1. */
static void cancel_super(Cpu *c, uint32_t kind, int r5_697, const uint32_t back[4]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    ARG(0) = o; c->r[31] = back[0]; f_088D30D8(c);
    c->r[17] = 1;
    uint32_t r = 0;
    if (c->r[2] == 1 && (S8(o + 251) != 0 || S8(o + 254) != 0)) {
        ARG(0) = o; c->r[31] = back[1]; f_0886BF84(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = back[2]; f_088D3488(c);
            if (r5_697) { c->r[4] = kind; c->r[5] = (uint32_t)S8(o + 697); }
            else c->r[4] = (uint32_t)S8(o + 697);
            *B(o + 236) = (uint8_t)kind; *B(o + 235) = 0;
            *B(o + 11) = (uint8_t)S8(o + 697);
            ARG(0) = o; c->r[31] = back[3]; f_08828C04(c);
            r = 1;
        }
    }
    c->r[2] = r;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): a cancel into super kind 1. */
SFA_FUNC(089C6ED8, cancel_super_1) {
    static const uint32_t b[4] = { 0x089C6EF0u, 0x089C6F1Cu, 0x089C6F2Cu, 0x089C6F44u };
    cancel_super(c, 1, 0, b);
}

/* (fighter): a cancel into super kind 7. */
SFA_FUNC(089B56DC, cancel_super_7) {
    static const uint32_t b[4] = { 0x089B56F4u, 0x089B5720u, 0x089B5730u, 0x089B574Cu };
    cancel_super(c, 7, 1, b);
}

/* (fighter): a cancel into super kind 8. */
SFA_FUNC(089B9B50, cancel_super_8) {
    static const uint32_t b[4] = { 0x089B9B68u, 0x089B9B94u, 0x089B9BA4u, 0x089B9BC0u };
    cancel_super(c, 8, 1, b);
}

/* (fighter): a cancel into super kind 7 (a second copy). */
SFA_FUNC(089C1064, cancel_super_7b) {
    static const uint32_t b[4] = { 0x089C107Cu, 0x089C10A8u, 0x089C10B8u, 0x089C10D4u };
    cancel_super(c, 7, 1, b);
}


/* (fighter): the cpu's special start: state 1 / 0 / 7 / 0, strength 2, +560..+562 cleared, +236 = 6 and
   +235 = 1 (cleared for +338 = 0xF). */
SFA_FUNC(0885EA78, cpu_special_start) {
    gaddr o = ARG(0);
    *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0; *B(o + 194) = 2;
    *B(o + 560) = 0; *B(o + 561) = 0; *B(o + 562) = 0; *B(o + 236) = 6;
    c->r[6] = 1;
    c->r[5] = (uint32_t)S8(o + 338);
    *B(o + 235) = 1;
    c->r[6] = 0xF;
    if (c->r[5] == 0xF) *B(o + 235) = 0;
}

/* (?, attack, fighter): on the cpu side for attack kinds from 6 up, the hit slots +112 / +114 / +420 from
   0x08A0E138 by the round timer at 0x095179E8 (capped at 50, plus its last digit) and 0x08A0E137. */
SFA_FUNC(08823ED4, cpu_hit_slot) {
    gaddr x = ARG(1), o = ARG(2);
    c->r[4] = (uint32_t)S8(o + 373);
    if (c->r[4] == 0) return;
    int32_t k = S8(x + 79) - 6;
    c->r[4] = 0x09510000u;
    if (k < 0) return;
    int32_t n = S16(0x095179E0u + 8);
    c->r[5] = (uint32_t)n;
    int32_t cl = n < 51 ? n : 0x32;
    c->r[7] = 0xA;
    DIV(c, (uint32_t)n, 0xA);
    int32_t off = S8(0x08A0E134u + 3);
    c->r[5] = (uint32_t)off;
    c->r[7] = 0x08A0E138u;
    c->r[8] = c->hi;
    uint32_t v = *B(0x08A0E138u + (uint32_t)(c->hi + cl + off));
    c->r[4] = v;
    *H(o + 114) = (uint16_t)v; *H(o + 112) = (uint16_t)v; *H(o + 420) = (uint16_t)v;
}
