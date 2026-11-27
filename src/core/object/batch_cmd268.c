/* Special moves started from a command (each character's list): the command check for the special's
   command (at fighter + off), an optional choice / busy test, its condition (0x0886BF84 or the gauge
   0x0886C6EC), its preparation, the special's number (+236), facing the opponent, +235 = 1 and the start
   with the command. The result is whether it started. Offsets stay numbers until something shows what
   they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_0886BF84(Cpu *c); void f_0886C6EC(Cpu *c); void f_0886D2EC(Cpu *c); void f_0886D3E0(Cpu *c);
void f_0886DB94(Cpu *c); void f_0886DBE8(Cpu *c); void f_088D3278(Cpu *c); void f_088D3488(Cpu *c);
void f_088D2F18(Cpu *c); void f_088D2F34(Cpu *c); void f_088D2F50(Cpu *c); void f_088D2F6C(Cpu *c);
void f_088D2F88(Cpu *c); void f_088D2FA4(Cpu *c); void f_088D2FDC(Cpu *c); void f_088D2FF8(Cpu *c);
void f_088D3014(Cpu *c); void f_088D304C(Cpu *c); void f_088D3068(Cpu *c);

/* the extra test before the condition: what stops it */
enum { T_NONE, T_NEG, T_SET, T_POS, T_CLEAR };   /* below zero, set, above zero, clear */
typedef struct {
    uint16_t off; void (*cmd)(Cpu *); uint32_t b_cmd;
    uint16_t t_off; uint8_t t_kind;
    void (*chk)(Cpu *); uint32_t b_chk; void (*prep)(Cpu *); uint32_t b_prep;
    uint8_t k236; void (*start)(Cpu *); uint32_t b_start;
    uint8_t noface;            /* 1: the facing is left (an air special) */
} CmdSp;

static void cmd_special(Cpu *c, const CmdSp *k) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr cmd = o + k->off;
    c->r[18] = cmd;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = cmd; c->r[31] = k->b_cmd; k->cmd(c);
    c->r[17] = 1;
    if (c->r[2] != 1) goto out;
    if (k->t_kind != T_NONE) {
        int32_t v = k->t_off == 536 ? (int32_t)*B(o + k->t_off) : S8(o + k->t_off);
        if (k->t_kind == T_NEG && v < 0) goto out;
        if (k->t_kind == T_SET && v != 0) goto out;
        if (k->t_kind == T_POS && v > 0) goto out;
        if (k->t_kind == T_CLEAR && v == 0) goto out;
    }
    ARG(0) = o; c->r[31] = k->b_chk; k->chk(c);
    if (c->r[2] != 1) goto out;
    ARG(0) = o; c->r[31] = k->b_prep; k->prep(c);
    *B(o + 236) = k->k236;
    if (!k->noface) *B(o + 11) = *B(o + 697);
    *B(o + 235) = 1;
    ARG(0) = o; ARG(1) = cmd; c->r[31] = k->b_start; k->start(c);
    ok = 1;
out:
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(089B53A0, cmd_special_53a0) {
    static const CmdSp k = { 0x350, f_088D2FF8, 0x089B53C4u, 0, T_NONE, f_0886BF84, 0x089B53D8u, f_088D3488, 0x089B53E8u,
                             0, f_0886DB94, 0x089B5404u };
    cmd_special(c, &k);
}
SFA_FUNC(089C0E68, cmd_special_0e68) {
    static const CmdSp k = { 0x358, f_088D2FA4, 0x089C0E8Cu, 0, T_NONE, f_0886BF84, 0x089C0EA0u, f_088D3488, 0x089C0EB0u,
                             1, f_0886DB94, 0x089C0ECCu };
    cmd_special(c, &k);
}
SFA_FUNC(089B563C, cmd_special_563c) {
    static const CmdSp k = { 0x360, f_088D2F6C, 0x089B5660u, 0, T_NONE, f_0886BF84, 0x089B5674u, f_088D3488, 0x089B5684u,
                             2, f_0886DBE8, 0x089B56A4u };
    cmd_special(c, &k);
}
SFA_FUNC(089C0D20, cmd_special_0d20) {
    static const CmdSp k = { 0x370, f_088D3068, 0x089C0D44u, 0, T_NONE, f_0886BF84, 0x089C0D58u, f_088D3488, 0x089C0D68u,
                             4, f_0886DBE8, 0x089C0D88u };
    cmd_special(c, &k);
}
SFA_FUNC(089C7220, cmd_special_7220) {
    static const CmdSp k = { 0x370, f_088D3014, 0x089C7244u, 0, T_NONE, f_0886BF84, 0x089C7258u, f_088D3488, 0x089C7268u,
                             4, f_0886DBE8, 0x089C7288u };
    cmd_special(c, &k);
}
SFA_FUNC(089C72C0, cmd_special_72c0) {
    static const CmdSp k = { 0x3A0, f_088D2F34, 0x089C72E4u, 0, T_NONE, f_0886BF84, 0x089C72F8u, f_088D3488, 0x089C7308u,
                             10, f_0886DBE8, 0x089C7328u };
    cmd_special(c, &k);
}
SFA_FUNC(089B5594, cmd_special_5594) {
    static const CmdSp k = { 0x358, f_088D2FDC, 0x089B55B8u, 536, T_SET, f_0886BF84, 0x089B55D8u, f_088D3488, 0x089B55E8u,
                             1, f_0886DBE8, 0x089B5604u };
    cmd_special(c, &k);
}
SFA_FUNC(089B9734, cmd_special_9734) {
    static const CmdSp k = { 0x350, f_088D2FA4, 0x089B9758u, 536, T_SET, f_0886BF84, 0x089B9778u, f_088D3488, 0x089B9788u,
                             0, f_0886DB94, 0x089B97A4u };
    cmd_special(c, &k);
}
SFA_FUNC(089C0DC0, cmd_special_0dc0) {
    static const CmdSp k = { 0x350, f_088D2F18, 0x089C0DE4u, 386, T_NEG, f_0886BF84, 0x089C0E04u, f_088D3488, 0x089C0E14u,
                             0, f_0886DB94, 0x089C0E30u };
    cmd_special(c, &k);
}
SFA_FUNC(089B543C, cmd_special_543c) {
    static const CmdSp k = { 0x368, f_088D2F18, 0x089B5460u, 386, T_NEG, f_0886BF84, 0x089B5480u, f_088D3488, 0x089B5490u,
                             3, f_0886DB94, 0x089B54B0u };
    cmd_special(c, &k);
}
SFA_FUNC(089B54E8, cmd_special_54e8) {
    static const CmdSp k = { 0x370, f_088D2FA4, 0x089B550Cu, 386, T_NEG, f_0886BF84, 0x089B552Cu, f_088D3488, 0x089B553Cu,
                             4, f_0886DB94, 0x089B555Cu };
    cmd_special(c, &k);
}
SFA_FUNC(089B93A8, cmd_special_93a8) {
    static const CmdSp k = { 0x378, f_088D2F50, 0x089B93CCu, 386, T_POS, f_0886C6EC, 0x089B93ECu, f_088D3278, 0x089B93FCu,
                             5, f_0886D2EC, 0x089B941Cu };
    cmd_special(c, &k);
}
SFA_FUNC(089B9454, cmd_special_9454) {
    static const CmdSp k = { 0x388, f_088D2F88, 0x089B9478u, 386, T_SET, f_0886C6EC, 0x089B9498u, f_088D3278, 0x089B94A8u,
                             7, f_0886D3E0, 0x089B94C8u };
    cmd_special(c, &k);
}
SFA_FUNC(089C0AA8, cmd_special_0aa8) {
    static const CmdSp k = { 0x378, f_088D2F50, 0x089C0ACCu, 386, T_SET, f_0886C6EC, 0x089C0AECu, f_088D3278, 0x089C0AFCu,
                             5, f_0886D2EC, 0x089C0B1Cu };
    cmd_special(c, &k);
}
SFA_FUNC(089C0F04, cmd_special_0f04) {
    static const CmdSp k = { 0x360, f_088D3014, 0x089C0F28u, 386, T_CLEAR, f_0886BF84, 0x089C0F48u, f_088D3488, 0x089C0F58u,
                             2, f_0886DBE8, 0x089C0F78u };
    cmd_special(c, &k);
}
SFA_FUNC(089C7594, cmd_special_7594) {
    static const CmdSp k = { 0x390, f_088D304C, 0x089C75B8u, 386, T_SET, f_0886C6EC, 0x089C75D8u, f_088D3278, 0x089C75E8u,
                             8, f_0886D3E0, 0x089C7608u };
    cmd_special(c, &k);
}


void f_08868A34(Cpu *c); void f_08868220(Cpu *c);

/* (fighter, command): the command check over 0x08A1BD4C for entry 0x27. */
SFA_FUNC(088D34D8, command_27) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    ARG(2) = 0x27; c->r[31] = 0x088D34E8u; f_08868A34(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): +828 = 1 when command 0x20 of the list at +952 (checked over 0x08A1BD30) comes in. */
SFA_FUNC(089C0980, command_20_flag) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    ARG(1) = o + 0x3B8; ARG(2) = 0x20; c->r[31] = 0x089C099Cu; f_08868220(c);
    if (c->r[2] != 0) *W(o + 828) = 1;
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

void f_0886C2B4(Cpu *c); void f_088D2FC0(Cpu *c); void f_08869C88(Cpu *c); void f_08869E20(Cpu *c);

/* in the air only (+73 set), no facing */
SFA_FUNC(089B97DC, cmd_special_97dc) {
    static const CmdSp k = { 0x360, f_088D2FC0, 0x089B9800u, 73, T_CLEAR, f_0886C2B4, 0x089B9820u, f_088D3488,
                             0x089B9830u, 2, f_0886DB94, 0x089B9848u, 1 };
    cmd_special(c, &k);
}

/* (fighter, n): the held-all-kicks check (both pads). */
SFA_FUNC(088D3084, held_kicks_both) {
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    c->r[31] = 0x088D3094u; f_08869E20(c);
    c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): punches or kicks held by +627, the other way round for character 0x11. */
SFA_FUNC(0894CB54, held_by_627) {
    gaddr o = ARG(0);
    uint32_t ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = ra;
    int32_t k = S8(o + 627);
    ARG(1) = (uint32_t)k;
    if (S8(o + 338) == 0x11) {
        if (k != 0) { c->r[31] = 0x0894CB7Cu; f_08869C88(c); } else { c->r[31] = 0x0894CB8Cu; f_08869E20(c); }
    } else {
        if (k != 0) { c->r[31] = 0x0894CBA8u; f_08869E20(c); } else { c->r[31] = 0x0894CBB8u; f_08869C88(c); }
        c->r[4] = c->r[2];
    }
    c->r[31] = ra; c->r[29] += 0x20;
}


void f_088D3084(Cpu *c); void f_08867708(Cpu *c);

/* (fighter, n): both pads' kicks held, a positive +112, the condition 0x0886BF84 and the preparation
   0x088D3488 make it special 4 (facing the opponent, +235 = 1) without a start; 1 when it does. */
SFA_FUNC(089B9AC4, held_special_4) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    uint32_t ok = 0;
    c->r[31] = 0x089B9ADCu; f_088D3084(c);
    c->r[17] = 1;
    if (c->r[2] == 1 && S16(o + 112) > 0) {
        ARG(0) = o; c->r[31] = 0x089B9AFCu; f_0886BF84(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089B9B0Cu; f_088D3488(c);
            *B(o + 236) = 4; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter, n): with a choice made, both pads' kicks held and the condition (and without +251 a clear
   +542) special 0xE (facing, +235 cleared) without a start; 1 when it does. */
SFA_FUNC(089C785C, held_special_e) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[16] = o;
    uint32_t ok = 0;
    c->r[31] = 0x089C7874u; f_088D3084(c);
    c->r[17] = 1;
    if (c->r[2] == 1 && S8(o + 386) != 0) {
        ARG(0) = o; c->r[31] = 0x089C7894u; f_0886BF84(c);
        if (c->r[2] != 0 && (S8(o + 251) != 0 || S8(o + 542) == 0)) {
            ARG(0) = o; c->r[31] = 0x089C78BCu; f_088D3488(c);
            *B(o + 236) = 0xE; *B(o + 11) = *B(o + 697); *B(o + 235) = 0;
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): command 1 of the list at +888 (with +895 = 1); when it comes in +824 = -1, the list put back
   from its copy at +904, +895 = 1 and the start 0x0886DB94. The result is the check's or the start's. */
SFA_FUNC(089A03AC, command1_restart) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = s2;
    c->r[16] = o; c->r[18] = 1;
    gaddr list = o + 0x378;
    c->r[17] = list;
    *B(o + 895) = 1;
    *W(sp + 28) = ra;
    ARG(1) = list; ARG(2) = 1; c->r[31] = 0x089A03DCu; f_08867708(c);
    if (c->r[2] != 0) {
        *B(o + 0x334 + 4) = 0xFF;
        gaddr from = o + 0x388;
        for (int i = 0; i < 8; i++) *B(list + i) = *B(from + i);
        *B(o + 895) = 1;
        ARG(0) = o; ARG(1) = *B(from + 6); ARG(2) = *B(from + 4); ARG(3) = *B(from + 5);
        c->r[31] = 0x089A043Cu; f_0886DB94(c);
    }
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): command 3 at +864; with the condition and the preparation special 2 (facing, +235 / +193
   cleared) through 0x0886DB94; 1 when it starts. */
SFA_FUNC(089C76FC, cmd_special_76fc) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr cmd = o + 0x360;
    c->r[18] = cmd;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = cmd; c->r[31] = 0x089C7720u; f_088D2FF8(c);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        ARG(0) = o; c->r[31] = 0x089C7734u; f_0886BF84(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089C7744u; f_088D3488(c);
            *B(o + 236) = 2; *B(o + 11) = *B(o + 697); *B(o + 235) = 0; *B(o + 193) = 0;
            ARG(0) = o; ARG(1) = cmd; c->r[31] = 0x089C7768u; f_0886DB94(c);
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


void f_089B5104(Cpu *c); void f_088D2F50(Cpu *c); void f_088D3030(Cpu *c); void f_088D32C8(Cpu *c);
void f_088D35B8(Cpu *c);

/* a super started from a command (s2 = the command): the command, the choice test (`gt0`: above 0 stops
   it, else any choice), the gauge, 0x089B5104, the preparation 0x088D3278, the number, facing, +235 = 1
   and the start 0x0886D2EC. */
static void cmd_super_5104(Cpu *c, uint32_t off, void (*cmd)(Cpu *), int gt0, uint8_t num, const uint32_t b[5]) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + off;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = b[0]; cmd(c);
    c->r[17] = 1;
    int32_t ch;
    if (c->r[2] == 1 && (ch = S8(o + 386), gt0 ? ch <= 0 : ch == 0)) {
        ARG(0) = o; c->r[31] = b[1]; f_0886C6EC(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = b[2]; f_089B5104(c);
            ARG(0) = o; c->r[31] = b[3]; f_088D3278(c);
            *B(o + 236) = num; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
            ARG(0) = o; ARG(1) = q; c->r[31] = b[4]; f_0886D2EC(c);
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
SFA_FUNC(089B5238, cmd_super_6) {
    static const uint32_t b[5] = { 0x089B525Cu, 0x089B527Cu, 0x089B528Cu, 0x089B5294u, 0x089B52B4u };
    cmd_super_5104(c, 0x380, f_088D2F50, 0, 6, b);
}
SFA_FUNC(089B52EC, cmd_super_5) {
    static const uint32_t b[5] = { 0x089B5310u, 0x089B5330u, 0x089B5340u, 0x089B5348u, 0x089B5368u };
    cmd_super_5104(c, 0x378, f_088D3030, 1, 5, b);
}

/* (fighter): command 3 at +936 in the air (+73) with +238 set, the condition 0x0886C2B4 and the
   preparation: special 3 (+235 = 1, +193 cleared, no facing) through 0x0886DB94; 1 when it starts. */
SFA_FUNC(089B9A10, cmd_air_special_3) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x3A8;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089B9A34u; f_088D2FF8(c);
    c->r[17] = 1;
    if (c->r[2] == 1 && S8(o + 73) != 0 && S8(o + 238) != 0) {
        ARG(0) = o; c->r[31] = 0x089B9A60u; f_0886C2B4(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089B9A70u; f_088D3488(c);
            *B(o + 236) = 3; *B(o + 235) = 1; *B(o + 193) = 0;
            ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B9A8Cu; f_0886DB94(c);
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): command 0x1A at +896 with no choice above 0, 0x088D32C8 and the gauge: super 6 (facing, +235 =
   1, strength 2) through 0x088D35B8 after the preparation 0x088D3278; 1 when it starts. */
SFA_FUNC(089C0B54, cmd_super_6_35b8) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    uint32_t ok = 0;
    ARG(1) = o + 0x380; c->r[31] = 0x089C0B70u; f_088D3030(c);
    c->r[17] = 1;
    if (c->r[2] == 1 && S8(o + 386) <= 0) {
        ARG(0) = o; c->r[31] = 0x089C0B90u; f_088D32C8(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089C0BA0u; f_0886C6EC(c);
            if (c->r[2] == 1) {
                ARG(0) = o; c->r[31] = 0x089C0BB0u; f_088D3278(c);
                uint32_t f = *B(o + 697);
                *B(o + 236) = 6; *B(o + 11) = (uint8_t)f; *B(o + 235) = 1;
                *B(o + 194) = 2; *B(o + 552) = 2;
                ARG(0) = o; ARG(1) = (uint32_t)S8(o + 697); c->r[31] = 0x089C0BD8u; f_088D35B8(c);
                ok = 1;
            }
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


void f_088D30A0(Cpu *c);

/* (fighter): command 3 at +872 with the condition and the preparation: special 3 (facing, +235 = 1), +820
   = 1 kept only when 0x088D30A0 says so, then the start 0x0886DB94; 1 when it starts. */
SFA_FUNC(089C0FB0, cmd_special_820) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x368;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089C0FD4u; f_088D2FF8(c);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        ARG(0) = o; c->r[31] = 0x089C0FE8u; f_0886BF84(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089C0FF8u; f_088D3488(c);
            *B(o + 236) = 3; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
            *W(o + 820) = 1;
            ARG(0) = o; c->r[31] = 0x089C1018u; f_088D30A0(c);
            if (c->r[2] == 0) *W(o + 820) = 0;
            ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C102Cu; f_0886DB94(c);
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): unless the choice is 1, with the gauge super 9 (+833 cleared, the super state, facing),
   turned into super 0xF (+833 = -1) when both pads' kicks are held and 0x092E1228 is 0x1C; started
   through 0x0886D3E0. The result (s2) is whether it started. */
SFA_FUNC(08983FFC, super_9_or_f) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 24) = s2; c->r[18] = 0;
    *W(sp + 20) = s1; *W(sp + 28) = ra; *W(sp + 16) = s0;
    c->r[16] = o;
    if (S8(o + 386) != 1) {
        ARG(0) = o; c->r[31] = 0x0898402Cu; f_0886C6EC(c);
        gaddr q = o + 0x33C;
        c->r[17] = q;
        if (c->r[2] != 0) {
            *B(q + 5) = 0;
            *B(o + 236) = 9;
            *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 8; *B(o + 7) = 0;
            c->r[18] = 1;
            *B(o + 11) = *B(o + 697);
            ARG(0) = o; ARG(1) = (uint32_t)S8(o + 697); c->r[31] = 0x0898406Cu; f_08869E20(c);
            if (c->r[2] != 0 && S16(0x092E1228u) == 0x1C) { *B(o + 236) = 0xF; *B(q + 5) = 0xFF; }
            ARG(0) = o; c->r[31] = 0x08984098u; f_0886D3E0(c);
        }
    }
    c->r[2] = c->r[18];
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): with the condition 0x0886BF84 the special state (+4..+7 = 1 / 0 / 7 / 0), facing, +833
   cleared; with both pads' kicks held and 0x092E1228 0x1C special 0xF at strength 2 without a start,
   else special 3 through 0x0886DBE8. 1 when it goes, else 0. */
SFA_FUNC(08984194, special_3_or_f) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; c->r[16] = o;
    *W(c->r[29] + 20) = s1; *W(c->r[29] + 24) = ra;
    c->r[17] = 0;
    ARG(0) = o; c->r[31] = 0x089841B0u; f_0886BF84(c);
    if (c->r[2] != 0) {
        *B(o + 4) = 1; *B(o + 5) = 0; *B(o + 6) = 7; *B(o + 7) = 0;
        *B(o + 11) = *B(o + 697);
        *B(o + 0x33C + 5) = 0;
        c->r[17] = 1;
        ARG(0) = o; c->r[31] = 0x089841E8u; f_08869E20(c);
        if (c->r[2] != 0 && S16(0x092E1228u) == 0x1C) { *B(o + 236) = 0xF; *B(o + 194) = 2; }
        else { *B(o + 236) = 3; ARG(0) = o; c->r[31] = 0x08984214u; f_0886DBE8(c); }
    }
    c->r[2] = c->r[17];
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter): command 1 at +848; free of projectiles (+536 clear, or fewer than 2 with +251), the condition
   and the preparation: special 0 (facing, +235 = 1) through 0x0886DB94; 1 when it starts. */
SFA_FUNC(089C7640, cmd_special_shots) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x350;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089C7664u; f_088D2FA4(c);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        uint32_t n = *B(o + 536);
        int free = S8(o + 251) != 0 ? (int32_t)n < 2 : n == 0;
        if (free) {
            ARG(0) = o; c->r[31] = 0x089C768Cu; f_0886BF84(c);
            if (c->r[2] == 1) {
                ARG(0) = o; c->r[31] = 0x089C769Cu; f_088D3488(c);
                *B(o + 236) = 0; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
                ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C76B8u; f_0886DB94(c);
                ok = 1;
            }
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): command 0x1C of the list at +896, no choice made, no projectile out and the gauge: super 6
   (facing, +235 = 1) through 0x0886D2EC after 0x088D3278; 1 when it starts. */
SFA_FUNC(089C77A0, cmd_super_6_1c) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x380;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; ARG(2) = 0x1C; c->r[31] = 0x089C77C8u; f_08867708(c);
    if (c->r[2] != 0 && S8(o + 386) == 0 && *B(o + 536) == 0) {
        ARG(0) = o; c->r[31] = 0x089C77F0u; f_0886C6EC(c);
        c->r[17] = 1;
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089C7804u; f_088D3278(c);
            *B(o + 236) = 6; *B(o + 11) = *B(o + 697); *B(o + 235) = 1;
            ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C7824u; f_0886D2EC(c);
            ok = 1;
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter, command): command 1 at +904; in the air at least 64 above the floor the air condition and
   special 7 (+235 = 1), on the ground the condition and special 3 (facing, +235 cleared, +193 = 1), both
   through 0x0886DBE8 after the preparation; 1 when it starts. */
SFA_FUNC(089C7360, cmd_special_air_or_ground) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x388;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089C7384u; f_088D2FDC(c);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        if (S8(o + 73) != 0) {
            if (!(S16(o + 22) < S16(o + 132) + 0x40)) {
                ARG(0) = o; c->r[31] = 0x089C73BCu; f_0886C2B4(c);
                if (c->r[2] == 1) {
                    ARG(0) = o; c->r[31] = 0x089C73CCu; f_088D3488(c);
                    *B(o + 236) = 7; *B(o + 235) = 1;
                    ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C73E4u; f_0886DBE8(c);
                    ok = 1;
                }
            }
        } else {
            ARG(0) = o; c->r[31] = 0x089C7408u; f_0886BF84(c);
            if (c->r[2] == 1) {
                ARG(0) = o; c->r[31] = 0x089C7418u; f_088D3488(c);
                *B(o + 236) = 3; *B(o + 11) = *B(o + 697); *B(o + 235) = 0; *B(o + 193) = 1;
                ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C743Cu; f_0886DBE8(c);
                ok = 1;
            }
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}


/* (fighter): command 2 at +856; straight after special 0xA (state 1 / 0 / 7 / 1) special 1 follows on
   without the condition (+235 / +78 = 1, no facing), else with the condition special 1 (facing, +235 = 1,
   +78 cleared); both through 0x0886DBE8 after the preparation; 1 when it starts. */
SFA_FUNC(089B9614, cmd_special_chain_1) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x358;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089B9638u; f_088D3014(c);
    c->r[17] = 1;
    if (c->r[2] == 1) {
        if (*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7 && *B(o + 7) == 1 && S8(o + 236) == 0xA) {
            ARG(0) = o; c->r[31] = 0x089B96E4u; f_088D3488(c);
            *B(o + 236) = 1; *B(o + 235) = 1; *B(o + 78) = 1;
            ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B96FCu; f_0886DBE8(c);
            ok = 1;
        } else {
            ARG(0) = o; c->r[31] = 0x089B9690u; f_0886BF84(c);
            if (c->r[2] == 1) {
                ARG(0) = o; c->r[31] = 0x089B96A0u; f_088D3488(c);
                *B(o + 236) = 1; *B(o + 11) = *B(o + 697); *B(o + 235) = 1; *B(o + 78) = 0;
                ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B96C0u; f_0886DBE8(c);
                ok = 1;
            }
        }
    }
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}

/* (fighter): command 0x1D of the list at +920 with no choice above 0, no projectile, 0x088D32C8 and the
   gauge: super 9 (facing, +235 cleared, +552 = 2) through 0x0886D2EC after 0x088D3278, and with no choice
   made the strength by the punch held (+760 | +762 bits 6 / 5, else 0); 1 when it starts. A 0x30 frame
   keeping s0..s3. */
SFA_FUNC(089C7474, cmd_super_9_1d) {
    gaddr o = ARG(0);
    uint32_t sv[4] = { c->r[16], c->r[17], c->r[18], c->r[19] };
    uint32_t ra = c->r[31];
    c->r[29] -= 0x30;
    gaddr sp = c->r[29];
    *W(sp + 16) = sv[0]; c->r[16] = o;
    *W(sp + 20) = sv[1];
    gaddr q = o + 0x398;
    c->r[17] = q;
    *W(sp + 24) = sv[2]; *W(sp + 28) = sv[3]; *W(sp + 32) = ra;
    uint32_t ok = 0;
    ARG(1) = q; ARG(2) = 0x1D; c->r[31] = 0x089C74A0u; f_08867708(c);
    if (c->r[2] != 0 && S8(o + 386) <= 0 && *B(o + 536) == 0) {
        ARG(0) = o; c->r[31] = 0x089C74C8u; f_088D32C8(c);
        c->r[18] = 1;
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089C74DCu; f_0886C6EC(c);
            c->r[19] = q;
            if (c->r[2] == 1) {
                ARG(0) = o; c->r[31] = 0x089C74ECu; f_088D3278(c);
                *B(o + 236) = 9; *B(o + 11) = *B(o + 697);
                c->r[17] = 2;
                *B(o + 235) = 0; *B(o + 552) = 2;
                ARG(0) = o; ARG(1) = q; c->r[31] = 0x089C7514u; f_0886D2EC(c);
                if (S8(o + 386) < 0) {
                    uint32_t h = (*H(o + 760) | *H(o + 762)) & 0xFFFF;
                    *B(o + 194) = (h & 0x40) ? 2 : (h & 0x20) ? 1 : 0;
                }
                ok = 1;
            }
        }
    }
    c->r[2] = ok;
    c->r[16] = sv[0]; c->r[17] = sv[1]; c->r[18] = sv[2]; c->r[19] = sv[3];
    c->r[31] = ra; c->r[29] += 0x30;
}


/* (fighter): command 4 at +872; straight after special 1 (state 1 / 0 / 7 / 1) special 0xA follows on
   (+235 cleared, +78 = 1, no facing); else with +238 in the air the air condition gives special 3 (+235 /
   +193 = 1), failing that the condition special 0xA (facing, +235 / +78 cleared); all through 0x0886DBE8
   after the preparation; 1 when it starts. */
SFA_FUNC(089B9880, cmd_special_chain_a) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], s2 = c->r[18], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; c->r[16] = o;
    *W(sp + 24) = s2;
    gaddr q = o + 0x368;
    c->r[18] = q;
    *W(sp + 20) = s1; *W(sp + 28) = ra;
    uint32_t ok = 0;
    ARG(1) = q; c->r[31] = 0x089B98A4u; f_088D2FDC(c);
    c->r[17] = 1;
    if (c->r[2] != 1) goto out;
    if (*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7 && *B(o + 7) == 1 && S8(o + 236) == 1) {
        ARG(0) = o; c->r[31] = 0x089B990Cu; f_088D3488(c);
        *B(o + 236) = 0xA; *B(o + 235) = 0; *B(o + 78) = 1;
        ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B9928u; f_0886DBE8(c);
        ok = 1; goto out;
    }
    if (S8(o + 238) != 0 && S8(o + 73) != 0) {
        ARG(0) = o; c->r[31] = 0x089B9954u; f_0886C2B4(c);
        if (c->r[2] == 1) {
            ARG(0) = o; c->r[31] = 0x089B99BCu; f_088D3488(c);
            *B(o + 236) = 3; *B(o + 235) = 1; *B(o + 193) = 1;
            ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B99D8u; f_0886DBE8(c);
            ok = 1; goto out;
        }
    }
    ARG(0) = o; c->r[31] = 0x089B9964u; f_0886BF84(c);
    if (c->r[2] == 1) {
        ARG(0) = o; c->r[31] = 0x089B9974u; f_088D3488(c);
        *B(o + 236) = 0xA; *B(o + 11) = *B(o + 697); *B(o + 235) = 0; *B(o + 78) = 0;
        ARG(0) = o; ARG(1) = q; c->r[31] = 0x089B9998u; f_0886DBE8(c);
        ok = 1;
    }
out:
    c->r[2] = ok;
    c->r[16] = s0; c->r[17] = s1; c->r[18] = s2; c->r[31] = ra; c->r[29] += 0x20;
}
