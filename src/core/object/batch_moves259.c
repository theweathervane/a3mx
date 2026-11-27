/* The buttons-to-special check (0x0886BB1C): when a fighter may act and has pressed a button, the
   special it asks for (strength +194 and kind +193 from 0x08863A9C) is checked against the character's
   allowed list and started; and the attack step 0x0886D678 that ends standing / crouched or starts it.
   Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08836E60(Cpu *c); void f_088632C4(Cpu *c); void f_0886BCB8(Cpu *c); void f_08863A9C(Cpu *c);
void f_0886DC3C(Cpu *c); void f_0886469C(Cpu *c); void f_08864740(Cpu *c); void f_08876A0C(Cpu *c);
void f_0886BE50(Cpu *c); void f_08861848(Cpu *c);
void f_088371F8(Cpu *c); void f_0886BD68(Cpu *c); void f_0886BE08(Cpu *c); void f_0886BB1C(Cpu *c);

static void call1(Cpu *c, gaddr o, void (*fn)(Cpu *), uint32_t back) { ARG(0) = o; c->r[31] = back; fn(c); }

/* (fighter): unless the game mode (0x08E75910 +57) is 3, a fighter with a choice made calls 0x08836E60 (1). */
SFA_FUNC(088371F8, special_cue_if_chosen) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    int32_t m = S8(0x08E75910u + 57);
    *W(c->r[29] + 16) = ra;
    if (m != 3 && S8(o + 373) != 0) {
        ARG(0) = 1; ARG(1) = 3; ARG(2) = (uint32_t)m; c->r[31] = 0x0883722Cu; f_08836E60(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, strength, held): 1 unless the fighter holds a direction (+752 bits 0 / 1) outside the
   crouch-step (+192 2) and the character's list (0x08A200F4 by +338; the crouching half with +752 bit 2)
   has the bit for this strength (+6 when held, +1 without bit 1). */
SFA_FUNC(0886BD68, special_not_listed) {
    gaddr o = ARG(0);
    uint32_t s = ARG(1) & 0xFF, held = ARG(2) & 0xFF;
    uint32_t h = *H(o + 752);
    c->r[2] = 1;
    if ((h & 3) == 0 || S8(o + 192) == 2) return;
    gaddr tbl = *W(0x08A200F4u + ((uint32_t)S8(o + 338) << 2));
    uint32_t v = *H(tbl + ((h & 4) ? 2u : 0u));
    if (held) s = (s + 6) & 0xFF;
    if ((h & 2) == 0) s = (s + 1) & 0xFF;
    c->r[2] = (v & (1u << (s & 31))) == 0;
}

/* (fighter, buttons, strength, held): the button bit for this strength: +6 crouching (+752 bit 2, not in
   the crouch-step), +3 held. */
SFA_FUNC(0886BE08, special_button_bit) {
    gaddr o = ARG(0);
    uint32_t btn = ARG(1), s = ARG(2) & 0xFF, held = ARG(3) & 0xFF;
    uint32_t n = 0;
    if (S8(o + 192) != 2 && (*H(o + 752) & 4)) n = 6;
    if (held) n += 3;
    c->r[2] = btn & (1u << ((n + s) & 31));
}

/* (fighter): 1 when a special starts. Not while paused (0x092E1228 +13 / 0x08E75910 +38) or when
   0x088632C4 says so; characters 0x11 / 0x13 with no choice made need +792 bits 0 / 20 / 21 (outside the
   crouch-step only without +327); it needs +123 and a button (+760 bits 4..9). The buttons come from
   0x0886BCB8 and the special (strength, kind) from 0x08863A9C; on the ground (+80 clear) it needs +638,
   the button's bit and the character's list. Then +194 / +193 set, +192 = 1 crouching (+752 bit 2) else
   0, 0x088371F8 and 0x0886DC3C. */
SFA_FUNC(0886BB1C, special_from_buttons) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s0; c->r[16] = o;
    *W(sp + 24) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    if ((S8(0x092E1228u + 13) | S8(0x08E75910u + 38)) != 0) goto out;
    call1(c, o, f_088632C4, 0x0886BB5Cu);
    if (c->r[2] != 0) goto out;
    if (S8(o + 386) == -1) {
        int32_t ch = S8(o + 338);
        if (ch == 0x11 || ch == 0x13) {
            if (!(S8(o + 192) == 2 && S8(o + 327) != 0)) {
                if ((*W(o + 792) & 0x300001u) == 0) goto out;
            }
        }
    }
    if (S8(o + 123) == 0 || (*H(o + 760) & 0x3F0) == 0) goto out;
    call1(c, o, f_0886BCB8, 0x0886BBF0u);
    c->r[17] = c->r[2];
    ARG(0) = o; ARG(1) = sp + 16; c->r[31] = 0x0886BC00u; f_08863A9C(c);
    if (S8(o + 80) == 0) {
        if (S8(o + 638) == 0) goto out;
        ARG(0) = o; ARG(1) = c->r[17]; ARG(2) = *B(sp + 16); ARG(3) = *B(sp + 17);
        c->r[31] = 0x0886BC2Cu; f_0886BE08(c);
        if (c->r[2] == 0) goto out;
        ARG(0) = o; ARG(1) = *B(sp + 16); ARG(2) = *B(sp + 17);
        c->r[31] = 0x0886BC44u; f_0886BD68(c);
        if (c->r[2] == 0) goto out;
    }
    *B(o + 194) = *B(sp + 16); *B(o + 193) = *B(sp + 17);
    c->r[17] = 1;
    *B(o + 192) = (*H(o + 752) & 4) ? 1 : 0;
    call1(c, o, f_088371F8, 0x0886BC80u);
    call1(c, o, f_0886DC3C, 0x0886BC88u);
    ok = c->r[17];
out:
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): once the move ends it stands or crouches (by +192; character 7 clears +820 first);
   until then a fighter with a choice made goes back to step 0 when 0x08876A0C says so, else a special
   from the buttons (after 0x0886BE50 when +77 is below zero) sends it to step 0 with +567 = -1; ticked. */
SFA_FUNC(0886D678, attack_step) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (S8(o + 75) < 0) {
        if (S8(o + 338) == 7) *B(o + 0x334) = 0;
        if (S8(o + 192) == 0) call1(c, o, f_0886469C, 0x0886D6B8u);
        else call1(c, o, f_08864740, 0x0886D6C8u);
        goto out;
    }
    if (S8(o + 373) != 0) {
        call1(c, o, f_08876A0C, 0x0886D6E4u);
        if (c->r[2] != 0) *B(o + 7) = 0;
    } else if (S8(o + 77) < 0) {
        call1(c, o, f_0886BE50, 0x0886D704u);
        if (c->r[2] != 0) { *B(o + 7) = 0; *B(o + 567) = 0xFF; }
        else {
            call1(c, o, f_0886BB1C, 0x0886D720u);
            if (c->r[2] != 0) { *B(o + 7) = 0; *B(o + 567) = 0xFF; }
        }
    } else {
        call1(c, o, f_0886BB1C, 0x0886D73Cu);
        if (c->r[2] != 0) { *B(o + 7) = 0; *B(o + 567) = 0xFF; }
    }
    call1(c, o, f_08861848, 0x0886D754u);
out:
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
