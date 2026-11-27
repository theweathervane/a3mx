/* The command checks by number (0x088D2F18 ...: the command input check with n fixed), and the command
   table's second checker set (0x088694E0). Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867708(Cpu *c); void f_08867538(Cpu *c);

/* (fighter, command): the command input check for entry n */
static void check_n(Cpu *c, uint32_t n, uint32_t back) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(2) = n; c->r[31] = back; f_08867708(c);
    c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(088D2F18, command_5) { check_n(c, 0x5, 0x088D2F28u); }
SFA_FUNC(088D2F34, command_6) { check_n(c, 0x6, 0x088D2F44u); }
SFA_FUNC(088D2F50, command_17) { check_n(c, 0x17, 0x088D2F60u); }
SFA_FUNC(088D2F6C, command_12) { check_n(c, 0x12, 0x088D2F7Cu); }
SFA_FUNC(088D2F88, command_18) { check_n(c, 0x18, 0x088D2F98u); }
SFA_FUNC(088D2FA4, command_1) { check_n(c, 0x1, 0x088D2FB4u); }
SFA_FUNC(088D2FC0, command_11) { check_n(c, 0x11, 0x088D2FD0u); }
SFA_FUNC(088D2FDC, command_4) { check_n(c, 0x4, 0x088D2FECu); }
SFA_FUNC(088D2FF8, command_3) { check_n(c, 0x3, 0x088D3008u); }
SFA_FUNC(088D3014, command_2) { check_n(c, 0x2, 0x088D3024u); }
SFA_FUNC(088D3030, command_1a) { check_n(c, 0x1A, 0x088D3040u); }
SFA_FUNC(088D304C, command_1b) { check_n(c, 0x1B, 0x088D305Cu); }
SFA_FUNC(088D3068, command_14) { check_n(c, 0x14, 0x088D3078u); }

/* (fighter, command, n): the table entry for n and the command's checker from 0x08A1BD74 by its kind,
   with the entry and n. */
SFA_FUNC(088694E0, command_check_b) {
    gaddr o = ARG(0), cmd = ARG(1); uint32_t n = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2;
    c->r[17] = cmd; c->r[16] = n; c->r[18] = o;
    *W(sp + 28) = ra;
    ARG(0) = o; ARG(1) = n; c->r[31] = 0x08869508u; f_08867538(c);
    uint32_t fn = *W(0x08A1BD74u + ((uint32_t)*B(cmd) << 2));
    ARG(0) = o; ARG(1) = cmd; ARG(2) = c->r[2]; ARG(3) = n; ARG(4) = fn;
    c->r[31] = 0x08869534u; rt_call_indirect(c, fn);
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

void f_08867C30(Cpu *c);

/* (fighter, command, n): the command input check with the checkers from `tab`: the table entry for n,
   1 when new presses came in, else the checker for the command's kind. Its return addresses sit at the
   same offsets from `base` in each copy. A 0x30 frame keeping s0..s3. */
static void command_check_tab(Cpu *c, gaddr tab, uint32_t base) {
    gaddr o = ARG(0), cmd = ARG(1); uint32_t n = ARG(2);
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; *W(sp + 20) = sv[1]; *W(sp + 28) = sv[3];
    c->r[16] = cmd; c->r[19] = n; c->r[17] = o;
    *W(sp + 24) = sv[2]; *W(sp + 32) = ra;
    ARG(0) = o; ARG(1) = n; c->r[31] = base + 0x2C; f_08867538(c);
    uint32_t entry = c->r[2];
    c->r[18] = entry;
    ARG(0) = o; ARG(1) = n; c->r[31] = base + 0x3C; f_08867C30(c);
    if (c->r[2] != 0) c->r[2] = 1;
    else {
        uint32_t fn = *W(tab + ((uint32_t)*B(cmd) << 2));
        ARG(0) = o; ARG(1) = cmd; ARG(2) = entry; ARG(3) = fn;
        c->r[31] = base + 0x70; rt_call_indirect(c, fn);
    }
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra; c->r[29] += 0x30;
}
SFA_FUNC(08868220, command_check_bd30) { command_check_tab(c, 0x08A1BD30u, 0x08868220u); }
SFA_FUNC(08868488, command_check_bd38) { command_check_tab(c, 0x08A1BD38u, 0x08868488u); }
SFA_FUNC(08868640, command_check_bd40) { command_check_tab(c, 0x08A1BD40u, 0x08868640u); }
SFA_FUNC(08868A34, command_check_bd4c) { command_check_tab(c, 0x08A1BD4Cu, 0x08868A34u); }
SFA_FUNC(08869008, command_check_bd60) { command_check_tab(c, 0x08A1BD60u, 0x08869008u); }
SFA_FUNC(088699FC, command_check_bd8c) { command_check_tab(c, 0x08A1BD8Cu, 0x088699FCu); }

void f_0886A0A4(Cpu *c); void f_0886A0F4(Cpu *c); void f_0886C508(Cpu *c); void f_0886DB94(Cpu *c);

/* (fighter): 1 with new presses for entry `n` or when `chk` says so, else whether all of `mask` was held
   in +760 before. */
static void held_all(Cpu *c, uint32_t n, void (*chk)(Cpu *), uint32_t mask, const uint32_t b[2]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    uint32_t h = *H(o + 760);
    c->r[17] = h;
    *W(c->r[29] + 24) = ra;
    ARG(1) = n; c->r[31] = b[0]; f_08867C30(c);
    if (c->r[2] != 0) c->r[2] = 1;
    else {
        ARG(0) = o; c->r[31] = b[1]; chk(c);
        c->r[2] = c->r[2] != 0 ? 1 : (h & mask) == mask;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(08869D9C, held_all_punches) {
    static const uint32_t b[2] = { 0x08869DBCu, 0x08869DCCu }; held_all(c, 0x81, f_0886A0A4, 0x70, b);
}
SFA_FUNC(08869EB0, held_all_kicks) {
    static const uint32_t b[2] = { 0x08869ED0u, 0x08869EE0u }; held_all(c, 0x82, f_0886A0F4, 0x380, b);
}

/* (fighter): characters 6 and 0x14 on command 9 of their list at +1128 with 0x0886C508's leave enter
   state 1 / 8 / 0 / 0 with +563 = 1 and 0x0886DB94. */
SFA_FUNC(0885E16C, command9_state) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    int32_t ch = S8(o + 338);
    if (ch == 6 || ch == 0x14) {
        ARG(0) = o; ARG(1) = o + 0x468; ARG(2) = 9; c->r[31] = 0x0885E1A8u; f_08867708(c);
        if (c->r[2] != 0) {
            ARG(0) = o; c->r[31] = 0x0885E1B8u; f_0886C508(c);
            if (c->r[2] != 0) {
                *B(o + 4) = 1; *B(o + 5) = 8; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 563) = 1;
                ARG(0) = o; c->r[31] = 0x0885E1E4u; f_0886DB94(c);
            }
        }
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08869FEC(Cpu *c); void f_0886A048(Cpu *c);

/* as held_all, with the held buttons of both pads (+760 | +762) */
static void held_all2(Cpu *c, uint32_t n, void (*chk)(Cpu *), uint32_t mask, const uint32_t b[2]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1;
    uint32_t h = (*H(o + 760) | *H(o + 762)) & 0xFFFF;
    c->r[17] = h;
    *W(c->r[29] + 24) = ra;
    ARG(1) = n; c->r[31] = b[0]; f_08867C30(c);
    if (c->r[2] != 0) c->r[2] = 1;
    else {
        ARG(0) = o; c->r[31] = b[1]; chk(c);
        c->r[2] = c->r[2] != 0 ? 1 : (h & mask) == mask;
    }
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(08869C88, held_all_punches2) {
    static const uint32_t b[2] = { 0x08869CB4u, 0x08869CC4u }; held_all2(c, 0x81, f_08869FEC, 0x70, b);
}
SFA_FUNC(08869E20, held_all_kicks2) {
    static const uint32_t b[2] = { 0x08869E4Cu, 0x08869E5Cu }; held_all2(c, 0x82, f_0886A048, 0x380, b);
}

/* (fighter, command, a2): 1 with new presses for entry 0x80, else the command's checker from 0x08A1BD80
   by its kind with 1 (2 without a2). */
SFA_FUNC(088697C0, command_check_80) {
    gaddr o = ARG(0), cmd = ARG(1); uint32_t a2 = ARG(2);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2;
    c->r[17] = cmd; c->r[16] = a2; c->r[18] = o;
    *W(sp + 28) = ra;
    ARG(1) = 0x80; c->r[31] = 0x088697E8u; f_08867C30(c);
    if (c->r[2] != 0) c->r[2] = 1;
    else {
        uint32_t fn = *W(0x08A1BD80u + ((uint32_t)*B(cmd) << 2));
        ARG(0) = o; ARG(1) = cmd; ARG(2) = a2 == 0 ? 2 : 1; ARG(3) = fn;
        c->r[31] = 0x08869848u; rt_call_indirect(c, fn);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_08821368(Cpu *c);

/* (fighter): the guard-cancel / throw-escape check: unless the fighter is off the floor by 0..79, has
   no choice made or +568 set, a held kick pair (+760 | +762 bits 7..9, two or three) with 0x0886C508's
   leave takes state 1 / 8 / 0 / 0 (+563 cleared). The computer in mode 2 with its bit clear and
   0x0886C508's leave does it at level 4, at level 5 one time in 4 (in a blockstun, +586 / +587) or 8, else
   it marks +568. Otherwise 0x0885E16C. */
SFA_FUNC(0885DFB0, escape_check) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    int32_t d = (int16_t)(S16(o + 22) - S16(o + 132));
    int stop = d < 0 || d >= 80 || S8(o + 568) != 0 || S8(o + 386) == -1;
    if (stop) { ARG(0) = o; c->r[31] = 0x0885E00Cu; f_0885E16C(c); goto out; }
    {
        uint32_t h = (*H(o + 760) | *H(o + 762)) & 0x380;
        if (h == 0x300) c->r[17] = 0x08E70000u;
        if (h == 0x380 || h == 0x180 || h == 0x280 || h == 0x300) {
            ARG(0) = o; c->r[31] = 0x0885E050u; f_0886C508(c);
            if (c->r[2] != 0) goto state;
            goto out;
        }
        c->r[17] = 0x08E70000u;
        const gaddr g = 0x08E75910u;
        c->r[17] = g;
        if (S8(g + 114) != 2) goto other;
        c->r[17] = 1;
        if (S8(g + 53) & (1 << (S8(o + 337) & 31))) goto other;
        ARG(0) = o; c->r[31] = 0x0885E0B0u; f_0886C508(c);
        if (c->r[2] == 0) goto other;
        uint32_t lvl = *B(0x0950FF68u);
        if (lvl == 4) goto state;
        if (lvl != 5) goto other;
        int busy = S8(o + 586) != 0 || S8(o + 587) != 0;
        c->r[31] = busy ? 0x0885E0F4u : 0x0885E128u; f_08821368(c);
        int32_t v = (int32_t)c->r[2], m = busy ? 3 : 7;
        int32_t r = v >= 0 ? (v & m) : -((-v) & m);
        if (r == 0) goto state;
        *B(o + 568) = 1;
        goto other;
    }
state:
    *B(o + 4) = 1; *B(o + 5) = 8; *B(o + 6) = 0; *B(o + 7) = 0; *B(o + 563) = 0;
    goto out;
other:
    ARG(0) = o; c->r[31] = 0x0885E158u; f_0885E16C(c);
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
