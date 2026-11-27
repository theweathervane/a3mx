/* A fighter's command input check (0x08867708, called about 1.5 million times a run): the button layout
   for the player or the computer (0x08867538), new presses from the L2 / R2 hook 0x08867794 merged into
   +760 (0x08867C30), else the command's own checker by its kind. Offsets stay numbers until something
   shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_088D7C1C(Cpu *c); void f_08867794(Cpu *c); void f_08867A60(Cpu *c);
void f_088674D4(Cpu *c); void f_08867538(Cpu *c); void f_08867BB0(Cpu *c); void f_08867C30(Cpu *c);

/* the button layout of a fighter's player slot (+775) in the 20-byte table at 0x08A058A0 */
static int32_t slot_layout(gaddr o) { return S8(0x08A058A0u + (uint32_t)(S8(o + 775) * 20) + 17); }

/* (fighter): the layout a computer fighter uses: its slot's, unless its bit (+337) in 0x08E75910 +53 is
   clear and 0x0950FF64 is set, when 0x09525D99. */
SFA_FUNC(088674D4, cpu_layout) {
    gaddr o = ARG(0);
    int32_t id = S8(o + 337);
    if ((S8(0x08E75910u + 53) & (1 << (id & 31))) == 0 && *B(0x0950FF60u + 4) != 0)
        c->r[2] = (uint32_t)S8(0x09520000u + 23961);
    else c->r[2] = (uint32_t)slot_layout(o);
}

/* (fighter, n): entry n of the layout's command table: 0x08A1BC1C with a layout, 0x08A1B8C8 without. The
   layout is the player's (0x09523438, 208 bytes a slot) when 0x088D7C1C says so, the computer's in mode 2,
   else the slot's. */
SFA_FUNC(08867538, command_table_entry) {
    gaddr o = ARG(0); uint32_t n = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = n; c->r[17] = o;
    c->r[31] = 0x08867554u; f_088D7C1C(c);
    uint32_t idx = n << 2;
    c->r[16] = idx;
    int32_t lay;
    if (c->r[2] != 0) {
        int32_t i = S8(o + 775);
        lay = S8(0x09523438u + (uint32_t)(i * 208) + 17);
    } else if (S8(0x08E75910u + 114) == 2) {
        ARG(0) = o; c->r[31] = 0x088675A8u; f_088674D4(c);
        lay = (int32_t)c->r[2];
    } else lay = slot_layout(o);
    c->r[17] = (uint32_t)lay;
    c->r[2] = *W((lay != 0 ? 0x08A1BC1Cu : 0x08A1B8C8u) + idx);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, a1): the hook's new presses (0x08867794) passed to 0x08867A60. */
SFA_FUNC(08867BB0, new_presses) {
    gaddr o = ARG(0); uint32_t a1 = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = a1; c->r[17] = o;
    c->r[31] = 0x08867BCCu; f_08867794(c);
    ARG(0) = o; ARG(1) = a1; ARG(2) = c->r[2]; c->r[31] = 0x08867BDCu; f_08867A60(c);
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, a1): 1 with the new presses merged into +760, 0 without any. */
SFA_FUNC(08867C30, merge_presses) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08867C44u; f_08867BB0(c);
    uint32_t v = c->r[2];
    if (v != 0) { *H(o + 760) = (uint16_t)(*H(o + 760) | v); c->r[2] = 1; }
    else c->r[2] = 0;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, command, n): the table entry for n, then 1 when new presses came in; otherwise the command's
   checker from 0x08A1BD24 by its kind (command +0) with the entry. A 0x30 frame keeping s0..s3. */
SFA_FUNC(08867708, command_check) {
    gaddr o = ARG(0), cmd = ARG(1); uint32_t n = ARG(2);
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 28) = sv[3];
    c->r[16] = cmd; c->r[19] = n; c->r[17] = o;
    *W(sp + 24) = sv[2]; *W(sp + 32) = ra;
    ARG(0) = o; ARG(1) = n; c->r[31] = 0x08867734u; f_08867538(c);
    uint32_t entry = c->r[2];
    c->r[18] = entry;
    ARG(0) = o; ARG(1) = n; c->r[31] = 0x08867744u; f_08867C30(c);
    if (c->r[2] != 0) c->r[2] = 1;
    else {
        uint32_t fn = *W(0x08A1BD24u + ((uint32_t)*B(cmd) << 2));
        ARG(0) = o; ARG(1) = cmd; ARG(2) = entry; ARG(3) = fn;
        c->r[31] = 0x08867778u; rt_call_indirect(c, fn);
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra; c->r[29] += 0x30;
}


/* (fighter, a, b): 1 when the two button sets match; for b 8 (the layout's alternate) when the layout
   in use (the player's, the computer's in mode 2, else the slot's second byte) is set and they share a
   bit; else 0. */
SFA_FUNC(0886892C, buttons_match) {
    gaddr o = ARG(0);
    uint32_t a = ARG(1) & 0xFFFF, b = ARG(2) & 0xFFFF;
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[18] = a; c->r[17] = b; c->r[16] = o;
    uint32_t ret;
    if (a == b) ret = 1;
    else if (b != 8) ret = 0;
    else {
        c->r[31] = 0x08868964u; f_088D7C1C(c);
        int32_t lay;
        if (c->r[2] != 0) lay = S8(0x09523438u + (uint32_t)(S8(o + 775) * 208) + 17);
        else if (S8(0x08E75910u + 114) == 2) { ARG(0) = o; c->r[31] = 0x088689ECu; f_088674D4(c); lay = (int32_t)c->r[2]; }
        else lay = S8(0x08A058A0u + (uint32_t)(S8(o + 775) * 20) + 16);
        c->r[16] = (uint32_t)lay;
        ret = lay != 0 && (a & b) != 0;
    }
    c->r[2] = ret;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08821368(Cpu *c); void f_08868208(Cpu *c); void f_08867608(Cpu *c); void f_0886766C(Cpu *c);

/* (fighter): the computer's alternate setting: its slot's, unless its bit is clear and 0x0950FF64 is
   set, when 0x09525D98. */
SFA_FUNC(08867608, cpu_alt_setting) {
    gaddr o = ARG(0);
    int32_t id = S8(o + 337);
    if ((S8(0x08E75910u + 53) & (1 << (id & 31))) == 0 && *B(0x0950FF60u + 4) != 0)
        c->r[2] = (uint32_t)S8(0x09520000u + 23960);
    else c->r[2] = (uint32_t)S8(0x08A058A0u + (uint32_t)(S8(o + 775) * 20) + 16);
}

/* (fighter): the alternate setting in use: the player's (0x09523438, 208 bytes a slot) when 0x088D7C1C
   says so, the computer's in mode 2, else the slot's. */
SFA_FUNC(0886766C, alt_setting) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    c->r[31] = 0x08867680u; f_088D7C1C(c);
    if (c->r[2] != 0) c->r[2] = (uint32_t)S8(0x09523438u + (uint32_t)(S8(o + 775) * 208) + 17);
    else {
        if (S8(0x08E75910u + 114) == 2) { ARG(0) = o; c->r[31] = 0x088676D0u; f_08867608(c); c->r[16] = c->r[2]; }
        else c->r[16] = (uint32_t)S8(0x08A058A0u + (uint32_t)(S8(o + 775) * 20) + 16);
        c->r[2] = c->r[16];
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, input timer): +1 on, +4 = a random wait (0x08A1B5F0 by choice + 1 and 5 random bits) plus
   the setting's (0x08A1B670), then 0x08868208. */
SFA_FUNC(0886806C, input_wait) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = t;
    *B(t + 1) = (uint8_t)(*B(t + 1) + 1);
    *W(sp + 20) = s1; c->r[17] = o;
    *W(sp + 24) = s2; *W(sp + 28) = ra;
    ARG(0) = o; c->r[31] = 0x0886809Cu; f_0886766C(c);
    uint32_t set = c->r[2];
    c->r[18] = set;
    c->r[31] = 0x088680A4u; f_08821368(c);
    uint32_t idx = (uint32_t)((S8(o + 386) + 1) << 5) + (c->r[2] & 0x1F);
    uint32_t v = *B(0x08A1B5F0u + idx) + *B(0x08A1B670u + set);
    *B(t + 4) = (uint8_t)v;
    ARG(0) = t; ARG(1) = v; c->r[31] = 0x088680E8u; f_08868208(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter, input timer): +4 = a random wait (0x08A1B5F0 by choice + 1 and 5 random bits) plus the
   setting's (0x08A1B670), then 0x08868208. */
SFA_FUNC(08868100, input_wait_b) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 20) = s1; *W(sp + 24) = s2; *W(sp + 28) = ra;
    c->r[17] = t;
    c->r[31] = 0x08868120u; f_0886766C(c);
    uint32_t set = c->r[2];
    c->r[18] = set;
    c->r[31] = 0x08868128u; f_08821368(c);
    uint32_t idx = (uint32_t)((S8(o + 386) + 1) << 5) + (c->r[2] & 0x1F);
    uint32_t v = *B(0x08A1B5F0u + idx) + *B(0x08A1B670u + set);
    *B(t + 4) = (uint8_t)v;
    ARG(0) = t; ARG(1) = v; c->r[31] = 0x0886816Cu; f_08868208(c);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_088681F8(Cpu *c); void f_08868878(Cpu *c); void f_08868100(Cpu *c);

/* (fighter, input timer, step): the computer's direction step: while the held direction (+764) is the
   step's (its low 4 bits) the timer's count goes on and a wait is drawn (with step bit 10 a second count
   and 0x08868878), else 0x088681F8 clears the timer. */
SFA_FUNC(088686CC, cpu_dir_step) {
    gaddr o = ARG(0), t = ARG(1), st = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2;
    *B(t + 1) = 0;
    c->r[18] = o;
    uint32_t v = *H(st);
    *W(sp + 20) = s1; *W(sp + 28) = ra; *W(sp + 16) = s0;
    c->r[17] = t; c->r[16] = st;
    if ((v & 0xF) != (*H(o + 764) & 0xF)) { ARG(0) = t; c->r[31] = 0x08868760u; f_088681F8(c); }
    else {
        *B(t) = (uint8_t)(*B(t) + 1);
        if ((v & 0x400) == 0) { ARG(0) = o; ARG(1) = t; c->r[31] = 0x08868784u; f_0886806C(c); }
        else {
            *B(t) = (uint8_t)(*B(t) + 1);
            ARG(0) = o; ARG(1) = t; c->r[31] = 0x08868730u; f_0886806C(c);
            ARG(0) = o; ARG(1) = t; ARG(2) = st; c->r[31] = 0x08868740u; f_08868878(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, input timer, step): the fighter's held direction (+362: 8, 2, 4 or 1) counted into the timer
   (+1 its index, +2 the step's low byte) with a wait drawn; no direction or another clears the timer. */
SFA_FUNC(08868AC0, cpu_dir_count) {
    gaddr o = ARG(0), t = ARG(1), st = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t d = *H(o + 362) & 0xF;
    int32_t idx = d == 8 ? 0 : d == 2 ? 1 : d == 4 ? 2 : d == 1 ? 3 : -1;
    if (idx < 0) {
        ARG(0) = t; c->r[31] = d == 0 ? 0x08868B28u : 0x08868B84u; f_088681F8(c);
    } else {
        *B(t) = (uint8_t)(*B(t) + 1);
        *B(t + 1) = (uint8_t)idx;
        *B(t + 2) = (uint8_t)*H(st);
        ARG(0) = o; ARG(1) = t; c->r[31] = 0x08868B14u; f_08868100(c);
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter, input timer, steps): the timer's wait (+4) counts down and clears it at 0; else the step's
   entry (+1 into `steps`): none draws a new wait; a direction other than the fighter's (+752 with +7, else
   +764) is 0x08868208; the same takes another count with bit 10 and draws a new wait. */
SFA_FUNC(08867DD0, cpu_step_wait) {
    gaddr o = ARG(0), t = ARG(1), steps = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t n = (uint32_t)(*B(t + 4) - 1) & 0xFF;
    *B(t + 4) = (uint8_t)n;
    ARG(0) = t; ARG(1) = o;
    if (n == 0) { c->r[31] = 0x08867E24u; f_088681F8(c); }
    else {
        uint32_t e = *H(steps + (uint32_t)*B(t + 1) * 2);
        uint32_t held = (*B(t + 7) != 0 ? *H(o + 752) : *H(o + 764)) & 0xF;
        if (e == 0) { ARG(0) = o; ARG(1) = t; c->r[31] = 0x08867E4Cu; f_0886806C(c); }
        else if (held != (e & 0xF)) { ARG(2) = e & 0x400; c->r[31] = 0x08867E78u; f_08868208(c); }
        else {
            if (e & 0x400) *B(t) = (uint8_t)(*B(t) + 1);
            ARG(0) = o; ARG(1) = t; c->r[31] = 0x08867E9Cu; f_0886806C(c);
        }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, input timer, steps): as 0x08867DD0 with the step's low 10 bits against the held direction
   (+764). */
SFA_FUNC(0886879C, cpu_step_wait_b) {
    gaddr o = ARG(0), t = ARG(1), steps = ARG(2);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    uint32_t n = (uint32_t)(*B(t + 4) - 1) & 0xFF;
    *B(t + 4) = (uint8_t)n;
    ARG(0) = t; ARG(1) = o;
    if (n == 0) { c->r[31] = 0x08868808u; f_088681F8(c); }
    else {
        uint32_t e = *H(steps + (uint32_t)*B(t + 1) * 2);
        uint32_t want = *H(o + 764) & 0xF;
        if (e == 0) { ARG(0) = o; ARG(1) = t; c->r[31] = 0x088687F4u; f_0886806C(c); }
        else if ((e & 0x3FF) != want) { ARG(2) = e & 0x400; c->r[31] = 0x08868854u; f_08868208(c); }
        else if (e & 0x400) {
            *B(t) = (uint8_t)(*B(t) + 1);
            ARG(0) = o; ARG(1) = t; c->r[31] = 0x08868840u; f_0886806C(c);
        } else { ARG(0) = o; ARG(1) = t; c->r[31] = 0x0886886Cu; f_0886806C(c); }
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_08867EA8(Cpu *c);

/* (fighter, input timer, step): as 0x088686CC, with the direction from +752 instead when the timer asks
   for it (+7) and the fighter is in a special state (1 / 0 / 7 or 8), and 0x08867EA8 for the second count. */
SFA_FUNC(08867CBC, cpu_dir_step_b) {
    gaddr o = ARG(0), t = ARG(1), st = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 20) = s1; c->r[17] = t; c->r[16] = st;
    *B(t + 1) = 0;
    *W(sp + 24) = s2; c->r[18] = o;
    uint32_t v = *H(st);
    *W(sp + 28) = ra; *W(sp + 16) = s0;
    uint32_t dir = *H(o + 764);
    if (*B(t + 7) != 0 && *B(o + 4) == 1 && *B(o + 5) == 0 && (*B(o + 6) == 7 || *B(o + 6) == 8)) dir = *H(o + 752);
    if ((dir & 0xF) != (v & 0xF)) { ARG(0) = t; c->r[31] = 0x08867D94u; f_088681F8(c); }
    else {
        *B(t) = (uint8_t)(*B(t) + 1);
        if ((v & 0x400) == 0) { ARG(0) = o; ARG(1) = t; c->r[31] = 0x08867DB8u; f_0886806C(c); }
        else {
            *B(t) = (uint8_t)(*B(t) + 1);
            ARG(0) = o; ARG(1) = t; c->r[31] = 0x08867D64u; f_0886806C(c);
            ARG(0) = o; ARG(1) = t; ARG(2) = st; c->r[31] = 0x08867D74u; f_08867EA8(c);
        }
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_0886892C(Cpu *c);

/* (fighter, input timer): a rotation: the wait (+4) counts down and clears the timer at 0; with a direction
   held (+362) that matches the next (or the previous) of the four in 0x08A1B674 from +1 the timer turns
   that way (+3 = 1 / -1, +1 moved, +0 on, +2 down) and draws a wait; anything else is 0x08868208. A 0x30
   frame keeping s0..s3. */
SFA_FUNC(08868B90, cpu_rotation) {
    gaddr o = ARG(0), t = ARG(1);
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    uint32_t n = (uint32_t)(*B(t + 4) - 1) & 0xFF;
    *W(sp + 20) = sv[1]; c->r[17] = o;
    *B(t + 4) = (uint8_t)n;
    *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra; *W(sp + 16) = sv[0];
    c->r[16] = t;
    if (n == 0) { ARG(0) = t; c->r[31] = 0x08868CA8u; f_088681F8(c); goto out; }
    {
        uint32_t d = *H(o + 362) & 0xF;
        c->r[19] = d;
        if (d == 0) { ARG(0) = t; c->r[31] = 0x08868C84u; f_08868208(c); goto out; }
        uint32_t i = *B(t + 1);
        uint32_t next = *H(0x08A1B674u + (((i + 1) & 3) << 1));
        uint32_t prev = *H(0x08A1B674u + (((i - 1) & 3) << 1));
        c->r[18] = prev;
        ARG(0) = o; ARG(1) = d; ARG(2) = next; c->r[31] = 0x08868C1Cu; f_0886892C(c);
        int32_t way;
        if (c->r[2] != 0) way = 1;
        else {
            ARG(0) = o; ARG(1) = d; ARG(2) = prev; c->r[31] = 0x08868CD0u; f_0886892C(c);
            if (c->r[2] == 0) { ARG(0) = t; c->r[31] = 0x08868D0Cu; f_08868208(c); goto out; }
            way = -1;
        }
        *B(t + 3) = (uint8_t)way;
        *B(t + 1) = (uint8_t)((i + (uint32_t)way) & 3);
        *B(t) = (uint8_t)(*B(t) + 1);
        *B(t + 2) = (uint8_t)(*B(t + 2) - 1);
        ARG(0) = o; ARG(1) = t; c->r[31] = 0x08868C60u; f_08868100(c);
    }
out:
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra; c->r[29] += 0x30;
}
