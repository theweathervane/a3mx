/* Four more characters' move inputs (see batch_inputs317.c): three plain chains, and one whose
   starts answer the other way round. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867708(Cpu *c); void f_08868220(Cpu *c); void f_08868488(Cpu *c); void f_08868A34(Cpu *c);
void f_08869008(Cpu *c); void f_08869C88(Cpu *c); void f_08869E20(Cpu *c); void f_0886A1E4(Cpu *c);
void f_0886A264(Cpu *c); void f_0886A2D8(Cpu *c); void f_08934F70(Cpu *c); void f_08934FCC(Cpu *c);
void f_08935064(Cpu *c); void f_089350E4(Cpu *c); void f_089351BC(Cpu *c); void f_089351EC(Cpu *c);
void f_089352B4(Cpu *c); void f_0893537C(Cpu *c); void f_08935430(Cpu *c); void f_089354E4(Cpu *c);
void f_08935598(Cpu *c); void f_08935630(Cpu *c); void f_08935768(Cpu *c); void f_08935818(Cpu *c);
void f_08983E64(Cpu *c); void f_08983EFC(Cpu *c); void f_08983F74(Cpu *c); void f_08983FFC(Cpu *c);
void f_089840B4(Cpu *c); void f_08984128(Cpu *c); void f_08984194(Cpu *c); void f_08984250(Cpu *c);
void f_089842B8(Cpu *c); void f_0898431C(Cpu *c); void f_089843F8(Cpu *c); void f_0898454C(Cpu *c);
void f_0898E978(Cpu *c); void f_0898EA04(Cpu *c); void f_0898EAC4(Cpu *c); void f_0898EB44(Cpu *c);
void f_0898EBC8(Cpu *c); void f_0898EC9C(Cpu *c); void f_0898ED20(Cpu *c); void f_0898EDC0(Cpu *c);
void f_0898EE5C(Cpu *c); void f_0898EEFC(Cpu *c); void f_0898F02C(Cpu *c); void f_08994144(Cpu *c);
void f_089941D0(Cpu *c); void f_08994290(Cpu *c); void f_08994314(Cpu *c); void f_089943C4(Cpu *c);
void f_08994448(Cpu *c); void f_089944C0(Cpu *c); void f_089945AC(Cpu *c); void f_08994630(Cpu *c);
void f_089946D0(Cpu *c); void f_0899476C(Cpu *c); void f_0899489C(Cpu *c); void f_0899493C(Cpu *c);

#define NONE 0xFFFFFFFFu

/* One step: `check` (fighter, fighter + off, code), where NONE leaves that argument as it was, and
   the move's `start` (fighter) when it passes. */
typedef struct {
    void (*check)(Cpu *);
    uint32_t off, code, back_check;
    void (*start)(Cpu *);
    uint32_t back_start;
} Check;

/* (fighter): the steps in order; a start that returns nonzero ends them. The last start's answer
   is not looked at. A 0x20 frame keeping s0. */
static void run_checks(Cpu *c, const Check *t, int n) {
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    for (int i = 0; i < n; i++) {
        if (t[i].off != NONE) ARG(1) = o + t[i].off;
        ARG(0) = o;
        if (t[i].code != NONE) ARG(2) = t[i].code;
        c->r[31] = t[i].back_check;
        t[i].check(c);
        if (c->r[2] == 0) continue;
        ARG(0) = o;
        c->r[31] = t[i].back_start;
        t[i].start(c);
        if (c->r[2] != 0) break;
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}

SFA_FUNC(0898E7CC, move_inputs_8e7cc) {
    static const Check t[11] = {
        { f_0886A1E4, NONE, NONE, 0x0898E7E0u, f_0898EA04, 0x0898E7F0u },
        { f_08868488, 0x350, 0x21, 0x0898E808u, f_0898EDC0, 0x0898E818u },
        { f_08867708, 0x358, 0x1B, 0x0898E830u, f_0898EE5C, 0x0898E840u },
        { f_08867708, 0x360, 0x18, 0x0898E858u, f_0898ED20, 0x0898E868u },
        { f_08867708, 0x368, 0x17, 0x0898E880u, f_0898EEFC, 0x0898E890u },
        { f_08867708, 0x370, 0x6, 0x0898E8A8u, f_0898EB44, 0x0898E8B8u },
        { f_08867708, 0x378, 0x13, 0x0898E8D0u, f_0898EBC8, 0x0898E8E0u },
        { f_08867708, 0x380, 0x2, 0x0898E8F8u, f_0898EC9C, 0x0898E908u },
        { f_0886A264, NONE, NONE, 0x0898E918u, f_0898F02C, 0x0898E928u },
        { f_08869C88, NONE, NONE, 0x0898E938u, f_0898EAC4, 0x0898E948u },
        { f_0886A2D8, NONE, NONE, 0x0898E958u, f_0898E978, 0x0898E968u },
    };
    run_checks(c, t, 11);
}
SFA_FUNC(08983C98, move_inputs_83c98) {
    static const Check t[12] = {
        { f_0886A1E4, NONE, NONE, 0x08983CACu, f_0898431C, 0x08983CBCu },
        { f_08869008, 0x368, 0x35, 0x08983CD4u, f_08983FFC, 0x08983CE4u },
        { f_08869008, 0x378, 0x32, 0x08983CFCu, f_08983EFC, 0x08983D0Cu },
        { f_08869008, 0x380, 0x31, 0x08983D24u, f_08983E64, 0x08983D34u },
        { f_08869008, 0x358, 0x2B, 0x08983D4Cu, f_08984194, 0x08983D5Cu },
        { f_08869008, 0x370, 0x30, 0x08983D74u, f_08983F74, 0x08983D84u },
        { f_08869008, 0x350, 0x2C, 0x08983D9Cu, f_089840B4, 0x08983DACu },
        { f_08869008, 0x360, 0x2A, 0x08983DC4u, f_08984128, 0x08983DD4u },
        { f_08869C88, NONE, NONE, 0x08983DE4u, f_089842B8, 0x08983DF4u },
        { f_08869E20, NONE, NONE, 0x08983E04u, f_08984250, 0x08983E14u },
        { f_0886A264, NONE, NONE, 0x08983E24u, f_089843F8, 0x08983E34u },
        { f_0886A2D8, NONE, NONE, 0x08983E44u, f_0898454C, 0x08983E54u },
    };
    run_checks(c, t, 12);
}
SFA_FUNC(08993F48, move_inputs_93f48) {
    static const Check t[13] = {
        { f_0886A1E4, NONE, NONE, 0x08993F5Cu, f_089941D0, 0x08993F6Cu },
        { f_08868488, 0x350, 0x21, 0x08993F84u, f_089946D0, 0x08993F94u },
        { f_08867708, 0x358, 0x17, 0x08993FACu, f_0899476C, 0x08993FBCu },
        { f_08868A34, 0x360, 0x26, 0x08993FD4u, f_089945AC, 0x08993FE4u },
        { f_08869008, 0x368, 0x31, 0x08993FFCu, f_0899489C, 0x0899400Cu },
        { f_08869008, 0x370, 0x35, 0x08994024u, f_08994630, 0x08994034u },
        { f_08867708, 0x378, 0xE, 0x0899404Cu, f_089943C4, 0x0899405Cu },
        { f_08867708, 0x380, 0x2, 0x08994074u, f_089944C0, 0x08994084u },
        { f_08869008, 0x388, 0x2B, 0x0899409Cu, f_08994290, 0x089940ACu },
        { f_08869008, 0x390, 0x2D, 0x089940C4u, f_08994314, 0x089940D4u },
        { f_0886A264, NONE, NONE, 0x089940E4u, f_0899493C, 0x089940F4u },
        { f_08869E20, NONE, NONE, 0x08994104u, f_08994448, 0x08994114u },
        { f_0886A2D8, NONE, NONE, 0x08994124u, f_08994144, 0x08994134u },
    };
    run_checks(c, t, 13);
}

/* The same, for a character whose starts answer the other way round (0 when the move started),
   and three of whose starts take a number. A choice below 0 leaves two of the steps out. */
typedef struct {
    void (*check)(Cpu *);
    uint32_t off, code, back_check;
    void (*start)(Cpu *);
    uint32_t arg, back_start;
} CheckArg;

/* The steps in order; returns 1 as soon as a start returns 0. */
static int run_until_zero(Cpu *c, gaddr o, const CheckArg *t, int n) {
    for (int i = 0; i < n; i++) {
        if (t[i].off != NONE) ARG(1) = o + t[i].off;
        ARG(0) = o;
        if (t[i].code != NONE) ARG(2) = t[i].code;
        c->r[31] = t[i].back_check;
        t[i].check(c);
        if (c->r[2] == 0) continue;
        ARG(0) = o;
        if (t[i].arg != NONE) ARG(1) = t[i].arg;
        c->r[31] = t[i].back_start;
        t[i].start(c);
        if (c->r[2] == 0) return 1;
    }
    return 0;
}

SFA_FUNC(08934D48, move_inputs_34d48) {
    static const CheckArg first[] = {
        { f_0886A1E4, NONE, NONE, 0x08934D5Cu, f_08935818, NONE, 0x08934D6Cu },
        { f_08867708, 0x378, 0x1C, 0x08934D84u, f_08935430, NONE, 0x08934D94u },
        { f_08867708, 0x370, 0x1D, 0x08934DACu, f_0893537C, NONE, 0x08934DBCu },
        { f_08867708, 0x368, 0x18, 0x08934DD4u, f_089354E4, NONE, 0x08934DE4u },
        { f_08867708, 0x350, 0x6, 0x08934DFCu, f_08934F70, NONE, 0x08934E0Cu },
        { f_08867708, 0x358, 0x4, 0x08934E24u, f_08934FCC, 1, 0x08934E38u },
        { f_08867708, 0x380, 0x1, 0x08934E50u, f_08935598, 7, 0x08934E64u },
        { f_08867708, 0x388, 0x3, 0x08934E7Cu, f_08935598, 0xC, 0x08934E8Cu },
        { f_08869C88, NONE, NONE, 0x08934E9Cu, f_08935064, NONE, 0x08934EACu },
        { f_08869E20, NONE, NONE, 0x08934EBCu, f_089350E4, NONE, 0x08934ECCu },
    };
    static const CheckArg others[] = {
        { f_089351BC, NONE, NONE, 0x08934EE8u, f_089351EC, NONE, 0x08934EF8u },
        { f_08868220, 0x360, 0x20, 0x08934F10u, f_089352B4, NONE, 0x08934F20u },
    };
    static const CheckArg last[] = {
        { f_0886A264, NONE, NONE, 0x08934F30u, f_08935630, NONE, 0x08934F40u },
        { f_0886A2D8, NONE, NONE, 0x08934F50u, f_08935768, NONE, 0x08934F60u },
    };
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], ra = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = s0; *W(c->r[29] + 20) = ra;
    c->r[16] = o;
    if (!run_until_zero(c, o, first, 10)) {
        int ended = S8(o + 386) < 0 ? 0 : run_until_zero(c, o, others, 2);
        if (!ended) run_until_zero(c, o, last, 2);
    }
    c->r[16] = s0; c->r[31] = ra; c->r[29] += 0x20;
}
