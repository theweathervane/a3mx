/* More characters' move inputs, looked at every frame a fighter is free to act (see
   batch_inputs317.c for the plain ones): these chains have a turn in them: a different order of
   inputs for one form of the character, an input counted instead of checked, or a start that
   takes a number. Offsets stay numbers until something shows what they are.
 */
#include "step.h"
#include "../guest_mem.h"

void f_08867708(Cpu *c); void f_08868488(Cpu *c); void f_08868640(Cpu *c); void f_08868A34(Cpu *c);
void f_08869008(Cpu *c); void f_088694E0(Cpu *c); void f_088699FC(Cpu *c); void f_08869D9C(Cpu *c);
void f_08869E20(Cpu *c); void f_08869EB0(Cpu *c); void f_0886A1E4(Cpu *c); void f_0886A264(Cpu *c);
void f_0886A2D8(Cpu *c); void f_088EEC60(Cpu *c); void f_088EED14(Cpu *c); void f_088EED9C(Cpu *c);
void f_088EEEE4(Cpu *c); void f_088EEF64(Cpu *c); void f_088EF01C(Cpu *c); void f_088EF0D8(Cpu *c);
void f_088EF15C(Cpu *c); void f_088EF1F8(Cpu *c); void f_088EF288(Cpu *c); void f_088EF33C(Cpu *c);
void f_08914CB8(Cpu *c); void f_08914D38(Cpu *c); void f_08914DEC(Cpu *c); void f_08914E84(Cpu *c);
void f_08914F88(Cpu *c); void f_08915024(Cpu *c); void f_089150E0(Cpu *c); void f_08915168(Cpu *c);
void f_089207F4(Cpu *c); void f_08920918(Cpu *c); void f_089209A8(Cpu *c); void f_08920A38(Cpu *c);
void f_08920AB0(Cpu *c); void f_08920B34(Cpu *c); void f_08920BB4(Cpu *c); void f_08920C8C(Cpu *c);
void f_08920D64(Cpu *c); void f_08925D8C(Cpu *c); void f_08925E40(Cpu *c); void f_08925ED0(Cpu *c);
void f_08925F6C(Cpu *c); void f_08926100(Cpu *c); void f_08926198(Cpu *c); void f_08926250(Cpu *c);
void f_0892630C(Cpu *c); void f_08926390(Cpu *c); void f_08926430(Cpu *c); void f_089264D0(Cpu *c);
void f_0893D0E8(Cpu *c); void f_0893D1B0(Cpu *c); void f_0893D22C(Cpu *c); void f_0893D314(Cpu *c);
void f_0893D3C4(Cpu *c); void f_0893D478(Cpu *c); void f_0893D508(Cpu *c); void f_0893D5B8(Cpu *c);
void f_0893D720(Cpu *c); void f_0893D80C(Cpu *c); void f_0893D964(Cpu *c); void f_0893DA50(Cpu *c);
void f_0893DAF8(Cpu *c); void f_0894339C(Cpu *c); void f_089434B8(Cpu *c); void f_08943550(Cpu *c);
void f_089435D8(Cpu *c); void f_08943644(Cpu *c); void f_089436B0(Cpu *c); void f_0894371C(Cpu *c);
void f_08943780(Cpu *c); void f_089437E4(Cpu *c); void f_08943948(Cpu *c); void f_08949310(Cpu *c);
void f_089493A0(Cpu *c); void f_08949430(Cpu *c); void f_089494D4(Cpu *c); void f_08949568(Cpu *c);
void f_08949618(Cpu *c); void f_089496C8(Cpu *c); void f_08949778(Cpu *c); void f_0894984C(Cpu *c);
void f_089498F8(Cpu *c); void f_08949980(Cpu *c); void f_08949B5C(Cpu *c); void f_0894CB54(Cpu *c);
void f_08962C20(Cpu *c); void f_08962CD8(Cpu *c); void f_08962D60(Cpu *c); void f_08962EA8(Cpu *c);
void f_08962F28(Cpu *c); void f_08962FE0(Cpu *c); void f_0896309C(Cpu *c); void f_08963138(Cpu *c);
void f_089631C8(Cpu *c); void f_0896327C(Cpu *c); void f_0896331C(Cpu *c); void f_0896341C(Cpu *c);
void f_08963490(Cpu *c); void f_0896C4F8(Cpu *c); void f_0896C58C(Cpu *c); void f_0896C620(Cpu *c);
void f_0896C6C4(Cpu *c); void f_0896C7A0(Cpu *c); void f_0896C870(Cpu *c); void f_0896C958(Cpu *c);
void f_0896CAC8(Cpu *c); void f_0899EC38(Cpu *c); void f_0899ECE8(Cpu *c); void f_0899ED7C(Cpu *c);
void f_0899EEC4(Cpu *c); void f_0899EF58(Cpu *c); void f_0899EFE8(Cpu *c); void f_0899F084(Cpu *c);
void f_0899F0F8(Cpu *c); void f_0899F178(Cpu *c); void f_089A03AC(Cpu *c); void f_089B08D8(Cpu *c);
void f_089B09E0(Cpu *c); void f_089B0A50(Cpu *c); void f_089B0BD4(Cpu *c); void f_089B0C54(Cpu *c);
void f_089B0CD4(Cpu *c); void f_089B0D64(Cpu *c); void f_089B0DF4(Cpu *c); void f_089B0EA4(Cpu *c);
void f_089B0F2C(Cpu *c); void f_089B0FF4(Cpu *c); void f_089B108C(Cpu *c);

#define NONE 0xFFFFFFFFu

/* One step: `check` (fighter, fighter + off, code), where NONE leaves that argument as it was, and
   the move's `start` (fighter, arg) when it passes. */
typedef struct {
    void (*check)(Cpu *);
    uint32_t off, code, back_check;
    void (*start)(Cpu *);
    uint32_t arg, back_start;
} Check;

#define PLAIN(check, b1, start, b2) { check, NONE, NONE, b1, start, NONE, b2 }
#define INPUT(check, off, code, b1, start, b2) { check, off, code, b1, start, NONE, b2 }

/* The steps in order; returns 1 as soon as a start returns nonzero. */
static int run(Cpu *c, gaddr o, const Check *t, int n) {
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
        if (c->r[2] != 0) return 1;
    }
    return 0;
}

static void enter(Cpu *c, gaddr o, uint32_t sv[2]) {
    sv[0] = c->r[16]; sv[1] = c->r[31];
    c->r[29] -= 0x20;
    *W(c->r[29] + 16) = sv[0]; *W(c->r[29] + 20) = sv[1];
    c->r[16] = o;
}
static void leave(Cpu *c, const uint32_t sv[2]) { c->r[16] = sv[0]; c->r[31] = sv[1]; c->r[29] += 0x20; }
#define COUNT(t) ((int)(sizeof(t) / sizeof((t)[0])))

/* (fighter): three of the inputs come in another order while +251 is set. */
SFA_FUNC(088EEA20, move_inputs_eea20) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x088EEA34u, f_088EEC60, 0x088EEA44u),
        INPUT(f_08867708, 0x370, 0x1B, 0x088EEA5Cu, f_088EF1F8, 0x088EEA6Cu),
        INPUT(f_08867708, 0x368, 0x1C, 0x088EEA84u, f_088EF15C, 0x088EEA94u),
        INPUT(f_08867708, 0x388, 0x18, 0x088EEAACu, f_088EF33C, 0x088EEABCu),
    };
    static const Check free_order[] = {
        INPUT(f_08867708, 0x358, 0x5, 0x088EEAE0u, f_088EEEE4, 0x088EEAF0u),
        INPUT(f_08867708, 0x380, 0x11, 0x088EEB08u, f_088EF01C, 0x088EEB18u),
        INPUT(f_08867708, 0x350, 0x1, 0x088EEB30u, f_088EEF64, 0x088EEB40u),
    };
    static const Check held_order[] = {
        INPUT(f_08867708, 0x380, 0x11, 0x088EEB60u, f_088EF01C, 0x088EEB70u),
        INPUT(f_08867708, 0x350, 0x1, 0x088EEB88u, f_088EEF64, 0x088EEB98u),
        INPUT(f_08867708, 0x358, 0x5, 0x088EEBB0u, f_088EEEE4, 0x088EEBC0u),
    };
    static const Check last[] = {
        INPUT(f_08867708, 0x360, 0x4, 0x088EEBD8u, f_088EF288, 0x088EEBE8u),
        INPUT(f_08868640, 0x378, 0x22, 0x088EEC00u, f_088EF0D8, 0x088EEC10u),
        PLAIN(f_0886A264, 0x088EEC20u, f_088EED9C, 0x088EEC30u),
        PLAIN(f_0886A2D8, 0x088EEC40u, f_088EED14, 0x088EEC50u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        int ended = S8(o + 251) == 0 ? run(c, o, free_order, COUNT(free_order))
                                     : run(c, o, held_order, COUNT(held_order));
        if (!ended) run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): the same turn on +251. */
SFA_FUNC(08962990, move_inputs_62990) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x089629A4u, f_08962C20, 0x089629B4u),
        INPUT(f_08868488, 0x3A0, 0x21, 0x089629CCu, f_0896327C, 0x089629DCu),
        INPUT(f_08867708, 0x390, 0x15, 0x089629F4u, f_0896341C, 0x08962A04u),
        INPUT(f_08867708, 0x398, 0x16, 0x08962A1Cu, f_08963490, 0x08962A2Cu),
        INPUT(f_08867708, 0x370, 0x1B, 0x08962A44u, f_08963138, 0x08962A54u),
        INPUT(f_08867708, 0x368, 0x1C, 0x08962A6Cu, f_0896309C, 0x08962A7Cu),
        INPUT(f_08867708, 0x3A8, 0x18, 0x08962A94u, f_0896331C, 0x08962AA4u),
    };
    static const Check free_order[] = {
        INPUT(f_08867708, 0x358, 0x5, 0x08962AC8u, f_08962EA8, 0x08962AD8u),
        INPUT(f_08867708, 0x380, 0x11, 0x08962AF0u, f_08962FE0, 0x08962B00u),
        INPUT(f_08867708, 0x350, 0x1, 0x08962B18u, f_08962F28, 0x08962B28u),
    };
    static const Check held_order[] = {
        INPUT(f_08867708, 0x380, 0x11, 0x08962B48u, f_08962FE0, 0x08962B58u),
        INPUT(f_08867708, 0x350, 0x1, 0x08962B70u, f_08962F28, 0x08962B80u),
        INPUT(f_08867708, 0x358, 0x5, 0x08962B98u, f_08962EA8, 0x08962BA8u),
    };
    static const Check last[] = {
        INPUT(f_08867708, 0x360, 0x4, 0x08962BC0u, f_089631C8, 0x08962BD0u),
        PLAIN(f_0886A264, 0x08962BE0u, f_08962D60, 0x08962BF0u),
        PLAIN(f_0886A2D8, 0x08962C00u, f_08962CD8, 0x08962C10u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        int ended = S8(o + 251) == 0 ? run(c, o, free_order, COUNT(free_order))
                                     : run(c, o, held_order, COUNT(held_order));
        if (!ended) run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): on the ground +0x3B8 is cleared first; two of the inputs are different ones for a
   choice below 0. */
SFA_FUNC(0893CE6C, move_inputs_3ce6c) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x0893CE94u, f_0893D22C, 0x0893CEA4u),
        INPUT(f_08867708, 0x390, 0x1F, 0x0893CEBCu, f_0893D3C4, 0x0893CECCu),
        INPUT(f_08867708, 0x388, 0x18, 0x0893CEE4u, f_0893D478, 0x0893CEF4u),
        INPUT(f_08867708, 0x380, 0x1C, 0x0893CF0Cu, f_0893D314, 0x0893CF1Cu),
        INPUT(f_08867708, 0x398, 0x1D, 0x0893CF34u, f_0893D508, 0x0893CF44u),
        INPUT(f_08867708, 0x350, 0x15, 0x0893CF5Cu, f_0893D720, 0x0893CF6Cu),
        INPUT(f_08867708, 0x358, 0x16, 0x0893CF84u, f_0893D80C, 0x0893CF94u),
    };
    static const Check below[] = {
        INPUT(f_08867708, 0x3A0, 0x11, 0x0893CFB8u, f_0893DA50, 0x0893CFC8u),
        INPUT(f_08867708, 0x3A8, 0x12, 0x0893CFE0u, f_0893DAF8, 0x0893CFF0u),
    };
    static const Check others[] = {
        INPUT(f_08867708, 0x368, 0x13, 0x0893D010u, f_0893DA50, 0x0893D020u),
        INPUT(f_08867708, 0x370, 0x14, 0x0893D038u, f_0893DAF8, 0x0893D048u),
    };
    static const Check last[] = {
        INPUT(f_08867708, 0x360, 0x1, 0x0893D060u, f_0893D964, 0x0893D070u),
        INPUT(f_08867708, 0x378, 0xB, 0x0893D088u, f_0893D1B0, 0x0893D098u),
        PLAIN(f_0886A264, 0x0893D0A8u, f_0893D5B8, 0x0893D0B8u),
        PLAIN(f_0886A2D8, 0x0893D0C8u, f_0893D0E8, 0x0893D0D8u),
    };
    gaddr o = ARG(0);
    int32_t air = S8(o + 73);
    uint32_t sv[2];
    enter(c, o, sv);
    if (air == 0) *B(o + 0x3B8) = 0;
    if (!run(c, o, first, COUNT(first))) {
        int ended = S8(o + 386) < 0 ? run(c, o, below, COUNT(below)) : run(c, o, others, COUNT(others));
        if (!ended) run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): the input 0x28 starts 0x08943644 and ends the chain whatever that says; after it one
   input is 5 or, for a choice below 0, 9. */
SFA_FUNC(089431D8, move_inputs_431d8) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x089431ECu, f_0894339C, 0x089431FCu),
        INPUT(f_08868A34, 0x368, 0x27, 0x08943214u, f_089434B8, 0x08943224u),
        INPUT(f_08867708, 0x370, 0x18, 0x0894323Cu, f_08943550, 0x0894324Cu),
        INPUT(f_08868A34, 0x360, 0x26, 0x08943264u, f_089435D8, 0x08943274u),
    };
    static const Check others[] = { INPUT(f_08867708, 0x350, 0x5, 0x089432D0u, f_089436B0, 0x089432E0u) };
    static const Check below[] = { INPUT(f_08867708, 0x350, 0x9, 0x089432FCu, f_089436B0, 0x0894330Cu) };
    static const Check last[] = {
        PLAIN(f_0886A264, 0x0894331Cu, f_089437E4, 0x0894332Cu),
        PLAIN(f_08869D9C, 0x0894333Cu, f_0894371C, 0x0894334Cu),
        PLAIN(f_08869EB0, 0x0894335Cu, f_08943780, 0x0894336Cu),
        PLAIN(f_0886A2D8, 0x0894337Cu, f_08943948, 0x0894338Cu),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        ARG(1) = o + 0x358; ARG(0) = o; ARG(2) = 0x28;
        c->r[31] = 0x0894328Cu; f_08868A34(c);
        if (c->r[2] != 0) {
            ARG(0) = o;
            c->r[31] = 0x089432B0u; f_08943644(c);
        } else {
            int ended = S8(o + 386) < 0 ? run(c, o, below, 1) : run(c, o, others, 1);
            if (!ended) run(c, o, last, COUNT(last));
        }
    }
    leave(c, sv);
}

/* (fighter): four of the starts take a number. */
SFA_FUNC(0896C368, move_inputs_6c368) {
    static const Check t[] = {
        PLAIN(f_0886A1E4, 0x0896C37Cu, f_0896C870, 0x0896C38Cu),
        INPUT(f_08869008, 0x378, 0x31, 0x0896C3A4u, f_0896C6C4, 0x0896C3B4u),
        INPUT(f_08869008, 0x370, 0x35, 0x0896C3CCu, f_0896C620, 0x0896C3DCu),
        { f_088699FC, 0x358, 0x3E, 0x0896C3F4u, f_0896C4F8, 3, 0x0896C408u },
        { f_08869008, 0x350, 0x2C, 0x0896C420u, f_0896C4F8, 4, 0x0896C434u },
        { f_08869008, 0x360, 0x2D, 0x0896C44Cu, f_0896C58C, 6, 0x0896C460u },
        { f_08869008, 0x368, 0x2B, 0x0896C478u, f_0896C58C, 7, 0x0896C488u },
        PLAIN(f_0886A264, 0x0896C498u, f_0896C958, 0x0896C4A8u),
        PLAIN(f_08869E20, 0x0896C4B8u, f_0896C7A0, 0x0896C4C8u),
        PLAIN(f_0886A2D8, 0x0896C4D8u, f_0896CAC8, 0x0896C4E8u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    run(c, o, t, COUNT(t));
    leave(c, sv);
}

/* (fighter): five of the starts take a number. */
SFA_FUNC(0892062C, move_inputs_2062c) {
    static const Check t[] = {
        PLAIN(f_0886A1E4, 0x08920640u, f_089207F4, 0x08920650u),
        INPUT(f_08869008, 0x380, 0x32, 0x08920668u, f_08920C8C, 0x08920678u),
        INPUT(f_08869008, 0x378, 0x31, 0x08920690u, f_08920BB4, 0x089206A0u),
        { f_08867708, 0x368, 0x15, 0x089206B8u, f_08920B34, 0, 0x089206CCu },
        { f_08867708, 0x370, 0x16, 0x089206E4u, f_08920B34, 2, 0x089206F8u },
        { f_08869008, 0x390, 0x2A, 0x08920710u, f_089209A8, 6, 0x08920724u },
        { f_08869008, 0x360, 0x2D, 0x0892073Cu, f_08920918, 0, 0x08920750u },
        { f_08869008, 0x358, 0x2B, 0x08920768u, f_08920918, 1, 0x0892077Cu },
        INPUT(f_08869008, 0x350, 0x2C, 0x08920794u, f_08920AB0, 0x089207A4u),
        PLAIN(f_0886A264, 0x089207B4u, f_08920D64, 0x089207C4u),
        PLAIN(f_0886A2D8, 0x089207D4u, f_08920A38, 0x089207E4u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    run(c, o, t, COUNT(t));
    leave(c, sv);
}

/* (fighter): one input is 6 or, for a choice below 0, 0x10. */
SFA_FUNC(08925B9C, move_inputs_25b9c) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x08925BB0u, f_08925D8C, 0x08925BC0u),
        INPUT(f_08867708, 0x370, 0x18, 0x08925BD8u, f_08926430, 0x08925BE8u),
        INPUT(f_08867708, 0x378, 0x1C, 0x08925C00u, f_089264D0, 0x08925C10u),
        INPUT(f_08867708, 0x380, 0x1B, 0x08925C28u, f_08926390, 0x08925C38u),
        INPUT(f_08867708, 0x368, 0x5, 0x08925C50u, f_08926100, 0x08925C60u),
    };
    static const Check others[] = { INPUT(f_08867708, 0x360, 0x6, 0x08925C84u, f_0892630C, 0x08925C94u) };
    static const Check below[] = { INPUT(f_08867708, 0x360, 0x10, 0x08925CB4u, f_0892630C, 0x08925CC4u) };
    static const Check last[] = {
        INPUT(f_08867708, 0x350, 0x1, 0x08925CDCu, f_08926198, 0x08925CECu),
        INPUT(f_08867708, 0x358, 0x2, 0x08925D04u, f_08926250, 0x08925D14u),
        PLAIN(f_0886A264, 0x08925D24u, f_08925F6C, 0x08925D34u),
        INPUT(f_08868640, 0x388, 0x22, 0x08925D4Cu, f_08925E40, 0x08925D5Cu),
        PLAIN(f_0886A2D8, 0x08925D6Cu, f_08925ED0, 0x08925D7Cu),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        int ended = S8(o + 386) < 0 ? run(c, o, below, 1) : run(c, o, others, 1);
        if (!ended) run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): one check (0x0894CB54) is left out for the choice -1. */
SFA_FUNC(0894912C, move_inputs_4912c) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x08949140u, f_08949B5C, 0x08949150u),
        INPUT(f_08867708, 0x388, 0x1B, 0x08949168u, f_08949778, 0x08949178u),
        INPUT(f_08867708, 0x380, 0x18, 0x08949190u, f_089496C8, 0x089491A0u),
        INPUT(f_08867708, 0x378, 0x17, 0x089491B8u, f_08949618, 0x089491C8u),
        INPUT(f_08867708, 0x370, 0x1A, 0x089491E0u, f_08949568, 0x089491F0u),
        INPUT(f_08869008, 0x368, 0x2B, 0x08949208u, f_089494D4, 0x08949218u),
        INPUT(f_08869008, 0x360, 0x2C, 0x08949230u, f_08949430, 0x08949240u),
        INPUT(f_08867708, 0x358, 0x6, 0x08949258u, f_089493A0, 0x08949268u),
        INPUT(f_088699FC, 0x350, 0x3E, 0x08949280u, f_08949310, 0x08949290u),
    };
    static const Check extra[] = { PLAIN(f_0894CB54, 0x089492B0u, f_0894984C, 0x089492C0u) };
    static const Check last[] = {
        PLAIN(f_0886A2D8, 0x089492D0u, f_089498F8, 0x089492E0u),
        PLAIN(f_0886A264, 0x089492F0u, f_08949980, 0x08949300u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        int ended = S8(o + 386) != -1 ? run(c, o, extra, 1) : 0;
        if (!ended) run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): in state 1 / 0 / 7 with the move 6 running, 0x089A03AC runs between two inputs. */
SFA_FUNC(0899EA90, move_inputs_9ea90) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x0899EAA4u, f_0899EC38, 0x0899EAB4u),
        INPUT(f_08867708, 0x350, 0x1A, 0x0899EACCu, f_0899EEC4, 0x0899EADCu),
        INPUT(f_08867708, 0x358, 0x1B, 0x0899EAF4u, f_0899EF58, 0x0899EB04u),
        INPUT(f_08867708, 0x360, 0x17, 0x0899EB1Cu, f_0899EFE8, 0x0899EB2Cu),
        INPUT(f_08867708, 0x368, 0x8, 0x0899EB44u, f_0899F084, 0x0899EB54u),
        INPUT(f_08867708, 0x370, 0x10, 0x0899EB6Cu, f_0899F0F8, 0x0899EB7Cu),
    };
    static const Check last[] = {
        INPUT(f_08867708, 0x378, 0x1, 0x0899EBD8u, f_0899F178, 0x0899EBE8u),
        PLAIN(f_0886A264, 0x0899EBF8u, f_0899ED7C, 0x0899EC08u),
        PLAIN(f_0886A2D8, 0x0899EC18u, f_0899ECE8, 0x0899EC28u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (!run(c, o, first, COUNT(first))) {
        if (*B(o + 4) == 1 && *B(o + 5) == 0 && *B(o + 6) == 7 && S8(o + 236) == 6) {
            ARG(0) = o;
            c->r[31] = 0x0899EBC8u; f_089A03AC(c);
        }
        run(c, o, last, COUNT(last));
    }
    leave(c, sv);
}

/* (fighter): eight inputs are counted (0x088694E0 says how many presses): any count starts
   0x08914D38 with one less than it and the input's own number. */
SFA_FUNC(08917CD0, move_inputs_17cd0) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x08917CE4u, f_08915024, 0x08917CF4u),
        INPUT(f_08869008, 0x350, 0x31, 0x08917D0Cu, f_08914F88, 0x08917D1Cu),
        INPUT(f_08867708, 0x358, 0x19, 0x08917D34u, f_08914E84, 0x08917D44u),
        INPUT(f_08868A34, 0x360, 0x29, 0x08917D5Cu, f_08914DEC, 0x08917D6Cu),
        INPUT(f_08869008, 0x368, 0x2C, 0x08917D84u, f_08914CB8, 0x08917D94u),
    };
    static const struct { uint32_t off, code, back_check, number, back_start; } counted[8] = {
        { 0x370, 0x39, 0x08917DACu, 3, 0x08917DD4u }, { 0x378, 0x36, 0x08917DECu, 0, 0x08917E14u },
        { 0x380, 0x37, 0x08917E2Cu, 1, 0x08917E54u }, { 0x388, 0x38, 0x08917E6Cu, 2, 0x08917E94u },
        { 0x390, 0x3D, 0x08917EACu, 3, 0x08917ED4u }, { 0x398, 0x3A, 0x08917EECu, 0, 0x08917F14u },
        { 0x3A0, 0x3B, 0x08917F2Cu, 1, 0x08917F54u }, { 0x3A8, 0x3C, 0x08917F6Cu, 2, 0x08917F90u },
    };
    static const Check last[] = {
        PLAIN(f_0886A264, 0x08917FA0u, f_08915168, 0x08917FB0u),
        PLAIN(f_0886A2D8, 0x08917FC0u, f_089150E0, 0x08917FD0u),
    };
    gaddr o = ARG(0);
    uint32_t sv[2];
    enter(c, o, sv);
    if (run(c, o, first, COUNT(first))) goto out;
    for (int i = 0; i < 8; i++) {
        ARG(1) = o + counted[i].off; ARG(0) = o; ARG(2) = counted[i].code;
        c->r[31] = counted[i].back_check; f_088694E0(c);
        int32_t n = (int16_t)c->r[2];
        if (n == 0) continue;
        ARG(1) = (uint32_t)(int32_t)(int16_t)(n - 1); ARG(0) = o; ARG(2) = counted[i].number;
        c->r[31] = counted[i].back_start; f_08914D38(c);
        if (c->r[2] != 0) goto out;
    }
    run(c, o, last, COUNT(last));
out:
    leave(c, sv);
}

/* (fighter): eight inputs are counted and the highest count, if any, starts 0x089B0EA4 with one
   less than it. A frame keeping s0 and s1, the count. */
SFA_FUNC(089B3DB8, move_inputs_b3db8) {
    static const Check first[] = {
        PLAIN(f_0886A1E4, 0x089B3DD0u, f_089B08D8, 0x089B3DE0u),
        INPUT(f_08869008, 0x388, 0x33, 0x089B3DF8u, f_089B0F2C, 0x089B3E08u),
        INPUT(f_08869008, 0x390, 0x32, 0x089B3E20u, f_089B0FF4, 0x089B3E30u),
        INPUT(f_08869008, 0x398, 0x31, 0x089B3E48u, f_089B108C, 0x089B3E58u),
        INPUT(f_08869008, 0x370, 0x2A, 0x089B3E70u, f_089B0DF4, 0x089B3E80u),
        INPUT(f_08869008, 0x350, 0x2C, 0x089B3E98u, f_089B0BD4, 0x089B3EA8u),
        INPUT(f_08869008, 0x358, 0x2D, 0x089B3EC0u, f_089B0C54, 0x089B3ED0u),
        INPUT(f_08869008, 0x360, 0x2E, 0x089B3EE8u, f_089B0CD4, 0x089B3EF8u),
        INPUT(f_08869008, 0x368, 0x2F, 0x089B3F10u, f_089B0D64, 0x089B3F20u),
    };
    static const struct { uint32_t off, code, back; } counted[8] = {
        { 0x378, 0x39, 0x089B3F38u }, { 0x3A0, 0x36, 0x089B3F4Cu }, { 0x3A8, 0x37, 0x089B3F6Cu },
        { 0x3B0, 0x38, 0x089B3F8Cu }, { 0x380, 0x3D, 0x089B3FACu }, { 0x3B8, 0x3A, 0x089B3FCCu },
        { 0x3C0, 0x3B, 0x089B3FECu }, { 0x3C8, 0x3C, 0x089B400Cu },
    };
    static const Check last[] = {
        PLAIN(f_0886A264, 0x089B403Cu, f_089B0A50, 0x089B404Cu),
        PLAIN(f_0886A2D8, 0x089B405Cu, f_089B09E0, 0x089B406Cu),
    };
    gaddr o = ARG(0);
    uint32_t s0 = c->r[16], s1 = c->r[17], ra = c->r[31];
    c->r[29] -= 0x20;
    gaddr sp = c->r[29];
    *W(sp + 16) = s0; *W(sp + 20) = s1; *W(sp + 24) = ra;
    c->r[16] = o;
    if (run(c, o, first, COUNT(first))) goto out;
    int32_t best = 0;
    for (int i = 0; i < 8; i++) {
        ARG(1) = o + counted[i].off; ARG(0) = o; ARG(2) = counted[i].code;
        c->r[31] = counted[i].back; f_088694E0(c);
        int32_t n = (int32_t)c->r[2];
        if (i == 0 || best < n) best = n;
        c->r[17] = (uint32_t)best;
    }
    if (best != 0) {
        ARG(1) = (uint32_t)(best - 1); ARG(0) = o;
        c->r[31] = 0x089B402Cu; f_089B0EA4(c);
        if (c->r[2] != 0) goto out;
    }
    run(c, o, last, COUNT(last));
out:
    c->r[16] = s0; c->r[17] = s1; c->r[31] = ra; c->r[29] += 0x20;
}
